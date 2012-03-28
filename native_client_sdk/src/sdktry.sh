#!/bin/bash

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# gcl wrapper to kick off nacl-sdk try jobs.

readonly script_dir=$(dirname $0)
gcl try $* $(${script_dir}/sdktry-list.sh)
