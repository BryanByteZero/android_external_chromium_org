// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter.h"

#include "base/stl_util.h"
#include "device/bluetooth/bluetooth_device.h"

namespace device {

BluetoothAdapter::BluetoothAdapter() {
}

BluetoothAdapter::~BluetoothAdapter() {
  STLDeleteValues(&devices_);
}

void BluetoothAdapter::StartDiscovering(const base::Closure& callback,
                                        const ErrorCallback& error_callback) {
  AddDiscoverySession(callback, error_callback);
}

void BluetoothAdapter::StopDiscovering(const base::Closure& callback,
                                       const ErrorCallback& error_callback) {
  RemoveDiscoverySession(callback, error_callback);
}

BluetoothAdapter::DeviceList BluetoothAdapter::GetDevices() {
  ConstDeviceList const_devices =
    const_cast<const BluetoothAdapter *>(this)->GetDevices();

  DeviceList devices;
  for (ConstDeviceList::const_iterator i = const_devices.begin();
       i != const_devices.end(); ++i)
    devices.push_back(const_cast<BluetoothDevice *>(*i));

  return devices;
}

BluetoothAdapter::ConstDeviceList BluetoothAdapter::GetDevices() const {
  ConstDeviceList devices;
  for (DevicesMap::const_iterator iter = devices_.begin();
       iter != devices_.end();
       ++iter)
    devices.push_back(iter->second);

  return devices;
}

BluetoothDevice* BluetoothAdapter::GetDevice(const std::string& address) {
  return const_cast<BluetoothDevice *>(
      const_cast<const BluetoothAdapter *>(this)->GetDevice(address));
}

const BluetoothDevice* BluetoothAdapter::GetDevice(
    const std::string& address) const {
  DevicesMap::const_iterator iter = devices_.find(address);
  if (iter != devices_.end())
    return iter->second;

  return NULL;
}

void BluetoothAdapter::AddPairingDelegate(
    BluetoothDevice::PairingDelegate* pairing_delegate,
    PairingDelegatePriority priority) {
  // Remove the delegate, if it already exists, before inserting to allow a
  // change of priority.
  RemovePairingDelegate(pairing_delegate);

  // Find the first point with a lower priority, or the end of the list.
  std::list<PairingDelegatePair>::iterator iter = pairing_delegates_.begin();
  while (iter != pairing_delegates_.end() && iter->second >= priority)
    ++iter;

  pairing_delegates_.insert(iter, std::make_pair(pairing_delegate, priority));
}

void BluetoothAdapter::RemovePairingDelegate(
    BluetoothDevice::PairingDelegate* pairing_delegate) {
  for (std::list<PairingDelegatePair>::iterator iter =
       pairing_delegates_.begin(); iter != pairing_delegates_.end(); ++iter) {
    if (iter->first == pairing_delegate) {
      RemovePairingDelegateInternal(pairing_delegate);
      pairing_delegates_.erase(iter);
      return;
    }
  }
}

BluetoothDevice::PairingDelegate* BluetoothAdapter::DefaultPairingDelegate() {
  if (pairing_delegates_.empty())
    return NULL;

  return pairing_delegates_.front().first;
}

}  // namespace device
