// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_MAC_H_
#define CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_MAC_H_
#pragma once

#import <Cocoa/Cocoa.h>

#include "base/memory/scoped_nsobject.h"
#include "base/memory/scoped_ptr.h"
#include "base/task.h"
#include "base/time.h"
#include "chrome/browser/ui/cocoa/base_view.h"
#include "content/browser/accessibility/browser_accessibility_delegate_mac.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/renderer_host/accelerated_surface_container_manager_mac.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "content/common/edit_command.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCompositionUnderline.h"
#include "webkit/glue/webcursor.h"

@class AcceleratedPluginView;
class RenderWidgetHostViewMac;
class RWHVMEditCommandHelper;
@class ToolTip;

@protocol RenderWidgetHostViewMacOwner
- (RenderWidgetHostViewMac*)renderWidgetHostViewMac;
@end

// This is the view that lives in the Cocoa view hierarchy. In Windows-land,
// RenderWidgetHostViewWin is both the view and the delegate. We split the roles
// but that means that the view needs to own the delegate and will dispose of it
// when it's removed from the view system.
// See http://crbug.com/47890 for why we don't use NSTextInputClient yet.
@interface RenderWidgetHostViewCocoa
    : BaseView <RenderWidgetHostViewMacOwner,
                NSTextInput,
                NSChangeSpelling,
                BrowserAccessibilityDelegateCocoa> {
 @private
  scoped_ptr<RenderWidgetHostViewMac> renderWidgetHostView_;
  BOOL canBeKeyView_;
  BOOL takesFocusOnlyOnMouseDown_;
  BOOL closeOnDeactivate_;
  scoped_ptr<RWHVMEditCommandHelper> editCommand_helper_;

  // These are part of the magic tooltip code from WebKit's WebHTMLView:
  id trackingRectOwner_;              // (not retained)
  void *trackingRectUserData_;
  NSTrackingRectTag lastToolTipTag_;
  scoped_nsobject<NSString> toolTip_;

  // Is YES if there was a mouse-down as yet unbalanced with a mouse-up.
  BOOL hasOpenMouseDown_;

  NSWindow* lastWindow_;  // weak

  // Variables used by our implementaion of the NSTextInput protocol.
  // An input method of Mac calls the methods of this protocol not only to
  // notify an application of its status, but also to retrieve the status of
  // the application. That is, an application cannot control an input method
  // directly.
  // This object keeps the status of a composition of the renderer and returns
  // it when an input method asks for it.
  // We need to implement Objective-C methods for the NSTextInput protocol. On
  // the other hand, we need to implement a C++ method for an IPC-message
  // handler which receives input-method events from the renderer.

  // Represents the input-method attributes supported by this object.
  scoped_nsobject<NSArray> validAttributesForMarkedText_;

  // Indicates if we are currently handling a key down event.
  BOOL handlingKeyDown_;

  // Indicates if there is any marked text.
  BOOL hasMarkedText_;

  // Indicates if unmarkText is called or not when handling a keyboard
  // event.
  BOOL unmarkTextCalled_;

  // The range of current marked text inside the whole content of the DOM node
  // being edited.
  // TODO(suzhe): This is currently a fake value, as we do not support accessing
  // the whole content yet.
  NSRange markedRange_;

  // The selected range, cached from a message sent by the renderer.
  NSRange selectedRange_;

  // Text to be inserted which was generated by handling a key down event.
  string16 textToBeInserted_;

  // Marked text which was generated by handling a key down event.
  string16 markedText_;

  // Underline information of the |markedText_|.
  std::vector<WebKit::WebCompositionUnderline> underlines_;

  // Indicates if doCommandBySelector method receives any edit command when
  // handling a key down event.
  BOOL hasEditCommands_;

  // Contains edit commands received by the -doCommandBySelector: method when
  // handling a key down event, not including inserting commands, eg. insertTab,
  // etc.
  EditCommands editCommands_;

  // The plugin that currently has focus (-1 if no plugin has focus).
  int focusedPluginIdentifier_;

  // Whether or not plugin IME is currently enabled active.
  BOOL pluginImeActive_;

  // Whether the previous mouse event was ignored due to hitTest check.
  BOOL mouseEventWasIgnored_;

  // If a scroll event came back unhandled from the renderer. Set to |NO| at
  // the start of a scroll gesture, and then to |YES| if a scroll event comes
  // back unhandled from the renderer.
  // Used for history swiping.
  BOOL gotUnhandledWheelEvent_;

  // Cummulative scroll delta since scroll gesture start. Only valid during
  // scroll gesture handling. Used for history swiping.
  NSSize totalScrollDelta_;

  // If the viewport is scrolled all the way to the left or right.
  // Used for history swiping.
  BOOL isPinnedLeft_;
  BOOL isPinnedRight_;

  // If the main frame has a horizontal scrollbar.
  // Used for history swiping.
  BOOL hasHorizontalScrollbar_;

  // Event monitor for gesture-end events.
  id endGestureMonitor_;
}

