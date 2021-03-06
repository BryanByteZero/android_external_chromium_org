#!/usr/bin/env python
#
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os
import sys

from util import build_utils
from util import md5_check


def DoDex(options, paths):
  dx_binary = os.path.join(options.android_sdk_tools, 'dx')
  # See http://crbug.com/272064 for context on --force-jumbo.
  dex_cmd = [dx_binary, '--dex', '--force-jumbo', '--output', options.dex_path]
  if options.no_locals != '0':
    dex_cmd.append('--no-locals')
  dex_cmd += paths

  record_path = '%s.md5.stamp' % options.dex_path
  md5_check.CallAndRecordIfStale(
      lambda: build_utils.CheckOutput(dex_cmd, print_stderr=False),
      record_path=record_path,
      input_paths=paths,
      input_strings=dex_cmd,
      force=not os.path.exists(options.dex_path))
  build_utils.WriteJson(paths, options.dex_path + '.inputs')


def main():
  parser = optparse.OptionParser()
  parser.add_option('--android-sdk-tools',
                    help='Android sdk build tools directory.')
  parser.add_option('--dex-path', help='Dex output path.')
  parser.add_option('--configuration-name',
                    help='The build CONFIGURATION_NAME.')
  parser.add_option('--proguard-enabled',
                    help='"true" if proguard is enabled.')
  parser.add_option('--proguard-enabled-input-path',
                    help=('Path to dex in Release mode when proguard '
                          'is enabled.'))
  parser.add_option('--no-locals',
                    help='Exclude locals list from the dex file.')
  parser.add_option('--excluded-paths-file',
                    help='Path to a file containing a list of paths to exclude '
                    'from the dex file.')

  options, paths = parser.parse_args()

  if (options.proguard_enabled == 'true'
      and options.configuration_name == 'Release'):
    paths = [options.proguard_enabled_input_path]

  if options.excluded_paths_file:
    exclude_paths = build_utils.ReadJson(options.excluded_paths_file)
    paths = [p for p in paths if not p in exclude_paths]

  DoDex(options, paths)


if __name__ == '__main__':
  sys.exit(main())
