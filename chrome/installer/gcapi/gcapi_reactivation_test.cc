// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/basictypes.h"
#include "base/string_number_conversions.h"
#include "base/stringprintf.h"
#include "base/test/test_reg_util_win.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "base/win/registry.h"
#include "chrome/common/guid.h"
#include "chrome/installer/gcapi/gcapi.h"
#include "chrome/installer/gcapi/gcapi_reactivation.h"
#include "chrome/installer/util/google_update_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Time;
using base::TimeDelta;
using base::win::RegKey;


class GCAPIReactivationTest : public ::testing::Test {
 protected:
  void SetUp() {
    // Override keys - this is undone during destruction.
    std::wstring hkcu_override = base::StringPrintf(
        L"hkcu_override\\%ls", ASCIIToWide(guid::GenerateGUID()));
    override_manager_.OverrideRegistry(HKEY_CURRENT_USER, hkcu_override);
  }

  bool SetChromeInstallMarker(HKEY hive) {
    // Create the client state keys in the right places.
    std::wstring reg_path(google_update::kRegPathClients);
    reg_path += L"\\";
    reg_path += google_update::kChromeUpgradeCode;
    RegKey client_state(hive,
                        reg_path.c_str(),
                        KEY_CREATE_SUB_KEY | KEY_SET_VALUE);
    return (client_state.Valid() &&
            client_state.WriteValue(
                google_update::kRegVersionField, L"1.2.3.4") == ERROR_SUCCESS);
  }

  bool SetLastRunTime(HKEY hive, int64 last_run_time) {
    return SetLastRunTimeString(hive, base::Int64ToString16(last_run_time));
  }

  bool SetLastRunTimeString(HKEY hive, const string16& last_run_time_string) {
    const wchar_t* base_path =
        (hive == HKEY_LOCAL_MACHINE) ?
            google_update::kRegPathClientStateMedium :
            google_update::kRegPathClientState;
    std::wstring path(base_path);
    path += L"\\";
    path += google_update::kChromeUpgradeCode;

    RegKey client_state(hive, path.c_str(), KEY_SET_VALUE);
    return (client_state.Valid() &&
            client_state.WriteValue(
                google_update::kRegLastRunTimeField,
                last_run_time_string.c_str()) == ERROR_SUCCESS);
  }

  std::wstring GetReactivationString(HKEY hive) {
    const wchar_t* base_path =
        (hive == HKEY_LOCAL_MACHINE) ?
            google_update::kRegPathClientStateMedium :
            google_update::kRegPathClientState;
    std::wstring path(base_path);
    path += L"\\";
    path += google_update::kChromeUpgradeCode;

    RegKey client_state(hive, path.c_str(), KEY_QUERY_VALUE);
    if (client_state.Valid()) {
      std::wstring actual_brand;
      if (client_state.ReadValue(google_update::kRegRLZReactivationBrandField,
                                 &actual_brand) == ERROR_SUCCESS) {
        return actual_brand;
      }
    }

    return L"ERROR";
  }

 private:
  registry_util::RegistryOverrideManager override_manager_;
};

TEST_F(GCAPIReactivationTest, CheckSetReactivationBrandCode) {
  EXPECT_TRUE(SetReactivationBrandCode(L"GAGA"));
  EXPECT_EQ(L"GAGA", GetReactivationString(HKEY_CURRENT_USER));

  std::vector<std::wstring> check_codes;
  check_codes.push_back(L"GAGA");
  EXPECT_TRUE(HasBeenReactivatedByBrandCodes(check_codes));

  check_codes.push_back(L"GOOGOO");
  EXPECT_TRUE(HasBeenReactivatedByBrandCodes(check_codes));

  check_codes.erase(check_codes.begin());
  EXPECT_FALSE(HasBeenReactivatedByBrandCodes(check_codes));
}

