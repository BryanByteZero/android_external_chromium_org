#!/usr/bin/python2.4
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''This file contains item formatters for resource_map_header and
resource_map_source files.  A resource map is a mapping between resource names
(string) and the internal resource ID.'''

import os

from grit import util
from grit.format import interface

def GetMapName(root):
  '''Get the name of the resource map based on the header file name.  E.g.,
  if our header filename is theme_resources.h, we name our resource map
  kThemeResourcesMap.

  |root| is the grd file root.'''
  outputs = root.GetOutputFiles()
  rc_header_file = None
  for output in outputs:
    if 'rc_header' == output.GetType():
      rc_header_file = output.GetFilename()
  if not rc_header_file:
    raise Exception('unable to find resource header filename')
  filename = os.path.splitext(os.path.split(rc_header_file)[1])[0]
  filename = filename[0].upper() + filename[1:]
  while filename.find('_') != -1:
    pos = filename.find('_')
    if pos >= len(filename):
      break
    filename = filename[:pos] + filename[pos + 1].upper() + filename[pos + 2:]
  return 'k' + filename


class HeaderTopLevel(interface.ItemFormatter):
  '''Create the header file for the resource mapping.  This file just declares
  an array of name/value pairs.'''
  def Format(self, item, lang='en', begin_item=True, output_dir='.'):
    if not begin_item:
      return ''
    return '''\
// Copyright (c) %(year)d The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// This file is automatically generated by GRIT.  Do not edit.

#include <stddef.h>

#ifndef GRIT_RESOURCE_MAP_STRUCT_
#define GRIT_RESOURCE_MAP_STRUCT_
struct GritResourceMap {
  const char* const name;
  int value;
};
#endif // GRIT_RESOURCE_MAP_STRUCT_

extern const GritResourceMap %(map_name)s[];
extern const size_t %(map_name)sSize;
''' % { 'year': util.GetCurrentYear(),
        'map_name': GetMapName(item.GetRoot()),
      }


class SourceTopLevel(interface.ItemFormatter):
  '''Create the C++ source file for the resource mapping.  This class handles
  the header/footer of the file.'''
  def Format(self, item, lang='en', begin_item=True, output_dir='.'):
    if begin_item:
      grit_root = item.GetRoot()
      outputs = grit_root.GetOutputFiles()
      rc_header_file = None
      map_header_file = None
      for output in outputs:
        if 'rc_header' == output.GetType():
          rc_header_file = output.GetFilename()
        elif 'resource_map_header' == output.GetType():
          map_header_file = output.GetFilename()
      if not rc_header_file or not map_header_file:
        raise Exception('resource_map_source output type requires '
            'resource_map_header and rc_header outputs')

      return '''\
// Copyright (c) %(year)d The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// This file is automatically generated by GRIT.  Do not edit.

#include "%(map_header_file)s"

#include "base/basictypes.h"
#include "%(rc_header_file)s"

const GritResourceMap %(map_name)s[] = {
''' % { 'year': util.GetCurrentYear(),
        'map_header_file': map_header_file,
        'rc_header_file': rc_header_file,
        'map_name': GetMapName(item.GetRoot()),
      }
    else:
      # Return the footer text.
      return '''\
};

const size_t %(map_name)sSize = arraysize(%(map_name)s);
''' % { 'map_name': GetMapName(item.GetRoot()) }


class SourceInclude(interface.ItemFormatter):
  '''Populate the resource mapping.  For each include, we map a string to
  the resource ID.'''
  def Format(self, item, lang='en', begin_item=True, output_dir='.'):
    if not begin_item:
      return ''
    return '  {"%s", %s},\n' % (item.attrs['name'], item.attrs['name'])


class SourceFileInclude(interface.ItemFormatter):
  '''Populate the resource mapping.  For each include, we map a filename to
  the resource ID.'''
  def Format(self, item, lang='en', begin_item=True, output_dir='.'):
    if not begin_item:
      return ''
    filename = item.attrs['file'].replace("\\", "/")
    return '  {"%s", %s},\n' % (filename, item.attrs['name'])
