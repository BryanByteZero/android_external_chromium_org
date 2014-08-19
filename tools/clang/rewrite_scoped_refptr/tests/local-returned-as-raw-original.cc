// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "scoped_refptr.h"

struct Foo {
  int dummy;
};

// Case 1: An example of an unsafe conversion, where the object is freed by
// the time the function returns.
Foo* GetBuggyFoo() {
  scoped_refptr<Foo> unsafe(new Foo);
  // FIXME: The tool should rewrite the return type of the function.
  return unsafe;
}
