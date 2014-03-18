// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/prefix_set.h"

#include <algorithm>
#include <iterator>

#include "base/file_util.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/md5.h"
#include "base/memory/scoped_ptr.h"
#include "base/rand_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

const SBPrefix kHighBitClear = 1000u * 1000u * 1000u;
const SBPrefix kHighBitSet = 3u * 1000u * 1000u * 1000u;

class PrefixSetTest : public PlatformTest {
 protected:
  // Constants for the v1 format.
  static const size_t kMagicOffset = 0 * sizeof(uint32);
  static const size_t kVersionOffset = 1 * sizeof(uint32);
  static const size_t kIndexSizeOffset = 2 * sizeof(uint32);
  static const size_t kDeltasSizeOffset = 3 * sizeof(uint32);
  static const size_t kPayloadOffset = 4 * sizeof(uint32);

  // Generate a set of random prefixes to share between tests.  For
  // most tests this generation was a large fraction of the test time.
  //
  // The set should contain sparse areas where adjacent items are more
  // than 2^16 apart, and dense areas where adjacent items are less
  // than 2^16 apart.
  static void SetUpTestCase() {
    // Distribute clusters of prefixes.
    for (size_t i = 0; i < 250; ++i) {
      // Unsigned for overflow characteristics.
      const uint32 base = static_cast<uint32>(base::RandUint64());
      for (size_t j = 0; j < 10; ++j) {
        const uint32 delta = static_cast<uint32>(base::RandUint64() & 0xFFFF);
        const SBPrefix prefix = static_cast<SBPrefix>(base + delta);
        shared_prefixes_.push_back(prefix);
      }
    }

    // Lay down a sparsely-distributed layer.
    const size_t count = shared_prefixes_.size();
    for (size_t i = 0; i < count; ++i) {
      const SBPrefix prefix = static_cast<SBPrefix>(base::RandUint64());
      shared_prefixes_.push_back(prefix);
    }

    // Sort for use with PrefixSet constructor.
    std::sort(shared_prefixes_.begin(), shared_prefixes_.end());
  }

  // Check that all elements of |prefixes| are in |prefix_set|, and
  // that nearby elements are not (for lack of a more sensible set of
  // items to check for absence).
  static void CheckPrefixes(const safe_browsing::PrefixSet& prefix_set,
                            const std::vector<SBPrefix> &prefixes) {
    // The set can generate the prefixes it believes it has, so that's
    // a good starting point.
    std::set<SBPrefix> check(prefixes.begin(), prefixes.end());
    std::vector<SBPrefix> prefixes_copy;
    prefix_set.GetPrefixes(&prefixes_copy);
    EXPECT_EQ(prefixes_copy.size(), check.size());
    EXPECT_TRUE(std::equal(check.begin(), check.end(), prefixes_copy.begin()));

    for (size_t i = 0; i < prefixes.size(); ++i) {
      EXPECT_TRUE(prefix_set.Exists(prefixes[i]));

      const SBPrefix left_sibling = prefixes[i] - 1;
      if (check.count(left_sibling) == 0)
        EXPECT_FALSE(prefix_set.Exists(left_sibling));

      const SBPrefix right_sibling = prefixes[i] + 1;
      if (check.count(right_sibling) == 0)
        EXPECT_FALSE(prefix_set.Exists(right_sibling));
    }
  }

  // Generate a |PrefixSet| file from |shared_prefixes_|, store it in
  // a temporary file, and return the filename in |filenamep|.
  // Returns |true| on success.
  bool GetPrefixSetFile(base::FilePath* filenamep) {
    if (!temp_dir_.IsValid() && !temp_dir_.CreateUniqueTempDir())
      return false;

    base::FilePath filename = temp_dir_.path().AppendASCII("PrefixSetTest");

    safe_browsing::PrefixSetBuilder builder(shared_prefixes_);
    if (!builder.GetPrefixSet()->WriteFile(filename))
      return false;

    *filenamep = filename;
    return true;
  }

  // Helper function to read the uint32 value at |offset|, increment it
  // by |inc|, and write it back in place.  |fp| should be opened in
  // r+ mode.
  static void IncrementIntAt(FILE* fp, long offset, int inc) {
    uint32 value = 0;

    ASSERT_NE(-1, fseek(fp, offset, SEEK_SET));
    ASSERT_EQ(1U, fread(&value, sizeof(value), 1, fp));

    value += inc;

    ASSERT_NE(-1, fseek(fp, offset, SEEK_SET));
    ASSERT_EQ(1U, fwrite(&value, sizeof(value), 1, fp));
  }

