// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_PERMISSIONS_PERMISSION_SET_H_
#define EXTENSIONS_COMMON_PERMISSIONS_PERMISSION_SET_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/strings/string16.h"
#include "extensions/common/manifest.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/api_permission_set.h"
#include "extensions/common/permissions/manifest_permission.h"
#include "extensions/common/permissions/manifest_permission_set.h"
#include "extensions/common/url_pattern_set.h"

namespace extensions {
class Extension;

// The PermissionSet is an immutable class that encapsulates an
// extension's permissions. The class exposes set operations for combining and
// manipulating the permissions.
class PermissionSet
    : public base::RefCountedThreadSafe<PermissionSet> {
 public:
  // Creates an empty permission set (e.g. default permissions).
  PermissionSet();

  // Creates a new permission set based on the specified data: the API
  // permissions, manifest key permissions, host permissions, and scriptable
  // hosts. The effective hosts of the newly created permission set will be
  // inferred from the given host permissions.
  PermissionSet(const APIPermissionSet& apis,
                const ManifestPermissionSet& manifest_permissions,
                const URLPatternSet& explicit_hosts,
                const URLPatternSet& scriptable_hosts);

  // Creates a new permission set equal to |set1| - |set2|, passing ownership of
  // the new set to the caller.
  static PermissionSet* CreateDifference(
      const PermissionSet* set1, const PermissionSet* set2);

  // Creates a new permission set equal to the intersection of |set1| and
  // |set2|, passing ownership of the new set to the caller.
  static PermissionSet* CreateIntersection(
      const PermissionSet* set1, const PermissionSet* set2);

  // Creates a new permission set equal to the union of |set1| and |set2|.
  // Passes ownership of the new set to the caller.
  static PermissionSet* CreateUnion(
      const PermissionSet* set1, const PermissionSet* set2);

  bool operator==(const PermissionSet& rhs) const;

  // Returns true if every API or host permission available to |set| is also
  // available to this. In other words, if the API permissions of |set| are a
  // subset of this, and the host permissions in this encompass those in |set|.
  bool Contains(const PermissionSet& set) const;

  // Gets the API permissions in this set as a set of strings.
  std::set<std::string> GetAPIsAsStrings() const;

  // Returns true if this is an empty set (e.g., the default permission set).
  bool IsEmpty() const;

  // Returns true if the set has the specified API permission.
  bool HasAPIPermission(APIPermission::ID permission) const;

  // Returns true if the |extension| explicitly requests access to the given
  // |permission_name|. Note this does not include APIs without no corresponding
  // permission, like "runtime" or "browserAction".
  bool HasAPIPermission(const std::string& permission_name) const;

  // Returns true if the set allows the given permission with the default
  // permission detal.
  bool CheckAPIPermission(APIPermission::ID permission) const;

  // Returns true if the set allows the given permission and permission param.
  bool CheckAPIPermissionWithParam(APIPermission::ID permission,
      const APIPermission::CheckParam* param) const;

  // Returns true if this includes permission to access |origin|.
  bool HasExplicitAccessToOrigin(const GURL& origin) const;

  // Returns true if this permission set includes access to script |url|.
  bool HasScriptableAccessToURL(const GURL& url) const;

  // Returns true if this permission set includes effective access to all
  // origins.
  bool HasEffectiveAccessToAllHosts() const;

  // Returns true if this permission set includes effective access to |url|.
  bool HasEffectiveAccessToURL(const GURL& url) const;

  // Returns true if this permission set effectively represents full access
  // (e.g. native code).
  bool HasEffectiveFullAccess() const;

  const APIPermissionSet& apis() const { return apis_; }

  const ManifestPermissionSet& manifest_permissions() const {
      return manifest_permissions_;
  }

  const URLPatternSet& effective_hosts() const { return effective_hosts_; }

  const URLPatternSet& explicit_hosts() const { return explicit_hosts_; }

  const URLPatternSet& scriptable_hosts() const { return scriptable_hosts_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(PermissionsTest, GetWarningMessages_AudioVideo);
  friend class base::RefCountedThreadSafe<PermissionSet>;

  ~PermissionSet();

  void AddAPIPermission(APIPermission::ID id);

  // Adds permissions implied independently of other context.
  void InitImplicitPermissions();

  // Initializes the effective host permission based on the data in this set.
  void InitEffectiveHosts();

  // The api list is used when deciding if an extension can access certain
  // extension APIs and features.
  APIPermissionSet apis_;

  // The manifest key permission list is used when deciding if an extension
  // can access certain extension APIs and features.
  ManifestPermissionSet manifest_permissions_;

  // The list of hosts that can be accessed directly from the extension.
  // TODO(jstritar): Rename to "hosts_"?
  URLPatternSet explicit_hosts_;

  // The list of hosts that can be scripted by content scripts.
  // TODO(jstritar): Rename to "user_script_hosts_"?
  URLPatternSet scriptable_hosts_;

  // The list of hosts this effectively grants access to.
  URLPatternSet effective_hosts_;
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_PERMISSIONS_PERMISSION_SET_H_
