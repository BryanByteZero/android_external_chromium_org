// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines the interface for peer-to-peer transport. There
// are two types of transport: StreamTransport and DatagramTransport.
// They must both be created using TransportFactory instances and they
// provide the same interface, except that one should be used for
// reliable stream connection and the other one for unreliable
// datagram connection. The Transport interface itself doesn't provide
// methods to send/receive data. Instead it creates an instance of
// net::Socket or net::SocketStream which provides access to the data
// channel. After a new transport is Initialize()'ed the Connect()
// method must be called. Connect() starts asynchronous creation and
// initialization of the connection socket that can be used later to
// send and receive data. The socket is passed to the callback
// specified in the Connect() call. The Transport object must exist
// during the whole lifetime of the connection socket. Later deletion
// of the connection socket causes teardown of the corresponding
// Transport object.

#ifndef REMOTING_PROTOCOL_TRANSPORT_H_
#define REMOTING_PROTOCOL_TRANSPORT_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "net/base/ip_endpoint.h"

namespace cricket {
class Candidate;
}  // namespace cricket

namespace net {
class Socket;
class StreamSocket;
}  // namespace net

namespace remoting {
namespace protocol {

class ChannelAuthenticator;

struct TransportRoute {
  enum RouteType {
    DIRECT,
    STUN,
    RELAY,
  };

  // Helper method to get string representation of the type.
  static std::string GetTypeString(RouteType type);

  TransportRoute();
  ~TransportRoute();

  RouteType type;
  net::IPEndPoint remote_address;
  net::IPEndPoint local_address;
};

class Transport : public base::NonThreadSafe {
 public:
  class EventHandler {
   public:
    EventHandler() {};
    virtual ~EventHandler() {};

    // Called when the transport generates a new candidate that needs
    // to be passed to the AddRemoteCandidate() method on the remote
    // end of the connection.
    virtual void OnTransportCandidate(Transport* transport,
                                      const cricket::Candidate& candidate) = 0;

    // Called when transport route changes. Can be called even before
    // the transport is connected.
    virtual void OnTransportRouteChange(Transport* transport,
                                        const TransportRoute& route) = 0;

    // Called when the transport inactivity state changes. When
    // |ready| is set to false incoming and outgoing data may be
    // delayed until connection goes back to the active state, at
    // which point that method is called again with |ready| set to
    // true. This is useful for UI indication of temporarily broken
    // connections.
    virtual void OnTransportReady(Transport* transport, bool ready) = 0;

    // Called when when the transport has failed to connect or reconnect.
    virtual void OnTransportFailed(Transport* transport) = 0;

    // Called when the transport is about to be deleted.
    virtual void OnTransportDeleted(Transport* transport) = 0;
  };

  Transport() {}
  virtual ~Transport() {}

  // Intialize the transport with the specified parameters.
  // |authenticator| is used to secure and authenticate the connection.
  virtual void Initialize(const std::string& name,
                          Transport::EventHandler* event_handler,
                          scoped_ptr<ChannelAuthenticator> authenticator) = 0;

  // Adds |candidate| received from the peer.
  virtual void AddRemoteCandidate(const cricket::Candidate& candidate) = 0;

  // Name of the channel. It is used to identify the channel and
  // disambiguate candidates it generates from candidates generated by
  // parallel connections.
  virtual const std::string& name() const = 0;

  // Returns true if the channel is already connected.
  virtual bool is_connected() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Transport);
};

class StreamTransport : public Transport {
 public:
  typedef base::Callback<void(scoped_ptr<net::StreamSocket>)> ConnectedCallback;

  StreamTransport() { }
  virtual ~StreamTransport() { }

  virtual void Connect(const ConnectedCallback& callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(StreamTransport);
};

class DatagramTransport : public Transport {
 public:
  typedef base::Callback<void(scoped_ptr<net::Socket>)> ConnectedCallback;

  DatagramTransport() { }
  virtual ~DatagramTransport() { }

  virtual void Connect(const ConnectedCallback& callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(DatagramTransport);
};

class TransportFactory {
 public:
  TransportFactory() { }
  virtual ~TransportFactory() { }

  // Called to notify transport factory that a new transport might be created
  // soon, e.g. when a new session is being created. Implementation may use it
  // to start asynchronous preparation, e.g. fetch a new relay token if
  // necessary while the session is being authenticated.
  virtual void PrepareTokens() = 0;

  virtual scoped_ptr<StreamTransport> CreateStreamTransport() = 0;
  virtual scoped_ptr<DatagramTransport> CreateDatagramTransport() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TransportFactory);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_TRANSPORT_H_