  // Helper function to re-generated |fp|'s checksum to be correct for
  // the file's contents.  |fp| should be opened in r+ mode.
  static void CleanChecksum(FILE* fp) {
    base::MD5Context context;
    base::MD5Init(&context);

    ASSERT_NE(-1, fseek(fp, 0, SEEK_END));
    long file_size = ftell(fp);

    using base::MD5Digest;
    size_t payload_size = static_cast<size_t>(file_size) - sizeof(MD5Digest);
    size_t digested_size = 0;
    ASSERT_NE(-1, fseek(fp, 0, SEEK_SET));
    while (digested_size < payload_size) {
      char buf[1024];
      size_t nitems = std::min(payload_size - digested_size, sizeof(buf));
      ASSERT_EQ(nitems, fread(buf, 1, nitems, fp));
      base::MD5Update(&context, base::StringPiece(buf, nitems));
      digested_size += nitems;
    }
    ASSERT_EQ(digested_size, payload_size);
    ASSERT_EQ(static_cast<long>(digested_size), ftell(fp));

    base::MD5Digest new_digest;
    base::MD5Final(&new_digest, &context);
    ASSERT_NE(-1, fseek(fp, digested_size, SEEK_SET));
    ASSERT_EQ(1U, fwrite(&new_digest, sizeof(new_digest), 1, fp));
    ASSERT_EQ(file_size, ftell(fp));
  }

  // Open |filename| and increment the uint32 at |offset| by |inc|.
  // Then re-generate the checksum to account for the new contents.
  void ModifyAndCleanChecksum(const base::FilePath& filename, long offset,
                              int inc) {
    int64 size_64;
    ASSERT_TRUE(base::GetFileSize(filename, &size_64));

    base::ScopedFILE file(base::OpenFile(filename, "r+b"));
    IncrementIntAt(file.get(), offset, inc);
    CleanChecksum(file.get());
    file.reset();

    int64 new_size_64;
    ASSERT_TRUE(base::GetFileSize(filename, &new_size_64));
    ASSERT_EQ(new_size_64, size_64);
  }

  // Tests should not modify this shared resource.
  static std::vector<SBPrefix> shared_prefixes_;

  base::ScopedTempDir temp_dir_;
};

std::vector<SBPrefix> PrefixSetTest::shared_prefixes_;

// Test that a small sparse random input works.
TEST_F(PrefixSetTest, Baseline) {
  safe_browsing::PrefixSetBuilder builder(shared_prefixes_);
  CheckPrefixes(*builder.GetPrefixSet(), shared_prefixes_);
}

// Test that the empty set doesn't appear to have anything in it.
TEST_F(PrefixSetTest, Empty) {
  const std::vector<SBPrefix> empty;
  safe_browsing::PrefixSetBuilder builder(empty);
  scoped_ptr<safe_browsing::PrefixSet> prefix_set = builder.GetPrefixSet();
  for (size_t i = 0; i < shared_prefixes_.size(); ++i) {
    EXPECT_FALSE(prefix_set->Exists(shared_prefixes_[i]));
  }
}

// Single-element set should work fine.
TEST_F(PrefixSetTest, OneElement) {
  const std::vector<SBPrefix> prefixes(100, 0u);
  safe_browsing::PrefixSetBuilder builder(prefixes);
  scoped_ptr<safe_browsing::PrefixSet> prefix_set = builder.GetPrefixSet();
  EXPECT_FALSE(prefix_set->Exists(static_cast<SBPrefix>(-1)));
  EXPECT_TRUE(prefix_set->Exists(prefixes[0]));
  EXPECT_FALSE(prefix_set->Exists(1u));

  // Check that |GetPrefixes()| returns the same set of prefixes as
  // was passed in.
  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  EXPECT_EQ(1U, prefixes_copy.size());
  EXPECT_EQ(prefixes[0], prefixes_copy[0]);
}

// Edges of the 32-bit integer range.
TEST_F(PrefixSetTest, IntMinMax) {
  std::vector<SBPrefix> prefixes;

  // Using bit patterns rather than portable constants because this
  // really is testing how the entire 32-bit integer range is handled.
  prefixes.push_back(0x00000000);
  prefixes.push_back(0x0000FFFF);
  prefixes.push_back(0x7FFF0000);
  prefixes.push_back(0x7FFFFFFF);
  prefixes.push_back(0x80000000);
  prefixes.push_back(0x8000FFFF);
  prefixes.push_back(0xFFFF0000);
  prefixes.push_back(0xFFFFFFFF);

  std::sort(prefixes.begin(), prefixes.end());
  safe_browsing::PrefixSetBuilder builder(prefixes);
  scoped_ptr<safe_browsing::PrefixSet> prefix_set = builder.GetPrefixSet();

  // Check that |GetPrefixes()| returns the same set of prefixes as
  // was passed in.
  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  ASSERT_EQ(prefixes_copy.size(), prefixes.size());
  EXPECT_TRUE(std::equal(prefixes.begin(), prefixes.end(),
                         prefixes_copy.begin()));
}

