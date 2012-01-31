// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_TESTS_TEST_EXCEPTION_H_
#define PPAPI_TESTS_TEST_EXCEPTION_H_

#include "ppapi/tests/test_case.h"

class TestException : public TestCase {
 public:
  explicit TestException(TestingInstance* instance);

  // TestCase implementation.
  virtual bool Init();
  virtual void RunTests(const std::string& filter);

 private:
  std::string TestCatchException();
};

#endif  // PPAPI_TESTS_TEST_EXCEPTION_H_
