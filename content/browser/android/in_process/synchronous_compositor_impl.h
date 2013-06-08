// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_IN_PROCESS_SYNCHRONOUS_COMPOSITOR_IMPL_H_
#define CONTENT_BROWSER_ANDROID_IN_PROCESS_SYNCHRONOUS_COMPOSITOR_IMPL_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/browser/android/in_process/synchronous_compositor_output_surface.h"
#include "content/port/common/input_event_ack_state.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "content/public/browser/web_contents_user_data.h"

namespace WebKit {
class WebInputEvent;
}

namespace content {

// The purpose of this class is to act as the intermediary between the various
// components that make up the 'synchronous compositor mode' implementation and
// expose their functionality via the SynchronousCompositor interface.
// This class is created on the main thread but most of the APIs are called
// from the Compositor thread.
class SynchronousCompositorImpl
    : public SynchronousCompositor,
      public SynchronousCompositorOutputSurfaceDelegate,
      public WebContentsUserData<SynchronousCompositorImpl> {
 public:
  // When used from browser code, use both |process_id| and |routing_id|.
  static SynchronousCompositorImpl* FromID(int process_id, int routing_id);
  // When handling upcalls from renderer code, use this version; the process id
  // is implicitly that of the in-process renderer.
  static SynchronousCompositorImpl* FromRoutingID(int routing_id);

  InputEventAckState HandleInputEvent(const WebKit::WebInputEvent& input_event);

  // SynchronousCompositor
  virtual bool IsHwReady() OVERRIDE;
  virtual void SetClient(SynchronousCompositorClient* compositor_client)
      OVERRIDE;
  virtual bool DemandDrawSw(SkCanvas* canvas) OVERRIDE;
  virtual bool DemandDrawHw(
      gfx::Size view_size,
      const gfx::Transform& transform,
      gfx::Rect clip) OVERRIDE;

  // SynchronousCompositorOutputSurfaceDelegate
  virtual void DidBindOutputSurface(
      SynchronousCompositorOutputSurface* output_surface) OVERRIDE;
  virtual void DidDestroySynchronousOutputSurface(
      SynchronousCompositorOutputSurface* output_surface) OVERRIDE;
  virtual void SetContinuousInvalidate(bool enable) OVERRIDE;

 private:
  explicit SynchronousCompositorImpl(WebContents* contents);
  virtual ~SynchronousCompositorImpl();
  friend class WebContentsUserData<SynchronousCompositorImpl>;

  void DidCreateSynchronousOutputSurface(
      SynchronousCompositorOutputSurface* output_surface);
  bool CalledOnValidThread() const;

  SynchronousCompositorClient* compositor_client_;
  SynchronousCompositorOutputSurface* output_surface_;
  WebContents* contents_;

  DISALLOW_COPY_AND_ASSIGN(SynchronousCompositorImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_IN_PROCESS_SYNCHRONOUS_COMPOSITOR_IMPL_H_
