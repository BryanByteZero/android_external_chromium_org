// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "chrome/common/extensions/api/extension_action/page_action_handler.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_manifest_constants.h"
#include "chrome/common/extensions/manifest_tests/extension_manifest_test.h"
#include "extensions/common/error_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace errors = extension_manifest_errors;
namespace keys = extension_manifest_keys;

namespace extensions {

class PageActionManifestTest : public ExtensionManifestTest {
 protected:
  virtual void SetUp() OVERRIDE {
    ExtensionManifestTest::SetUp();
    (new PageActionHandler)->Register();
  }

  virtual const char* test_data_dir() OVERRIDE {
    return "page_action";
  }

  scoped_ptr<ActionInfo> LoadAction(const std::string& manifest_filename);
};

scoped_ptr<ActionInfo> PageActionManifestTest::LoadAction(
    const std::string& manifest_filename) {
  scoped_refptr<Extension> extension = LoadAndExpectSuccess(
      manifest_filename.c_str());
  const ActionInfo* page_action_info = ActionInfo::GetPageActionInfo(extension);
  EXPECT_TRUE(page_action_info);
  if (page_action_info) {
    return make_scoped_ptr(new ActionInfo(*page_action_info));
  }
  ADD_FAILURE() << "Expected manifest in " << manifest_filename
                << " to include a page_action section.";
  return scoped_ptr<ActionInfo>();
}

TEST_F(PageActionManifestTest, ManifestVersion2) {
  scoped_refptr<Extension> extension(
      LoadAndExpectSuccess("page_action_manifest_version_2.json"));
  ASSERT_TRUE(extension);
  const ActionInfo* page_action_info = ActionInfo::GetPageActionInfo(extension);
  ASSERT_TRUE(page_action_info);

  EXPECT_EQ("", page_action_info->id);
  EXPECT_TRUE(page_action_info->default_icon.empty());
  EXPECT_EQ("", page_action_info->default_title);
  EXPECT_TRUE(page_action_info->default_popup_url.is_empty());

  LoadAndExpectError("page_action_manifest_version_2b.json",
                     errors::kInvalidPageActionPopup);
}

TEST_F(PageActionManifestTest, LoadPageActionHelper) {
  scoped_ptr<ActionInfo> action;

  // First try with an empty dictionary.
  action = LoadAction("page_action_empty.json");
  ASSERT_TRUE(action);

  // Now setup some values to use in the action.
  const std::string id("MyExtensionActionId");
  const std::string name("MyExtensionActionName");
  std::string img1("image1.png");

  action = LoadAction("page_action.json");
  ASSERT_TRUE(action);
  ASSERT_EQ(id, action->id);

  // No title, so fall back to name.
  ASSERT_EQ(name, action->default_title);
  ASSERT_EQ(img1,
            action->default_icon.Get(extension_misc::EXTENSION_ICON_ACTION,
                                     ExtensionIconSet::MATCH_EXACTLY));

  // Same test with explicitly set type.
  action = LoadAction("page_action_type.json");
  ASSERT_TRUE(action);

  // Try an action without id key.
  action = LoadAction("page_action_no_id.json");
  ASSERT_TRUE(action);

  // Then try without the name key. It's optional, so no error.
  action = LoadAction("page_action_no_name.json");
  ASSERT_TRUE(action);
  ASSERT_TRUE(action->default_title.empty());

  // Then try without the icon paths key.
  action = LoadAction("page_action_no_icon.json");
  ASSERT_TRUE(action);

  // Now test that we can parse the new format for page actions.
  const std::string kTitle("MyExtensionActionTitle");
  const std::string kIcon("image1.png");
  const std::string kPopupHtmlFile("a_popup.html");

  action = LoadAction("page_action_new_format.json");
  ASSERT_TRUE(action);
  ASSERT_EQ(kTitle, action->default_title);
  ASSERT_FALSE(action->default_icon.empty());

  // Invalid title should give an error even with a valid name.
  LoadAndExpectError("page_action_invalid_title.json",
                     errors::kInvalidPageActionDefaultTitle);

  // Invalid name should give an error only with no title.
  action = LoadAction("page_action_invalid_name.json");
  ASSERT_TRUE(action);
  ASSERT_EQ(kTitle, action->default_title);

  LoadAndExpectError("page_action_invalid_name_no_title.json",
                     errors::kInvalidPageActionName);

  // Test that keys "popup" and "default_popup" both work, but can not
  // be used at the same time.
  // These tests require an extension_url, so we also load the manifest.

  // Only use "popup", expect success.
  scoped_refptr<Extension> extension =
      LoadAndExpectSuccess("page_action_popup.json");
  const ActionInfo* extension_action = ActionInfo::GetPageActionInfo(extension);
  ASSERT_TRUE(extension_action);
  ASSERT_STREQ(
      extension->url().Resolve(kPopupHtmlFile).spec().c_str(),
      extension_action->default_popup_url.spec().c_str());

  // Use both "popup" and "default_popup", expect failure.
  LoadAndExpectError("page_action_popup_and_default_popup.json",
                     ErrorUtils::FormatErrorMessage(
                         errors::kInvalidPageActionOldAndNewKeys,
                         keys::kPageActionDefaultPopup,
                         keys::kPageActionPopup));

  // Use only "default_popup", expect success.
  extension = LoadAndExpectSuccess("page_action_popup.json");
  extension_action = ActionInfo::GetPageActionInfo(extension);
  ASSERT_TRUE(extension_action);
  ASSERT_STREQ(
      extension->url().Resolve(kPopupHtmlFile).spec().c_str(),
      extension_action->default_popup_url.spec().c_str());

  // Setting default_popup to "" is the same as having no popup.
  action = LoadAction("page_action_empty_default_popup.json");
  ASSERT_TRUE(action);
  EXPECT_TRUE(action->default_popup_url.is_empty());
  ASSERT_STREQ(
      "",
      action->default_popup_url.spec().c_str());

  // Setting popup to "" is the same as having no popup.
  action = LoadAction("page_action_empty_popup.json");

  ASSERT_TRUE(action);
  EXPECT_TRUE(action->default_popup_url.is_empty());
  ASSERT_STREQ(
      "",
      action->default_popup_url.spec().c_str());
}

}  // namespace extensions
