// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/styled_text_field_cell.h"

#include "base/logging.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/nsview_additions.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/font.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

@implementation StyledTextFieldCell

- (CGFloat)baselineAdjust {
  return 0.0;
}

- (CGFloat)cornerRadius {
  return 0.0;
}

- (rect_path_utils::RoundedCornerFlags)roundedCornerFlags {
  return rect_path_utils::RoundedCornerAll;
}

- (BOOL)shouldDrawBezel {
  return NO;
}

// Returns the same value as textCursorFrameForFrame, but does not call it
// directly to avoid potential infinite loops.
- (NSRect)textFrameForFrame:(NSRect)cellFrame {
  return NSInsetRect(cellFrame, 0, [self baselineAdjust]);
}

// Returns the same value as textFrameForFrame, but does not call it directly to
// avoid potential infinite loops.
- (NSRect)textCursorFrameForFrame:(NSRect)cellFrame {
  return NSInsetRect(cellFrame, 0, [self baselineAdjust]);
}

// Override to show the I-beam cursor only in the area given by
// |textCursorFrameForFrame:|.
- (void)resetCursorRect:(NSRect)cellFrame inView:(NSView *)controlView {
  [super resetCursorRect:[self textCursorFrameForFrame:cellFrame]
                  inView:controlView];
}

// For NSTextFieldCell this is the area within the borders.  For our
// purposes, we count the info decorations as being part of the
// border.
- (NSRect)drawingRectForBounds:(NSRect)theRect {
  return [super drawingRectForBounds:[self textFrameForFrame:theRect]];
}

// TODO(shess): This code is manually drawing the cell's border area,
// but otherwise the cell assumes -setBordered:YES for purposes of
// calculating things like the editing area.  This is probably
// incorrect.  I know that this affects -drawingRectForBounds:.
- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  const CGFloat lineWidth = [controlView cr_lineWidth];
  const CGFloat halfLineWidth = lineWidth / 2.0;

  DCHECK([controlView isFlipped]);
  rect_path_utils::RoundedCornerFlags roundedCornerFlags =
      [self roundedCornerFlags];

  // TODO(shess): This inset is also reflected by |kFieldVisualInset|
  // in omnibox_popup_view_mac.mm.
  const NSRect frame = NSInsetRect(cellFrame, 0, lineWidth);
  const CGFloat radius = [self cornerRadius];

  // Paint button background image if there is one (otherwise the border won't
  // look right).
  ThemeService* themeProvider =
      static_cast<ThemeService*>([[controlView window] themeProvider]);
  if (themeProvider) {
    NSColor* backgroundImageColor =
        themeProvider->GetNSImageColorNamed(IDR_THEME_BUTTON_BACKGROUND, false);
    if (backgroundImageColor) {
      // Set the phase to match window.
      NSRect trueRect = [controlView convertRect:cellFrame toView:nil];
      NSPoint midPoint = NSMakePoint(NSMinX(trueRect), NSMaxY(trueRect));
      [[NSGraphicsContext currentContext] setPatternPhase:midPoint];

      // NOTE(shess): This seems like it should be using a 0.0 inset,
      // but AFAICT using a halfLineWidth inset is important in mixing the
      // toolbar background and the omnibox background.
      rect_path_utils::FillRectWithInset(roundedCornerFlags, frame,
                                         halfLineWidth, halfLineWidth, radius,
                                         backgroundImageColor);
    }

    // Draw the outer stroke (over the background).
    BOOL active = [[controlView window] isMainWindow];
    NSColor* strokeColor = themeProvider->GetNSColor(
        active ? ThemeService::COLOR_TOOLBAR_BUTTON_STROKE :
                 ThemeService::COLOR_TOOLBAR_BUTTON_STROKE_INACTIVE,
        true);
    rect_path_utils::FrameRectWithInset(roundedCornerFlags, frame, 0.0, 0.0,
                                        radius, lineWidth, strokeColor);
  }

  // Fill interior with background color.
  rect_path_utils::FillRectWithInset(roundedCornerFlags, frame, lineWidth,
                                     lineWidth, radius,
                                     [self backgroundColor]);

  // Draw the shadow.  For the rounded-rect case, the shadow needs to
  // slightly turn in at the corners.  |shadowFrame| is at the same
  // midline as the inner border line on the top and left, but at the
  // outer border line on the bottom and right.  The clipping change
  // will clip the bottom and right edges (and corner).
  {
    gfx::ScopedNSGraphicsContextSaveGState state;
    [rect_path_utils::RectPathWithInset(roundedCornerFlags, frame, lineWidth,
                                        lineWidth, radius) addClip];
    const NSRect shadowFrame =
        NSOffsetRect(frame, halfLineWidth, halfLineWidth);
    NSColor* shadowShade = [NSColor colorWithCalibratedWhite:0.0
                                                       alpha:0.05 / lineWidth];
    rect_path_utils::FrameRectWithInset(roundedCornerFlags, shadowFrame,
                                        halfLineWidth, halfLineWidth,
                                        radius - halfLineWidth, lineWidth,
                                        shadowShade);
  }

  // Draw optional bezel below bottom stroke.
  if ([self shouldDrawBezel] && themeProvider &&
      themeProvider->UsingDefaultTheme()) {

    NSColor* bezelColor = themeProvider->GetNSColor(
        ThemeService::COLOR_TOOLBAR_BEZEL, true);
    [[bezelColor colorWithAlphaComponent:0.5 / lineWidth] set];
    NSRect bezelRect = NSMakeRect(cellFrame.origin.x,
                                  NSMaxY(cellFrame) - lineWidth,
                                  NSWidth(cellFrame),
                                  lineWidth);
    bezelRect = NSInsetRect(bezelRect, radius - halfLineWidth, 0.0);
    NSRectFillUsingOperation(bezelRect, NSCompositeSourceOver);
  }

  // Draw the interior before the focus ring, to make sure nothing overlaps it.
  [self drawInteriorWithFrame:cellFrame inView:controlView];

  // Draw the focus ring if needed.
  if ([self showsFirstResponder]) {
    NSColor* color = [[NSColor keyboardFocusIndicatorColor]
        colorWithAlphaComponent:0.5 / lineWidth];
    rect_path_utils::FrameRectWithInset(roundedCornerFlags, frame, 0.0, 0.0,
                                        radius, lineWidth * 2, color);
  }
}

@end
