// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/pickle.h"
#include "base/values.h"
#include "chrome/common/extensions/extension_messages.h"
#include "extensions/common/permissions/manifest_permission.h"
#include "extensions/common/permissions/manifest_permission_set.h"
#include "ipc/ipc_message.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

class MockManifestPermission : public ManifestPermission {
 public:
  MockManifestPermission(const std::string& name)
      : name_(name) {
  }

  virtual std::string name() const OVERRIDE {
    return name_;
  }

  virtual std::string id() const OVERRIDE {
    return name();
  }

  virtual bool HasMessages() const OVERRIDE {
    return false;
  }

  virtual PermissionMessages GetMessages() const OVERRIDE {
    return PermissionMessages();
  }

  virtual bool FromValue(const base::Value* value) OVERRIDE {
    return false;
  }

  virtual scoped_ptr<base::Value> ToValue() const OVERRIDE {
    return scoped_ptr<base::Value>(base::Value::CreateNullValue());
  }

  virtual ManifestPermission* Clone() const OVERRIDE {
    return new MockManifestPermission(name_);
  }

  virtual ManifestPermission* Diff(const ManifestPermission* rhs)
      const OVERRIDE {
    const MockManifestPermission* other =
        static_cast<const MockManifestPermission*>(rhs);
    EXPECT_EQ(name_, other->name_);
    return NULL;
  }

  virtual ManifestPermission* Union(const ManifestPermission* rhs)
      const OVERRIDE {
    const MockManifestPermission* other =
        static_cast<const MockManifestPermission*>(rhs);
    EXPECT_EQ(name_, other->name_);
    return new MockManifestPermission(name_);
  }

  virtual ManifestPermission* Intersect(const ManifestPermission* rhs)
      const OVERRIDE {
    const MockManifestPermission* other =
        static_cast<const MockManifestPermission*>(rhs);
    EXPECT_EQ(name_, other->name_);
    return new MockManifestPermission(name_);
  }

  virtual bool Contains(const ManifestPermission* rhs) const OVERRIDE {
    const MockManifestPermission* other =
        static_cast<const MockManifestPermission*>(rhs);
    EXPECT_EQ(name_, other->name_);
    return true;
  }

  virtual bool Equal(const ManifestPermission* rhs) const OVERRIDE {
    const MockManifestPermission* other =
        static_cast<const MockManifestPermission*>(rhs);
    EXPECT_EQ(name_, other->name_);
    return true;
  }

  virtual void Write(IPC::Message* m) const OVERRIDE {
    IPC::WriteParam(m, name_);
  }

  virtual bool Read(const IPC::Message* m, PickleIterator* iter) OVERRIDE {
    std::string read_name;
    bool result = IPC::ReadParam(m, iter, &read_name);
    if (!result)
      return result;
    EXPECT_EQ(read_name, name_);
    return true;
  }

  virtual void Log(std::string* log) const OVERRIDE {
  }

 private:
  std::string name_;
};

TEST(ManifestPermissionSetTest, General) {
  ManifestPermissionSet set;
  set.insert(new MockManifestPermission("p1"));
  set.insert(new MockManifestPermission("p2"));
  set.insert(new MockManifestPermission("p3"));
  set.insert(new MockManifestPermission("p4"));
  set.insert(new MockManifestPermission("p5"));

  EXPECT_EQ(set.find("p1")->id(), "p1");
  EXPECT_TRUE(set.find("p10") == set.end());

  EXPECT_EQ(set.size(), 5u);

  EXPECT_EQ(set.erase("p1"), 1u);
  EXPECT_EQ(set.size(), 4u);

  EXPECT_EQ(set.erase("p1"), 0u);
  EXPECT_EQ(set.size(), 4u);
}

