// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_file_error_injector.h"

#include <vector>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_file_impl.h"
#include "content/browser/download/download_file_manager.h"
#include "content/browser/download/download_interrupt_reasons_impl.h"
#include "content/browser/power_save_blocker.h"
#include "content/browser/renderer_host/resource_dispatcher_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_id.h"
#include "googleurl/src/gurl.h"

namespace content {
class ByteStreamReader;
}

namespace {

DownloadFileManager* GetDownloadFileManager() {
  content::ResourceDispatcherHostImpl* rdh =
      content::ResourceDispatcherHostImpl::Get();
  DCHECK(rdh != NULL);
  return rdh->download_file_manager();
}

// A class that performs file operations and injects errors.
class DownloadFileWithErrors: public DownloadFileImpl {
 public:
  typedef base::Callback<void(const GURL& url, content::DownloadId id)>
      ConstructionCallback;
  typedef base::Callback<void(const GURL& url)> DestructionCallback;

  DownloadFileWithErrors(
      scoped_ptr<DownloadCreateInfo> info,
      scoped_ptr<content::ByteStreamReader> stream,
      scoped_ptr<DownloadRequestHandleInterface> request_handle,
      content::DownloadManager* download_manager,
      bool calculate_hash,
      const net::BoundNetLog& bound_net_log,
      content::DownloadId download_id,
      const GURL& source_url,
      const content::TestFileErrorInjector::FileErrorInfo& error_info,
      const ConstructionCallback& ctor_callback,
      const DestructionCallback& dtor_callback);

  ~DownloadFileWithErrors();

  // DownloadFile interface.
  virtual content::DownloadInterruptReason Initialize() OVERRIDE;
  virtual content::DownloadInterruptReason AppendDataToFile(
      const char* data, size_t data_len) OVERRIDE;
  virtual void Rename(const FilePath& full_path,
                      bool overwrite_existing_file,
                      const RenameCompletionCallback& callback) OVERRIDE;

 private:
  // Error generating helper.
  content::DownloadInterruptReason ShouldReturnError(
      content::TestFileErrorInjector::FileOperationCode code,
      content::DownloadInterruptReason original_error);

  // Determine whether to overwrite an operation with the given code
  // with a substitute error; if returns true, |*original_error| is
  // written with the error to use for overwriting.
  // NOTE: This routine changes state; specifically, it increases the
  // operations counts for the specified code.  It should only be called
  // once per operation.
  bool OverwriteError(
    content::TestFileErrorInjector::FileOperationCode code,
    content::DownloadInterruptReason* output_error);

  // Source URL for the file being downloaded.
  GURL source_url_;

  // Our injected error.  Only one per file.
  content::TestFileErrorInjector::FileErrorInfo error_info_;

  // Count per operation.  0-based.
  std::map<content::TestFileErrorInjector::FileOperationCode, int>
      operation_counter_;

  // Callback for destruction.
  DestructionCallback destruction_callback_;
};

static void RenameErrorCallback(
    const content::DownloadFile::RenameCompletionCallback original_callback,
    content::DownloadInterruptReason overwrite_error,
    content::DownloadInterruptReason original_error,
    const FilePath& path_result) {
  original_callback.Run(
      overwrite_error,
      overwrite_error == content::DOWNLOAD_INTERRUPT_REASON_NONE ?
      path_result : FilePath());
}


DownloadFileWithErrors::DownloadFileWithErrors(
    scoped_ptr<DownloadCreateInfo> info,
    scoped_ptr<content::ByteStreamReader> stream,
    scoped_ptr<DownloadRequestHandleInterface> request_handle,
    content::DownloadManager* download_manager,
    bool calculate_hash,
    const net::BoundNetLog& bound_net_log,
    content::DownloadId download_id,
    const GURL& source_url,
    const content::TestFileErrorInjector::FileErrorInfo& error_info,
    const ConstructionCallback& ctor_callback,
    const DestructionCallback& dtor_callback)
    : DownloadFileImpl(info.Pass(),
                       stream.Pass(),
                       request_handle.Pass(),
                       download_manager,
                       calculate_hash,
                       scoped_ptr<content::PowerSaveBlocker>(),
                       bound_net_log),
      source_url_(source_url),
      error_info_(error_info),
      destruction_callback_(dtor_callback) {
  ctor_callback.Run(source_url_, download_id);
}

DownloadFileWithErrors::~DownloadFileWithErrors() {
  destruction_callback_.Run(source_url_);
}

content::DownloadInterruptReason DownloadFileWithErrors::Initialize() {
  return ShouldReturnError(
      content::TestFileErrorInjector::FILE_OPERATION_INITIALIZE,
      DownloadFileImpl::Initialize());
}

content::DownloadInterruptReason DownloadFileWithErrors::AppendDataToFile(
    const char* data, size_t data_len) {
  return ShouldReturnError(
      content::TestFileErrorInjector::FILE_OPERATION_WRITE,
      DownloadFileImpl::AppendDataToFile(data, data_len));
}

void DownloadFileWithErrors::Rename(
    const FilePath& full_path,
    bool overwrite_existing_file,
    const RenameCompletionCallback& callback) {
  content::DownloadInterruptReason error_to_return =
      content::DOWNLOAD_INTERRUPT_REASON_NONE;
  RenameCompletionCallback callback_to_use = callback;

  // Replace callback if the error needs to be overwritten.
  if (OverwriteError(
          content::TestFileErrorInjector::FILE_OPERATION_RENAME,
          &error_to_return)) {
    callback_to_use = base::Bind(&RenameErrorCallback, callback,
                                 error_to_return);
  }

  DownloadFileImpl::Rename(full_path, overwrite_existing_file, callback_to_use);
}

bool DownloadFileWithErrors::OverwriteError(
    content::TestFileErrorInjector::FileOperationCode code,
    content::DownloadInterruptReason* output_error) {
  int counter = operation_counter_[code]++;

  if (code != error_info_.code)
    return false;

  if (counter != error_info_.operation_instance)
    return false;

  *output_error = error_info_.error;
  return true;
}

content::DownloadInterruptReason DownloadFileWithErrors::ShouldReturnError(
    content::TestFileErrorInjector::FileOperationCode code,
    content::DownloadInterruptReason original_error) {
  content::DownloadInterruptReason output_error = original_error;
  OverwriteError(code, &output_error);
  return output_error;
}

}  // namespace

