// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/performance_monitor/performance_monitor_ui_constants.h"

#include "base/logging.h"
#include "base/time.h"

namespace performance_monitor {

namespace {

// Memory measurements
const int64 kBytesPerKilobyte = 1 << 10;
const int64 kBytesPerMegabyte = kBytesPerKilobyte * (1 << 10);
const int64 kBytesPerGigabyte = kBytesPerMegabyte * (1 << 10);
const int64 kBytesPerTerabyte = kBytesPerGigabyte * (1 << 10);

// Time measurements - Most of these are imported from base/time.h
// These units are used for display (and it's related calculations), not for
// any mathematical analysis. Thus we can estimate for values without an exact
// conversion.
const int64 kMicrosecondsPerMonth = base::Time::kMicrosecondsPerDay * 30;
const int64 kMicrosecondsPerYear = base::Time::kMicrosecondsPerDay * 365;

// Keep this list synced with the enum declared in the .h file.
const UnitDetails kUnitDetailsList[] = {
  { UNIT_BYTES, MEASUREMENT_TYPE_MEMORY, 1 },
  { UNIT_KILOBYTES, MEASUREMENT_TYPE_MEMORY, kBytesPerKilobyte },
  { UNIT_MEGABYTES, MEASUREMENT_TYPE_MEMORY, kBytesPerMegabyte },
  { UNIT_GIGABYTES, MEASUREMENT_TYPE_MEMORY, kBytesPerGigabyte },
  { UNIT_TERABYTES, MEASUREMENT_TYPE_MEMORY, kBytesPerTerabyte },
  { UNIT_MICROSECONDS, MEASUREMENT_TYPE_TIME, 1 },
  { UNIT_MILLISECONDS, MEASUREMENT_TYPE_TIME,
        base::Time::kMicrosecondsPerMillisecond },
  { UNIT_SECONDS, MEASUREMENT_TYPE_TIME, base::Time::kMicrosecondsPerSecond },
  { UNIT_MINUTES, MEASUREMENT_TYPE_TIME, base::Time::kMicrosecondsPerMinute },
  { UNIT_HOURS, MEASUREMENT_TYPE_TIME, base::Time::kMicrosecondsPerHour },
  { UNIT_DAYS, MEASUREMENT_TYPE_TIME, base::Time::kMicrosecondsPerDay },
  { UNIT_WEEKS, MEASUREMENT_TYPE_TIME, base::Time::kMicrosecondsPerWeek },
  { UNIT_MONTHS, MEASUREMENT_TYPE_TIME, kMicrosecondsPerMonth },
  { UNIT_YEARS, MEASUREMENT_TYPE_TIME, kMicrosecondsPerYear },
  { UNIT_PERCENT, MEASUREMENT_TYPE_PERCENT, 1 },
};

COMPILE_ASSERT(ARRAYSIZE_UNSAFE(kUnitDetailsList) == UNIT_UNDEFINED,
               unit_details_doesnt_match_unit_types);

}  // namespace

const UnitDetails* GetUnitDetails(Unit unit) {
  if (unit == UNIT_UNDEFINED) {
    LOG(ERROR) << "Request for undefined unit";
    return NULL;
  }

  return &kUnitDetailsList[unit];
}

}  // namespace performance_monitor
