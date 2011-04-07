// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview TimelineView visualizes GPU_TRACE events using the
 * gpu.Timeline component.
 */
cr.define('gpu', function() {
  function tsRound(ts) {
    return Math.round(ts * 1000.0) / 1000.0;
  }
  function getPadding(text, width) {
    width = width || 0;

    if (typeof text != 'string')
      text = String(text);

    if (text.length >= width)
      return '';

    var pad = '';
    for (var i = 0; i < width - text.length; i++)
      pad += ' ';
    return pad;
  }

  function leftAlign(text, width) {
    return text + getPadding(text, width);
  }

  function rightAlign(text, width) {
    return getPadding(text, width) + text;
  }

  /**
   * TimelineView
   * @constructor
   * @extends {gpu.Tab}
   */
  TimelineView = cr.ui.define(gpu.Tab);

  TimelineView.prototype = {
    __proto__: gpu.Tab.prototype,

    decorate: function() {
      tracingController.addEventListener('traceBegun',
                                         this.setNeedsRefresh_.bind(this));
      tracingController.addEventListener('traceEnded',
                                         this.setNeedsRefresh_.bind(this));
      this.addEventListener('selectedChange', this.onViewSelectedChange_);

      this.setNeedsRefresh_();
    },

    onViewSelectedChange_: function() {
      if (this.selected) {
        if (!tracingController.traceEvents.length) {
          tracingController.beginTracing();
        }
        if (this.needsRefreshOnShow_) {
          this.refresh();
        }
      }
    },

    setNeedsRefresh_: function() {
      if (!this.selected) {
        this.needsRefreshOnShow_ = true;
        return;
      } else {
        this.refresh();
      }
    },

    /**
     * Updates the view based on its currently known data
     */
    refresh: function() {
      this.needsRefreshOnShow_ = false;

      console.log('TimelineView.refresh');
      var events = tracingController.traceEvents;
      this.timelineModel_ = new gpu.TimelineModel(events);

      // remove old timeline
      var timelineContainer = $('timeline-view-timeline-container');
      timelineContainer.textContent = '';

      // create new timeline if needed
      if (events.length) {
        this.timeline_ = new gpu.Timeline();
        this.timeline_.model = this.timelineModel_;
        timelineContainer.appendChild(this.timeline_);
        this.timeline_.onResize();
        this.timeline_.addEventListener('selectionChange',
                                        this.onSelectionChanged_.bind(this));
        this.onSelectionChanged_();
      } else {
        this.timeline_ = null;
      }
    },

    onSelectionChanged_: function(e) {
      console.log('selection changed');
      var timeline = this.timeline_;
      var selection = timeline.selection;
      if (!selection.length) {
        var outputDiv = $('timeline-selection-summary');
        outputDiv.textContent = timeline.keyHelp;
        return;
      }

      var text = '';
      if (selection.length == 1) {
        var c0Width = 10;
        var slice = selection[0].slice;
        text = 'Selected item:\n';
        text += leftAlign('Title', c0Width) + ': ' + slice.title + '\n';
        text += leftAlign('Start', c0Width) + ': ' +
            tsRound(slice.start) + ' ms\n';
        text += leftAlign('Duration', c0Width) + ': ' +
            tsRound(slice.duration) + ' ms\n';

        var n = 0;
        for (var argName in slice.args) {
          n += 1;
        }
        if (n > 0) {
          text += leftAlign('Args', c0Width) + ':\n';
          for (var argName in slice.args) {
            var argVal = slice.args[argName];
            text += leftAlign(' ' + argName, c0Width) + ': ' + argVal + '\n';
          }
        }
      } else {
        var c0Width = 55;
        var c1Width = 12;
        var c2Width = 5;
        text = 'Selection summary:\n';
        var tsLo = Math.min.apply(Math, selection.map(
            function(s) {return s.slice.start;}));
        var tsHi = Math.max.apply(Math, selection.map(
            function(s) {return s.slice.end;}));

        // compute total selection duration
        var titles = selection.map(function(i) { return i.slice.title; });

        var slicesByTitle = {};
        for (var i = 0; i < selection.length; i++) {
          var slice = selection[i].slice;
          if (!slicesByTitle[slice.title])
            slicesByTitle[slice.title] = {
              slices: []
            };
          slicesByTitle[slice.title].slices.push(slice);
        }
        var totalDuration = 0;
        for (var sliceGroupTitle in slicesByTitle) {
          var sliceGroup = slicesByTitle[sliceGroupTitle];
          var duration = 0;
          for (i = 0; i < sliceGroup.slices.length; i++)
            duration += sliceGroup.slices[i].duration;
          totalDuration += duration;

          text += ' ' +
              leftAlign(sliceGroupTitle, c0Width) + ': ' +
              rightAlign(tsRound(duration) + 'ms', c1Width) + '   ' +
              rightAlign(String(sliceGroup.slices.length), c2Width) +
              ' occurrences' + '\n';
        }

        text += leftAlign('*Totals', c0Width) + ' : ' +
            rightAlign(tsRound(totalDuration) + 'ms', c1Width) + '   ' +
            rightAlign(String(selection.length), c2Width) + ' occurrences' +
            '\n';

        text += '\n';

        text += leftAlign('Selection start', c0Width) + ' : ' +
            rightAlign(tsRound(tsLo) + 'ms', c1Width) +
            '\n';
        text += leftAlign('Selection extent', c0Width) + ' : ' +
            rightAlign(tsRound(tsHi - tsLo) + 'ms', c1Width) +
            '\n';
      }

      // done
      var outputDiv = $('timeline-selection-summary');
      outputDiv.innerHTML = text;
    }
  };

  return {
    TimelineView: TimelineView
  };
});