namespace content {

// A factory for constructing DownloadFiles that inject errors.
class DownloadFileWithErrorsFactory : public content::DownloadFileFactory {
 public:

  DownloadFileWithErrorsFactory(
      const DownloadFileWithErrors::ConstructionCallback& ctor_callback,
      const DownloadFileWithErrors::DestructionCallback& dtor_callback);
  virtual ~DownloadFileWithErrorsFactory();

  // DownloadFileFactory interface.
  virtual DownloadFile* CreateFile(
      scoped_ptr<DownloadCreateInfo> info,
      scoped_ptr<content::ByteStreamReader> stream,
      content::DownloadManager* download_manager,
      bool calculate_hash,
      const net::BoundNetLog& bound_net_log);

  bool AddError(
      const TestFileErrorInjector::FileErrorInfo& error_info);

  void ClearErrors();

 private:
  // Our injected error list, mapped by URL.  One per file.
   TestFileErrorInjector::ErrorMap injected_errors_;

  // Callback for creation and destruction.
  DownloadFileWithErrors::ConstructionCallback construction_callback_;
  DownloadFileWithErrors::DestructionCallback destruction_callback_;
};

DownloadFileWithErrorsFactory::DownloadFileWithErrorsFactory(
    const DownloadFileWithErrors::ConstructionCallback& ctor_callback,
    const DownloadFileWithErrors::DestructionCallback& dtor_callback)
        : construction_callback_(ctor_callback),
          destruction_callback_(dtor_callback) {
}

DownloadFileWithErrorsFactory::~DownloadFileWithErrorsFactory() {
}

content::DownloadFile* DownloadFileWithErrorsFactory::CreateFile(
    scoped_ptr<DownloadCreateInfo> info,
    scoped_ptr<content::ByteStreamReader> stream,
    content::DownloadManager* download_manager,
    bool calculate_hash,
    const net::BoundNetLog& bound_net_log) {
  GURL url = info->url();

  if (injected_errors_.find(url.spec()) == injected_errors_.end()) {
    // Have to create entry, because FileErrorInfo is not a POD type.
    TestFileErrorInjector::FileErrorInfo err_info = {
      url.spec(),
      TestFileErrorInjector::FILE_OPERATION_INITIALIZE,
      -1,
      content::DOWNLOAD_INTERRUPT_REASON_NONE
    };
    injected_errors_[url.spec()] = err_info;
  }

  scoped_ptr<DownloadRequestHandleInterface> request_handle(
      new DownloadRequestHandle(info->request_handle));
  DownloadId download_id(info->download_id);

  return new DownloadFileWithErrors(
      info.Pass(),
      stream.Pass(),
      request_handle.Pass(),
      download_manager,
      calculate_hash,
      bound_net_log,
      download_id,
      url,
      injected_errors_[url.spec()],
      construction_callback_,
      destruction_callback_);
}

bool DownloadFileWithErrorsFactory::AddError(
    const TestFileErrorInjector::FileErrorInfo& error_info) {
  // Creates an empty entry if necessary.  Duplicate entries overwrite.
  injected_errors_[error_info.url] = error_info;

  return true;
}

void DownloadFileWithErrorsFactory::ClearErrors() {
  injected_errors_.clear();
}

TestFileErrorInjector::TestFileErrorInjector()
    : created_factory_(NULL) {
  // Record the value of the pointer, for later validation.
  created_factory_ =
      new DownloadFileWithErrorsFactory(
          base::Bind(&TestFileErrorInjector::
                         RecordDownloadFileConstruction,
                     this),
          base::Bind(&TestFileErrorInjector::
                         RecordDownloadFileDestruction,
                     this));

  // We will transfer ownership of the factory to the download file manager.
  scoped_ptr<DownloadFileWithErrorsFactory> download_file_factory(
      created_factory_);

  content::BrowserThread::PostTask(
      content::BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&TestFileErrorInjector::AddFactory,
                 this,
                 base::Passed(&download_file_factory)));
}

