// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/prediction_options.h"

#include "base/logging.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/network_change_notifier.h"

namespace {

// Since looking up preferences and current network connection are presumably
// both cheap, we do not cache them here.
bool CanPredictNetworkActions(int NetworkPredictionOptions,
                              bool NetworkPredictionEnabled) {
  switch (NetworkPredictionOptions) {
    case chrome_browser_net::NETWORK_PREDICTION_ALWAYS:
      return true;
    case chrome_browser_net::NETWORK_PREDICTION_WIFI_ONLY:
      return !net::NetworkChangeNotifier::IsConnectionCellular(
          net::NetworkChangeNotifier::GetConnectionType());
    case chrome_browser_net::NETWORK_PREDICTION_NEVER:
      return false;
    case chrome_browser_net::NETWORK_PREDICTION_UNSET:
      return NetworkPredictionEnabled;
    default:
      NOTREACHED() << "Unknown kNetworkPredictionOptions value.";
      return false;
  }
}

}  // namespace

namespace chrome_browser_net {

void RegisterPredictionOptionsProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(
      prefs::kNetworkPredictionOptions,
      chrome_browser_net::NETWORK_PREDICTION_UNSET,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

void MigrateNetworkPredictionUserPrefs(PrefService* pref_service) {
  // Nothing to do if the user or this migration code has already set the new
  // preference.
  if (pref_service->GetUserPrefValue(prefs::kNetworkPredictionOptions))
    return;

  // Nothing to do if the user has not set the old preference.
  const base::Value* network_prediction_enabled =
      pref_service->GetUserPrefValue(prefs::kNetworkPredictionEnabled);
  if (!network_prediction_enabled)
    return;

  bool value = false;
  if (network_prediction_enabled->GetAsBoolean(&value)) {
    pref_service->SetInteger(
        prefs::kNetworkPredictionOptions,
        value ? chrome_browser_net::NETWORK_PREDICTION_WIFI_ONLY
              : chrome_browser_net::NETWORK_PREDICTION_NEVER);
  }
}

bool CanPredictNetworkActionsIO(ProfileIOData* profile_io_data) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  DCHECK(profile_io_data);

  return CanPredictNetworkActions(
      profile_io_data->network_prediction_options()->GetValue(),
      profile_io_data->network_prediction_enabled()->GetValue());
}

bool CanPredictNetworkActionsUI(PrefService* prefs) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(prefs);
  return CanPredictNetworkActions(
      prefs->GetInteger(prefs::kNetworkPredictionOptions),
      prefs->GetBoolean(prefs::kNetworkPredictionEnabled));
}

}  // namespace chrome_browser_net
