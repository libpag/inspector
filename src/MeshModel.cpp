/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2025 Tencent. All rights reserved.
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

#include "MeshModel.h"
#include "FormatFloatToString.h"
#include "IndicesProvider.h"
#include "tgfx/core/Clock.h"

namespace inspector {
MeshModel::MeshModel(Worker* worker, ViewData* viewData, QObject* parent)
    : QObject(parent), worker(worker), viewData(viewData) {
}

MeshModel::~MeshModel() {
  reset();
}

void MeshModel::reset() {
  selectMeshItem = -1;
  items.clear();
  meshDataDivider.reset();
}

size_t MeshModel::getItemSize() const {
  if (meshDataDivider == nullptr) {
    return 0;
  }
  return meshDataDivider->getAttributes().size() + 2;
}

QList<int> MeshModel::getItemValueCount() const {
  QList<int> itemValueCount = {};
  itemValueCount.push_back(1);
  itemValueCount.push_back(1);
  for (auto& attribute : meshDataDivider->getAttributes()) {
    itemValueCount.push_back(attribute.numberCount);
  }
  return itemValueCount;
}

void MeshModel::refreshData() {
  reset();
  selectMeshIndex(-1);
  auto selectOpTask = viewData->selectOpTask;
  if (selectOpTask == -1) {
    Q_EMIT itemsChanged();
    return;
  }
  const auto& dataContext = worker->getDataContext();
  auto opTask = dataContext.opTasks[static_cast<size_t>(selectOpTask)];
  auto meshDataIter = dataContext.meshDatas.find(opTask->ptr);
  if (meshDataIter == dataContext.meshDatas.end()) {
    Q_EMIT itemsChanged();
    return;
  }
  auto& meshData = meshDataIter->second;
  meshDataDivider = MeshDataDivider::MakeFrom(static_cast<tgfx::inspect::OpTaskType>(opTask->type),
                                              meshData->type, meshData->info);
  auto indicesProvider = IndicesProvider::MakeFrom(
      static_cast<tgfx::inspect::OpTaskType>(opTask->type), meshData->type, meshData->info);
  refreshItems(meshData->vertexData, indicesProvider);
  Q_EMIT itemsChanged();
}

QList<QString> MeshModel::getNames() const {
  if (meshDataDivider == nullptr) {
    return {};
  }

  QList<QString> names = {};
  names.push_back("VTX");
  names.push_back("IDX");
  names.reserve(static_cast<qsizetype>(meshDataDivider->getAttributes().size()));
  for (auto& attribute : meshDataDivider->getAttributes()) {
    names.push_back(attribute.name.c_str());
  }
  return names;
}

QList<QList<QList<QVariant>>> MeshModel::getValues() {
  return items;
}

int MeshModel::getSelectMeshItem() const {
  return selectMeshItem;
}

void MeshModel::setSelectMeshItem(int index) {
  selectMeshItem = index;
  Q_EMIT selectMeshIndex(index);
  Q_EMIT selectChanged();
}

void MeshModel::refreshItems(const std::shared_ptr<tgfx::Data>& data,
                             const std::shared_ptr<IndicesProvider>& indicesProvider) {
  auto meshItems = encodeData(data);
  if (meshItems.empty()) {
    return;
  }
  QList<QList<QVariant>> vertex = {};
  QList<QList<QVariant>> index = {};
  const auto& indices = indicesProvider->indices();
  if (indices.empty()) {
    items.reserve(meshItems.size() + 1);
    vertex.reserve(static_cast<qsizetype>(meshItems.size()));
    index.reserve(static_cast<qsizetype>(meshItems.size()));
    for (size_t i = 0; i < static_cast<size_t>(meshItems[0].size()); ++i) {
      QList<QVariant> vertexItem = {static_cast<int>(i)};
      QList<QVariant> indexItem = {static_cast<int>(i)};
      vertex.push_back(std::move(vertexItem));
      index.push_back(std::move(indexItem));
    }

    items.push_back(std::move(vertex));
    items.push_back(std::move(index));
    for (auto& item : meshItems) {
      items.push_back(item);
    }
  }
  else {
    items.reserve(meshItems.size() + 2);
    vertex.reserve(static_cast<qsizetype>(indices.size()));
    index.reserve(static_cast<qsizetype>(indices.size()));
    for (size_t i = 0; i < indices.size(); ++i) {
      QList<QVariant> vertexItem = {static_cast<int>(i)};
      QList<QVariant> indexItem = {indices[i]};
      vertex.push_back(std::move(vertexItem));
      index.push_back(std::move(indexItem));
    }

    items.push_back(std::move(vertex));
    items.push_back(std::move(index));
    for (auto& item : meshItems) {
      QList<QList<QVariant>> valueItem = {};
      valueItem.reserve(static_cast<qsizetype>(indices.size()));
      for (auto indice : indices) {
        valueItem.push_back(item[static_cast<qsizetype>(indice)]);
      }
      items.push_back(std::move(valueItem));
    }
  }
}

QList<QList<QList<QVariant>>> MeshModel::encodeData(const std::shared_ptr<tgfx::Data>& data) const {
  tgfx::DataView dataView(data->bytes(), data->size());
  size_t offset = 0;
  QMap<QString, QList<QList<QVariant>>> meshItems = {};
  QList<QList<QList<QVariant>>> resultItems = {};
  while (offset < data->size()) {
    for (auto& attribute : meshDataDivider->getAttributes()) {
      QString name = attribute.name.c_str();
      if (meshItems.find(name) == meshItems.end()) {
        meshItems[name] = {};
      }
      auto& values = meshItems[name];
      switch (attribute.format) {
        case MeshDataDivider::Format::Float: {
          QList<QVariant> item = {};
          item.reserve(1);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Float2: {
          QList<QVariant> item = {};
          item.reserve(2);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Float3: {
          QList<QVariant> item = {};
          item.reserve(3);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Float4: {
          QList<QVariant> item = {};
          item.reserve(4);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          item.emplace_back(FormatFloatToString(dataView.getFloat(offset)));
          offset += sizeof(float);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Half: {
          QList<QVariant> item = {};
          item.reserve(1);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Half2: {
          QList<QVariant> item = {};
          item.reserve(2);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Half3: {
          QList<QVariant> item = {};
          item.reserve(3);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Half4: {
          QList<QVariant> item = {};
          item.reserve(4);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          item.emplace_back(dataView.getUint16(offset));
          offset += sizeof(uint16_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Int: {
          QList<QVariant> item = {};
          item.reserve(1);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Int2: {
          QList<QVariant> item = {};
          item.reserve(2);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Int3: {
          QList<QVariant> item = {};
          item.reserve(3);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::Int4: {
          QList<QVariant> item = {};
          item.reserve(4);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          item.emplace_back(dataView.getInt32(offset));
          offset += sizeof(int32_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::UByteNormalized: {
          QList<QVariant> item = {};
          item.reserve(1);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::UByte2Normalized: {
          QList<QVariant> item = {};
          item.reserve(2);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::UByte3Normalized: {
          QList<QVariant> item = {};
          item.reserve(3);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          values.push_back(item);
          break;
        }
        case MeshDataDivider::Format::UByte4Normalized: {
          QList<QVariant> item = {};
          item.reserve(4);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          item.emplace_back(dataView.getUint8(offset));
          offset += sizeof(uint8_t);
          values.push_back(item);
          break;
        }
        default:
          break;
      }
    }
  }

  resultItems.reserve(static_cast<qsizetype>(meshDataDivider->getAttributes().size()));
  for (auto& attribute : meshDataDivider->getAttributes()) {
    resultItems.push_back(meshItems[attribute.name.c_str()]);
  }
  return resultItems;
}
}  // namespace inspector