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

OpOrTask getOpTaskType(tgfx::debug::OpTaskType type) {
  switch (type) {
    case tgfx::debug::OpTaskType::TextureUploadTask:
    case tgfx::debug::OpTaskType::ShapeBufferUploadTask:
    case tgfx::debug::OpTaskType::GpuUploadTask:
    case tgfx::debug::OpTaskType::TextureCreateTask:
    case tgfx::debug::OpTaskType::RenderTargetCreateTask:
    case tgfx::debug::OpTaskType::TextureFlattenTask:
    case tgfx::debug::OpTaskType::RenderTargetCopyTask:
    case tgfx::debug::OpTaskType::RuntimeDrawTask:
    case tgfx::debug::OpTaskType::TextureResolveTask:
      return OpOrTask::Task;
    case tgfx::debug::OpTaskType::ClearOp:
    case tgfx::debug::OpTaskType::RectDrawOp:
    case tgfx::debug::OpTaskType::RRectDrawOp:
    case tgfx::debug::OpTaskType::ShapeDrawOp:
    case tgfx::debug::OpTaskType::DstTextureCopyOp:
    case tgfx::debug::OpTaskType::ResolveOp:
      return OpOrTask::Op;
    default:
      return OpOrTask::NoType;
  }
}
}  // namespace inspector