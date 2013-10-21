// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file implements the native methods of
// org.content.chromium.app.LinkerTests
// Unlike the content of linker_jni.cc, it is part of the content library and
// can
// thus use base/ and the C++ STL.

#include "content/shell/android/linker_test_apk/content_linker_test_linker_tests.h"

#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string>

#include "base/basictypes.h"
#include "base/debug/proc_maps_linux.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"

#include "jni/LinkerTests_jni.h"

namespace content {

namespace {

using base::debug::MappedMemoryRegion;

jboolean RunChecks(bool in_browser_process, bool need_relros) {

  // IMPORTANT NOTE: The Python test control script reads the logcat for
  // lines like:
  //   BROWSER_LINKER_TEST: <status>
  //   RENDERER_LINKER_TEST: <status>
  //
  // Where <status> can be either SUCCESS or FAIL. Other lines starting
  // with the same prefixes, but not using SUCCESS or FAIL are ignored.
  const char* prefix =
      in_browser_process ? "BROWSER_LINKER_TEST: " : "RENDERER_LINKER_TEST: ";

  // The RELRO section(s), after being copied into an ashmem region, will
  // appear in /proc/self/maps as a mapped memory region for a file name
  // that begins with the following prefix.
  //
  // Note that the full name will be something like:
  //   "/dev/ashmem/RELRO:<libname> (deleted)"
  //
  // Where <libname> is the library name and '(deleted)' is actually
  // added by the kernel to indicate there is no corresponding file
  // on the filesystem.
  //
  // For regular builds, there is only one library, and thus one RELRO
  // section, but for the component build, there are several libraries,
  // each one with its own RELRO.
  static const char kRelroSectionPrefix[] = "/dev/ashmem/RELRO:";

  // Parse /proc/self/maps and builds a list of region mappings in this
  // process.
  std::string maps;
  base::debug::ReadProcMaps(&maps);
  if (maps.empty()) {
    LOG(ERROR) << prefix << "FAIL Cannot parse /proc/self/maps";
    return false;
  }

  std::vector<MappedMemoryRegion> regions;
  base::debug::ParseProcMaps(maps, &regions);
  if (regions.empty()) {
    LOG(ERROR) << prefix << "FAIL Cannot read memory mappings in this process";
    return false;
  }

  size_t num_shared_relros = 0;
  size_t num_bad_shared_relros = 0;

  for (size_t n = 0; n < regions.size(); ++n) {
    MappedMemoryRegion& region = regions[n];

    if (region.path.find(kRelroSectionPrefix) != 0) {
      // Ignore any mapping that isn't a shared RELRO.
      continue;
    }

    num_shared_relros++;

    void* region_start = reinterpret_cast<void*>(region.start);
    void* region_end = reinterpret_cast<void*>(region.end);

    // Check that it is mapped read-only.
    const uint8 expected_flags = MappedMemoryRegion::READ;
    const uint8 expected_mask = MappedMemoryRegion::READ |
                                MappedMemoryRegion::WRITE |
                                MappedMemoryRegion::EXECUTE;

    uint8 region_flags = region.permissions & expected_mask;
    if (region_flags != expected_flags) {
      LOG(ERROR)
          << prefix
          << base::StringPrintf(
                 "Shared RELRO section at %p-%p is not mapped read-only. "
                 "Protection flags are %d (%d expected)!",
                 region_start,
                 region_end,
                 region_flags,
                 expected_flags);
      num_bad_shared_relros++;
      continue;
    }

    // Check that trying to remap it read-write fails with EACCES
    size_t region_size = region.end - region.start;
    int ret = ::mprotect(region_start, region_size, PROT_READ | PROT_WRITE);
    if (ret != -1) {
      LOG(ERROR)
          << prefix
          << base::StringPrintf(
                 "Shared RELRO section at %p-%p could be remapped read-write!?",
                 region_start,
                 region_end);
      num_bad_shared_relros++;
      // Just in case.
      ::mprotect(region_start, region_size, PROT_READ);
    } else if (errno != EACCES) {
      LOG(ERROR) << prefix << base::StringPrintf(
                                  "Shared RELRO section at %p-%p failed "
                                  "read-write mprotect with "
                                  "unexpected error %d (EACCES:%d wanted): %s",
                                  region_start,
                                  region_end,
                                  errno,
                                  EACCES,
                                  strerror(errno));
      num_bad_shared_relros++;
    }
  }

  LOG(INFO)
      << prefix
      << base::StringPrintf(
             "There are %d shared RELRO sections in this process, %d are bad",
             num_shared_relros,
             num_bad_shared_relros);

  if (num_bad_shared_relros > 0) {
    LOG(ERROR) << prefix << "FAIL Bad Relros sections in this process";
    return false;
  }

  if (need_relros) {
    if (num_shared_relros == 0) {
      LOG(ERROR) << prefix
                 << "FAIL Missing shared RELRO sections in this process!";
      return false;
    }
  } else {
    if (num_shared_relros > 0) {
      LOG(ERROR) << prefix << "FAIL Unexpected " << num_shared_relros
                 << " shared RELRO sections in this process!";
      return false;
    }
  }

  LOG(INFO) << prefix << "SUCCESS";
  return true;
}

}  // namespace

jboolean CheckForSharedRelros(JNIEnv* env,
                              jclass clazz,
                              jboolean in_browser_process) {
  return RunChecks(in_browser_process, true);
}

jboolean CheckForNoSharedRelros(JNIEnv* env,
                                jclass clazz,
                                jboolean in_browser_process) {
  return RunChecks(in_browser_process, false);
}

bool RegisterLinkerTestsJni(JNIEnv* env) { return RegisterNativesImpl(env); }

}  // namespace content