// A range with only large deltas.
TEST_F(PrefixSetTest, AllBig) {
  std::vector<SBPrefix> prefixes;

  const unsigned kDelta = 10 * 1000 * 1000;
  for (SBPrefix prefix = kHighBitSet;
       prefix < kHighBitClear; prefix += kDelta) {
    prefixes.push_back(prefix);
  }

  std::sort(prefixes.begin(), prefixes.end());
  safe_browsing::PrefixSetBuilder builder(prefixes);
  scoped_ptr<safe_browsing::PrefixSet> prefix_set = builder.GetPrefixSet();

  // Check that |GetPrefixes()| returns the same set of prefixes as
  // was passed in.
  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  prefixes.erase(std::unique(prefixes.begin(), prefixes.end()), prefixes.end());
  EXPECT_EQ(prefixes_copy.size(), prefixes.size());
  EXPECT_TRUE(std::equal(prefixes.begin(), prefixes.end(),
                         prefixes_copy.begin()));
}

// Use artificial inputs to test various edge cases in Exists().
// Items before the lowest item aren't present.  Items after the
// largest item aren't present.  Create a sequence of items with
// deltas above and below 2^16, and make sure they're all present.
// Create a very long sequence with deltas below 2^16 to test crossing
// |kMaxRun|.
TEST_F(PrefixSetTest, EdgeCases) {
  std::vector<SBPrefix> prefixes;

  // Put in a high-bit prefix.
  SBPrefix prefix = kHighBitSet;
  prefixes.push_back(prefix);

  // Add a sequence with very large deltas.
  unsigned delta = 100 * 1000 * 1000;
  for (int i = 0; i < 10; ++i) {
    prefix += delta;
    prefixes.push_back(prefix);
  }

  // Add a sequence with deltas that start out smaller than the
  // maximum delta, and end up larger.  Also include some duplicates.
  delta = 256 * 256 - 100;
  for (int i = 0; i < 200; ++i) {
    prefix += delta;
    prefixes.push_back(prefix);
    prefixes.push_back(prefix);
    delta++;
  }

  // Add a long sequence with deltas smaller than the maximum delta,
  // so a new index item will be injected.
  delta = 256 * 256 - 1;
  prefix = kHighBitClear - delta * 1000;
  prefixes.push_back(prefix);
  for (int i = 0; i < 1000; ++i) {
    prefix += delta;
    prefixes.push_back(prefix);
    delta--;
  }

  std::sort(prefixes.begin(), prefixes.end());
  safe_browsing::PrefixSetBuilder builder(prefixes);
  scoped_ptr<safe_browsing::PrefixSet> prefix_set = builder.GetPrefixSet();

  // Check that |GetPrefixes()| returns the same set of prefixes as
  // was passed in.
  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  prefixes.erase(std::unique(prefixes.begin(), prefixes.end()), prefixes.end());
  EXPECT_EQ(prefixes_copy.size(), prefixes.size());
  EXPECT_TRUE(std::equal(prefixes.begin(), prefixes.end(),
                         prefixes_copy.begin()));

  // Items before and after the set are not present, and don't crash.
  EXPECT_FALSE(prefix_set->Exists(kHighBitSet - 100));
  EXPECT_FALSE(prefix_set->Exists(kHighBitClear + 100));

  // Check that the set correctly flags all of the inputs, and also
  // check items just above and below the inputs to make sure they
  // aren't present.
  for (size_t i = 0; i < prefixes.size(); ++i) {
    EXPECT_TRUE(prefix_set->Exists(prefixes[i]));

    EXPECT_FALSE(prefix_set->Exists(prefixes[i] - 1));
    EXPECT_FALSE(prefix_set->Exists(prefixes[i] + 1));
  }
}

