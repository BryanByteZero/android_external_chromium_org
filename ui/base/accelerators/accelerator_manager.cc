// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/accelerators/accelerator_manager.h"

#include <algorithm>

#include "base/logging.h"

namespace ui {

AcceleratorManager::AcceleratorManager() : last_event_type_(ET_KEY_PRESSED) {
}

AcceleratorManager::~AcceleratorManager() {
}

void AcceleratorManager::Register(const Accelerator& accelerator,
                                  AcceleratorTarget* target) {
  AcceleratorTargetList& targets = accelerators_[accelerator];
  DCHECK(std::find(targets.begin(), targets.end(), target) == targets.end())
      << "Registering the same target multiple times";
  targets.push_front(target);
}

void AcceleratorManager::Unregister(const Accelerator& accelerator,
                                    AcceleratorTarget* target) {
  AcceleratorMap::iterator map_iter = accelerators_.find(accelerator);
  if (map_iter == accelerators_.end()) {
    NOTREACHED() << "Unregistering non-existing accelerator";
    return;
  }

  AcceleratorTargetList* targets = &map_iter->second;
  AcceleratorTargetList::iterator target_iter =
      std::find(targets->begin(), targets->end(), target);
  if (target_iter == targets->end()) {
    NOTREACHED() << "Unregistering accelerator for wrong target";
    return;
  }

  targets->erase(target_iter);
}

void AcceleratorManager::UnregisterAll(AcceleratorTarget* target) {
  for (AcceleratorMap::iterator map_iter = accelerators_.begin();
       map_iter != accelerators_.end(); ++map_iter) {
    AcceleratorTargetList* targets = &map_iter->second;
    targets->remove(target);
  }
}

bool AcceleratorManager::Process(const Accelerator& accelerator) {
  bool result = false;
  AcceleratorMap::iterator map_iter = accelerators_.find(accelerator);
  if (map_iter != accelerators_.end() && ShouldHandle(accelerator)) {
    // We have to copy the target list here, because an AcceleratorPressed
    // event handler may modify the list.
    AcceleratorTargetList targets(map_iter->second);
    for (AcceleratorTargetList::iterator iter = targets.begin();
         iter != targets.end(); ++iter) {
      if ((*iter)->CanHandleAccelerators() &&
          (*iter)->AcceleratorPressed(accelerator)) {
        result = true;
        break;
      }
    }
  }
  last_event_type_ = accelerator.type();
  return result;
}

AcceleratorTarget* AcceleratorManager::GetCurrentTarget(
    const Accelerator& accelerator) const {
  AcceleratorMap::const_iterator map_iter = accelerators_.find(accelerator);
  if (map_iter == accelerators_.end() || map_iter->second.empty())
    return NULL;
  return map_iter->second.front();
}

bool AcceleratorManager::ShouldHandle(const Accelerator& accelerator) const {
  if (accelerator.type() != ET_KEY_RELEASED &&
      accelerator.type() != ET_TRANSLATED_KEY_RELEASE) {
    return true;
  }
  // This check is necessary e.g. not to process the Shift+Alt+ET_KEY_RELEASED
  // Accelerator for Chrome OS (see ash/accelerators/accelerator_controller.cc)
  // when Shift+Alt+Tab is pressed and then Tab is released.
  if (last_event_type_ == ET_KEY_PRESSED ||
      last_event_type_ == ET_TRANSLATED_KEY_PRESS) {
    return true;
  }
  return false;
}

}  // namespace ui
