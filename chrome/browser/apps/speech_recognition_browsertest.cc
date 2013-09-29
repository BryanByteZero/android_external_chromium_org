// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_browsertest_util.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/fake_speech_recognition_manager.h"

class SpeechRecognitionTest : public extensions::PlatformAppBrowserTest {
 public:
   SpeechRecognitionTest() {}
   virtual ~SpeechRecognitionTest() {}

 protected:
  virtual void SetUp() OVERRIDE {
    const testing::TestInfo* const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    // For SpeechRecognitionTest.SpeechFromBackgroundPage test, we need to
    // fake the speech input to make tests run OK in bots.
    if (!strcmp(test_info->name(), "SpeechFromBackgroundPage")) {
      fake_speech_recognition_manager_.reset(
          new content::FakeSpeechRecognitionManager());
      fake_speech_recognition_manager_->set_should_send_fake_response(true);
      // Inject the fake manager factory so that the test result is returned to
      // the web page.
      content::SpeechRecognitionManager::SetManagerForTests(
          fake_speech_recognition_manager_.get());
    }

    extensions::PlatformAppBrowserTest::SetUp();
  }

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    command_line->AppendSwitch(switches::kUseFakeDeviceForMediaStream);
    command_line->AppendSwitch(switches::kUseFakeUIForMediaStream);
    extensions::PlatformAppBrowserTest::SetUpCommandLine(command_line);
  }

 private:
  scoped_ptr<content::FakeSpeechRecognitionManager>
      fake_speech_recognition_manager_;

  DISALLOW_COPY_AND_ASSIGN(SpeechRecognitionTest);
};

IN_PROC_BROWSER_TEST_F(SpeechRecognitionTest, SpeechFromBackgroundPage) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/speech/background_page"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(SpeechRecognitionTest,
                       SpeechFromBackgroundPageWithoutPermission) {
  ASSERT_TRUE(
      RunPlatformAppTest("platform_apps/speech/background_page_no_permission"))
          << message_;
}
