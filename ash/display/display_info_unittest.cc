// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_info.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace ash {
namespace internal {

typedef testing::Test DisplayInfoTest;

TEST_F(DisplayInfoTest, CreateFromSpec) {
  DisplayInfo info = DisplayInfo::CreateFromSpecWithID("200x100", 10);
  EXPECT_EQ(10, info.id());
  EXPECT_EQ("0,0 200x100", info.bounds_in_pixel().ToString());
  EXPECT_EQ("200x100", info.size_in_pixel().ToString());
  EXPECT_EQ(DisplayInfo::ROTATE_0, info.rotation());
  EXPECT_EQ("0,0,0,0", info.overscan_insets_in_dip().ToString());
  EXPECT_EQ(1.0f, info.ui_scale());

  info = DisplayInfo::CreateFromSpecWithID("10+20-300x400*2/o", 10);
  EXPECT_EQ("10,20 300x400", info.bounds_in_pixel().ToString());
  EXPECT_EQ("286x380", info.size_in_pixel().ToString());
  EXPECT_EQ(DisplayInfo::ROTATE_0, info.rotation());
  EXPECT_EQ("5,3,5,3", info.overscan_insets_in_dip().ToString());

  info = DisplayInfo::CreateFromSpecWithID("10+20-300x400*2/ob", 10);
  EXPECT_EQ("10,20 300x400", info.bounds_in_pixel().ToString());
  EXPECT_EQ("286x380", info.size_in_pixel().ToString());
  EXPECT_EQ(DisplayInfo::ROTATE_0, info.rotation());
  EXPECT_EQ("5,3,5,3", info.overscan_insets_in_dip().ToString());

  info = DisplayInfo::CreateFromSpecWithID("10+20-300x400*2/or", 10);
  EXPECT_EQ("10,20 300x400", info.bounds_in_pixel().ToString());
  EXPECT_EQ("380x286", info.size_in_pixel().ToString());
  EXPECT_EQ(DisplayInfo::ROTATE_90, info.rotation());
  // TODO(oshima): This should be rotated too. Fix this.
  EXPECT_EQ("5,3,5,3", info.overscan_insets_in_dip().ToString());

  info = DisplayInfo::CreateFromSpecWithID("10+20-300x400*2/l@1.5", 10);
  EXPECT_EQ("10,20 300x400", info.bounds_in_pixel().ToString());
  EXPECT_EQ(DisplayInfo::ROTATE_270, info.rotation());
  EXPECT_EQ(1.5f, info.ui_scale());
}

}  // namespace internal
}  // namespace ash
