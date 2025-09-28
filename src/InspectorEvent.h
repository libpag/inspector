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
};

struct FrameData {
  std::vector<FrameEvent> frames;
  uint8_t continuous = 1;
};

struct OpTaskData {
  int64_t start = 0;
  int64_t end = 0;
  uint32_t id = 0;
  uint8_t type = static_cast<uint8_t>(tgfx::inspect::OpTaskType::Unknown);
  uint64_t ptr = 0;
};

static std::unordered_map<uint8_t, const char*> OpTaskName = {
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::Unknown), "Unknown"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::Flush), "Flush"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::ResourceTask), "ResourceTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::TextureUploadTask), "TextureUploadTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::ShapeBufferUploadTask),
     "ShapeBufferUploadTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::GpuUploadTask), "GpuUploadTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::TextureCreateTask), "TextureCreateTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::RenderTargetCreateTask),
     "RenderTargetCreateTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::TextureFlattenTask), "TextureFlattenTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::RenderTask), "RenderTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::RenderTargetCopyTask), "RenderTargetCopyTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::RuntimeDrawTask), "RuntimeDrawTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::TextureResolveTask), "TextureResolveTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::OpsRenderTask), "OpsRenderTask"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::ClearOp), "ClearOp"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::RectDrawOp), "RectDrawOp"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::RRectDrawOp), "RRectDrawOp"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::ShapeDrawOp), "ShapeDrawOp"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::AtlasTextOp), "AtlasTextOp"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::DstTextureCopyOp), "DstTextureCopyOp"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::ResolveOp), "ResolveOp"},
    {static_cast<uint8_t>(tgfx::inspect::OpTaskType::OpTaskTypeSize), "OpTaskTypeSize"},
};

static std::unordered_map<tgfx::PixelFormat, const char*> PixelFormatName = {
    {tgfx::PixelFormat::Unknown, "Unknown"},     {tgfx::PixelFormat::ALPHA_8, "ALPHA_8"},
    {tgfx::PixelFormat::GRAY_8, "GRAY_8"},       {tgfx::PixelFormat::RG_88, "RG_88"},
    {tgfx::PixelFormat::RGBA_8888, "RGBA_8888"}, {tgfx::PixelFormat::BGRA_8888, "BGRA_8888"},
};

enum class DataType : uint8_t { Color, Vec4, Mat4, Int, Uint32, Bool, Float, Enum, String, Count };
enum class OpOrTask : uint8_t { Op, Task, NoType };

static std::unordered_map<tgfx::inspect::CustomEnumType, std::vector<std::string>> TGFXEnumName = {
    {tgfx::inspect::CustomEnumType::BufferType, {"Index", "Vertex"}},
    {tgfx::inspect::CustomEnumType::BlendMode,
     {"Clear",       "Src",       "Dst",        "SrcOver",   "DstOver",    "SrcIn",
      "DstIn",       "SrcOut",    "DstOut",     "SrcTop",    "DstTop",     "Xor",
      "PlusLighter", "Modulate",  "Screen",     "OverLay",   "Darken",     "Lighten",
      "ColorDodge",  "ColorBurn", "HardLight",  "SoftLight", "Difference", "Exclusion",
      "Multiply",    "Hue",       "Saturation", "Color",     "Luminosity", "PlusDarker"}},
    {tgfx::inspect::CustomEnumType::AAType, {"None", "Coverage", "MSAA"}},
    {tgfx::inspect::CustomEnumType::PixelFormat,
     {"Unknown", "ALPHA_8", "GRAY_8", "RG_88", "RGBA_8888", "BGRA_8888"}},
    {tgfx::inspect::CustomEnumType::ImageOrigin, {"TopLeft", "BottomLeft"}},
};

struct DataHead {
  DataType type = DataType::Color;
  uint64_t name = 0;
};

struct PropertyData {
  std::vector<DataHead> summaryName = {};
  std::vector<DataHead> processName = {};
  std::vector<std::shared_ptr<tgfx::Data>> summaryData = {};
  std::vector<std::shared_ptr<tgfx::Data>> processData = {};
};

struct ImageTexture {
  bool isInput = false;
  tgfx::PixelFormat format = tgfx::PixelFormat::Unknown;
  int width = 0;
  int height = 0;
  size_t rowBytes = 0;
  std::shared_ptr<tgfx::Data> data = nullptr;
};

struct TextureData {
  std::vector<uint64_t> inputTextures = {};
  uint64_t outputTexture = 0;
};

enum class UniformFormat {
  Float,                   // 32-bit floating point scalar.
  Float2,                  // 2-component vector of 32-bit floating point values.
  Float3,                  // 3-component vector of 32-bit floating point values.
  Float4,                  // 4-component vector of 32-bit floating point values.
  Float2x2,                // 2x2 matrix of 32-bit floating point values.
  Float3x3,                // 3x3 matrix of 32-bit floating point values.
  Float4x4,                // 4x4 matrix of 32-bit floating point values.
  Int,                     // 32-bit signed integer scalar.
  Int2,                    // 2-component vector of 32-bit signed integer values.
  Int3,                    // 3-component vector of 32-bit signed integer values.
  Int4,                    // 4-component vector of 32-bit signed integer values.
  Texture2DSampler,        // 2D texture sampler.
  TextureExternalSampler,  // External texture sampler (e.g. for camera input).
  Texture2DRectSampler,    // Rectangle texture sampler.
};

struct ShaderData {
  std::array<std::string, 2> shaderText = {};
  std::unordered_map<std::string, UniformFormat> uniforms = {};
};

struct UniformValueData {
  std::string name = {};
  std::shared_ptr<tgfx::Data> value = nullptr;
};

struct MeshData {
  tgfx::inspect::VertexProviderType type = tgfx::inspect::VertexProviderType::RectsVertexProvider;
  std::shared_ptr<tgfx::inspect::MeshInfo> info = nullptr;
  std::shared_ptr<tgfx::Data> vertexData = nullptr;
};

struct VertexData {
  std::vector<float> vertexData = {};
  bool hasUV = false;
  bool hasColor = false;
};

OpOrTask getOpTaskType(tgfx::inspect::OpTaskType type);
}  // namespace inspector