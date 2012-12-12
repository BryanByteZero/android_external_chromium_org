// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/corewm/focus_controller.h"

#include "base/auto_reset.h"
#include "ui/aura/client/activation_change_observer.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/env.h"
#include "ui/base/events/event.h"
#include "ui/views/corewm/focus_rules.h"

namespace views {
namespace corewm {
namespace {

// When a modal window is activated, we bring its entire transient parent chain
// to the front. This function must be called before the modal transient is
// stacked at the top to ensure correct stacking order.
void StackTransientParentsBelowModalWindow(aura::Window* window) {
  if (window->GetProperty(aura::client::kModalKey) != ui::MODAL_TYPE_WINDOW)
    return;

  aura::Window* transient_parent = window->transient_parent();
  while (transient_parent) {
    transient_parent->parent()->StackChildAtTop(transient_parent);
    transient_parent = transient_parent->transient_parent();
  }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// FocusController, public:

FocusController::FocusController(FocusRules* rules)
    : active_window_(NULL),
      focused_window_(NULL),
      rules_(rules),
      ALLOW_THIS_IN_INITIALIZER_LIST(observer_manager_(this)) {
  DCHECK(rules);
  aura::Env::GetInstance()->AddObserver(this);
}

FocusController::~FocusController() {
  aura::Env::GetInstance()->RemoveObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// FocusController, aura::client::ActivationClient implementation:

void FocusController::AddObserver(
    aura::client::ActivationChangeObserver* observer) {
  activation_observers_.AddObserver(observer);
}

void FocusController::RemoveObserver(
    aura::client::ActivationChangeObserver* observer) {
  activation_observers_.RemoveObserver(observer);
}

void FocusController::ActivateWindow(aura::Window* window) {
  FocusWindow(window, NULL);
}

void FocusController::DeactivateWindow(aura::Window* window) {
  if (window)
    FocusWindow(rules_->GetNextActivatableWindow(window), NULL);
}

aura::Window* FocusController::GetActiveWindow() {
  return active_window_;
}

aura::Window* FocusController::GetActivatableWindow(aura::Window* window) {
  return rules_->GetActivatableWindow(window);
}

bool FocusController::OnWillFocusWindow(aura::Window* window,
                                        const ui::Event* event) {
  NOTREACHED();
  return false;
}

bool FocusController::CanActivateWindow(aura::Window* window) const {
  return rules_->CanActivateWindow(window);
}

////////////////////////////////////////////////////////////////////////////////
// FocusController, aura::client::FocusClient implementation:

void FocusController::AddObserver(
    aura::client::FocusChangeObserver* observer) {
  focus_observers_.AddObserver(observer);
}

void FocusController::RemoveObserver(
    aura::client::FocusChangeObserver* observer) {
  focus_observers_.RemoveObserver(observer);
}

void FocusController::FocusWindow(aura::Window* window,
                                  const ui::Event* event) {
  // Focusing a window also activates its containing activatable window. Note
  // that the rules could redirect activation activation and/or focus.
  aura::Window* focusable = rules_->GetFocusableWindow(window);
  aura::Window* activatable =
      focusable ? rules_->GetActivatableWindow(focusable) : NULL;
  SetActiveWindow(activatable);
  if (focusable && activatable)
    DCHECK(GetActiveWindow()->Contains(focusable));
  SetFocusedWindow(focusable);
}

aura::Window* FocusController::GetFocusedWindow() {
  return focused_window_;
}

void FocusController::OnWindowHiddenInRootWindow(
    aura::Window* window,
    aura::RootWindow* root_window,
    bool destroyed) {
  //NOTREACHED();
  // This method is only for compat with aura::FocusManager. It should not be
  // needed in the new FocusController.
}

////////////////////////////////////////////////////////////////////////////////
// FocusController, ui::EventHandler implementation:
ui::EventResult FocusController::OnKeyEvent(ui::KeyEvent* event) {
  return ui::ER_UNHANDLED;
}

ui::EventResult FocusController::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_PRESSED)
    WindowFocusedFromInputEvent(static_cast<aura::Window*>(event->target()));
  return ui::ER_UNHANDLED;
}

ui::EventResult FocusController::OnScrollEvent(ui::ScrollEvent* event) {
  return ui::ER_UNHANDLED;
}

ui::EventResult FocusController::OnTouchEvent(ui::TouchEvent* event) {
  return ui::ER_UNHANDLED;
}

void FocusController::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_BEGIN &&
      event->details().touch_points() == 1) {
    WindowFocusedFromInputEvent(static_cast<aura::Window*>(event->target()));
  }
}

////////////////////////////////////////////////////////////////////////////////
// FocusController, aura::WindowObserver implementation:

void FocusController::OnWindowVisibilityChanged(aura::Window* window,
                                                bool visible) {
  if (!visible) {
    WindowLostFocusFromDispositionChange(window);
    // Despite the focus change, we need to keep the window being hidden
    // stacked above the new window so it stays open on top as it animates away.
    aura::Window* next_window = GetActiveWindow();
    if (next_window && next_window->parent() == window->parent()) {
      window->layer()->parent()->StackAbove(window->layer(),
                                            next_window->layer());
    }
  }
}

void FocusController::OnWindowDestroying(aura::Window* window) {
  WindowLostFocusFromDispositionChange(window);
}

void FocusController::OnWindowDestroyed(aura::Window* window) {
  observer_manager_.Remove(window);
}

void FocusController::OnWindowRemovingFromRootWindow(aura::Window* window) {
  WindowLostFocusFromDispositionChange(window);
}

void FocusController::OnWindowInitialized(aura::Window* window) {
  observer_manager_.Add(window);
}

////////////////////////////////////////////////////////////////////////////////
// FocusController, private:

void FocusController::SetFocusedWindow(aura::Window* window) {
  if (window == focused_window_)
    return;
  DCHECK(rules_->CanFocusWindow(window));
  if (window)
    DCHECK_EQ(window, rules_->GetFocusableWindow(window));

  aura::Window* lost_focus = focused_window_;
  focused_window_ = window;

  FOR_EACH_OBSERVER(aura::client::FocusChangeObserver,
                    focus_observers_,
                    OnWindowFocused(focused_window_, lost_focus));
  aura::client::FocusChangeObserver* observer =
      aura::client::GetFocusChangeObserver(lost_focus);
  if (observer)
    observer->OnWindowFocused(focused_window_, lost_focus);
  observer = aura::client::GetFocusChangeObserver(focused_window_);
  if (observer)
    observer->OnWindowFocused(focused_window_, lost_focus);
}

void FocusController::SetActiveWindow(aura::Window* window) {
  if (window == active_window_)
    return;
  DCHECK(rules_->CanActivateWindow(window));
  if (window)
    DCHECK_EQ(window, rules_->GetActivatableWindow(window));

  aura::Window* lost_activation = active_window_;
  active_window_ = window;
  if (active_window_) {
    StackTransientParentsBelowModalWindow(active_window_);
    active_window_->parent()->StackChildAtTop(active_window_);
  }

  FOR_EACH_OBSERVER(aura::client::ActivationChangeObserver,
                    activation_observers_,
                    OnWindowActivated(active_window_, lost_activation));
  aura::client::ActivationChangeObserver* observer =
      aura::client::GetActivationChangeObserver(lost_activation);
  if (observer)
    observer->OnWindowActivated(active_window_, lost_activation);
  observer = aura::client::GetActivationChangeObserver(active_window_);
  if (observer)
    observer->OnWindowActivated(active_window_, lost_activation);
}

void FocusController::WindowLostFocusFromDispositionChange(
    aura::Window* window) {
  // TODO(beng): See if this function can be replaced by a call to
  //             FocusWindow().
  // Activation adjustments are handled first in the event of a disposition
  // changed. If an activation change is necessary, focus is reset as part of
  // that process so there's no point in updating focus independently.
  if (window->Contains(active_window_)) {
    aura::Window* next_activatable = rules_->GetNextActivatableWindow(window);
    SetActiveWindow(next_activatable);
    SetFocusedWindow(next_activatable);
  } else if (window->Contains(focused_window_)) {
    // Active window isn't changing, but focused window might be.
    SetFocusedWindow(rules_->GetNextFocusableWindow(window));
  }
}

void FocusController::WindowFocusedFromInputEvent(aura::Window* window) {
  FocusWindow(window, NULL);
}

}  // namespace corewm
}  // namespace views
