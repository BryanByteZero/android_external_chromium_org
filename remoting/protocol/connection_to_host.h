// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_CONNECTION_TO_HOST_H_
#define REMOTING_PROTOCOL_CONNECTION_TO_HOST_H_

#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "base/timer/timer.h"
#include "remoting/jingle_glue/signal_strategy.h"
#include "remoting/proto/internal.pb.h"
#include "remoting/protocol/clipboard_filter.h"
#include "remoting/protocol/errors.h"
#include "remoting/protocol/input_filter.h"
#include "remoting/protocol/message_reader.h"
#include "remoting/protocol/monitored_video_stub.h"
#include "remoting/protocol/session.h"
#include "remoting/protocol/session_manager.h"

namespace pp {
class Instance;
}  // namespace pp

namespace remoting {

class XmppProxy;
class VideoPacket;

namespace protocol {

class AudioReader;
class AudioStub;
class Authenticator;
class ClientControlDispatcher;
class ClientEventDispatcher;
class ClientStub;
class ClipboardStub;
class HostStub;
class InputStub;
class SessionConfig;
class TransportFactory;
class VideoReader;
class VideoStub;

class ConnectionToHost : public SignalStrategy::Listener,
                         public SessionManager::Listener,
                         public Session::EventHandler,
                         public base::NonThreadSafe {
 public:
  // The UI implementations maintain corresponding definitions of this
  // enumeration in webapp/client_session.js and
  // android/java/src/org/chromium/chromoting/jni/JniInterface.java. Be sure to
  // update these locations if you make any changes to the ordering.
  enum State {
    INITIALIZING,
    CONNECTING,
    AUTHENTICATED,
    CONNECTED,
    FAILED,
    CLOSED,
  };

  class HostEventCallback {
   public:
    virtual ~HostEventCallback() {}

    // Called when state of the connection changes.
    virtual void OnConnectionState(State state, ErrorCode error) = 0;

    // Called when ready state of the connection changes. When |ready|
    // is set to false some data sent by the peers may be
    // delayed. This is used to indicate in the UI when connection is
    // temporarily broken.
    virtual void OnConnectionReady(bool ready) = 0;

    // Called when the route type (direct vs. STUN vs. proxied) changes.
    virtual void OnRouteChanged(const std::string& channel_name,
                                const protocol::TransportRoute& route) = 0;
  };

  ConnectionToHost(bool allow_nat_traversal);
  virtual ~ConnectionToHost();

  // |signal_strategy| must outlive connection. |audio_stub| may be
  // null, in which case audio will not be requested.
  virtual void Connect(SignalStrategy* signal_strategy,
                       const std::string& host_jid,
                       const std::string& host_public_key,
                       scoped_ptr<TransportFactory> transport_factory,
                       scoped_ptr<Authenticator> authenticator,
                       HostEventCallback* event_callback,
                       ClientStub* client_stub,
                       ClipboardStub* clipboard_stub,
                       VideoStub* video_stub,
                       AudioStub* audio_stub);

  virtual const SessionConfig& config();

  // Stubs for sending data to the host.
  virtual ClipboardStub* clipboard_stub();
  virtual HostStub* host_stub();
  virtual InputStub* input_stub();

  // SignalStrategy::StatusObserver interface.
  virtual void OnSignalStrategyStateChange(
      SignalStrategy::State state) OVERRIDE;
  virtual bool OnSignalStrategyIncomingStanza(
      const buzz::XmlElement* stanza) OVERRIDE;

  // SessionManager::Listener interface.
  virtual void OnSessionManagerReady() OVERRIDE;
  virtual void OnIncomingSession(
      Session* session,
      SessionManager::IncomingSessionResponse* response) OVERRIDE;

  // Session::EventHandler interface.
  virtual void OnSessionStateChange(Session::State state) OVERRIDE;
  virtual void OnSessionRouteChange(const std::string& channel_name,
                                    const TransportRoute& route) OVERRIDE;

  // MonitoredVideoStub::EventHandler interface.
  virtual void OnVideoChannelStatus(bool active);

  // Return the current state of ConnectionToHost.
  State state() const;

 private:
  // Callbacks for channel initialization
  void OnChannelInitialized(bool successful);

  void NotifyIfChannelsReady();

  void CloseOnError(ErrorCode error);

  // Stops writing in the channels.
  void CloseChannels();

  void SetState(State state, ErrorCode error);

  bool allow_nat_traversal_;

  std::string host_jid_;
  std::string host_public_key_;
  scoped_ptr<Authenticator> authenticator_;

  HostEventCallback* event_callback_;

  // Stub for incoming messages.
  ClientStub* client_stub_;
  ClipboardStub* clipboard_stub_;
  AudioStub* audio_stub_;

  SignalStrategy* signal_strategy_;
  scoped_ptr<SessionManager> session_manager_;
  scoped_ptr<Session> session_;
  scoped_ptr<MonitoredVideoStub> monitored_video_stub_;

  scoped_ptr<VideoReader> video_reader_;
  scoped_ptr<AudioReader> audio_reader_;
  scoped_ptr<ClientControlDispatcher> control_dispatcher_;
  scoped_ptr<ClientEventDispatcher> event_dispatcher_;
  ClipboardFilter clipboard_forwarder_;
  InputFilter event_forwarder_;

  // Internal state of the connection.
  State state_;
  ErrorCode error_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ConnectionToHost);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_CONNECTION_TO_HOST_H_
