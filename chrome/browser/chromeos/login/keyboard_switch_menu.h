// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_KEYBOARD_SWITCH_MENU_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_KEYBOARD_SWITCH_MENU_H_
#pragma once

#include <string>

#include "base/scoped_ptr.h"
#include "chrome/browser/chromeos/status/language_menu_button.h"
#include "chrome/browser/chromeos/status/status_area_host.h"

namespace chromeos {

// A class for the button in the OOBE network configuration screen which expands
// a dropdown menu for switching keyboard layout. Note that the InputMethodMenu
// class implements the views::ViewMenuDelegate interface.
class KeyboardSwitchMenu : public InputMethodMenu {
 public:
  KeyboardSwitchMenu();
  virtual ~KeyboardSwitchMenu() {}

  // InputMethodMenu::InputMethodMenuHost implementation.
  virtual void UpdateUI(
      const std::wstring& name, const std::wstring& tooltip);
  virtual bool ShouldSupportConfigUI() { return false; }
  virtual void OpenConfigUI() {}

  // views::ViewMenuDelegate implementation which overrides the implementation
  // in InputMethodMenu.
  virtual void RunMenu(views::View* source, const gfx::Point& pt);

  // Returns current keyboard name to be placed on the keyboard menu-button.
  std::wstring GetCurrentKeyboardName() const;

  // Changes the position of the dropdown menu.
  void SetMenuOffset(int delta_x, int delta_y) {
    delta_x_ = delta_x;
    delta_y_ = delta_y;
  }

 private:
  int delta_x_;
  int delta_y_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardSwitchMenu);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_KEYBOARD_SWITCH_MENU_H_
