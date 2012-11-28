// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/test/ui_test_utils_aura.h"

#include "chrome/test/base/ui_test_utils.h"
#include "ui/aura/window.h"

namespace ui_test_utils {

#if !defined(OS_WIN)
void HideNativeWindow(gfx::NativeWindow window) {
  HideNativeWindowAura(window);
}

bool ShowAndFocusNativeWindow(gfx::NativeWindow window) {
  return ShowAndFocusNativeWindowAura(window);
}
#endif

void HideNativeWindowAura(gfx::NativeWindow window) {
  window->Hide();
}

bool ShowAndFocusNativeWindowAura(gfx::NativeWindow window) {
  window->Show();
  return true;
}

}  // namespace ui_test_utils
