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

#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "Protocol.h"
#include "tgfx/core/Data.h"
#include "tgfx/core/Pixmap.h"
#include "tgfx/gpu/PixelFormat.h"

namespace inspector {

#define SPATIAL_PRECISION 0.05f
#define BEZIER_PRECISION 0.005f
#define GRADIENT_PRECISION 0.00002f

struct StringLocation {
  const char* ptr;
  uint32_t idx;
};

struct FrameEvent {
  bool captured = false;
  int64_t start = 0;
  int64_t end = -1;
  int64_t drawCall = 0;
  int64_t triangles = 0;
  int32_t frameImage = 0;
};

struct FrameData {
  std::vector<FrameEvent> frames;
  uint8_t continuous = 1;
};

struct OpTaskData {
  int64_t start = 0;
  int64_t end = 0;
  uint32_t id = 0;
  uint8_t type = static_cast<uint8_t>(tgfx::debug::OpTaskType::Unknown);
};

static std::unordered_map<uint8_t, const char*> OpTaskName = {
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::Unknown), "Unknown"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::Flush), "Flush"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::ResourceTask), "ResourceTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::TextureUploadTask), "TextureUploadTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::ShapeBufferUploadTask), "ShapeBufferUploadTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::GpuUploadTask), "GpuUploadTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::TextureCreateTask), "TextureCreateTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::RenderTargetCreateTask),
     "RenderTargetCreateTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::TextureFlattenTask), "TextureFlattenTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::RenderTask), "RenderTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::RenderTargetCopyTask), "RenderTargetCopyTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::RuntimeDrawTask), "RuntimeDrawTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::TextureResolveTask), "TextureResolveTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::OpsRenderTask), "OpsRenderTask"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::ClearOp), "ClearOp"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::RectDrawOp), "RectDrawOp"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::RRectDrawOp), "RRectDrawOp"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::ShapeDrawOp), "ShapeDrawOp"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::DstTextureCopyOp), "DstTextureCopyOp"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::ResolveOp), "ResolveOp"},
    {static_cast<uint8_t>(tgfx::debug::OpTaskType::OpTaskTypeSize), "OpTaskTypeSize"},
};

static std::unordered_map<tgfx::PixelFormat, const char*> PixelFormatName = {
    {tgfx::PixelFormat::Unknown, "Unknown"},     {tgfx::PixelFormat::ALPHA_8, "ALPHA_8"},
    {tgfx::PixelFormat::GRAY_8, "GRAY_8"},       {tgfx::PixelFormat::RG_88, "RG_88"},
    {tgfx::PixelFormat::RGBA_8888, "RGBA_8888"}, {tgfx::PixelFormat::BGRA_8888, "BGRA_8888"},
};

enum class DataType : uint8_t { Color, Vec4, Mat4, Int, Uint32, Bool, Float, Enum, String, Count };
enum class OpOrTask : uint8_t { Op, Task, NoType };

static std::unordered_map<tgfx::debug::CustomEnumType, std::vector<std::string>> TGFXEnumName = {
    {tgfx::debug::CustomEnumType::BufferType, {"Index", "Vertex"}},
    {tgfx::debug::CustomEnumType::BlendMode,
     {"Clear",       "Src",       "Dst",        "SrcOver",   "DstOver",    "SrcIn",
      "DstIn",       "SrcOut",    "DstOut",     "SrcTop",    "DstTop",     "Xor",
      "PlusLighter", "Modulate",  "Screen",     "OverLay",   "Darken",     "Lighten",
      "ColorDodge",  "ColorBurn", "HardLight",  "SoftLight", "Difference", "Exclusion",
      "Multiply",    "Hue",       "Saturation", "Color",     "Luminosity", "PlusDarker"}},
    {tgfx::debug::CustomEnumType::AAType, {"None", "Coverage", "MSAA"}},
    {tgfx::debug::CustomEnumType::PixelFormat,
     {"Unknown", "ALPHA_8", "GRAY_8", "RG_88", "RGBA_8888", "BGRA_8888"}},
    {tgfx::debug::CustomEnumType::ImageOrigin, {"TopLeft", "BottomLeft"}},
};

struct DataHead {
  DataType type;
  uint64_t name;
};

struct PropertyData {
  std::vector<DataHead> summaryName;
  std::vector<DataHead> processName;
  std::vector<std::shared_ptr<tgfx::Data>> summaryData;
  std::vector<std::shared_ptr<tgfx::Data>> processData;
};

struct ImageTexture {
  bool isInput;
  uint8_t format;
  int width;
  int height;
  size_t rowBytes;
  std::shared_ptr<tgfx::Data> data;
};

struct TextureData {
  std::vector<uint64_t> inputTextures = {};
  uint64_t outputTexture = 0;
};

struct VertexData {
  std::vector<float> vertexData;
  bool hasUV;
  bool hasColor;
};

OpOrTask getOpTaskType(tgfx::debug::OpTaskType type);
}  // namespace inspector