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

#include "WSSession.h"
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <winsock.h>
#endif

#include "FrameTag.h"
#include "Sha1.h"
#include "tgfx/core/Buffer.h"

namespace inspector {

#define MIN(a, b) ((a) <= (b) ? (a) : (b))

uint64_t ntoh64(uint64_t x) {
  return ntohl(x >> 32) | ((uint64_t)ntohl(x & 0xFFFFFFFFu) << 32);
}

static const uint8_t Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void Base64Encode(void* dst, const void* src, size_t len) {  // thread-safe, re-entrant
  auto d = static_cast<uint32_t*>(dst);
  auto s = static_cast<const uint8_t*>(src);
  const uint8_t* end = s + len;
  while (s < end) {
    auto e = static_cast<uint32_t>(*s++ << 16);
    if (s < end) e |= static_cast<uint32_t>(*s++ << 8);
    if (s < end) e |= static_cast<uint32_t>(*s++);
    *d++ = static_cast<uint32_t>(Base64[e >> 18] | (Base64[(e >> 12) & 0x3F] << 8) |
                                 (Base64[(e >> 6) & 0x3F] << 16) | (Base64[e & 0x3F] << 24));
  }
  size_t pad = (3 - len % 3) % 3;
  char* ptr = reinterpret_cast<char*>(d) - pad;
  for (size_t i = 0; i < pad; i++) {
    ptr[i] = '=';
  }
}

static int GetHttpHeader(const char* headers, const char* header, char* out,
                         int maxBytesOut) {  // thread-safe, re-entrant
  const char* pos = strstr(headers, header);
  if (!pos) return 0;
  pos += strlen(header);
  const char* end = pos;
  while (*end != '\r' && *end != '\n' && *end != '\0') ++end;
  auto numBytesToWrite = MIN((int)(end - pos), maxBytesOut - 1);
  memcpy(out, pos, static_cast<size_t>(numBytesToWrite));
  out[numBytesToWrite] = '\0';
  return (int)(end - pos);
}

bool SendHandshake(tgfx::inspect::Socket* socket, const char* request) {
  const char webSocketGlobalGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";  // 36 characters long
  char key[128 + sizeof(webSocketGlobalGuid)];
  GetHttpHeader(request, "Sec-WebSocket-Key: ", key, sizeof(key) / 2);
  strcat(key, webSocketGlobalGuid);

  char sha1[21];
  SHA1(sha1, key, (int)strlen(key));

  char handshakeMsg[] =
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Accept: 0000000000000000000000000000\r\n"
      "\r\n";

  Base64Encode(strstr(handshakeMsg, "Sec-WebSocket-Accept: ") + strlen("Sec-WebSocket-Accept: "),
               sha1, 20);

  size_t handshakeMsgSize = strlen(handshakeMsg);
  int error = socket->sendData(handshakeMsg, handshakeMsgSize);
  if (error < 0) {
    return false;
  }
  return true;
}

int32_t WSSendMessage(tgfx::inspect::Socket* socket, const void* buf, uint64_t numBytes) {
  uint8_t headerData[sizeof(WebSocketMessageHeader) + 8] = "";
  memset(headerData, 0, sizeof(headerData));
  auto header = reinterpret_cast<WebSocketMessageHeader*>(headerData);
  header->opcode = 0x02;
  header->fin = 1;
  size_t headerBytes = 2;

  if (numBytes < 126) {
    header->payloadLength = static_cast<uint32_t>(numBytes);
  } else if (numBytes <= 65535) {
    header->payloadLength = 126;
    *(uint16_t*)(headerData + headerBytes) = htons((unsigned short)numBytes);
    headerBytes += 2;
  } else {
    header->payloadLength = 127;
    *(uint64_t*)(headerData + headerBytes) = hton64(numBytes);
    headerBytes += 8;
  }

  socket->sendData(headerData, headerBytes);  // header
  socket->sendData(buf, numBytes);            // payload
  return 0;
}

bool WSRecvMessage(tgfx::inspect::Socket* socket, tgfx::Buffer& data, int timeout) {
  int opcode = 0;
  uint64_t currentLength = 0;
  do {
    WebSocketMessageHeader webSocketHead = {};
    if (!socket->readRaw(&webSocketHead, sizeof(webSocketHead), timeout)) {
      continue;
    }

    if (webSocketHead.opcode == WebSocketMessageHeader::OPCODE::CLOSE) {
      break;
    } else if (webSocketHead.opcode == WebSocketMessageHeader::OPCODE::CONTINUE ||
               webSocketHead.opcode == WebSocketMessageHeader::OPCODE::TEXT_FRAME ||
               webSocketHead.opcode == WebSocketMessageHeader::OPCODE::BIN_FRAME) {
      uint64_t length = 0;
      if (webSocketHead.payloadLength == 126) {
        uint16_t len = 0;
        if (!socket->readRaw(&len, sizeof(len), timeout)) {
          break;
        }
        length = ntohs(len);
      } else if (webSocketHead.payloadLength == 127) {
        uint64_t len = 0;
        if (!socket->readRaw(&len, sizeof(len), timeout)) {
          break;
        }
        length = ntoh64(len);
      } else {
        length = webSocketHead.payloadLength;
      }

      char mask[4] = {};
      if (webSocketHead.mask) {
        if (!socket->readRaw(mask, sizeof(mask), timeout)) {
          break;
        }
      }
      std::shared_ptr<tgfx::Data> templateData = nullptr;
      if (currentLength > 0) {
        templateData = data.release();
      }
      data.alloc(currentLength + length);
      if (templateData != nullptr) {
        memcpy(data.data(), templateData->data(), templateData->size());
      }
      templateData.reset();
      if (!socket->readRaw(data.bytes() + currentLength, length, timeout)) {
        break;
      }
      if (webSocketHead.mask) {
        for (size_t i = 0; i < length; ++i) {
          data[currentLength + i] ^= mask[i % 4];
        }
      }
      currentLength += length;

      if (!opcode && webSocketHead.opcode != WebSocketMessageHeader::OPCODE::CONTINUE) {
        opcode = webSocketHead.opcode;
      }
      if (webSocketHead.fin) {
        return true;
      }
    }
  } while (true);
  return false;
}
}  // namespace inspector