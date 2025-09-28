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

#pragma once
#include <QAbstractItemModel>
#include "IndicesProvider.h"
#include "MeshDataDivider.h"
#include "ViewData.h"
#include "Worker.h"
#include "tgfx/core/Clock.h"

namespace inspector {
class MeshModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QList<QString> names READ getNames NOTIFY itemsChanged)
  Q_PROPERTY(QList<QList<QList<QVariant>>> values READ getValues NOTIFY itemsChanged)
  Q_PROPERTY(size_t itemSize READ getItemSize NOTIFY itemsChanged)
  Q_PROPERTY(QList<int> itemValueCount READ getItemValueCount NOTIFY itemsChanged)
  Q_PROPERTY(int selectMeshItem READ getSelectMeshItem NOTIFY selectChanged)
 public:
  MeshModel(Worker* worker, ViewData* viewData, QObject* parent = nullptr);
  ~MeshModel() override;

  void reset();
  Q_INVOKABLE size_t getItemSize() const;
  Q_INVOKABLE QList<int> getItemValueCount() const;
  Q_INVOKABLE int getSelectMeshItem() const;
  Q_INVOKABLE QList<QString> getNames() const;
  Q_INVOKABLE QList<QList<QList<QVariant>>> getValues();

  Q_INVOKABLE void setSelectMeshItem(int index);
  Q_SLOT void refreshData();
  Q_SIGNAL void itemsChanged();
  Q_SIGNAL void selectChanged();
  Q_SIGNAL void selectMeshIndex(int index);

 protected:
  void refreshItems(const std::shared_ptr<tgfx::Data>& data,
                    const std::shared_ptr<IndicesProvider>& indicesProvider);
  QList<QList<QList<QVariant>>> encodeData(const std::shared_ptr<tgfx::Data>& data) const;

 private:
  Worker* worker = nullptr;
  ViewData* viewData = nullptr;
  std::shared_ptr<MeshDataDivider> meshDataDivider = {};
  QList<QList<QList<QVariant>>> items = {};
  int selectMeshItem = -1;
};
}  // namespace inspector