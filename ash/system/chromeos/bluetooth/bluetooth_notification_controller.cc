// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/chromeos/bluetooth/bluetooth_notification_controller.h"

#include "ash/system/system_notifier.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_device.h"
#include "grit/ash_resources.h"
#include "grit/ash_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/notification.h"
#include "ui/message_center/notification_delegate.h"
#include "ui/message_center/notification_types.h"

using device::BluetoothAdapter;
using device::BluetoothAdapterFactory;
using device::BluetoothDevice;
using message_center::Notification;

namespace {

// Identifier for the pairing notification; the Bluetooth code ensures we
// only receive one pairing request at a time, so a single id is sufficient and
// means we "update" one notification if not handled rather than continually
// bugging the user.
const char kBluetoothDevicePairingNotificationId[] =
    "chrome://settings/bluetooth/pairing";

// Identifier for the notification that a device has been paired with the
// system.
const char kBluetoothDevicePairedNotificationId[] =
    "chrome://settings/bluetooth/paired";

// The BluetoothPairingNotificationDelegate handles user interaction with the
// pairing notification and sending the confirmation, rejection or cancellation
// back to the underlying device.
class BluetoothPairingNotificationDelegate
    : public message_center::NotificationDelegate {
 public:
  BluetoothPairingNotificationDelegate(scoped_refptr<BluetoothAdapter> adapter,
                                       const std::string& address);

 protected:
  virtual ~BluetoothPairingNotificationDelegate();

  // message_center::NotificationDelegate overrides.
  virtual void Display() OVERRIDE;
  virtual void Error() OVERRIDE;
  virtual void Close(bool by_user) OVERRIDE;
  virtual bool HasClickedListener() OVERRIDE;
  virtual void Click() OVERRIDE;
  virtual void ButtonClick(int button_index) OVERRIDE;

 private:
  // Buttons that appear in notifications.
  enum Button {
    BUTTON_ACCEPT,
    BUTTON_REJECT
  };

  // Reference to the underlying Bluetooth Adapter, holding onto this
  // reference ensures the adapter object doesn't go out of scope while we have
  // a pending request and user interaction.
  scoped_refptr<BluetoothAdapter> adapter_;

  // Address of the device being paired.
  const std::string address_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothPairingNotificationDelegate);
};

BluetoothPairingNotificationDelegate::BluetoothPairingNotificationDelegate(
    scoped_refptr<BluetoothAdapter> adapter,
    const std::string& address)
    : adapter_(adapter),
      address_(address) {
}

BluetoothPairingNotificationDelegate::~BluetoothPairingNotificationDelegate() {
}

void BluetoothPairingNotificationDelegate::Display() {
}

void BluetoothPairingNotificationDelegate::Error() {
}

void BluetoothPairingNotificationDelegate::Close(bool by_user) {
  VLOG(1) << "Pairing notification closed. by_user = " << by_user;
  // Ignore notification closes generated as a result of pairing completion.
  if (!by_user)
    return;

  // Cancel the pairing of the device, if the object still exists.
  BluetoothDevice* device = adapter_->GetDevice(address_);
  if (device)
    device->CancelPairing();
}

bool BluetoothPairingNotificationDelegate::HasClickedListener() {
  return false;
}

void BluetoothPairingNotificationDelegate::Click() {
}

void BluetoothPairingNotificationDelegate::ButtonClick(int button_index) {
  VLOG(1) << "Pairing notification, button click: " << button_index;
  // If the device object still exists, send the appropriate response either
  // confirming or rejecting the pairing.
  BluetoothDevice* device = adapter_->GetDevice(address_);
  if (device) {
    switch (button_index) {
      case BUTTON_ACCEPT:
        device->ConfirmPairing();
        break;
      case BUTTON_REJECT:
        device->RejectPairing();
        break;
    }
  }

  // In any case, remove this pairing notification.
  message_center::MessageCenter::Get()->RemoveNotification(
      kBluetoothDevicePairingNotificationId, false /* by_user */);
}

}  // namespace


