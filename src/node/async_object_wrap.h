#pragma once

#include <initializer_list>
#include <mutex>

#include <node-addon-api/napi.h>

#include "src/node/async_context_releaser.h"

namespace node_webrtc {

template <typename T>
class AsyncObjectWrap: public Napi::ObjectWrap<T> {
 private:
  AddonState::Ptr _addon;
  std::unique_ptr<Napi::AsyncContext> _async_context;
  std::mutex _async_context_mutex;

  void DestroyAsyncContext() {
    std::lock_guard<std::mutex> lock(_async_context_mutex);
    if (auto releaser = _addon->asyncContextReleaser) {
      releaser->Release(std::move(_async_context));
    }
  }

 public:
  AsyncObjectWrap(
      const char* name,
      const Napi::CallbackInfo& info):
    Napi::ObjectWrap<T>(info),
    _addon(AddonState::Get(info.Env())),
    _async_context(std::make_unique<Napi::AsyncContext>(info.Env(), name, this->Value())) {
    AsyncContextReleaser::EnsureInitialized(info.Env());
  }

  ~AsyncObjectWrap() override {
    DestroyAsyncContext();
  }

  Napi::AsyncContext* context() {
    return _async_context.get();
  }

 protected:
  AddonState& addon() const { return *_addon; }

  void MakeCallback(const char* name, const std::initializer_list<napi_value>& args) {
    auto self = this->Value();
    auto maybeFunction = self.Get(name);
    if (maybeFunction.IsFunction()) {
      _async_context_mutex.lock();
      if (_async_context) {
        napi_async_context context = *_async_context;
        maybeFunction.template As<Napi::Function>().MakeCallback(self, args, context);
      }
      _async_context_mutex.unlock();
    }
  }
};

}  // namespace node_webrtc
