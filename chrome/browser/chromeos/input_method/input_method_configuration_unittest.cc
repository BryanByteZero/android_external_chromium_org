// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/input_method_configuration.h"
#include "chrome/browser/chromeos/input_method/mock_input_method_manager.h"
#include "content/public/browser/browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace input_method {

TEST(InputMethodConfigurationTest, TestInitialize) {
  InputMethodManager* manager = InputMethodManager::Get();
  EXPECT_FALSE(manager);

  Initialize(
      content::BrowserThread::GetMessageLoopProxyForThread(
          content::BrowserThread::UI),
      content::BrowserThread::GetMessageLoopProxyForThread(
          content::BrowserThread::FILE));
  manager = InputMethodManager::Get();
  EXPECT_TRUE(manager);
  Shutdown();
}

TEST(InputMethodConfigurationTest, TestInitializeForTesting) {
  InputMethodManager* manager = InputMethodManager::Get();
  EXPECT_FALSE(manager);

  InitializeForTesting(new MockInputMethodManager);
  manager = InputMethodManager::Get();
  EXPECT_TRUE(manager);
  Shutdown();
}

}  // namespace input_method
}  // namespace chromeos
