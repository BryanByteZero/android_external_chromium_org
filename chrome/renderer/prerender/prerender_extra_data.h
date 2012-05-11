// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_PRERENDER_PRERENDER_EXTRA_DATA_H_
#define CHROME_RENDERER_PRERENDER_PRERENDER_EXTRA_DATA_H_
#pragma once

#include "base/compiler_specific.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebPrerender.h"
#include "ui/gfx/size.h"

namespace prerender {

class PrerenderExtraData : public WebKit::WebPrerender::ExtraData {
 public:
  PrerenderExtraData(int prerender_id,
                     int render_view_route_id,
                     const gfx::Size& size);
  virtual ~PrerenderExtraData();

  int prerender_id() const { return prerender_id_; }
  int render_view_route_id() const { return render_view_route_id_; }
  const gfx::Size& size() const { return size_; }

  static const PrerenderExtraData& FromPrerender(
      const WebKit::WebPrerender& prerender);

 private:
  const int prerender_id_;
  const int render_view_route_id_;
  const gfx::Size size_;

  DISALLOW_COPY_AND_ASSIGN(PrerenderExtraData);
};

}  // namespace prerender

#endif  // CHROME_RENDERER_PRERENDER_PRERENDER_EXTRA_DATA_H_

