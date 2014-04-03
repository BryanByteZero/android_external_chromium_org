// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_overrides/settings_overrides_api.h"

#include "base/lazy_instance.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/preference/preference_api.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/extensions/manifest_handlers/settings_overrides_handler.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

namespace {

base::LazyInstance<BrowserContextKeyedAPIFactory<SettingsOverridesAPI> >
    g_factory = LAZY_INSTANCE_INITIALIZER;

const char kManyStartupPagesWarning[] = "* specifies more than 1 startup URL. "
    "All but the first will be ignored.";

using api::manifest_types::ChromeSettingsOverrides;

std::string SubstituteInstallParam(std::string str,
                                   const std::string& install_parameter) {
  ReplaceSubstringsAfterOffset(&str, 0, "__PARAM__", install_parameter);
  return str;
}

TemplateURLData ConvertSearchProvider(
    const ChromeSettingsOverrides::Search_provider& search_provider,
    const std::string& install_parameter) {
  TemplateURLData data;

  data.short_name = base::UTF8ToUTF16(search_provider.name);
  data.SetKeyword(base::UTF8ToUTF16(search_provider.keyword));
  data.SetURL(
      SubstituteInstallParam(search_provider.search_url, install_parameter));
  if (search_provider.suggest_url) {
    data.suggestions_url =
        SubstituteInstallParam(*search_provider.suggest_url, install_parameter);
  }
  if (search_provider.instant_url) {
    data.instant_url =
        SubstituteInstallParam(*search_provider.instant_url, install_parameter);
  }
  if (search_provider.image_url) {
    data.image_url =
        SubstituteInstallParam(*search_provider.image_url, install_parameter);
  }
  if (search_provider.search_url_post_params)
    data.search_url_post_params = *search_provider.search_url_post_params;
  if (search_provider.suggest_url_post_params)
    data.suggestions_url_post_params = *search_provider.suggest_url_post_params;
  if (search_provider.instant_url_post_params)
    data.instant_url_post_params = *search_provider.instant_url_post_params;
  if (search_provider.image_url_post_params)
    data.image_url_post_params = *search_provider.image_url_post_params;
  data.favicon_url = GURL(
      SubstituteInstallParam(search_provider.favicon_url, install_parameter));
  data.show_in_default_list = true;
  data.safe_for_autoreplace = false;
  data.input_encodings.push_back(search_provider.encoding);
  data.date_created = base::Time();
  data.last_modified = base::Time();
  data.prepopulate_id = 0;
  if (search_provider.alternate_urls) {
    for (size_t i = 0; i < search_provider.alternate_urls->size(); ++i) {
      if (!search_provider.alternate_urls->at(i).empty())
        data.alternate_urls.push_back(SubstituteInstallParam(
            search_provider.alternate_urls->at(i), install_parameter));
    }
  }
  return data;
}

}  // namespace

SettingsOverridesAPI::SettingsOverridesAPI(content::BrowserContext* context)
    : profile_(Profile::FromBrowserContext(context)),
      url_service_(TemplateURLServiceFactory::GetForProfile(profile_)) {
  DCHECK(profile_);
  registrar_.Add(this,
                 chrome::NOTIFICATION_EXTENSION_LOADED,
                 content::Source<Profile>(profile_));
  registrar_.Add(this,
                 chrome::NOTIFICATION_EXTENSION_UNLOADED_DEPRECATED,
                 content::Source<Profile>(profile_));
}

SettingsOverridesAPI::~SettingsOverridesAPI() {
}

BrowserContextKeyedAPIFactory<SettingsOverridesAPI>*
SettingsOverridesAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

void SettingsOverridesAPI::SetPref(const std::string& extension_id,
                                   const std::string& pref_key,
                                   base::Value* value) {
  PreferenceAPI* prefs = PreferenceAPI::Get(profile_);
  if (!prefs)
    return;  // Expected in unit tests.
  prefs->SetExtensionControlledPref(extension_id,
                                    pref_key,
                                    kExtensionPrefsScopeRegular,
                                    value);
}

void SettingsOverridesAPI::UnsetPref(const std::string& extension_id,
                                     const std::string& pref_key) {
  PreferenceAPI* prefs = PreferenceAPI::Get(profile_);
  if (!prefs)
    return;  // Expected in unit tests.
  prefs->RemoveExtensionControlledPref(
      extension_id,
      pref_key,
      kExtensionPrefsScopeRegular);
}

