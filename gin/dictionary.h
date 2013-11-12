// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_DICTIONARY_H_
#define GIN_DICTIONARY_H_

#include "gin/converter.h"

namespace gin {

class Dictionary {
 public:
  explicit Dictionary(v8::Isolate* isolate);
  Dictionary(v8::Isolate* isolate, v8::Handle<v8::Object> object);
  ~Dictionary();

  static Dictionary CreateEmpty(v8::Isolate* isolate);

  template<typename T>
  bool Get(const std::string& key, T* out) {
    v8::Handle<v8::Value> val = object_->Get(StringToV8(isolate_, key));
    return ConvertFromV8(val, out);
  }

  template<typename T>
  bool Set(const std::string& key, T val) {
    return object_->Set(StringToV8(isolate_, key), ConvertToV8(isolate_, val));
  }

  v8::Isolate* isolate() const { return isolate_; }

 private:
  friend struct Converter<Dictionary>;

  v8::Isolate* isolate_;
  v8::Handle<v8::Object> object_;
};

template<>
struct Converter<Dictionary> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    Dictionary val);
  static bool FromV8(v8::Handle<v8::Value> val,
                     Dictionary* out);
};

}  // namespace gin

#endif  // GIN_DICTIONARY_H_
