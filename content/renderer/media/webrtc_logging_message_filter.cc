// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc_logging_message_filter.h"

#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "content/common/media/webrtc_logging_messages.h"
#include "content/renderer/media/webrtc_logging_handler_impl.h"
#include "ipc/ipc_logging.h"
#include "third_party/libjingle/overrides/talk/base/logging.h"

namespace content {

WebRtcLoggingMessageFilter::WebRtcLoggingMessageFilter(
    const scoped_refptr<base::MessageLoopProxy>& io_message_loop)
    : logging_handler_(NULL),
      io_message_loop_(io_message_loop),
      channel_(NULL) {
}

WebRtcLoggingMessageFilter::~WebRtcLoggingMessageFilter() {
}

bool WebRtcLoggingMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebRtcLoggingMessageFilter, message)
    IPC_MESSAGE_HANDLER(WebRtcLoggingMsg_LogOpened, OnLogOpened)
    IPC_MESSAGE_HANDLER(WebRtcLoggingMsg_OpenLogFailed, OnOpenLogFailed)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void WebRtcLoggingMessageFilter::OnFilterAdded(IPC::Channel* channel) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  channel_ = channel;
}

void WebRtcLoggingMessageFilter::OnFilterRemoved() {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  channel_ = NULL;
  logging_handler_->OnFilterRemoved();
}

void WebRtcLoggingMessageFilter::OnChannelClosing() {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  channel_ = NULL;
  logging_handler_->OnFilterRemoved();
}

void WebRtcLoggingMessageFilter::InitLogging(
    WebRtcLoggingHandlerImpl* logging_handler,
    const std::string& app_session_id) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  DCHECK(!logging_handler_);
  logging_handler_ = logging_handler;
  Send(new WebRtcLoggingMsg_OpenLog(app_session_id));
}

void WebRtcLoggingMessageFilter::OnLogOpened(
    base::SharedMemoryHandle handle,
    uint32 length) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  if (logging_handler_)
    logging_handler_->OnLogOpened(handle, length);
}

void WebRtcLoggingMessageFilter::OnOpenLogFailed() {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  if (logging_handler_)
    logging_handler_->OnOpenLogFailed();
}

void WebRtcLoggingMessageFilter::Send(IPC::Message* message) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  if (!channel_) {
    DLOG(ERROR) << "IPC channel not available.";
    delete message;
  } else {
    channel_->Send(message);
  }
}

}  // namespace content
