// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This enum defines item identifiers for Autofill popup controller.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_POPUP_ITEM_ID_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_POPUP_ITEM_ID_H_

namespace autofill {

enum PopupItemId {
  POPUP_ITEM_ID_AUTOCOMPLETE_ENTRY = 0,
  POPUP_ITEM_ID_WARNING_MESSAGE = -1,
  POPUP_ITEM_ID_PASSWORD_ENTRY = -2,
  POPUP_ITEM_ID_SEPARATOR = -3,
  POPUP_ITEM_ID_CLEAR_FORM = -4,
  POPUP_ITEM_ID_AUTOFILL_OPTIONS = -5,
  POPUP_ITEM_ID_DATALIST_ENTRY = -6,
  POPUP_ITEM_ID_MAC_ACCESS_CONTACTS = -7,
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_POPUP_ITEM_ID_H_
