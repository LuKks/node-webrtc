#pragma once

#include <mutex>
#include <queue>

#include <node-addon-api/napi.h>

#include "src/node/addon_state.h"
#include "src/node/deferrer.h"

namespace node_webrtc {

class AsyncContextReleaser
  : public Napi::ObjectWrap<AsyncContextReleaser>
  , private Deferrer {
 public:
  AsyncContextReleaser(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<AsyncContextReleaser>(info)
    , Deferrer(info.Env())
    , _addon(AddonState::Get(info.Env())) {}

  ~AsyncContextReleaser() override {
    if (_addon->asyncContextReleaser == this) {
      _addon->asyncContextReleaser = nullptr;
    }
  }

  static void EnsureInitialized(Napi::Env);
  static void Init(Napi::Env, Napi::Object);

  void Release(std::unique_ptr<Napi::AsyncContext>);

 protected:
  void Execute(Napi::Env) override;

 private:
  AddonState::Ptr _addon;
  std::queue<std::unique_ptr<Napi::AsyncContext>> _contexts;
  std::mutex _contexts_mutex{};
};

}  // namespace node_webrtc
