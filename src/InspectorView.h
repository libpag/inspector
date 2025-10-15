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

#include <QMainWindow>
#include <QQuickWindow>
#include "MeshModel.h"
#include "SelectFrameModel.h"
#include "ShaderTextModel.h"
#include "StartView.h"
#include "TaskFilterModel.h"
#include "TaskTreeModel.h"
#include "UniformModel.h"
#include "ViewData.h"
#include "Worker.h"

namespace inspector {
class ClientData;
class InspectorView : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isOpenFile READ getIsOpenFile CONSTANT)
  Q_PROPERTY(bool hasSaveFilePath READ getHasSaveFilePath CONSTANT)
 public:
  InspectorView(uint16_t webPort, int width, QObject* parent = nullptr);
  InspectorView(std::string filePath, int width, QObject* parent = nullptr);
  InspectorView(ClientData* clientData, int width, QObject* parent = nullptr);
  ~InspectorView() override;

  void initView();
  void initConnect();
  void failedCreateWorker();
  bool getIsOpenFile() const;
  bool getHasSaveFilePath() const;

  Q_INVOKABLE void openStartView();
  Q_INVOKABLE bool saveFileAs(const QUrl& filePath);
  Q_INVOKABLE bool saveFile();
  Q_INVOKABLE void nextFrame();
  Q_INVOKABLE void preFrame();
  Q_INVOKABLE void captureFrame();

  Q_SLOT void onCloseView(QQuickCloseEvent*);
  Q_SIGNAL void closeView(QObject* view);
  Q_SIGNAL void failedOpenInspectorView(QString errorMsg);
  Q_SIGNAL void viewHide();

 private:
  int width;
  bool isOpenFile = false;
  Worker worker;
  ViewData viewData;
  std::string saveFilePath;
  ClientData* clientData = nullptr;
  std::unique_ptr<QQmlApplicationEngine> ispEngine = nullptr;
  std::unique_ptr<TaskTreeModel> taskTreeModel = nullptr;
  std::unique_ptr<SelectFrameModel> selectFrameModel = nullptr;
  std::unique_ptr<TaskFilterModel> taskFilterModel = nullptr;
  std::unique_ptr<AttributeModel> attributeModel = nullptr;
  std::unique_ptr<ShaderTextModel> shaderTextModel = nullptr;
  std::unique_ptr<UniformModel> uniformModel = nullptr;
  std::unique_ptr<MeshModel> meshModel = nullptr;
};
}  // namespace inspector
