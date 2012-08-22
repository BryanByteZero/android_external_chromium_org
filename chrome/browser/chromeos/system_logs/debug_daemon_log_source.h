// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SYSTEM_LOGS_DEBUG_DAEMON_LOG_SOURCE_H_
#define CHROME_BROWSER_CHROMEOS_SYSTEM_LOGS_DEBUG_DAEMON_LOG_SOURCE_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/system_logs/system_logs_fetcher.h"

namespace chromeos {

// Gathers log data from Debug Daemon.
class DebugDaemonLogSource : public SystemLogsSource {
 public:
  DebugDaemonLogSource();
  virtual ~DebugDaemonLogSource();

  // Fetches logs from the daemon over dbus. After the fetch is complete, the
  // results will be forwarded to the request supplied to the constructor and
  // this instance will free itself.
  // SystemLogsSource override.
  virtual void Fetch(const SysLogsSourceCallback& callback) OVERRIDE;

 private:
  // Callbacks for the 4 different dbus calls
  void OnGetRoutes(bool succeeded, const std::vector<std::string>& routes);
  void OnGetNetworkStatus(bool succeeded, const std::string& status);
  void OnGetModemStatus(bool succeeded, const std::string& status);
  void OnGetLogs(bool succeeded,
                 const std::map<std::string, std::string>& logs);
  // Sends the data to the callback_ when all the requests are completed
  void RequestCompleted();

  scoped_ptr<SystemLogsResponse> response_;
  SysLogsSourceCallback callback_;
  int num_pending_requests_;
  base::WeakPtrFactory<DebugDaemonLogSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DebugDaemonLogSource);
};


}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SYSTEM_LOGS_DEBUG_DAEMON_LOG_SOURCE_H_
