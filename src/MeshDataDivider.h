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
#include <ProcessUtils.h>
#include <QObject>
#include <QVector>
#include <memory>

namespace inspector {
class MeshDataDivider {
 public:
  enum class Format {
    Float,             // 32-bit floating point scalar.
    Float2,            // 2-component vector of 32-bit floating point values.
    Float3,            // 3-component vector of 32-bit floating point values.
    Float4,            // 4-component vector of 32-bit floating point values.
    Half,              // 16-bit floating point scalar.
    Half2,             // 2-component vector of 16-bit floating point values.
    Half3,             // 3-component vector of 16-bit floating point values.
    Half4,             // 4-component vector of 16-bit floating point values.
    Int,               // 32-bit signed integer scalar.
    Int2,              // 2-component vector of 32-bit signed integer values.
    Int3,              // 3-component vector of 32-bit signed integer values.
    Int4,              // 4-component vector of 32-bit signed integer values.
    UByteNormalized,   // 8-bit unsigned integer scalar, normalized to [0,1].
    UByte2Normalized,  // 2-component vector of 8-bit unsigned integer values, normalized to [0,1].
    UByte3Normalized,  // 3-component vector of 8-bit unsigned integer values, normalized to [0,1].
    UByte4Normalized,  // 4-component vector of 8-bit unsigned integer values, normalized to [0,1].
  };

  enum class UniformFormat {
    Position,
    Other,
  };

  struct Attribute {
    std::string name;
    Format format;
    UniformFormat uniformFormat;
    int numberCount = 0;
  };

  static std::shared_ptr<MeshDataDivider> MakeFrom(
      tgfx::inspect::OpTaskType opTaskType, tgfx::inspect::VertexProviderType type,
      const std::shared_ptr<tgfx::inspect::MeshInfo>& meshInfo);

  virtual ~MeshDataDivider() {
    attributes.clear();
  }

  const std::vector<Attribute>& getAttributes() const {
    return attributes;
  }

  std::vector<Attribute> attributes = {};
};
}  // namespace inspector