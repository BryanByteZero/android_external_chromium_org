// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/monitored_video_stub.h"

#include "base/bind.h"
#include "base/logging.h"
#include "remoting/codec/video_decoder.h"
#include "remoting/codec/video_decoder_verbatim.h"
#include "remoting/codec/video_decoder_vpx.h"

namespace remoting {
namespace protocol {

MonitoredVideoStub::MonitoredVideoStub(VideoStub* video_stub,
                                       base::TimeDelta connectivity_check_delay,
                                       const ChannelStateCallback& callback)
    : video_stub_(video_stub),
      callback_(callback),
      is_connected_(false),
      connectivity_check_timer_(
          FROM_HERE,
          connectivity_check_delay,
          this,
          &MonitoredVideoStub::OnConnectivityCheckTimeout) {
}

MonitoredVideoStub::~MonitoredVideoStub() {
}

void MonitoredVideoStub::ProcessVideoPacket(scoped_ptr<VideoPacket> packet,
                                            const base::Closure& done) {
  DCHECK(thread_checker_.CalledOnValidThread());

  connectivity_check_timer_.Reset();

  NotifyChannelState(true);

  video_stub_->ProcessVideoPacket(packet.Pass(), done);
}

void MonitoredVideoStub::OnConnectivityCheckTimeout() {
  DCHECK(thread_checker_.CalledOnValidThread());
  NotifyChannelState(false);
}

void MonitoredVideoStub::NotifyChannelState(bool connected) {
  if (is_connected_ != connected) {
    is_connected_ = connected;
    callback_.Run(is_connected_);
  }
}

}  // namespace protocol
}  // namespace remoting
