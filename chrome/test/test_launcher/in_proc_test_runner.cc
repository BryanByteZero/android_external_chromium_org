// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/string_util.h"

#include "chrome/test/test_launcher/test_runner.h"

// This version of the test launcher loads a dynamic library containing the
// tests and executes the them in that library. When the test has been run the
// library is unloaded, to ensure atexit handlers are run and static
// initializers will be run again for the next test.

namespace {

const wchar_t* const kLibNameFlag = L"lib";
const wchar_t* const kGTestListTestsFlag = L"gtest_list_tests";

class InProcTestRunner : public tests::TestRunner {
 public:
  explicit InProcTestRunner(const std::wstring& lib_name)
      : lib_name_(lib_name),
        dynamic_lib_(NULL),
        run_test_proc_(NULL),
        uninitialize_proc_(NULL) {
  }

  ~InProcTestRunner() {
    if (!dynamic_lib_)
      return;
    if (uninitialize_proc_ && (uninitialize_proc_)()) {
      LOG(ERROR) << "Uninitialization of " <<
          base::GetNativeLibraryName(lib_name_) << " failed.";
    }
    base::UnloadNativeLibrary(dynamic_lib_);
    LOG(INFO) << "Unloaded " << base::GetNativeLibraryName(lib_name_);
  }

  bool Init() {
    FilePath lib_path;
    CHECK(PathService::Get(base::FILE_EXE, &lib_path));
    lib_path = lib_path.DirName().Append(base::GetNativeLibraryName(lib_name_));

    LOG(INFO) << "Loading '" <<  lib_path.value() << "'";

    dynamic_lib_ = base::LoadNativeLibrary(lib_path);
    if (!dynamic_lib_) {
      LOG(ERROR) << "Failed to load " << lib_path.value();
      return false;
    }

    run_test_proc_ = reinterpret_cast<RunTestProc>(
        base::GetFunctionPointerFromNativeLibrary(dynamic_lib_, "RunTests"));
    if (!run_test_proc_) {
      LOG(ERROR) <<
          "Failed to find RunTest function in " << lib_path.value();
      return false;
    }

    uninitialize_proc_ = reinterpret_cast<UninitializeProc>(
        base::GetFunctionPointerFromNativeLibrary(dynamic_lib_,
                                                  "UninitializeTest"));

    return true;
  }

  // Returns true if the test succeeded, false if it failed.
  bool RunTest(const std::string& test_name) {
    std::string filter_flag = StringPrintf("--gtest_filter=%s",
                                           test_name.c_str());
    char* argv[3];
    argv[0] = const_cast<char*>("");
    argv[1] = const_cast<char*>(filter_flag.c_str());
    // Always enable disabled tests.  This method is not called with disabled
    // tests unless this flag was specified to the test launcher.
    argv[2] = "--gtest_also_run_disabled_tests";
    return RunAsIs(3, argv) == 0;
  }

  // Calls-in to GTest with the arguments we were started with.
  int RunAsIs(int argc, char** argv) {
    return (run_test_proc_)(argc, argv);
  }

 private:
  typedef int (CDECL *RunTestProc)(int, char**);
  typedef int (CDECL *UninitializeProc)();

  std::wstring lib_name_;
  base::NativeLibrary dynamic_lib_;
  RunTestProc run_test_proc_;

  // An optional UnitializeTest method called before the library containing the
  // test is unloaded.
  UninitializeProc uninitialize_proc_;

  DISALLOW_COPY_AND_ASSIGN(InProcTestRunner);
};

class InProcTestRunnerFactory : public tests::TestRunnerFactory {
 public:
  explicit InProcTestRunnerFactory(const std::wstring& lib_name)
      : lib_name_(lib_name) {
  }

  virtual tests::TestRunner* CreateTestRunner() const {
    return new InProcTestRunner(lib_name_);
  }

 private:
  std::wstring lib_name_;

  DISALLOW_COPY_AND_ASSIGN(InProcTestRunnerFactory);
};

}  // namespace

int main(int argc, char** argv) {
  base::AtExitManager at_exit_manager;

  CommandLine::Init(argc, argv);
  const CommandLine* command_line = CommandLine::ForCurrentProcess();
  std::wstring lib_name = command_line->GetSwitchValue(kLibNameFlag);
  if (lib_name.empty()) {
    LOG(ERROR) << "No dynamic library name specified. You must specify one with"
        " the --lib=<lib_name> option.";
    return 1;
  }

  if (command_line->HasSwitch(kGTestListTestsFlag)) {
    InProcTestRunner test_runner(lib_name);
    if (!test_runner.Init())
      return 1;
    return test_runner.RunAsIs(argc, argv);
  }

  InProcTestRunnerFactory test_runner_factory(lib_name);
  return tests::RunTests(test_runner_factory) ? 0 : 1;
}