@property(nonatomic, readonly) NSRange selectedRange;
@property(nonatomic) BOOL gotUnhandledWheelEvent;
@property(nonatomic, setter=setPinnedLeft:) BOOL isPinnedLeft;
@property(nonatomic, setter=setPinnedRight:) BOOL isPinnedRight;
@property(nonatomic) BOOL hasHorizontalScrollbar;

- (void)setCanBeKeyView:(BOOL)can;
- (void)setTakesFocusOnlyOnMouseDown:(BOOL)b;
- (void)setCloseOnDeactivate:(BOOL)b;
- (void)setToolTipAtMousePoint:(NSString *)string;
// Set frame, then notify the RenderWidgetHost that the frame has been changed,
// but do it in a separate task, using |performSelector:withObject:afterDelay:|.
// This stops the flickering issue in http://crbug.com/31970
- (void)setFrameWithDeferredUpdate:(NSRect)frame;
// Notify the RenderWidgetHost that the frame was updated so it can resize
// its contents.
- (void)renderWidgetHostWasResized;
// Cancel ongoing composition (abandon the marked text).
- (void)cancelComposition;
// Confirm ongoing composition.
- (void)confirmComposition;
// Enables or disables plugin IME.
- (void)setPluginImeActive:(BOOL)active;
// Updates the current plugin focus state.
- (void)pluginFocusChanged:(BOOL)focused forPlugin:(int)pluginId;
// Evaluates the event in the context of plugin IME, if plugin IME is enabled.
// Returns YES if the event was handled.
- (BOOL)postProcessEventForPluginIme:(NSEvent*)event;

@end

///////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostViewMac
//
//  An object representing the "View" of a rendered web page. This object is
//  responsible for displaying the content of the web page, and integrating with
//  the Cocoa view system. It is the implementation of the RenderWidgetHostView
//  that the cross-platform RenderWidgetHost object uses
//  to display the data.
//
//  Comment excerpted from render_widget_host.h:
//
//    "The lifetime of the RenderWidgetHost* is tied to the render process.
//     If the render process dies, the RenderWidgetHost* goes away and all
//     references to it must become NULL."
//
class RenderWidgetHostViewMac : public RenderWidgetHostView {
 public:
  // The view will associate itself with the given widget. The native view must
  // be hooked up immediately to the view hierarchy, or else when it is
  // deleted it will delete this out from under the caller.
  explicit RenderWidgetHostViewMac(RenderWidgetHost* widget);
  virtual ~RenderWidgetHostViewMac();

  RenderWidgetHostViewCocoa* native_view() const { return cocoa_view_; }

