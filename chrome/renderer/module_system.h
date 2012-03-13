// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_MODULE_SYSTEM_H_
#define CHROME_RENDERER_MODULE_SYSTEM_H_
#pragma once

#include "base/compiler_specific.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/scoped_ptr.h"
#include "base/string_piece.h"
#include "chrome/renderer/native_handler.h"
#include "v8/include/v8.h"

#include <map>
#include <string>

class SourceMap;

// A module system for JS similar to node.js' require() function.
// Each module has three variables in the global scope:
//   - exports, an object returned to dependencies who require() this
//     module.
//   - require, a function that takes a module name as an argument and returns
//     that module's exports object.
//   - requireNative, a function that takes the name of a registered
//     NativeHandler and returns an object that contains the functions the
//     NativeHandler defines.
//
// Each module in a ModuleSystem is executed at most once and its exports
// object cached.
//
// Note that a ModuleSystem must be used only in conjunction with a single
// v8::Context.
// TODO(koz): Rename this to JavaScriptModuleSystem.
class ModuleSystem : public NativeHandler {
 public:
  class SourceMap {
   public:
    virtual v8::Handle<v8::Value> GetSource(const std::string& name) = 0;
    virtual bool Contains(const std::string& name) = 0;
  };

  // |source_map| is a weak pointer.
  explicit ModuleSystem(SourceMap* source_map);
  virtual ~ModuleSystem();

  // Require the specified module. This is the equivalent of calling
  // require('module_name') from the loaded JS files.
  void Require(const std::string& module_name);

  // Register |native_handler| as a potential target for requireNative(), so
  // calls to requireNative(|name|) from JS will return a new object created by
  // |native_handler|.
  void RegisterNativeHandler(const std::string& name,
                             scoped_ptr<NativeHandler> native_handler);

  // Executes |code| in the current context with |name| as the filename.
  void RunString(const std::string& code, const std::string& name);

  // When false |natives_enabled_| causes calls to GetNative() (the basis of
  // requireNative() in JS) to throw an exception.
  void set_natives_enabled(bool natives_enabled) {
    natives_enabled_ = natives_enabled;
  }

 private:
  typedef std::map<std::string, linked_ptr<NativeHandler> > NativeHandlerMap;

  // Ensure that require_ has been evaluated from require.js.
  void EnsureRequireLoaded();

  // Run |code| in the current context with the name |name| used for stack
  // traces.
  v8::Handle<v8::Value> RunString(v8::Handle<v8::String> code,
                                  v8::Handle<v8::String> name);

  // Return the named source file stored in the source map.
  // |args[0]| - the name of a source file in source_map_.
  v8::Handle<v8::Value> GetSource(const v8::Arguments& args);

  // Return an object that contains the native methods defined by the named
  // NativeHandler.
  // |args[0]| - the name of a native handler object.
  v8::Handle<v8::Value> GetNative(const v8::Arguments& args);

  base::StringPiece GetResource(int resource_id);

  // Throws an exception in the calling JS context.
  v8::Handle<v8::Value> ThrowException(const std::string& message);

  // A map from module names to the JS source for that module. GetSource()
  // performs a lookup on this map.
  SourceMap* source_map_;
  NativeHandlerMap native_handler_map_;
  v8::Handle<v8::Function> require_;
  bool natives_enabled_;
};

#endif  // CHROME_RENDERER_MODULE_SYSTEM_H_