void SettingsOverridesAPI::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_EXTENSION_LOADED: {
      const Extension* extension =
          content::Details<const Extension>(details).ptr();
      const SettingsOverrides* settings =
          SettingsOverrides::Get(extension);
      if (settings) {
        std::string install_parameter =
            ExtensionPrefs::Get(profile_)->GetInstallParam(extension->id());
        if (settings->homepage) {
          SetPref(extension->id(),
                  prefs::kHomePage,
                  new base::StringValue(SubstituteInstallParam(
                      settings->homepage->spec(), install_parameter)));
          SetPref(extension->id(), prefs::kHomePageIsNewTabPage,
                  new base::FundamentalValue(false));
        }
        if (!settings->startup_pages.empty()) {
          SetPref(extension->id(), prefs::kRestoreOnStartup,
                  new base::FundamentalValue(
                      SessionStartupPref::kPrefValueURLs));
          if (settings->startup_pages.size() > 1) {
            VLOG(1) << extensions::ErrorUtils::FormatErrorMessage(
                kManyStartupPagesWarning, manifest_keys::kSettingsOverride);
          }
          scoped_ptr<base::ListValue> url_list(new base::ListValue);
          url_list->Append(new base::StringValue(SubstituteInstallParam(
              settings->startup_pages[0].spec(), install_parameter)));
          SetPref(extension->id(), prefs::kURLsToRestoreOnStartup,
                  url_list.release());
        }
        if (settings->search_engine) {
          SetPref(extension->id(), prefs::kDefaultSearchProviderEnabled,
                  new base::FundamentalValue(true));
          DCHECK(url_service_);
          if (url_service_->loaded()) {
            RegisterSearchProvider(extension);
          } else {
            if (!template_url_sub_) {
              template_url_sub_ = url_service_->RegisterOnLoadedCallback(
                  base::Bind(&SettingsOverridesAPI::OnTemplateURLsLoaded,
                             base::Unretained(this)));
            }
            url_service_->Load();
            pending_extensions_.insert(extension);
          }
        }
      }
      break;
    }
    case chrome::NOTIFICATION_EXTENSION_UNLOADED_DEPRECATED: {
      const Extension* extension =
          content::Details<UnloadedExtensionInfo>(details)->extension;
      const SettingsOverrides* settings = SettingsOverrides::Get(extension);
      if (settings) {
        if (settings->homepage) {
          UnsetPref(extension->id(), prefs::kHomePage);
          UnsetPref(extension->id(), prefs::kHomePageIsNewTabPage);
        }
        if (!settings->startup_pages.empty()) {
          UnsetPref(extension->id(), prefs::kRestoreOnStartup);
          UnsetPref(extension->id(), prefs::kURLsToRestoreOnStartup);
        }
        if (settings->search_engine) {
          UnsetPref(extension->id(), prefs::kDefaultSearchProviderEnabled);
          DCHECK(url_service_);
          if (url_service_->loaded())
            url_service_->RemoveExtensionControlledTURL(extension->id());
          else
            pending_extensions_.erase(extension);
        }
      }
      break;
    }
    default: {
      NOTREACHED();
      break;
    }
  }
}

void SettingsOverridesAPI::Shutdown() {
  template_url_sub_.reset();
}

void SettingsOverridesAPI::OnTemplateURLsLoaded() {
  // Register search providers for pending extensions.
  template_url_sub_.reset();
  for (PendingExtensions::const_iterator i(pending_extensions_.begin());
       i != pending_extensions_.end(); ++i) {
    RegisterSearchProvider(*i);
  }
  pending_extensions_.clear();
}

void SettingsOverridesAPI::RegisterSearchProvider(
    const Extension* extension) const {
  DCHECK(url_service_);
  DCHECK(extension);
  const SettingsOverrides* settings = SettingsOverrides::Get(extension);
  DCHECK(settings);
  DCHECK(settings->search_engine);
  scoped_ptr<AssociatedExtensionInfo> info(new AssociatedExtensionInfo);
  info->extension_id = extension->id();
  info->wants_to_be_default_engine = settings->search_engine->is_default;
  ExtensionPrefs* prefs = ExtensionPrefs::Get(profile_);
  info->install_time = prefs->GetInstallTime(extension->id());
  std::string install_parameter = prefs->GetInstallParam(extension->id());
  TemplateURLData data =
      ConvertSearchProvider(*settings->search_engine, install_parameter);
  url_service_->AddExtensionControlledTURL(new TemplateURL(profile_, data),
                                           info.Pass());
}

template <>
void BrowserContextKeyedAPIFactory<
    SettingsOverridesAPI>::DeclareFactoryDependencies() {
  DependsOn(ExtensionPrefsFactory::GetInstance());
  DependsOn(PreferenceAPI::GetFactoryInstance());
  DependsOn(TemplateURLServiceFactory::GetInstance());
}

}  // namespace extensions
