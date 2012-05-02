// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OPTIONS2_OPTIONS_UI2_H_
#define CHROME_BROWSER_UI_WEBUI_OPTIONS2_OPTIONS_UI2_H_
#pragma once

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "chrome/browser/autocomplete/autocomplete.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_message_handler.h"

#if defined(OS_CHROMEOS)
namespace chromeos {
namespace system {
class PointerDeviceObserver;
}  // namespace system
}  // namespace chromeos
#endif

namespace options2 {

// The base class handler of Javascript messages of options pages.
class OptionsPageUIHandler : public content::WebUIMessageHandler,
                             public content::NotificationObserver {
 public:
  OptionsPageUIHandler();
  virtual ~OptionsPageUIHandler();

  // Is this handler enabled?
  virtual bool IsEnabled();

  // Collects localized strings for options page.
  virtual void GetLocalizedValues(base::DictionaryValue* localized_strings) = 0;

  // Will be called only once in the life time of the handler. Generally used to
  // add observers, initializes preferences, or start asynchronous calls from
  // various services.
  virtual void InitializeHandler() {}

  // Initialize the page. Called once the DOM is available for manipulation.
  // This will be called when a RenderView is re-used (when navigated to with
  // back/forward or session restored in some cases) or when created.
  virtual void InitializePage() {}

  // Uninitializes the page.  Called just before the object is destructed.
  virtual void Uninitialize() {}

  // WebUIMessageHandler implementation.
  virtual void RegisterMessages() OVERRIDE {}

  // content::NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE {}

 protected:
  struct OptionsStringResource {
    // The name of the resource in templateData.
    const char* name;
    // The .grd ID for the resource (IDS_*).
    int id;
  };
  // A helper for simplifying the process of registering strings in WebUI.
  static void RegisterStrings(base::DictionaryValue* localized_strings,
                              const OptionsStringResource* resources,
                              size_t length);

  // Registers string resources for a page's header and tab title.
  static void RegisterTitle(base::DictionaryValue* localized_strings,
                            const std::string& variable_name,
                            int title_id);

  content::NotificationRegistrar registrar_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OptionsPageUIHandler);
};

// An interface for common operations that a host of OptionsPageUIHandlers
// should provide.
class OptionsPageUIHandlerHost {
 public:
  virtual void InitializeHandlers() = 0;

 protected:
  virtual ~OptionsPageUIHandlerHost() {}
};

// The WebUI for chrome:settings-frame.
class OptionsUI : public content::WebUIController,
                  public OptionsPageUIHandlerHost {
 public:
  explicit OptionsUI(content::WebUI* web_ui);
  virtual ~OptionsUI();

  // Takes the suggestions from |autocompleteResult| and adds them to
  // |suggestions| so that they can be passed to a JavaScript function.
  static void ProcessAutocompleteSuggestions(
      const AutocompleteResult& autocompleteResult,
      base::ListValue * const suggestions);

  static RefCountedMemory* GetFaviconResourceBytes();

  // Overridden from OptionsPageUIHandlerHost:
  virtual void InitializeHandlers() OVERRIDE;

 private:
  // Adds OptionsPageUiHandler to the handlers list if handler is enabled.
  void AddOptionsPageUIHandler(base::DictionaryValue* localized_strings,
                               OptionsPageUIHandler* handler);

  bool initialized_handlers_;

  std::vector<OptionsPageUIHandler*> handlers_;

#if defined(OS_CHROMEOS)
  scoped_ptr<chromeos::system::PointerDeviceObserver>
      pointer_device_observer_;
#endif

  DISALLOW_COPY_AND_ASSIGN(OptionsUI);
};

}  // namespace options2

#endif  // CHROME_BROWSER_UI_WEBUI_OPTIONS2_OPTIONS_UI2_H_
