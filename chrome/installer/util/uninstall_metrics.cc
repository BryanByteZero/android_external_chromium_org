// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/uninstall_metrics.h"

#include <string>

#include "base/files/file_path.h"
#include "base/json/json_file_value_serializer.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/pref_names.h"
#include "chrome/installer/util/util_constants.h"

namespace installer {

namespace {

// Given a DictionaryValue containing a set of uninstall metrics,
// this builds a URL parameter list of all the contained metrics.
// Returns true if at least one uninstall metric was found in
// uninstall_metrics_dict, false otherwise.
bool BuildUninstallMetricsString(
    const base::DictionaryValue* uninstall_metrics_dict,
    base::string16* metrics) {
  DCHECK(NULL != metrics);
  bool has_values = false;

  for (base::DictionaryValue::Iterator iter(*uninstall_metrics_dict);
       !iter.IsAtEnd();
       iter.Advance()) {
    has_values = true;
    metrics->append(L"&");
    metrics->append(UTF8ToWide(iter.key()));
    metrics->append(L"=");

    std::string value;
    iter.value().GetAsString(&value);
    metrics->append(UTF8ToWide(value));
  }

  return has_values;
}

}  // namespace

bool ExtractUninstallMetrics(const base::DictionaryValue& root,
                             base::string16* uninstall_metrics_string) {
  // Make sure that the user wants us reporting metrics. If not, don't
  // add our uninstall metrics.
  bool metrics_reporting_enabled = false;
  if (!root.GetBoolean(prefs::kMetricsReportingEnabled,
                       &metrics_reporting_enabled) ||
      !metrics_reporting_enabled) {
    return false;
  }

  const base::DictionaryValue* uninstall_metrics_dict = NULL;
  if (!root.HasKey(installer::kUninstallMetricsName) ||
      !root.GetDictionary(installer::kUninstallMetricsName,
                          &uninstall_metrics_dict)) {
    return false;
  }

  if (!BuildUninstallMetricsString(uninstall_metrics_dict,
                                   uninstall_metrics_string)) {
    return false;
  }

  return true;
}

bool ExtractUninstallMetricsFromFile(const base::FilePath& file_path,
                                     base::string16* uninstall_metrics_string) {
  JSONFileValueSerializer json_serializer(file_path);

  std::string json_error_string;
  scoped_ptr<base::Value> root(json_serializer.Deserialize(NULL, NULL));
  if (!root.get())
    return false;

  // Preferences should always have a dictionary root.
  if (!root->IsType(base::Value::TYPE_DICTIONARY))
    return false;

  return ExtractUninstallMetrics(
      *static_cast<base::DictionaryValue*>(root.get()),
      uninstall_metrics_string);
}

}  // namespace installer
