// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_MDNS_CLIENT_IMPL_H_
#define NET_DNS_MDNS_CLIENT_IMPL_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/cancelable_callback.h"
#include "base/observer_list.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/dns/mdns_cache.h"
#include "net/dns/mdns_client.h"
#include "net/udp/datagram_server_socket.h"
#include "net/udp/udp_server_socket.h"
#include "net/udp/udp_socket.h"

namespace net {

// A connection to the network for multicast DNS clients. It reads data into
// DnsResponse objects and alerts the delegate that a packet has been received.
class NET_EXPORT_PRIVATE MDnsConnection {
 public:
  class SocketFactory {
   public:
    virtual ~SocketFactory() {}

    virtual scoped_ptr<DatagramServerSocket> CreateSocket() = 0;

    static scoped_ptr<SocketFactory> CreateDefault();
  };

  class Delegate {
   public:
    // Handle an mDNS packet buffered in |response| with a size of |bytes_read|.
    virtual void HandlePacket(DnsResponse* response, int bytes_read) = 0;
    virtual void OnConnectionError(int error) = 0;
    virtual ~Delegate() {}
  };

  explicit MDnsConnection(SocketFactory* socket_factory,
                          MDnsConnection::Delegate* delegate);

  virtual ~MDnsConnection();

  int Init();
  int Send(IOBuffer* buffer, unsigned size);

 private:
  class SocketHandler {
   public:
    SocketHandler(MDnsConnection* connection,
                  const IPEndPoint& multicast_addr,
                  SocketFactory* socket_factory);
    ~SocketHandler();
    int DoLoop(int rv);
    int Start();

    int Send(IOBuffer* buffer, unsigned size);

   private:
    int BindSocket();
    void OnDatagramReceived(int rv);

    // Callback for when sending a query has finished.
    void SendDone(int rv);

    scoped_ptr<DatagramServerSocket> socket_;

    MDnsConnection* connection_;
    IPEndPoint recv_addr_;
    scoped_ptr<DnsResponse> response_;
    IPEndPoint multicast_addr_;
  };

  // Callback for handling a datagram being received on either ipv4 or ipv6.
  void OnDatagramReceived(DnsResponse* response,
                          const IPEndPoint& recv_addr,
                          int bytes_read);

  void OnError(SocketHandler* loop, int error);

  IPEndPoint GetMDnsIPEndPoint(const char* address);

  SocketHandler socket_handler_ipv4_;
  SocketHandler socket_handler_ipv6_;

  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(MDnsConnection);
};

class MDnsListenerImpl;

class NET_EXPORT_PRIVATE MDnsClientImpl : public MDnsClient {
 public:
  // The core object exists while the MDnsClient is listening, and is deleted
  // whenever the number of listeners reaches zero. The deletion happens
  // asychronously, so destroying the last listener does not immediately
  // invalidate the core.
  class Core : public base::SupportsWeakPtr<Core>, MDnsConnection::Delegate {
   public:
    Core(MDnsClientImpl* client,
         MDnsConnection::SocketFactory* socket_factory);
    virtual ~Core();

    // Initialize the core. Returns true on success.
    bool Init();

    // Send a query with a specific rrtype and name. Returns true on success.
    bool SendQuery(uint16 rrtype, std::string name);

    // Add/remove a listener to the list of listeners.
    void AddListener(MDnsListenerImpl* listener);
    void RemoveListener(MDnsListenerImpl* listener);

    // Query the cache for records of a specific type and name.
    void QueryCache(uint16 rrtype, const std::string& name,
                    std::vector<const RecordParsed*>* records) const;

    // Parse the response and alert relevant listeners.
    virtual void HandlePacket(DnsResponse* response, int bytes_read) OVERRIDE;

    virtual void OnConnectionError(int error) OVERRIDE;

   private:
    typedef std::pair<std::string, uint16> ListenerKey;
    typedef std::map<ListenerKey, ObserverList<MDnsListenerImpl>* >
    ListenerMap;

    // Alert listeners of an update to the cache.
    void AlertListeners(MDnsListener::UpdateType update_type,
                        const ListenerKey& key, const RecordParsed* record);

    // Schedule a cache cleanup to a specific time, cancelling other cleanups.
    void ScheduleCleanup(base::Time cleanup);

    // Clean up the cache and schedule a new cleanup.
    void DoCleanup();

    // Callback for when a record is removed from the cache.
    void OnRecordRemoved(const RecordParsed* record);

    void NotifyNsecRecord(const RecordParsed* record);

    // Delete and erase the observer list for |key|. Only deletes the observer
    // list if is empty.
    void CleanupObserverList(const ListenerKey& key);

    ListenerMap listeners_;

