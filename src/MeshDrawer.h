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
#include <QQuickItem>
#include "AppHost.h"
#include "IndicesProvider.h"
#include "MeshDataDivider.h"
#include "ViewData.h"
#include "Worker.h"
#include "tgfx/core/Clock.h"
#include "tgfx/gpu/opengl/qt/QGLWindow.h"

namespace inspector {
class TestTime {
 public:
  explicit TestTime(const char* name) : time(tgfx::Clock::Now()), name(name) {
  }

  ~TestTime() {
    auto costTime = tgfx::Clock::Now() - time;
    LOGI("%s cost time: %lld us", name.c_str(), costTime);
  }

 private:
  int64_t time;
  std::string name;
};

class MeshDrawer : public QQuickItem {
  Q_OBJECT
  Q_PROPERTY(Worker* worker READ getWorker WRITE setWorker)
  Q_PROPERTY(ViewData* viewData READ getViewData WRITE setViewData)
 public:
  MeshDrawer(QQuickItem* parent = nullptr);
  ~MeshDrawer() override;

  Worker* getWorker() const {
    return worker;
  }
  void setWorker(Worker* worker) {
    this->worker = worker;
  }

  ViewData* getViewData() const {
    return viewData;
  }
  void setViewData(ViewData* viewData) {
    this->viewData = viewData;
  }

  void clear();

  Q_SLOT void refreshData();
  Q_SLOT void selectMeshIndex(int index);

 protected:
  QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
  void drawWireFrame(QSGNode* root);
  void drawSelectTriangle(QSGNode* root);
  void drawSelectPoint(QSGNode* root);
  void updateMatrix();
  void encodeData(const std::shared_ptr<tgfx::Data>& data,
                  const std::shared_ptr<MeshDataDivider>& meshDataDivider);

 private:
  Worker* worker = nullptr;
  ViewData* viewData = nullptr;
  int selectIndex = -1;
  std::shared_ptr<tgfx::QGLWindow> tgfxWindow = nullptr;
  std::shared_ptr<AppHost> appHost = nullptr;
  std::shared_ptr<IndicesProvider> indicesProvider;
  std::vector<std::array<float, 2>> postionData = {};
  tgfx::Rect bounds = {};
  tgfx::Matrix matrix = {};
};
}  // namespace inspector
