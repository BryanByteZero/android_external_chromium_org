// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_REFCOUNTED_PROFILE_KEYED_SERVICE_H_
#define CHROME_BROWSER_PROFILES_REFCOUNTED_PROFILE_KEYED_SERVICE_H_

#include "base/memory/ref_counted.h"
#include "chrome/browser/profiles/profile_keyed_base.h"
#include "content/public/browser/browser_thread.h"

class RefcountedProfileKeyedService;

namespace impl {

struct RefcountedProfileKeyedServiceTraits {
  static void Destruct(const RefcountedProfileKeyedService* obj);
};

}  // namespace impl

// Base class for refcounted objects that hang off the Profile.
//
// The two pass shutdown described in ProfileKeyedService works a bit
// differently because there could be outstanding references on other
// threads. ShutdownOnUIThread() will be called on the UI thread, and then the
// destructor will run when the last reference is dropped, which may or may not
// be after the corresponding Profile has been destroyed.
//
// Optionally, if you initialize your service with the constructor that takes a
// thread ID, your service will be deleted on that thread. We can't use
// content::DeleteOnThread<> directly because RefcountedProfileKeyedService
// must be one type that RefcountedProfileKeyedServiceFactory can use.
class RefcountedProfileKeyedService
    : public ProfileKeyedBase,
      public base::RefCountedThreadSafe<
          RefcountedProfileKeyedService,
          impl::RefcountedProfileKeyedServiceTraits> {
 public:
  // Unlike ProfileKeyedService, ShutdownOnUI is not optional. You must do
  // something to drop references during the first pass Shutdown() because this
  // is the only point where you are guaranteed that something is running on
  // the UI thread. The PKSF framework will ensure that this is only called on
  // the UI thread; you do not need to check for that yourself.
  virtual void ShutdownOnUIThread() = 0;

  // The second pass destruction can happen anywhere unless you specify which
  // thread this service must be destroyed on by using the second constructor.
  virtual ~RefcountedProfileKeyedService();

 protected:
  // If you want your service deleted wherever, use the default constructor.
  RefcountedProfileKeyedService();

  // If you need your service to be deleted on a specific thread (for example,
  // you're converting a service that used content::DeleteOnThread<IO>), then
  // use this constructor with the ID of the thread.
  explicit RefcountedProfileKeyedService(
      const content::BrowserThread::ID thread_id);

 private:
  friend struct impl::RefcountedProfileKeyedServiceTraits;

  // Do we have to delete this object on a specific thread?
  bool requires_destruction_on_thread_;
  content::BrowserThread::ID thread_id_;
};

#endif  // CHROME_BROWSER_PROFILES_REFCOUNTED_PROFILE_KEYED_SERVICE_H_
