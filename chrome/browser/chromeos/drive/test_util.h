// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_TEST_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_TEST_UTIL_H_

#include <string>

#include "google_apis/drive/test_util.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/network_change_notifier.h"
#include "net/base/test_completion_callback.h"
#include "third_party/cros_system_api/constants/cryptohome.h"

class PrefRegistrySimple;

namespace net {
class IOBuffer;
}  // namespace net

namespace drive {

namespace test_util {

// Disk space size used by FakeFreeDiskSpaceGetter.
const int64 kLotsOfSpace = cryptohome::kMinFreeSpaceInBytes * 10;

// Runs a task posted to the blocking pool, including subsequent tasks posted
// to the UI message loop and the blocking pool.
//
// A task is often posted to the blocking pool with PostTaskAndReply(). In
// that case, a task is posted back to the UI message loop, which can again
// post a task to the blocking pool. This function processes these tasks
// repeatedly.
void RunBlockingPoolTask();

// Helper to destroy objects which needs Destroy() to be called on destruction.
// Note: When using this helper, you should destruct objects before
// BrowserThread.
struct DestroyHelperForTests {
  template<typename T>
  void operator()(T* object) const {
    if (object) {
      object->Destroy();
      test_util::RunBlockingPoolTask();  // Finish destruction.
    }
  }
};

// Reads all the data from |reader| and copies to |content|. Returns net::Error
// code.
template<typename Reader>
int ReadAllData(Reader* reader, std::string* content) {
  const int kBufferSize = 10;
  scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));
  while (true) {
    net::TestCompletionCallback callback;
    int result = reader->Read(buffer.get(), kBufferSize, callback.callback());
    result = callback.GetResult(result);
    if (result <= 0) {
      // Found an error or EOF. Return it. Note: net::OK is 0.
      return result;
    }
    content->append(buffer->data(), result);
  }
}

// Registers Drive related preferences in |pref_registry|. Drive related
// preferences should be registered as TestingPrefServiceSimple will crash if
// unregistered preference is referenced.
void RegisterDrivePrefs(PrefRegistrySimple* pref_registry);

// Fake NetworkChangeNotifier implementation.
class FakeNetworkChangeNotifier : public net::NetworkChangeNotifier {
 public:
  FakeNetworkChangeNotifier();

  void SetConnectionType(ConnectionType type);

  // NetworkChangeNotifier override.
  virtual ConnectionType GetCurrentConnectionType() const OVERRIDE;

 private:
  net::NetworkChangeNotifier::ConnectionType type_;
};

}  // namespace test_util
}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_TEST_UTIL_H_
