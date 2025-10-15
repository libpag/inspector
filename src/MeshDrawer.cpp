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

#include "MeshDrawer.h"
#include <QOpenGLExtraFunctions>
#include <QSGFlatColorMaterial>
#include <QSGImageNode>

namespace inspector {
MeshDrawer::MeshDrawer(QQuickItem* parent) : QQuickItem(parent), appHost(AppHost::GetAppHost()) {
  setFlag(ItemHasContents, true);
  setFlag(ItemAcceptsInputMethod, true);
  setFlag(ItemIsFocusScope, true);
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true);
  clear();
}

MeshDrawer::~MeshDrawer() {
  clear();
}

void MeshDrawer::clear() {
  indicesProvider.reset();
  postionData.clear();
  matrix = tgfx::Matrix::I();
  bounds =
      tgfx::Rect::MakeLTRB(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                           -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
}

void MeshDrawer::refreshData() {
  clear();
  auto selectOpTask = viewData->selectOpTask;
  if (selectOpTask == -1) {
    return;
  }
  const auto& dataContext = worker->getDataContext();
  auto opTask = dataContext.opTasks[static_cast<size_t>(selectOpTask)];
  auto meshDataIter = dataContext.meshDatas.find(opTask->ptr);
  if (meshDataIter == dataContext.meshDatas.end()) {
    return;
  }
  auto& meshData = meshDataIter->second;
  auto meshDataDivider = MeshDataDivider::MakeFrom(
      static_cast<tgfx::inspect::OpTaskType>(opTask->type), meshData->type, meshData->info);
  indicesProvider = IndicesProvider::MakeFrom(static_cast<tgfx::inspect::OpTaskType>(opTask->type),
                                              meshData->type, meshData->info);

  encodeData(meshData->vertexData, meshDataDivider);
  updateMatrix();
  update();
}

void MeshDrawer::selectMeshIndex(int index) {
  selectIndex = index;
  update();
}

void MeshDrawer::drawWireFrame(QSGNode* root) {
  auto lineNode = new QSGGeometryNode();
  const auto& indices = indicesProvider->indices();
  int vertexCount = 0;
  if (indices.empty()) {
    vertexCount = static_cast<int>(postionData.size());
  } else {
    if (indices.size() % 3 > 0) {
      return;
    }
    auto indicesSize = static_cast<int>(indices.size());
    vertexCount = indicesSize * 2;
  }
  auto geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), vertexCount);
  geometry->setDrawingMode(QSGGeometry::DrawLines);
  geometry->setLineWidth(1.f);
  lineNode->setGeometry(geometry);

  auto mat = new QSGFlatColorMaterial();
  mat->setColor(QColor(221, 221, 211, 255));
  lineNode->setMaterial(mat);
  lineNode->setFlag(QSGNode::OwnsMaterial);

  auto vertices = geometry->vertexDataAsPoint2D();
  if (indices.empty()) {
    for (size_t i = 0; i < postionData.size(); ++i) {
      tgfx::Point pos = {};
      matrix.mapXY(postionData[i][0], postionData[i][1], &pos);
      vertices[i].set(pos.x, pos.y);
    }
  } else {
    size_t i = 0;
    size_t vertexIndex = 0;
    for (; i < indices.size();) {
      tgfx::Point firstPos = {};
      tgfx::Point secondPos = {};
      tgfx::Point thirdPos = {};
      auto firstIndex = indices[i++];
      auto secondIndex = indices[i++];
      auto thirdIndex = indices[i++];
      matrix.mapXY(postionData[firstIndex][0], postionData[firstIndex][1], &firstPos);
      matrix.mapXY(postionData[secondIndex][0], postionData[secondIndex][1], &secondPos);
      matrix.mapXY(postionData[thirdIndex][0], postionData[thirdIndex][1], &thirdPos);
      vertices[vertexIndex++].set(firstPos.x, firstPos.y);
      vertices[vertexIndex++].set(secondPos.x, secondPos.y);
      vertices[vertexIndex++].set(secondPos.x, secondPos.y);
      vertices[vertexIndex++].set(thirdPos.x, thirdPos.y);
      vertices[vertexIndex++].set(thirdPos.x, thirdPos.y);
      vertices[vertexIndex++].set(firstPos.x, firstPos.y);
    }
  }

  lineNode->markDirty(QSGNode::DirtyGeometry);
  root->appendChildNode(lineNode);
}

