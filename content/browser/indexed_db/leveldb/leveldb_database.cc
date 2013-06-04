// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/leveldb/leveldb_database.h"

#include <string>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/histogram.h"
#include "base/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_info.h"
#include "content/browser/indexed_db/leveldb/leveldb_comparator.h"
#include "content/browser/indexed_db/leveldb/leveldb_iterator.h"
#include "content/browser/indexed_db/leveldb/leveldb_slice.h"
#include "content/browser/indexed_db/leveldb/leveldb_write_batch.h"
#include "third_party/leveldatabase/env_idb.h"
#include "third_party/leveldatabase/src/helpers/memenv/memenv.h"
#include "third_party/leveldatabase/src/include/leveldb/comparator.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/env.h"
#include "third_party/leveldatabase/src/include/leveldb/slice.h"

namespace content {

static leveldb::Slice MakeSlice(const std::vector<char>& value) {
  DCHECK_GT(value.size(), static_cast<size_t>(0));
  return leveldb::Slice(&*value.begin(), value.size());
}

static leveldb::Slice MakeSlice(const LevelDBSlice& s) {
  return leveldb::Slice(s.begin(), s.end() - s.begin());
}

static LevelDBSlice MakeLevelDBSlice(const leveldb::Slice& s) {
  return LevelDBSlice(s.data(), s.data() + s.size());
}

class ComparatorAdapter : public leveldb::Comparator {
 public:
  explicit ComparatorAdapter(const LevelDBComparator* comparator)
      : comparator_(comparator) {}

  virtual int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const
      OVERRIDE {
    return comparator_->Compare(MakeLevelDBSlice(a), MakeLevelDBSlice(b));
  }

  virtual const char* Name() const OVERRIDE { return comparator_->Name(); }

  // TODO(jsbell): Support the methods below in the future.
  virtual void FindShortestSeparator(std::string* start,
                                     const leveldb::Slice& limit) const
      OVERRIDE {}
  virtual void FindShortSuccessor(std::string* key) const OVERRIDE {}

