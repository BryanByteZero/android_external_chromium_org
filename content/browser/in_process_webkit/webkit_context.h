// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_IN_PROCESS_WEBKIT_WEBKIT_CONTEXT_H_
#define CONTENT_BROWSER_IN_PROCESS_WEBKIT_WEBKIT_CONTEXT_H_
#pragma once

#include <vector>

#include "base/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/content_export.h"

class DOMStorageContextImpl;
class IndexedDBContextImpl;

namespace base {
class MessageLoopProxy;
}

namespace quota {
class QuotaManagerProxy;
class SpecialStoragePolicy;
}

// There's one WebKitContext per browser context.  Various DispatcherHost
// classes have a pointer to the Context to store shared state.  Unfortunately,
// this class has become a bit of a dumping ground for calls made on the UI
// thread that need to be proxied over to the WebKit thread.
//
// This class is created on the UI thread and accessed on the UI, IO, and WebKit
// threads.
class CONTENT_EXPORT WebKitContext
    : public base::RefCountedThreadSafe<WebKitContext> {
 public:
  WebKitContext(bool is_incognito, const FilePath& data_path,
                quota::SpecialStoragePolicy* special_storage_policy,
                quota::QuotaManagerProxy* quota_manager_proxy,
                base::MessageLoopProxy* webkit_thread_loop);

  const FilePath& data_path() const { return data_path_; }
  bool is_incognito() const { return is_incognito_; }

  DOMStorageContextImpl* dom_storage_context() { return dom_storage_context_; }
  IndexedDBContextImpl* indexed_db_context() { return indexed_db_context_; }

 private:
  friend class base::RefCountedThreadSafe<WebKitContext>;
  virtual ~WebKitContext();

  // Copies of browser context data that can be accessed on any thread.
  const FilePath data_path_;
  const bool is_incognito_;

  scoped_refptr<DOMStorageContextImpl> dom_storage_context_;
  scoped_refptr<IndexedDBContextImpl> indexed_db_context_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(WebKitContext);
};

#endif  // CONTENT_BROWSER_IN_PROCESS_WEBKIT_WEBKIT_CONTEXT_H_
