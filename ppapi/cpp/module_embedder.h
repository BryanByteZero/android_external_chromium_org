// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_MODULE_EMBEDDER_H_
#define PPAPI_CPP_MODULE_EMBEDDER_H_

/// @file
/// This file defines the APIs for creating a Module object.
namespace pp {

class Module;

/// This function creates the <code>pp::Module</code> object associated with
/// this module.
///
/// <strong>Note: </strong>NaCl module developers must implement this function.
///
/// @return Returns the module if it was successfully created, or NULL on
/// failure. Upon failure, the module will be unloaded.
pp::Module* CreateModule();

}  // namespace pp

#endif  // PPAPI_CPP_MODULE_EMBEDDER_H_