 private:
  const LevelDBComparator* comparator_;
};

LevelDBSnapshot::LevelDBSnapshot(LevelDBDatabase* db)
    : db_(db->db_.get()), snapshot_(db_->GetSnapshot()) {}

LevelDBSnapshot::~LevelDBSnapshot() { db_->ReleaseSnapshot(snapshot_); }

LevelDBDatabase::LevelDBDatabase() {}

LevelDBDatabase::~LevelDBDatabase() {
  // db_'s destructor uses comparator_adapter_; order of deletion is important.
  db_.reset();
  comparator_adapter_.reset();
  env_.reset();
}

static leveldb::Status OpenDB(leveldb::Comparator* comparator,
                              leveldb::Env* env,
                              const base::FilePath& path,
                              leveldb::DB** db) {
  leveldb::Options options;
  options.comparator = comparator;
  options.create_if_missing = true;
  options.paranoid_checks = true;

  // Marking compression as explicitly off so snappy support can be
  // compiled in for other leveldb clients without implicitly enabling
  // it for IndexedDB. http://crbug.com/81384
  options.compression = leveldb::kNoCompression;

  // 20 max_open_files is the minimum LevelDB allows but its cache behaves
  // poorly with less than 4 files per shard. As of this writing the latest
  // leveldb (1.9) hardcodes 16 shards. See
  // https://code.google.com/p/chromium/issues/detail?id=227313#c11
  options.max_open_files = 80;
  options.env = env;

  // ChromiumEnv assumes UTF8, converts back to FilePath before using.
  return leveldb::DB::Open(options, path.AsUTF8Unsafe(), db);
}

bool LevelDBDatabase::Destroy(const base::FilePath& file_name) {
  leveldb::Options options;
  options.env = leveldb::IDBEnv();
  // ChromiumEnv assumes UTF8, converts back to FilePath before using.
  const leveldb::Status s =
      leveldb::DestroyDB(file_name.AsUTF8Unsafe(), options);
  return s.ok();
}

static void HistogramFreeSpace(const char* type,
                               const base::FilePath& file_name) {
  string16 name = ASCIIToUTF16("WebCore.IndexedDB.LevelDB.Open") +
                  ASCIIToUTF16(type) + ASCIIToUTF16("FreeDiskSpace");
  int64 free_disk_space_in_k_bytes =
      base::SysInfo::AmountOfFreeDiskSpace(file_name) / 1024;
  if (free_disk_space_in_k_bytes < 0) {
    base::Histogram::FactoryGet(
        "WebCore.IndexedDB.LevelDB.FreeDiskSpaceFailure",
        1,
        2 /*boundary*/,
        2 /*boundary*/ + 1,
        base::HistogramBase::kUmaTargetedHistogramFlag)->Add(1 /*sample*/);
    return;
  }
  int clamped_disk_space_k_bytes =
      free_disk_space_in_k_bytes > INT_MAX ? INT_MAX
                                           : free_disk_space_in_k_bytes;
  const uint64 histogram_max = static_cast<uint64>(1e9);
  COMPILE_ASSERT(histogram_max <= INT_MAX, histogram_max_too_big);
  base::Histogram::FactoryGet(UTF16ToUTF8(name),
                              1,
                              histogram_max,
                              11 /*buckets*/,
                              base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(clamped_disk_space_k_bytes);
}

static void HistogramLevelDBError(const char* histogram_name,
                                  const leveldb::Status& s) {
  DCHECK(!s.ok());
  enum {
    LEVEL_DB_NOT_FOUND,
    LEVEL_DB_CORRUPTION,
    LEVEL_DB_IO_ERROR,
    LEVEL_DB_OTHER,
    LEVEL_DB_MAX_ERROR
  };
  int leveldb_error = LEVEL_DB_OTHER;
  if (s.IsNotFound())
    leveldb_error = LEVEL_DB_NOT_FOUND;
  else if (s.IsCorruption())
    leveldb_error = LEVEL_DB_CORRUPTION;
  else if (s.IsIOError())
    leveldb_error = LEVEL_DB_IO_ERROR;
  base::Histogram::FactoryGet(histogram_name,
                              1,
                              LEVEL_DB_MAX_ERROR,
                              LEVEL_DB_MAX_ERROR + 1,
                              base::HistogramBase::kUmaTargetedHistogramFlag)
      ->Add(leveldb_error);
}

scoped_ptr<LevelDBDatabase> LevelDBDatabase::Open(
    const base::FilePath& file_name,
    const LevelDBComparator* comparator) {
  scoped_ptr<ComparatorAdapter> comparator_adapter(
      new ComparatorAdapter(comparator));

  leveldb::DB* db;
  const leveldb::Status s =
      OpenDB(comparator_adapter.get(), leveldb::IDBEnv(), file_name, &db);

  if (!s.ok()) {
    HistogramLevelDBError("WebCore.IndexedDB.LevelDBOpenErrors", s);
    HistogramFreeSpace("Failure", file_name);

    LOG(ERROR) << "Failed to open LevelDB database from "
               << file_name.AsUTF8Unsafe() << "," << s.ToString();
    return scoped_ptr<LevelDBDatabase>();
  }

  HistogramFreeSpace("Success", file_name);

  scoped_ptr<LevelDBDatabase> result(new LevelDBDatabase);
  result->db_ = make_scoped_ptr(db);
  result->comparator_adapter_ = comparator_adapter.Pass();
  result->comparator_ = comparator;

  return result.Pass();
}

scoped_ptr<LevelDBDatabase> LevelDBDatabase::OpenInMemory(
    const LevelDBComparator* comparator) {
  scoped_ptr<ComparatorAdapter> comparator_adapter(
      new ComparatorAdapter(comparator));
  scoped_ptr<leveldb::Env> in_memory_env(leveldb::NewMemEnv(leveldb::IDBEnv()));

  leveldb::DB* db;
  const leveldb::Status s = OpenDB(
      comparator_adapter.get(), in_memory_env.get(), base::FilePath(), &db);

  if (!s.ok()) {
    LOG(ERROR) << "Failed to open in-memory LevelDB database: " << s.ToString();
    return scoped_ptr<LevelDBDatabase>();
  }

  scoped_ptr<LevelDBDatabase> result(new LevelDBDatabase);
  result->env_ = in_memory_env.Pass();
  result->db_ = make_scoped_ptr(db);
  result->comparator_adapter_ = comparator_adapter.Pass();
  result->comparator_ = comparator;

  return result.Pass();
}

bool LevelDBDatabase::Put(const LevelDBSlice& key,
                          const std::vector<char>& value) {
  leveldb::WriteOptions write_options;
  write_options.sync = true;

  const leveldb::Status s =
      db_->Put(write_options, MakeSlice(key), MakeSlice(value));
  if (s.ok())
    return true;
  LOG(ERROR) << "LevelDB put failed: " << s.ToString();
  return false;
}

bool LevelDBDatabase::Remove(const LevelDBSlice& key) {
  leveldb::WriteOptions write_options;
  write_options.sync = true;

  const leveldb::Status s = db_->Delete(write_options, MakeSlice(key));
  if (s.ok())
    return true;
  if (s.IsNotFound())
    return false;
  LOG(ERROR) << "LevelDB remove failed: " << s.ToString();
  return false;
}

bool LevelDBDatabase::Get(const LevelDBSlice& key,
                          std::vector<char>& value,
                          bool& found,
                          const LevelDBSnapshot* snapshot) {
  found = false;
  std::string result;
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;  // TODO(jsbell): Disable this if the
                                         // performance impact is too great.
  read_options.snapshot = snapshot ? snapshot->snapshot_ : 0;

  const leveldb::Status s = db_->Get(read_options, MakeSlice(key), &result);
  if (s.ok()) {
    found = true;
    value.clear();
    value.insert(value.end(), result.begin(), result.end());
    return true;
  }
  if (s.IsNotFound())
    return true;
  LOG(ERROR) << "LevelDB get failed: " << s.ToString();
  return false;
}

bool LevelDBDatabase::Write(LevelDBWriteBatch& write_batch) {
  leveldb::WriteOptions write_options;
  write_options.sync = true;

  const leveldb::Status s =
      db_->Write(write_options, write_batch.write_batch_.get());
  if (s.ok())
    return true;
  HistogramLevelDBError("WebCore.IndexedDB.LevelDBWriteErrors", s);
  LOG(ERROR) << "LevelDB write failed: " << s.ToString();
  return false;
}

namespace {
class IteratorImpl : public LevelDBIterator {
 public:
  virtual ~IteratorImpl() {}

  virtual bool IsValid() const OVERRIDE;
  virtual void SeekToLast() OVERRIDE;
  virtual void Seek(const LevelDBSlice& target) OVERRIDE;
  virtual void Next() OVERRIDE;
  virtual void Prev() OVERRIDE;
  virtual LevelDBSlice Key() const OVERRIDE;
  virtual LevelDBSlice Value() const OVERRIDE;

 private:
  friend class content::LevelDBDatabase;
  explicit IteratorImpl(scoped_ptr<leveldb::Iterator> iterator);
  void CheckStatus();

  scoped_ptr<leveldb::Iterator> iterator_;
};
}

IteratorImpl::IteratorImpl(scoped_ptr<leveldb::Iterator> it)
    : iterator_(it.Pass()) {}

void IteratorImpl::CheckStatus() {
  const leveldb::Status s = iterator_->status();
  if (!s.ok())
    LOG(ERROR) << "LevelDB iterator error: " << s.ToString();
}

bool IteratorImpl::IsValid() const { return iterator_->Valid(); }

void IteratorImpl::SeekToLast() {
  iterator_->SeekToLast();
  CheckStatus();
}

void IteratorImpl::Seek(const LevelDBSlice& target) {
  iterator_->Seek(MakeSlice(target));
  CheckStatus();
}

void IteratorImpl::Next() {
  DCHECK(IsValid());
  iterator_->Next();
  CheckStatus();
}

void IteratorImpl::Prev() {
  DCHECK(IsValid());
  iterator_->Prev();
  CheckStatus();
}

LevelDBSlice IteratorImpl::Key() const {
  DCHECK(IsValid());
  return MakeLevelDBSlice(iterator_->key());
}

LevelDBSlice IteratorImpl::Value() const {
  DCHECK(IsValid());
  return MakeLevelDBSlice(iterator_->value());
}

scoped_ptr<LevelDBIterator> LevelDBDatabase::CreateIterator(
    const LevelDBSnapshot* snapshot) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;  // TODO(jsbell): Disable this if the
                                         // performance impact is too great.
  read_options.snapshot = snapshot ? snapshot->snapshot_ : 0;
  scoped_ptr<leveldb::Iterator> i(db_->NewIterator(read_options));
  if (!i)  // TODO(jsbell): Double check if we actually need to check this.
    return scoped_ptr<LevelDBIterator>();
  return scoped_ptr<LevelDBIterator>(new IteratorImpl(i.Pass()));
}

const LevelDBComparator* LevelDBDatabase::Comparator() const {
  return comparator_;
}

}  // namespace content
