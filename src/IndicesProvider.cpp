/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2025 THL A29 Limited, a Tencent company. All rights reserved.
//
//  Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
//  in compliance with the License. You may obtain a copy of the License at
//
//      https://opensource.org/licenses/BSD-3-Clause
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "IndicesProvider.h"
#include <ProcessUtils.h>
#include <qtypes.h>

namespace inspector {
static constexpr uint16_t VerticesPerNonAAQuad = 4;
static constexpr uint16_t VerticesPerAAQuad = 8;

// clang-format off
static constexpr uint16_t NonAAQuadIndexPattern[] = {
  0, 1, 2, 1, 3, 2,
};

static constexpr uint16_t AAQuadIndexPattern[] = {
  0, 1, 2, 1, 3, 2,
  0, 4, 1, 4, 5, 1,
  0, 6, 4, 0, 2, 6,
  2, 3, 6, 3, 7, 6,
  1, 5, 3, 3, 5, 7,
};

class RectIndicesProvider : public IndicesProvider {
 public:
  static constexpr uint16_t MaxNumRects = 2048;

  static constexpr uint16_t IndicesPerNonAAQuad = 6;

  static constexpr uint16_t IndicesPerAAQuad = 30;

  RectIndicesProvider(const uint16_t* pattern, uint16_t patternSize, uint16_t reps,
                      uint16_t vertCount)
      : pattern(pattern), patternSize(patternSize), reps(reps), vertCount(vertCount) {
  }

  std::vector<uint16_t> indices() override;

 private:
  const uint16_t* pattern = nullptr;
  uint16_t patternSize = 0;
  uint16_t reps = 0;
  uint16_t vertCount = 0;
};

std::vector<uint16_t> RectIndicesProvider::indices() {
  auto size = reps * patternSize;
  std::vector<uint16_t> resultIndices = {};
  resultIndices.resize(static_cast<size_t>(size));
  for (uint16_t i = 0; i < reps; ++i) {
    uint16_t baseIdx = i * patternSize;
    auto baseVert = static_cast<uint16_t>(i * vertCount);
    for (uint16_t j = 0; j < patternSize; ++j) {
      resultIndices[baseIdx + j] = baseVert + pattern[j];
    }
  }

  return resultIndices;
}

// clang-format off
static const uint16_t OverstrokeRRectIndices[] = {
  // overstroke quads
  // we place this at the beginning so that we can skip these indices when rendering normally
  16, 17, 19, 16, 19, 18,
  19, 17, 23, 19, 23, 21,
  21, 23, 22, 21, 22, 20,
  22, 16, 18, 22, 18, 20,

  // corners
  0, 1, 5, 0, 5, 4,
  2, 3, 7, 2, 7, 6,
  8, 9, 13, 8, 13, 12,
  10, 11, 15, 10, 15, 14,

  // edges
  1, 2, 6, 1, 6, 5,
  4, 5, 9, 4, 9, 8,
  6, 7, 11, 6, 11, 10,
  9, 10, 14, 9, 14, 13,

  // center
  // we place this at the end so that we can ignore these indices when not rendering as filled
  5, 6, 10, 5, 10, 9,
};
// clang-format on

static constexpr int OverstrokeIndicesCount = 6 * 4;
// fill and standard stroke indices skip the overstroke "ring"
static const uint16_t* StandardRRectIndices = OverstrokeRRectIndices + OverstrokeIndicesCount;

class RRectIndicesProvider : public IndicesProvider {
 public:
  static constexpr uint16_t MaxNumRRects = 1024;

  static constexpr uint16_t IndicesPerFillRRect = 54;

  static constexpr uint16_t IndicesPerStrokeRRect = 48;

  explicit RRectIndicesProvider(size_t rectSize, bool stroke) : rectSize(rectSize), stroke(stroke) {
  }

  std::vector<uint16_t> indices() override;

 private:
  size_t rectSize = 0;
  bool stroke = false;
};

std::vector<uint16_t> RRectIndicesProvider::indices() {
  auto indicesCount = stroke ? IndicesPerStrokeRRect : IndicesPerFillRRect;
  auto indicesSize = rectSize * indicesCount;
  std::vector<uint16_t> resultIndices = {};
  resultIndices.resize(indicesSize);
  int index = 0;
  for (size_t i = 0; i < rectSize; ++i) {
    auto offset = static_cast<uint16_t>(i * 16);
    for (size_t j = 0; j < indicesCount; ++j) {
      resultIndices[static_cast<size_t>(index++)] = StandardRRectIndices[j] + offset;
    }
  }
  return resultIndices;
}

std::shared_ptr<IndicesProvider> IndicesProvider::MakeFrom(
    tgfx::inspect::OpTaskType opTaskType, tgfx::inspect::VertexProviderType type,
    const std::shared_ptr<tgfx::inspect::MeshInfo>& meshInfo) {
  if (opTaskType == tgfx::inspect::OpTaskType::ShapeDrawOp) {
    return std::make_shared<IndicesProvider>();
  }
  if (type == tgfx::inspect::VertexProviderType::RRectsVertexProvider) {
    auto rrectMeshInfo = std::static_pointer_cast<tgfx::inspect::RRectMeshInfo>(meshInfo);
    return std::make_shared<RRectIndicesProvider>(meshInfo->rectCount, rrectMeshInfo->hasStroke);
  }

  auto rectMeshInfo = std::static_pointer_cast<tgfx::inspect::RectMeshInfo>(meshInfo);
  if (rectMeshInfo->aaType == 1) {
    return std::make_shared<RectIndicesProvider>(AAQuadIndexPattern,
                                                 RectIndicesProvider::IndicesPerAAQuad,
                                                 meshInfo->rectCount, VerticesPerAAQuad);
  }
  return std::make_shared<RectIndicesProvider>(NonAAQuadIndexPattern,
                                               RectIndicesProvider::IndicesPerNonAAQuad,
                                               meshInfo->rectCount, VerticesPerNonAAQuad);
}

}  // namespace inspector