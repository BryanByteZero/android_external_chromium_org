// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_PREWARMER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_PREWARMER_H_

#include "base/basictypes.h"
#include "base/callback.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace chromeos {

class NetworkState;

// Class for prewarming authentication network connection.
class AuthPrewarmer : public NetworkStateHandlerObserver,
                      public content::NotificationObserver {
 public:
  AuthPrewarmer();
  virtual ~AuthPrewarmer();

  void PrewarmAuthentication(const base::Closure& completion_callback);

 private:
  // chromeos::NetworkStateHandlerObserver overrides.
  virtual void DefaultNetworkChanged(const NetworkState* network) OVERRIDE;

  // content::NotificationObserver overrides.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  bool IsNetworkConnected() const;
  net::URLRequestContextGetter* GetRequestContext() const;
  void DoPrewarm();

  content::NotificationRegistrar registrar_;
  base::Closure completion_callback_;
  bool doing_prewarm_;

  DISALLOW_COPY_AND_ASSIGN(AuthPrewarmer);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_PREWARMER_H_
