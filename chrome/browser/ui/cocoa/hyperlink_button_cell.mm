// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/hyperlink_button_cell.h"

@interface HyperlinkButtonCell (Private)
- (NSDictionary*)linkAttributres;
- (void)customizeButtonCell;
@end

@implementation HyperlinkButtonCell
@dynamic textColor;

+ (NSColor*)defaultTextColor {
  return [NSColor blueColor];
}

// Designated initializer.
- (id)init {
  if ((self = [super init])) {
    [self customizeButtonCell];
  }
  return self;
}

// Initializer called when the cell is loaded from the NIB.
- (id)initWithCoder:(NSCoder*)aDecoder {
  if ((self = [super initWithCoder:aDecoder])) {
    [self customizeButtonCell];
  }
  return self;
}

// Initializer for code-based creation.
- (id)initTextCell:(NSString*)title {
  if ((self = [super initTextCell:title])) {
    [self customizeButtonCell];
  }
  return self;
}

// Because an NSButtonCell has multiple initializers, this method performs the
// common cell customization code.
- (void)customizeButtonCell {
  [self setBordered:NO];
  [self setTextColor:[HyperlinkButtonCell defaultTextColor]];

  CGFloat fontSize = [NSFont systemFontSizeForControlSize:[self controlSize]];
  NSFont* font = [NSFont controlContentFontOfSize:fontSize];
  [self setFont:font];

  // Do not change button appearance when we are pushed.
  [self setHighlightsBy:NSNoCellMask];

  // We need to set this so that we can override |-mouseEntered:| and
  // |-mouseExited:| to change the cursor style on hover states.
  [self setShowsBorderOnlyWhileMouseInside:YES];
}

- (void)setControlSize:(NSControlSize)size {
  [super setControlSize:size];
  [self customizeButtonCell];  // recompute |font|.
}

// Creates the NSDictionary of attributes for the attributed string.
- (NSDictionary*)linkAttributes {
  NSUInteger underlineMask = NSUnderlinePatternSolid | NSUnderlineStyleSingle;
  scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
    [[NSParagraphStyle defaultParagraphStyle] mutableCopy]);
  [paragraphStyle setAlignment:[self alignment]];

  return [NSDictionary dictionaryWithObjectsAndKeys:
      [self textColor], NSForegroundColorAttributeName,
      [NSNumber numberWithInt:underlineMask], NSUnderlineStyleAttributeName,
      [self font], NSFontAttributeName,
      [NSCursor pointingHandCursor], NSCursorAttributeName,
      paragraphStyle.get(), NSParagraphStyleAttributeName,
      nil
  ];
}

// Override the drawing for the cell so that the custom style attributes
// can always be applied and so that ellipses will appear when appropriate.
- (NSRect)drawTitle:(NSAttributedString*)title
          withFrame:(NSRect)frame
             inView:(NSView*)controlView {
  NSDictionary* linkAttributes = [self linkAttributes];
  NSString* plainTitle = [title string];
  [plainTitle drawWithRect:frame
                   options:(NSStringDrawingUsesLineFragmentOrigin |
                            NSStringDrawingTruncatesLastVisibleLine)
                attributes:linkAttributes];
  return frame;
}

// Override the default behavior to draw the border. Instead, change the cursor.
- (void)mouseEntered:(NSEvent*)event {
  if ([self isEnabled])
    [[NSCursor pointingHandCursor] push];
  else
    [[NSCursor currentCursor] push];
}

- (void)mouseExited:(NSEvent*)event {
  [NSCursor pop];
}

// Setters and getters.
- (NSColor*)textColor {
  if ([self isEnabled])
    return textColor_.get();
  else
    return [NSColor disabledControlTextColor];
}

- (void)setTextColor:(NSColor*)color {
  textColor_.reset([color retain]);
}

// Override so that |-sizeToFit| works better with this type of cell.
- (NSSize)cellSize {
  NSSize size = [super cellSize];
  size.width += 2;
  return size;
}

@end
