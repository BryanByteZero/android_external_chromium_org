// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PANELS_PANEL_BROWSER_FRAME_VIEW_H_
#define CHROME_BROWSER_UI_PANELS_PANEL_BROWSER_FRAME_VIEW_H_
#pragma once

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "chrome/browser/extensions/extension_uninstall_dialog.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/tab_icon_view.h"
#include "ui/base/models/simple_menu_model.h"
#include "views/controls/button/button.h"
#include "views/controls/menu/view_menu_delegate.h"

class Extension;
class PanelBrowserView;
namespace views {
class ImageButton;
class Label;
class Menu2;
class MenuButton;
}

class PanelBrowserFrameView : public BrowserNonClientFrameView,
                              public views::ButtonListener,
                              public views::ViewMenuDelegate,
                              public ui::SimpleMenuModel::Delegate,
                              public TabIconView::TabIconViewModel,
                              public ExtensionUninstallDialog::Delegate {
 public:
  PanelBrowserFrameView(BrowserFrame* frame, PanelBrowserView* browser_view);
  virtual ~PanelBrowserFrameView();

  void UpdateTitleBar();
  void OnActivationChanged(bool active);

 protected:
  // Overridden from BrowserNonClientFrameView:
  virtual gfx::Rect GetBoundsForTabStrip(views::View* tabstrip) const OVERRIDE;
  virtual int GetHorizontalTabStripVerticalOffset(bool restored) const OVERRIDE;
  virtual void UpdateThrobber(bool running) OVERRIDE;

  // Overridden from views::NonClientFrameView:
  virtual gfx::Rect GetBoundsForClientView() const OVERRIDE;
  virtual gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const OVERRIDE;
  virtual int NonClientHitTest(const gfx::Point& point) OVERRIDE;
  virtual void GetWindowMask(const gfx::Size& size, gfx::Path* window_mask)
      OVERRIDE;
  virtual void EnableClose(bool enable) OVERRIDE;
  virtual void ResetWindowControls() OVERRIDE;
  virtual void UpdateWindowIcon() OVERRIDE;

  // Overridden from views::View:
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;
  virtual void OnThemeChanged() OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;
  virtual bool OnMousePressed(const views::MouseEvent& event) OVERRIDE;
  virtual bool OnMouseDragged(const views::MouseEvent& event) OVERRIDE;
  virtual void OnMouseReleased(const views::MouseEvent& event) OVERRIDE;
  virtual void OnMouseCaptureLost() OVERRIDE;

  // Overridden from views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender, const views::Event& event)
      OVERRIDE;

  // Overridden from views::ViewMenuDelegate:
  virtual void RunMenu(View* source, const gfx::Point& pt) OVERRIDE;

  // Overridden from ui::SimpleMenuModel::Delegate:
  virtual bool IsCommandIdChecked(int command_id) const OVERRIDE;
  virtual bool IsCommandIdEnabled(int command_id) const OVERRIDE;
  virtual bool GetAcceleratorForCommandId(
      int command_id, ui::Accelerator* accelerator) OVERRIDE;
  virtual void ExecuteCommand(int command_id) OVERRIDE;

  // Overridden from TabIconView::TabIconViewModel:
  virtual bool ShouldTabIconViewAnimate() const OVERRIDE;
  virtual SkBitmap GetFaviconForTabIconView() OVERRIDE;

  // ExtensionUninstallDialog::Delegate:
  virtual void ExtensionDialogAccepted() OVERRIDE;
  virtual void ExtensionDialogCanceled() OVERRIDE;

 private:
  friend class PanelBrowserViewTest;
  FRIEND_TEST_ALL_PREFIXES(PanelBrowserViewTest, CreatePanel);
  FRIEND_TEST_ALL_PREFIXES(PanelBrowserViewTest, ShowOrHideSettingsButton);

  enum PaintState {
    NOT_PAINTED,
    PAINT_AS_INACTIVE,
    PAINT_AS_ACTIVE
  };

  enum {
    COMMAND_NAME = 0,
    COMMAND_CONFIGURE,
    COMMAND_DISABLE,
    COMMAND_UNINSTALL,
    COMMAND_MANAGE
  };

  class MouseWatcher : public MessageLoopForUI::Observer {
   public:
    explicit MouseWatcher(PanelBrowserFrameView* view);
    virtual ~MouseWatcher();

    virtual bool IsCursorInViewBounds() const;

  #if defined(OS_WIN)
    virtual void WillProcessMessage(const MSG& msg) OVERRIDE { }
    virtual void DidProcessMessage(const MSG& msg) OVERRIDE;
  #else
    virtual void WillProcessEvent(GdkEvent* event) OVERRIDE { }
    virtual void DidProcessEvent(GdkEvent* event) OVERRIDE;
  #endif

   private:
    void HandleGlobalMouseMoveEvent();

    PanelBrowserFrameView* view_;
    bool is_mouse_within_;

    DISALLOW_COPY_AND_ASSIGN(MouseWatcher);
  };

  // Returns the thickness of the entire nonclient left, right, and bottom
  // borders, including both the window frame and any client edge.
  int NonClientBorderThickness() const;

  // Returns the height of the entire nonclient top border, including the window
  // frame, any title area, and any connected client edge.
  int NonClientTopBorderHeight() const;

  // Update control styles to indicate if the title bar is active or not.
  void UpdateControlStyles(PaintState paint_state);

  // Custom draw the frame.
  void PaintFrameBorder(gfx::Canvas* canvas);
  void PaintClientEdge(gfx::Canvas* canvas);

  // Called by MouseWatcher to notify if the mouse enters or leaves the window.
  void OnMouseEnterOrLeaveWindow(bool mouse_entered);

  // Make settings button visible if either of the conditions is met:
  // 1) The panel is active, i.e. having focus.
  // 2) The mouse is over the panel.
  void UpdateSettingsButtonVisibility(bool active, bool cursor_in_view);

  const Extension* GetExtension() const;

  void EnsureSettingsMenuCreated();

#ifdef UNIT_TEST
  void set_mouse_watcher(MouseWatcher* mouse_watcher) {
    mouse_watcher_.reset(mouse_watcher);
  }
#endif

  // The frame that hosts this view. This is a weak reference such that frame_
  // will always be valid in the lifetime of this view.
  BrowserFrame* frame_;

  // The client view hosted within this non-client frame view that is
  // guaranteed to be freed before the client view.
  // (see comments about view hierarchies in non_client_view.h)
  PanelBrowserView* browser_view_;

  PaintState paint_state_;
  views::MenuButton* settings_button_;
  views::ImageButton* close_button_;
  TabIconView* title_icon_;
  views::Label* title_label_;
  gfx::Rect client_view_bounds_;
  scoped_ptr<MouseWatcher> mouse_watcher_;
  scoped_ptr<views::Menu2> settings_menu_;
  scoped_ptr<ui::SimpleMenuModel> settings_menu_contents_;
  scoped_ptr<ExtensionUninstallDialog> extension_uninstall_dialog_;

  DISALLOW_COPY_AND_ASSIGN(PanelBrowserFrameView);
};

#endif  // CHROME_BROWSER_UI_PANELS_PANEL_BROWSER_FRAME_VIEW_H_
