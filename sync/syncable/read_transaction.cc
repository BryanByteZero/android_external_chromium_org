// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/syncable/read_transaction.h"

namespace syncable {

ReadTransaction::ReadTransaction(const tracked_objects::Location& location,
                                 Directory* directory)
    : BaseTransaction(location, "ReadTransaction", INVALID, directory) {
  Lock();
}

ReadTransaction::~ReadTransaction() {
  HandleUnrecoverableErrorIfSet();
  Unlock();
}

} // namespace syncable
