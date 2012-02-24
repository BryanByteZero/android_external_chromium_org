// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data_local_storage_helper.h"

#include "base/bind.h"
#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/dom_storage_context.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebCString.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebSecurityOrigin.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebString.h"
#include "webkit/glue/webkit_glue.h"

using content::BrowserContext;
using content::BrowserThread;
using content::DOMStorageContext;
using WebKit::WebSecurityOrigin;

BrowsingDataLocalStorageHelper::LocalStorageInfo::LocalStorageInfo()
    : port(0),
      size(0) {
}

BrowsingDataLocalStorageHelper::LocalStorageInfo::LocalStorageInfo(
    const std::string& protocol,
    const std::string& host,
    unsigned short port,
    const std::string& database_identifier,
    const std::string& origin,
    const FilePath& file_path,
    int64 size,
    base::Time last_modified)
    : protocol(protocol),
      host(host),
      port(port),
      database_identifier(database_identifier),
      origin(origin),
      file_path(file_path),
      size(size),
      last_modified(last_modified) {
      }

BrowsingDataLocalStorageHelper::LocalStorageInfo::~LocalStorageInfo() {}

BrowsingDataLocalStorageHelper::BrowsingDataLocalStorageHelper(
    Profile* profile)
    : dom_storage_context_(BrowserContext::GetDOMStorageContext(profile)),
      is_fetching_(false) {
  DCHECK(dom_storage_context_.get());
}

BrowsingDataLocalStorageHelper::~BrowsingDataLocalStorageHelper() {
}

void BrowsingDataLocalStorageHelper::StartFetching(
    const base::Callback<void(const std::list<LocalStorageInfo>&)>& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!is_fetching_);
  DCHECK_EQ(false, callback.is_null());

  is_fetching_ = true;
  completion_callback_ = callback;
  BrowserThread::PostTask(
      BrowserThread::WEBKIT_DEPRECATED, FROM_HERE,
      base::Bind(
          &BrowsingDataLocalStorageHelper::FetchLocalStorageInfoInWebKitThread,
          this));
}

void BrowsingDataLocalStorageHelper::CancelNotification() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  completion_callback_.Reset();
}

void BrowsingDataLocalStorageHelper::DeleteLocalStorageFile(
    const FilePath& file_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(
      BrowserThread::WEBKIT_DEPRECATED, FROM_HERE,
      base::Bind(
          &BrowsingDataLocalStorageHelper::DeleteLocalStorageFileInWebKitThread,
          this, file_path));
}

void BrowsingDataLocalStorageHelper::FetchLocalStorageInfoInWebKitThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::WEBKIT_DEPRECATED));
  std::vector<FilePath> files = dom_storage_context_->GetAllStorageFiles();
  for (size_t i = 0; i < files.size(); ++i) {
    FilePath file_path = files[i];
    WebSecurityOrigin web_security_origin =
        WebSecurityOrigin::createFromDatabaseIdentifier(
            webkit_glue::FilePathToWebString(file_path.BaseName()));
    if (EqualsASCII(web_security_origin.protocol(), chrome::kExtensionScheme)) {
      // Extension state is not considered browsing data.
      continue;
    }
    base::PlatformFileInfo file_info;
    bool ret = file_util::GetFileInfo(file_path, &file_info);
    if (ret) {
      local_storage_info_.push_back(LocalStorageInfo(
          web_security_origin.protocol().utf8(),
          web_security_origin.host().utf8(),
          web_security_origin.port(),
          web_security_origin.databaseIdentifier().utf8(),
          web_security_origin.toString().utf8(),
          file_path,
          file_info.size,
          file_info.last_modified));
    }
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&BrowsingDataLocalStorageHelper::NotifyInUIThread, this));
}

void BrowsingDataLocalStorageHelper::NotifyInUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(is_fetching_);
  // Note: completion_callback_ mutates only in the UI thread, so it's safe to
  // test it here.
  if (!completion_callback_.is_null()) {
    completion_callback_.Run(local_storage_info_);
    completion_callback_.Reset();
  }
  is_fetching_ = false;
}

void BrowsingDataLocalStorageHelper::DeleteLocalStorageFileInWebKitThread(
    const FilePath& file_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::WEBKIT_DEPRECATED));
  dom_storage_context_->DeleteLocalStorageFile(file_path);
}

CannedBrowsingDataLocalStorageHelper::CannedBrowsingDataLocalStorageHelper(
    Profile* profile)
    : BrowsingDataLocalStorageHelper(profile),
      profile_(profile) {
}

CannedBrowsingDataLocalStorageHelper*
CannedBrowsingDataLocalStorageHelper::Clone() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CannedBrowsingDataLocalStorageHelper* clone =
      new CannedBrowsingDataLocalStorageHelper(profile_);

  base::AutoLock auto_lock(lock_);
  clone->pending_local_storage_info_ = pending_local_storage_info_;
  clone->local_storage_info_ = local_storage_info_;
  return clone;
}

void CannedBrowsingDataLocalStorageHelper::AddLocalStorage(
    const GURL& origin) {
  base::AutoLock auto_lock(lock_);
  pending_local_storage_info_.insert(origin);
}

void CannedBrowsingDataLocalStorageHelper::Reset() {
  base::AutoLock auto_lock(lock_);
  local_storage_info_.clear();
  pending_local_storage_info_.clear();
}

bool CannedBrowsingDataLocalStorageHelper::empty() const {
  base::AutoLock auto_lock(lock_);
  return local_storage_info_.empty() && pending_local_storage_info_.empty();
}

void CannedBrowsingDataLocalStorageHelper::StartFetching(
    const base::Callback<void(const std::list<LocalStorageInfo>&)>& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!is_fetching_);
  DCHECK_EQ(false, callback.is_null());

  is_fetching_ = true;
  completion_callback_ = callback;
  BrowserThread::PostTask(
      BrowserThread::WEBKIT_DEPRECATED, FROM_HERE,
      base::Bind(&CannedBrowsingDataLocalStorageHelper::
          ConvertPendingInfoInWebKitThread, this));
}

CannedBrowsingDataLocalStorageHelper::~CannedBrowsingDataLocalStorageHelper() {}

void CannedBrowsingDataLocalStorageHelper::ConvertPendingInfoInWebKitThread() {
  base::AutoLock auto_lock(lock_);
  for (std::set<GURL>::iterator info = pending_local_storage_info_.begin();
       info != pending_local_storage_info_.end(); ++info) {
    WebSecurityOrigin web_security_origin =
        WebSecurityOrigin::createFromString(
            UTF8ToUTF16(info->spec()));
    std::string security_origin(web_security_origin.toString().utf8());

    bool duplicate = false;
    for (std::list<LocalStorageInfo>::iterator
         local_storage = local_storage_info_.begin();
         local_storage != local_storage_info_.end(); ++local_storage) {
      if (local_storage->origin == security_origin) {
        duplicate = true;
        break;
      }
    }
    if (duplicate)
      continue;

    local_storage_info_.push_back(LocalStorageInfo(
        web_security_origin.protocol().utf8(),
        web_security_origin.host().utf8(),
        web_security_origin.port(),
        web_security_origin.databaseIdentifier().utf8(),
        security_origin,
        dom_storage_context_->
            GetFilePath(web_security_origin.databaseIdentifier()),
        0,
        base::Time()));
  }
  pending_local_storage_info_.clear();

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&CannedBrowsingDataLocalStorageHelper::NotifyInUIThread,
                 this));
}
