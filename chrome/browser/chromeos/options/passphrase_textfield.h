// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OPTIONS_PASSPHRASE_TEXTFIELD_H_
#define CHROME_BROWSER_CHROMEOS_OPTIONS_PASSPHRASE_TEXTFIELD_H_
#pragma once

#include "ui/views/controls/textfield/textfield.h"

namespace chromeos {

class PassphraseTextfield : public views::Textfield {
 public:
  // If show_already_set is true, then the text field will show a fake password.
  explicit PassphraseTextfield(bool show_fake);

  // Override views::Textfield so that when focus is gained, then clear out the
  // fake password if appropriate. Replace it when focus is lost if the user has
  // not typed in a new password.
  virtual void OnFocus();
  virtual void OnBlur();

  // Returns the passphrase. If it's unchanged, then returns an empty string.
  std::string GetPassphrase();

 private:
  bool show_fake_;
  bool changed_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OPTIONS_PASSPHRASE_TEXTFIELD_H_
