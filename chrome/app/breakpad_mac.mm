// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/app/breakpad_mac.h"

#import <Foundation/Foundation.h>

#include "base/base_switches.h"
#import "base/basictypes.h"
#include "base/command_line.h"
#import "base/logging.h"
#import "base/scoped_nsautorelease_pool.h"
#include "base/sys_string_conversions.h"
#import "breakpad/src/client/mac/Framework/Breakpad.h"
#include "chrome/common/child_process_logging.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/google_update_settings.h"

namespace {

BreakpadRef gBreakpadRef = NULL;

}  // namespace

bool IsCrashReporterDisabled() {
  return gBreakpadRef == NULL;
}

void DestructCrashReporter() {
  if (gBreakpadRef) {
    BreakpadRelease(gBreakpadRef);
    gBreakpadRef = NULL;
  }
}

// Only called for a branded build of Chrome.app.
void InitCrashReporter() {
  DCHECK(gBreakpadRef == NULL);
  base::ScopedNSAutoreleasePool autorelease_pool;

  // Check whether the user has consented to stats and crash reporting.  The
  // browser process can make this determination directly.  Helper processes
  // may not have access to the disk or to the same data as the browser
  // process, so the browser passes the consent preference to them on the
  // command line.
  NSBundle* main_bundle = [NSBundle mainBundle];
  NSDictionary* info_dictionary = [main_bundle infoDictionary];
  bool is_browser = [[info_dictionary objectForKey:@"LSUIElement"]
                                   isEqualToString:@"1"] ? false : true;
  bool enable_breakpad =
      is_browser ? GoogleUpdateSettings::GetCollectStatsConsent() :
                   CommandLine::ForCurrentProcess()->
                       HasSwitch(switches::kEnableCrashReporter);

  if (!enable_breakpad) {
    LOG(WARNING) << "Breakpad disabled";
    return;
  }

  // Tell Breakpad where crash_inspector and crash_report_sender are.
  NSString* resource_path = [main_bundle resourcePath];
  NSString *inspector_location =
      [resource_path stringByAppendingPathComponent:@"crash_inspector"];
  NSString *reporter_bundle_location =
      [resource_path stringByAppendingPathComponent:@"crash_report_sender.app"];
  NSString *reporter_location =
      [[NSBundle bundleWithPath:reporter_bundle_location] executablePath];

  NSMutableDictionary *breakpad_config =
      [[info_dictionary mutableCopy] autorelease];
  [breakpad_config setObject:inspector_location
                      forKey:@BREAKPAD_INSPECTOR_LOCATION];
  [breakpad_config setObject:reporter_location
                      forKey:@BREAKPAD_REPORTER_EXE_LOCATION];

  // In the main application (the browser process), crashes can be passed to
  // the system's Crash Reporter.  This allows the system to notify the user
  // when the application crashes, and provide the user with the option to
  // restart it.
  if (is_browser)
    [breakpad_config setObject:@"NO" forKey:@BREAKPAD_SEND_AND_EXIT];

  // Initialize Breakpad.
  gBreakpadRef = BreakpadCreate(breakpad_config);
  if (!gBreakpadRef) {
    LOG(ERROR) << "Breakpad initializaiton failed";
    return;
  }

  // Set Breakpad metadata values.  These values are added to Info.plist during
  // the branded Google Chrome.app build.
  SetCrashKeyValue(@"ver", [info_dictionary objectForKey:@BREAKPAD_VERSION]);
  SetCrashKeyValue(@"prod", [info_dictionary objectForKey:@BREAKPAD_PRODUCT]);
  SetCrashKeyValue(@"plat", @"OS X");

  // Enable child process crashes to include the page URL.
  // TODO: Should this only be done for certain process types?
  child_process_logging::SetCrashKeyFunctions(SetCrashKeyValue,
                                              ClearCrashKeyValue);
}

void InitCrashProcessInfo() {
  if (gBreakpadRef == NULL) {
    return;
  }

  // Determine the process type.
  NSString* process_type = @"browser";
  std::wstring process_type_switch =
      CommandLine::ForCurrentProcess()->GetSwitchValue(switches::kProcessType);
  if (!process_type_switch.empty()) {
    process_type = base::SysWideToNSString(process_type_switch);
  }

  // Store process type in crash dump.
  SetCrashKeyValue(@"ptype", process_type);
}

void SetCrashKeyValue(NSString* key, NSString* value) {
  // Comment repeated from header to prevent confusion:
  // IMPORTANT: On OS X, the key/value pairs are sent to the crash server
  // out of bounds and not recorded on disk in the minidump, this means
  // that if you look at the minidump file locally you won't see them!
  if (gBreakpadRef == NULL) {
    return;
  }

  BreakpadAddUploadParameter(gBreakpadRef, key, value);
}

void ClearCrashKeyValue(NSString* key) {
  if (gBreakpadRef == NULL) {
    return;
  }

  BreakpadRemoveUploadParameter(gBreakpadRef, key);
}
