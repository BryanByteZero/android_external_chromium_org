// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// require: event_tracker.js

cr.define('cr.ui', function() {
  'use strict';

  /**
   * ExpandableBubble is a free-floating compact informational bubble with an
   * arrow that points at a place of interest on the page. When clicked, the
   * bubble expands to show more of its content. Width of the bubble is the
   * width of the node it is overlapping when unexpanded. Expanded, it is of a
   * fixed width, but variable height. Currently the arrow is always positioned
   * at the bottom right and points down.
   * @constructor
   * @extends {cr.ui.div}
   */
  var ExpandableBubble = cr.ui.define('div');

  ExpandableBubble.prototype = {
    __proto__: HTMLDivElement.prototype,

    /** @inheritDoc */
    decorate: function() {
      this.className = 'expandable-bubble';
      this.innerHTML =
          '<div class="expandable-bubble-contents">' +
            '<div class="expandable-bubble-title"></div>' +
            '<div class="expandable-bubble-main" hidden></div>' +
          '</div>' +
          '<div class="expandable-bubble-close" hidden></div>' +
          '<div class="expandable-bubble-shadow"></div>' +
          '<div class="expandable-bubble-arrow"></div>';

      this.hidden = true;
      this.bubbleSuppressed = false;
      this.handleCloseEvent = this.hide;
    },

    /**
     * Sets the title of the bubble. The title is always visible when the
     * bubble is visible.
     * @type {Node} An HTML element to set as the title.
     */
    set contentTitle(node) {
      var bubbleTitle = this.querySelector('.expandable-bubble-title');
      bubbleTitle.textContent = '';
      bubbleTitle.appendChild(node);
    },

    /**
     * Sets the content node of the bubble. The content node is only visible
     * when the bubble is expanded.
     * @param {Node} An HTML element.
     */
    set content(node) {
      var bubbleMain = this.querySelector('.expandable-bubble-main');
      bubbleMain.textContent = '';
      bubbleMain.appendChild(node);
    },

    /**
     * Sets the anchor node, i.e. the node that this bubble points at and
     * partially overlaps.
     * @param {HTMLElement} node The new anchor node.
     */
    set anchorNode(node) {
      this.anchorNode_ = node;

      if (!this.hidden)
        this.resizeAndReposition();
    },

    /**
     * Handles the close event which is triggered when the close button
     * is clicked. By default is set to this.hide.
     * @param {function} A function with no parameters
     */
    set handleCloseEvent(func) {
      this.handleCloseEvent_ = func;
    },

    /**
     * Temporarily suppresses the bubble from view (and toggles it back).
     * 'Suppressed' and 'hidden' are two bubble states that both indicate that
     * the bubble should not be visible, but when you 'un-suppress' a bubble,
     * only a suppressed bubble becomes visible. This can be handy, for example,
     * if the user switches away from the app card (then we need to know which
     * bubbles to show (only the suppressed ones, not the hidden ones). Hiding
     * and un-hiding a bubble overrides the suppressed state (a bubble cannot
     * be suppressed but not hidden).
     */
    set suppressed(suppress) {
      if (suppress) {
        // If the bubble is already hidden, then we don't need to suppress it.
        if (this.hidden)
          return;

        this.hidden = true;
      } else if (this.bubbleSuppressed) {
        this.hidden = false;
      }
      this.bubbleSuppressed = suppress;
      this.resizeAndReposition(this);
    },

    /**
     * Updates the position of the bubble.
     * @private
     */
    reposition_: function() {
      var clientRect = this.anchorNode_.getBoundingClientRect();

      this.style.left = this.style.right = clientRect.left + 'px';

      var top = clientRect.top - 1;
      this.style.top = this.expanded ?
          (top - this.offsetHeight + this.unexpandedHeight) + 'px' :
          top + 'px';
    },

    /**
     * Resizes the bubble and then repositions it.
     * @private
     */
    resizeAndReposition: function() {
      var clientRect = this.anchorNode_.getBoundingClientRect();
      var width = clientRect.width;

      var bubbleTitle = this.querySelector('.expandable-bubble-title');
      var closeElement = this.querySelector('.expandable-bubble-close');
      var closeWidth = this.expanded ? closeElement.clientWidth : 0;
      var margin = 12;

      if (this.expanded) {
        // We always show the full title but never show less width than 250
        // pixels.
        var expandedWidth =
            Math.max(250, bubbleTitle.scrollWidth + closeWidth + margin);
        this.style.marginLeft = (width - expandedWidth) + 'px';
        width = expandedWidth;
      } else {
        this.style.marginLeft = '0';
      }

      // Width is dynamic (when not expanded) based on the width of the anchor
      // node, and the title and shadow need to follow suit.
      this.style.width = width + 'px';
      bubbleTitle.style.width = Math.max(0, width - margin - closeWidth) + 'px';
      var bubbleContent = this.querySelector('.expandable-bubble-main');
      bubbleContent.style.width = Math.max(0, width - margin) + 'px';
      var bubbleShadow = this.querySelector('.expandable-bubble-shadow');
      bubbleShadow.style.width = width ? width + 2 + 'px' : 0 + 'px';

      // Also reposition the bubble -- dimensions have potentially changed.
      this.reposition_();
    },

    /*
     * Expand the bubble (bringing the full content into view).
     * @private
     */
    expandBubble_: function() {
      this.querySelector('.expandable-bubble-main').hidden = false;
      this.querySelector('.expandable-bubble-close').hidden = false;
      this.expanded = true;
      this.resizeAndReposition();
    },

    /**
     * Collapse the bubble, hiding the main content and the close button.
     * This is automatically called when the window is resized.
     * @private
     */
    collapseBubble_: function() {
      this.querySelector('.expandable-bubble-main').hidden = true;
      this.querySelector('.expandable-bubble-close').hidden = true;
      this.expanded = false;
      this.resizeAndReposition();
    },

    /**
     * The onclick handler for the notification (expands the bubble).
     * @param {Event} e The event.
     * @private
     */
    onNotificationClick_ : function(e) {
      if (!this.contains(e.target))
        return;

      if (!this.expanded) {
        // Save the height of the unexpanded bubble, so we can make sure to
        // position it correctly (arrow points in the same location) after
        // we expand it.
        this.unexpandedHeight = this.offsetHeight;
      }

      this.expandBubble_();
    },

    /**
     * Shows the bubble. The bubble will start collapsed and expand when
     * clicked.
     */
    show: function() {
      if (!this.hidden)
        return;

      document.body.appendChild(this);
      this.hidden = false;
      this.resizeAndReposition();

      this.eventTracker_ = new EventTracker;
      this.eventTracker_.add(window,
                             'load', this.resizeAndReposition.bind(this));
      this.eventTracker_.add(window,
                             'resize', this.resizeAndReposition.bind(this));
      this.eventTracker_.add(this, 'click', this.onNotificationClick_);

      var doc = this.ownerDocument;
      this.eventTracker_.add(doc, 'keydown', this, true);
      this.eventTracker_.add(doc, 'mousedown', this, true);
    },

    /**
     * Hides the bubble from view.
     */
    hide: function() {
      this.hidden = true;
      this.bubbleSuppressed = false;
      this.eventTracker_.removeAll();
      this.parentNode.removeChild(this);
    },

    /**
     * Handles keydown and mousedown events, dismissing the bubble if
     * necessary.
     * @param {Event} e The event.
     * @private
     */
    handleEvent: function(e) {
      var handled = false;
      switch (e.type) {
        case 'keydown':
          if (e.keyCode == 27) {  // Esc.
            if (this.expanded) {
              this.collapseBubble_();
              handled = true;
            }
          }
          break;

        case 'mousedown':
          if (e.target == this.querySelector('.expandable-bubble-close')) {
            this.handleCloseEvent_();
            handled = true;
          } else if (!this.contains(e.target)) {
            if (this.expanded) {
              this.collapseBubble_();
              handled = true;
            }
          }
          break;
      }

      if (handled) {
        // The bubble emulates a focus grab when expanded, so when we've
        // collapsed/hide the bubble we consider the event handles and don't
        // need to propagate it further.
        e.stopPropagation();
        e.preventDefault();
      }
    },
  };

  /**
   * Whether the bubble is expanded or not.
   * @type {boolean}
   */
  cr.defineProperty(ExpandableBubble, 'expanded', cr.PropertyKind.BOOL_ATTR);

  return {
    ExpandableBubble: ExpandableBubble
  };
});
