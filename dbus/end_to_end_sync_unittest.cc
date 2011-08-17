// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "dbus/test_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// The end-to-end test exercises the synchronos APIs in ObjectProxy and
// ExportedObject. The test will launch a thread for the service side
// operations (i.e. ExportedObject side).
class EndToEndSyncTest : public testing::Test {
 public:
  EndToEndSyncTest() {
  }

  void SetUp() {
    // Start the test service;
    test_service_.reset(new dbus::TestService);
    test_service_->StartService();
    test_service_->WaitUntilServiceIsStarted();

    // Create the client.
    dbus::Bus::Options client_bus_options;
    client_bus_options.bus_type = dbus::Bus::SESSION;
    client_bus_options.connection_type = dbus::Bus::PRIVATE;
    client_bus_ = new dbus::Bus(client_bus_options);
    object_proxy_ = client_bus_->GetObjectProxy("org.chromium.TestService",
                                                "/org/chromium/TestObject");
  }

  void TearDown() {
    test_service_->Stop();
    client_bus_->ShutdownAndBlock();
  }

 protected:
  scoped_ptr<dbus::TestService> test_service_;
  scoped_refptr<dbus::Bus> client_bus_;
  dbus::ObjectProxy* object_proxy_;
};

TEST_F(EndToEndSyncTest, Echo) {
  const std::string kHello = "hello";

  // Create the method call.
  dbus::MethodCall method_call("org.chromium.TestInterface", "Echo");
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kHello);

  // Call the method.
  dbus::Response response;
  const int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT;
  const bool success =
      object_proxy_->CallMethodAndBlock(&method_call, timeout_ms, &response);
  ASSERT_TRUE(success);

  // Check the response. kHello should be echoed back.
  dbus::MessageReader reader(&response);
  std::string returned_message;
  ASSERT_TRUE(reader.PopString(&returned_message));
  EXPECT_EQ(kHello, returned_message);
}

TEST_F(EndToEndSyncTest, Timeout) {
  const std::string kHello = "hello";

  // Create the method call.
  dbus::MethodCall method_call("org.chromium.TestInterface", "DelayedEcho");
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kHello);

  // Call the method with timeout smaller than TestService::kSlowEchoSleepMs.
  dbus::Response response;
  const int timeout_ms = dbus::TestService::kSlowEchoSleepMs / 10;
  const bool success =
      object_proxy_->CallMethodAndBlock(&method_call, timeout_ms, &response);
  // Should fail because of timeout.
  ASSERT_FALSE(success);
}

TEST_F(EndToEndSyncTest, NonexistentMethod) {
  dbus::MethodCall method_call("org.chromium.TestInterface", "Nonexistent");

  dbus::Response response;
  const int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT;
  const bool success =
      object_proxy_->CallMethodAndBlock(&method_call, timeout_ms, &response);
  ASSERT_FALSE(success);
}

TEST_F(EndToEndSyncTest, BrokenMethod) {
  dbus::MethodCall method_call("org.chromium.TestInterface", "BrokenMethod");

  dbus::Response response;
  const int timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT;
  const bool success =
      object_proxy_->CallMethodAndBlock(&method_call, timeout_ms, &response);
  ASSERT_FALSE(success);
}
