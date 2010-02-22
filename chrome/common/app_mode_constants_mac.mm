// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/app_mode_constants_mac.h"

namespace app_mode {

#if defined(GOOGLE_CHROME_BUILD)
extern const CFStringRef kAppPrefsID = CFSTR("com.google.Chrome");
#else
extern const CFStringRef kAppPrefsID = CFSTR("org.chromium.Chromium");
#endif

const CFStringRef kLastRunAppBundlePathPrefsKey = CFSTR("LastRunAppBundlePath");

}  // namespace app_mode
