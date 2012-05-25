// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_SEARCH_RESULT_H_
#define UI_APP_LIST_SEARCH_RESULT_H_
#pragma once

#include <vector>

#include "base/basictypes.h"
#include "base/string16.h"
#include "ui/app_list/app_list_export.h"
#include "ui/base/models/list_model.h"
#include "ui/base/range/range.h"
#include "ui/gfx/image/image_skia.h"

namespace app_list {

// SearchResult consists of an icon, title text and details text. Title and
// details text can have tagged ranges that are displayed differently from
// default style.
class APP_LIST_EXPORT SearchResult {
 public:
  // A tagged range in search result text.
  struct Tag {
    // Similar to ACMatchClassification::Style, the style values are not
    // mutually exclusive.
    enum Style {
      NONE  = 0,
      URL   = 1 << 0,
      MATCH = 1 << 1,
      DIM   = 1 << 2,
    };

    Tag(int styles, size_t start, size_t end)
        : styles(styles),
          range(start, end) {
    }

    int styles;
    ui::Range range;
  };
  typedef std::vector<Tag> Tags;

  SearchResult();
  virtual ~SearchResult();

  const gfx::ImageSkia& icon() const { return icon_; }
  void set_icon(const gfx::ImageSkia& icon) { icon_ = icon; }

  const string16& title() const { return title_; }
  void set_title(const string16& title) { title_ = title;}

  const Tags& title_tags() const { return title_tags_; }
  void set_title_tags(const Tags& tags) { title_tags_ = tags; }

  const string16& details() const { return details_; }
  void set_details(const string16& details) { details_ = details; }

  const Tags& details_tags() const { return details_tags_; }
  void set_details_tags(const Tags& tags) { details_tags_ = tags; }

 private:
  gfx::ImageSkia icon_;

  string16 title_;
  Tags title_tags_;

  string16 details_;
  Tags details_tags_;

  DISALLOW_COPY_AND_ASSIGN(SearchResult);
};

}  // namespace app_list

#endif  // UI_APP_LIST_SEARCH_RESULT_H_
