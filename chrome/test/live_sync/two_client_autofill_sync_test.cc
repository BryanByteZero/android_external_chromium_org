// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/utf_string_conversions.h"
#include "chrome/browser/sync/profile_sync_service_harness.h"
#include "chrome/browser/webdata/autofill_entry.h"
#include "chrome/test/live_sync/autofill_helper.h"
#include "chrome/test/live_sync/live_sync_test.h"

// Autofill entry length is limited to 1024.  See http://crbug.com/49332.
const size_t kMaxDataLength = 1024;

class TwoClientAutofillSyncTest : public LiveSyncTest {
 public:
  TwoClientAutofillSyncTest() : LiveSyncTest(TWO_CLIENT) {}
  virtual ~TwoClientAutofillSyncTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TwoClientAutofillSyncTest);
};

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, WebDataServiceSanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Client0 adds a key.
  std::set<AutofillKey> keys;
  keys.insert(AutofillKey("name0", "value0"));
  AutofillHelper::AddKeys(0, keys);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::KeysMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllKeys(0).size());

  // Client1 adds a key.
  keys.clear();
  keys.insert(AutofillKey("name1", "value1-0"));
  AutofillHelper::AddKeys(1, keys);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(AutofillHelper::KeysMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllKeys(0).size());

  // Client0 adds a key with the same name.
  keys.clear();
  keys.insert(AutofillKey("name1", "value1-1"));
  AutofillHelper::AddKeys(0, keys);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::KeysMatch(0, 1));
  ASSERT_EQ(3U, AutofillHelper::GetAllKeys(0).size());

  // Client1 removes a key.
  AutofillHelper::RemoveKey(1, AutofillKey("name1", "value1-0"));
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(AutofillHelper::KeysMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllKeys(0).size());

  // Client0 removes the rest.
  AutofillHelper::RemoveKey(0, AutofillKey("name0", "value0"));
  AutofillHelper::RemoveKey(0, AutofillKey("name1", "value1-1"));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::KeysMatch(0, 1));
  ASSERT_EQ(0U, AutofillHelper::GetAllKeys(0).size());
}

// TCM ID - 3678296.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddUnicodeProfile) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys;
  keys.insert(AutofillKey(WideToUTF16(L"Sigur R\u00F3s"),
                          WideToUTF16(L"\u00C1g\u00E6tis byrjun")));
  AutofillHelper::AddKeys(0, keys);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::KeysMatch(0, 1));
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       AddDuplicateNamesToSameProfile) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys;
  keys.insert(AutofillKey("name0", "value0-0"));
  keys.insert(AutofillKey("name0", "value0-1"));
  keys.insert(AutofillKey("name1", "value1"));
  AutofillHelper::AddKeys(0, keys);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::KeysMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllKeys(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       AddDuplicateNamesToDifferentProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  std::set<AutofillKey> keys0;
  keys0.insert(AutofillKey("name0", "value0-0"));
  keys0.insert(AutofillKey("name1", "value1"));
  AutofillHelper::AddKeys(0, keys0);

  std::set<AutofillKey> keys1;
  keys1.insert(AutofillKey("name0", "value0-1"));
  keys1.insert(AutofillKey("name2", "value2"));
  keys1.insert(AutofillKey("name3", "value3"));
  AutofillHelper::AddKeys(1, keys1);

  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::KeysMatch(0, 1));
  ASSERT_EQ(5U, AutofillHelper::GetAllKeys(0).size());
}

IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest,
                       PersonalDataManagerSanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  // Client0 adds a profile.
  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  // Client1 adds a profile.
  AutofillHelper::AddProfile(
      1, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_MARION));
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllProfiles(0).size());

  // Client0 adds the same profile.
  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_MARION));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllProfiles(0).size());

  // Client1 removes a profile.
  AutofillHelper::RemoveProfile(
      1, AutofillHelper::GetAllProfiles(1)[0]->guid());
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  // Client0 updates a profile.
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(NAME_FIRST),
      ASCIIToUTF16("Bart"));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  // Client1 removes remaining profile.
  AutofillHelper::RemoveProfile(
      1, AutofillHelper::GetAllProfiles(1)[0]->guid());
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(0U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 7261786.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddDuplicateProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3636294.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, SameProfileWithConflict) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AutofillProfile profile0 =
      AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER);
  AutofillProfile profile1 =
      AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER);
  profile1.SetInfo(PHONE_FAX_WHOLE_NUMBER, ASCIIToUTF16("1234567890"));

  AutofillHelper::AddProfile(0, profile0);
  AutofillHelper::AddProfile(1, profile1);
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3626291.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddEmptyProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_NULL));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(0U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3616283.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3632260.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, AddMultipleProfiles) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_MARION));
  AutofillHelper::AddProfile(
      0,
      AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_FRASIER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(3U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3602257.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, DeleteProfile) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  AutofillHelper::RemoveProfile(
      1, AutofillHelper::GetAllProfiles(1)[0]->guid());
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(0U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3627300.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, MergeProfiles) {
  ASSERT_TRUE(SetupClients()) << "SetupClients() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  AutofillHelper::AddProfile(
      1, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_MARION));
  AutofillHelper::AddProfile(
      1,
      AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_FRASIER));
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(3U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3665264.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, UpdateFields) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(NAME_FIRST),
      ASCIIToUTF16("Lisa"));
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(EMAIL_ADDRESS),
      ASCIIToUTF16("grrrl@TV.com"));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3628299.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, ConflictingFields) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(NAME_FIRST),
      ASCIIToUTF16("Lisa"));
  AutofillHelper::UpdateProfile(
      1,
      AutofillHelper::GetAllProfiles(1)[0]->guid(),
      AutofillType(NAME_FIRST),
      ASCIIToUTF16("Bart"));
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3663293.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, DisableAutofill) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  ASSERT_TRUE(GetClient(0)->DisableSyncForDatatype(syncable::AUTOFILL));
  AutofillHelper::AddProfile(
      0,
      AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_FRASIER));
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_FALSE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllProfiles(0).size());
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(1).size());

  ASSERT_TRUE(GetClient(0)->EnableSyncForDatatype(syncable::AUTOFILL));
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3661291.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, DisableSync) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  ASSERT_TRUE(GetClient(1)->DisableSyncForAllDatatypes());
  AutofillHelper::AddProfile(
      0,
      AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_FRASIER));
  ASSERT_TRUE(GetClient(0)->AwaitSyncCycleCompletion("Added a profile."));
  ASSERT_FALSE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllProfiles(0).size());
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(1).size());

  ASSERT_TRUE(GetClient(1)->EnableSyncForAllDatatypes());
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(2U, AutofillHelper::GetAllProfiles(0).size());
}

// TCM ID - 3608295.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, MaxLength) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  string16 max_length_string(kMaxDataLength, '.');
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(NAME_FIRST),
      max_length_string);
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(NAME_LAST),
      max_length_string);
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(EMAIL_ADDRESS),
      max_length_string);
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(ADDRESS_HOME_LINE1),
      max_length_string);

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
}

// TODO(braffert): Remove FAILS annotation when crbug.com/85769 is resolved.
// TCM ID - 7735472.
IN_PROC_BROWSER_TEST_F(TwoClientAutofillSyncTest, FAILS_ExceedsMaxLength) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  AutofillHelper::AddProfile(
      0, AutofillHelper::CreateAutofillProfile(AutofillHelper::PROFILE_HOMER));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(AutofillHelper::ProfilesMatch(0, 1));
  ASSERT_EQ(1U, AutofillHelper::GetAllProfiles(0).size());

  string16 exceeds_max_length_string(kMaxDataLength + 1, '.');
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(NAME_FIRST),
      exceeds_max_length_string);
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(NAME_LAST),
      exceeds_max_length_string);
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(EMAIL_ADDRESS),
      exceeds_max_length_string);
  AutofillHelper::UpdateProfile(
      0,
      AutofillHelper::GetAllProfiles(0)[0]->guid(),
      AutofillType(ADDRESS_HOME_LINE1),
      exceeds_max_length_string);

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(AutofillHelper::ProfilesMatch(0, 1));
}