// Test writing a prefix set to disk and reading it back in.
TEST_F(PrefixSetTest, ReadWrite) {
  base::FilePath filename;

  // Write the sample prefix set out, read it back in, and check all
  // the prefixes.  Leaves the path in |filename|.
  {
    ASSERT_TRUE(GetPrefixSetFile(&filename));
    scoped_ptr<safe_browsing::PrefixSet> prefix_set =
        safe_browsing::PrefixSet::LoadFile(filename);
    ASSERT_TRUE(prefix_set.get());
    CheckPrefixes(*prefix_set, shared_prefixes_);
  }

  // Test writing and reading a very sparse set containing no deltas.
  {
    std::vector<SBPrefix> prefixes;
    prefixes.push_back(kHighBitClear);
    prefixes.push_back(kHighBitSet);

    safe_browsing::PrefixSetBuilder builder(prefixes);
    ASSERT_TRUE(builder.GetPrefixSet()->WriteFile(filename));

    scoped_ptr<safe_browsing::PrefixSet> prefix_set =
        safe_browsing::PrefixSet::LoadFile(filename);
    ASSERT_TRUE(prefix_set.get());
    CheckPrefixes(*prefix_set, prefixes);
  }

  // Test writing and reading an empty set.
  {
    std::vector<SBPrefix> prefixes;
    safe_browsing::PrefixSetBuilder builder(prefixes);
    ASSERT_TRUE(builder.GetPrefixSet()->WriteFile(filename));

    scoped_ptr<safe_browsing::PrefixSet> prefix_set =
        safe_browsing::PrefixSet::LoadFile(filename);
    ASSERT_TRUE(prefix_set.get());
    CheckPrefixes(*prefix_set, prefixes);
  }
}

// Check that |CleanChecksum()| makes an acceptable checksum.
TEST_F(PrefixSetTest, CorruptionHelpers) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  // This will modify data in |index_|, which will fail the digest check.
  base::ScopedFILE file(base::OpenFile(filename, "r+b"));
  IncrementIntAt(file.get(), kPayloadOffset, 1);
  file.reset();
  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());

  // Fix up the checksum and it will read successfully (though the
  // data will be wrong).
  file.reset(base::OpenFile(filename, "r+b"));
  CleanChecksum(file.get());
  file.reset();
  prefix_set = safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_TRUE(prefix_set.get());
}

// Bad |index_| size is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionMagic) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(
      ModifyAndCleanChecksum(filename, kMagicOffset, 1));
  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());
}

// Bad |index_| size is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionVersion) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(
      ModifyAndCleanChecksum(filename, kVersionOffset, 1));
  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());
}

// Bad |index_| size is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionIndexSize) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(
      ModifyAndCleanChecksum(filename, kIndexSizeOffset, 1));
  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());
}

// Bad |deltas_| size is caught by the sanity check.
TEST_F(PrefixSetTest, CorruptionDeltasSize) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  ASSERT_NO_FATAL_FAILURE(
      ModifyAndCleanChecksum(filename, kDeltasSizeOffset, 1));
  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());
}

// Test that the digest catches corruption in the middle of the file
// (in the payload between the header and the digest).
TEST_F(PrefixSetTest, CorruptionPayload) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  base::ScopedFILE file(base::OpenFile(filename, "r+b"));
  ASSERT_NO_FATAL_FAILURE(IncrementIntAt(file.get(), 666, 1));
  file.reset();
  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());
}

// Test corruption in the digest itself.
TEST_F(PrefixSetTest, CorruptionDigest) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  int64 size_64;
  ASSERT_TRUE(base::GetFileSize(filename, &size_64));
  base::ScopedFILE file(base::OpenFile(filename, "r+b"));
  long digest_offset = static_cast<long>(size_64 - sizeof(base::MD5Digest));
  ASSERT_NO_FATAL_FAILURE(IncrementIntAt(file.get(), digest_offset, 1));
  file.reset();
  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());
}

// Test excess data after the digest (fails the size test).
TEST_F(PrefixSetTest, CorruptionExcess) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  // Add some junk to the trunk.
  base::ScopedFILE file(base::OpenFile(filename, "ab"));
  const char buf[] = "im in ur base, killing ur d00dz.";
  ASSERT_EQ(strlen(buf), fwrite(buf, 1, strlen(buf), file.get()));
  file.reset();
  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());
}

