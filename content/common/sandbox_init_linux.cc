// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/sandbox_init.h"

#include "base/memory/scoped_ptr.h"
#include "content/common/sandbox_seccomp_bpf_linux.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf_policy.h"

namespace content {

bool InitializeSandbox(scoped_ptr<playground2::SandboxBpfPolicy> policy) {
  return SandboxSeccompBpf::StartSandboxWithExternalPolicy(policy.Pass());
}

scoped_ptr<playground2::SandboxBpfPolicy> GetBpfSandboxBaselinePolicy() {
  return SandboxSeccompBpf::GetBaselinePolicy().Pass();
}

}  // namespace content
