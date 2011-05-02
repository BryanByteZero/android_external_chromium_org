// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_ATTRIBUTED_STRING_CODER_MAC_H_
#define CHROME_COMMON_ATTRIBUTED_STRING_CODER_MAC_H_

#include <set>

#include "base/memory/scoped_ptr.h"
#include "base/string16.h"
#include "content/common/font_descriptor_mac.h"
#include "ipc/ipc_message_utils.h"
#include "ui/base/range/range.h"

#if __OBJC__
@class NSAttributedString;
@class NSDictionary;
#else
class NSAttributedString;
class NSDictionary;
#endif

namespace mac {

// This class will serialize the font information of an NSAttributedString so
// that it can be sent over IPC. This class only stores the information of the
// NSFontAttributeName. The motive is that of security: using NSArchiver and
// friends to send objects from the renderer to the browser could lead to
// deserialization of arbitrary objects. This class restricts serialization to
// a specific object class and specific attributes of that object.
class AttributedStringCoder {
 public:
  // A C++ IPC-friendly representation of the NSFontAttributeName attribute
  // set.
  class FontAttribute {
   public:
    FontAttribute(NSDictionary* ns_attributes, ui::Range effective_range);
    FontAttribute(FontDescriptor font, ui::Range range);
    FontAttribute();
    ~FontAttribute();

    // Creates an autoreleased NSDictionary that can be attached to an
    // NSAttributedString.
    NSDictionary* ToAttributesDictionary() const;

    // Whether or not the attribute should be placed in the EncodedString. This
    // can return false, e.g. if the Cocoa-based constructor can't find any
    // information to encode.
    bool ShouldEncode() const;

    // Accessors:
    FontDescriptor font_descriptor() const { return font_descriptor_; }
    ui::Range effective_range() const { return effective_range_; }

   private:
    FontDescriptor font_descriptor_;
    ui::Range effective_range_;
  };

  // A class that contains the pertinent information from an NSAttributedString,
  // which can be serialized over IPC.
  class EncodedString {
   public:
    explicit EncodedString(string16 string);
    EncodedString();
    ~EncodedString();

    // Accessors:
    string16 string() const { return string_; }
    const std::vector<FontAttribute>& attributes() const {
      return attributes_;
    }
    std::vector<FontAttribute>* attributes() { return &attributes_; }

   private:
    // The plain-text string.
    string16 string_;
    // The set of attributes that style |string_|.
    std::vector<FontAttribute> attributes_;
  };

  // Takes an NSAttributedString, extracts the pertinent attributes, and returns
  // an object that represents it. Caller owns the result.
  static const EncodedString* Encode(NSAttributedString* str);

  // Returns an autoreleased NSAttributedString from an encoded representation.
  static NSAttributedString* Decode(const EncodedString* str);

 private:
  AttributedStringCoder();
};

}  // namespace mac

// IPC ParamTraits specialization //////////////////////////////////////////////

namespace IPC {

template <>
struct ParamTraits<mac::AttributedStringCoder::EncodedString> {
  typedef mac::AttributedStringCoder::EncodedString param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, void** iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct ParamTraits<mac::AttributedStringCoder::FontAttribute> {
  typedef mac::AttributedStringCoder::FontAttribute param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, void** iter, param_type* r);
  static void Log(const param_type& p, std::string* l);
};

}  // namespace IPC

#endif  // CHROME_COMMON_ATTRIBUTED_STRING_CODER_MAC_H_
