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

#include "StartView.h"
#include <QApplication>
#include <QFileInfo>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include "Protocol.h"
#include "Socket.h"
#include "tgfx/core/Clock.h"

namespace inspector {
ClientData::ClientData(Data data) : data(std::move(data)) {
}

StartView::StartView(QObject* parent) : QObject(parent) {
  resolv = std::make_unique<ResolvService>(port);
  loadRecentFiles();

  broadcastTimer = new QTimer(this);
  connect(broadcastTimer, &QTimer::timeout, this, &StartView::updateBroadcastClients);
  broadcastTimer->start(1000);
}

StartView::~StartView() {
  saveRecentFiles();
  qDeleteAll(fileItems);
  if (broadcastTimer) {
    broadcastTimer->stop();
    delete broadcastTimer;
  }
  for (auto& it : clients) {
    delete it.second;
  }
  clients.clear();
  broadcastListen.reset();
  Q_EMIT quitStartView();
}

QList<QObject*> StartView::getFileItems() const {
  QList<QObject*> items = {};
  items.reserve(fileItems.size());
  for (const auto& fileItem : fileItems) {
    items.append(fileItem);
  }
  return items;
}

void StartView::openFile(const QString& fPath) {
  if (fPath.isEmpty() || !QFileInfo::exists(fPath)) {
    return;
  }
  addRecentFile(fPath);
  // TODO Open file
}

void StartView::openFile(const QUrl& filePath) {
  openFile(filePath.path());
}

void StartView::addRecentFile(const QString& fPath) {
  if (fPath.isEmpty()) {
    return;
  }
  recentFiles.removeAll(fPath);
  recentFiles.prepend(fPath);
  if (lastOpenFile != fPath) {
    lastOpenFile = fPath;
    Q_EMIT lastOpenFileChanged();
  }
  while (recentFiles.size() >= 15) {
    recentFiles.removeLast();
  }
  qDeleteAll(fileItems);
  fileItems.clear();
  for (const QString& file : recentFiles) {
    fileItems.append(new FileItem(file, QFileInfo(file).fileName(), this));
  }
  Q_EMIT fileItemsChanged();
  saveRecentFiles();
}

void StartView::clearRecentFiles() {
  recentFiles.clear();
  qDeleteAll(fileItems);
  fileItems.clear();

  Q_EMIT fileItemsChanged();

  saveRecentFiles();
}

QVector<QObject*> StartView::getFrameCaptureClientItems() const {
  QVector<QObject*> clientDatas = {};
  clientDatas.reserve(static_cast<int64_t>(clients.size()));
  for (auto& client : clients) {
    if (client.second->data.type == static_cast<uint8_t>(tgfx::debug::ToolType::FrameCapture)) {
      clientDatas.push_back(client.second);
    }
  }
  return clientDatas;
}

QVector<QObject*> StartView::getLayerTreeClientItems() const {
  QVector<QObject*> clientDatas = {};
  clientDatas.reserve(static_cast<int64_t>(clients.size()));
  for (auto& client : clients) {
    if (client.second->data.type == static_cast<uint8_t>(tgfx::debug::ToolType::LayerTree)) {
      clientDatas.push_back(client.second);
    }
  }
  return clientDatas;
}

void StartView::connectToClient(QObject* object) {
  auto client = dynamic_cast<ClientData*>(object);
  if (client) {
    // TODO Connect to Insepector
  }
}

void StartView::connectToClientByLayerInspector(QObject* object) {
  auto client = dynamic_cast<ClientData*>(object);
  if (client) {
    // TODO Connect to Layer Inspector
  }
}

void StartView::showStartView() {
  if (!qmlEngine) {
    qmlEngine = new QQmlApplicationEngine(this);
    qmlEngine->rootContext()->setContextProperty("startViewModel", this);
    qmlEngine->load(QUrl(QStringLiteral("qrc:/qml/StartView.qml")));

    if (!qmlEngine->rootObjects().isEmpty()) {
      auto startWindow = dynamic_cast<QQuickWindow*>(qmlEngine->rootObjects().first());
      startWindow->setFlags(Qt::Window);
      startWindow->setTitle("Inspector - Start");
      startWindow->resize(1000, 600);
      startWindow->show();

      connect(startWindow, &QQuickWindow::closing, this, &StartView::onCloseAllView);
      connect(this, &StartView::quitStartView, QApplication::instance(), &QApplication::quit);
    } else {
      qWarning() << "无法加载StartView.qml";
    }
  }

  if (!qmlEngine->rootObjects().isEmpty()) {
    auto startWindow = dynamic_cast<QQuickWindow*>(qmlEngine->rootObjects().first());
    startWindow->show();
  }
}

void StartView::loadRecentFiles() {
  QSettings settings("TGFX", "Inspector");
  recentFiles = settings.value(QStringLiteral("recentFiles")).toStringList();

  QStringList validFiles;
  for (const QString& file : recentFiles) {
    if (QFileInfo::exists(file)) {
      validFiles.append(file);
    }
  }

  recentFiles = validFiles;
  qDeleteAll(fileItems);
  fileItems.clear();

  for (const QString& file : recentFiles) {
    fileItems.append(new FileItem(file, QFileInfo(file).fileName(), this));
  }

  Q_EMIT fileItemsChanged();
}

void StartView::saveRecentFiles() {
  QSettings settings("TGFXInspector", "Inspector");
  settings.setValue(QStringLiteral("recentFiles"), recentFiles);
  settings.sync();
}

void StartView::updateBroadcastClients() {
  const auto time = tgfx::Clock::Now();
  if (!broadcastListen) {
    bool isListen = false;
    broadcastListen = std::make_unique<tgfx::debug::UdpListen>();
    for (uint16_t i = 0; i < tgfx::debug::BroadcastNum; i++) {
      isListen = broadcastListen->listenSock(port + i);
      if (isListen) {
        break;
      }
    }
    if (!isListen) {
      broadcastListen.reset();
    }
  } else {
    tgfx::debug::IpAddress addr;
    size_t len = 0;
    for (;;) {
      auto broadcastMessage = broadcastListen->readData(len, addr, 0);
      if (!broadcastMessage) {
        break;
      }
      if (len > sizeof(tgfx::debug::BroadcastMessage)) {
        continue;
      }
      tgfx::debug::BroadcastMessage bm = {};
      memcpy(&bm, broadcastMessage, len);
      auto protoVer = bm.protocolVersion;
      char procname[tgfx::debug::WelcomeMessageProgramNameSize] = "";
      strcpy(procname, bm.programName);
      auto activeTime = bm.activeTime;
      auto listenPort = bm.listenPort;
      auto pid = bm.pid;
      auto type = bm.type;

      auto address = addr.getText();
      const auto ipNumerical = addr.getNumber();
      const auto clientId = uint64_t(ipNumerical) | (uint64_t(listenPort) << 32);
      auto it = clients.find(clientId);
      if (activeTime >= 0) {
        if (it == clients.end()) {
          std::string ip(address);
          resolvLock.lock();
          if (resolvMap.find(ip) == resolvMap.end()) {
            resolvMap.emplace(ip, ip);
            resolv->query(ipNumerical, [&, ip](const char* name) {
              std::lock_guard<std::mutex> lock(resolvLock);
              auto iter = resolvMap.find(ip);
              assert(iter != resolvMap.end());
              iter->second = name;
            });
          }
          resolvLock.unlock();
          auto client = new ClientData(
              {time, protoVer, activeTime, listenPort, pid, procname, std::move(ip), type});
          clients.emplace(clientId, client);
          Q_EMIT clientItemsChanged();
        } else {
          auto client = it->second;
          client->data.time = time;
          client->data.activeTime = activeTime;
          client->data.port = listenPort;
          client->data.pid = pid;
          client->data.protocolVersion = protoVer;
          if (strcmp(client->data.procName.c_str(), procname) != 0) {
            client->data.procName = procname;
          }
          client->data.type = type;
        }
      } else if (it != clients.end()) {
        clients.erase(it);
        Q_EMIT clientItemsChanged();
      }
    }

    auto it = clients.begin();
    while (it != clients.end()) {
      const auto diff = time - it->second->data.time;
      if (diff > 4000) {
        it = clients.erase(it);
        Q_EMIT clientItemsChanged();
      } else {
        ++it;
      }
    }
  }
}

void StartView::onCloseView(QObject* view) {
  view->deleteLater();
}

void StartView::onCloseAllView() {
  this->deleteLater();
}
}  // namespace inspector
