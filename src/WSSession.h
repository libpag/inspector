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

#include <Socket.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "tgfx/core/Buffer.h"

namespace inspector {
uint64_t ntoh64(uint64_t x);
#define hton64 ntoh64

typedef struct
#if defined(__GNUC__)
    __attribute__((packed, aligned(1)))
#endif
    WebSocketMessageHeader {
  enum OPCODE {
    /// 数据分片帧
    CONTINUE = 0,
    /// 文本帧
    TEXT_FRAME = 1,
    /// 二进制帧
    BIN_FRAME = 2,
    /// 断开连接
    CLOSE = 8,
    /// PING
    PING = 0x9,
    /// PONG
    PONG = 0xA
  };
  unsigned opcode : 4;
  unsigned rsv : 3;
  unsigned fin : 1;
  unsigned payloadLength : 7;
  unsigned mask : 1;
} WebSocketMessageHeader;

bool SendHandshake(tgfx::inspect::Socket* socket, const char* request);
bool WSRecvMessage(tgfx::inspect::Socket* socket, tgfx::Buffer& frameData, int timeout);
int32_t WSSendMessage(tgfx::inspect::Socket* socket, const void* buf, uint64_t numBytes);

}  // namespace inspector
