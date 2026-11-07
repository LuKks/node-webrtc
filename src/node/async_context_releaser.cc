#include "src/node/async_context_releaser.h"

namespace node_webrtc {

void AsyncContextReleaser::Release(std::unique_ptr<Napi::AsyncContext> context) {
  {
    std::lock_guard<std::mutex> lock(_contexts_mutex);
    _contexts.push(std::move(context));
  }
  Queue();
}

void AsyncContextReleaser::Execute(Napi::Env) {
  std::queue<std::unique_ptr<Napi::AsyncContext>> contexts;
  std::lock_guard<std::mutex> lock(_contexts_mutex);
  _contexts.swap(contexts);
}

void AsyncContextReleaser::EnsureInitialized(Napi::Env env) {
  auto& addon = *AddonState::Get(env);
  if (!addon.asyncContextReleaser) {
    Napi::HandleScope scope(env);
    auto object = AddonState::Constructor<AsyncContextReleaser>(env).New({});
    addon.asyncContextReleaser = Unwrap(object);
    addon.asyncContextReleaser->Ref();
  }
}

void AsyncContextReleaser::Init(Napi::Env env, Napi::Object) {
  auto func = DefineClass(env, "AsyncContextReleaser", {});
  AddonState::Constructor<AsyncContextReleaser>(env) = Napi::Persistent(func);
}

}  // namespace node_webrtc
