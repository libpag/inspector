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

#include "ShaderTextModel.h"

namespace inspector {
ShaderTextModel::ShaderTextModel(Worker* worker, ViewData* viewData, QObject* parent)
    : QObject(parent), worker(worker), viewData(viewData) {
}

ShaderTextModel::~ShaderTextModel() = default;

QString ShaderTextModel::shaderText(size_t index) {
  auto selectOpTask = viewData->selectOpTask;
  const auto& dataContext = worker->getDataContext();
  auto programKeyIter = dataContext.programKeys.find(static_cast<uint32_t>(selectOpTask));
  if (programKeyIter == dataContext.programKeys.end()) {
    return "Not found Shader";
  }
  auto shaderIter = dataContext.shaderData.find(programKeyIter->second);
  if (shaderIter == dataContext.shaderData.end()) {
    return "Not found Shader";
  }
  return shaderIter->second.shaderText[index].c_str();
}

QString ShaderTextModel::vertexShaderText() {
  return shaderText(0);
}

QString ShaderTextModel::fragmentShaderText() {
  return shaderText(1);
}

void ShaderTextModel::refreshShaderText() {
  Q_EMIT updateShaderText();
}

}  // namespace inspector