// Test that files which had 64-bit size_t are discarded.
TEST_F(PrefixSetTest, SizeTRecovery) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  // Open the file for rewrite.
  base::ScopedFILE file(base::OpenFile(filename, "r+b"));

  // Leave existing magic and version.
  ASSERT_NE(-1, fseek(file.get(), sizeof(uint32) * 2, SEEK_SET));

  // Indicate two index values and two deltas.
  uint32 val = 2;
  ASSERT_EQ(sizeof(val), fwrite(&val, 1, sizeof(val), file.get()));
  ASSERT_EQ(sizeof(val), fwrite(&val, 1, sizeof(val), file.get()));

  // Write two index values with 64-bit "size_t".
  std::pair<SBPrefix, uint64> item;
  memset(&item, 0, sizeof(item));  // Includes any padding.
  item.first = 17;
  item.second = 0;
  ASSERT_EQ(sizeof(item), fwrite(&item, 1, sizeof(item), file.get()));
  item.first = 100042;
  item.second = 1;
  ASSERT_EQ(sizeof(item), fwrite(&item, 1, sizeof(item), file.get()));

  // Write two delta values.
  uint16 delta = 23;
  ASSERT_EQ(sizeof(delta), fwrite(&delta, 1, sizeof(delta), file.get()));
  ASSERT_EQ(sizeof(delta), fwrite(&delta, 1, sizeof(delta), file.get()));

  // Leave space for the digest at the end, and regenerate it.
  base::MD5Digest dummy = { { 0 } };
  ASSERT_EQ(sizeof(dummy), fwrite(&dummy, 1, sizeof(dummy), file.get()));
  ASSERT_TRUE(base::TruncateFile(file.get()));
  CleanChecksum(file.get());
  file.reset();  // Flush updates.

  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_FALSE(prefix_set.get());
}

// Test that a version 1 file is re-ordered correctly on read.
TEST_F(PrefixSetTest, ReadWriteSigned) {
  base::FilePath filename;
  ASSERT_TRUE(GetPrefixSetFile(&filename));

  // Open the file for rewrite.
  base::ScopedFILE file(base::OpenFile(filename, "r+b"));

  // Leave existing magic.
  ASSERT_NE(-1, fseek(file.get(), sizeof(uint32), SEEK_SET));

  // Version 1.
  uint32 version = 1;
  ASSERT_EQ(sizeof(version), fwrite(&version, 1, sizeof(version), file.get()));

  // Indicate two index values and two deltas.
  uint32 val = 2;
  ASSERT_EQ(sizeof(val), fwrite(&val, 1, sizeof(val), file.get()));
  ASSERT_EQ(sizeof(val), fwrite(&val, 1, sizeof(val), file.get()));

  std::pair<int32, uint32> item;
  memset(&item, 0, sizeof(item));  // Includes any padding.
  item.first = -1000;
  item.second = 0;
  ASSERT_EQ(sizeof(item), fwrite(&item, 1, sizeof(item), file.get()));
  item.first = 1000;
  item.second = 1;
  ASSERT_EQ(sizeof(item), fwrite(&item, 1, sizeof(item), file.get()));

  // Write two delta values.
  uint16 delta = 23;
  ASSERT_EQ(sizeof(delta), fwrite(&delta, 1, sizeof(delta), file.get()));
  ASSERT_EQ(sizeof(delta), fwrite(&delta, 1, sizeof(delta), file.get()));

  // Leave space for the digest at the end, and regenerate it.
  base::MD5Digest dummy = { { 0 } };
  ASSERT_EQ(sizeof(dummy), fwrite(&dummy, 1, sizeof(dummy), file.get()));
  ASSERT_TRUE(base::TruncateFile(file.get()));
  CleanChecksum(file.get());
  file.reset();  // Flush updates.

  scoped_ptr<safe_browsing::PrefixSet> prefix_set =
      safe_browsing::PrefixSet::LoadFile(filename);
  ASSERT_TRUE(prefix_set.get());

  // |Exists()| uses |std::upper_bound()| to find a starting point, which
  // assumes |index_| is sorted.  Depending on how |upper_bound()| is
  // implemented, if the actual list is sorted by |int32|, then one of these
  // test pairs should fail.
  EXPECT_TRUE(prefix_set->Exists(1000u));
  EXPECT_TRUE(prefix_set->Exists(1023u));
  EXPECT_TRUE(prefix_set->Exists(static_cast<uint32>(-1000)));
  EXPECT_TRUE(prefix_set->Exists(static_cast<uint32>(-1000 + 23)));

  std::vector<SBPrefix> prefixes_copy;
  prefix_set->GetPrefixes(&prefixes_copy);
  EXPECT_EQ(prefixes_copy.size(), 4u);
  EXPECT_EQ(prefixes_copy[0], 1000u);
  EXPECT_EQ(prefixes_copy[1], 1023u);
  EXPECT_EQ(prefixes_copy[2], static_cast<uint32>(-1000));
  EXPECT_EQ(prefixes_copy[3], static_cast<uint32>(-1000 + 23));
}

}  // namespace
