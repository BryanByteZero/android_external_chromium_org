// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a build test that includes the basic OpenGL ES 2 headers, to make
// sure that the header layout in the NaCl toolchain is correct.  Note that
// unlike the cpp header test files, this is not generated by a script.

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
