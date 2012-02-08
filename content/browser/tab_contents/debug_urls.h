// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TAB_CONTENTS_DEBUG_URLS_H_
#define CONTENT_BROWSER_TAB_CONTENTS_DEBUG_URLS_H_
#pragma once

#include "content/public/common/page_transition_types.h"

class GURL;

namespace content {

// Checks if the given url is a url used for debugging purposes, and if so
// handles it and returns true.
bool HandleDebugURL(const GURL& url, content::PageTransition transition);

}  // namespace content

#endif  // CONTENT_BROWSER_TAB_CONTENTS_DEBUG_URLS_H_
