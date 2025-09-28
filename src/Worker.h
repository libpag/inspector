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
#include "FrameCaptureMessage.h"
#include "LZ4CompressionHandler.h"
#include "LZ4DecompressionHandler.h"
#include "Protocol.h"
#include "Socket.h"

namespace inspector {
static constexpr size_t MaxDecodeBufferSize = 10 * 1024 * 1024;
class Worker : public QObject {
 public:
  struct NetBuffer {
    int bufferOffset = 0;
    size_t size = 0;
  };

  explicit Worker(std::string& filePath);
  Worker(const char* addr, uint16_t port);
  ~Worker() override;

  bool openFile(const std::string& filePath);
  bool saveFile(const std::string& filePath);
  void queryCaptureFrame();

  int64_t getFrameTime(const FrameData& fd, size_t idx) const;
  int64_t getLastTime() const;
  int64_t getFrameStart(uint32_t index) const;
  int64_t getFrameDrawCall(uint32_t index) const;
  int64_t getFrameTriangles(uint32_t index) const;
  bool getFrameCaptured(uint32_t index) const;
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
  void query(tgfx::inspect::ServerQuery type, uint64_t data, uint32_t extra = 0);
  void queryTerminate();
  bool dispatchProcess(const tgfx::inspect::FrameCaptureMessageItem& ev, const char*& ptr);
  bool process(const tgfx::inspect::FrameCaptureMessageItem& ev);
  void processOperateBegin(const tgfx::inspect::OperateBeginMessage& ev);
  void processOperateEnd(const tgfx::inspect::OperateEndMessage& ev);
  void processOperatePtr(const tgfx::inspect::DrawOpPtrMessage& ev);
  void processAttributeImpl(DataHead& head, std::shared_ptr<tgfx::Data> data);
  void processFloatValue(const tgfx::inspect::AttributeDataFloatMessage& ev);
  void processFloat4Value(const tgfx::inspect::AttributeDataFloat4Message& ev);
  void processIntValue(const tgfx::inspect::AttributeDataIntMessage& ev);
  void processBoolValue(const tgfx::inspect::AttributeDataBoolMessage& ev);
  void processMat4Value(const tgfx::inspect::AttributeDataMat4Message& ev);
  void processEnumValue(const tgfx::inspect::AttributeDataEnumMessage& ev);
  void processUint32Value(const tgfx::inspect::AttributeDataUInt32Message& ev);
  void processColorValue(const tgfx::inspect::AttributeDataUInt32Message& ev);
  void processFrameMark(const tgfx::inspect::FrameMarkMessage& ev);
  void processTextureData(const tgfx::inspect::TextureDataMessage& ev);
  void processTexture(const tgfx::inspect::TextureSamplerMessage& ev, bool isInput);

  void handleValueName(uint64_t name, const char* str, size_t size);
  void addTextureData(const char* data, size_t size);
  void addProgramKey(const char* data, size_t size);
  void addShaderText(std::string shaderCode, size_t index);
  void addVertexShaderText(const char* data, size_t size);
  void addFragmentShaderText(const char* data, size_t size);
  void addUniformInfo(const char*& data, size_t size);
  void addUniformValue(const char*& data, size_t size);
  void addMeshdata(const char*& data, size_t size);

  int64_t tscTime(int64_t tsc) const {
    return int64_t(tsc - dataContext.baseTime);
  }
  int64_t tscTime(uint64_t tsc) const {
    return int64_t(int64_t(tsc) - dataContext.baseTime);
  }

 private:
  tgfx::inspect::Socket sock = {};
  std::string addr = {};
  uint16_t port = 0;

  std::unique_ptr<LZ4DecompressionHandler> lz4Handler = nullptr;
  char* dataBuffer = nullptr;
  int bufferOffset = 0;

  DataContext dataContext = {};

  std::thread workThread;
  std::thread netThread;
  std::atomic<bool> isConnected = false;
  std::atomic<bool> isShutDown = false;
  std::atomic<bool> hasData = false;
  std::atomic<uint8_t> handshake = 0;

  std::atomic<uint64_t> bytes = 0;
  std::atomic<uint64_t> decBytes = 0;

  std::vector<NetBuffer> netRead = {};
  std::mutex netReadLock = {};
  std::condition_variable netReadCv = {};

  int netWriteCnt = 0;
  std::mutex netWriteLock = {};
  std::condition_variable netWriteCv = {};

  std::vector<tgfx::inspect::ServerQueryPacket> serverQueryQueue = {};
  std::vector<tgfx::inspect::ServerQueryPacket> serverQueryQueuePrio = {};
  // Control the rate at which query requests are sent to the server to avoid
  // excessive server pressure caused by sending too many requests
  size_t serverQuerySpaceLeft = 0;
  size_t serverQuerySpaceBase = 0;

  int64_t refTime = -1;
  std::shared_ptr<tgfx::Data> penddingTextureData = nullptr;
  tgfx::BytesKey penddingByteKey = {};
};

}  // namespace inspector