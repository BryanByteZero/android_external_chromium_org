// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/midi_dispatcher_host.h"

#include "base/bind.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/common/media/midi_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace content {

MidiDispatcherHost::PendingPermission::PendingPermission(
    int render_process_id,
    int render_view_id,
    int bridge_id)
    : render_process_id(render_process_id),
      render_view_id(render_view_id),
      bridge_id(bridge_id) {
}

MidiDispatcherHost::PendingPermission::~PendingPermission() {
}

MidiDispatcherHost::MidiDispatcherHost(WebContents* web_contents)
    : WebContentsObserver(web_contents),
      weak_factory_(this) {
}

MidiDispatcherHost::~MidiDispatcherHost() {
}

bool MidiDispatcherHost::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MidiDispatcherHost, message)
    IPC_MESSAGE_HANDLER(MidiHostMsg_RequestSysExPermission,
                        OnRequestSysExPermission)
    IPC_MESSAGE_HANDLER(MidiHostMsg_CancelSysExPermissionRequest,
                        OnCancelSysExPermissionRequest)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void MidiDispatcherHost::OnRequestSysExPermission(int bridge_id,
                                                  const GURL& origin,
                                                  bool user_gesture) {
  int render_process_id = web_contents()->GetRenderProcessHost()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();

  PendingPermission pending_permission(
      render_process_id, render_view_id, bridge_id);
  pending_permissions_.push_back(pending_permission);

  GetContentClient()->browser()->RequestMidiSysExPermission(
      web_contents(),
      bridge_id,
      origin,
      user_gesture,
      base::Bind(&MidiDispatcherHost::WasSysExPermissionGranted,
                 weak_factory_.GetWeakPtr(),
                 render_process_id, render_view_id, bridge_id),
      &pending_permissions_.back().cancel);
}

void MidiDispatcherHost::OnCancelSysExPermissionRequest(
    int bridge_id,
    const GURL& requesting_frame) {
  int render_process_id = web_contents()->GetRenderProcessHost()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();

  for (size_t i = 0; i < pending_permissions_.size(); ++i) {
    if (pending_permissions_[i].render_process_id == render_process_id &&
        pending_permissions_[i].render_view_id == render_view_id &&
        pending_permissions_[i].bridge_id == bridge_id) {
      if (!pending_permissions_[i].cancel.is_null())
        pending_permissions_[i].cancel.Run();
      pending_permissions_.erase(pending_permissions_.begin() + i);
      return;
    }
  }
}

void MidiDispatcherHost::WasSysExPermissionGranted(int render_process_id,
                                                   int render_view_id,
                                                   int bridge_id,
                                                   bool is_allowed) {
  for (size_t i = 0; i < pending_permissions_.size(); ++i) {
    if (pending_permissions_[i].render_process_id == render_process_id &&
        pending_permissions_[i].render_view_id == render_view_id &&
        pending_permissions_[i].bridge_id == bridge_id) {
      RenderViewHost* render_view_host =
          RenderViewHost::FromID(render_process_id, render_view_id);
      if (render_view_host) {
        render_view_host->Send(new MidiMsg_SysExPermissionApproved(
            render_view_id, bridge_id, is_allowed));
      }

      if (is_allowed) {
        ChildProcessSecurityPolicyImpl::GetInstance()->
            GrantSendMidiSysExMessage(render_process_id);
      }

      pending_permissions_.erase(pending_permissions_.begin() + i);
      return;
    }
  }
}

}  // namespace content
