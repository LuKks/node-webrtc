/* Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "peer_connection_factory.h"

#include <memory>

#include <webrtc/api/audio_codecs/builtin_audio_decoder_factory.h>
#include <webrtc/api/audio_codecs/builtin_audio_encoder_factory.h>
#include <webrtc/api/audio/create_audio_device_module.h>
#include <webrtc/api/create_peerconnection_factory.h>
#include <webrtc/api/environment/environment_factory.h>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/video_codecs/builtin_video_decoder_factory.h>
#include <webrtc/api/video_codecs/builtin_video_encoder_factory.h>
#include <webrtc/api/video_codecs/video_decoder_factory.h>
#include <webrtc/api/video_codecs/video_encoder_factory.h>
#include <webrtc/modules/audio_device/include/audio_device.h>
#include <webrtc/modules/audio_device/include/fake_audio_device.h>
#include <webrtc/p2p/base/basic_packet_socket_factory.h>
#include <webrtc/rtc_base/ssl_adapter.h>
#include <webrtc/rtc_base/thread.h>


namespace node_webrtc {

Napi::FunctionReference& PeerConnectionFactory::constructor(Napi::Env env) {
  return AddonState::Constructor<PeerConnectionFactory>(env);
}

PeerConnectionFactory::PeerConnectionFactory(const Napi::CallbackInfo& info)
  : Napi::ObjectWrap<PeerConnectionFactory>(info)
  , _addon(AddonState::Get(info.Env())) {
  auto env = info.Env();

  if (!info.IsConstructCall()) {
    Napi::TypeError::New(env, "Use the new operator to construct a PeerConnectionFactory.").ThrowAsJavaScriptException();
    return;
  }

  // TODO(mroberts): Read `audioLayer` from some PeerConnectionFactoryOptions?
  _workerThread = rtc::Thread::CreateWithSocketServer();
  assert(_workerThread);

  if (!_workerThread->SetName("PeerConnectionFactory:workerThread", nullptr)) {
    Napi::Error::New(env, "Failed to name the WebRTC worker thread").ThrowAsJavaScriptException();
    return;
  }

  if (!_workerThread->Start()) {
    Napi::Error::New(env, "Failed to start the WebRTC worker thread").ThrowAsJavaScriptException();
    return;
  }

  _audioDeviceModule = _workerThread->BlockingCall([] {
    return webrtc::CreateAudioDeviceModule(
        webrtc::CreateEnvironment(), webrtc::AudioDeviceModule::kDummyAudio);
  });
  assert(_audioDeviceModule);

  _signalingThread = rtc::Thread::Create();
  assert(_signalingThread);

  if (!_signalingThread->SetName("PeerConnectionFactory:signalingThread", nullptr)) {
    Napi::Error::New(env, "Failed to name the WebRTC signaling thread").ThrowAsJavaScriptException();
    return;
  }

  if (!_signalingThread->Start()) {
    Napi::Error::New(env, "Failed to start the WebRTC signaling thread").ThrowAsJavaScriptException();
    return;
  }

  _factory = webrtc::CreatePeerConnectionFactory(
          _workerThread.get(),
          _workerThread.get(),
          _signalingThread.get(),
          _audioDeviceModule,
          webrtc::CreateBuiltinAudioEncoderFactory(),
          webrtc::CreateBuiltinAudioDecoderFactory(),
          webrtc::CreateBuiltinVideoEncoderFactory(),
          webrtc::CreateBuiltinVideoDecoderFactory(),
          nullptr,
          nullptr);
  assert(_factory);

  webrtc::PeerConnectionFactoryInterface::Options options;
  options.network_ignore_mask = 0;
  _factory->SetOptions(options);

  _networkManager = std::unique_ptr<rtc::NetworkManager>(
      new webrtc::BasicNetworkManager(webrtc::CreateEnvironment(), _workerThread->socketserver()));
  assert(_networkManager != nullptr);

  _socketFactory = std::unique_ptr<rtc::PacketSocketFactory>(
      new webrtc::BasicPacketSocketFactory(_workerThread->socketserver()));
  assert(_socketFactory != nullptr);
}

PeerConnectionFactory::~PeerConnectionFactory() {
  _factory = nullptr;

  _workerThread->BlockingCall([this]() {
    this->_audioDeviceModule = nullptr;
  });

  _workerThread->Stop();
  _signalingThread->Stop();

  _workerThread = nullptr;
  _signalingThread = nullptr;

  _networkManager = nullptr;
  _socketFactory = nullptr;
}

PeerConnectionFactory* PeerConnectionFactory::GetOrCreateDefault(Napi::Env env) {
  auto& addon = *AddonState::Get(env);
  std::lock_guard<std::mutex> lock(addon.factoryMutex);
  if (!addon.defaultFactory) {
    Napi::HandleScope scope(env);
    auto object = constructor(env).New({});
    addon.defaultFactory = Unwrap(object);
    addon.defaultFactory->Ref();
  }
  ++addon.defaultFactoryReferences;
  return addon.defaultFactory;
}

void PeerConnectionFactory::Release(AddonState& addon) {
  std::lock_guard<std::mutex> lock(addon.factoryMutex);
  if (addon.closing) {
    return;
  }
  assert(addon.defaultFactory && addon.defaultFactoryReferences > 0);
  if (--addon.defaultFactoryReferences == 0) {
    addon.defaultFactory->Unref();
    addon.defaultFactory = nullptr;
  }
}

void PeerConnectionFactory::Init(Napi::Env env, Napi::Object exports) {
  // libwebrtc shares SSL initialization across Node environments.
  static const auto initialized = webrtc::InitializeSSL();
  assert(initialized);
  (void) initialized;

  auto func = DefineClass(env, "RTCPeerConnectionFactory", {});

  constructor(env) = Napi::Persistent(func);

  exports.Set("RTCPeerConnectionFactory", func);
}

}  // namespace node_webrtc
