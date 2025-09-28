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

#include "UniformModel.h"
#include "FormatFloatToString.h"

namespace inspector {
UniformModel::UniformModel(Worker* worker, ViewData* viewData, QObject* parent)
    : QObject(parent), worker(worker), viewData(viewData) {
  refreshData();
}

UniformModel::~UniformModel() {
  clearItems();
}

void UniformModel::clearItems() {
  uniformItems.clear();
}

QList<QObject*> UniformModel::getUniformItems() const {
  QList<QObject*> items = {};
  items.reserve(static_cast<qsizetype>(uniformItems.size()));
  for (auto& item : uniformItems) {
    items.append(item.get());
  }
  return items;
}

void UniformModel::refreshData() {
  clearItems();
  auto selectOpTask = viewData->selectOpTask;
  if (selectOpTask == -1) {
    Q_EMIT itemsChanged();
    return;
  }
  const auto& dataContext = worker->getDataContext();
  auto programKeyIter = dataContext.programKeys.find(static_cast<uint32_t>(selectOpTask));
  if (programKeyIter == dataContext.programKeys.end()) {
    Q_EMIT itemsChanged();
    return;
  }
  auto shaderIter = dataContext.shaderData.find(programKeyIter->second);
  if (shaderIter == dataContext.shaderData.end()) {
    Q_EMIT itemsChanged();
    return;
  }
  auto uniformValueIter = dataContext.uniformValues.find(static_cast<uint32_t>(selectOpTask));
  if (uniformValueIter == dataContext.uniformValues.end()) {
    Q_EMIT itemsChanged();
    return;
  }
  auto& uniformValues = uniformValueIter->second;
  auto& uniformInfos = shaderIter->second.uniforms;
  uniformItems.reserve(uniformItems.size());
  for (const auto& uniform : uniformValues) {
    auto uniformName = uniform.name;
    auto uniformInfoIter = uniformInfos.find(uniformName);
    if (uniformInfoIter == uniformInfos.end()) {
      continue;
    }
    auto uniformType = uniformInfoIter->second;
    auto formatName = toUniformFormatName(uniformType);
    uniformItems.push_back(std::make_shared<UniformItem>(uniformName.c_str(), formatName,
                                                         readData(uniform.value, uniformType),
                                                         getFormatHeight(uniformType), this));
  }
  Q_EMIT itemsChanged();
}

QString UniformModel::toUniformFormatName(UniformFormat format) {
  switch (format) {
    case UniformFormat::Float:
      return "Float";
    case UniformFormat::Float2:
      return "Float2";
    case UniformFormat::Float3:
      return "Float3";
    case UniformFormat::Float4:
      return "Float4";
    case UniformFormat::Float2x2:
      return "Float2x2";
    case UniformFormat::Float3x3:
      return "Float3x3";
    case UniformFormat::Float4x4:
      return "Float4x4";
    case UniformFormat::Int:
      return "Int";
    case UniformFormat::Int2:
      return "Int2";
    case UniformFormat::Int3:
      return "Int3";
    case UniformFormat::Int4:
      return "Int4";
    default:
      return "?";
  }
}

int UniformModel::getFormatHeight(UniformFormat format) {
  switch (format) {
    case UniformFormat::Float2x2:
      return 2;
    case UniformFormat::Float3x3:
      return 3;
    case UniformFormat::Float4x4:
      return 4;
    default:
      return 1;
  }
}

QVariant UniformModel::readData(std::shared_ptr<tgfx::Data> valueData, UniformFormat format) {
  if (valueData == nullptr) {
    return tr("nullptr(Parsing exception)");
  }
  auto dataView = tgfx::DataView(valueData->bytes(), valueData->size());
  switch (format) {
    case UniformFormat::Float: {
      auto value = dataView.getFloat(0);
      return FormatFloatToString(value);
    }
    case UniformFormat::Float2: {
      auto size = sizeof(float);
      auto data0 = dataView.getFloat(0);
      auto data1 = dataView.getFloat(size);
      return "(" + FormatFloatToString(data0) + ", " + FormatFloatToString(data1) + ")";
    }
    case UniformFormat::Float3: {
      auto size = sizeof(float);
      auto data0 = dataView.getFloat(0);
      auto data1 = dataView.getFloat(size);
      auto data2 = dataView.getFloat(2 * size);
      return "(" + FormatFloatToString(data0) + ", " + FormatFloatToString(data1) + ", " +
             FormatFloatToString(data2) + ")";
    }
    case UniformFormat::Float4: {
      auto size = sizeof(float);
      auto data0 = dataView.getFloat(0);
      auto data1 = dataView.getFloat(size);
      auto data2 = dataView.getFloat(2 * size);
      auto data3 = dataView.getFloat(3 * size);
      return "(" + FormatFloatToString(data0) + ", " + FormatFloatToString(data1) + ", " +
             FormatFloatToString(data2) + ", " + FormatFloatToString(data3) + ")";
    }
    case UniformFormat::Float2x2: {
      auto size = sizeof(float);
      auto data0 = dataView.getFloat(0);
      auto data1 = dataView.getFloat(size);
      auto data2 = dataView.getFloat(2 * size);
      auto data3 = dataView.getFloat(3 * size);
      return "[" + FormatFloatToString(data0) + ", " + FormatFloatToString(data1) + "\n" +
             FormatFloatToString(data2) + ", " + FormatFloatToString(data3) + "]";
    }
    case UniformFormat::Float3x3: {
      auto size = sizeof(float);
      auto data0 = dataView.getFloat(0);
      auto data1 = dataView.getFloat(size);
      auto data2 = dataView.getFloat(size * 2);
      auto data3 = dataView.getFloat(size * 3);
      auto data4 = dataView.getFloat(size * 4);
      auto data5 = dataView.getFloat(size * 5);
      auto data6 = dataView.getFloat(size * 6);
      auto data7 = dataView.getFloat(size * 7);
      auto data8 = dataView.getFloat(size * 8);
      return "[" + FormatFloatToString(data0) + ", " + FormatFloatToString(data1) + ", " +
             FormatFloatToString(data2) + "\n" + FormatFloatToString(data3) + ", " +
             FormatFloatToString(data4) + ", " + FormatFloatToString(data5) + "\n " +
             FormatFloatToString(data6) + ", " + FormatFloatToString(data7) + ", " +
             FormatFloatToString(data8) + "]";
    }
    case UniformFormat::Float4x4: {
      auto size = sizeof(float);
      auto data0 = dataView.getFloat(0);
      auto data1 = dataView.getFloat(size);
      auto data2 = dataView.getFloat(size * 2);
      auto data3 = dataView.getFloat(size * 3);
      auto data4 = dataView.getFloat(size * 4);
      auto data5 = dataView.getFloat(size * 5);
      auto data6 = dataView.getFloat(size * 6);
      auto data7 = dataView.getFloat(size * 7);
      auto data8 = dataView.getFloat(size * 8);
      auto data9 = dataView.getFloat(size * 9);
      auto data10 = dataView.getFloat(size * 10);
      auto data11 = dataView.getFloat(size * 11);
      auto data12 = dataView.getFloat(size * 12);
      auto data13 = dataView.getFloat(size * 13);
      auto data14 = dataView.getFloat(size * 14);
      auto data15 = dataView.getFloat(size * 15);
      return "[" + FormatFloatToString(data0) + ", " + FormatFloatToString(data1) + ", " +
             FormatFloatToString(data2) + "," + FormatFloatToString(data3) + "\n " +
             FormatFloatToString(data4) + ", " + FormatFloatToString(data5) + ", " +
             FormatFloatToString(data6) + ", " + FormatFloatToString(data7) + "\n " +
             FormatFloatToString(data8) + ", " + FormatFloatToString(data9) + ", " +
             FormatFloatToString(data10) + ", " + FormatFloatToString(data11) + "\n " +
             FormatFloatToString(data12) + ", " + FormatFloatToString(data13) + ", " +
             FormatFloatToString(data14) + ", " + FormatFloatToString(data15) + "]";
    }
    case UniformFormat::Int:
      return dataView.getInt32(0);
    case UniformFormat::Int2: {
      auto size = sizeof(int32_t);
      auto data0 = dataView.getInt32(0);
      auto data1 = dataView.getInt32(size);
      return "(" + QString::number(data0) + ", " + QString::number(data1) + ")";
    }
    case UniformFormat::Int3: {
      auto size = sizeof(int32_t);
      auto data0 = dataView.getInt32(0);
      auto data1 = dataView.getInt32(size);
      auto data2 = dataView.getInt32(size * 2);
      return "(" + QString::number(data0) + ", " + QString::number(data1) + ", " +
             QString::number(data2) + ")";
    }
    case UniformFormat::Int4: {
      auto size = sizeof(int32_t);
      auto data0 = dataView.getInt32(0);
      auto data1 = dataView.getInt32(size);
      auto data2 = dataView.getInt32(size * 2);
      auto data3 = dataView.getInt32(size * 3);
      return "(" + QString::number(data0) + ", " + QString::number(data1) + ", " +
             QString::number(data2) + ", " + QString::number(data3) + ")";
    }
    default:
      return "?";
  }
}
}  // namespace inspector