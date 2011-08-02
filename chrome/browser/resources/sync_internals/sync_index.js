// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Allow platform specific CSS rules.
//
// TODO(akalin): BMM and options page does something similar, too.
// Move this to util.js.
if (cr.isWindows)
  document.documentElement.setAttribute('os', 'win');

cr.ui.decorate('tabbox', cr.ui.TabBox);
