// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/api/networking_private/networking_private_crypto.h"

namespace networking_private_crypto {

const uint8 kTrustedCAPublicKeyDER[] = {
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xbc, 0x22, 0x80,
    0xbd, 0x80, 0xf6, 0x3a, 0x21, 0x00, 0x3b, 0xae, 0x76, 0x5e, 0x35, 0x7f,
    0x3d, 0xc3, 0x64, 0x5c, 0x55, 0x94, 0x86, 0x34, 0x2f, 0x05, 0x87, 0x28,
    0xcd, 0xf7, 0x69, 0x8c, 0x17, 0xb3, 0x50, 0xa7, 0xb8, 0x82, 0xfa, 0xdf,
    0xc7, 0x43, 0x2d, 0xd6, 0x7e, 0xab, 0xa0, 0x6f, 0xb7, 0x13, 0x72, 0x80,
    0xa4, 0x47, 0x15, 0xc1, 0x20, 0x99, 0x50, 0xcd, 0xec, 0x14, 0x62, 0x09,
    0x5b, 0xa4, 0x98, 0xcd, 0xd2, 0x41, 0xb6, 0x36, 0x4e, 0xff, 0xe8, 0x2e,
    0x32, 0x30, 0x4a, 0x81, 0xa8, 0x42, 0xa3, 0x6c, 0x9b, 0x33, 0x6e, 0xca,
    0xb2, 0xf5, 0x53, 0x66, 0xe0, 0x27, 0x53, 0x86, 0x1a, 0x85, 0x1e, 0xa7,
    0x39, 0x3f, 0x4a, 0x77, 0x8e, 0xfb, 0x54, 0x66, 0x66, 0xfb, 0x58, 0x54,
    0xc0, 0x5e, 0x39, 0xc7, 0xf5, 0x50, 0x06, 0x0b, 0xe0, 0x8a, 0xd4, 0xce,
    0xe1, 0x6a, 0x55, 0x1f, 0x8b, 0x17, 0x00, 0xe6, 0x69, 0xa3, 0x27, 0xe6,
    0x08, 0x25, 0x69, 0x3c, 0x12, 0x9d, 0x8d, 0x05, 0x2c, 0xd6, 0x2e, 0xa2,
    0x31, 0xde, 0xb4, 0x52, 0x50, 0xd6, 0x20, 0x49, 0xde, 0x71, 0xa0, 0xf9,
    0xad, 0x20, 0x40, 0x12, 0xf1, 0xdd, 0x25, 0xeb, 0xd5, 0xe6, 0xb8, 0x36,
    0xf4, 0xd6, 0x8f, 0x7f, 0xca, 0x43, 0xdc, 0xd7, 0x10, 0x5b, 0xe6, 0x3f,
    0x51, 0x8a, 0x85, 0xb3, 0xf3, 0xff, 0xf6, 0x03, 0x2d, 0xcb, 0x23, 0x4f,
    0x9c, 0xad, 0x18, 0xe7, 0x93, 0x05, 0x8c, 0xac, 0x52, 0x9a, 0xf7, 0x4c,
    0xe9, 0x99, 0x7a, 0xbe, 0x6e, 0x7e, 0x4d, 0x0a, 0xe3, 0xc6, 0x1c, 0xa9,
    0x93, 0xfa, 0x3a, 0xa5, 0x91, 0x5d, 0x1c, 0xbd, 0x66, 0xeb, 0xcc, 0x60,
    0xdc, 0x86, 0x74, 0xca, 0xcf, 0xf8, 0x92, 0x1c, 0x98, 0x7d, 0x57, 0xfa,
    0x61, 0x47, 0x9e, 0xab, 0x80, 0xb7, 0xe4, 0x48, 0x80, 0x2a, 0x92, 0xc5,
    0x1b, 0x02, 0x03, 0x01, 0x00, 0x01};

const size_t kTrustedCAPublicKeyDERLength = sizeof(kTrustedCAPublicKeyDER);

}  // namespace networking_private_crypto
