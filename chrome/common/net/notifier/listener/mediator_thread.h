// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// These methods should post messages to a queue which a different thread will
// later come back and read from.

#ifndef CHROME_COMMON_NET_NOTIFIER_LISTENER_MEDIATOR_THREAD_H_
#define CHROME_COMMON_NET_NOTIFIER_LISTENER_MEDIATOR_THREAD_H_

#include <string>
#include <vector>

#include "chrome/common/net/notifier/listener/notification_defines.h"

namespace buzz {
class XmppClientSettings;
}  // namespace buzz

namespace notifier {

class MediatorThread {
 public:
  virtual ~MediatorThread() {}

  class Delegate {
   public:
    virtual ~Delegate() {}

    virtual void OnConnectionStateChange(bool logged_in) = 0;

    virtual void OnSubscriptionStateChange(bool subscribed) = 0;

    virtual void OnIncomingNotification(
        const IncomingNotificationData& notification_data) = 0;

    virtual void OnOutgoingNotification() = 0;
  };

  // |delegate| can be NULL if we're shutting down.
  // TODO(akalin): Handle messages during shutdown gracefully so that
  // we don't have to deal with NULL delegates.
  virtual void SetDelegate(Delegate* delegate) = 0;
  virtual void Login(const buzz::XmppClientSettings& settings) = 0;
  virtual void Logout() = 0;
  virtual void Start() = 0;
  virtual void SubscribeForUpdates(
      const std::vector<std::string>& subscribed_services_list) = 0;
  virtual void ListenForUpdates() = 0;
  virtual void SendNotification(const OutgoingNotificationData& data) = 0;
};

}  // namespace notifier

#endif  // CHROME_COMMON_NET_NOTIFIER_LISTENER_MEDIATOR_THREAD_H_
