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

#include "InspectorEvent.h"

namespace inspector {

OpOrTask getOpTaskType(tgfx::inspect::OpTaskType type) {
  switch (type) {
    case tgfx::inspect::OpTaskType::TextureUploadTask:
    case tgfx::inspect::OpTaskType::ShapeBufferUploadTask:
    case tgfx::inspect::OpTaskType::GpuUploadTask:
    case tgfx::inspect::OpTaskType::TextureCreateTask:
    case tgfx::inspect::OpTaskType::RenderTargetCreateTask:
    case tgfx::inspect::OpTaskType::TextureFlattenTask:
    case tgfx::inspect::OpTaskType::RenderTargetCopyTask:
    case tgfx::inspect::OpTaskType::RuntimeDrawTask:
    case tgfx::inspect::OpTaskType::TextureResolveTask:
      return OpOrTask::Task;
    case tgfx::inspect::OpTaskType::ClearOp:
    case tgfx::inspect::OpTaskType::RectDrawOp:
    case tgfx::inspect::OpTaskType::RRectDrawOp:
    case tgfx::inspect::OpTaskType::ShapeDrawOp:
    case tgfx::inspect::OpTaskType::AtlasTextOp:
    case tgfx::inspect::OpTaskType::DstTextureCopyOp:
    case tgfx::inspect::OpTaskType::ResolveOp:
      return OpOrTask::Op;
    default:
      return OpOrTask::NoType;
  }
}
}  // namespace inspector