void MeshDrawer::drawSelectTriangle(QSGNode* root) {
  if (selectIndex < 0) {
    return;
  }
  const auto& indices = indicesProvider->indices();
  auto select = static_cast<size_t>(selectIndex / 3 * 3);
  auto triangleNode = new QSGGeometryNode();
  auto triangleGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 3);
  triangleGeometry->setDrawingMode(GL_TRIANGLES);
  triangleNode->setGeometry(triangleGeometry);
  triangleNode->setFlag(QSGNode::OwnsGeometry);

  auto materialNode = new QSGFlatColorMaterial();
  materialNode->setColor(QColor(255, 0, 0, 200));  // 半透明红色，可修改
  triangleNode->setMaterial(materialNode);
  triangleNode->setFlag(QSGNode::OwnsMaterial);

  auto vertexData = triangleGeometry->vertexDataAsPoint2D();
  for (size_t i = select; i < 3 + select; ++i) {
    tgfx::Point pos = {};
    if (indices.empty()) {
      matrix.mapXY(postionData[i][0], postionData[i][1], &pos);
    } else {
      auto index = indices[i];
      matrix.mapXY(postionData[index][0], postionData[index][1], &pos);
    }
    vertexData[i - select].set(pos.x, pos.y);
  }

  triangleNode->markDirty(QSGNode::DirtyGeometry);
  root->appendChildNode(triangleNode);
}

void MeshDrawer::drawSelectPoint(QSGNode* root) {
  if (selectIndex < 0) {
    return;
  }
  auto select = static_cast<size_t>(selectIndex);
  const auto& indices = indicesProvider->indices();
  tgfx::Point selectPoint = {};
  if (indices.empty()) {
    matrix.mapXY(postionData[select][0], postionData[select][1], &selectPoint);
  } else {
    auto index = indices[select];
    matrix.mapXY(postionData[index][0], postionData[index][1], &selectPoint);
  }

  const auto rectSize = 8.f;
  const auto half = rectSize / 2;
  auto rectNode = new QSGGeometryNode();
  auto rectGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 6);
  rectGeometry->setDrawingMode(GL_TRIANGLES);
  rectNode->setGeometry(rectGeometry);
  rectNode->setFlag(QSGNode::OwnsGeometry);

  auto matRect = new QSGFlatColorMaterial();
  matRect->setColor(QColor(0, 255, 0, 220));
  rectNode->setMaterial(matRect);
  rectNode->setFlag(QSGNode::OwnsMaterial);

  QSGGeometry::Point2D* vdata = rectGeometry->vertexDataAsPoint2D();
  vdata[0].x = float(selectPoint.x - half);
  vdata[0].y = float(selectPoint.y - half);
  vdata[1].x = float(selectPoint.x + half);
  vdata[1].y = float(selectPoint.y - half);
  vdata[2].x = float(selectPoint.x + half);
  vdata[2].y = float(selectPoint.y + half);
  vdata[3].x = float(selectPoint.x - half);
  vdata[3].y = float(selectPoint.y - half);
  vdata[4].x = float(selectPoint.x + half);
  vdata[4].y = float(selectPoint.y + half);
  vdata[5].x = float(selectPoint.x - half);
  vdata[5].y = float(selectPoint.y + half);

  rectNode->markDirty(QSGNode::DirtyGeometry);
  root->appendChildNode(rectNode);
}

