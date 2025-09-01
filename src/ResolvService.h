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

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace inspector {

class ResolvService {
  struct QueueItem {
    uint32_t ip = 0;
    std::function<void(const char*)> callback = nullptr;
  };

 public:
  explicit ResolvService(uint16_t port);

  ~ResolvService();

  void query(uint32_t ip, const std::function<void(const char*)>& callback);

 private:
  void worker();

  std::atomic<bool> exit = false;
  std::mutex mutex = {};
  std::condition_variable conditionVariable = {};
  std::vector<QueueItem> queue = {};
  uint16_t port = 0;
  std::unique_ptr<std::thread> thread = nullptr;
};
}  // namespace inspector
