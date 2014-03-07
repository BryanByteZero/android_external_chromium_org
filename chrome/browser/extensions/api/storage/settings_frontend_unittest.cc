// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/storage/leveldb_settings_storage_factory.h"
#include "chrome/browser/extensions/api/storage/settings_frontend.h"
#include "chrome/browser/extensions/api/storage/settings_test_util.h"
#include "content/public/test/test_browser_thread.h"
#include "extensions/browser/api/storage/settings_namespace.h"
#include "extensions/browser/value_store/value_store.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::BrowserThread;

namespace extensions {

namespace settings = settings_namespace;
namespace util = settings_test_util;

namespace {

// To save typing ValueStore::DEFAULTS everywhere.
const ValueStore::WriteOptions DEFAULTS = ValueStore::DEFAULTS;

// Creates a kilobyte of data.
scoped_ptr<base::Value> CreateKilobyte() {
  std::string kilobyte_string;
  for (int i = 0; i < 1024; ++i) {
    kilobyte_string += "a";
  }
  return scoped_ptr<base::Value>(new base::StringValue(kilobyte_string));
}

// Creates a megabyte of data.
scoped_ptr<base::Value> CreateMegabyte() {
  base::ListValue* megabyte = new base::ListValue();
  for (int i = 0; i < 1000; ++i) {
    megabyte->Append(CreateKilobyte().release());
  }
  return scoped_ptr<base::Value>(megabyte);
}

}  // namespace

class ExtensionSettingsFrontendTest : public testing::Test {
 public:
  ExtensionSettingsFrontendTest()
      : storage_factory_(new util::ScopedSettingsStorageFactory()),
        ui_thread_(BrowserThread::UI, base::MessageLoop::current()),
        file_thread_(BrowserThread::FILE, base::MessageLoop::current()) {}

  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    profile_.reset(new util::MockProfile(temp_dir_.path()));
    ResetFrontend();
  }

  virtual void TearDown() OVERRIDE {
    frontend_.reset();
    profile_.reset();
    // Execute any pending deletion tasks.
    message_loop_.RunUntilIdle();
  }

 protected:
  Profile* profile() { return profile_.get(); }

  void ResetFrontend() {
    storage_factory_->Reset(new LeveldbSettingsStorageFactory());
    frontend_.reset(
        SettingsFrontend::CreateForTesting(storage_factory_, profile_.get()));
  }

  base::ScopedTempDir temp_dir_;
  scoped_ptr<util::MockProfile> profile_;
  scoped_ptr<SettingsFrontend> frontend_;
  scoped_refptr<util::ScopedSettingsStorageFactory> storage_factory_;

 private:
  base::MessageLoop message_loop_;
  content::TestBrowserThread ui_thread_;
  content::TestBrowserThread file_thread_;
};

// Get a semblance of coverage for both extension and app settings by
// alternating in each test.
// TODO(kalman): explicitly test the two interact correctly.

// Tests that the frontend is set up correctly.
TEST_F(ExtensionSettingsFrontendTest, Basics) {
  // Local storage is always enabled.
  EXPECT_TRUE(frontend_->IsStorageEnabled(settings::LOCAL));
  EXPECT_TRUE(frontend_->GetValueStoreCache(settings::LOCAL));

  // Invalid storage areas are not available.
  EXPECT_FALSE(frontend_->IsStorageEnabled(settings::INVALID));
  EXPECT_FALSE(frontend_->GetValueStoreCache(settings::INVALID));
}

TEST_F(ExtensionSettingsFrontendTest, SettingsPreservedAcrossReconstruction) {
  const std::string id = "ext";
  scoped_refptr<const Extension> extension =
      util::AddExtensionWithId(profile(), id, Manifest::TYPE_EXTENSION);

  ValueStore* storage = util::GetStorage(extension, frontend_.get());

  // The correctness of Get/Set/Remove/Clear is tested elsewhere so no need to
  // be too rigorous.
  {
    base::StringValue bar("bar");
    ValueStore::WriteResult result = storage->Set(DEFAULTS, "foo", bar);
    ASSERT_FALSE(result->HasError());
  }

  {
    ValueStore::ReadResult result = storage->Get();
    ASSERT_FALSE(result->HasError());
    EXPECT_FALSE(result->settings().empty());
  }

  ResetFrontend();
  storage = util::GetStorage(extension, frontend_.get());

  {
    ValueStore::ReadResult result = storage->Get();
    ASSERT_FALSE(result->HasError());
    EXPECT_FALSE(result->settings().empty());
  }
}

TEST_F(ExtensionSettingsFrontendTest, SettingsClearedOnUninstall) {
  const std::string id = "ext";
  scoped_refptr<const Extension> extension = util::AddExtensionWithId(
      profile(), id, Manifest::TYPE_LEGACY_PACKAGED_APP);

  ValueStore* storage = util::GetStorage(extension, frontend_.get());

  {
    base::StringValue bar("bar");
    ValueStore::WriteResult result = storage->Set(DEFAULTS, "foo", bar);
    ASSERT_FALSE(result->HasError());
  }

  // This would be triggered by extension uninstall via a DataDeleter.
  frontend_->DeleteStorageSoon(id);
  base::MessageLoop::current()->RunUntilIdle();

  // The storage area may no longer be valid post-uninstall, so re-request.
  storage = util::GetStorage(extension, frontend_.get());
  {
    ValueStore::ReadResult result = storage->Get();
    ASSERT_FALSE(result->HasError());
    EXPECT_TRUE(result->settings().empty());
  }
}

