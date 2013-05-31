// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_WINDOW_REORDERER_H_
#define UI_VIEWS_WIDGET_WINDOW_REORDERER_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "ui/aura/window_observer.h"

namespace aura {
class Window;
}

namespace views {
class View;

// Class which reorders the widget's child windows which have an associated view
// in the widget's view tree according the z-order of the views in the view
// tree. Windows not associated to a view are stacked above windows with an
// associated view. The child windows' layers are additionally reordered
// according to the z-order of the associated views relative to views with
// layers.
class WindowReorderer : public aura::WindowObserver {
 public:
  WindowReorderer(aura::Window* window, View* root_view);
  virtual ~WindowReorderer();

  // Explicitly reorder the children of |window_| (and their layers). This
  // method should be called when the position of a view with an associated
  // window changes in the view hierarchy. This method assumes that the
  // child layers of |window_| which are owned by views are already in the
  // correct z-order relative to each other and does no reordering if there
  // are no views with an associated window.
  void ReorderChildWindows();

 private:
  // aura::WindowObserver overrides:
  virtual void OnWindowAdded(aura::Window* new_window) OVERRIDE;
  virtual void OnWillRemoveWindow(aura::Window* window) OVERRIDE;
  virtual void OnWindowDestroying(aura::Window* window) OVERRIDE;

  // The window and the root view of the native widget which owns the
  // WindowReorderer.
  aura::Window* parent_window_;
  View* root_view_;

  // Reorders windows as a result of the kHostViewKey being set on a child of
  // |parent_window_|.
  class AssociationObserver;
  scoped_ptr<AssociationObserver> association_observer_;

  DISALLOW_COPY_AND_ASSIGN(WindowReorderer);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_WINDOW_REORDERER_H_
