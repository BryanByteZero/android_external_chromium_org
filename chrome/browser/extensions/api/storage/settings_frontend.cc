// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/storage/settings_frontend.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "chrome/browser/extensions/api/storage/leveldb_settings_storage_factory.h"
#include "chrome/browser/extensions/api/storage/local_value_store_cache.h"
#include "chrome/common/extensions/api/storage.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"

using content::BrowserContext;
using content::BrowserThread;

namespace extensions {

namespace {

base::LazyInstance<BrowserContextKeyedAPIFactory<SettingsFrontend> > g_factory =
    LAZY_INSTANCE_INITIALIZER;

// Settings change Observer which forwards changes on to the extension
// processes for |context| and its incognito partner if it exists.
class DefaultObserver : public SettingsObserver {
 public:
  explicit DefaultObserver(BrowserContext* context)
      : browser_context_(context) {}

  // SettingsObserver implementation.
  virtual void OnSettingsChanged(
      const std::string& extension_id,
      settings_namespace::Namespace settings_namespace,
      const std::string& change_json) OVERRIDE {
    // TODO(gdk): This is a temporary hack while the refactoring for
    // string-based event payloads is removed. http://crbug.com/136045
    scoped_ptr<base::ListValue> args(new base::ListValue());
    args->Append(base::JSONReader::Read(change_json));
    args->Append(new base::StringValue(settings_namespace::ToString(
        settings_namespace)));
    scoped_ptr<Event> event(new Event(
        api::storage::OnChanged::kEventName, args.Pass()));
    ExtensionSystem::Get(browser_context_)->event_router()->
        DispatchEventToExtension(extension_id, event.Pass());
  }

 private:
  BrowserContext* const browser_context_;
};

}  // namespace

// static
SettingsFrontend* SettingsFrontend::Get(BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<SettingsFrontend>::Get(context);
}

// static
SettingsFrontend* SettingsFrontend::CreateForTesting(
    const scoped_refptr<SettingsStorageFactory>& storage_factory,
    BrowserContext* context) {
  return new SettingsFrontend(storage_factory, context);
}

SettingsFrontend::SettingsFrontend(BrowserContext* context)
    : browser_context_(context) {
  Init(new LeveldbSettingsStorageFactory());
}

SettingsFrontend::SettingsFrontend(
    const scoped_refptr<SettingsStorageFactory>& factory,
    BrowserContext* context)
    : browser_context_(context) {
  Init(factory);
}

void SettingsFrontend::Init(
    const scoped_refptr<SettingsStorageFactory>& factory) {
  observers_ = new SettingsObserverList();
  browser_context_observer_.reset(new DefaultObserver(browser_context_));
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!browser_context_->IsOffTheRecord());

  observers_->AddObserver(browser_context_observer_.get());

  caches_[settings_namespace::LOCAL] =
      new LocalValueStoreCache(factory, browser_context_->GetPath());

  // Add any additional caches the embedder supports (for example, caches
  // for chrome.storage.managed and chrome.storage.sync).
  ExtensionsAPIClient::Get()->AddAdditionalValueStoreCaches(
      browser_context_, factory, observers_, &caches_);
}

SettingsFrontend::~SettingsFrontend() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  observers_->RemoveObserver(browser_context_observer_.get());
  for (CacheMap::iterator it = caches_.begin(); it != caches_.end(); ++it) {
    ValueStoreCache* cache = it->second;
    cache->ShutdownOnUI();
    BrowserThread::DeleteSoon(BrowserThread::FILE, FROM_HERE, cache);
  }
}

ValueStoreCache* SettingsFrontend::GetValueStoreCache(
    settings_namespace::Namespace settings_namespace) const {
  CacheMap::const_iterator it = caches_.find(settings_namespace);
  if (it != caches_.end())
    return it->second;
  return NULL;
}

bool SettingsFrontend::IsStorageEnabled(
    settings_namespace::Namespace settings_namespace) const {
  return caches_.find(settings_namespace) != caches_.end();
}

void SettingsFrontend::RunWithStorage(
    scoped_refptr<const Extension> extension,
    settings_namespace::Namespace settings_namespace,
    const ValueStoreCache::StorageCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CHECK(extension.get());

  ValueStoreCache* cache = caches_[settings_namespace];
  CHECK(cache);

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&ValueStoreCache::RunWithValueStoreForExtension,
                 base::Unretained(cache), callback, extension));
}

void SettingsFrontend::DeleteStorageSoon(
    const std::string& extension_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  for (CacheMap::iterator it = caches_.begin(); it != caches_.end(); ++it) {
    ValueStoreCache* cache = it->second;
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(&ValueStoreCache::DeleteStorageSoon,
                   base::Unretained(cache),
                   extension_id));
  }
}

scoped_refptr<SettingsObserverList> SettingsFrontend::GetObservers() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  return observers_;
}

void SettingsFrontend::DisableStorageForTesting(
    settings_namespace::Namespace settings_namespace) {
  CacheMap::iterator it = caches_.find(settings_namespace);
  if (it != caches_.end()) {
    ValueStoreCache* cache = it->second;
    cache->ShutdownOnUI();
    BrowserThread::DeleteSoon(BrowserThread::FILE, FROM_HERE, cache);
    caches_.erase(it);
  }
}

// BrowserContextKeyedAPI implementation.

// static
BrowserContextKeyedAPIFactory<SettingsFrontend>*
SettingsFrontend::GetFactoryInstance() {
  return g_factory.Pointer();
}

// static
const char* SettingsFrontend::service_name() {
  return "SettingsFrontend";
}

}  // namespace extensions
