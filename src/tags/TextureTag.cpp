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

#include "TextureTag.h"
#include "DataContext.h"

namespace inspector {
void ReadTextureTag(DecodeStream* stream) {
  auto count = stream->readEncodedUint32();
  auto context = dynamic_cast<DataContext*>(stream->context);
  auto& textures = context->textures;
  for (uint32_t i = 0; i < count; ++i) {
    auto ptr = std::make_shared<TextureData>();
    auto childIndex = stream->readEncodedUint32();
    auto& inputTextures = ptr->inputTextures;
    auto inputTexturesCount = stream->readEncodedUint32();
    inputTextures.reserve(inputTexturesCount);
    for (uint32_t j = 0; j < inputTexturesCount; ++j) {
      inputTextures[j] = stream->readEncodedUint64();
    }
    ptr->outputTexture = stream->readEncodedUint64();
    textures[childIndex] = std::move(ptr);
  }

  auto& imageDatas = context->images;
  count = stream->readEncodedUint32();
  for (uint32_t i = 0; i < count; ++i) {
    auto imageTextrue = std::make_shared<ImageTexture>();
    auto textureId = stream->readEncodedUint64();
    imageTextrue->isInput = stream->readBoolean();
    imageTextrue->format = static_cast<tgfx::PixelFormat>(stream->readUint8());
    imageTextrue->width = stream->readEncodedInt32();
    imageTextrue->height = stream->readEncodedInt32();
    imageTextrue->rowBytes = stream->readEncodedUint32();
    imageTextrue->data = stream->readData();
    imageDatas[textureId] = std::move(imageTextrue);
  }
}

TagType WriteTextureTag(EncodeStream* stream, DataContext* context) {
  const auto& textures = context->textures;
  stream->writeEncodedUint32(static_cast<uint32_t>(textures.size()));
  for (const auto& texture : textures) {
    stream->writeEncodedUint32(texture.first);
    const auto& textureData = texture.second;
    stream->writeEncodedUint32(static_cast<uint32_t>(textureData->inputTextures.size()));
    for (const auto& inputTexture : textureData->inputTextures) {
      stream->writeEncodedUint64(inputTexture);
    }
    stream->writeEncodedUint64(textureData->outputTexture);
  }

  const auto& imageDatas = context->images;
  stream->writeEncodedUint32(static_cast<uint32_t>(imageDatas.size()));
  for (const auto& imageData: imageDatas) {
    stream->writeEncodedUint64(imageData.first);
    const auto& imageTexture = imageData.second;
    stream->writeBoolean(imageTexture->isInput);
    stream->writeUint8(static_cast<uint8_t>(imageTexture->format));
    stream->writeEncodedInt32(imageTexture->width);
    stream->writeEncodedInt32(imageTexture->height);
    stream->writeEncodedUint32(static_cast<uint32_t>(imageTexture->rowBytes));
    stream->writeData(imageTexture->data.get());
  }
  return TagType::Texture;
}

}  // namespace inspector
