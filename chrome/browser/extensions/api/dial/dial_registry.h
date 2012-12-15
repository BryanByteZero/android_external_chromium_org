// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_DIAL_DIAL_REGISTRY_H_
#define CHROME_BROWSER_EXTENSIONS_API_DIAL_DIAL_REGISTRY_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/hash_tables.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "base/time.h"
#include "base/timer.h"
#include "chrome/browser/extensions/api/dial/dial_service.h"
#include "chrome/browser/profiles/profile.h"

namespace extensions {

// Keeps track of devices that have responded to discovery requests and notifies
// the observer with an updated, complete set of active devices.  The registry's
// observer (i.e., the Dial API) owns the registry instance.
class DialRegistry : public DialService::Observer {
 public:
  typedef std::vector<DialDeviceData> DeviceList;

  class Observer {
   public:
    // Invoked on the IO thread by the DialRegistry during a device event (new
    // device discovered or an update triggered by dial.discoverNow).
    virtual void OnDialDeviceEvent(const DeviceList& devices) = 0;

   protected:
    virtual ~Observer() {}
  };

  // Create the DIAL registry and pass a reference to allow it to notify on
  // DIAL device events.
  DialRegistry(Observer* dial_api,
               const base::TimeDelta& refresh_interval,
               const base::TimeDelta& expiration,
               const size_t max_devices);

  virtual ~DialRegistry();

  // Called by the DIAL API when event listeners are added or removed. The dial
  // service is started after the first listener is added and stopped after the
  // last listener is removed.
  void OnListenerAdded();
  void OnListenerRemoved();

  // Called by the DIAL API to try to kickoff a discovery if there is not one
  // already active.
  bool DiscoverNow();

 protected:
  // Returns a new instance of the DIAL service.  Overridden by tests.
  virtual DialService* CreateDialService();

  // Returns the current time.  Overridden by tests.
  virtual base::Time Now() const;

 private:
  typedef base::hash_map<std::string, linked_ptr<DialDeviceData> >
      DeviceByIdMap;
  typedef std::map<std::string, linked_ptr<DialDeviceData> > DeviceByLabelMap;

  // DialService::Observer:
  virtual void OnDiscoveryRequest(DialService* service) OVERRIDE;
  virtual void OnDeviceDiscovered(DialService* service,
                                  const DialDeviceData& device) OVERRIDE;
  virtual void OnDiscoveryFinished(DialService* service) OVERRIDE;
  virtual void OnError(DialService* service, const std::string& msg) OVERRIDE;

  // Starts and stops periodic discovery.  Periodic discovery is done when there
  // are registered event listeners.
  void StartPeriodicDiscovery();
  void StopPeriodicDiscovery();

  // Purge our whole registry. We may need to do this occasionally, e.g. when
  // the network status changes.  Increments the registry generation.
  void Clear();

  // The repeating timer schedules discoveries with this method.
  void DoDiscovery();

  // Attempts to add a newly discovered device to the registry.  Returns true if
  // successful.
  bool MaybeAddDevice(const linked_ptr<DialDeviceData>& device_data);

  // Remove devices from the registry that have expired, i.e. not responded
  // after some time.  Returns true if the registry was modified.
  bool PruneExpiredDevices();

  // Returns true if the device has expired and should be removed from the
  // active set.
  bool IsDeviceExpired(const DialDeviceData& device) const;

  // Notify clients with the current device list if necessary.
  void MaybeSendEvent();

  // Returns the next label to use for a newly-seen device.
  std::string NextLabel();

  // The current number of event listeners attached to this registry.
  int num_listeners_;

  // Incremented each time we DoDiscovery().
  int discovery_generation_;

  // Incremented each time we modify the registry of active devices.
  int registry_generation_;

  // The discovery generation associated with the last time we sent an event.
  // Used to ensure that we generate at least one event per round of discovery.
  int last_event_discovery_generation_;

  // The registry generation associated with the last time we sent an event.
  // Used to suppress events with duplicate device lists.
  int last_event_registry_generation_;

  // Counter to generate device labels.
  int label_count_;

  // Registry parameters
  base::TimeDelta refresh_interval_delta_;
  base::TimeDelta expiration_delta_;
  size_t max_devices_;

  // The DIAL service.
  scoped_refptr<DialService> dial_;

  // A map used to track known devices by their device_id.
  DeviceByIdMap device_by_id_map_;

  // A map used to track known devices sorted by label.  We iterate over this to
  // construct the device list sent to API clients.
  DeviceByLabelMap device_by_label_map_;

  // Timer used to manage periodic discovery requests.
  base::RepeatingTimer<DialRegistry> repeating_timer_;

  // Interface from which the DIAL API is notified of DIAL device events. the
  // DIAL API owns this DIAL registry.
  Observer* const dial_api_;

  // Thread checker.
  base::ThreadChecker thread_checker_;

  FRIEND_TEST_ALL_PREFIXES(DialRegistryTest, TestAddRemoveListeners);
  FRIEND_TEST_ALL_PREFIXES(DialRegistryTest, TestNoDevicesDiscovered);
  FRIEND_TEST_ALL_PREFIXES(DialRegistryTest, TestDevicesDiscovered);
  FRIEND_TEST_ALL_PREFIXES(DialRegistryTest, TestDeviceExpires);
  DISALLOW_COPY_AND_ASSIGN(DialRegistry);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_DIAL_DIAL_REGISTRY_H_