  // Implementation of RenderWidgetHostView:
  virtual void InitAsPopup(RenderWidgetHostView* parent_host_view,
                           const gfx::Rect& pos) OVERRIDE;
  virtual void InitAsFullscreen(
      RenderWidgetHostView* reference_host_view) OVERRIDE;
  virtual RenderWidgetHost* GetRenderWidgetHost() const OVERRIDE;
  virtual void DidBecomeSelected() OVERRIDE;
  virtual void WasHidden() OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual void SetBounds(const gfx::Rect& rect) OVERRIDE;
  virtual gfx::NativeView GetNativeView() OVERRIDE;
  virtual void MovePluginWindows(
      const std::vector<webkit::npapi::WebPluginGeometry>& moves) OVERRIDE;
  virtual void Focus() OVERRIDE;
  virtual void Blur() OVERRIDE;
  virtual bool HasFocus() OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual bool IsShowing() OVERRIDE;
  virtual gfx::Rect GetViewBounds() const OVERRIDE;
  virtual void UpdateCursor(const WebCursor& cursor) OVERRIDE;
  virtual void SetIsLoading(bool is_loading) OVERRIDE;
  virtual void ImeUpdateTextInputState(ui::TextInputType state,
                                       bool can_compose_inline,
                                       const gfx::Rect& caret_rect) OVERRIDE;
  virtual void ImeCancelComposition() OVERRIDE;
  virtual void ImeCompositionRangeChanged(const ui::Range& range) OVERRIDE;
  virtual void DidUpdateBackingStore(
      const gfx::Rect& scroll_rect, int scroll_dx, int scroll_dy,
      const std::vector<gfx::Rect>& copy_rects) OVERRIDE;
  virtual void RenderViewGone(base::TerminationStatus status,
                              int error_code) OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual void SetTooltipText(const std::wstring& tooltip_text) OVERRIDE;
  virtual void SelectionChanged(const std::string& text,
                                const ui::Range& range,
                                const gfx::Point& start,
                                const gfx::Point& end) OVERRIDE;
  virtual void ShowingContextMenu(bool showing) OVERRIDE;
  virtual BackingStore* AllocBackingStore(const gfx::Size& size) OVERRIDE;
  virtual void SetTakesFocusOnlyOnMouseDown(bool flag) OVERRIDE;
  // See comment in RenderWidgetHostView!
  virtual gfx::Rect GetViewCocoaBounds() const OVERRIDE;
  virtual gfx::Rect GetRootWindowRect() OVERRIDE;
  virtual void SetActive(bool active) OVERRIDE;
  virtual void SetWindowVisibility(bool visible) OVERRIDE;
  virtual void WindowFrameChanged() OVERRIDE;
  virtual void SetBackground(const SkBitmap& background) OVERRIDE;

  virtual void OnAccessibilityNotifications(
      const std::vector<ViewHostMsg_AccessibilityNotification_Params>& params
      ) OVERRIDE;

  virtual void PluginFocusChanged(bool focused, int plugin_id) OVERRIDE;
  virtual void StartPluginIme() OVERRIDE;
  virtual bool PostProcessEventForPluginIme(
      const NativeWebKeyboardEvent& event) OVERRIDE;

  // Methods associated with GPU-accelerated plug-in instances and the
  // accelerated compositor.
  virtual gfx::PluginWindowHandle AllocateFakePluginWindowHandle(
      bool opaque, bool root) OVERRIDE;
  virtual void DestroyFakePluginWindowHandle(
      gfx::PluginWindowHandle window) OVERRIDE;

  // Exposed for testing.
  AcceleratedPluginView* ViewForPluginWindowHandle(
      gfx::PluginWindowHandle window);

  // Helper to do the actual cleanup after a plugin handle has been destroyed.
  // Required because DestroyFakePluginWindowHandle() isn't always called for
  // all handles (it's e.g. not called on navigation, when the RWHVMac gets
  // destroyed anyway).
  void DeallocFakePluginWindowHandle(gfx::PluginWindowHandle window);

  virtual void AcceleratedSurfaceSetIOSurface(
      gfx::PluginWindowHandle window,
      int32 width,
      int32 height,
      uint64 io_surface_identifier) OVERRIDE;
  virtual void AcceleratedSurfaceSetTransportDIB(
      gfx::PluginWindowHandle window,
      int32 width,
      int32 height,
      TransportDIB::Handle transport_dib) OVERRIDE;
  virtual void AcceleratedSurfaceBuffersSwapped(
      gfx::PluginWindowHandle window,
      uint64 surface_id,
      int renderer_id,
      int32 route_id,
      int gpu_host_id,
      uint64 swap_buffers_count);
  virtual void GpuRenderingStateDidChange() OVERRIDE;

  virtual gfx::PluginWindowHandle GetCompositingSurface() OVERRIDE;

  // Returns |true| if a context menu is currently being shown.
  bool is_showing_context_menu() const { return is_showing_context_menu_; }

  void DrawAcceleratedSurfaceInstance(
      CGLContextObj context,
      gfx::PluginWindowHandle plugin_handle,
      NSSize size);
  // Forces the textures associated with any accelerated plugin instances
  // to be reloaded.
  void ForceTextureReload();

  virtual void SetVisuallyDeemphasized(const SkColor* color, bool animate);

