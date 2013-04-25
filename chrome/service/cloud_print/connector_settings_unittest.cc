// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/cloud_print/connector_settings.h"

#include <string>

#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/values.h"
#include "chrome/common/cloud_print/cloud_print_constants.h"
#include "chrome/service/service_process_prefs.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cloud_print {

const char kServiceStateContent[] =
    "{"
    "   'cloud_print': {"
    "      'auth_token': 'token',"
    "      'email': 'user@gmail.com',"
    "      'enabled': true,"
    "      'proxy_id': 'PROXY',"
    "      'robot_email': '123@cloudprint.googleusercontent.com',"
    "      'robot_refresh_token': '123',"
    "      'service_url': 'http://cp.google.com',"
    "      'xmpp_auth_token': 'xmp token',"
    "      'xmpp_ping_enabled': true,"
    "      'xmpp_ping_timeout_sec': 256,"
    "      'user_settings': {"
    "        'printers': ["
    "          { 'name': 'prn1', 'connect': false },"
    "          { 'name': 'prn2', 'connect': false },"
    "          { 'name': 'prn3', 'connect': true }"
    "        ],"
    "        'connectNewPrinters': false"
    "      },"
    "      'print_system_settings': {"
    "         'delete_on_enum_fail' : true"
    "      }"
    "   }"
    "}";


class ConnectorSettingsTest : public testing::Test {
 protected:
  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    message_loop_proxy_ = base::MessageLoopProxy::current();
  }

  ServiceProcessPrefs* CreateTestFile(const char* json) {
    base::FilePath file_name = temp_dir_.path().AppendASCII("file.txt");
    file_util::Delete(file_name, false);
    if (json) {
      std::string content = json;
      std::replace(content.begin(), content.end(), '\'', '"');
      file_util::WriteFile(file_name, content.c_str(), content.size());
    }
    ServiceProcessPrefs* prefs =
        new ServiceProcessPrefs(file_name, message_loop_proxy_);
    prefs->ReadPrefs();
    return prefs;
  }

  base::ScopedTempDir temp_dir_;
  MessageLoop message_loop_;
  scoped_refptr<base::MessageLoopProxy> message_loop_proxy_;
};

TEST_F(ConnectorSettingsTest, InitFromEmpty) {
  const char* const kEmptyJSons[] = {
    NULL,
    "{}",
    "{'foo': []}",
    "{'foo',,}",
  };
  for (size_t i = 0; i < arraysize(kEmptyJSons); ++i) {
    scoped_ptr<ServiceProcessPrefs> prefs(CreateTestFile(kEmptyJSons[i]));
    ConnectorSettings settings;
    settings.InitFrom(prefs.get());

    EXPECT_EQ("https://www.google.com/cloudprint",
              settings.server_url().spec());
    EXPECT_FALSE(settings.proxy_id().empty());
    EXPECT_FALSE(settings.delete_on_enum_fail());
    EXPECT_EQ(NULL, settings.print_system_settings());
    EXPECT_TRUE(settings.ShouldConnect("prn1"));
    EXPECT_FALSE(settings.xmpp_ping_enabled());
  }
}

TEST_F(ConnectorSettingsTest, InitFromFile) {
  scoped_ptr<ServiceProcessPrefs> prefs(CreateTestFile(kServiceStateContent));
  ConnectorSettings settings;
  settings.InitFrom(prefs.get());
  EXPECT_EQ("http://cp.google.com/", settings.server_url().spec());
  EXPECT_EQ("PROXY", settings.proxy_id());
  EXPECT_FALSE(settings.proxy_id().empty());
  EXPECT_TRUE(settings.delete_on_enum_fail());
  EXPECT_TRUE(settings.print_system_settings());
  EXPECT_TRUE(settings.xmpp_ping_enabled());
  EXPECT_EQ(settings.xmpp_ping_timeout_sec(), 256);
  EXPECT_FALSE(settings.ShouldConnect("prn0"));
  EXPECT_FALSE(settings.ShouldConnect("prn1"));
  EXPECT_TRUE(settings.ShouldConnect("prn3"));
}

TEST_F(ConnectorSettingsTest, CopyFrom) {
  scoped_ptr<ServiceProcessPrefs> prefs(CreateTestFile(kServiceStateContent));
  ConnectorSettings settings1;
  settings1.InitFrom(prefs.get());

  ConnectorSettings settings2;
  settings2.CopyFrom(settings1);

  EXPECT_EQ(settings1.server_url(), settings2.server_url());
  EXPECT_EQ(settings1.proxy_id(), settings2.proxy_id());
  EXPECT_EQ(settings1.delete_on_enum_fail(), settings2.delete_on_enum_fail());
  EXPECT_EQ(settings1.print_system_settings()->size(),
            settings2.print_system_settings()->size());
  EXPECT_EQ(settings1.xmpp_ping_enabled(), settings2.xmpp_ping_enabled());
  EXPECT_EQ(settings1.xmpp_ping_timeout_sec(),
            settings2.xmpp_ping_timeout_sec());
  EXPECT_FALSE(settings2.ShouldConnect("prn0"));
  EXPECT_FALSE(settings2.ShouldConnect("prn1"));
  EXPECT_TRUE(settings2.ShouldConnect("prn3"));
}

TEST_F(ConnectorSettingsTest, SettersTest) {
  scoped_ptr<ServiceProcessPrefs> prefs(CreateTestFile("{}"));
  ConnectorSettings settings;
  settings.InitFrom(prefs.get());
  EXPECT_FALSE(settings.xmpp_ping_enabled());

  // Set and check valid settings.
  settings.set_xmpp_ping_enabled(true);
  settings.SetXmppPingTimeoutSec(256);
  EXPECT_TRUE(settings.xmpp_ping_enabled());
  EXPECT_EQ(settings.xmpp_ping_timeout_sec(), 256);

  // Set invalid settings, and check correct defaults.
  settings.SetXmppPingTimeoutSec(1);
  EXPECT_EQ(settings.xmpp_ping_timeout_sec(), kMinimumXmppPingTimeoutSecs);
}

}  // namespace cloud_print
