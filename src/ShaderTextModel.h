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

#include <QObject>
#include "ViewData.h"
#include "Worker.h"

namespace inspector {
class ShaderTextModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString vertexShaderText READ vertexShaderText NOTIFY updateShaderText)
  Q_PROPERTY(QString fragmentShaderText READ fragmentShaderText NOTIFY updateShaderText)
 public:
  ShaderTextModel(Worker* worker, ViewData* viewData, QObject* parent = nullptr);
  ~ShaderTextModel() override;

  QString shaderText(size_t index);
  QString vertexShaderText();
  QString fragmentShaderText();

  Q_SLOT void refreshShaderText();
  Q_SIGNAL void updateShaderText();

 private:
  Worker* worker = nullptr;
  ViewData* viewData = nullptr;
};
}  // namespace inspector