    MDnsClientImpl* client_;
    MDnsCache cache_;

    base::CancelableCallback<void()> cleanup_callback_;
    base::Time scheduled_cleanup_;

    scoped_ptr<MDnsConnection> connection_;

    DISALLOW_COPY_AND_ASSIGN(Core);
  };

  explicit MDnsClientImpl(
      scoped_ptr<MDnsConnection::SocketFactory> socket_factory_);
  virtual ~MDnsClientImpl();

  // MDnsClient implementation:
  virtual scoped_ptr<MDnsListener> CreateListener(
      uint16 rrtype,
      const std::string& name,
      MDnsListener::Delegate* delegate) OVERRIDE;

  virtual scoped_ptr<MDnsTransaction> CreateTransaction(
      uint16 rrtype,
      const std::string& name,
      int flags,
      const MDnsTransaction::ResultCallback& callback) OVERRIDE;

  // Returns true when the client is listening for network packets.
  bool IsListeningForTests();

  bool AddListenRef();
  void SubtractListenRef();

  Core* core() { return core_.get(); }

 private:
  // This method causes the client to stop listening for packets. The
  // call for it is deferred through the message loop after the last
  // listener is removed. If another listener is added after a
  // shutdown is scheduled but before it actually runs, the shutdown
  // will be canceled.
  void Shutdown();

  scoped_ptr<Core> core_;
  int listen_refs_;

  scoped_ptr<MDnsConnection::SocketFactory> socket_factory_;

  DISALLOW_COPY_AND_ASSIGN(MDnsClientImpl);
};

class MDnsListenerImpl : public MDnsListener,
                         public base::SupportsWeakPtr<MDnsListenerImpl> {
 public:
  MDnsListenerImpl(uint16 rrtype,
                   const std::string& name,
                   MDnsListener::Delegate* delegate,
                   MDnsClientImpl* client);

  virtual ~MDnsListenerImpl();

  // MDnsListener implementation:
  virtual bool Start() OVERRIDE;

  virtual const std::string& GetName() const OVERRIDE;

  virtual uint16 GetType() const OVERRIDE;

  MDnsListener::Delegate* delegate() { return delegate_; }

  // Alert the delegate of a record update.
  void AlertDelegate(MDnsListener::UpdateType update_type,
                     const RecordParsed* record_parsed);

  // Alert the delegate of the existence of an Nsec record.
  void AlertNsecRecord();

 private:
  uint16 rrtype_;
  std::string name_;
  MDnsClientImpl* client_;
  MDnsListener::Delegate* delegate_;

  bool started_;
  DISALLOW_COPY_AND_ASSIGN(MDnsListenerImpl);
};

class MDnsTransactionImpl : public base::SupportsWeakPtr<MDnsTransactionImpl>,
                            public MDnsTransaction,
                            public MDnsListener::Delegate {
 public:
  MDnsTransactionImpl(uint16 rrtype,
                      const std::string& name,
                      int flags,
                      const MDnsTransaction::ResultCallback& callback,
                      MDnsClientImpl* client);
  virtual ~MDnsTransactionImpl();

  // MDnsTransaction implementation:
  virtual bool Start() OVERRIDE;

  virtual const std::string& GetName() const OVERRIDE;
  virtual uint16 GetType() const OVERRIDE;

  // MDnsListener::Delegate implementation:
  virtual void OnRecordUpdate(MDnsListener::UpdateType update,
                              const RecordParsed* record) OVERRIDE;
  virtual void OnNsecRecord(const std::string& name, unsigned type) OVERRIDE;

  virtual void OnCachePurged() OVERRIDE;

 private:
  bool is_active() { return !callback_.is_null(); }

  void Reset();

  // Trigger the callback and reset all related variables.
  void TriggerCallback(MDnsTransaction::Result result,
                       const RecordParsed* record);

  // Internal callback for when a cache record is found.
  void CacheRecordFound(const RecordParsed* record);

  // Signal the transactionis over and release all related resources.
  void SignalTransactionOver();

  // Reads records from the cache and calls the callback for every
  // record read.
  void ServeRecordsFromCache();

  // Send a query to the network and set up a timeout to time out the
  // transaction. Returns false if it fails to start listening on the network
  // or if it fails to send a query.
  bool QueryAndListen();

  uint16 rrtype_;
  std::string name_;
  MDnsTransaction::ResultCallback callback_;

  scoped_ptr<MDnsListener> listener_;
  base::CancelableCallback<void()> timeout_;

  MDnsClientImpl* client_;

  bool started_;
  int flags_;

  DISALLOW_COPY_AND_ASSIGN(MDnsTransactionImpl);
};

}  // namespace net
#endif  // NET_DNS_MDNS_CLIENT_IMPL_H_