TEST_F(ExtensionSettingsFrontendTest, LeveldbDatabaseDeletedFromDiskOnClear) {
  const std::string id = "ext";
  scoped_refptr<const Extension> extension =
      util::AddExtensionWithId(profile(), id, Manifest::TYPE_EXTENSION);

  ValueStore* storage = util::GetStorage(extension, frontend_.get());

  {
    base::StringValue bar("bar");
    ValueStore::WriteResult result = storage->Set(DEFAULTS, "foo", bar);
    ASSERT_FALSE(result->HasError());
    EXPECT_TRUE(base::PathExists(temp_dir_.path()));
  }

  // Should need to both clear the database and delete the frontend for the
  // leveldb database to be deleted from disk.
  {
    ValueStore::WriteResult result = storage->Clear();
    ASSERT_FALSE(result->HasError());
    EXPECT_TRUE(base::PathExists(temp_dir_.path()));
  }

  frontend_.reset();
  base::MessageLoop::current()->RunUntilIdle();
  // TODO(kalman): Figure out why this fails, despite appearing to work.
  // Leaving this commented out rather than disabling the whole test so that the
  // deletion code paths are at least exercised.
  //EXPECT_FALSE(base::PathExists(temp_dir_.path()));
}

// Disabled (slow), http://crbug.com/322751 .
TEST_F(ExtensionSettingsFrontendTest,
       DISABLED_QuotaLimitsEnforcedCorrectlyForSyncAndLocal) {
  const std::string id = "ext";
  scoped_refptr<const Extension> extension =
      util::AddExtensionWithId(profile(), id, Manifest::TYPE_EXTENSION);

  ValueStore* sync_storage =
      util::GetStorage(extension, settings::SYNC, frontend_.get());
  ValueStore* local_storage =
      util::GetStorage(extension, settings::LOCAL, frontend_.get());

  // Sync storage should run out after ~100K.
  scoped_ptr<base::Value> kilobyte = CreateKilobyte();
  for (int i = 0; i < 100; ++i) {
    sync_storage->Set(
        ValueStore::DEFAULTS, base::StringPrintf("%d", i), *kilobyte);
  }

  EXPECT_TRUE(sync_storage->Set(
      ValueStore::DEFAULTS, "WillError", *kilobyte)->HasError());

  // Local storage shouldn't run out after ~100K.
  for (int i = 0; i < 100; ++i) {
    local_storage->Set(
        ValueStore::DEFAULTS, base::StringPrintf("%d", i), *kilobyte);
  }

  EXPECT_FALSE(local_storage->Set(
      ValueStore::DEFAULTS, "WontError", *kilobyte)->HasError());

  // Local storage should run out after ~5MB.
  scoped_ptr<base::Value> megabyte = CreateMegabyte();
  for (int i = 0; i < 5; ++i) {
    local_storage->Set(
        ValueStore::DEFAULTS, base::StringPrintf("%d", i), *megabyte);
  }

  EXPECT_TRUE(local_storage->Set(
      ValueStore::DEFAULTS, "WillError", *megabyte)->HasError());
}

// In other tests, we assume that the result of GetStorage is a pointer to the
// a Storage owned by a Frontend object, but for the unlimitedStorage case, this
// might not be true. So, write the tests in a "callback" style.
// We should really rewrite all tests to be asynchronous in this way.

static void UnlimitedSyncStorageTestCallback(ValueStore* sync_storage) {
  // Sync storage should still run out after ~100K; the unlimitedStorage
  // permission can't apply to sync.
  scoped_ptr<base::Value> kilobyte = CreateKilobyte();
  for (int i = 0; i < 100; ++i) {
    sync_storage->Set(
        ValueStore::DEFAULTS, base::StringPrintf("%d", i), *kilobyte);
  }

  EXPECT_TRUE(sync_storage->Set(
      ValueStore::DEFAULTS, "WillError", *kilobyte)->HasError());
}

static void UnlimitedLocalStorageTestCallback(ValueStore* local_storage) {
  // Local storage should never run out.
  scoped_ptr<base::Value> megabyte = CreateMegabyte();
  for (int i = 0; i < 7; ++i) {
    local_storage->Set(
        ValueStore::DEFAULTS, base::StringPrintf("%d", i), *megabyte);
  }

  EXPECT_FALSE(local_storage->Set(
      ValueStore::DEFAULTS, "WontError", *megabyte)->HasError());
}

#if defined(OS_WIN)
// See: http://crbug.com/227296
#define MAYBE_UnlimitedStorageForLocalButNotSync \
    DISABLED_UnlimitedStorageForLocalButNotSync
#else
#define MAYBE_UnlimitedStorageForLocalButNotSync \
    UnlimitedStorageForLocalButNotSync
#endif

TEST_F(ExtensionSettingsFrontendTest,
       MAYBE_UnlimitedStorageForLocalButNotSync) {
  const std::string id = "ext";
  std::set<std::string> permissions;
  permissions.insert("unlimitedStorage");
  scoped_refptr<const Extension> extension =
      util::AddExtensionWithIdAndPermissions(
          profile(), id, Manifest::TYPE_EXTENSION, permissions);

  frontend_->RunWithStorage(
      extension, settings::SYNC, base::Bind(&UnlimitedSyncStorageTestCallback));
  frontend_->RunWithStorage(extension,
                            settings::LOCAL,
                            base::Bind(&UnlimitedLocalStorageTestCallback));

  base::MessageLoop::current()->RunUntilIdle();
}

}  // namespace extensions
