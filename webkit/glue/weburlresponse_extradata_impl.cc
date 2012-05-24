// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/weburlresponse_extradata_impl.h"

namespace webkit_glue {

WebURLResponseExtraDataImpl::WebURLResponseExtraDataImpl(
    const std::string& npn_negotiated_protocol)
    : npn_negotiated_protocol_(npn_negotiated_protocol),
      is_ftp_directory_listing_(false) {
}

WebURLResponseExtraDataImpl::~WebURLResponseExtraDataImpl() {
}

}  // namespace webkit_glue
