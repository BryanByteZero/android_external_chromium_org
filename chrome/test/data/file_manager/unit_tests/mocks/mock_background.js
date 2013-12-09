// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Mock class for Background.
 * @constructor
 */
function MockBackground() {
  this.fileOperationManager = new MockFileOperationManager();
  this.progressCenter = new MockProgressCenter();
  this.closeRequestCount = 0;
}

/**
 * Increments the close request counter.
 */
MockBackground.prototype.tryClose = function() {
  this.closeRequestCount++;
};
