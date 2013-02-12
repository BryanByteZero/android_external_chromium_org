// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_HOST_RESOLVER_PRIVATE_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_HOST_RESOLVER_PRIVATE_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "content/common/content_export.h"
#include "content/public/common/process_type.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/host/resource_message_filter.h"

struct PP_HostResolver_Private_Hint;
struct PP_NetAddress_Private;

namespace net {
class AddressList;
}

namespace ppapi {
struct HostPortPair;

namespace host {
struct HostMessageContext;
}
}

namespace content {

class BrowserPpapiHostImpl;
class ResourceContext;

class CONTENT_EXPORT PepperHostResolverPrivateMessageFilter
    : public ppapi::host::ResourceMessageFilter {
 public:
  PepperHostResolverPrivateMessageFilter(BrowserPpapiHostImpl* host,
                                         PP_Instance instance);

 protected:
  virtual ~PepperHostResolverPrivateMessageFilter();

 private:
  typedef std::vector<PP_NetAddress_Private> NetAddressList;

  // ppapi::host::ResourceMessageFilter overrides.
  virtual scoped_refptr<base::TaskRunner> OverrideTaskRunnerForMessage(
      const IPC::Message& message) OVERRIDE;
  virtual int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) OVERRIDE;

  int32_t OnMsgResolve(const ppapi::host::HostMessageContext* context,
                       const ppapi::HostPortPair& host_port,
                       const PP_HostResolver_Private_Hint& hint);

  // Backend for OnMsgResolve(). Delegates host resolution to the
  // Browser's HostResolver. Must be called on the IO thread.
  void DoResolve(const ppapi::host::ReplyMessageContext& context,
                 const ppapi::HostPortPair& host_port,
                 const PP_HostResolver_Private_Hint& hint,
                 ResourceContext* resource_context);

  void OnLookupFinished(int result,
                        const net::AddressList& addresses,
                        const ppapi::host::ReplyMessageContext& bound_info);
  void SendResolveReply(int result,
                        const std::string& canonical_name,
                        const NetAddressList& net_address_list,
                        const ppapi::host::ReplyMessageContext& context);
  void SendResolveError(const ppapi::host::ReplyMessageContext& context);

  ProcessType plugin_process_type_;
  int render_process_id_;
  int render_view_id_;

  DISALLOW_COPY_AND_ASSIGN(PepperHostResolverPrivateMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_HOST_RESOLVER_PRIVATE_MESSAGE_FILTER_H_
