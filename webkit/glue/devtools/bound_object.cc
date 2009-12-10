// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "V8Proxy.h"
#include "webkit/glue/devtools/bound_object.h"

BoundObject::BoundObject(
    v8::Handle<v8::Context> context,
    void* v8_this,
    const char* object_name)
    : object_name_(object_name),
      context_(context),
      v8_this_(v8_this) {
  v8::Context::Scope context_scope(context);
  v8::Local<v8::FunctionTemplate> local_template =
      v8::FunctionTemplate::New(WebCore::V8Proxy::checkNewLegal);
  host_template_ = v8::Persistent<v8::FunctionTemplate>::New(local_template);
  host_template_->SetClassName(v8::String::New(object_name));
}

BoundObject::~BoundObject() {
  host_template_.Dispose();
}

void BoundObject::AddProtoFunction(
    const char* name,
    v8::InvocationCallback callback) {
  v8::Context::Scope context_scope(context_);
  v8::Local<v8::Signature> signature = v8::Signature::New(host_template_);
  v8::Local<v8::ObjectTemplate> proto = host_template_->PrototypeTemplate();
  v8::Local<v8::External> v8_this = v8::External::New(v8_this_);
  proto->Set(
      v8::String::New(name),
      v8::FunctionTemplate::New(
          callback,
          v8_this,
          signature),
      static_cast<v8::PropertyAttribute>(v8::DontDelete));
}

void BoundObject::Build() {
  v8::Context::Scope context_scope(context_);
  v8::Local<v8::Function> constructor = host_template_->GetFunction();
  v8::Local<v8::Object> bound_object =
      WebCore::SafeAllocation::newInstance(constructor);

  v8::Handle<v8::Object> global = context_->Global();
  global->Set(v8::String::New(object_name_), bound_object);
}
