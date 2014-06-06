// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/net/delay_network_call.h"

#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/chromeos/net/network_portal_detector.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

const unsigned chromeos::kDefaultNetworkRetryDelayMS = 3000;

void chromeos::DelayNetworkCall(const base::Closure& callback,
                                base::TimeDelta retry) {
  bool delay_network_call = false;
  const NetworkState* default_network =
      NetworkHandler::Get()->network_state_handler()->DefaultNetwork();
  if (!default_network) {
    delay_network_call = true;
    DVLOG(1) << "DelayNetworkCall: No default network.";
  } else {
    std:: string default_connection_state = default_network->connection_state();
    if (!NetworkState::StateIsConnected(default_connection_state)) {
      delay_network_call = true;
      DVLOG(1) << "DelayNetworkCall: "
               << "Default network: " << default_network->name()
               << " State: " << default_connection_state;
    }
  }
  if (!delay_network_call && NetworkPortalDetector::IsInitialized()) {
    NetworkPortalDetector* detector = NetworkPortalDetector::Get();
    NetworkPortalDetector::CaptivePortalStatus status =
        detector->GetCaptivePortalState(default_network->path()).status;
    if (status != NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE) {
      delay_network_call = true;
      DVLOG(1) << "DelayNetworkCall: Captive portal status for "
              << default_network->name() << ": "
              << NetworkPortalDetector::CaptivePortalStatusString(status);
    }
  }
  if (delay_network_call) {
    content::BrowserThread::PostDelayedTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&chromeos::DelayNetworkCall, callback, retry),
        retry);
  } else {
    callback.Run();
  }
}
