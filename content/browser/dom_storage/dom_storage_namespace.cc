// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/dom_storage_namespace.h"

#include <set>
#include <utility>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "content/browser/dom_storage/dom_storage_area.h"
#include "content/browser/dom_storage/dom_storage_task_runner.h"
#include "content/browser/dom_storage/session_storage_database.h"
#include "content/common/child_process_host_impl.h"
#include "content/common/dom_storage/dom_storage_types.h"

namespace content {

namespace {

static const unsigned int kMaxTransactionLogEntries = 8 * 1024;

}  // namespace

DOMStorageNamespace::DOMStorageNamespace(
    const base::FilePath& directory,
    DOMStorageTaskRunner* task_runner)
    : namespace_id_(kLocalStorageNamespaceId),
      directory_(directory),
      task_runner_(task_runner) {
}

DOMStorageNamespace::DOMStorageNamespace(
    int64 namespace_id,
    const std::string& persistent_namespace_id,
    SessionStorageDatabase* session_storage_database,
    DOMStorageTaskRunner* task_runner)
    : namespace_id_(namespace_id),
      persistent_namespace_id_(persistent_namespace_id),
      task_runner_(task_runner),
      session_storage_database_(session_storage_database) {
  DCHECK_NE(kLocalStorageNamespaceId, namespace_id);
}

DOMStorageNamespace::~DOMStorageNamespace() {
  STLDeleteValues(&transactions_);
}

DOMStorageArea* DOMStorageNamespace::OpenStorageArea(const GURL& origin) {
  if (AreaHolder* holder = GetAreaHolder(origin)) {
    ++(holder->open_count_);
    return holder->area_.get();
  }
  DOMStorageArea* area;
  if (namespace_id_ == kLocalStorageNamespaceId) {
    area = new DOMStorageArea(origin, directory_, task_runner_.get());
  } else {
    area = new DOMStorageArea(
        namespace_id_, persistent_namespace_id_, origin,
        session_storage_database_.get(), task_runner_.get());
  }
  areas_[origin] = AreaHolder(area, 1);
  return area;
}

void DOMStorageNamespace::CloseStorageArea(DOMStorageArea* area) {
  AreaHolder* holder = GetAreaHolder(area->origin());
  DCHECK(holder);
  DCHECK_EQ(holder->area_.get(), area);
  --(holder->open_count_);
  // TODO(michaeln): Clean up areas that aren't needed in memory anymore.
  // The in-process-webkit based impl didn't do this either, but would be nice.
}

DOMStorageArea* DOMStorageNamespace::GetOpenStorageArea(const GURL& origin) {
  AreaHolder* holder = GetAreaHolder(origin);
  if (holder && holder->open_count_)
    return holder->area_.get();
  return NULL;
}

DOMStorageNamespace* DOMStorageNamespace::Clone(
    int64 clone_namespace_id,
    const std::string& clone_persistent_namespace_id) {
  DCHECK_NE(kLocalStorageNamespaceId, namespace_id_);
  DCHECK_NE(kLocalStorageNamespaceId, clone_namespace_id);
  DOMStorageNamespace* clone = new DOMStorageNamespace(
      clone_namespace_id, clone_persistent_namespace_id,
      session_storage_database_.get(), task_runner_.get());
  AreaMap::const_iterator it = areas_.begin();
  // Clone the in-memory structures.
  for (; it != areas_.end(); ++it) {
    DOMStorageArea* area = it->second.area_->ShallowCopy(
        clone_namespace_id, clone_persistent_namespace_id);
    clone->areas_[it->first] = AreaHolder(area, 0);
  }
  // And clone the on-disk structures, too.
  if (session_storage_database_.get()) {
    task_runner_->PostShutdownBlockingTask(
        FROM_HERE,
        DOMStorageTaskRunner::COMMIT_SEQUENCE,
        base::Bind(base::IgnoreResult(&SessionStorageDatabase::CloneNamespace),
                   session_storage_database_.get(), persistent_namespace_id_,
                   clone_persistent_namespace_id));
  }
  return clone;
}

void DOMStorageNamespace::DeleteLocalStorageOrigin(const GURL& origin) {
  DCHECK(!session_storage_database_.get());
  AreaHolder* holder = GetAreaHolder(origin);
  if (holder) {
    holder->area_->DeleteOrigin();
    return;
  }
  if (!directory_.empty()) {
    scoped_refptr<DOMStorageArea> area =
        new DOMStorageArea(origin, directory_, task_runner_.get());
    area->DeleteOrigin();
  }
}

void DOMStorageNamespace::DeleteSessionStorageOrigin(const GURL& origin) {
  DOMStorageArea* area = OpenStorageArea(origin);
  area->FastClear();
  CloseStorageArea(area);
}

void DOMStorageNamespace::PurgeMemory(PurgeOption option) {
  if (directory_.empty())
    return;  // We can't purge w/o backing on disk.
  AreaMap::iterator it = areas_.begin();
  while (it != areas_.end()) {
    // Leave it alone if changes are pending
    if (it->second.area_->HasUncommittedChanges()) {
      ++it;
      continue;
    }

    // If not in use, we can shut it down and remove
    // it from our collection entirely.
    if (it->second.open_count_ == 0) {
      it->second.area_->Shutdown();
      areas_.erase(it++);
      continue;
    }

    if (option == PURGE_AGGRESSIVE) {
      // If aggressive is true, we clear caches and such
      // for opened areas.
      it->second.area_->PurgeMemory();
    }

    ++it;
  }
}

void DOMStorageNamespace::Shutdown() {
  AreaMap::const_iterator it = areas_.begin();
  for (; it != areas_.end(); ++it)
    it->second.area_->Shutdown();
}

unsigned int DOMStorageNamespace::CountInMemoryAreas() const {
  unsigned int area_count = 0;
  for (AreaMap::const_iterator it = areas_.begin(); it != areas_.end(); ++it) {
    if (it->second.area_->IsLoadedInMemory())
      ++area_count;
  }
  return area_count;
}

DOMStorageNamespace::AreaHolder*
DOMStorageNamespace::GetAreaHolder(const GURL& origin) {
  AreaMap::iterator found = areas_.find(origin);
  if (found == areas_.end())
    return NULL;
  return &(found->second);
}

void DOMStorageNamespace::AddTransactionLogProcessId(int process_id) {
  DCHECK(process_id != ChildProcessHostImpl::kInvalidChildProcessId);
  DCHECK(transactions_.count(process_id) == 0);
  TransactionData* transaction_data = new TransactionData;
  transactions_[process_id] = transaction_data;
}

void DOMStorageNamespace::RemoveTransactionLogProcessId(int process_id) {
  DCHECK(process_id != ChildProcessHostImpl::kInvalidChildProcessId);
  DCHECK(transactions_.count(process_id) == 1);
  delete transactions_[process_id];
  transactions_.erase(process_id);
}

SessionStorageNamespace::MergeResult DOMStorageNamespace::CanMerge(
    int process_id,
    DOMStorageNamespace* other) {
  if (transactions_.count(process_id) < 1)
    return SessionStorageNamespace::MERGE_RESULT_NOT_LOGGING;
  TransactionData* data = transactions_[process_id];
  if (data->max_log_size_exceeded)
    return SessionStorageNamespace::MERGE_RESULT_TOO_MANY_TRANSACTIONS;
  if (data->log.size() < 1)
    return SessionStorageNamespace::MERGE_RESULT_NO_TRANSACTIONS;

  // skip_areas and skip_keys store areas and (area, key) pairs, respectively,
  // that have already been handled previously. Any further modifications to
  // them will not change the result of the hypothetical merge.
  std::set<GURL> skip_areas;
  typedef std::pair<GURL, base::string16> OriginKey;
  std::set<OriginKey> skip_keys;
  // Indicates whether we could still merge the namespaces preserving all
  // individual transactions.
  for (unsigned int i = 0; i < data->log.size(); i++) {
    TransactionRecord& transaction = data->log[i];
    if (transaction.transaction_type == TRANSACTION_CLEAR) {
      skip_areas.insert(transaction.origin);
      continue;
    }
    if (skip_areas.find(transaction.origin) != skip_areas.end())
      continue;
    if (skip_keys.find(OriginKey(transaction.origin, transaction.key))
        != skip_keys.end()) {
      continue;
    }
    if (transaction.transaction_type == TRANSACTION_REMOVE ||
        transaction.transaction_type == TRANSACTION_WRITE) {
      skip_keys.insert(OriginKey(transaction.origin, transaction.key));
      continue;
    }
    if (transaction.transaction_type == TRANSACTION_READ) {
      DOMStorageArea* area = other->OpenStorageArea(transaction.origin);
      base::NullableString16 other_value = area->GetItem(transaction.key);
      other->CloseStorageArea(area);
      if (transaction.value != other_value)
        return SessionStorageNamespace::MERGE_RESULT_NOT_MERGEABLE;
      continue;
    }
    NOTREACHED();
  }
  return SessionStorageNamespace::MERGE_RESULT_MERGEABLE;
}

bool DOMStorageNamespace::IsLoggingRenderer(int process_id) {
  DCHECK(process_id != ChildProcessHostImpl::kInvalidChildProcessId);
  if (transactions_.count(process_id) < 1)
    return false;
  return !transactions_[process_id]->max_log_size_exceeded;
}

void DOMStorageNamespace::AddTransaction(
    int process_id, const TransactionRecord& transaction) {
  if (!IsLoggingRenderer(process_id))
    return;
  TransactionData* transaction_data = transactions_[process_id];
  DCHECK(transaction_data);
  if (transaction_data->max_log_size_exceeded)
    return;
  transaction_data->log.push_back(transaction);
  if (transaction_data->log.size() > kMaxTransactionLogEntries) {
    transaction_data->max_log_size_exceeded = true;
    transaction_data->log.clear();
  }
}

DOMStorageNamespace::TransactionData::TransactionData()
    : max_log_size_exceeded(false) {
}

DOMStorageNamespace::TransactionData::~TransactionData() {
}

DOMStorageNamespace::TransactionRecord::TransactionRecord() {
}

DOMStorageNamespace::TransactionRecord::~TransactionRecord() {
}

// AreaHolder

DOMStorageNamespace::AreaHolder::AreaHolder()
    : open_count_(0) {
}

DOMStorageNamespace::AreaHolder::AreaHolder(
    DOMStorageArea* area, int count)
    : area_(area), open_count_(count) {
}

DOMStorageNamespace::AreaHolder::~AreaHolder() {
}

}  // namespace content
