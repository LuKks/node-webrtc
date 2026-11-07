/* Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include <mutex>

#include "src/utilities/bidi_map.h"

namespace node_webrtc {

template<typename T, typename U, typename ...V>
class Wrap {
 public:
  Wrap() = delete;

  explicit Wrap(T(*create)(V..., U)): _create(create) {}

  Wrap(Wrap const&) = delete;

  Wrap& operator=(Wrap const&) = delete;

  T GetOrCreate(V... args, U key) {
    if (auto wrapper = Get(key)) {
      return wrapper;
    }
    // Only the owning Node thread creates wrappers. Creation can wait on a
    // WebRTC thread that also reads this map.
    auto wrapper = _create(args..., key);
    std::lock_guard<std::mutex> lock(_mutex);
    _map.set(key, wrapper);
    return wrapper;
  }

  T Get(U key) {
    std::lock_guard<std::mutex> lock(_mutex);
    return _map.get(key).FromMaybe(nullptr);
  }

  void Release(T value) {
    std::unique_lock<std::mutex> lock(_mutex);
    // Dropping the last native reference can wait on a WebRTC thread.
    [[maybe_unused]] auto key = _map.reverseRemove(value);
    lock.unlock();
  }

 private:
  T(*_create)(V..., U);
  BidiMap<U, T> _map;
  std::mutex _mutex;
};

}  // namespace node_webrtc
