#include "src/node/addon_state.h"

namespace node_webrtc {

bool AddonState::Init(Napi::Env env) {
  auto state = std::make_unique<Ptr>(std::make_shared<AddonState>());
  auto status = napi_set_instance_data(env, state.get(), [](napi_env, void* data, void*) {
    delete static_cast<Ptr*>(data);
  }, nullptr);
  if (status != napi_ok) {
    Napi::Error::New(env, "Could not store WebRTC addon state").ThrowAsJavaScriptException();
    return false;
  }

  auto addon = state->get();
  state.release();
  status = napi_add_env_cleanup_hook(env, BeginShutdown, addon);
  if (status != napi_ok) {
    Napi::Error::New(env, "Could not register WebRTC addon cleanup").ThrowAsJavaScriptException();
    return false;
  }
  return true;
}

const AddonState::Ptr& AddonState::Get(Napi::Env env) {
  return *env.GetInstanceData<Ptr>();
}

void AddonState::BeginShutdown(void* data) {
  auto addon = static_cast<AddonState*>(data);
  {
    std::lock_guard<std::mutex> lock(addon->factoryMutex);
    addon->closing = true;
    addon->defaultFactory = nullptr;
    addon->defaultFactoryReferences = 0;
  }
  addon->asyncContextReleaser = nullptr;
  // Wrappers retain their state for finalization. Drop JS references while
  // the environment is still valid.
  addon->_constructors.clear();
}

}  // namespace node_webrtc