TestFileErrorInjector::~TestFileErrorInjector() {
}

void TestFileErrorInjector::AddFactory(
    scoped_ptr<DownloadFileWithErrorsFactory> factory) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::FILE));

  DownloadFileManager* download_file_manager = GetDownloadFileManager();
  DCHECK(download_file_manager);

  // Convert to base class pointer, for GCC.
  scoped_ptr<content::DownloadFileFactory> plain_factory(
      factory.release());

  download_file_manager->SetFileFactoryForTesting(plain_factory.Pass());
}

bool TestFileErrorInjector::AddError(const FileErrorInfo& error_info) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK_LE(0, error_info.operation_instance);
  DCHECK(injected_errors_.find(error_info.url) == injected_errors_.end());

  // Creates an empty entry if necessary.
  injected_errors_[error_info.url] = error_info;

  return true;
}

void TestFileErrorInjector::ClearErrors() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  injected_errors_.clear();
}

bool TestFileErrorInjector::InjectErrors() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  ClearFoundFiles();

  content::BrowserThread::PostTask(
      content::BrowserThread::FILE,
      FROM_HERE,
      base::Bind(&TestFileErrorInjector::InjectErrorsOnFileThread,
                 this,
                 injected_errors_,
                 created_factory_));

  return true;
}

void TestFileErrorInjector::InjectErrorsOnFileThread(
    ErrorMap map, DownloadFileWithErrorsFactory* factory) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::FILE));

  // Validate that our factory is in use.
  DownloadFileManager* download_file_manager = GetDownloadFileManager();
  DCHECK(download_file_manager);

  content::DownloadFileFactory* file_factory =
      download_file_manager->GetFileFactoryForTesting();

  // Validate that we still have the same factory.
  DCHECK_EQ(static_cast<content::DownloadFileFactory*>(factory),
            file_factory);

  // We want to replace all existing injection errors.
  factory->ClearErrors();

  for (ErrorMap::const_iterator it = map.begin(); it != map.end(); ++it)
    factory->AddError(it->second);
}

size_t TestFileErrorInjector::CurrentFileCount() const {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  return files_.size();
}

size_t TestFileErrorInjector::TotalFileCount() const {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  return found_files_.size();
}


bool TestFileErrorInjector::HadFile(const GURL& url) const {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  return (found_files_.find(url) != found_files_.end());
}

const content::DownloadId TestFileErrorInjector::GetId(
    const GURL& url) const {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  FileMap::const_iterator it = found_files_.find(url);
  if (it == found_files_.end())
    return content::DownloadId::Invalid();

  return it->second;
}

void TestFileErrorInjector::ClearFoundFiles() {
  found_files_.clear();
}

void TestFileErrorInjector::DownloadFileCreated(GURL url,
                                                    content::DownloadId id) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(files_.find(url) == files_.end());

  files_[url] = id;
  found_files_[url] = id;
}

void TestFileErrorInjector::DestroyingDownloadFile(GURL url) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(files_.find(url) != files_.end());

  files_.erase(url);
}

void TestFileErrorInjector::RecordDownloadFileConstruction(
    const GURL& url, content::DownloadId id) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&TestFileErrorInjector::DownloadFileCreated,
                 this,
                 url,
                 id));
}

void TestFileErrorInjector::RecordDownloadFileDestruction(const GURL& url) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&TestFileErrorInjector::DestroyingDownloadFile,
                 this,
                 url));
}

// static
scoped_refptr<TestFileErrorInjector> TestFileErrorInjector::Create() {
  static bool visited = false;
  DCHECK(!visited);  // Only allowed to be called once.
  visited = true;

  scoped_refptr<TestFileErrorInjector> single_injector(
      new TestFileErrorInjector);

  return single_injector;
}

// static
std::string TestFileErrorInjector::DebugString(FileOperationCode code) {
  switch (code) {
    case FILE_OPERATION_INITIALIZE:
      return "INITIALIZE";
    case FILE_OPERATION_WRITE:
      return "WRITE";
    case FILE_OPERATION_RENAME:
      return "RENAME";
    default:
      break;
  }

  return "Unknown";
}

}  // namespace content
