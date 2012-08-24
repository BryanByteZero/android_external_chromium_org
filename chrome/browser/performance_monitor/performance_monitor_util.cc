// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/performance_monitor/performance_monitor_util.h"

#include "base/json/json_writer.h"
#include "base/memory/scoped_ptr.h"
#include "base/string_number_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/time.h"
#include "chrome/browser/performance_monitor/events.h"
#include "content/public/browser/browser_thread.h"

namespace performance_monitor {
namespace util {

Database::MetricVector AggregateMetric(const Database::MetricVector& metrics,
                                       const base::Time& start,
                                       const base::TimeDelta& resolution) {
  Database::MetricVector results;
  // Ignore all the points before the aggregation start.
  Database::MetricVector::const_iterator it = metrics.begin();
  for (; it != metrics.end() && it->time < start; ++it) { }

  while (it != metrics.end()) {
    // Finds the beginning of the next aggregation window.
    int64 window_offset = (it->time - start) / resolution;
    base::Time window_start = start + (window_offset * resolution);
    base::Time window_end = window_start + resolution;
    base::Time last_sample_time = window_start;
    double integrated = 0.0;
    double metric_value = 0.0;

    // Aggregate the step function defined by the Metrics in |metrics|.
    while (it != metrics.end() && it->time <= window_end) {
      metric_value = it->value;
      integrated += metric_value * (it->time - last_sample_time).InSecondsF();
      last_sample_time = it->time;
      ++it;
    }
    if (it != metrics.end())
      metric_value = it->value;

    // If the window splits an area of the step function, split the aggregation
    // at the end of the window.
    integrated += metric_value * (window_end - last_sample_time).InSecondsF();
    double average = integrated / resolution.InSecondsF();
    results.push_back(Metric(window_end, average));
  }
  return results;
}

bool PostTaskToDatabaseThreadAndReply(
    const tracked_objects::Location& from_here,
    const base::Closure& request,
    const base::Closure& reply) {
  base::SequencedWorkerPool* pool = content::BrowserThread::GetBlockingPool();
  base::SequencedWorkerPool::SequenceToken token =
      pool->GetNamedSequenceToken(Database::kDatabaseSequenceToken);
  return pool->GetSequencedTaskRunner(token)->PostTaskAndReply(
      from_here, request, reply);
}

scoped_ptr<Event> CreateExtensionEvent(const EventType type,
                                       const base::Time& time,
                                       const std::string& id,
                                       const std::string& name,
                                       const std::string& url,
                                       const int location,
                                       const std::string& version,
                                       const std::string& description) {
  events::ExtensionEvent event;
  event.event_type = type;
  event.time = static_cast<double>(time.ToInternalValue());
  event.extension_id = id;
  event.extension_name = name;
  event.extension_url = url;
  event.extension_location = location;
  event.extension_version = version;
  event.extension_description = description;
  scoped_ptr<base::DictionaryValue> value = event.ToValue();
  return scoped_ptr<Event>(new Event(
      type, time, value.Pass()));
}

scoped_ptr<Event> CreateRendererFreezeEvent(const base::Time& time,
                                            const std::string& url) {
  events::RendererFreeze event;
  event.event_type = EVENT_RENDERER_FREEZE;
  event.time = static_cast<double>(time.ToInternalValue());
  event.url = url;
  scoped_ptr<base::DictionaryValue> value = event.ToValue();
  return scoped_ptr<Event>(new Event(
      EVENT_RENDERER_FREEZE, time, value.Pass()));
}

scoped_ptr<Event> CreateCrashEvent(const base::Time& time,
                                   const EventType& type) {
  events::RendererCrash event;
  event.event_type = type;
  event.time = static_cast<double>(time.ToInternalValue());
  scoped_ptr<base::DictionaryValue> value = event.ToValue();
  return scoped_ptr<Event>(new Event(type, time, value.Pass()));
}

scoped_ptr<Event> CreateUncleanExitEvent(const base::Time& time,
                                         const std::string& profile_name) {
  events::UncleanExit event;
  event.event_type = EVENT_UNCLEAN_EXIT;
  event.time = static_cast<double>(time.ToInternalValue());
  event.profile_name = profile_name;
  scoped_ptr<base::DictionaryValue> value = event.ToValue();
  return scoped_ptr<Event>(new Event(
      EVENT_UNCLEAN_EXIT, time, value.Pass()));
}

scoped_ptr<Event> CreateChromeUpdateEvent(const base::Time& time,
                                          const std::string& previous_version,
                                          const std::string& current_version) {
  events::ChromeUpdate event;
  event.event_type = EVENT_CHROME_UPDATE;
  event.time = static_cast<double>(time.ToInternalValue());
  event.previous_version = previous_version;
  event.current_version = current_version;
  scoped_ptr<base::DictionaryValue> value = event.ToValue();
  return scoped_ptr<Event>(new Event(
      EVENT_CHROME_UPDATE, time, value.Pass()));
}

}  // namespace util
}  // namespace performance_monitor
