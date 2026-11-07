/* Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include <memory>

#include <node-addon-api/napi.h>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/modules/audio_device/include/audio_device.h>

#include "src/functional/maybe.h"
#include "src/node/addon_state.h"

namespace webrtc {

class PeerConnectionFactoryInterface;

}  // namespace webrtc

namespace node_webrtc {

class PeerConnectionFactory
  : public Napi::ObjectWrap<PeerConnectionFactory> {
 public:
  explicit PeerConnectionFactory(const Napi::CallbackInfo&);

  ~PeerConnectionFactory() override;

  /**
   * Get or create the default PeerConnectionFactory. The default uses
   * webrtc::AudioDeviceModule::AudioLayer::kDummyAudio. Call {@link Release} when done.
   */
  static PeerConnectionFactory* GetOrCreateDefault(Napi::Env);

  /**
   * Release a reference to the default PeerConnectionFactory.
   */
  static void Release(AddonState&);

  void Release() { Release(*_addon); }

  /**
   * Get the underlying webrtc::PeerConnectionFactoryInterface.
   */
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory() { return _factory; }

  rtc::NetworkManager* getNetworkManager() { return _networkManager.get(); }

  rtc::PacketSocketFactory* getSocketFactory() { return _socketFactory.get(); }

  static void Init(Napi::Env, Napi::Object);

  static Napi::FunctionReference& constructor(Napi::Env);

  AddonState& addon() const { return *_addon; }

  std::unique_ptr<rtc::Thread> _signalingThread;
  std::unique_ptr<rtc::Thread> _workerThread;

 private:
  AddonState::Ptr _addon;

  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _factory;
  rtc::scoped_refptr<webrtc::AudioDeviceModule> _audioDeviceModule;

  std::unique_ptr<rtc::NetworkManager> _networkManager;
  std::unique_ptr<rtc::PacketSocketFactory> _socketFactory;
};

}  // namespace node_webrtc
