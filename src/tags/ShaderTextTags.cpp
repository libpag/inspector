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
#include "ShaderTextTags.h"

namespace inspector {
void ReadShaderTextTag(DecodeStream* stream) {
  auto context = dynamic_cast<DataContext*>(stream->context);
  auto& programKeys = context->programKeys;
  auto count = stream->readEncodedUint32();
  for (uint32_t i = 0; i < count; ++i) {
    auto opTaskId = stream->readEncodedUint32();
    auto data = stream->readData();
    tgfx::BytesKey key = {};
    auto keySize = data->size() / sizeof(uint32_t);
    tgfx::DataView dataView(data->bytes(), data->size());
    for (size_t j = 0; j < keySize; ++j) {
      uint32_t keyValue = dataView.getUint32(j * sizeof(uint32_t));
      key.write(keyValue);
    }
    programKeys[opTaskId] = key;
  }

  auto& shaderData = context->shaderData;
  count = stream->readEncodedUint32();
  for (uint32_t i = 0; i < count; ++i) {
    auto data = stream->readData();
    tgfx::BytesKey key = {};
    auto keySize = data->size() / sizeof(uint32_t);
    tgfx::DataView dataView(data->bytes(), data->size());
    for (size_t j = 0; j < keySize; ++j) {
      uint32_t keyValue = dataView.getUint32(j * sizeof(uint32_t));
      key.write(keyValue);
    }

    ShaderData shader = {};
    auto& shaderText = shader.shaderText;
    auto vertexData = stream->readData();
    auto fragmentData = stream->readData();
    shaderText[0] = std::string(static_cast<const char*>(vertexData->data()), vertexData->size());
    shaderText[1] = std::string(static_cast<const char*>(fragmentData->data()), fragmentData->size());

    auto& uniforms = shader.uniforms;
    auto uniformsCount = stream->readEncodedUint32();
    for (uint32_t j = 0; j < uniformsCount; ++j) {
      auto uniformNameData = stream->readData();
      auto uniformName = std::string(static_cast<const char*>(uniformNameData->data()), uniformNameData->size());
      uniforms[uniformName] = static_cast<UniformFormat>(stream->readUint8());
    }

    shaderData[key] = std::move(shader);
  }

  auto& uniformValues = context->uniformValues;
  count = stream->readEncodedUint32();
  for (uint32_t i = 0; i < count; ++i) {
    auto opTaskId = stream->readEncodedUint32();
    auto valueSize = stream->readEncodedUint32();
    std::vector<UniformValueData> uniformValueDatas = {};
    uniformValueDatas.reserve(valueSize);
    for (uint32_t j = 0; j < valueSize; ++j) {
      UniformValueData uniformValueData = {};
      auto uniformNameData = stream->readData();
      uniformValueData.name = std::string(static_cast<const char*>(uniformNameData->data()), uniformNameData->size());
      uniformValueData.value = stream->readData();
      uniformValueDatas.push_back(std::move(uniformValueData));
    }
    uniformValues[opTaskId] = std::move(uniformValueDatas);
  }
}

TagType WriteShaderTextTag(EncodeStream* stream, DataContext* context) {
  const auto& programKeys = context->programKeys;
  stream->writeEncodedUint32(static_cast<uint32_t>(programKeys.size()));
  for (const auto& programKey: programKeys) {
    stream->writeEncodedUint32(programKey.first);
    auto keyData = tgfx::Data::MakeWithoutCopy(programKey.second.data(), programKey.second.size());
    stream->writeData(keyData.get());
  }
  const auto& shaderData = context->shaderData;
  stream->writeEncodedUint32(static_cast<uint32_t>(shaderData.size()));
  for (const auto& shader: shaderData) {
    auto keyData = tgfx::Data::MakeWithoutCopy(shader.first.data(), shader.first.size());
    stream->writeData(keyData.get());

    const auto& shaderText = shader.second.shaderText;
    auto vertexShaderData = tgfx::Data::MakeWithoutCopy(shaderText[0].c_str(), shaderText[0].length());
    auto fragmentShaderData = tgfx::Data::MakeWithoutCopy(shaderText[1].c_str(), shaderText[1].length());
    stream->writeData(vertexShaderData.get());
    stream->writeData(fragmentShaderData.get());

    const auto& uniforms = shader.second.uniforms;
    stream->writeEncodedUint32(static_cast<uint32_t>(uniforms.size()));
    for (const auto& uniform: uniforms) {
      auto uniformName = tgfx::Data::MakeWithoutCopy(uniform.first.c_str(), uniform.first.length());
      stream->writeData(uniformName.get());
      stream->writeUint8(static_cast<uint8_t>(uniform.second));
    }
  }
  const auto& uniformValues = context->uniformValues;
  stream->writeEncodedUint32(static_cast<uint32_t>(uniformValues.size()));
  for (const auto& uniformValue: uniformValues) {
    stream->writeEncodedUint32(uniformValue.first);
    stream->writeEncodedUint32(static_cast<uint32_t>(uniformValue.second.size()));
    for(const auto& uniformValueData: uniformValue.second) {
      auto name = tgfx::Data::MakeWithoutCopy(uniformValueData.name.c_str(), uniformValueData.name.length());
      stream->writeData(name.get());
      stream->writeData(uniformValueData.value.get());
    }
  }
  return TagType::ShaderAndUniform;
}
}