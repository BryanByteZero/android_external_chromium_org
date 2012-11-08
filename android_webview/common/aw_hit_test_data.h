// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_COMMON_AW_HIT_TEST_DATA_H_
#define ANDROID_WEBVIEW_COMMON_AW_HIT_TEST_DATA_H_

#include "base/string16.h"
#include "googleurl/src/gurl.h"

namespace android_webview {

// Holdes all hit test data needed by public WebView APIs.
// The Java counter part to this is AwContents.HitTestData.
struct AwHitTestData {

  // Matches exactly with constants in WebView.HitTestResult, with deprecated
  // values removed.
  enum Type {
    // Default type where nothing we are interested in is hit.
    // |extra_data_for_type| will be empty. All other values should be emtpy
    // except the special case described below.
    // For special case of invalid or javascript scheme url that would
    // otherwise be type an LINK type, |href|, |anchor_text|, |img_src| contain
    // their normal values for the respective type.
    UNKNOWN_TYPE = 0,

    // Content detection types. Not used yet.
    // TODO(boliu): Hook up content detection.
    PHONE_TYPE = 2,
    GEO_TYPE = 3,
    EMAIL_TYPE = 4,

    // Hit on a pure image (without links). |extra_data_for_type|, |href|,
    // and |anchor_text| will be empty. |img_src| will contain the absolute
    // source url of the image.
    IMAGE_TYPE = 5,

    // Hit on a link with valid and non-javascript url and without embedded
    // image. |extra_data_for_type| is the valid absolute url of the link.
    // |href| will contain the exact href attribute string. |anchor_text| will
    // contain the anchor text if the link is an anchor tag. |img_src| will be
    // empty.
    // Note 1: If the link url is invalid or javascript scheme, then the type
    // will be UNKNOWN_TYPE.
    // Note 2: Note that this matches SRC_ANCHOR_TYPE in the public WebView
    // Java API, but the actual tag can be something other than <a>, such as
    // <link> or <area>.
    SRC_LINK_TYPE = 7,

    // Same as SRC_LINK_TYPE except the link contains an image. |img_src| and
    // |extra_data_for_type| will contain the absolute valid url of the image
    // source. |href| will contain the (possibly invalid or javascript-scheme)
    // link href attribute. |anchor_text| will be empty.
    // Both notes from SRC_LINK_TYPE apply.
    SRC_IMAGE_LINK_TYPE = 8,

    // Hit on an editable text input element. All other values will be empty.
    EDIT_TEXT_TYPE = 9,
  };

  // For all strings/GURLs, empty/invalid will become null upon conversion to
  // Java.
  int type;  // Only values from enum Type above.
  std::string extra_data_for_type;
  string16 href;
  string16 anchor_text;
  GURL img_src;

  AwHitTestData();
  ~AwHitTestData();
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_COMMON_AW_HIT_TEST_DATA_H_
