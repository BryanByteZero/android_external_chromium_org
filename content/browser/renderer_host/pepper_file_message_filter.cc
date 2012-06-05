// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper_file_message_filter.h"

#include "base/callback.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/platform_file.h"
#include "base/process_util.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_constants.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/proxy/pepper_file_messages.h"
#include "ppapi/shared_impl/file_path.h"

#if defined(OS_POSIX)
#include "base/file_descriptor_posix.h"
#endif

using content::BrowserThread;

// Used to check if the renderer has permission for the requested operation.
// TODO(viettrungluu): Verify these. They don't necessarily quite make sense,
// but it seems to be approximately what the file system code does.
const int kReadPermissions = base::PLATFORM_FILE_OPEN |
                             base::PLATFORM_FILE_READ |
                             base::PLATFORM_FILE_EXCLUSIVE_READ;
const int kWritePermissions = base::PLATFORM_FILE_OPEN |
                              base::PLATFORM_FILE_CREATE |
                              base::PLATFORM_FILE_CREATE_ALWAYS |
                              base::PLATFORM_FILE_OPEN_TRUNCATED |
                              base::PLATFORM_FILE_WRITE |
                              base::PLATFORM_FILE_EXCLUSIVE_WRITE |
                              base::PLATFORM_FILE_WRITE_ATTRIBUTES;

PepperFileMessageFilter::PepperFileMessageFilter(int child_id)
        : child_id_(child_id),
          channel_(NULL) {
}

void PepperFileMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message,
    BrowserThread::ID* thread) {
  if (IPC_MESSAGE_CLASS(message) == PepperFileMsgStart)
    *thread = BrowserThread::FILE;
}

bool PepperFileMessageFilter::OnMessageReceived(const IPC::Message& message,
                                                bool* message_was_ok) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_EX(PepperFileMessageFilter, message, *message_was_ok)
    IPC_MESSAGE_HANDLER(PepperFileMsg_OpenFile, OnOpenFile)
    IPC_MESSAGE_HANDLER(PepperFileMsg_RenameFile, OnRenameFile)
    IPC_MESSAGE_HANDLER(PepperFileMsg_DeleteFileOrDir, OnDeleteFileOrDir)
    IPC_MESSAGE_HANDLER(PepperFileMsg_CreateDir, OnCreateDir)
    IPC_MESSAGE_HANDLER(PepperFileMsg_QueryFile, OnQueryFile)
    IPC_MESSAGE_HANDLER(PepperFileMsg_GetDirContents, OnGetDirContents)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP_EX()
  return handled;
}

void PepperFileMessageFilter::OnDestruct() const {
  BrowserThread::DeleteOnIOThread::Destruct(this);
}

// static
FilePath PepperFileMessageFilter::GetDataDirName(const FilePath& profile_path) {
  return profile_path.Append(content::kPepperDataDirname);
}

PepperFileMessageFilter::~PepperFileMessageFilter() {
  // This function should be called on the IO thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
}

// Called on the FILE thread:
void PepperFileMessageFilter::OnOpenFile(
    const ppapi::PepperFilePath& path,
    int flags,
    base::PlatformFileError* error,
    IPC::PlatformFileForTransit* file) {
  FilePath full_path = ValidateAndConvertPepperFilePath(path, flags);
  if (full_path.empty()) {
    *error = base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    *file = IPC::InvalidPlatformFileForTransit();
    return;
  }

  base::PlatformFile file_handle = base::CreatePlatformFile(
      full_path, flags, NULL, error);

  if (*error != base::PLATFORM_FILE_OK ||
      file_handle == base::kInvalidPlatformFileValue) {
    *file = IPC::InvalidPlatformFileForTransit();
    return;
  }

  // Make sure we didn't try to open a directory: directory fd shouldn't pass
  // to untrusted processes because they open security holes.
  base::PlatformFileInfo info;
  if (!base::GetPlatformFileInfo(file_handle, &info) || info.is_directory) {
    // When in doubt, throw it out.
    *error = base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    *file = IPC::InvalidPlatformFileForTransit();
    return;
  }

#if defined(OS_WIN)
  // Duplicate the file handle so that the renderer process can access the file.
  if (!DuplicateHandle(GetCurrentProcess(), file_handle,
                       peer_handle(), file, 0, false,
                       DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)) {
    // file_handle is closed whether or not DuplicateHandle succeeds.
    *error = base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    *file = INVALID_HANDLE_VALUE;
  }
#else
  *file = base::FileDescriptor(file_handle, true);
#endif
}

