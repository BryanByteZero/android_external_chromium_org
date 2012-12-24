// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_MANAGER_IMPL_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_MANAGER_IMPL_H_

#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"
#include "content/browser/devtools/devtools_agent_host.h"
#include "content/common/content_export.h"
#include "content/public/browser/devtools_client_host.h"
#include "content/public/browser/devtools_manager.h"

class GURL;

namespace IPC {
class Message;
}

namespace content {

class DevToolsAgentHost;
class RenderViewHost;

// This class is a singleton that manages DevToolsClientHost instances and
// routes messages between developer tools clients and agents.
//
// Methods below that accept inspected RenderViewHost as a parameter are
// just convenience methods that call corresponding methods accepting
// DevToolAgentHost.
class CONTENT_EXPORT DevToolsManagerImpl
    : public DevToolsAgentHost::CloseListener,
      public DevToolsManager {
 public:
  // Returns single instance of this class. The instance is destroyed on the
  // browser main loop exit so this method MUST NOT be called after that point.
  static DevToolsManagerImpl* GetInstance();

  DevToolsManagerImpl();
  virtual ~DevToolsManagerImpl();

  void DispatchOnInspectorFrontend(DevToolsAgentHost* agent_host,
                                   const std::string& message);

  // DevToolsManager implementation
  virtual bool DispatchOnInspectorBackend(DevToolsClientHost* from,
                                          const std::string& message) OVERRIDE;
  virtual void CloseAllClientHosts() OVERRIDE;
  virtual DevToolsClientHost* GetDevToolsClientHostFor(
      DevToolsAgentHost* agent_host) OVERRIDE;
  virtual DevToolsAgentHost* GetDevToolsAgentHostFor(
      DevToolsClientHost* client_host) OVERRIDE;
  virtual void RegisterDevToolsClientHostFor(
      DevToolsAgentHost* agent_host,
      DevToolsClientHost* client_host) OVERRIDE;
  virtual void UnregisterDevToolsClientHostFor(
      DevToolsAgentHost* agent_host) OVERRIDE;
  virtual void ClientHostClosing(DevToolsClientHost* host) OVERRIDE;
  virtual void InspectElement(DevToolsAgentHost* agent_host,
                              int x, int y) OVERRIDE;
  virtual void AddMessageToConsole(DevToolsAgentHost* agent_host,
                                   ConsoleMessageLevel level,
                                   const std::string& message) OVERRIDE;

 private:
  friend struct DefaultSingletonTraits<DevToolsManagerImpl>;

  // DevToolsAgentHost::CloseListener implementation.
  virtual void AgentHostClosing(DevToolsAgentHost* host) OVERRIDE;

  void BindClientHost(DevToolsAgentHost* agent_host,
                      DevToolsClientHost* client_host);
  void UnbindClientHost(DevToolsAgentHost* agent_host,
                        DevToolsClientHost* client_host);

  // These two maps are for tracking dependencies between inspected contents and
  // their DevToolsClientHosts. They are useful for routing devtools messages
  // and allow us to have at most one devtools client host per contents.
  //
  // DevToolsManagerImpl starts listening to DevToolsClientHosts when they are
  // put into these maps and removes them when they are closing.
  typedef std::map<DevToolsAgentHost*, DevToolsClientHost*>
      AgentToClientHostMap;
  AgentToClientHostMap agent_to_client_host_;

  typedef std::map<DevToolsClientHost*, DevToolsAgentHost*>
      ClientToAgentHostMap;
  ClientToAgentHostMap client_to_agent_host_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsManagerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_MANAGER_IMPL_H_
