// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/custom_frame_view_ash.h"

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/maximize_bubble_controller.h"
#include "ash/wm/window_util.h"
#include "ash/wm/workspace/frame_maximize_button.h"
#include "ash/wm/workspace/snap_sizer.h"
#include "base/command_line.h"
#include "ui/aura/aura_switches.h"
#include "ui/aura/focus_manager.h"
#include "ui/aura/test/event_generator.h"
#include "ui/aura/window.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {
namespace internal {

namespace {

class ShellViewsDelegate : public views::TestViewsDelegate {
 public:
  ShellViewsDelegate() {}
  virtual ~ShellViewsDelegate() {}

  // Overridden from views::TestViewsDelegate:
  virtual views::NonClientFrameView* CreateDefaultNonClientFrameView(
      views::Widget* widget) OVERRIDE {
    return ash::Shell::GetInstance()->CreateDefaultNonClientFrameView(widget);
  }
  bool UseTransparentWindows() const OVERRIDE {
    // Ash uses transparent window frames.
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellViewsDelegate);
};

class TestWidgetDelegate : public views::WidgetDelegateView {
 public:
  TestWidgetDelegate() {}
  virtual ~TestWidgetDelegate() {}

  // Overridden from views::WidgetDelegate:
  virtual views::View* GetContentsView() OVERRIDE {
    return this;
  }
  virtual bool CanResize() const OVERRIDE {
    return true;
  }
  virtual bool CanMaximize() const OVERRIDE {
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestWidgetDelegate);
};

}  // namespace

class CustomFrameViewAshTest : public ash::test::AshTestBase {
 public:
  CustomFrameViewAshTest() {}
  virtual ~CustomFrameViewAshTest() {}

  views::Widget* CreateWidget() {
    views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
    views::Widget* widget = new views::Widget;
    params.delegate = new TestWidgetDelegate;
    widget->Init(params);
    widget->Show();
    return widget;
  }

  CustomFrameViewAsh* custom_frame_view_ash(views::Widget* widget) const {
    return static_cast<CustomFrameViewAsh*>(widget->non_client_view()->
        frame_view());
  }

  virtual void SetUp() OVERRIDE {
    AshTestBase::SetUp();
    if (!views::ViewsDelegate::views_delegate) {
      views_delegate_.reset(new ShellViewsDelegate);
      views::ViewsDelegate::views_delegate = views_delegate_.get();
    }
  }

  virtual void TearDown() OVERRIDE {
    if (views_delegate_.get()) {
      views::ViewsDelegate::views_delegate = NULL;
      views_delegate_.reset();
    }
    AshTestBase::TearDown();
  }

 private:
  scoped_ptr<views::ViewsDelegate> views_delegate_;

