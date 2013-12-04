// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/commands/command_service.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/common/pref_names.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "extensions/common/manifest_constants.h"

namespace {

// Get another command platform, whcih is used for simulating a command has been
// assigned with a shortcut on another platform.
std::string GetAnotherCommandPlatform() {
#if defined(OS_WIN)
  return extensions::manifest_values::kKeybindingPlatformMac;
#elif defined(OS_MACOSX)
  return extensions::manifest_values::kKeybindingPlatformChromeOs;
#elif defined(OS_CHROMEOS)
  return extensions::manifest_values::kKeybindingPlatformLinux;
#elif defined(OS_LINUX)
  return extensions::manifest_values::kKeybindingPlatformWin;
#else
  return "";
#endif
}

}  // namespace

namespace extensions {

typedef ExtensionApiTest CommandServiceTest;

IN_PROC_BROWSER_TEST_F(CommandServiceTest, RemoveShortcutSurvivesUpdate) {
  base::ScopedTempDir scoped_temp_dir;
  EXPECT_TRUE(scoped_temp_dir.CreateUniqueTempDir());
  base::FilePath pem_path = test_data_dir_.
      AppendASCII("keybinding").AppendASCII("keybinding.pem");
  base::FilePath path_v1 = PackExtensionWithOptions(
      test_data_dir_.AppendASCII("keybinding").AppendASCII("update")
                    .AppendASCII("v1"),
      scoped_temp_dir.path().AppendASCII("v1.crx"),
      pem_path,
      base::FilePath());
  base::FilePath path_v2 = PackExtensionWithOptions(
      test_data_dir_.AppendASCII("keybinding").AppendASCII("update")
                    .AppendASCII("v2"),
      scoped_temp_dir.path().AppendASCII("v2.crx"),
      pem_path,
      base::FilePath());

  ExtensionService* service = ExtensionSystem::Get(browser()->profile())->
      extension_service();
  CommandService* command_service = CommandService::Get(browser()->profile());

  const char kId[] = "pgoakhfeplldmjheffidklpoklkppipp";

  // Install v1 of the extension.
  ASSERT_TRUE(InstallExtension(path_v1, 1));
  EXPECT_TRUE(service->GetExtensionById(kId, false) != NULL);

  // Verify it has a command of Alt+Shift+F.
  ui::Accelerator accelerator = command_service->FindCommandByName(
      kId, manifest_values::kBrowserActionCommandEvent).accelerator();
  EXPECT_EQ(ui::VKEY_F, accelerator.key_code());
  EXPECT_FALSE(accelerator.IsCtrlDown());
  EXPECT_TRUE(accelerator.IsShiftDown());
  EXPECT_TRUE(accelerator.IsAltDown());

  // Remove the keybinding.
  command_service->RemoveKeybindingPrefs(
      kId, manifest_values::kBrowserActionCommandEvent);

  // Verify it got removed.
  accelerator = command_service->FindCommandByName(
      kId, manifest_values::kBrowserActionCommandEvent).accelerator();
  EXPECT_EQ(ui::VKEY_UNKNOWN, accelerator.key_code());

  // Update to version 2.
  EXPECT_TRUE(UpdateExtension(kId, path_v2, 0));
  EXPECT_TRUE(service->GetExtensionById(kId, false) != NULL);

  // Verify it is still set to nothing.
  accelerator = command_service->FindCommandByName(
      kId, manifest_values::kBrowserActionCommandEvent).accelerator();
  EXPECT_EQ(ui::VKEY_UNKNOWN, accelerator.key_code());
}

IN_PROC_BROWSER_TEST_F(CommandServiceTest,
                       RemoveKeybindingPrefsShouldBePlatformSpecific) {
  base::FilePath extension_dir =
      test_data_dir_.AppendASCII("keybinding").AppendASCII("basics");
  const Extension* extension = InstallExtension(extension_dir, 1);
  ASSERT_TRUE(extension);

  DictionaryPrefUpdate updater(browser()->profile()->GetPrefs(),
                               prefs::kExtensionCommands);
  base::DictionaryValue* bindings = updater.Get();

  // Simulate command |toggle-feature| has been assigned with a shortcut on
  // another platform.
  std::string anotherPlatformKey = GetAnotherCommandPlatform() + ":Alt+G";
  const char kNamedCommandName[] = "toggle-feature";
  base::DictionaryValue* keybinding = new base::DictionaryValue();
  keybinding->SetString("extension", extension->id());
  keybinding->SetString("command_name", kNamedCommandName);
  keybinding->SetBoolean("global", false);
  bindings->Set(anotherPlatformKey, keybinding);

  CommandService* command_service = CommandService::Get(browser()->profile());
  command_service->RemoveKeybindingPrefs(extension->id(), kNamedCommandName);

  // Removal of keybinding preference should be platform-specific, so the key on
  // another platform should always remained.
  EXPECT_TRUE(bindings->HasKey(anotherPlatformKey));
}

}  // namespace extensions
