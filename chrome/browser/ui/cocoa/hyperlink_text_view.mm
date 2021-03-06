// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/hyperlink_text_view.h"

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/nsview_additions.h"

// The baseline shift for text in the NSTextView.
const float kTextBaselineShift = -1.0;

@interface HyperlinkTextView(Private)
// Initialize the NSTextView properties for this subclass.
- (void)configureTextView;

// Change the current IBeamCursor to an arrowCursor.
- (void)fixupCursor;
@end

@implementation HyperlinkTextView

@synthesize drawsBackgroundUsingSuperview = drawsBackgroundUsingSuperview_;

- (id)initWithCoder:(NSCoder*)decoder {
  if ((self = [super initWithCoder:decoder]))
    [self configureTextView];
  return self;
}

- (id)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect]))
    [self configureTextView];
  return self;
}

- (BOOL)acceptsFirstResponder {
  return acceptsFirstResponder_;
}

- (void)drawViewBackgroundInRect:(NSRect)rect {
  if (drawsBackgroundUsingSuperview_)
    [self cr_drawUsingAncestor:[self superview] inRect:rect];
  else
    [super drawViewBackgroundInRect:rect];
}

// Never draw the insertion point (otherwise, it shows up without any user
// action if full keyboard accessibility is enabled).
- (BOOL)shouldDrawInsertionPoint {
  return NO;
}

- (NSRange)selectionRangeForProposedRange:(NSRange)proposedSelRange
                              granularity:(NSSelectionGranularity)granularity {
  // Do not allow selections.
  return NSMakeRange(0, 0);
}

// Convince NSTextView to not show an I-Beam cursor when the cursor is over the
// text view but not over actual text.
//
// http://www.mail-archive.com/cocoa-dev@lists.apple.com/msg10791.html
// "NSTextView sets the cursor over itself dynamically, based on considerations
// including the text under the cursor. It does so in -mouseEntered:,
// -mouseMoved:, and -cursorUpdate:, so those would be points to consider
// overriding."
- (void)mouseMoved:(NSEvent*)e {
  [super mouseMoved:e];
  [self fixupCursor];
}

- (void)mouseEntered:(NSEvent*)e {
  [super mouseEntered:e];
  [self fixupCursor];
}

- (void)cursorUpdate:(NSEvent*)e {
  [super cursorUpdate:e];
  [self fixupCursor];
}

- (void)configureTextView {
  [self setEditable:NO];
  [self setDrawsBackground:NO];
  [self setHorizontallyResizable:NO];
  [self setVerticallyResizable:NO];

  // When text is rendered, linkTextAttributes override anything set via
  // addAttributes for text that has NSLinkAttributeName. Set to nil to allow
  // custom attributes to take precedence.
  [self setLinkTextAttributes:nil];
  [self setDisplaysLinkToolTips:NO];

  acceptsFirstResponder_ = YES;
  drawsBackgroundUsingSuperview_ = NO;
}

- (void)fixupCursor {
  if ([[NSCursor currentCursor] isEqual:[NSCursor IBeamCursor]])
    [[NSCursor arrowCursor] set];
}

- (void)setMessageAndLink:(NSString*)message
                 withLink:(NSString*)link
                 atOffset:(NSUInteger)linkOffset
                     font:(NSFont*)font
             messageColor:(NSColor*)messageColor
                linkColor:(NSColor*)linkColor {
  NSMutableString* finalMessage = [NSMutableString stringWithString:message];
  [finalMessage insertString:link atIndex:linkOffset];
  [self setMessage:finalMessage withFont:font messageColor:messageColor];
  if ([link length] != 0) {
    [self addLinkRange:NSMakeRange(linkOffset, [link length])
              withName:@""
             linkColor:linkColor];
  }
}

- (void)setMessage:(NSString*)message
          withFont:(NSFont*)font
      messageColor:(NSColor*)messageColor {
  // Create an attributes dictionary for the message and link.
  NSDictionary* attributes = @{
    NSForegroundColorAttributeName : messageColor,
    NSCursorAttributeName : [NSCursor arrowCursor],
    NSFontAttributeName : font,
    NSBaselineOffsetAttributeName : @(kTextBaselineShift)
  };

  // Create the attributed string for the message.
  base::scoped_nsobject<NSAttributedString> attributedMessage(
      [[NSMutableAttributedString alloc] initWithString:message
                                             attributes:attributes]);

  // Update the text view with the new text.
  [[self textStorage] setAttributedString:attributedMessage];
}

- (void)addLinkRange:(NSRange)range
            withName:(id)name
           linkColor:(NSColor*)linkColor {
  NSDictionary* attributes = @{
    NSForegroundColorAttributeName : linkColor,
    NSUnderlineStyleAttributeName : @(YES),
    NSCursorAttributeName : [NSCursor pointingHandCursor],
    NSLinkAttributeName : name,
    NSUnderlineStyleAttributeName : @(NSSingleUnderlineStyle)
  };

  [[self textStorage] addAttributes:attributes range:range];
}

- (void)setAcceptsFirstResponder:(BOOL)acceptsFirstResponder {
  acceptsFirstResponder_ = acceptsFirstResponder;
}

@end
