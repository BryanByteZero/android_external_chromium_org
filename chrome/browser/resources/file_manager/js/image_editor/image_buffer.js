// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A stack of overlays that display itself and handle mouse events.
 * TODO(kaznacheev) Consider disbanding this class and moving
 * the functionality to individual objects that display anything or handle
 * mouse events.
 * @constructor
 */
function ImageBuffer() {
  this.overlays_ = [];
}

/**
 * @param {ImageBuffer.Overlay} overlay
 */
ImageBuffer.prototype.addOverlay = function(overlay) {
  var zIndex = overlay.getZIndex();
  // Store the overlays in the ascending Z-order.
  var i;
  for (i = 0; i != this.overlays_.length; i++) {
    if (zIndex < this.overlays_[i].getZIndex()) break;
  }
  this.overlays_.splice(i, 0, overlay);
};

/**
 * @param {ImageBuffer.Overlay} overlay
 */
ImageBuffer.prototype.removeOverlay = function(overlay) {
  for (var i = 0; i != this.overlays_.length; i++) {
    if (this.overlays_[i] == overlay) {
      this.overlays_.splice(i, 1);
      return;
    }
  }
  throw new Error('Cannot remove overlay ' + overlay);
};

/**
 * Draws overlays in the ascending Z-order.
 */
ImageBuffer.prototype.draw = function() {
  for (var i = 0; i != this.overlays_.length; i++) {
    this.overlays_[i].draw();
  }
};

/**
 * Searches for a cursor style in the descending Z-order.
 * @return {String} A value for style.cursor CSS property.
 */
ImageBuffer.prototype.getCursorStyle = function(x, y, mouseDown) {
  for (var i = this.overlays_.length - 1; i >= 0; i--) {
    var style = this.overlays_[i].getCursorStyle(x, y, mouseDown);
    if (style) return style;
  }
  return 'default';
};

/**
 * Searches for a click handler in the descending Z-order.
 * @return {Boolean} True if handled.
 */
ImageBuffer.prototype.onClick = function(x, y) {
  for (var i = this.overlays_.length - 1; i >= 0; i--) {
    if (this.overlays_[i].onClick(x, y)) return true;
  }
  return false;
};

/**
 * Searches for a drag handler in the descending Z-order.
 * @param {number} x Event X coordinate.
 * @param {number} y Event Y coordinate.
 * @param {boolean} touch True if it's a touch event, false if mouse.
 * @return {function(number,number)} A function to be called on mouse drag.
 */
ImageBuffer.prototype.getDragHandler = function(x, y, touch) {
  for (var i = this.overlays_.length - 1; i >= 0; i--) {
    var handler = this.overlays_[i].getDragHandler(x, y, touch);
    if (handler)
      return handler;
  }
  return null;
};

/**
 * Searches for an action for the double tap enumerating
 * layers in the descending Z-order.
 * @param {number} x X coordinate of the event.
 * @param {number} y Y coordinate of the event.
 * @return {ImageBuffer.DoubleTapAction}
 */
ImageBuffer.prototype.getDoubleTapAction = function(x, y) {
  for (var i = this.overlays_.length - 1; i >= 0; i--) {
    var action = this.overlays_[i].getDoubleTapAction(x, y);
    if (action != ImageBuffer.DoubleTapAction.NOTHING)
      return action;
  }
  return ImageBuffer.DoubleTapAction.NOTHING;
};

/**
 * Possible double tap actions.
 * @enum
 */
ImageBuffer.DoubleTapAction = {
  NOTHING: 0,
  COMMIT: 1,
  CANCEL: 2
};

/**
 * ImageBuffer.Overlay is a pluggable extension that modifies the outlook
 * and the behavior of the ImageBuffer instance.
 */
ImageBuffer.Overlay = function() {};

ImageBuffer.Overlay.prototype.getZIndex = function() { return 0 };

ImageBuffer.Overlay.prototype.draw = function() {};

ImageBuffer.Overlay.prototype.getCursorStyle = function() { return null };

ImageBuffer.Overlay.prototype.onClick = function() { return false };

ImageBuffer.Overlay.prototype.getDragHandler = function() { return null };

ImageBuffer.Overlay.prototype.getDoubleTapAction = function(x, y) {
  return ImageBuffer.DoubleTapAction.NOTHING;
};
