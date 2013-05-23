// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_USER_TRAY_USER_H_
#define ASH_SYSTEM_USER_TRAY_USER_H_

#include "ash/session_state_delegate.h"
#include "ash/system/tray/system_tray_item.h"
#include "ash/system/user/user_observer.h"
#include "base/compiler_specific.h"

namespace views {
class ImageView;
class Label;
}

namespace ash {
namespace internal {

namespace tray {
class UserView;
class RoundedImageView;
}

class TrayUser : public SystemTrayItem,
                 public UserObserver {
 public:
  // The given |multiprofile_index| is the number of the user in a multi profile
  // scenario. Index #0 is the running user, the other indices are other
  // logged in users (if there are any). Only index #0 will add an icon to
  // the system tray.
  TrayUser(SystemTray* system_tray, MultiProfileIndex index);
  virtual ~TrayUser();

 private:
  // Overridden from SystemTrayItem.
  virtual views::View* CreateTrayView(user::LoginStatus status) OVERRIDE;
  virtual views::View* CreateDefaultView(user::LoginStatus status) OVERRIDE;
  virtual views::View* CreateDetailedView(user::LoginStatus status) OVERRIDE;
  virtual void DestroyTrayView() OVERRIDE;
  virtual void DestroyDefaultView() OVERRIDE;
  virtual void DestroyDetailedView() OVERRIDE;
  virtual void UpdateAfterLoginStatusChange(user::LoginStatus status) OVERRIDE;
  virtual void UpdateAfterShelfAlignmentChange(
      ShelfAlignment alignment) OVERRIDE;

  // Overridden from UserObserver.
  virtual void OnUserUpdate() OVERRIDE;

  // The user index to use.
  MultiProfileIndex multiprofile_index_;

  tray::UserView* user_;

  // View that contains label and/or avatar.
  views::View* layout_view_;
  tray::RoundedImageView* avatar_;
  views::Label* label_;

  DISALLOW_COPY_AND_ASSIGN(TrayUser);
};

}  // namespace internal
}  // namespace ash

#endif  // ASH_SYSTEM_USER_TRAY_USER_H_
