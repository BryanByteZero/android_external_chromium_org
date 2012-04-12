// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/panels/panel_resize_controller.h"

#include "base/logging.h"
#include "chrome/browser/ui/panels/detached_panel_strip.h"
#include "chrome/browser/ui/panels/docked_panel_strip.h"
#include "chrome/browser/ui/panels/panel.h"
#include "chrome/browser/ui/panels/panel_manager.h"
#include "chrome/browser/ui/panels/panel_strip.h"

namespace {
  static bool ResizingLeft(panel::ResizingSides sides) {
    return sides == panel::RESIZE_TOP_LEFT ||
           sides == panel::RESIZE_LEFT ||
           sides == panel::RESIZE_BOTTOM_LEFT;
  }

  static bool ResizingRight(panel::ResizingSides sides) {
    return sides == panel::RESIZE_TOP_RIGHT ||
           sides == panel::RESIZE_RIGHT ||
           sides == panel::RESIZE_BOTTOM_RIGHT;
  }

  static bool ResizingTop(panel::ResizingSides sides) {
    return sides == panel::RESIZE_TOP_LEFT ||
           sides == panel::RESIZE_TOP ||
           sides == panel::RESIZE_TOP_RIGHT;
  }

  static bool ResizingBottom(panel::ResizingSides sides) {
    return sides == panel::RESIZE_BOTTOM_RIGHT ||
           sides == panel::RESIZE_BOTTOM ||
           sides == panel::RESIZE_BOTTOM_LEFT;
  }
}

PanelResizeController::PanelResizeController(PanelManager* panel_manager)
  : panel_manager_(panel_manager),
    resizing_panel_(NULL),
    sides_resized_(panel::RESIZE_NONE) {
}

void PanelResizeController::StartResizing(Panel* panel,
                                          const gfx::Point& mouse_location,
                                          panel::ResizingSides sides) {
  DCHECK(!IsResizing());
  DCHECK(panel->panel_strip() &&
         panel->panel_strip()->CanResizePanel(panel));

  DCHECK_NE(panel::RESIZE_NONE, sides);

  mouse_location_at_start_ = mouse_location;
  bounds_at_start_ = panel->GetBounds();
  sides_resized_ = sides;
  resizing_panel_ = panel;
}

void PanelResizeController::Resize(const gfx::Point& mouse_location) {
  DCHECK(IsResizing());
  if (resizing_panel_->panel_strip() == NULL ||
      !resizing_panel_->panel_strip()->CanResizePanel(resizing_panel_)) {
    EndResizing(false);
    return;
  }
  gfx::Rect bounds = resizing_panel_->GetBounds();

  if (ResizingRight(sides_resized_)) {
    bounds.set_width(std::max(bounds_at_start_.width() +
                     mouse_location.x() - mouse_location_at_start_.x(), 0));
  }
  if (ResizingBottom(sides_resized_)) {
    bounds.set_height(std::max(bounds_at_start_.height() +
                      mouse_location.y() - mouse_location_at_start_.y(), 0));
  }
  if (ResizingLeft(sides_resized_)) {
    bounds.set_width(std::max(bounds_at_start_.width() +
                     mouse_location_at_start_.x() - mouse_location.x(), 0));
  }
  if (ResizingTop(sides_resized_)) {
    bounds.set_height(std::max(bounds_at_start_.height() +
                      mouse_location_at_start_.y() - mouse_location.y(), 0));
  }

  // Give the panel a chance to adjust the bounds before setting them.
  gfx::Size size = bounds.size();
  resizing_panel_->ClampSize(&size);
  bounds.set_size(size);

  if (ResizingLeft(sides_resized_)) {
    bounds.set_x(bounds_at_start_.x() -
                 (bounds.width() - bounds_at_start_.width()));
  }

  if (ResizingTop(sides_resized_)) {
    bounds.set_y(bounds_at_start_.y() -
                 (bounds.height() - bounds_at_start_.height()));
  }

  if (bounds != resizing_panel_->GetBounds())
    panel_manager_->OnPanelResizedByMouse(resizing_panel_, bounds);
}

void PanelResizeController::EndResizing(bool cancelled) {
  DCHECK(IsResizing());

  if (cancelled) {
    panel_manager_->OnPanelResizedByMouse(resizing_panel_,
                                          bounds_at_start_);
  }

  // Do a thorough cleanup.
  resizing_panel_ = NULL;
  sides_resized_ = panel::RESIZE_NONE;
  bounds_at_start_ = gfx::Rect();
  mouse_location_at_start_ = gfx::Point();
}

void PanelResizeController::OnPanelClosed(Panel* panel) {
  if (!resizing_panel_)
    return;

  // If the resizing panel is closed, abort the resize operation.
  if (resizing_panel_ == panel)
    EndResizing(false);
}
