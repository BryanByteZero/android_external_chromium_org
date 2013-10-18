// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/desktop_focus_rules.h"

#include "ui/aura/root_window.h"
#include "ui/aura/window.h"

namespace views {

DesktopFocusRules::DesktopFocusRules(aura::Window* content_window)
    : content_window_(content_window) {}

DesktopFocusRules::~DesktopFocusRules() {}

bool DesktopFocusRules::SupportsChildActivation(aura::Window* window) const {
  // In Desktop-Aura, only the content_window or children of the RootWindow are
  // activatable.
  return window == content_window_->parent() ||
         window->GetRootWindow() == window;
}

aura::Window* DesktopFocusRules::GetToplevelWindow(
    aura::Window* window) const {
  aura::Window* top_level_window =
      corewm::BaseFocusRules::GetToplevelWindow(window);
  // In Desktop-Aura, only the content_window or children of the RootWindow are
  // considered as top level windows.
  if (top_level_window == content_window_->parent())
    return content_window_;
  return top_level_window;
}

aura::Window* DesktopFocusRules::GetNextActivatableWindow(
    aura::Window* window) const {
  aura::Window* next_activatable_window =
      corewm::BaseFocusRules::GetNextActivatableWindow(window);
  // In Desktop-Aura the content_window_'s parent is a dummy window and thus
  // should never be activated. We should return the content_window_ if it
  // can be activated in this case.
  if (next_activatable_window == content_window_->parent() &&
      CanActivateWindow(content_window_))
    return content_window_;
  return next_activatable_window;
}

}  // namespace views
