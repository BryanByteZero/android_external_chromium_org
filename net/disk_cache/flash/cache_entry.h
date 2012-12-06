// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DISK_CACHE_FLASH_LOG_ENTRY_H_
#define NET_DISK_CACHE_FLASH_LOG_ENTRY_H_

#include <vector>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "net/base/net_export.h"
#include "net/disk_cache/flash/format.h"

namespace net {
class IOBuffer;
};

namespace disk_cache {

class LogStructuredStore;

class NET_EXPORT_PRIVATE CacheEntry {
 public:
  explicit CacheEntry(LogStructuredStore* store);
  CacheEntry(LogStructuredStore* store, int32 id);
  ~CacheEntry();

  bool Init();
  bool Close();

  int32 id() const;
  int32 GetDataSize(int index) const;

  int ReadData(int index, int offset, net::IOBuffer* buf, int buf_len);
  int WriteData(int index, int offset, net::IOBuffer* buf, int buf_len);

 private:
  struct Stream {
    Stream();
    ~Stream();
    int offset;
    int size;
    std::vector<char> write_buffer;
  };

  bool OnDisk() const;
  bool InvalidStream(int stream_index) const;
  int32 Size() const;
  bool Save();

  LogStructuredStore* store_;
  int32 id_;
  Stream streams_[kFlashCacheEntryNumStreams];
  bool init_;
  bool closed_;

  DISALLOW_COPY_AND_ASSIGN(CacheEntry);
};

}  // namespace disk_cache

#endif  // NET_DISK_CACHE_FLASH_LOG_ENTRY_H_
