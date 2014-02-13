// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ime/input_method_menu_manager.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {
namespace ime {

TEST(InputMethodMenuManagerTest, TestUninitializedGet) {
  EXPECT_DEATH(InputMethodMenuManager::Get(),
               "g_input_method_menu_manager not initialized");
}

TEST(InputMethodMenuManagerTest, TestUninitializedShutdown) {
  EXPECT_DEATH(InputMethodMenuManager::Shutdown(),
               "g_input_method_menu_manager not initialized");
}

TEST(InputMethodMenuManagerTest, TestNormalOperation) {
  InputMethodMenuManager::Initialize();
  EXPECT_TRUE(NULL != InputMethodMenuManager::Get());
  InputMethodMenuManager::Shutdown();
}

class MockObserver : public InputMethodMenuManager::Observer {
 public:
  MockObserver() : input_method_menu_item_changed_count_(0) {}
  virtual ~MockObserver() {}

  // Called when the list of menu items is changed.
  virtual void InputMethodMenuItemChanged(
      InputMethodMenuManager* manager) OVERRIDE {
    input_method_menu_item_changed_count_++;
  }
  int input_method_menu_item_changed_count_;
};

class InputMethodMenuManagerStatefulTest : public testing::Test{
 public:
  InputMethodMenuManagerStatefulTest()
      : observer_(new MockObserver()) {}
  virtual ~InputMethodMenuManagerStatefulTest() {}
  virtual void SetUp() OVERRIDE {
    InputMethodMenuManager::Initialize();
    menu_manager_ = InputMethodMenuManager::Get();
    menu_manager_->AddObserver(observer_.get());
  }

  virtual void TearDown() OVERRIDE {
    InputMethodMenuManager::Shutdown();
  }

  InputMethodMenuManager* menu_manager_;
  scoped_ptr<MockObserver> observer_;
};

TEST_F(InputMethodMenuManagerStatefulTest, AddAndObserve) {
  EXPECT_EQ(observer_->input_method_menu_item_changed_count_, 0);
  menu_manager_->SetCurrentInputMethodMenuItemList(InputMethodMenuItemList());
  EXPECT_EQ(observer_->input_method_menu_item_changed_count_, 1);
}

TEST_F(InputMethodMenuManagerStatefulTest, AddAndCheckExists) {
  InputMethodMenuItemList list;
  list.push_back(InputMethodMenuItem("key1", "label1", false, false));
  list.push_back(InputMethodMenuItem("key2", "label2", false, false));
  menu_manager_->SetCurrentInputMethodMenuItemList(list);
  EXPECT_EQ(menu_manager_->GetCurrentInputMethodMenuItemList().size(), 2U);
  EXPECT_EQ(
      menu_manager_->GetCurrentInputMethodMenuItemList().at(0).ToString(),
      "key=key1, label=label1, "
      "is_selection_item=0, is_selection_item_checked=0");
  EXPECT_EQ(
      menu_manager_->GetCurrentInputMethodMenuItemList().at(1).ToString(),
      "key=key2, label=label2, "
      "is_selection_item=0, is_selection_item_checked=0");

  EXPECT_TRUE(menu_manager_->HasInputMethodMenuItemForKey("key1"));
  EXPECT_TRUE(menu_manager_->HasInputMethodMenuItemForKey("key2"));
  EXPECT_FALSE(menu_manager_->HasInputMethodMenuItemForKey("key-not-exist"));
}

}  // namespace ime
}  // namespace ash
