// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/internal_api/js_sync_manager_observer.h"

#include <cstddef>

#include "base/location.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "sync/internal_api/public/base/model_type.h"
#include "sync/internal_api/public/change_record.h"
#include "sync/internal_api/public/sessions/sync_session_snapshot.h"
#include "sync/internal_api/public/util/sync_string_conversions.h"
#include "sync/js/js_arg_list.h"
#include "sync/js/js_event_details.h"
#include "sync/js/js_event_handler.h"

namespace syncer {

JsSyncManagerObserver::JsSyncManagerObserver() {}

JsSyncManagerObserver::~JsSyncManagerObserver() {}

void JsSyncManagerObserver::SetJsEventHandler(
    const WeakHandle<JsEventHandler>& event_handler) {
  event_handler_ = event_handler;
}

void JsSyncManagerObserver::OnSyncCycleCompleted(
    const sessions::SyncSessionSnapshot& snapshot) {
  if (!event_handler_.IsInitialized()) {
    return;
  }
  base::DictionaryValue details;
  details.Set("snapshot", snapshot.ToValue());
  HandleJsEvent(FROM_HERE, "onSyncCycleCompleted", JsEventDetails(&details));
}

void JsSyncManagerObserver::OnConnectionStatusChange(ConnectionStatus status) {
  if (!event_handler_.IsInitialized()) {
    return;
  }
  base::DictionaryValue details;
  details.SetString("status", ConnectionStatusToString(status));
  HandleJsEvent(FROM_HERE,
                "onConnectionStatusChange", JsEventDetails(&details));
}

void JsSyncManagerObserver::OnActionableError(
    const SyncProtocolError& sync_error) {
  if (!event_handler_.IsInitialized()) {
    return;
  }
  base::DictionaryValue details;
  details.Set("syncError",  sync_error.ToValue());
  HandleJsEvent(FROM_HERE, "onActionableError",
                JsEventDetails(&details));
}

void JsSyncManagerObserver::OnMigrationRequested(ModelTypeSet types) { }

void JsSyncManagerObserver::OnInitializationComplete(
    const WeakHandle<JsBackend>& js_backend,
    const WeakHandle<DataTypeDebugInfoListener>& debug_info_listener,
    bool success, syncer::ModelTypeSet restored_types) {
  if (!event_handler_.IsInitialized()) {
    return;
  }
  // Ignore the |js_backend| argument; it's not really convertible to
  // JSON anyway.

  base::DictionaryValue details;
  details.Set("restoredTypes", ModelTypeSetToValue(restored_types));

  HandleJsEvent(FROM_HERE,
                "onInitializationComplete",
                JsEventDetails(&details));
}

void JsSyncManagerObserver::HandleJsEvent(
    const tracked_objects::Location& from_here,
    const std::string& name, const JsEventDetails& details) {
  if (!event_handler_.IsInitialized()) {
    NOTREACHED();
    return;
  }
  event_handler_.Call(from_here,
                      &JsEventHandler::HandleJsEvent, name, details);
}

}  // namespace syncer
