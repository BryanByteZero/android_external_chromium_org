// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/in_process_webkit/indexed_db_quota_client.h"

#include <vector>

#include "base/file_util.h"
#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "content/browser/in_process_webkit/indexed_db_context_impl.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_util.h"
#include "webkit/database/database_util.h"

using content::BrowserThread;
using quota::QuotaClient;
using webkit_database::DatabaseUtil;

namespace {

quota::QuotaStatusCode DeleteOriginDataOnWebKitThread(
    IndexedDBContextImpl* context,
    const GURL& origin) {
  context->DeleteForOrigin(origin);
  return quota::kQuotaStatusOk;
}

int64 GetOriginUsageOnWebKitThread(
    IndexedDBContextImpl* context,
    const GURL& origin) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::WEBKIT_DEPRECATED));
  return context->GetOriginDiskUsage(origin);
}

}  // namespace

// Helper tasks ---------------------------------------------------------------

class IndexedDBQuotaClient::HelperTask : public quota::QuotaThreadTask {
 protected:
  HelperTask(
      IndexedDBQuotaClient* client,
      base::MessageLoopProxy* webkit_thread_message_loop)
      : QuotaThreadTask(client, webkit_thread_message_loop),
        client_(client), indexed_db_context_(client->indexed_db_context_) {
  }

  IndexedDBQuotaClient* client_;
  scoped_refptr<IndexedDBContextImpl> indexed_db_context_;

 protected:
  virtual ~HelperTask() {}
};

class IndexedDBQuotaClient::GetOriginsTaskBase : public HelperTask {
 protected:
  GetOriginsTaskBase(
      IndexedDBQuotaClient* client,
      base::MessageLoopProxy* webkit_thread_message_loop)
      : HelperTask(client, webkit_thread_message_loop) {
  }

  virtual bool ShouldAddOrigin(const GURL& origin) = 0;

  virtual void RunOnTargetThread() OVERRIDE {
    std::vector<GURL> origins =  indexed_db_context_->GetAllOrigins();
    for (std::vector<GURL>::const_iterator iter = origins.begin();
         iter != origins.end(); ++iter) {
      if (ShouldAddOrigin(*iter))
        origins_.insert(*iter);
    }
  }

  std::set<GURL> origins_;

 protected:
  virtual ~GetOriginsTaskBase() {}
};

class IndexedDBQuotaClient::GetAllOriginsTask : public GetOriginsTaskBase {
 public:
  GetAllOriginsTask(
      IndexedDBQuotaClient* client,
      base::MessageLoopProxy* webkit_thread_message_loop,
      quota::StorageType type)
      : GetOriginsTaskBase(client, webkit_thread_message_loop),
        type_(type) {
  }

 protected:
  virtual ~GetAllOriginsTask() {}

  virtual bool ShouldAddOrigin(const GURL& origin) OVERRIDE {
    return true;
  }

  virtual void Completed() OVERRIDE {
    client_->DidGetAllOrigins(origins_, type_);
  }

 private:
  quota::StorageType type_;
};

class IndexedDBQuotaClient::GetOriginsForHostTask : public GetOriginsTaskBase {
 public:
  GetOriginsForHostTask(
      IndexedDBQuotaClient* client,
      base::MessageLoopProxy* webkit_thread_message_loop,
      const std::string& host,
      quota::StorageType type)
      : GetOriginsTaskBase(client, webkit_thread_message_loop),
        host_(host),
        type_(type) {
  }

 private:
  virtual ~GetOriginsForHostTask() {}

  virtual bool ShouldAddOrigin(const GURL& origin) OVERRIDE {
    return host_ == net::GetHostOrSpecFromURL(origin);
  }

  virtual void Completed() OVERRIDE {
    client_->DidGetOriginsForHost(host_, origins_, type_);
  }

  std::string host_;
  quota::StorageType type_;
};

// IndexedDBQuotaClient --------------------------------------------------------

IndexedDBQuotaClient::IndexedDBQuotaClient(
    base::MessageLoopProxy* webkit_thread_message_loop,
    IndexedDBContextImpl* indexed_db_context)
    : webkit_thread_message_loop_(webkit_thread_message_loop),
      indexed_db_context_(indexed_db_context) {
}

IndexedDBQuotaClient::~IndexedDBQuotaClient() {
}

QuotaClient::ID IndexedDBQuotaClient::id() const {
  return kIndexedDatabase;
}

void IndexedDBQuotaClient::OnQuotaManagerDestroyed() {
  delete this;
}

void IndexedDBQuotaClient::GetOriginUsage(
    const GURL& origin_url,
    quota::StorageType type,
    const GetUsageCallback& callback) {
  DCHECK(!callback.is_null());
  DCHECK(indexed_db_context_.get());

  // IndexedDB is in the temp namespace for now.
  if (type != quota::kStorageTypeTemporary) {
    callback.Run(0);
    return;
  }

  base::PostTaskAndReplyWithResult(
      webkit_thread_message_loop_,
      FROM_HERE,
      base::Bind(&GetOriginUsageOnWebKitThread,
                 indexed_db_context_,
                 origin_url),
      callback);
}

void IndexedDBQuotaClient::GetOriginsForType(
    quota::StorageType type,
    const GetOriginsCallback& callback) {
  DCHECK(!callback.is_null());
  DCHECK(indexed_db_context_.get());

  // All databases are in the temp namespace for now.
  if (type != quota::kStorageTypeTemporary) {
    callback.Run(std::set<GURL>(), type);
    return;
  }

  if (origins_for_type_callbacks_.Add(callback)) {
    scoped_refptr<GetAllOriginsTask> task(
        new GetAllOriginsTask(this, webkit_thread_message_loop_, type));
    task->Start();
  }
}

void IndexedDBQuotaClient::GetOriginsForHost(
    quota::StorageType type,
    const std::string& host,
    const GetOriginsCallback& callback) {
  DCHECK(!callback.is_null());
  DCHECK(indexed_db_context_.get());

  // All databases are in the temp namespace for now.
  if (type != quota::kStorageTypeTemporary) {
    callback.Run(std::set<GURL>(), type);
    return;
  }

  if (origins_for_host_callbacks_.Add(host, callback)) {
    scoped_refptr<GetOriginsForHostTask> task(
        new GetOriginsForHostTask(
            this, webkit_thread_message_loop_, host, type));
    task->Start();
  }
}

void IndexedDBQuotaClient::DeleteOriginData(
    const GURL& origin,
    quota::StorageType type,
    const DeletionCallback& callback) {
  if (type != quota::kStorageTypeTemporary) {
    callback.Run(quota::kQuotaErrorNotSupported);
    return;
  }

  base::PostTaskAndReplyWithResult(
      webkit_thread_message_loop_,
      FROM_HERE,
      base::Bind(&DeleteOriginDataOnWebKitThread,
                 indexed_db_context_,
                 origin),
      callback);
}

void IndexedDBQuotaClient::DidGetAllOrigins(const std::set<GURL>& origins,
                                            quota::StorageType type) {
  DCHECK(origins_for_type_callbacks_.HasCallbacks());
  origins_for_type_callbacks_.Run(origins, type);
}

void IndexedDBQuotaClient::DidGetOriginsForHost(
    const std::string& host, const std::set<GURL>& origins,
    quota::StorageType type) {
  DCHECK(origins_for_host_callbacks_.HasCallbacks(host));
  origins_for_host_callbacks_.Run(host, origins, type);
}
