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
#include <QObject>
#include <cstdint>
#include <string>
#include <thread>
#include "DataContext.h"
#include "DecodeStream.h"
#include "LZ4CompressionHandler.h"
#include "Message.h"
#include "Protocol.h"
#include "Socket.h"

namespace inspector {
static constexpr size_t MaxDecodeBufferSize = 10 * 1024 * 1024;
class Worker : public QObject {
 public:
  struct NetBuffer {
    int bufferOffset;
    size_t size;
  };

  explicit Worker(std::string& filePath);
  Worker(const char* addr, uint16_t port);
  ~Worker() override;

  bool openFile(const std::string& filePath);
  bool saveFile(const std::string& filePath);

  int64_t getFrameTime(const FrameData& fd, size_t idx) const;
  int64_t getLastTime() const;
  int64_t getFrameStart(uint32_t index) const;
  int64_t getFrameDrawCall(uint32_t index) const;
  int64_t getFrameTriangles(uint32_t index) const;
  size_t getFrameCount() const;
  bool hasExpection() const;
  std::vector<std::string>& getErrorMessage();

  FrameData* getFrameData();
  const DataContext& getDataContext() const;

 protected:
  DecodeStream readBodyBytes(DecodeStream* stream);

  void shutdown();
  void exec();
  void netWork();

  void newOpTask(std::shared_ptr<OpTaskData> opTask);
  void query(tgfx::debug::ServerQuery type, uint64_t data, uint32_t extra = 0);
  void queryTerminate();
  bool dispatchProcess(const tgfx::debug::MsgItem& ev, const char*& ptr);
  bool process(const tgfx::debug::MsgItem& ev);
  void processOperateBegin(const tgfx::debug::OperateBeginMsg& ev);
  void processOperateEnd(const tgfx::debug::OperateEndMsg& ev);
  void processAttributeImpl(DataHead& head, std::shared_ptr<tgfx::Data> data);
  void processFloatValue(const tgfx::debug::AttributeDataFloatMsg& ev);
  void processFloat4Value(const tgfx::debug::AttributeDataFloat4Msg& ev);
  void processIntValue(const tgfx::debug::AttributeDataIntMsg& ev);
  void processBoolValue(const tgfx::debug::AttributeDataBoolMsg& ev);
  void processMat4Value(const tgfx::debug::AttributeDataMat4Msg& ev);
  void processEnumValue(const tgfx::debug::AttributeDataEnumMsg& ev);
  void processUint32Value(const tgfx::debug::AttributeDataUInt32Msg& ev);
  void processColorValue(const tgfx::debug::AttributeDataUInt32Msg& ev);
  void processFrameMark(const tgfx::debug::FrameMarkMsg& ev);
  void processTextureData(const tgfx::debug::TextureDataMsg& ev);
  void processTexture(const tgfx::debug::TextureSamplerMsg& ev);

  void handleValueName(uint64_t name, const char* str, size_t sz);
  void addTextureData(const char* data, size_t sz);

  int64_t tscTime(int64_t tsc) const {
    return int64_t(tsc - dataContext.baseTime);
  }
  int64_t tscTime(uint64_t tsc) const {
    return int64_t(int64_t(tsc) - dataContext.baseTime);
  }

 private:
  tgfx::debug::Socket sock = {};
  std::string addr = {};
  uint16_t port = 0;

  std::unique_ptr<tgfx::debug::LZ4CompressionHandler> lz4Handler = nullptr;
  // void* lz4Stream;
  char* dataBuffer = nullptr;
  int bufferOffset = 0;

  DataContext dataContext = {};

  std::thread workThread;
  std::thread netThread;
  std::atomic<bool> isConnected{false};
  std::atomic<bool> isShutDown{false};
  std::atomic<bool> hasData{false};
  std::atomic<uint8_t> handshake{0};

  std::atomic<uint64_t> bytes{0};
  std::atomic<uint64_t> decBytes{0};

  std::vector<NetBuffer> netRead;
  std::mutex netReadLock;
  std::condition_variable netReadCv;

  int netWriteCnt = 0;
  std::mutex netWriteLock;
  std::condition_variable netWriteCv;

  std::vector<tgfx::debug::ServerQueryPacket> serverQueryQueue;
  std::vector<tgfx::debug::ServerQueryPacket> serverQueryQueuePrio;
  // Control the rate at which query requests are sent to the server to avoid
  // excessive server pressure caused by sending too many requests
  size_t serverQuerySpaceLeft = 0;
  size_t serverQuerySpaceBase = 0;

  int64_t refTime = 0;
  std::shared_ptr<tgfx::Data> penddingTextureData = nullptr;
};

}  // namespace inspector