// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_NETWORK_MONITOR_RESOURCE_H_
#define PPAPI_PROXY_NETWORK_MONITOR_RESOURCE_H_

#include <list>

#include "ppapi/proxy/network_list_resource.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/shared_impl/scoped_pp_resource.h"
#include "ppapi/thunk/ppb_network_monitor_api.h"

namespace base {
class MessageLoopProxy;
}  // namespace base

namespace ppapi {
namespace proxy {

class NetworkMonitorResource : public PluginResource,
                               public thunk::PPB_NetworkMonitor_API {
 public:
  explicit NetworkMonitorResource(Connection connection,
                                  PP_Instance instance);
  virtual ~NetworkMonitorResource();

  // PluginResource overrides.
  ppapi::thunk::PPB_NetworkMonitor_API* AsPPB_NetworkMonitor_API() OVERRIDE;
  virtual void OnReplyReceived(const ResourceMessageReplyParams& params,
                               const IPC::Message& msg) OVERRIDE;

  // thunk::PPB_NetworkMonitor_API interface
  virtual int32_t UpdateNetworkList(
      PP_Resource* network_list,
      scoped_refptr<TrackedCallback> callback) OVERRIDE;

 private:
  // IPC message handlers for the messages received from the browser.
  void OnPluginMsgNetworkList(const ResourceMessageReplyParams& params,
                              const SerializedNetworkList& list);
  void OnPluginMsgForbidden(const ResourceMessageReplyParams& params);

  ScopedPPResource current_list_;
  bool forbidden_;

  // Parameters passed to UpdateNetworkList().
  PP_Resource* network_list_;
  scoped_refptr<TrackedCallback> update_callback_;

  DISALLOW_COPY_AND_ASSIGN(NetworkMonitorResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_NETWORK_MONITOR_RESOURCE_H_
