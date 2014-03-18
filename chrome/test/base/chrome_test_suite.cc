// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/chrome_test_suite.h"

#if defined(OS_CHROMEOS)
#include <stdio.h>
#include <unistd.h>
#endif

#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/url_constants.h"
#include "content/public/test/test_launcher.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "chrome/browser/android/chrome_jni_registrar.h"
#include "net/android/net_jni_registrar.h"
#include "ui/base/android/ui_base_jni_registrar.h"
#include "ui/gfx/android/gfx_jni_registrar.h"
#include "ui/gl/android/gl_jni_registrar.h"
#endif

#if defined(OS_CHROMEOS)
#include "base/process/process_metrics.h"
#include "chromeos/chromeos_paths.h"
#endif

#if defined(OS_MACOSX)
#include "base/mac/bundle_locations.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#if !defined(OS_IOS)
#include "base/mac/mac_util.h"
#include "chrome/browser/chrome_browser_application_mac.h"
#endif  // !defined(OS_IOS)
#endif

#if !defined(OS_IOS)
#include "media/base/media.h"
#endif

namespace {

bool IsCrosPythonProcess() {
#if defined(OS_CHROMEOS)
  char buf[80];
  int num_read = readlink(base::kProcSelfExe, buf, sizeof(buf) - 1);
  if (num_read == -1)
    return false;
  buf[num_read] = 0;
  const char kPythonPrefix[] = "/python";
  return !strncmp(strrchr(buf, '/'), kPythonPrefix, sizeof(kPythonPrefix) - 1);
#else
  return false;
#endif  // defined(OS_CHROMEOS)
}

}  // namespace

ChromeTestSuite::ChromeTestSuite(int argc, char** argv)
    : content::ContentTestSuiteBase(argc, argv) {
}

ChromeTestSuite::~ChromeTestSuite() {
}

void ChromeTestSuite::Initialize() {
#if defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool autorelease_pool;
#if !defined(OS_IOS)
  chrome_browser_application_mac::RegisterBrowserCrApp();
#endif  // !defined(OS_IOS)
#endif

#if defined(OS_ANDROID)
  // Register JNI bindings for android.
  gfx::android::RegisterJni(base::android::AttachCurrentThread());
  net::android::RegisterJni(base::android::AttachCurrentThread());
  ui::android::RegisterJni(base::android::AttachCurrentThread());
  ui::gl::android::RegisterJni(base::android::AttachCurrentThread());
  chrome::android::RegisterJni(base::android::AttachCurrentThread());
#endif

  if (!browser_dir_.empty()) {
    PathService::Override(base::DIR_EXE, browser_dir_);
    PathService::Override(base::DIR_MODULE, browser_dir_);
  }

#if !defined(OS_IOS)
  // Disable external libraries load if we are under python process in
  // ChromeOS.  That means we are autotest and, if ASAN is used,
  // external libraries load crashes.
  if (!IsCrosPythonProcess())
    media::InitializeMediaLibraryForTesting();
#endif

  // Initialize after overriding paths as some content paths depend on correct
  // values for DIR_EXE and DIR_MODULE.
  content::ContentTestSuiteBase::Initialize();

#if defined(OS_MACOSX) && !defined(OS_IOS)
  // Look in the framework bundle for resources.
  base::FilePath path;
  PathService::Get(base::DIR_EXE, &path);
  path = path.Append(chrome::kFrameworkName);
  base::mac::SetOverrideFrameworkBundlePath(path);
#endif
}

void ChromeTestSuite::Shutdown() {
#if defined(OS_MACOSX) && !defined(OS_IOS)
  base::mac::SetOverrideFrameworkBundle(NULL);
#endif

  content::ContentTestSuiteBase::Shutdown();
}
