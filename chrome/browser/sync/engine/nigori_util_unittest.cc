// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/protocol/bookmark_specifics.pb.h"
#include "chrome/browser/sync/protocol/sync.pb.h"
#include "chrome/browser/sync/util/cryptographer.h"
#include "chrome/browser/sync/engine/nigori_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncable {

typedef testing::Test NigoriUtilTest;

TEST(NigoriUtilTest, SpecificsNeedsEncryption) {
  ModelTypeSet encrypted_types;
  encrypted_types.Put(BOOKMARKS);
  encrypted_types.Put(PASSWORDS);

  sync_pb::EntitySpecifics specifics;
  EXPECT_FALSE(SpecificsNeedsEncryption(ModelTypeSet(), specifics));
  EXPECT_FALSE(SpecificsNeedsEncryption(encrypted_types, specifics));

  AddDefaultFieldValue(PREFERENCES, &specifics);
  EXPECT_FALSE(SpecificsNeedsEncryption(encrypted_types, specifics));

  sync_pb::EntitySpecifics bookmark_specifics;
  AddDefaultFieldValue(BOOKMARKS, &bookmark_specifics);
  EXPECT_TRUE(SpecificsNeedsEncryption(encrypted_types, bookmark_specifics));

  bookmark_specifics.mutable_bookmark()->set_title("title");
  bookmark_specifics.mutable_bookmark()->set_url("url");
  EXPECT_TRUE(SpecificsNeedsEncryption(encrypted_types, bookmark_specifics));
  EXPECT_FALSE(SpecificsNeedsEncryption(ModelTypeSet(), bookmark_specifics));

  bookmark_specifics.mutable_encrypted();
  EXPECT_FALSE(SpecificsNeedsEncryption(encrypted_types, bookmark_specifics));
  EXPECT_FALSE(SpecificsNeedsEncryption(ModelTypeSet(), bookmark_specifics));

  sync_pb::EntitySpecifics password_specifics;
  AddDefaultFieldValue(PASSWORDS, &password_specifics);
  EXPECT_FALSE(SpecificsNeedsEncryption(encrypted_types, password_specifics));
}

// ProcessUnsyncedChangesForEncryption and other methods that rely on the syncer
// are tested in apply_updates_command_unittest.cc

}  // namespace syncable
