// Copyright (c) 2009 The Chromium Authors. All rights reserved. Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#ifndef CHROME_BROWSER_VIEWS_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_GTK_H_
#define CHROME_BROWSER_VIEWS_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_GTK_H_

#include "views/widget/widget_gtk.h"

class AutocompleteEditView;
class AutocompletePopupContentsView;

class AutocompletePopupGtk : public views::WidgetGtk {
 public:
  // Creates the popup and shows it. |edit_view| is the edit that created us.
  AutocompletePopupGtk(AutocompleteEditView* edit_view,
                       AutocompletePopupContentsView* contents);
  virtual ~AutocompletePopupGtk();

 private:
  DISALLOW_COPY_AND_ASSIGN(AutocompletePopupGtk);
};

#endif  // #ifndef CHROME_BROWSER_VIEWS_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_GTK_H_