TEST(ManifestPermissionSetTest, CreateUnion) {
  ManifestPermissionSet permissions1;
  ManifestPermissionSet permissions2;
  ManifestPermissionSet expected_permissions;
  ManifestPermissionSet result;

  ManifestPermission* permission = new MockManifestPermission("p3");

  // Union with an empty set.
  permissions1.insert(new MockManifestPermission("p1"));
  permissions1.insert(new MockManifestPermission("p2"));
  permissions1.insert(permission->Clone());
  expected_permissions.insert(new MockManifestPermission("p1"));
  expected_permissions.insert(new MockManifestPermission("p2"));
  expected_permissions.insert(permission);

  ManifestPermissionSet::Union(permissions1, permissions2, &result);

  EXPECT_TRUE(permissions1.Contains(permissions2));
  EXPECT_TRUE(permissions1.Contains(result));
  EXPECT_FALSE(permissions2.Contains(permissions1));
  EXPECT_FALSE(permissions2.Contains(result));
  EXPECT_TRUE(result.Contains(permissions1));
  EXPECT_TRUE(result.Contains(permissions2));

  EXPECT_EQ(expected_permissions, result);

  // Now use a real second set.
  permissions2.insert(new MockManifestPermission("p1"));
  permissions2.insert(new MockManifestPermission("p2"));
  permissions2.insert(new MockManifestPermission("p33"));
  permissions2.insert(new MockManifestPermission("p4"));
  permissions2.insert(new MockManifestPermission("p5"));

  expected_permissions.insert(new MockManifestPermission("p1"));
  expected_permissions.insert(new MockManifestPermission("p2"));
  expected_permissions.insert(new MockManifestPermission("p3"));
  expected_permissions.insert(new MockManifestPermission("p4"));
  expected_permissions.insert(new MockManifestPermission("p5"));
  expected_permissions.insert(new MockManifestPermission("p33"));

  ManifestPermissionSet::Union(permissions1, permissions2, &result);

  {
    ManifestPermissionSet set1;
    set1.insert(new MockManifestPermission("p1"));
    set1.insert(new MockManifestPermission("p2"));
    ManifestPermissionSet set2;
    set2.insert(new MockManifestPermission("p3"));

    EXPECT_FALSE(set1.Contains(set2));
    EXPECT_FALSE(set2.Contains(set1));
  }

  EXPECT_FALSE(permissions1.Contains(permissions2));
  EXPECT_FALSE(permissions1.Contains(result));
  EXPECT_FALSE(permissions2.Contains(permissions1));
  EXPECT_FALSE(permissions2.Contains(result));
  EXPECT_TRUE(result.Contains(permissions1));
  EXPECT_TRUE(result.Contains(permissions2));

  EXPECT_EQ(expected_permissions, result);
}

TEST(ManifestPermissionSetTest, CreateIntersection) {
  ManifestPermissionSet permissions1;
  ManifestPermissionSet permissions2;
  ManifestPermissionSet expected_permissions;
  ManifestPermissionSet result;

  // Intersection with an empty set.
  permissions1.insert(new MockManifestPermission("p1"));
  permissions1.insert(new MockManifestPermission("p2"));
  permissions1.insert(new MockManifestPermission("p3"));

  ManifestPermissionSet::Intersection(permissions1, permissions2, &result);
  EXPECT_TRUE(permissions1.Contains(result));
  EXPECT_TRUE(permissions2.Contains(result));
  EXPECT_TRUE(permissions1.Contains(permissions2));
  EXPECT_FALSE(permissions2.Contains(permissions1));
  EXPECT_FALSE(result.Contains(permissions1));
  EXPECT_TRUE(result.Contains(permissions2));

  EXPECT_TRUE(result.empty());
  EXPECT_EQ(expected_permissions, result);

  // Now use a real second set.
  permissions2.insert(new MockManifestPermission("p1"));
  permissions2.insert(new MockManifestPermission("p3"));
  permissions2.insert(new MockManifestPermission("p4"));
  permissions2.insert(new MockManifestPermission("p5"));

  expected_permissions.insert(new MockManifestPermission("p1"));
  expected_permissions.insert(new MockManifestPermission("p3"));

  ManifestPermissionSet::Intersection(permissions1, permissions2, &result);

  EXPECT_TRUE(permissions1.Contains(result));
  EXPECT_TRUE(permissions2.Contains(result));
  EXPECT_FALSE(permissions1.Contains(permissions2));
  EXPECT_FALSE(permissions2.Contains(permissions1));
  EXPECT_FALSE(result.Contains(permissions1));
  EXPECT_FALSE(result.Contains(permissions2));

  EXPECT_EQ(expected_permissions, result);
}

TEST(ManifestPermissionSetTest, CreateDifference) {
  ManifestPermissionSet permissions1;
  ManifestPermissionSet permissions2;
  ManifestPermissionSet expected_permissions;
  ManifestPermissionSet result;

  // Difference with an empty set.
  permissions1.insert(new MockManifestPermission("p1"));
  permissions1.insert(new MockManifestPermission("p2"));
  permissions1.insert(new MockManifestPermission("p3"));

  ManifestPermissionSet::Difference(permissions1, permissions2, &result);

  EXPECT_EQ(permissions1, result);

  // Now use a real second set.
  permissions2.insert(new MockManifestPermission("p1"));
  permissions2.insert(new MockManifestPermission("p2"));
  permissions2.insert(new MockManifestPermission("p4"));
  permissions2.insert(new MockManifestPermission("p5"));
  permissions2.insert(new MockManifestPermission("p6"));

  expected_permissions.insert(new MockManifestPermission("p3"));

  ManifestPermissionSet::Difference(permissions1, permissions2, &result);

  EXPECT_TRUE(permissions1.Contains(result));
  EXPECT_FALSE(permissions2.Contains(result));

  EXPECT_EQ(expected_permissions, result);

  // |result| = |permissions1| - |permissions2| -->
  //   |result| intersect |permissions2| == empty_set
  ManifestPermissionSet result2;
  ManifestPermissionSet::Intersection(result, permissions2, &result2);
  EXPECT_TRUE(result2.empty());
}

}  // namespace extensions
