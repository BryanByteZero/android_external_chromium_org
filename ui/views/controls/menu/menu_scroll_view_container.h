// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_MENU_MENU_SCROLL_VIEW_CONTAINER_H_
#define UI_VIEWS_CONTROLS_MENU_MENU_SCROLL_VIEW_CONTAINER_H_

#include "ui/views/view.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/menu/menu_item_view.h"

namespace views {

class SubmenuView;

// MenuScrollViewContainer contains the SubmenuView (through a MenuScrollView)
// and two scroll buttons. The scroll buttons are only visible and enabled if
// the preferred height of the SubmenuView is bigger than our bounds.
class MenuScrollViewContainer : public View {
 public:
  explicit MenuScrollViewContainer(SubmenuView* content_view);

  // Returns the buttons for scrolling up/down.
  View* scroll_down_button() const { return scroll_down_button_; }
  View* scroll_up_button() const { return scroll_up_button_; }

  // External function to check if the bubble border is usd.
  bool HasBubbleBorder();

  // Offsets the Arrow from the default location.
  void SetBubbleArrowOffset(int offset);

  // View overrides.
  virtual void OnPaintBackground(gfx::Canvas* canvas) OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

 protected:
  // View override.
  virtual void OnBoundsChanged(const gfx::Rect& previous_bounds) OVERRIDE;

 private:
  // Create the default border.
  void CreateDefaultBorder();

  // Create the bubble border.
  void CreateBubbleBorder();

  BubbleBorder::Arrow BubbleBorderTypeFromAnchor(
      MenuItemView::AnchorPosition anchor);

  class MenuScrollView;

  // The scroll buttons.
  View* scroll_up_button_;
  View* scroll_down_button_;

  // The scroll view.
  MenuScrollView* scroll_view_;

  // The content view.
  SubmenuView* content_view_;

  // If set the currently set border is a bubble border.
  BubbleBorder::Arrow arrow_;

  // Weak reference to the currently set border.
  BubbleBorder* bubble_border_;

  DISALLOW_COPY_AND_ASSIGN(MenuScrollViewContainer);
};

}  // namespace views

#endif  // UI_VIEWS_CONTROLS_MENU_MENU_SCROLL_VIEW_CONTAINER_H_
