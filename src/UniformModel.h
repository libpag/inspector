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

#include <QAbstractItemModel>
#include "ViewData.h"
#include "Worker.h"

namespace inspector {
class UniformItem : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ getName CONSTANT)
  Q_PROPERTY(QString format READ getFormat CONSTANT)
  Q_PROPERTY(QVariant value READ getValue CONSTANT)
  Q_PROPERTY(int height READ getHeight CONSTANT)
 public:
  UniformItem(QString name, QString format, QVariant value, int height, QObject* parent = nullptr)
      : QObject(parent), name(std::move(name)), format(std::move(format)), value(std::move(value)),
        height(height) {
  }

  QString getName() const {
    return name;
  }

  QString getFormat() const {
    return format;
  }

  QVariant getValue() const {
    return value;
  }

  int getHeight() const {
    return height;
  }

 private:
  QString name = "";
  QString format = "";
  QVariant value = 0;
  int height = 0;
};

class UniformModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QList<QObject*> uniformItems READ getUniformItems NOTIFY itemsChanged)
 public:
  UniformModel(Worker* worker, ViewData* viewData, QObject* parent = nullptr);
  ~UniformModel() override;
  QString toUniformFormatName(UniformFormat format);
  int getFormatHeight(UniformFormat format);

  void clearItems();
  Q_INVOKABLE QList<QObject*> getUniformItems() const;
  Q_SLOT void refreshData();
  Q_SIGNAL void itemsChanged();

 protected:
  QVariant readData(std::shared_ptr<tgfx::Data> value, UniformFormat format);

 private:
  Worker* worker = nullptr;
  ViewData* viewData = nullptr;
  std::vector<std::shared_ptr<UniformItem>> uniformItems = {};
};
}  // namespace inspector