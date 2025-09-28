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

#include "MeshDataDivider.h"

namespace inspector {
class RRectsMeshDataDivider : public MeshDataDivider {
 public:
  explicit RRectsMeshDataDivider(std::shared_ptr<tgfx::inspect::RRectMeshInfo> meshInfo);
};

class RectsMeshDataDivider : public MeshDataDivider {
 public:
  explicit RectsMeshDataDivider(std::shared_ptr<tgfx::inspect::RectMeshInfo> meshInfo);
};

class AtloasTextMeshDataDivider : public MeshDataDivider {
 public:
  explicit AtloasTextMeshDataDivider(std::shared_ptr<tgfx::inspect::RectMeshInfo> meshInfo);
};

RRectsMeshDataDivider::RRectsMeshDataDivider(
    std::shared_ptr<tgfx::inspect::RRectMeshInfo> meshInfo) {
  attributes.push_back({"inPosition", Format::Float2, UniformFormat::Position, 2});
  if (meshInfo->hasColor) {
    attributes.push_back({"inColor", Format::UByteNormalized, UniformFormat::Other, 4});
  }
  if (meshInfo->useScale) {
    attributes.push_back({"inEllipseOffset", Format::Float3, UniformFormat::Other, 3});
  } else {
    attributes.push_back({"inEllipseOffset", Format::Float2, UniformFormat::Other, 2});
  }
  attributes.push_back({"inEllipseRadii", Format::Float4, UniformFormat::Other, 4});
}

RectsMeshDataDivider::RectsMeshDataDivider(std::shared_ptr<tgfx::inspect::RectMeshInfo> meshInfo) {
  attributes.push_back({"aPostion", Format::Float2, UniformFormat::Position, 2});
  if (meshInfo->aaType == 1) {
    attributes.push_back({"inCoverage", Format::Float, UniformFormat::Other, 1});
  }
  if (meshInfo->hasUVCoord) {
    attributes.push_back({"uvCoord", Format::Float2, UniformFormat::Other, 2});
  }
  if (meshInfo->hasColor) {
    attributes.push_back({"inColor", Format::UByte4Normalized, UniformFormat::Other, 4});
  }
  if (meshInfo->hasSubset) {
    attributes.push_back({"textSubset", Format::Float4, UniformFormat::Other, 4});
  }
}

AtloasTextMeshDataDivider::AtloasTextMeshDataDivider(
    std::shared_ptr<tgfx::inspect::RectMeshInfo> meshInfo) {
  attributes.push_back({"aPostion", Format::Float2, UniformFormat::Position, 2});
  if (meshInfo->aaType == 1) {
    attributes.push_back({"inCoverage", Format::Float, UniformFormat::Other, 1});
  }
  attributes.push_back({"maskCoord", Format::Float2, UniformFormat::Other, 2});
  if (meshInfo->hasColor) {
    attributes.push_back({"inColor", Format::UByte4Normalized, UniformFormat::Other, 4});
  }
}

std::shared_ptr<MeshDataDivider> MeshDataDivider::MakeFrom(
    tgfx::inspect::OpTaskType opTaskType, tgfx::inspect::VertexProviderType type,
    const std::shared_ptr<tgfx::inspect::MeshInfo>& meshInfo) {
  if (type == tgfx::inspect::VertexProviderType::RRectsVertexProvider) {
    auto rrectMeshInfo = std::static_pointer_cast<tgfx::inspect::RRectMeshInfo>(meshInfo);
    return std::make_shared<RRectsMeshDataDivider>(rrectMeshInfo);
  }
  auto rectMeshInfo = std::static_pointer_cast<tgfx::inspect::RectMeshInfo>(meshInfo);
  if (opTaskType == tgfx::inspect::OpTaskType::AtlasTextOp) {
    return std::make_shared<AtloasTextMeshDataDivider>(rectMeshInfo);
  }
  return std::make_shared<RectsMeshDataDivider>(rectMeshInfo);
}
}  // namespace inspector