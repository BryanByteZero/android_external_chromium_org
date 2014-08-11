// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/libaddressinput/src/cpp/src/util/json.h"

#include <map>
#include <utility>

#include "base/basictypes.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/stl_util.h"
#include "base/values.h"

namespace i18n {
namespace addressinput {

namespace {

// Returns |json| parsed into a JSON dictionary. Sets |parser_error| to true if
// parsing failed.
::scoped_ptr<const base::DictionaryValue> Parse(const std::string& json,
                                                bool* parser_error) {
  DCHECK(parser_error);
  ::scoped_ptr<const base::DictionaryValue> result;

  // |json| is converted to a |c_str()| here because rapidjson and other parts
  // of the standalone library use char* rather than std::string.
  ::scoped_ptr<const base::Value> parsed(base::JSONReader::Read(json.c_str()));
  *parser_error = !parsed || !parsed->IsType(base::Value::TYPE_DICTIONARY);

  if (*parser_error)
    result.reset(new base::DictionaryValue);
  else
    result.reset(static_cast<const base::DictionaryValue*>(parsed.release()));

  return result.Pass();
}

}  // namespace

// Implementation of JSON parser for libaddressinput using JSON parser in
// Chrome.
class Json::JsonImpl {
 public:
  explicit JsonImpl(const std::string& json)
      : owned_(Parse(json, &parser_error_)),
        dict_(*owned_) {}

  ~JsonImpl() { STLDeleteElements(&sub_dicts_); }

  bool parser_error() const { return parser_error_; }

  const std::vector<const Json*>& GetSubDictionaries() {
    if (sub_dicts_.empty()) {
      for (base::DictionaryValue::Iterator it(dict_); !it.IsAtEnd();
           it.Advance()) {
        if (it.value().IsType(base::Value::TYPE_DICTIONARY)) {
          const base::DictionaryValue* sub_dict = NULL;
          it.value().GetAsDictionary(&sub_dict);
          sub_dicts_.push_back(new Json(new JsonImpl(*sub_dict)));
        }
      }
    }
    return sub_dicts_;
  }

  bool GetStringValueForKey(const std::string& key, std::string* value) const {
    return dict_.GetStringWithoutPathExpansion(key, value);
  }

 private:
  explicit JsonImpl(const base::DictionaryValue& dict)
      : parser_error_(false), dict_(dict) {}

  const ::scoped_ptr<const base::DictionaryValue> owned_;
  bool parser_error_;
  const base::DictionaryValue& dict_;
  std::vector<const Json*> sub_dicts_;

  DISALLOW_COPY_AND_ASSIGN(JsonImpl);
};

Json::Json() {}

Json::~Json() {}

bool Json::ParseObject(const std::string& json) {
  DCHECK(!impl_);
  impl_.reset(new JsonImpl(json));
  if (impl_->parser_error())
    impl_.reset();
  return !!impl_;
}

const std::vector<const Json*>& Json::GetSubDictionaries() const {
  return impl_->GetSubDictionaries();
}

bool Json::GetStringValueForKey(const std::string& key,
                                std::string* value) const {
  return impl_->GetStringValueForKey(key, value);
}

Json::Json(JsonImpl* impl) : impl_(impl) {}

}  // namespace addressinput
}  // namespace i18n
