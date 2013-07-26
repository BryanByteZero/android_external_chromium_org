/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This file contains common certificates. It's designed to be #included in
 * another file, in a namespace. */

#include "net/quic/crypto/common_cert_set_1_50.inc"
#include "net/quic/crypto/common_cert_set_51_100.inc"

static const size_t kNumCerts = 102;
static const unsigned char* const kCerts[] = {
  kDERCert0,
  kDERCert1,
  kDERCert2,
  kDERCert3,
  kDERCert4,
  kDERCert5,
  kDERCert6,
  kDERCert7,
  kDERCert8,
  kDERCert9,
  kDERCert10,
  kDERCert11,
  kDERCert12,
  kDERCert13,
  kDERCert14,
  kDERCert15,
  kDERCert16,
  kDERCert17,
  kDERCert18,
  kDERCert19,
  kDERCert20,
  kDERCert21,
  kDERCert22,
  kDERCert23,
  kDERCert24,
  kDERCert25,
  kDERCert26,
  kDERCert27,
  kDERCert28,
  kDERCert29,
  kDERCert30,
  kDERCert31,
  kDERCert32,
  kDERCert33,
  kDERCert34,
  kDERCert35,
  kDERCert36,
  kDERCert37,
  kDERCert38,
  kDERCert39,
  kDERCert40,
  kDERCert41,
  kDERCert42,
  kDERCert43,
  kDERCert44,
  kDERCert45,
  kDERCert46,
  kDERCert47,
  kDERCert48,
  kDERCert49,
  kDERCert50,
  kDERCert51,
  kDERCert52,
  kDERCert53,
  kDERCert54,
  kDERCert55,
  kDERCert56,
  kDERCert57,
  kDERCert58,
  kDERCert59,
  kDERCert60,
  kDERCert61,
  kDERCert62,
  kDERCert63,
  kDERCert64,
  kDERCert65,
  kDERCert66,
  kDERCert67,
  kDERCert68,
  kDERCert69,
  kDERCert70,
  kDERCert71,
  kDERCert72,
  kDERCert73,
  kDERCert74,
  kDERCert75,
  kDERCert76,
  kDERCert77,
  kDERCert78,
  kDERCert79,
  kDERCert80,
  kDERCert81,
  kDERCert82,
  kDERCert83,
  kDERCert84,
  kDERCert85,
  kDERCert86,
  kDERCert87,
  kDERCert88,
  kDERCert89,
  kDERCert90,
  kDERCert91,
  kDERCert92,
  kDERCert93,
  kDERCert94,
  kDERCert95,
  kDERCert96,
  kDERCert97,
  kDERCert98,
  kDERCert99,
  kDERCert100,
  kDERCert101,
};

static const size_t kLens[] = {
  692,
  897,
  903,
  911,
  911,
  957,
  971,
  985,
  989,
  1022,
  1032,
  1049,
  1055,
  1071,
  1071,
  1073,
  1075,
  1078,
  1080,
  1082,
  1082,
  1084,
  1088,
  1090,
  1094,
  1097,
  1098,
  1107,
  1118,
  1119,
  1122,
  1124,
  1131,
  1134,
  1136,
  1147,
  1161,
  1162,
  1162,
  1167,
  1167,
  1171,
  1172,
  1181,
  1183,
  1184,
  1187,
  1191,
  1194,
  1194,
  1199,
  1223,
  1226,
  1231,
  1236,
  1239,
  1250,
  1254,
  1256,
  1256,
  1257,
  1260,
  1268,
  1269,
  1269,
  1270,
  1273,
  1276,
  1279,
  1280,
  1282,
  1285,
  1286,
  1287,
  1287,
  1290,
  1291,
  1291,
  1294,
  1294,
  1297,
  1302,
  1302,
  1303,
  1311,
  1330,
  1365,
  1512,
  1520,
  1535,
  1548,
  1550,
  1559,
  1570,
  1581,
  1584,
  1592,
  1592,
  1625,
  1628,
  1767,
  1770,
};

static const uint64 kHash = GG_UINT64_C(0xc9fef74053f99f39);