  DISALLOW_COPY_AND_ASSIGN(CustomFrameViewAshTest);
};

// Tests that clicking on the resize-button toggles between maximize and normal
// state.
TEST_F(CustomFrameViewAshTest, ResizeButtonToggleMaximize) {
  views::Widget* widget = CreateWidget();
  aura::Window* window = widget->GetNativeWindow();
  CustomFrameViewAsh* frame = custom_frame_view_ash(widget);
  CustomFrameViewAsh::TestApi test(frame);
  views::View* view = test.maximize_button();
  gfx::Point center = view->GetBoundsInScreen().CenterPoint();

  aura::test::EventGenerator generator(window->GetRootWindow(), center);

  EXPECT_FALSE(ash::wm::IsWindowMaximized(window));

  generator.ClickLeftButton();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(ash::wm::IsWindowMaximized(window));

  center = view->GetBoundsInScreen().CenterPoint();
  generator.MoveMouseTo(center);
  generator.ClickLeftButton();
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(ash::wm::IsWindowMaximized(window));

  generator.GestureTapAt(view->GetBoundsInScreen().CenterPoint());
  EXPECT_TRUE(ash::wm::IsWindowMaximized(window));

  generator.GestureTapAt(view->GetBoundsInScreen().CenterPoint());
  EXPECT_FALSE(ash::wm::IsWindowMaximized(window));

  generator.GestureTapDownAndUp(view->GetBoundsInScreen().CenterPoint());
  EXPECT_TRUE(ash::wm::IsWindowMaximized(window));

  generator.GestureTapDownAndUp(view->GetBoundsInScreen().CenterPoint());
  EXPECT_FALSE(ash::wm::IsWindowMaximized(window));

  widget->Close();
}

// Tests that click+dragging on the resize-button tiles or minimizes the window.
TEST_F(CustomFrameViewAshTest, ResizeButtonDrag) {
  views::Widget* widget = CreateWidget();
  aura::Window* window = widget->GetNativeWindow();
  CustomFrameViewAsh* frame = custom_frame_view_ash(widget);
  CustomFrameViewAsh::TestApi test(frame);
  views::View* view = test.maximize_button();
  gfx::Point center = view->GetBoundsInScreen().CenterPoint();
  const int kGridSize = ash::Shell::GetInstance()->GetGridSize();

  aura::test::EventGenerator generator(window->GetRootWindow(), center);

  EXPECT_TRUE(ash::wm::IsWindowNormal(window));

  // Snap right.
  {
    generator.PressLeftButton();
    generator.MoveMouseBy(10, 0);
    generator.ReleaseLeftButton();
    RunAllPendingInMessageLoop();

    EXPECT_FALSE(ash::wm::IsWindowMaximized(window));
    EXPECT_FALSE(ash::wm::IsWindowMinimized(window));
    internal::SnapSizer sizer(window, center,
        internal::SnapSizer::RIGHT_EDGE, kGridSize);
    EXPECT_EQ(sizer.target_bounds().ToString(), window->bounds().ToString());
  }

  // Snap left.
  {
    center = view->GetBoundsInScreen().CenterPoint();
    generator.MoveMouseTo(center);
    generator.PressLeftButton();
    generator.MoveMouseBy(-10, 0);
    generator.ReleaseLeftButton();
    RunAllPendingInMessageLoop();

    EXPECT_FALSE(ash::wm::IsWindowMaximized(window));
    EXPECT_FALSE(ash::wm::IsWindowMinimized(window));
    internal::SnapSizer sizer(window, center,
        internal::SnapSizer::LEFT_EDGE, kGridSize);
    EXPECT_EQ(sizer.target_bounds().ToString(), window->bounds().ToString());
  }

  // Minimize.
  {
    center = view->GetBoundsInScreen().CenterPoint();
    generator.MoveMouseTo(center);
    generator.PressLeftButton();
    generator.MoveMouseBy(0, 10);
    generator.ReleaseLeftButton();
    RunAllPendingInMessageLoop();

    EXPECT_TRUE(ash::wm::IsWindowMinimized(window));
  }

  ash::wm::RestoreWindow(window);

  // Now test the same behaviour for gesture events.

  // Snap right.
  {
    center = view->GetBoundsInScreen().CenterPoint();
    gfx::Point end = center;
    end.Offset(40, 0);
    generator.GestureScrollSequence(center, end,
        base::TimeDelta::FromMilliseconds(100),
        3);
    RunAllPendingInMessageLoop();

    EXPECT_FALSE(ash::wm::IsWindowMaximized(window));
    EXPECT_FALSE(ash::wm::IsWindowMinimized(window));
    internal::SnapSizer sizer(window, center,
        internal::SnapSizer::RIGHT_EDGE, kGridSize);
    EXPECT_EQ(sizer.target_bounds().ToString(), window->bounds().ToString());
  }

  // Snap left.
  {
    center = view->GetBoundsInScreen().CenterPoint();
    gfx::Point end = center;
    end.Offset(-40, 0);
    generator.GestureScrollSequence(center, end,
        base::TimeDelta::FromMilliseconds(100),
        3);
    RunAllPendingInMessageLoop();

    EXPECT_FALSE(ash::wm::IsWindowMaximized(window));
    EXPECT_FALSE(ash::wm::IsWindowMinimized(window));
    internal::SnapSizer sizer(window, center,
        internal::SnapSizer::LEFT_EDGE, kGridSize);
    EXPECT_EQ(sizer.target_bounds().ToString(), window->bounds().ToString());
  }

  // Minimize.
  {
    center = view->GetBoundsInScreen().CenterPoint();
    gfx::Point end = center;
    end.Offset(0, 40);
    generator.GestureScrollSequence(center, end,
        base::TimeDelta::FromMilliseconds(100),
        3);
    RunAllPendingInMessageLoop();

    EXPECT_TRUE(ash::wm::IsWindowMinimized(window));
  }

  // Test with gesture events.

  widget->Close();
}

// Test that closing the (browser) window with an opened balloon does not
// crash the system. In other words: Make sure that shutting down the frame
// destroys the opened balloon in an orderly fashion.
TEST_F(CustomFrameViewAshTest, MaximizeButtonExternalShutDown) {
  views::Widget* widget = CreateWidget();
  aura::Window* window = widget->GetNativeWindow();
  CustomFrameViewAsh* frame = custom_frame_view_ash(widget);
  CustomFrameViewAsh::TestApi test(frame);
  ash::FrameMaximizeButton* maximize_button = test.maximize_button();
  maximize_button->set_bubble_appearance_delay_ms(0);
  gfx::Point button_pos = maximize_button->GetBoundsInScreen().CenterPoint();
  gfx::Point off_pos(button_pos.x() + 100, button_pos.y() + 100);

  aura::test::EventGenerator generator(window->GetRootWindow(), off_pos);
  EXPECT_FALSE(maximize_button->maximizer());
  EXPECT_TRUE(ash::wm::IsWindowNormal(window));

  // Move the mouse cursor over the button to bring up the maximizer bubble.
  generator.MoveMouseTo(button_pos);
  EXPECT_TRUE(maximize_button->maximizer());

  // Even though the widget is closing the bubble menu should not crash upon
  // its delayed destruction.
  widget->CloseNow();
}

// Test that hovering over a button in the balloon dialog will show the phantom
// window. Moving then away from the button will hide it again. Then check that
// pressing and dragging the button itself off the button will also release the
// phantom window.
TEST_F(CustomFrameViewAshTest, MaximizeLeftButtonDragOut) {
  views::Widget* widget = CreateWidget();
  aura::Window* window = widget->GetNativeWindow();
  CustomFrameViewAsh* frame = custom_frame_view_ash(widget);
  CustomFrameViewAsh::TestApi test(frame);
  ash::FrameMaximizeButton* maximize_button = test.maximize_button();
  maximize_button->set_bubble_appearance_delay_ms(0);
  gfx::Point button_pos = maximize_button->GetBoundsInScreen().CenterPoint();
  gfx::Point off_pos(button_pos.x() + 100, button_pos.y() + 100);

  aura::test::EventGenerator generator(window->GetRootWindow(), off_pos);
  EXPECT_FALSE(maximize_button->maximizer());
  EXPECT_TRUE(ash::wm::IsWindowNormal(window));
  EXPECT_FALSE(maximize_button->phantom_window_open());

  // Move the mouse cursor over the button to bring up the maximizer bubble.
  generator.MoveMouseTo(button_pos);
  EXPECT_TRUE(maximize_button->maximizer());

  // Move the mouse over the left maximize button.
  gfx::Point left_max_pos = maximize_button->maximizer()->
      GetButtonForUnitTest(SNAP_LEFT)->GetBoundsInScreen().CenterPoint();

  generator.MoveMouseTo(left_max_pos);
  // Expect the phantom window to be open.
  EXPECT_TRUE(maximize_button->phantom_window_open());

  // Move away to see the window being destroyed.
  generator.MoveMouseTo(off_pos);
  EXPECT_FALSE(maximize_button->phantom_window_open());

  // Move back over the button.
  generator.MoveMouseTo(button_pos);
  generator.MoveMouseTo(left_max_pos);
  EXPECT_TRUE(maximize_button->phantom_window_open());

  // Press button and drag out of dialog.
  generator.PressLeftButton();
  generator.MoveMouseTo(off_pos);
  generator.ReleaseLeftButton();

  // Check that the phantom window is also gone.
  EXPECT_FALSE(maximize_button->phantom_window_open());
}

// Test that clicking a button in the maximizer bubble (in this case the
// maximize left button) will do the requested action.
TEST_F(CustomFrameViewAshTest, MaximizeLeftByButton) {
  const int kGridSize = ash::Shell::GetInstance()->GetGridSize();
  views::Widget* widget = CreateWidget();
  aura::Window* window = widget->GetNativeWindow();
  CustomFrameViewAsh* frame = custom_frame_view_ash(widget);
  CustomFrameViewAsh::TestApi test(frame);
  ash::FrameMaximizeButton* maximize_button = test.maximize_button();
  maximize_button->set_bubble_appearance_delay_ms(0);
  gfx::Point button_pos = maximize_button->GetBoundsInScreen().CenterPoint();
  gfx::Point off_pos(button_pos.x() + 100, button_pos.y() + 100);

  aura::test::EventGenerator generator(window->GetRootWindow(), off_pos);
  EXPECT_FALSE(maximize_button->maximizer());
  EXPECT_TRUE(ash::wm::IsWindowNormal(window));
  EXPECT_FALSE(maximize_button->phantom_window_open());

  // Move the mouse cursor over the button to bring up the maximizer bubble.
  generator.MoveMouseTo(button_pos);
  EXPECT_TRUE(maximize_button->maximizer());

  // Move the mouse over the left maximize button.
  gfx::Point left_max_pos = maximize_button->maximizer()->
      GetButtonForUnitTest(SNAP_LEFT)->GetBoundsInScreen().CenterPoint();
  generator.MoveMouseTo(left_max_pos);
  EXPECT_TRUE(maximize_button->phantom_window_open());
  generator.ClickLeftButton();

  EXPECT_FALSE(maximize_button->maximizer());
  EXPECT_FALSE(maximize_button->phantom_window_open());

  EXPECT_FALSE(ash::wm::IsWindowMaximized(window));
  EXPECT_FALSE(ash::wm::IsWindowMinimized(window));
  internal::SnapSizer sizer(window, button_pos,
      internal::SnapSizer::LEFT_EDGE, kGridSize);
  EXPECT_EQ(sizer.target_bounds().ToString(), window->bounds().ToString());
}

// Test that the activation focus does not change when the bubble gets shown.
TEST_F(CustomFrameViewAshTest, MaximizeKeepFocus) {
  views::Widget* widget = CreateWidget();
  aura::Window* window = widget->GetNativeWindow();
  CustomFrameViewAsh* frame = custom_frame_view_ash(widget);
  CustomFrameViewAsh::TestApi test(frame);
  ash::FrameMaximizeButton* maximize_button = test.maximize_button();
  maximize_button->set_bubble_appearance_delay_ms(0);
  gfx::Point button_pos = maximize_button->GetBoundsInScreen().CenterPoint();
  gfx::Point off_pos(button_pos.x() + 100, button_pos.y() + 100);

  aura::test::EventGenerator generator(window->GetRootWindow(), off_pos);
  EXPECT_FALSE(maximize_button->maximizer());
  EXPECT_TRUE(ash::wm::IsWindowNormal(window));

  aura::Window* active = window->GetFocusManager()->GetFocusedWindow();

  // Move the mouse cursor over the button to bring up the maximizer bubble.
  generator.MoveMouseTo(button_pos);
  EXPECT_TRUE(maximize_button->maximizer());

  // Check that the focused window is still the same.
  EXPECT_EQ(active, window->GetFocusManager()->GetFocusedWindow());
}

}  // namespace internal
}  // namespace ash
