# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re


def CreateNoOverheadFilter():
  """Returns a filter with the least overhead possible.

  This contains no sub-traces of thread tasks, so it's only useful for
  capturing the cpu-time spent on threads (as well as needed benchmark
  traces).

  FIXME: Remove webkit.console when blink.console lands in chromium and
  the ref builds are updated. crbug.com/386847
  """
  categories = [
    "toplevel",
    "benchmark",
    "webkit.console",
    "blink.console",
    "trace_event_overhead"
  ]
  return TracingCategoryFilter(filter_string=','.join(categories))


def CreateMinimalOverheadFilter():
  """Returns a filter with the best-effort amount of overhead."""
  return TracingCategoryFilter(filter_string='')


def CreateDebugOverheadFilter():
  """Returns a filter with as many traces enabled as is useful."""
  return TracingCategoryFilter(filter_string='*,disabled-by-default-cc.debug')


_delay_re = re.compile(r'DELAY[(][A-Za-z0-9._;]+[)]')


class TracingCategoryFilter(object):
  """A set of included and excluded categories that should be traced.

  The TraceCategoryFilter allows fine tuning of what data is traced. Basic
  choice of which tracers to use is done by TracingOptions.

  Providing filter_string=None gives the default category filter, which leaves
  what to trace up to the individual trace systems.
  """
  def __init__(self, filter_string=None):
    self._included_categories = set()
    self._excluded_categories = set()
    self._disabled_by_default_categories = set()
    self._synthetic_delays = set()
    self.contains_wildcards = False

    if filter_string == None:
      return

    if '*' in filter_string or '?' in filter_string:
      self.contains_wildcards = True

    filter_set = set(filter_string.split(','))
    for category in filter_set:
      if category == '':
        continue

      if _delay_re.match(category):
        self._synthetic_delays.add(category)
        continue

      if category[0] == '-':
        self._excluded_categories.add(category[1:])
        continue

      if category.startswith('disabled-by-default-'):
        self._disabled_by_default_categories.add(category)
        continue

      self.included_categories.add(category)

  @property
  def included_categories(self):
    return self._included_categories

  @property
  def excluded_categories(self):
    return self._excluded_categories

  @property
  def disabled_by_default_categories(self):
    return self._disabled_by_default_categories

  @property
  def synthetic_delays(self):
    return self._synthetic_delays

  @property
  def filter_string(self):
    return self._GetFilterString(stable_output=False)

  @property
  def stable_filter_string(self):
    return self._GetFilterString(stable_output=True)

  def _GetFilterString(self, stable_output):
    # Note: This outputs fields in an order that intentionally matches
    # trace_event_impl's CategoryFilter string order.
    lists = []
    lists.append(self._included_categories)
    lists.append(self._disabled_by_default_categories)
    lists.append(['-%s' % x for x in self._excluded_categories])
    lists.append(self._synthetic_delays)
    categories = []
    for l in lists:
      if stable_output:
        l = list(l)
        l.sort()
      categories.extend(l)
    return ','.join(categories)

  def AddIncludedCategory(self, category_glob):
    """Explicitly enables anything matching category_glob."""
    assert not category_glob.startswith('disabled-by-default-')
    if category_glob not in self._included_categories:
      self._included_categories.append(category_glob)

  def AddExcludedCategory(self, category_glob):
    """Explicitly disables anything matching category_glob."""
    assert not category_glob.startswith('disabled-by-default-')
    if category_glob not in self._excluded_categories:
      self._excluded_categories.append(category_glob)

  def AddSyntheticDelay(self, delay):
    assert _delay_re.match(delay)
    self._synthetic_delays.add(delay)

  def IsSubset(self, other):
    """ Determine if filter A (self) is a subset of filter B (other).
        Returns True if A is a subset of B, False if A is not a subset of B,
        and None if we can't tell for sure.
    """
    # We don't handle filters with wildcards in this test.
    if self.contains_wildcards or other.contains_wildcards:
      return None

    # Disabled categories get into a trace if and only if they are contained in
    # the 'disabled' set. Return False if A's disabled set is not a subset of
    # B's disabled set.
    if not self.disabled_by_default_categories <= \
       other.disabled_by_default_categories:
      return False

    # If A defines more or different synthetic delays than B, then A is not a
    # subset.
    if not self.synthetic_delays <= other.synthetic_delays:
      return False

    if self.included_categories and other.included_categories:
      # A and B have explicit include lists. If A includes something that B
      # doesn't, return False.
      if not self.included_categories <= other.included_categories:
        return False
    elif self.included_categories:
      # Only A has an explicit include list. If A includes something that B
      # excludes, return False.
      if self.included_categories.intersection(other.excluded_categories):
        return False
    elif other.included_categories:
      # Only B has an explicit include list. We don't know which categories are
      # contained in the default list, so return None.
      return None
    else:
      # None of the filter have explicit include list. If B excludes categories
      # that A doesn't exclude, return False.
      if not other.excluded_categories <= self.excluded_categories:
        return False

    return True
