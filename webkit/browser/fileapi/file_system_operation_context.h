// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BROWSER_FILEAPI_FILE_SYSTEM_OPERATION_CONTEXT_H_
#define WEBKIT_BROWSER_FILEAPI_FILE_SYSTEM_OPERATION_CONTEXT_H_

#include "base/files/file_path.h"
#include "base/supports_user_data.h"
#include "base/threading/thread_checker.h"
#include "webkit/browser/fileapi/task_runner_bound_observer_list.h"
#include "webkit/common/quota/quota_types.h"
#include "webkit/storage/webkit_storage_export.h"

namespace base {
class SequencedTaskRunner;
}

namespace fileapi {

class FileSystemContext;

// A context class which is carried around by FileSystemOperation and
// its delegated tasks. It is valid to reuse one context instance across
// multiple operations as far as those operations are supposed to share
// the same context (e.g. use the same task runner, share the quota etc).
// Note that the remaining quota bytes (allowed_bytes_growth) may be
// updated during the execution of write operations.
class WEBKIT_STORAGE_EXPORT_PRIVATE FileSystemOperationContext
    : public base::SupportsUserData {
 public:
  explicit FileSystemOperationContext(FileSystemContext* context);

  // Specifies |task_runner| which the operation is performed on.
  FileSystemOperationContext(FileSystemContext* context,
                             base::SequencedTaskRunner* task_runner);

  virtual ~FileSystemOperationContext();

  FileSystemContext* file_system_context() const {
    return file_system_context_;
  }

  // Updates the current remaining quota.
  // This can be called to update the remaining quota during the operation.
  void set_allowed_bytes_growth(const int64& allowed_bytes_growth) {
    allowed_bytes_growth_ = allowed_bytes_growth;
  }

  // Returns the current remaining quota.
  int64 allowed_bytes_growth() const { return allowed_bytes_growth_; }
  quota::QuotaLimitType quota_limit_type() const { return quota_limit_type_; }
  base::SequencedTaskRunner* task_runner() const { return task_runner_.get(); }
  const base::FilePath& root_path() const { return root_path_; }

  ChangeObserverList* change_observers() { return &change_observers_; }
  AccessObserverList* access_observers() { return &access_observers_; }
  UpdateObserverList* update_observers() { return &update_observers_; }

  // Following setters should be called only on the same thread as the
  // FileSystemOperationContext is created (i.e. are not supposed be updated
  // after the context's passed onto other task runners).
  void set_change_observers(const ChangeObserverList& list) {
    DCHECK(setter_thread_checker_.CalledOnValidThread());
    change_observers_ = list;
  }
  void set_access_observers(const AccessObserverList& list) {
    DCHECK(setter_thread_checker_.CalledOnValidThread());
    access_observers_ = list;
  }
  void set_update_observers(const UpdateObserverList& list) {
    DCHECK(setter_thread_checker_.CalledOnValidThread());
    update_observers_ = list;
  }
  void set_quota_limit_type(quota::QuotaLimitType limit_type) {
    DCHECK(setter_thread_checker_.CalledOnValidThread());
    quota_limit_type_ = limit_type;
  }
  void set_root_path(const base::FilePath& root_path) {
    DCHECK(setter_thread_checker_.CalledOnValidThread());
    root_path_ = root_path;
  }

  // Gets and sets value-type (or not-owned) variable as UserData.
  // (SetUserValue can be called only on the same thread as this context
  // is created as well as other setters.)
  template <typename T> T GetUserValue(const char* key) const {
    ValueAdapter<T>* v = static_cast<ValueAdapter<T>*>(GetUserData(key));
    return v ? v->value() : T();
  }
  template <typename T> void SetUserValue(const char* key, const T& value) {
    DCHECK(setter_thread_checker_.CalledOnValidThread());
    SetUserData(key, new ValueAdapter<T>(value));
  }

 private:
  // An adapter for setting a value-type (or not owned) data as UserData.
  template <typename T> class ValueAdapter
      : public base::SupportsUserData::Data {
   public:
    ValueAdapter(const T& value) : value_(value) {}
    const T& value() const { return value_; }
   private:
    T value_;
    DISALLOW_COPY_AND_ASSIGN(ValueAdapter);
  };

  FileSystemContext* file_system_context_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // The current remaining quota, used by ObfuscatedFileUtil.
  int64 allowed_bytes_growth_;

  // The current quota limit type, used by ObfuscatedFileUtil.
  quota::QuotaLimitType quota_limit_type_;

  // Observers attached to this context.
  AccessObserverList access_observers_;
  ChangeObserverList change_observers_;
  UpdateObserverList update_observers_;

  // Root path for the operation, used by LocalFileUtil.
  base::FilePath root_path_;

  // Used to check its setters are not called on arbitrary thread.
  base::ThreadChecker setter_thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemOperationContext);
};

}  // namespace fileapi

#endif  // WEBKIT_BROWSER_FILEAPI_FILE_SYSTEM_OPERATION_CONTEXT_H_
