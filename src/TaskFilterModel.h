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

namespace inspector {
class TaskFilterItem {
 public:
  explicit TaskFilterItem(const char* data, int32_t type, TaskFilterItem* parentItem = nullptr);

  void appendChild(std::unique_ptr<TaskFilterItem>& child);
  TaskFilterItem* child(uint32_t row);
  int childCount() const;
  int columnCount() const;
  int row() const;
  const std::string& data() const;
  int32_t filterType() const;
  TaskFilterItem* getParentItem() const;
  int32_t childFilterType(int depth = -1) const;

 private:
  int32_t type;
  std::vector<std::unique_ptr<TaskFilterItem>> childItems;
  std::string itemData;
  TaskFilterItem* parentItem;
};

class TaskFilterModel : public QAbstractItemModel {
  Q_OBJECT
  Q_PROPERTY(int filterType READ getFilterType NOTIFY filterTypeChange)
 public:
  enum Roles {
    NameRole = Qt::UserRole + 1,
    FilterTypeRole,
  };
  explicit TaskFilterModel(ViewData* viewData, QObject* parent = nullptr);
  ~TaskFilterModel() override;

  int getFilterType() const;
  Q_INVOKABLE void checkedItem(const QModelIndex& index, bool checked);
  Q_INVOKABLE void setTextFilter(const QString& text);
  Q_SIGNAL void filterTypeChange();

  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  QModelIndex index(int row, int column, const QModelIndex& parent) const override;
  QModelIndex parent(const QModelIndex& child) const override;
  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;

 protected:
  void checkedParentItem(const TaskFilterItem* item, bool checked);
  void checkedChildrenItem(const TaskFilterItem* item, bool checked);

 private:
  void setUpModelData();
  ViewData* viewData = nullptr;
  std::unique_ptr<TaskFilterItem> rootItem;
};
}  // namespace inspector