  virtual void UnhandledWheelEvent(
      const WebKit::WebMouseWheelEvent& event) OVERRIDE;
  virtual void SetHasHorizontalScrollbar(
      bool has_horizontal_scrollbar) OVERRIDE;
  virtual void SetScrollOffsetPinning(
      bool is_pinned_to_left, bool is_pinned_to_right) OVERRIDE;

  void KillSelf();

  void SetTextInputActive(bool active);

  // Sends completed plugin IME notification and text back to the renderer.
  void PluginImeCompositionCompleted(const string16& text, int plugin_id);

  const std::string& selected_text() const { return selected_text_; }

  void UpdateRootGpuViewVisibility(bool show_gpu_widget);

  // When rendering transitions from gpu to software, the gpu widget can't be
  // hidden until the software backing store has been updated. This method
  // checks if the GPU view needs to be hidden and hides it if necessary. It
  // should be called after the software backing store has been painted to.
  void HandleDelayedGpuViewHiding();

  // This is called from the display link thread, and provides the GPU
  // process a notion of how quickly the browser is able to keep up with it.
  void AcknowledgeSwapBuffers(int renderer_id,
                              int32 route_id,
                              int gpu_host_id,
                              uint64 swap_buffers_count);

  void ToggleSpellCheck(bool enabled, bool checked);

  // These member variables should be private, but the associated ObjC class
  // needs access to them and can't be made a friend.

  // The associated Model.  Can be NULL if Destroy() is called when
  // someone (other than superview) has retained |cocoa_view_|.
  RenderWidgetHost* render_widget_host_;

  // This is true when we are currently painting and thus should handle extra
  // paint requests by expanding the invalid rect rather than actually painting.
  bool about_to_validate_and_paint_;

  scoped_ptr<BrowserAccessibilityManager> browser_accessibility_manager_;

  // This is true when we have already scheduled a call to
  // |-callSetNeedsDisplayInRect:| but it has not been fulfilled yet.  Used to
  // prevent us from scheduling multiple calls.
  bool call_set_needs_display_in_rect_pending_;

  // The invalid rect that needs to be painted by callSetNeedsDisplayInRect.
  // This value is only meaningful when
  // |call_set_needs_display_in_rect_pending_| is true.
  NSRect invalid_rect_;

  // The time at which this view started displaying white pixels as a result of
  // not having anything to paint (empty backing store from renderer). This
  // value returns true for is_null() if we are not recording whiteout times.
  base::TimeTicks whiteout_start_time_;

  // The time it took after this view was selected for it to be fully painted.
  base::TimeTicks tab_switch_paint_time_;

  // Current text input type.
  ui::TextInputType text_input_type_;

  typedef std::map<gfx::PluginWindowHandle, AcceleratedPluginView*>
      PluginViewMap;
  PluginViewMap plugin_views_;  // Weak values.

  // Helper class for managing instances of accelerated plug-ins.
  AcceleratedSurfaceContainerManagerMac plugin_container_manager_;

  // Used for continuous spell checking.
  bool spellcheck_enabled_;
  bool spellcheck_checked_;

 private:
  // Returns whether this render view is a popup (autocomplete window).
  bool IsPopup() const;

  // Updates the display cursor if the current event is over the view's window.
  void UpdateCursorIfNecessary();

  // Shuts down the render_widget_host_.  This is a separate function so we can
  // invoke it from the message loop.
  void ShutdownHost();

  // The associated view. This is weak and is inserted into the view hierarchy
  // to own this RenderWidgetHostViewMac object.
  RenderWidgetHostViewCocoa* cocoa_view_;

  // The cursor for the page. This is passed up from the renderer.
  WebCursor current_cursor_;

  // Indicates if the page is loading.
  bool is_loading_;

  // true if the View is not visible.
  bool is_hidden_;

  // Whether we are showing a context menu.
  bool is_showing_context_menu_;

  // The text to be shown in the tooltip, supplied by the renderer.
  std::wstring tooltip_text_;

  // Factory used to safely scope delayed calls to ShutdownHost().
  ScopedRunnableMethodFactory<RenderWidgetHostViewMac> shutdown_factory_;

  // selected text on the renderer.
  std::string selected_text_;

  // When rendering transitions from gpu to software, the gpu widget can't be
  // hidden until the software backing store has been updated. This variable is
  // set when the gpu widget needs to be hidden once a paint is completed.
  bool needs_gpu_visibility_update_after_repaint_;

  gfx::PluginWindowHandle compositing_surface_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewMac);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_MAC_H_
