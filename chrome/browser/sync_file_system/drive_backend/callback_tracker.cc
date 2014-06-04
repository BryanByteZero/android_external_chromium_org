// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/drive_backend/callback_tracker.h"

#include <algorithm>

namespace sync_file_system {
namespace drive_backend {

CallbackTracker::CallbackTracker() {
}

CallbackTracker::~CallbackTracker() {
  AbortAll();
}

void CallbackTracker::AbortAll() {
  AbortClosureByHelper helpers;
  std::swap(helpers, helpers_);
  for (AbortClosureByHelper::iterator itr = helpers.begin();
       itr != helpers.end(); ++itr) {
    delete itr->first;
    itr->second.Run();
  }
}

scoped_ptr<internal::AbortHelper> CallbackTracker::PassAbortHelper(
    internal::AbortHelper* helper) {
  if (helpers_.erase(helper) == 1)
    return scoped_ptr<internal::AbortHelper>(helper);
  return scoped_ptr<internal::AbortHelper>();
}

}  // namespace drive_backend
}  // namespace sync_file_system
