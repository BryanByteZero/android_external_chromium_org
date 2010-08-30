// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_event_names.h"

namespace extension_event_names {

const char kOnTabAttached[] = "tabs.onAttached";
const char kOnTabCreated[] = "tabs.onCreated";
const char kOnTabDetached[] = "tabs.onDetached";
const char kOnTabMoved[] = "tabs.onMoved";
const char kOnTabRemoved[] = "tabs.onRemoved";
const char kOnTabSelectionChanged[] = "tabs.onSelectionChanged";
const char kOnTabUpdated[] = "tabs.onUpdated";

const char kOnWindowCreated[] = "windows.onCreated";
const char kOnWindowFocusedChanged[] = "windows.onFocusChanged";
const char kOnWindowRemoved[] = "windows.onRemoved";

const char kOnExtensionInstalled[] = "experimental.management.onInstalled";
const char kOnExtensionUninstalled[] = "experimental.management.onUninstalled";
const char kOnExtensionEnabled[] = "experimental.management.onEnabled";
const char kOnExtensionDisabled[] = "experimental.management.onDisabled";

}  // namespace extension_event_names
