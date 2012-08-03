// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/test/test_timeouts.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_result_codes.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/common/result_codes.h"

// These tests don't apply to the Mac version; see GetCommandLineForRelaunch
// for details.
#if defined(OS_MACOSX)
#error This test file should not be part of the Mac build.
#endif

namespace {

class CloudPrintPolicyTest : public InProcessBrowserTest {
 public:
  CloudPrintPolicyTest() {}
};

IN_PROC_BROWSER_TEST_F(CloudPrintPolicyTest, NormalPassedFlag) {
  FilePath test_file_path = ui_test_utils::GetTestFilePath(
      FilePath(), FilePath().AppendASCII("empty.html"));
  CommandLine new_command_line(GetCommandLineForRelaunch());
  new_command_line.AppendArgPath(test_file_path);

  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_TAB_ADDED,
      content::NotificationService::AllSources());

  base::ProcessHandle handle;
  bool launched =
      base::LaunchProcess(new_command_line, base::LaunchOptions(), &handle);
  EXPECT_TRUE(launched);

  observer.Wait();

  int exit_code = -100;
  bool exited =
      base::WaitForExitCodeWithTimeout(handle, &exit_code,
                                       TestTimeouts::action_timeout());

  EXPECT_TRUE(exited);
  EXPECT_EQ(chrome::RESULT_CODE_NORMAL_EXIT_PROCESS_NOTIFIED, exit_code);
  base::CloseProcessHandle(handle);
}

IN_PROC_BROWSER_TEST_F(CloudPrintPolicyTest, CloudPrintPolicyFlag) {
  CommandLine new_command_line(GetCommandLineForRelaunch());
  new_command_line.AppendSwitch(switches::kCheckCloudPrintConnectorPolicy);
  new_command_line.AppendSwitchASCII(
      switches::kSpeculativeResourcePrefetching,
      switches::kSpeculativeResourcePrefetchingDisabled);

  base::ProcessHandle handle;
  bool launched =
      base::LaunchProcess(new_command_line, base::LaunchOptions(), &handle);
  EXPECT_TRUE(launched);

  int exit_code = -100;
  bool exited =
      base::WaitForExitCodeWithTimeout(handle, &exit_code,
                                       TestTimeouts::action_timeout());

  EXPECT_TRUE(exited);
  EXPECT_EQ(content::RESULT_CODE_NORMAL_EXIT, exit_code);
  base::CloseProcessHandle(handle);
}

}  // namespace
