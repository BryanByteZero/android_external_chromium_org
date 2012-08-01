# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'murmurhash3',
      'type': 'static_library',
      'sources': [
        'src/MurmurHash3.h',
        'src/MurmurHash3.cpp',
      ],
    },
    {
      'target_name': 'pmurhash',
      'type': 'static_library',
      'sources': [
        'src/PMurHash.h',
        'src/PMurHash.c',
      ],
    },
  ],
}