void PepperFileMessageFilter::OnRenameFile(
    const ppapi::PepperFilePath& from_path,
    const ppapi::PepperFilePath& to_path,
    base::PlatformFileError* error) {
  FilePath from_full_path = ValidateAndConvertPepperFilePath(from_path,
                                                             kWritePermissions);
  FilePath to_full_path = ValidateAndConvertPepperFilePath(to_path,
                                                           kWritePermissions);
  if (from_full_path.empty() || to_full_path.empty()) {
    *error = base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    return;
  }

  bool result = file_util::Move(from_full_path, to_full_path);
  *error = result ? base::PLATFORM_FILE_OK
                  : base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
}

void PepperFileMessageFilter::OnDeleteFileOrDir(
    const ppapi::PepperFilePath& path,
    bool recursive,
    base::PlatformFileError* error) {
  FilePath full_path = ValidateAndConvertPepperFilePath(path,
                                                        kWritePermissions);
  if (full_path.empty()) {
    *error = base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    return;
  }

  bool result = file_util::Delete(full_path, recursive);
  *error = result ? base::PLATFORM_FILE_OK
                  : base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
}

void PepperFileMessageFilter::OnCreateDir(
    const ppapi::PepperFilePath& path,
    base::PlatformFileError* error) {
  FilePath full_path = ValidateAndConvertPepperFilePath(path,
                                                        kWritePermissions);
  if (full_path.empty()) {
    *error = base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    return;
  }

  bool result = file_util::CreateDirectory(full_path);
  *error = result ? base::PLATFORM_FILE_OK
                  : base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
}

void PepperFileMessageFilter::OnQueryFile(
    const ppapi::PepperFilePath& path,
    base::PlatformFileInfo* info,
    base::PlatformFileError* error) {
  FilePath full_path = ValidateAndConvertPepperFilePath(path, kReadPermissions);
  if (full_path.empty()) {
    *error = base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    return;
  }

  bool result = file_util::GetFileInfo(full_path, info);
  *error = result ? base::PLATFORM_FILE_OK
                  : base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
}

void PepperFileMessageFilter::OnGetDirContents(
    const ppapi::PepperFilePath& path,
    ppapi::DirContents* contents,
    base::PlatformFileError* error) {
  FilePath full_path = ValidateAndConvertPepperFilePath(path, kReadPermissions);
  if (full_path.empty()) {
    *error = base::PLATFORM_FILE_ERROR_ACCESS_DENIED;
    return;
  }

  contents->clear();

  file_util::FileEnumerator enumerator(
      full_path, false,
      static_cast<file_util::FileEnumerator::FileType>(
          file_util::FileEnumerator::FILES |
          file_util::FileEnumerator::DIRECTORIES |
          file_util::FileEnumerator::INCLUDE_DOT_DOT));

  while (!enumerator.Next().empty()) {
    file_util::FileEnumerator::FindInfo info;
    enumerator.GetFindInfo(&info);
    ppapi::DirEntry entry = {
      file_util::FileEnumerator::GetFilename(info),
      file_util::FileEnumerator::IsDirectory(info)
    };
    contents->push_back(entry);
  }

  *error = base::PLATFORM_FILE_OK;
}

FilePath PepperFileMessageFilter::ValidateAndConvertPepperFilePath(
    const ppapi::PepperFilePath& pepper_path, int flags) {
  FilePath file_path;  // Empty path returned on error.
  if (pepper_path.domain() == ppapi::PepperFilePath::DOMAIN_ABSOLUTE) {
    if (pepper_path.path().IsAbsolute() &&
        ChildProcessSecurityPolicyImpl::GetInstance()->HasPermissionsForFile(
            child_id(), pepper_path.path(), flags))
      file_path = pepper_path.path();
  }
  return file_path;
}

PepperTrustedFileMessageFilter::PepperTrustedFileMessageFilter(
    int child_id,
    const std::string& plugin_name,
    const FilePath& profile_data_directory)
    : PepperFileMessageFilter(child_id) {
  plugin_data_directory_ = GetDataDirName(profile_data_directory).Append(
      FilePath::FromUTF8Unsafe(plugin_name));
}

PepperTrustedFileMessageFilter::~PepperTrustedFileMessageFilter() {
}

FilePath PepperTrustedFileMessageFilter::ValidateAndConvertPepperFilePath(
    const ppapi::PepperFilePath& pepper_path,
    int flags) {
  FilePath file_path;  // Empty path returned on error.
  switch(pepper_path.domain()) {
    case ppapi::PepperFilePath::DOMAIN_ABSOLUTE:
      if (pepper_path.path().IsAbsolute() &&
          ChildProcessSecurityPolicyImpl::GetInstance()->HasPermissionsForFile(
              child_id(), pepper_path.path(), flags))
        file_path = pepper_path.path();
      break;
    case ppapi::PepperFilePath::DOMAIN_MODULE_LOCAL:
      if (!pepper_path.path().IsAbsolute() &&
          !pepper_path.path().ReferencesParent())
        file_path = plugin_data_directory_.Append(pepper_path.path());
      break;
    default:
      NOTREACHED();
      break;
  }
  return file_path;
}
