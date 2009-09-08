// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_PLUGIN_CONSTANTS_WIN_H_
#define WEBKIT_GLUE_PLUGIN_CONSTANTS_WIN_H_

// Used by the plugins_test when testing the older WMP plugin to force the new
// plugin to not get loaded.
#define kUseOldWMPPluginSwitch L"use-old-wmp"

// The window class name for a plugin window.
#define kNativeWindowClassName L"NativeWindowClass"

// The name of the window class name for the wrapper HWND around the actual
// plugin window that's used when running in multi-process mode.  This window
// is created on the browser UI thread.
#define kWrapperNativeWindowClassName L"WrapperNativeWindowClass"

// The name of the custom window message that the browser uses to tell the
// plugin process to paint a window.
#define kPaintMessageName L"Chrome_CustomPaint"

// The name of the registry key which NPAPI plugins update on installation.
#define kRegistryMozillaPlugins L"SOFTWARE\\MozillaPlugins"

#endif  // WEBKIT_GLUE_PLUGIN_PLUGIN_LIST_H_
