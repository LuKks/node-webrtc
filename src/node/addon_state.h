#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include <node-addon-api/napi.h>

namespace node_webrtc {

class AsyncContextReleaser;
class PeerConnectionFactory;

class AddonState {
 public:
  using Ptr = std::shared_ptr<AddonState>;

  static bool Init(Napi::Env env);
  static const Ptr& Get(Napi::Env env);

  template <typename T>
  static Napi::FunctionReference& Constructor(Napi::Env env) {
    return Get(env)->_constructors[Key<T>()];
  }

  template <typename T, typename Create>
  T* Wrapper(Create create) {
    std::lock_guard<std::mutex> lock(_wrapperMutex);
    auto& wrapper = _wrappers[Key<T>()];
    if (!wrapper) {
      wrapper = std::make_shared<T>(create);
    }
    return static_cast<T*>(wrapper.get());
  }

  AsyncContextReleaser* asyncContextReleaser = nullptr;
  PeerConnectionFactory* defaultFactory = nullptr;
  int defaultFactoryReferences = 0;
  std::mutex factoryMutex;
  bool closing = false;

 private:
  template <typename T>
  static const void* Key() {
    static const char key = 0;
    return &key;
  }

  static void BeginShutdown(void* data);

  std::unordered_map<const void*, Napi::FunctionReference> _constructors;
  std::unordered_map<const void*, std::shared_ptr<void>> _wrappers;
  std::mutex _wrapperMutex;
};

}  // namespace node_webrtc
