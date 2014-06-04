// This file was GENERATED by command:
//     pump.py callback_tracker_internal.h.pump
// DO NOT EDIT BY HAND!!!


// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CALLBACK_TRACKER_INTERNAL_H_
#define CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CALLBACK_TRACKER_INTERNAL_H_

#include "base/callback.h"
#include "base/callback_internal.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"

namespace sync_file_system {
namespace drive_backend {

class CallbackTracker;

namespace internal {

class AbortHelper {
 public:
  explicit AbortHelper(CallbackTracker* tracker);
  ~AbortHelper();
  base::WeakPtr<AbortHelper> AsWeakPtr();

  static scoped_ptr<AbortHelper> TakeOwnership(
      const base::WeakPtr<AbortHelper>& abort_helper);

 private:
  CallbackTracker* tracker_;  // Not owned.
  base::WeakPtrFactory<AbortHelper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AbortHelper);
};

template <typename>
struct InvokeAndInvalidateHelper;

template <>
struct InvokeAndInvalidateHelper<void()> {
  static void Run(const base::WeakPtr<AbortHelper>& abort_helper,
                  const base::Callback<void()>& callback) {
    scoped_ptr<AbortHelper> deleter = AbortHelper::TakeOwnership(abort_helper);
    if (deleter) {
      callback.Run();
    }
  }
};

template <typename A1>
struct InvokeAndInvalidateHelper<void(A1)> {
  static void Run(const base::WeakPtr<AbortHelper>& abort_helper,
                  const base::Callback<void(A1)>& callback, A1 a1) {
    scoped_ptr<AbortHelper> deleter = AbortHelper::TakeOwnership(abort_helper);
    if (deleter) {
      callback.Run(base::internal::CallbackForward(a1));
    }
  }
};

template <typename A1, typename A2>
struct InvokeAndInvalidateHelper<void(A1, A2)> {
  static void Run(const base::WeakPtr<AbortHelper>& abort_helper,
                  const base::Callback<void(A1, A2)>& callback, A1 a1, A2 a2) {
    scoped_ptr<AbortHelper> deleter = AbortHelper::TakeOwnership(abort_helper);
    if (deleter) {
      callback.Run(base::internal::CallbackForward(a1),
          base::internal::CallbackForward(a2));
    }
  }
};

template <typename A1, typename A2, typename A3>
struct InvokeAndInvalidateHelper<void(A1, A2, A3)> {
  static void Run(const base::WeakPtr<AbortHelper>& abort_helper,
                  const base::Callback<void(A1, A2, A3)>& callback, A1 a1,
                      A2 a2, A3 a3) {
    scoped_ptr<AbortHelper> deleter = AbortHelper::TakeOwnership(abort_helper);
    if (deleter) {
      callback.Run(base::internal::CallbackForward(a1),
          base::internal::CallbackForward(a2),
          base::internal::CallbackForward(a3));
    }
  }
};

template <typename A1, typename A2, typename A3, typename A4>
struct InvokeAndInvalidateHelper<void(A1, A2, A3, A4)> {
  static void Run(const base::WeakPtr<AbortHelper>& abort_helper,
                  const base::Callback<void(A1, A2, A3, A4)>& callback, A1 a1,
                      A2 a2, A3 a3, A4 a4) {
    scoped_ptr<AbortHelper> deleter = AbortHelper::TakeOwnership(abort_helper);
    if (deleter) {
      callback.Run(base::internal::CallbackForward(a1),
          base::internal::CallbackForward(a2),
          base::internal::CallbackForward(a3),
          base::internal::CallbackForward(a4));
    }
  }
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
struct InvokeAndInvalidateHelper<void(A1, A2, A3, A4, A5)> {
  static void Run(const base::WeakPtr<AbortHelper>& abort_helper,
                  const base::Callback<void(A1, A2, A3, A4, A5)>& callback,
                      A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
    scoped_ptr<AbortHelper> deleter = AbortHelper::TakeOwnership(abort_helper);
    if (deleter) {
      callback.Run(base::internal::CallbackForward(a1),
          base::internal::CallbackForward(a2),
          base::internal::CallbackForward(a3),
          base::internal::CallbackForward(a4),
          base::internal::CallbackForward(a5));
    }
  }
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6>
struct InvokeAndInvalidateHelper<void(A1, A2, A3, A4, A5, A6)> {
  static void Run(const base::WeakPtr<AbortHelper>& abort_helper,
                  const base::Callback<void(A1, A2, A3, A4, A5, A6)>& callback,
                      A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
    scoped_ptr<AbortHelper> deleter = AbortHelper::TakeOwnership(abort_helper);
    if (deleter) {
      callback.Run(base::internal::CallbackForward(a1),
          base::internal::CallbackForward(a2),
          base::internal::CallbackForward(a3),
          base::internal::CallbackForward(a4),
          base::internal::CallbackForward(a5),
          base::internal::CallbackForward(a6));
    }
  }
};

template <typename A1, typename A2, typename A3, typename A4, typename A5,
    typename A6, typename A7>
struct InvokeAndInvalidateHelper<void(A1, A2, A3, A4, A5, A6, A7)> {
  static void Run(const base::WeakPtr<AbortHelper>& abort_helper,
                  const base::Callback<void(A1, A2, A3, A4, A5, A6,
                      A7)>& callback, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6,
                      A7 a7) {
    scoped_ptr<AbortHelper> deleter = AbortHelper::TakeOwnership(abort_helper);
    if (deleter) {
      callback.Run(base::internal::CallbackForward(a1),
          base::internal::CallbackForward(a2),
          base::internal::CallbackForward(a3),
          base::internal::CallbackForward(a4),
          base::internal::CallbackForward(a5),
          base::internal::CallbackForward(a6),
          base::internal::CallbackForward(a7));
    }
  }
};

}  // namespace internal
}  // namespace drive_backend
}  // namespace sync_file_system

#endif  // CHROME_BROWSER_SYNC_FILE_SYSTEM_DRIVE_BACKEND_CALLBACK_TRACKER_INTERNAL_H_
