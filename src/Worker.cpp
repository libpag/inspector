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

#include "Worker.h"
#include <tgfx/core/WriteStream.h>
#include <cassert>
#include <iostream>
#include "DecodeStream.h"
#include "EncodeStream.h"
#include "FileTags.h"
#include "Protocol.h"
#include "TagHeader.h"
#include "lz4.h"
#include "tgfx/core/Data.h"
#include "tgfx/core/ImageCodec.h"

namespace inspector {
static constexpr size_t ServerQueryPacketSize = sizeof(tgfx::inspect::ServerQueryPacket);

tgfx::ColorType PixelFormatToColorType(tgfx::PixelFormat format) {
  switch (format) {
    case tgfx::PixelFormat::RGBA_8888:
      return tgfx::ColorType::RGBA_8888;
    case tgfx::PixelFormat::ALPHA_8:
      return tgfx::ColorType::ALPHA_8;
    case tgfx::PixelFormat::BGRA_8888:
      return tgfx::ColorType::BGRA_8888;
    case tgfx::PixelFormat::GRAY_8:
      return tgfx::ColorType::Gray_8;
    default:
      return tgfx::ColorType::Unknown;
  }
}

static bool IsQueryPrio(tgfx::inspect::ServerQuery type) {
  return type < tgfx::inspect::ServerQuery::Disconnect;
}

Worker::Worker(const char* addr, uint16_t port)
    : addr(addr), port(port), lz4Handler(LZ4DecompressionHandler::Make()),
      dataBuffer(new char[MaxDecodeBufferSize]) {
  workThread = std::thread([this] { exec(); });
  netThread = std::thread([this] { netWork(); });
}

Worker::Worker(std::string& filePath) {
  openFile(filePath);
}

Worker::~Worker() {
  shutdown();
  if (netThread.joinable()) {
    netThread.join();
  }
  if (workThread.joinable()) {
    workThread.join();
  }
  if (dataBuffer) {
    delete[] dataBuffer;
  }
}

bool Worker::openFile(const std::string& filePath) {
  auto data = tgfx::Data::MakeFromFile(filePath);
  DecodeStream stream(&dataContext, data->bytes(), static_cast<uint32_t>(data->size()));
  auto body = readBodyBytes(&stream);
  if (dataContext.hasException()) {
    return false;
  }
  ReadTags(&body, ReadTagsOfFile);
  if (dataContext.hasException()) {
    return false;
  }

  return true;
}

bool Worker::saveFile(const std::string& filePath) {
  EncodeStream bodyBytes(&dataContext);
  WriteTagsOfFile(&bodyBytes);

  EncodeStream fileBytes(&dataContext);
  fileBytes.writeInt8('T');
  fileBytes.writeInt8('G');
  fileBytes.writeInt8('F');
  fileBytes.writeInt8('X');
  fileBytes.writeUint8(tgfx::inspect::ProtocolVersion);
  fileBytes.writeEncodedUint32(bodyBytes.length());
  fileBytes.writeBytes(&bodyBytes);
  auto data = fileBytes.release();

  auto writeStream = tgfx::WriteStream::MakeFromFile(filePath);
  if (!writeStream) {
    return false;
  }
  writeStream->write(data->bytes(), data->size());
  return true;
}

DecodeStream Worker::readBodyBytes(DecodeStream* stream) {
  DecodeStream emptyStream(stream->context);
  auto T = stream->readInt8();
  auto G = stream->readInt8();
  auto F = stream->readInt8();
  auto X = stream->readInt8();
  if (T != 'T' || G != 'G' || F != 'F' || X != 'X') {
    InspectorThrowError(stream->context, "Invalid ISP file header");
    return emptyStream;
  }

  auto version = stream->readUint8();
  if (version > tgfx::inspect::ProtocolVersion) {
    InspectorThrowError(stream->context, "Isp file version is too high");
    return emptyStream;
  }
  auto bodyLength = stream->readEncodedUint32();
  bodyLength = std::min(bodyLength, stream->bytesAvailable());
  return stream->readBytes(bodyLength);
}

void Worker::queryCaptureFrame() {
  tgfx::inspect::ServerQueryPacket query{tgfx::inspect::ServerQuery::CaptureFrame, 0, 10};
  sock.sendData(&query, ServerQueryPacketSize);
}

int64_t Worker::getFrameTime(const FrameData& fd, size_t idx) const {
  if (fd.continuous) {
    if (idx < fd.frames.size() - 1) {
      return (fd.frames[idx + 1].start - fd.frames[idx].start) * 1000;
    }
    assert(dataContext.lastTime != 0);
    return (dataContext.lastTime - fd.frames.back().start) * 1000;
  }
  const auto& frame = fd.frames[idx];
  if (frame.end >= 0) {
    return frame.end - frame.start;
  }
  return (dataContext.lastTime - fd.frames.back().start) * 1000;
}

int64_t Worker::getLastTime() const {
  return dataContext.lastTime;
}

int64_t Worker::getFrameStart(uint32_t index) const {
  return dataContext.frameData.frames[index].start;
}

int64_t Worker::getFrameDrawCall(uint32_t index) const {
  return dataContext.frameData.frames[index].drawCall;
}

int64_t Worker::getFrameTriangles(uint32_t index) const {
  return dataContext.frameData.frames[index].triangles;
}

bool Worker::getFrameCaptured(uint32_t index) const {
  return dataContext.frameData.frames[index].captured;
}

FrameData* Worker::getFrameData() {
  return &dataContext.frameData;
}

const DataContext& Worker::getDataContext() const {
  return dataContext;
}

size_t Worker::getFrameCount() const {
  return dataContext.frameData.frames.size();
}

bool Worker::hasExpection() const {
  return dataContext.hasException();
}

std::vector<std::string>& Worker::getErrorMessage() {
  return dataContext.errorMessages;
}

void Worker::shutdown() {
  isShutDown.store(true, std::memory_order_relaxed);
}

#define CLOSE_EXEC                                     \
  shutdown();                                          \
  sock.socketClose();                                  \
  netWriteCv.notify_one();                             \
  isConnected.store(false, std::memory_order_relaxed); \
  return;

void Worker::exec() {
  auto ShouldExit = [this] { return isShutDown.load(std::memory_order_relaxed); };

  while (true) {
    if (isShutDown.load(std::memory_order_relaxed)) {
      return;
    }
    if (sock.connectAddress(addr.c_str(), port)) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  sock.sendData(tgfx::inspect::HandshakeShibboleth, tgfx::inspect::HandshakeShibbolethSize);
  uint32_t protocolVersion = tgfx::inspect::ProtocolVersion;
  sock.sendData(&protocolVersion, sizeof(protocolVersion));
  tgfx::inspect::HandshakeStatus handshake;
  if (!sock.readData(&handshake, sizeof(handshake), 10, ShouldExit)) {
    this->handshake.store(static_cast<uint8_t>(tgfx::inspect::HandshakeStatus::HandshakeDropped),
                          std::memory_order_relaxed);
    CLOSE_EXEC;
  }
  this->handshake.store(static_cast<uint8_t>(handshake), std::memory_order_relaxed);
  switch (handshake) {
    case tgfx::inspect::HandshakeStatus::HandshakeWelcome:
      break;
    case tgfx::inspect::HandshakeStatus::HandshakeProtocolMismatch:
    case tgfx::inspect::HandshakeStatus::HandshakeNotAvailable:
    default:
      CLOSE_EXEC;
  }

  {
    tgfx::inspect::WelcomeMessage welcome{};
    if (!sock.readData(&welcome, sizeof(welcome), 10, ShouldExit)) {
      this->handshake.store(static_cast<uint8_t>(tgfx::inspect::HandshakeStatus::HandshakeDropped),
                            std::memory_order_relaxed);
      CLOSE_EXEC;
    }
    dataContext.baseTime = welcome.initBegin;
    const auto initEnd = tscTime(welcome.initEnd);
    dataContext.frameData.frames.push_back(FrameEvent{false, 0, -1, 0, 0});
    dataContext.frameData.frames.push_back(FrameEvent{false, initEnd, -1, 0, 0});
    dataContext.lastTime = initEnd;
    refTime = welcome.refTime;
  }
  // leave space for terminate request
  serverQuerySpaceLeft = serverQuerySpaceBase = size_t(
      std::min(sock.getSendBufferSize() / static_cast<int>(ServerQueryPacketSize), 8 * 1024) - 4);
  hasData.store(true, std::memory_order_release);

  isConnected.store(true, std::memory_order_relaxed);
  {
    std::lock_guard<std::mutex> lock(netWriteLock);
    netWriteCnt = 2;
    netWriteCv.notify_one();
  }

  while (true) {
    if (isShutDown.load(std::memory_order_relaxed)) {
      queryTerminate();
      CLOSE_EXEC;
    }

    NetBuffer netbuf = {};
    {
      std::unique_lock<std::mutex> lock(netReadLock);
      netReadCv.wait(lock, [this] { return !netRead.empty(); });
      netbuf = netRead.front();
      netRead.erase(netRead.begin());
    }

    if (netbuf.bufferOffset < 0) {
      CLOSE_EXEC;
    }

    const char* ptr = dataBuffer + netbuf.bufferOffset;
    const char* end = ptr + netbuf.size;

    {
      std::lock_guard<std::mutex> lock(dataContext.lock);
      while (ptr < end) {
        auto ev = (const tgfx::inspect::FrameCaptureMessageItem*)ptr;
        if (!dispatchProcess(*ev, ptr)) {
          queryTerminate();
          CLOSE_EXEC;
        }
      }

      {
        std::lock_guard<std::mutex> lockNet(netWriteLock);
        ++netWriteCnt;
        netWriteCv.notify_one();
      }

      if (serverQuerySpaceLeft > 0 && !serverQueryQueuePrio.empty()) {
        const auto toSend = std::min(serverQuerySpaceLeft, serverQueryQueuePrio.size());
        sock.sendData(serverQueryQueuePrio.data(), toSend * ServerQueryPacketSize);
        serverQuerySpaceLeft -= toSend;
        if (toSend == serverQueryQueuePrio.size()) {
          serverQueryQueuePrio.clear();
        } else {
          serverQueryQueuePrio.erase(serverQueryQueuePrio.begin(),
                                     serverQueryQueuePrio.begin() + long(toSend));
        }
      }
      if (serverQuerySpaceLeft > 0 && !serverQueryQueue.empty()) {
        const auto toSend = std::min(serverQuerySpaceLeft, serverQueryQueue.size());
        sock.sendData(serverQueryQueue.data(), toSend * ServerQueryPacketSize);
        serverQuerySpaceLeft -= toSend;
        if (toSend == serverQueryQueue.size()) {
          serverQueryQueue.clear();
        } else {
          serverQueryQueue.erase(serverQueryQueue.begin(), serverQueryQueue.begin() + long(toSend));
        }
      }
    }
  }
}

#define CLOSE_NETWORK                            \
  std::lock_guard<std::mutex> lock(netReadLock); \
  netRead.push_back(NetBuffer{-1, 0});           \
  netReadCv.notify_one();                        \
  return;

void Worker::netWork() {
  auto ShouldExit = [this] { return isShutDown.load(std::memory_order_relaxed); };

  tgfx::Buffer lz4Buffer = {};
  while (true) {
    {
      std::unique_lock<std::mutex> lock(netWriteLock);
      netWriteCv.wait(
          lock, [this] { return netWriteCnt > 0 || isShutDown.load(std::memory_order_relaxed); });
      if (isShutDown.load(std::memory_order_relaxed)) {
        CLOSE_NETWORK;
      }
      netWriteCnt--;
    }

    auto buf = dataBuffer + bufferOffset;
    bool isLz4Encode = false;
    size_t lz4Size = 0;
    if (!sock.readData(&isLz4Encode, sizeof(bool), 10, ShouldExit)) {
      CLOSE_NETWORK;
    }
    if (!sock.readData(&lz4Size, sizeof(lz4Size), 10, ShouldExit)) {
      CLOSE_NETWORK;
    }
    if (lz4Buffer.size() < lz4Size) {
      lz4Buffer.alloc(lz4Size);
      if (lz4Buffer.isEmpty()) {
        CLOSE_NETWORK;
      }
    }
    if (!sock.readData(lz4Buffer.bytes(), lz4Size, 10, ShouldExit)) {
      CLOSE_NETWORK;
    }
    auto size = lz4Size;
    if (isLz4Encode) {
      size = lz4Handler->decode(reinterpret_cast<uint8_t*>(buf), MaxDecodeBufferSize,
                                lz4Buffer.bytes(), lz4Size);
    } else {
      memcpy(buf, lz4Buffer.bytes(), lz4Size);
    }
    {
      std::lock_guard<std::mutex> lock(netReadLock);
      netRead.push_back(NetBuffer{bufferOffset, size});
      netReadCv.notify_one();
    }

    bufferOffset += size;
    if (bufferOffset > tgfx::inspect::TargetFrameSize * 2) {
      bufferOffset = 0;
    }
  }
}

void Worker::newOpTask(std::shared_ptr<OpTaskData> opTask) {
  ++dataContext.opTaskCount;

  auto& stack = dataContext.opTaskStack;
  const auto size = stack.size();
  opTask->id = static_cast<uint32_t>(dataContext.opTasks.size());
  dataContext.opTasks.push_back(opTask);
  if (size != 0) {
    auto& back = stack.back();
    if (dataContext.opChilds.find(back->id) == dataContext.opChilds.end()) {
      dataContext.opChilds[back->id] = std::vector<uint32_t>{opTask->id};
    } else {
      dataContext.opChilds[back->id].push_back(opTask->id);
    }
  }
  stack.push_back(opTask);
}

void Worker::query(tgfx::inspect::ServerQuery type, uint64_t data, uint32_t extra) {
  tgfx::inspect::ServerQueryPacket query{type, data, extra};
  if (serverQuerySpaceLeft > 0 && serverQueryQueuePrio.empty() && serverQueryQueue.empty()) {
    serverQuerySpaceLeft--;
    sock.sendData(&query, ServerQueryPacketSize);
  } else if (IsQueryPrio(type)) {
    serverQueryQueuePrio.push_back(query);
  } else {
    serverQueryQueue.push_back(query);
  }
}

void Worker::queryTerminate() {
  tgfx::inspect::ServerQueryPacket query{tgfx::inspect::ServerQuery::Terminate, 0, 0};
  sock.sendData(&query, ServerQueryPacketSize);
}

bool Worker::dispatchProcess(const tgfx::inspect::FrameCaptureMessageItem& ev, const char*& ptr) {
  if (ev.hdr.idx >= static_cast<uint8_t>(tgfx::inspect::FrameCaptureMessageType::StringData)) {
    ptr += sizeof(tgfx::inspect::FrameCaptureMessageHeader) +
           sizeof(tgfx::inspect::StringTransferMessage);
    if (ev.hdr.type == tgfx::inspect::FrameCaptureMessageType::PixelsData) {
      uint32_t size = 0;
      memcpy(&size, ptr, sizeof(size));
      ptr += sizeof(size);
      addTextureData(ptr, size);
      ptr += size;
    } else if (ev.hdr.type == tgfx::inspect::FrameCaptureMessageType::UniformInfoData) {
      uint16_t size = 0;
      memcpy(&size, ptr, sizeof(size));
      ptr += sizeof(size);
      addUniformInfo(ptr, size);
    } else if (ev.hdr.type == tgfx::inspect::FrameCaptureMessageType::UniformValueData) {
      uint16_t size = 0;
      memcpy(&size, ptr, sizeof(size));
      ptr += sizeof(size);
      addUniformValue(ptr, size);
    } else if (ev.hdr.type == tgfx::inspect::FrameCaptureMessageType::MeshData) {
      uint32_t size = 0;
      memcpy(&size, ptr, sizeof(size));
      ptr += sizeof(size);
      addMeshdata(ptr, size);
    } else {
      uint16_t size = 0;
      memcpy(&size, ptr, sizeof(size));
      ptr += sizeof(size);
      switch (ev.hdr.type) {
        case tgfx::inspect::FrameCaptureMessageType::StringData: {
          serverQuerySpaceLeft++;
          break;
        }
        case tgfx::inspect::FrameCaptureMessageType::ValueName: {
          handleValueName(ev.stringTransfer.ptr, ptr, size);
          serverQuerySpaceLeft++;
          break;
        }
        case tgfx::inspect::FrameCaptureMessageType::ProgramKeyData: {
          addProgramKey(ptr, size);
          break;
        }
        case tgfx::inspect::FrameCaptureMessageType::VertexShaderTextData: {
          addVertexShaderText(ptr, size);
          break;
        }
        case tgfx::inspect::FrameCaptureMessageType::FragmentShaderTextData: {
          addFragmentShaderText(ptr, size);
          break;
        }
        default: {
          break;
        }
      }
      ptr += size;
    }
    return true;
  }
  ptr += tgfx::inspect::FrameCaptureMessageDataSize[ev.hdr.idx];
  return process(ev);
}

bool Worker::process(const tgfx::inspect::FrameCaptureMessageItem& ev) {
  switch (ev.hdr.type) {
    case tgfx::inspect::FrameCaptureMessageType::OperateBegin:
      processOperateBegin(ev.operateBegin);
      break;
    case tgfx::inspect::FrameCaptureMessageType::OperateEnd:
      processOperateEnd(ev.operateEnd);
      break;
    case tgfx::inspect::FrameCaptureMessageType::ValueDataUint32:
      processUint32Value(ev.attributeDataUint32);
      break;
    case tgfx::inspect::FrameCaptureMessageType::ValueDataFloat4:
      processFloat4Value(ev.attributeDataFloat4);
      break;
    case tgfx::inspect::FrameCaptureMessageType::ValueDataMat3:
      processMat4Value(ev.attributeDataMat4);
      break;
    case tgfx::inspect::FrameCaptureMessageType::ValueDataInt:
      processIntValue(ev.attributeDataInt);
      break;
    case tgfx::inspect::FrameCaptureMessageType::ValueDataColor:
      processColorValue(ev.attributeDataUint32);
      break;
    case tgfx::inspect::FrameCaptureMessageType::ValueDataFloat:
      processFloatValue(ev.attributeDataFloat);
      break;
    case tgfx::inspect::FrameCaptureMessageType::ValueDataBool:
      processBoolValue(ev.attributeDataBool);
      break;
    case tgfx::inspect::FrameCaptureMessageType::ValueDataEnum:
      processEnumValue(ev.attributeDataEnum);
      break;
    case tgfx::inspect::FrameCaptureMessageType::FrameMarkMessage:
      processFrameMark(ev.frameMark);
      break;
    case tgfx::inspect::FrameCaptureMessageType::TextureData:
      processTextureData(ev.textureData);
      break;
    case tgfx::inspect::FrameCaptureMessageType::InputTexture:
      processTexture(ev.textureSampler, true);
      break;
    case tgfx::inspect::FrameCaptureMessageType::OutputTexture:
      processTexture(ev.textureSampler, false);
      break;
    case tgfx::inspect::FrameCaptureMessageType::OperatePtr:
      processOperatePtr(ev.drawOpPtrMessage);
      break;
    case tgfx::inspect::FrameCaptureMessageType::KeepAlive:
    default:
      break;
  }
  return true;
}

static int64_t RefTime(int64_t& reference, int64_t delta) {
  const auto refTime = delta - reference;
  if (refTime == 0) {
    reference = delta;
  }
  return refTime;
}

void Worker::processOperateBegin(const tgfx::inspect::OperateBeginMessage& ev) {
  std::shared_ptr<OpTaskData> opTask(new OpTaskData);
  const auto start = tscTime(RefTime(refTime, ev.usTime));
  opTask->start = start;
  opTask->end = -1;
  opTask->type = ev.type;

  newOpTask(std::move(opTask));
}

void Worker::processOperateEnd(const tgfx::inspect::OperateEndMessage& ev) {
  auto& stack = dataContext.opTaskStack;
  if (stack.empty()) {
    return;
  }
  auto opTask = stack.back();
  stack.pop_back();
  assert(opTask->end == -1);
  assert(opTask->type == ev.type);
  const auto timeEnd = tscTime(RefTime(refTime, ev.usTime));
  opTask->end = timeEnd;
  assert(timeEnd >= opTask->start);
}

void Worker::processOperatePtr(const tgfx::inspect::DrawOpPtrMessage& ev) {
  auto& stack = dataContext.opTaskStack;
  if (stack.empty()) {
    return;
  }
  auto opTask = stack.back();
  opTask->ptr = ev.drawOpPtr;
}

void Worker::processAttributeImpl(DataHead& head, std::shared_ptr<tgfx::Data> data) {
  auto& stack = dataContext.opTaskStack;
  if (stack.empty()) {
    return;
  }
  auto opTask = stack.back();
  auto& nameMap = dataContext.nameMap;
  auto propertyIter = dataContext.properties.find(opTask->id);
  std::shared_ptr<PropertyData> propertyData;
  if (propertyIter == dataContext.properties.end()) {
    propertyData = std::make_shared<PropertyData>();
  } else {
    propertyData = propertyIter->second;
  }
  if (nameMap.find(head.name) == nameMap.end()) {
    query(tgfx::inspect::ServerQuery::ValueName, head.name);
  }
  propertyData->summaryName.push_back(head);
  auto& summaryData = propertyData->summaryData;
  summaryData.push_back(std::move(data));
  dataContext.properties[opTask->id] = propertyData;
}

void Worker::processFloatValue(const tgfx::inspect::AttributeDataFloatMessage& ev) {
  auto head = DataHead{DataType::Float, ev.name};
  auto data = tgfx::Data::MakeWithCopy(&ev.value, sizeof(float));
  processAttributeImpl(head, std::move(data));
}

void Worker::processFloat4Value(const tgfx::inspect::AttributeDataFloat4Message& ev) {
  auto head = DataHead{DataType::Vec4, ev.name};
  auto data = tgfx::Data::MakeWithCopy(ev.value, sizeof(float) * 4);
  processAttributeImpl(head, std::move(data));
}

void Worker::processIntValue(const tgfx::inspect::AttributeDataIntMessage& ev) {
  auto head = DataHead{DataType::Int, ev.name};
  auto data = tgfx::Data::MakeWithCopy(&ev.value, sizeof(int));
  processAttributeImpl(head, std::move(data));
}

void Worker::processBoolValue(const tgfx::inspect::AttributeDataBoolMessage& ev) {
  auto head = DataHead{DataType::Bool, ev.name};
  auto data = tgfx::Data::MakeWithCopy(&ev.value, sizeof(bool));
  processAttributeImpl(head, std::move(data));
}

void Worker::processMat4Value(const tgfx::inspect::AttributeDataMat4Message& ev) {
  auto head = DataHead{DataType::Mat4, ev.name};
  auto data = tgfx::Data::MakeWithCopy(ev.value, sizeof(float) * 6);
  processAttributeImpl(head, std::move(data));
}

void Worker::processEnumValue(const tgfx::inspect::AttributeDataEnumMessage& ev) {
  auto head = DataHead{DataType::Enum, ev.name};
  auto data = tgfx::Data::MakeWithCopy(&ev.value, sizeof(uint16_t));
  processAttributeImpl(head, std::move(data));
}

void Worker::processUint32Value(const tgfx::inspect::AttributeDataUInt32Message& ev) {
  auto head = DataHead{DataType::Uint32, ev.name};
  auto data = tgfx::Data::MakeWithCopy(&ev.value, sizeof(uint32_t));
  processAttributeImpl(head, std::move(data));
}

void Worker::processColorValue(const tgfx::inspect::AttributeDataUInt32Message& ev) {
  auto head = DataHead{DataType::Color, ev.name};
  auto data = tgfx::Data::MakeWithCopy(&ev.value, sizeof(uint32_t));
  processAttributeImpl(head, std::move(data));
}

void Worker::processFrameMark(const tgfx::inspect::FrameMarkMessage& ev) {
  auto& fd = dataContext.frameData;

  const auto time = tscTime(ev.usTime);
  fd.frames.push_back(FrameEvent{ev.captured, time, -1, 0, 0});
  if (dataContext.lastTime < time) {
    dataContext.lastTime = time;
  }
}

void Worker::processTextureData(const tgfx::inspect::TextureDataMessage& ev) {
  auto& images = dataContext.images;
  auto pixelsIter = images.find(ev.textureId);
  if (pixelsIter != images.end()) {
    penddingTextureData.reset();
    return;
  }
  auto image = std::make_shared<ImageTexture>();
  image->isInput = ev.isInput;
  image->format = ev.format;
  image->width = ev.width;
  image->height = ev.height;
  image->rowBytes = ev.rowBytes;
  image->data = std::move(penddingTextureData);
  images[ev.textureId] = std::move(image);
}

void Worker::processTexture(const tgfx::inspect::TextureSamplerMessage& ev, bool isInput) {
  auto& stack = dataContext.opTaskStack;
  if (stack.empty()) {
    return;
  }
  auto opTask = stack.back();
  auto& textures = dataContext.textures;
  std::shared_ptr<TextureData> textureData = nullptr;
  auto texture = textures.find(opTask->id);
  if (texture == textures.end()) {
    textureData = std::make_shared<TextureData>();
    textures[opTask->id] = textureData;
  } else {
    textureData = texture->second;
  }
  if (isInput) {
    textureData->inputTextures.push_back(ev.textureId);
  } else {
    textureData->outputTexture = ev.textureId;
  }
}

void Worker::handleValueName(uint64_t name, const char* str, size_t size) {
  auto& nameMap = dataContext.nameMap;
  if (nameMap.find(name) == nameMap.end()) {
    nameMap[name] = std::string(str, size);
  }
}

void Worker::addTextureData(const char* data, size_t size) {
  ASSERT(penddingTextureData == nullptr);
  auto imageData = tgfx::Data::MakeWithCopy(data, size);
  penddingTextureData = std::move(imageData);
}

void Worker::addProgramKey(const char* data, size_t size) {
  if (size == 0 || size % 4 != 0) {
    return;
  }
  auto& stack = dataContext.opTaskStack;
  if (stack.empty()) {
    return;
  }
  auto opTask = stack.back();
  auto& programKeys = dataContext.programKeys;
  auto value = (uint32_t*)data;
  auto valueSize = size / 4;
  auto programKey = tgfx::BytesKey(valueSize);
  for (size_t i = 0; i < valueSize; ++i) {
    programKey.write(value[i]);
  }
  programKeys[opTask->id] = programKey;

  auto& shaderData = dataContext.shaderData;
  if (shaderData.find(programKey) != shaderData.end()) {
    return;
  }
  shaderData[programKey] = {};
  penddingByteKey = programKey;
}

void Worker::addShaderText(std::string shaderCode, size_t index) {
  auto& shaderData = dataContext.shaderData;
  if (shaderData.find(penddingByteKey) == shaderData.end()) {
    return;
  }
  shaderData[penddingByteKey].shaderText[index] = std::move(shaderCode);
}

void Worker::addVertexShaderText(const char* data, size_t size) {
  auto shaderCode = std::string(data, size);
  addShaderText(std::move(shaderCode), 0);
}

void Worker::addFragmentShaderText(const char* data, size_t size) {
  auto shaderCode = std::string(data, size);
  addShaderText(std::move(shaderCode), 1);
}

template <class T>
static void ReadExtraData(const char*& data, T& val, size_t size = 0) {
  auto dataSize = sizeof(val);
  if (size > 0) {
    dataSize = size;
  }
  memcpy(&val, data, dataSize);
  data += dataSize;
}

void Worker::addUniformInfo(const char*& data, size_t size) {
  auto name = std::string(data, size);
  data += size;
  uint16_t formatLen = 0;
  UniformFormat format = UniformFormat::Float;
  ReadExtraData(data, formatLen);
  ReadExtraData(data, format, formatLen);

  auto& shaderData = dataContext.shaderData;
  if (shaderData.find(penddingByteKey) == shaderData.end()) {
    return;
  }
  shaderData[penddingByteKey].uniforms[name] = format;
}

void Worker::addUniformValue(const char*& data, size_t size) {
  auto name = std::string(data, size);
  data += size;
  uint16_t valueLen = 0;
  ReadExtraData(data, valueLen);
  if (size == 0 || valueLen == 0) {
    return;
  }
  auto valueData = tgfx::Data::MakeWithCopy(data, valueLen);
  data += valueLen;

  auto& stack = dataContext.opTaskStack;
  if (stack.empty()) {
    return;
  }
  auto opTask = stack.back();
  auto& uniformValues = dataContext.uniformValues;
  auto uniformValueIter = uniformValues.find(opTask->id);
  if (uniformValueIter == uniformValues.end()) {
    uniformValues[opTask->id] = {};
  }
  auto& values = uniformValues[opTask->id];
  values.emplace_back(UniformValueData{std::move(name), std::move(valueData)});
}

void Worker::addMeshdata(const char*& data, size_t size) {
  if (size == 0) {
    return;
  }
  auto vertexData = tgfx::Data::MakeWithCopy(data, size);
  data += size;

  uint32_t extraDataSize = 0;
  uint8_t meshType = 0;
  ReadExtraData(data, extraDataSize);
  ReadExtraData(data, meshType);
  std::shared_ptr<tgfx::inspect::MeshInfo> meshInfo = nullptr;
  if (static_cast<tgfx::inspect::VertexProviderType>(meshType) ==
      tgfx::inspect::VertexProviderType::RectsVertexProvider) {
    tgfx::inspect::RectMeshInfo rectMeshInfo = {};
    ReadExtraData(data, rectMeshInfo);
    meshInfo = std::make_shared<tgfx::inspect::RectMeshInfo>(rectMeshInfo);
  } else {
    tgfx::inspect::RRectMeshInfo rrectMeshInfo = {};
    ReadExtraData(data, rrectMeshInfo);
    meshInfo = std::make_shared<tgfx::inspect::RRectMeshInfo>(rrectMeshInfo);
  }
  auto drawOpPtr = meshInfo->drawOpPtr;
  auto& meshDatas = dataContext.meshDatas;
  if (meshDatas.find(drawOpPtr) != meshDatas.end()) {
    return;
  }
  auto meshData = std::make_shared<MeshData>();
  meshData->vertexData = std::move(vertexData);
  meshData->type = static_cast<tgfx::inspect::VertexProviderType>(meshType);
  meshData->info = std::move(meshInfo);
  meshDatas[drawOpPtr] = std::move(meshData);
}
}  // namespace inspector