QSGNode* MeshDrawer::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
  if (oldNode) {
    delete oldNode;
    oldNode = nullptr;
  }
  auto root = new QSGNode();
  if (indicesProvider == nullptr || postionData.empty()) {
    delete root;
    return nullptr;
  }
  drawWireFrame(root);
  drawSelectTriangle(root);
  drawSelectPoint(root);
  return root;
}

void MeshDrawer::updateMatrix() {
  auto screenSize = std::min(static_cast<int>(width()), static_cast<int>(height()));
  auto size = screenSize - 50;
  size = std::max(size, 50);
  auto meshSize = std::max(bounds.width(), bounds.height());
  auto meshScale = static_cast<float>(size) / meshSize;
  matrix = tgfx::Matrix::MakeTrans(-bounds.left, -bounds.top);
  matrix.postScale(meshScale, meshScale);
  matrix.postTranslate(static_cast<float>(width() - (bounds.width() * meshScale)) / 2,
                       static_cast<float>(height() - bounds.height() * meshScale) / 2);
}

void MeshDrawer::encodeData(const std::shared_ptr<tgfx::Data>& data,
                            const std::shared_ptr<MeshDataDivider>& meshDataDivider) {
  tgfx::DataView dataView(data->bytes(), data->size());
  size_t offset = 0;
  while (offset < data->size()) {
    for (auto& attribute : meshDataDivider->getAttributes()) {
      switch (attribute.format) {
        case MeshDataDivider::Format::Float: {
          offset += sizeof(float);
          break;
        }
        case MeshDataDivider::Format::Float2: {
          std::array<float, 2> pos = {};
          pos[0] = dataView.getFloat(offset);
          offset += sizeof(float);
          pos[1] = dataView.getFloat(offset);
          offset += sizeof(float);
          if (pos[0] < bounds.left) {
            bounds.left = pos[0];
          } else if (pos[0] > bounds.right) {
            bounds.right = pos[0];
          }
          if (pos[1] < bounds.top) {
            bounds.top = pos[1];
          } else if (pos[1] > bounds.bottom) {
            bounds.bottom = pos[1];
          }
          if (attribute.uniformFormat == MeshDataDivider::UniformFormat::Position) {
            postionData.push_back(pos);
          }
          break;
        }
        case MeshDataDivider::Format::Float3: {
          offset += sizeof(float) * 3;
          break;
        }
        case MeshDataDivider::Format::Float4: {
          offset += sizeof(float) * 4;
          break;
        }
        case MeshDataDivider::Format::Half: {
          offset += sizeof(uint16_t);
          break;
        }
        case MeshDataDivider::Format::Half2: {
          offset += sizeof(uint16_t) * 2;
          break;
        }
        case MeshDataDivider::Format::Half3: {
          offset += sizeof(uint16_t) * 3;
          break;
        }
        case MeshDataDivider::Format::Half4: {
          offset += sizeof(uint16_t) * 4;
          break;
        }
        case MeshDataDivider::Format::Int: {
          offset += sizeof(int32_t);
          break;
        }
        case MeshDataDivider::Format::Int2: {
          offset += sizeof(int32_t) * 2;
          break;
        }
        case MeshDataDivider::Format::Int3: {
          offset += sizeof(int32_t) * 3;
          break;
        }
        case MeshDataDivider::Format::Int4: {
          offset += sizeof(int32_t) * 4;
          break;
        }
        case MeshDataDivider::Format::UByteNormalized: {
          offset += sizeof(uint8_t);
          break;
        }
        case MeshDataDivider::Format::UByte2Normalized: {
          offset += sizeof(uint8_t) * 2;
          break;
        }
        case MeshDataDivider::Format::UByte3Normalized: {
          offset += sizeof(uint8_t) * 3;
          break;
        }
        case MeshDataDivider::Format::UByte4Normalized: {
          offset += sizeof(uint8_t) * 4;
          break;
        }
        default:
          break;
      }
    }
  }
}

}  // namespace inspector