namespace ash {
namespace internal {

BluetoothNotificationController::BluetoothNotificationController()
    : weak_ptr_factory_(this) {
  BluetoothAdapterFactory::GetAdapter(
      base::Bind(&BluetoothNotificationController::OnGetAdapter,
                 weak_ptr_factory_.GetWeakPtr()));
}

BluetoothNotificationController::~BluetoothNotificationController() {
  if (adapter_.get()) {
    adapter_->RemoveObserver(this);
    adapter_->RemovePairingDelegate(this);
    adapter_ = NULL;
  }
}


void BluetoothNotificationController::DeviceAdded(BluetoothAdapter* adapter,
                                                  BluetoothDevice* device) {
  // Add the new device to the list of currently paired devices; it doesn't
  // receive a notification since it's assumed it was previously notified.
  if (device->IsPaired())
    paired_devices_.insert(device->GetAddress());
}

void BluetoothNotificationController::DeviceChanged(BluetoothAdapter* adapter,
                                                    BluetoothDevice* device) {
  // If the device is already in the list of paired devices, then don't
  // notify.
  if (paired_devices_.find(device->GetAddress()) != paired_devices_.end())
    return;

  // Otherwise if it's marked as paired then it must be newly paired, so
  // notify the user about that.
  if (device->IsPaired()) {
    paired_devices_.insert(device->GetAddress());
    NotifyPairedDevice(device);
  }
}

void BluetoothNotificationController::DeviceRemoved(BluetoothAdapter* adapter,
                                                    BluetoothDevice* device) {
  paired_devices_.erase(device->GetAddress());
}


void BluetoothNotificationController::RequestPinCode(BluetoothDevice* device) {
  // Cannot provide keyboard entry in a notification; these devices (old car
  // audio systems for the most part) will need pairing to be initiated from
  // the Chromebook.
  device->CancelPairing();
}

void BluetoothNotificationController::RequestPasskey(BluetoothDevice* device) {
  // Cannot provide keyboard entry in a notification; fortunately the spec
  // doesn't allow for this to be an option when we're receiving the pairing
  // request anyway.
  device->CancelPairing();
}

void BluetoothNotificationController::DisplayPinCode(
    BluetoothDevice* device,
    const std::string& pincode) {
  base::string16 message = l10n_util::GetStringFUTF16(
          IDS_ASH_STATUS_TRAY_BLUETOOTH_DISPLAY_PINCODE,
          device->GetName(), base::UTF8ToUTF16(pincode));

  NotifyPairing(device, message, false);
}

void BluetoothNotificationController::DisplayPasskey(BluetoothDevice* device,
                                                     uint32 passkey) {
  base::string16 message = l10n_util::GetStringFUTF16(
          IDS_ASH_STATUS_TRAY_BLUETOOTH_DISPLAY_PASSKEY,
          device->GetName(), base::UTF8ToUTF16(
              base::StringPrintf("%06i", passkey)));

  NotifyPairing(device, message, false);
}

void BluetoothNotificationController::KeysEntered(BluetoothDevice* device,
                                                  uint32 entered) {
  // Ignored since we don't have CSS in the notification to update.
}

void BluetoothNotificationController::ConfirmPasskey(BluetoothDevice* device,
                                                     uint32 passkey) {
  base::string16 message = l10n_util::GetStringFUTF16(
          IDS_ASH_STATUS_TRAY_BLUETOOTH_CONFIRM_PASSKEY,
          device->GetName(), base::UTF8ToUTF16(
              base::StringPrintf("%06i", passkey)));

  NotifyPairing(device, message, true);
}

void BluetoothNotificationController::AuthorizePairing(
    BluetoothDevice* device) {
  base::string16 message = l10n_util::GetStringFUTF16(
          IDS_ASH_STATUS_TRAY_BLUETOOTH_AUTHORIZE_PAIRING,
          device->GetName());

  NotifyPairing(device, message, true);
}


void BluetoothNotificationController::OnGetAdapter(
    scoped_refptr<BluetoothAdapter> adapter) {
  DCHECK(!adapter_.get());
  adapter_ = adapter;
  adapter_->AddObserver(this);
  adapter_->AddPairingDelegate(this,
                               BluetoothAdapter::PAIRING_DELEGATE_PRIORITY_LOW);

  // Build a list of the currently paired devices; these don't receive
  // notifications since it's assumed they were previously notified.
  BluetoothAdapter::DeviceList devices = adapter_->GetDevices();
  for (BluetoothAdapter::DeviceList::const_iterator iter = devices.begin();
       iter != devices.end(); ++iter) {
    const BluetoothDevice* device = *iter;
    if (device->IsPaired())
      paired_devices_.insert(device->GetAddress());
  }
}


void BluetoothNotificationController::NotifyPairing(
    BluetoothDevice* device,
    const base::string16& message,
    bool with_buttons) {
  message_center::RichNotificationData optional;
  if (with_buttons) {
    optional.buttons.push_back(message_center::ButtonInfo(
        l10n_util::GetStringUTF16(
            IDS_ASH_STATUS_TRAY_BLUETOOTH_ACCEPT)));
    optional.buttons.push_back(message_center::ButtonInfo(
        l10n_util::GetStringUTF16(
            IDS_ASH_STATUS_TRAY_BLUETOOTH_REJECT)));
  }

  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();

  scoped_ptr<Notification> notification(new Notification(
      message_center::NOTIFICATION_TYPE_SIMPLE,
      kBluetoothDevicePairingNotificationId,
      base::string16()  /* title */,
      message,
      bundle.GetImageNamed(IDR_AURA_UBER_TRAY_BLUETOOTH),
      base::string16()  /* display source */,
      message_center::NotifierId(
          message_center::NotifierId::SYSTEM_COMPONENT,
          system_notifier::kNotifierBluetooth),
      optional,
      new BluetoothPairingNotificationDelegate(adapter_,
                                               device->GetAddress())));
  message_center::MessageCenter::Get()->AddNotification(notification.Pass());
}

void BluetoothNotificationController::NotifyPairedDevice(
    BluetoothDevice* device) {
  // Remove the currently presented pairing notification; since only one
  // pairing request is queued at a time, this is guaranteed to be the device
  // that just became paired.
  message_center::MessageCenter::Get()->RemoveNotification(
      kBluetoothDevicePairingNotificationId, false /* by_user */);

  message_center::RichNotificationData optional;

  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();

  scoped_ptr<Notification> notification(new Notification(
      message_center::NOTIFICATION_TYPE_SIMPLE,
      kBluetoothDevicePairedNotificationId,
      base::string16() /* title */,
      l10n_util::GetStringFUTF16(
          IDS_ASH_STATUS_TRAY_BLUETOOTH_PAIRED, device->GetName()),
      bundle.GetImageNamed(IDR_AURA_UBER_TRAY_BLUETOOTH),
      base::string16() /* display source */,
      message_center::NotifierId(
          message_center::NotifierId::SYSTEM_COMPONENT,
          system_notifier::kNotifierBluetooth),
      optional,
      NULL));
  message_center::MessageCenter::Get()->AddNotification(notification.Pass());
}

}  // namespace internal
}  // namespace ash
