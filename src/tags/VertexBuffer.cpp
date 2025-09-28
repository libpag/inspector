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

#include "DataContext.h"
#include "DecodeStream.h"
#include "VertexBufferTag.h"

namespace inspector {
void ReadVertexBufferTag(DecodeStream* stream) {
  auto count = stream->readEncodedUint32();
  auto context = dynamic_cast<DataContext*>(stream->context);
  auto& meshDatas = context->meshDatas;
  for (uint32_t i = 0; i < count; ++i) {
    auto opTaskPtr = stream->readEncodedUint64();
    auto meshData = std::make_shared<MeshData>();
    meshData->type = static_cast<tgfx::inspect::VertexProviderType>(stream->readUint8());
    if (meshData->type == tgfx::inspect::VertexProviderType::RectsVertexProvider) {
      auto meshInfo = std::make_shared<tgfx::inspect::RectMeshInfo>();
      meshInfo->rectCount = stream->readEncodedUint32();
      meshInfo->drawOpPtr = stream->readEncodedUint64();
      meshInfo->aaType = stream->readUint8();
      meshInfo->hasUVCoord = stream->readBoolean();
      meshInfo->hasColor = stream->readBoolean();
      meshInfo->hasSubset = stream->readBoolean();
      meshData->info = std::move(meshInfo);
    }
    else {
      auto meshInfo = std::make_shared<tgfx::inspect::RRectMeshInfo>();
      meshInfo->rectCount = stream->readEncodedUint32();
      meshInfo->drawOpPtr = stream->readEncodedUint64();
      meshInfo->hasColor = stream->readBoolean();
      meshInfo->hasStroke = stream->readBoolean();
      meshInfo->useScale = stream->readBoolean();
      meshData->info = std::move(meshInfo);
    }
    meshData->vertexData = stream->readData();
    meshDatas[opTaskPtr] = std::move(meshData);
  }
}

TagType WriteVertexBufferTag(
    EncodeStream* stream, std::unordered_map<uint64_t, std::shared_ptr<MeshData>>* vertexDatas) {
  stream->writeEncodedUint32(static_cast<uint32_t>(vertexDatas->size()));
  for (const auto& vertexBuffer : *vertexDatas) {
    stream->writeEncodedUint64(vertexBuffer.first);
    const auto& meshData = vertexBuffer.second;
    stream->writeUint8(static_cast<uint8_t>(meshData->type));
    if (meshData->type == tgfx::inspect::VertexProviderType::RectsVertexProvider) {
      auto meshInfo = std::static_pointer_cast<tgfx::inspect::RectMeshInfo>(meshData->info);
      stream->writeEncodedUint32(meshInfo->rectCount);
      stream->writeEncodedUint64(meshInfo->drawOpPtr);
      stream->writeUint8(meshInfo->aaType);
      stream->writeBoolean(meshInfo->hasUVCoord);
      stream->writeBoolean(meshInfo->hasColor);
      stream->writeBoolean(meshInfo->hasSubset);
    }
    else {
      auto meshInfo = std::static_pointer_cast<tgfx::inspect::RRectMeshInfo>(meshData->info);
      stream->writeEncodedUint32(meshInfo->rectCount);
      stream->writeEncodedUint64(meshInfo->drawOpPtr);
      stream->writeBoolean(meshInfo->hasColor);
      stream->writeBoolean(meshInfo->useScale);
      stream->writeBoolean(meshInfo->hasStroke);
    }
    stream->writeData(meshData->vertexData.get());
  }
  return TagType::VertexBuffer;
}

}  // namespace inspector