TEST_F(GCAPIReactivationTest, CanOfferReactivation_Basic) {
  const wchar_t* previous_brands[] = {L"GOOGOO", L"MAMA", L"DADA"};
  DWORD error;

  // We're not installed yet. Make sure CanOfferReactivation fails.
  EXPECT_FALSE(CanOfferReactivation(L"GAGA", arraysize(previous_brands),
                                    previous_brands, &error));
  EXPECT_EQ(REACTIVATE_ERROR_NOTINSTALLED, error);

  // Now pretend to be installed. CanOfferReactivation should pass.
  EXPECT_TRUE(SetChromeInstallMarker(HKEY_CURRENT_USER));
  EXPECT_TRUE(CanOfferReactivation(L"GAGA", arraysize(previous_brands),
                                   previous_brands, &error));

  // Now set a recent last_run value. CanOfferReactivation should fail again.
  Time hkcu_last_run = Time::NowFromSystemTime() - TimeDelta::FromDays(20);
  EXPECT_TRUE(SetLastRunTime(HKEY_CURRENT_USER,
                             hkcu_last_run.ToInternalValue()));
  EXPECT_FALSE(CanOfferReactivation(L"GAGA", arraysize(previous_brands),
                                    previous_brands, &error));
  EXPECT_EQ(REACTIVATE_ERROR_NOTDORMANT, error);

  // Now set a last_run value that exceeds the threshold.
  hkcu_last_run = Time::NowFromSystemTime() -
      TimeDelta::FromDays(kReactivationMinDaysDormant);
  EXPECT_TRUE(SetLastRunTime(HKEY_CURRENT_USER,
                             hkcu_last_run.ToInternalValue()));
  EXPECT_TRUE(CanOfferReactivation(L"GAGA", arraysize(previous_brands),
                                   previous_brands, &error));

  // Test some invalid inputs
  EXPECT_FALSE(CanOfferReactivation(NULL, arraysize(previous_brands),
                                    previous_brands, &error));
  EXPECT_EQ(REACTIVATE_ERROR_INVALID_INPUT, error);
  EXPECT_FALSE(CanOfferReactivation(L"GAGA", arraysize(previous_brands),
                                    NULL, &error));
  EXPECT_EQ(REACTIVATE_ERROR_INVALID_INPUT, error);

  // One more valid one
  EXPECT_TRUE(CanOfferReactivation(L"GAGA", 0, NULL, &error));

  // Check that the previous brands check works:
  EXPECT_TRUE(SetReactivationBrandCode(L"GOOGOO"));
  EXPECT_FALSE(CanOfferReactivation(L"GAGA", arraysize(previous_brands),
                                    previous_brands, &error));
  EXPECT_EQ(REACTIVATE_ERROR_ALREADY_REACTIVATED, error);
}

TEST_F(GCAPIReactivationTest, Reactivation_Flow) {
  const wchar_t* previous_brands[] = {L"GOOGOO", L"MAMA", L"DADA"};
  DWORD error;

  // Set us up as a candidate for reactivation.
  EXPECT_TRUE(SetChromeInstallMarker(HKEY_CURRENT_USER));

  Time hkcu_last_run = Time::NowFromSystemTime() -
      TimeDelta::FromDays(kReactivationMinDaysDormant);
  EXPECT_TRUE(SetLastRunTime(HKEY_CURRENT_USER,
                             hkcu_last_run.ToInternalValue()));

  EXPECT_TRUE(ReactivateChrome(L"GAGA", arraysize(previous_brands),
                               previous_brands, &error));
  EXPECT_EQ(L"GAGA", GetReactivationString(HKEY_CURRENT_USER));

  // Make sure we can't reactivate again:
  EXPECT_FALSE(ReactivateChrome(L"GAGA", arraysize(previous_brands),
                                previous_brands, &error));
  EXPECT_EQ(REACTIVATE_ERROR_ALREADY_REACTIVATED, error);

  // Should still be able to reactivate under other brands:
  EXPECT_TRUE(ReactivateChrome(L"MAMA", arraysize(previous_brands),
                               previous_brands, &error));
  EXPECT_EQ(L"MAMA", GetReactivationString(HKEY_CURRENT_USER));

  // Validate that previous_brands are rejected:
  EXPECT_FALSE(ReactivateChrome(L"PFFT", arraysize(previous_brands),
                                previous_brands, &error));
  EXPECT_EQ(REACTIVATE_ERROR_ALREADY_REACTIVATED, error);
  EXPECT_EQ(L"MAMA", GetReactivationString(HKEY_CURRENT_USER));
}
