// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  /**
   * Fit-to-page ticket item whose value is a {@code boolean} that indicates
   * whether to scale the document to fit the page.
   * @param {!print_preview.DocumentInfo} documentInfo Information about the
   *     document to print.
   * @param {!print_preview.DestinationStore} destinationStore Used to determine
   *     whether fit to page should be available.
   * @constructor
   * @extends {print_preview.ticket_items.TicketItem}
   */
  function FitToPage(documentInfo, destinationStore) {
    print_preview.ticket_items.TicketItem.call(this);

    /**
     * Information about the document to print.
     * @type {!print_preview.DocumentInfo}
     * @private
     */
    this.documentInfo_ = documentInfo;

    /**
     * Used to determine whether fit to page should be available.
     * @type {!print_preview.DestinationStore}
     * @private
     */
    this.destinationStore_ = destinationStore;
  };

  FitToPage.prototype = {
    __proto__: print_preview.ticket_items.TicketItem.prototype,

    /** @override */
    wouldValueBeValid: function(value) {
      return true;
    },

    /** @override */
    isCapabilityAvailable: function() {
      return !this.documentInfo_.isModifiable &&
          (!this.destinationStore_.selectedDestination ||
              this.destinationStore_.selectedDestination.id !=
                  print_preview.Destination.GooglePromotedId.SAVE_AS_PDF);
    },

    /** @override */
    getDefaultValueInternal: function() {
      return true;
    },

    /** @override */
    getCapabilityNotAvailableValueInternal: function() {
      return this.destinationStore_.selectedDestination &&
          this.destinationStore_.selectedDestination.id ==
              print_preview.Destination.GooglePromotedId.SAVE_AS_PDF;
    }
  };

  // Export
  return {
    FitToPage: FitToPage
  };
});
