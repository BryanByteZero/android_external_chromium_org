// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOM_UI_SHOWN_SECTIONS_HANDLER_H_
#define CHROME_BROWSER_DOM_UI_SHOWN_SECTIONS_HANDLER_H_
#pragma once

#include "chrome/browser/dom_ui/dom_ui.h"
#include "chrome/common/notification_observer.h"

class DOMUI;
class Value;
class PrefService;

// Use for the shown sections bitmask.
enum Section {
  THUMB = 1,
  LIST = 2,
  RECENT = 4,
  TIPS = 8,
  SYNC = 16,
  DEBUG = 32,
  APPS = 64
};

class ShownSectionsHandler : public DOMMessageHandler,
                             public NotificationObserver {
 public:
  explicit ShownSectionsHandler(PrefService* pref_service);
  virtual ~ShownSectionsHandler();

  // Helper to get the current shown sections.
  static int GetShownSections(PrefService* pref_service);

  // DOMMessageHandler implementation.
  virtual void RegisterMessages();

  // NotificationObserver implementation.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // Callback for "getShownSections" message.
  void HandleGetShownSections(const ListValue* args);

  // Callback for "setShownSections" message.
  void HandleSetShownSections(const ListValue* args);

  static void RegisterUserPrefs(PrefService* pref_service);

  static void MigrateUserPrefs(PrefService* pref_service,
                               int old_pref_version,
                               int new_pref_version);

 private:
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(ShownSectionsHandler);
};

#endif  // CHROME_BROWSER_DOM_UI_SHOWN_SECTIONS_HANDLER_H_
