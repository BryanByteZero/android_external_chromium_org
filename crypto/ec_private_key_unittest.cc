// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crypto/ec_private_key.h"

#include <vector>

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

// Generate random private keys. Export, then re-import. We should get
// back the same exact public key, and the private key should have the same
// value and elliptic curve params.
TEST(ECPrivateKeyUnitTest, InitRandomTest) {
  const std::string password1;
  const std::string password2 = "test";

  scoped_ptr<crypto::ECPrivateKey> keypair1(crypto::ECPrivateKey::Create());
  scoped_ptr<crypto::ECPrivateKey> keypair2(crypto::ECPrivateKey::Create());
  ASSERT_TRUE(keypair1.get());
  ASSERT_TRUE(keypair2.get());

  std::vector<uint8> key1value;
  std::vector<uint8> key2value;
  std::vector<uint8> key1params;
  std::vector<uint8> key2params;
  EXPECT_TRUE(keypair1->ExportValue(&key1value));
  EXPECT_TRUE(keypair2->ExportValue(&key2value));
  EXPECT_TRUE(keypair1->ExportECParams(&key1params));
  EXPECT_TRUE(keypair2->ExportECParams(&key2params));

  std::vector<uint8> privkey1;
  std::vector<uint8> privkey2;
  std::vector<uint8> pubkey1;
  std::vector<uint8> pubkey2;
  std::string raw_pubkey1;
  std::string raw_pubkey2;
  ASSERT_TRUE(keypair1->ExportEncryptedPrivateKey(password1, 1, &privkey1));
  ASSERT_TRUE(keypair2->ExportEncryptedPrivateKey(password2, 1, &privkey2));
  EXPECT_TRUE(keypair1->ExportPublicKey(&pubkey1));
  EXPECT_TRUE(keypair2->ExportPublicKey(&pubkey2));
  EXPECT_TRUE(keypair1->ExportRawPublicKey(&raw_pubkey1));
  EXPECT_TRUE(keypair2->ExportRawPublicKey(&raw_pubkey2));

  scoped_ptr<crypto::ECPrivateKey> keypair3(
      crypto::ECPrivateKey::CreateFromEncryptedPrivateKeyInfo(
          password1, privkey1, pubkey1));
  scoped_ptr<crypto::ECPrivateKey> keypair4(
      crypto::ECPrivateKey::CreateFromEncryptedPrivateKeyInfo(
          password2, privkey2, pubkey2));
  ASSERT_TRUE(keypair3.get());
  ASSERT_TRUE(keypair4.get());

  std::vector<uint8> key3value;
  std::vector<uint8> key4value;
  std::vector<uint8> key3params;
  std::vector<uint8> key4params;
  EXPECT_TRUE(keypair3->ExportValue(&key3value));
  EXPECT_TRUE(keypair4->ExportValue(&key4value));
  EXPECT_TRUE(keypair3->ExportECParams(&key3params));
  EXPECT_TRUE(keypair4->ExportECParams(&key4params));

  EXPECT_EQ(key1value, key3value);
  EXPECT_EQ(key2value, key4value);
  EXPECT_EQ(key1params, key3params);
  EXPECT_EQ(key2params, key4params);

  std::vector<uint8> pubkey3;
  std::vector<uint8> pubkey4;
  std::string raw_pubkey3;
  std::string raw_pubkey4;
  EXPECT_TRUE(keypair3->ExportPublicKey(&pubkey3));
  EXPECT_TRUE(keypair4->ExportPublicKey(&pubkey4));
  EXPECT_TRUE(keypair3->ExportRawPublicKey(&raw_pubkey3));
  EXPECT_TRUE(keypair4->ExportRawPublicKey(&raw_pubkey4));

  EXPECT_EQ(pubkey1, pubkey3);
  EXPECT_EQ(pubkey2, pubkey4);
  EXPECT_EQ(raw_pubkey1, raw_pubkey3);
  EXPECT_EQ(raw_pubkey2, raw_pubkey4);
}

#if !defined(USE_OPENSSL)
TEST(ECPrivateKeyUnitTest, Copy) {
  scoped_ptr<crypto::ECPrivateKey> keypair1(crypto::ECPrivateKey::Create());
  scoped_ptr<crypto::ECPrivateKey> keypair2(keypair1->Copy());
  ASSERT_TRUE(keypair1.get());
  ASSERT_TRUE(keypair2.get());

  std::vector<uint8> key1value;
  std::vector<uint8> key2value;
  EXPECT_TRUE(keypair1->ExportValue(&key1value));
  EXPECT_TRUE(keypair2->ExportValue(&key2value));
  EXPECT_EQ(key1value, key2value);

  std::vector<uint8> key1params;
  std::vector<uint8> key2params;
  EXPECT_TRUE(keypair1->ExportECParams(&key1params));
  EXPECT_TRUE(keypair2->ExportECParams(&key2params));
  EXPECT_EQ(key1params, key2params);

  std::vector<uint8> pubkey1;
  std::vector<uint8> pubkey2;
  EXPECT_TRUE(keypair1->ExportPublicKey(&pubkey1));
  EXPECT_TRUE(keypair2->ExportPublicKey(&pubkey2));
  EXPECT_EQ(pubkey1, pubkey2);

  std::string raw_pubkey1;
  std::string raw_pubkey2;
  EXPECT_TRUE(keypair1->ExportRawPublicKey(&raw_pubkey1));
  EXPECT_TRUE(keypair2->ExportRawPublicKey(&raw_pubkey2));
  EXPECT_EQ(raw_pubkey1, raw_pubkey2);
}
#endif  // !defined(USE_OPENSSL)

TEST(ECPrivateKeyUnitTest, BadPasswordTest) {
  const std::string password1;
  const std::string password2 = "test";

  scoped_ptr<crypto::ECPrivateKey> keypair1(
      crypto::ECPrivateKey::Create());
  ASSERT_TRUE(keypair1.get());

  std::vector<uint8> privkey1;
  std::vector<uint8> pubkey1;
  ASSERT_TRUE(keypair1->ExportEncryptedPrivateKey(
      password1, 1, &privkey1));
  ASSERT_TRUE(keypair1->ExportPublicKey(&pubkey1));

  scoped_ptr<crypto::ECPrivateKey> keypair2(
      crypto::ECPrivateKey::CreateFromEncryptedPrivateKeyInfo(
          password2, privkey1, pubkey1));
  ASSERT_FALSE(keypair2.get());
}

TEST(ECPrivateKeyUnitTest, LoadNSSKeyTest) {
  static const unsigned char nss_key[] = {
      0x30, 0x81, 0xb8, 0x30, 0x23, 0x06, 0x0a, 0x2a, 0x86, 0x48, 0x86, 0xf7,
      0x0d, 0x01, 0x0c, 0x01, 0x03, 0x30, 0x15, 0x04, 0x10, 0x3f, 0xac, 0xe9,
      0x38, 0xdb, 0x40, 0x6b, 0x26, 0x89, 0x09, 0x73, 0x18, 0x8d, 0x7f, 0x1c,
      0x82, 0x02, 0x01, 0x01, 0x04, 0x81, 0x90, 0x5e, 0x5e, 0x11, 0xef, 0xbb,
      0x7c, 0x4d, 0xec, 0xc0, 0xdc, 0xc7, 0x23, 0xd2, 0xc4, 0x77, 0xbc, 0xf4,
      0x5d, 0x59, 0x4c, 0x07, 0xc2, 0x8a, 0x26, 0xfa, 0x25, 0x1c, 0xaa, 0x42,
      0xed, 0xd0, 0xed, 0xbb, 0x5c, 0xe9, 0x13, 0x07, 0xaa, 0xdd, 0x52, 0x3c,
      0x65, 0x25, 0xbf, 0x94, 0x02, 0xaf, 0xd6, 0x97, 0xe9, 0x33, 0x00, 0x76,
      0x64, 0x4a, 0x73, 0xab, 0xfb, 0x99, 0x6e, 0x83, 0x12, 0x05, 0x86, 0x72,
      0x6c, 0xd5, 0xa4, 0xcf, 0xb1, 0xd5, 0x4d, 0x54, 0x87, 0x8b, 0x4b, 0x95,
      0x1d, 0xcd, 0xf3, 0xfe, 0xa8, 0xda, 0xe0, 0xb6, 0x72, 0x13, 0x3f, 0x2e,
      0x66, 0xe0, 0xb9, 0x2e, 0xfa, 0x69, 0x40, 0xbe, 0xd7, 0x67, 0x6e, 0x53,
      0x2b, 0x3f, 0x53, 0xe5, 0x39, 0x54, 0x77, 0xe1, 0x1d, 0xe6, 0x81, 0x92,
      0x58, 0x82, 0x14, 0xfb, 0x47, 0x85, 0x3c, 0xc3, 0xdf, 0xdd, 0xcc, 0x79,
      0x9f, 0x41, 0x83, 0x72, 0xf2, 0x0a, 0xe9, 0xe1, 0x2c, 0x12, 0xb0, 0xb0,
      0x0a, 0xb2, 0x1d, 0xca, 0x15, 0xb2, 0xca};
  static const unsigned char nss_pub_key[] = {
      0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
      0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
      0x42, 0x00, 0x04, 0x85, 0x92, 0x9e, 0x95, 0x5c, 0x6b, 0x9e, 0xd6, 0x1e,
      0xb8, 0x64, 0xea, 0xc2, 0xb3, 0xef, 0x18, 0xed, 0x3a, 0x5e, 0xc4, 0x5c,
      0x15, 0x37, 0x6a, 0xe9, 0xaa, 0x0b, 0x34, 0x03, 0xfd, 0xca, 0x83, 0x0f,
      0xd7, 0x5c, 0x5d, 0xc5, 0x53, 0x6e, 0xe5, 0xa9, 0x33, 0xd5, 0xcc, 0xab,
      0x53, 0x78, 0xdd, 0xd6, 0x12, 0x3a, 0x5e, 0xeb, 0xbf, 0xdf, 0x16, 0xd3,
      0x2c, 0x3b, 0xe8, 0xdb, 0x19, 0xfc, 0x5e};

  scoped_ptr<crypto::ECPrivateKey> keypair_nss(
      crypto::ECPrivateKey::CreateFromEncryptedPrivateKeyInfo(
          "",
          std::vector<uint8>(nss_key, nss_key + arraysize(nss_key)),
          std::vector<uint8>(nss_pub_key,
                             nss_pub_key + arraysize(nss_pub_key))));

  EXPECT_TRUE(keypair_nss.get());
}

// Although the plan is to transition from OpenSSL to NSS, ensure NSS can import
// OpenSSL's format so that it is possible to rollback.
TEST(ECPrivateKeyUnitTest, LoadOpenSSLKeyTest) {
  static const unsigned char openssl_key[] = {
      0x30, 0x81, 0xb0, 0x30, 0x1b, 0x06, 0x0a, 0x2a, 0x86, 0x48, 0x86, 0xf7,
      0x0d, 0x01, 0x0c, 0x01, 0x03, 0x30, 0x0d, 0x04, 0x08, 0xb2, 0xfe, 0x68,
      0xc2, 0xea, 0x0f, 0x10, 0x9c, 0x02, 0x01, 0x01, 0x04, 0x81, 0x90, 0xe2,
      0xf6, 0x1c, 0xca, 0xad, 0x64, 0x30, 0xbf, 0x88, 0x04, 0x35, 0xe5, 0x0f,
      0x11, 0x49, 0x06, 0x01, 0x14, 0x33, 0x80, 0xa2, 0x78, 0x44, 0x5b, 0xaa,
      0x0d, 0xd7, 0x00, 0x36, 0x9d, 0x91, 0x97, 0x37, 0x20, 0x7b, 0x27, 0xc1,
      0xa0, 0xa2, 0x73, 0x06, 0x15, 0xdf, 0xc8, 0x13, 0x9b, 0xc9, 0x8c, 0x9c,
      0xce, 0x00, 0xd0, 0xc8, 0x42, 0xc1, 0xda, 0x2b, 0x07, 0x2b, 0x12, 0xa3,
      0xce, 0x10, 0x39, 0x7a, 0xf1, 0x55, 0x69, 0x8d, 0xa5, 0xc4, 0x2a, 0x00,
      0x0d, 0x94, 0xc6, 0xde, 0x6a, 0x3d, 0xb7, 0xe5, 0x6d, 0x59, 0x3e, 0x09,
      0xb5, 0xe3, 0x3e, 0xfc, 0x50, 0x56, 0xe9, 0x50, 0x42, 0x7c, 0xe7, 0xf0,
      0x19, 0xbd, 0x31, 0xa7, 0x85, 0x47, 0xb3, 0xe9, 0xb3, 0x50, 0x3c, 0xc9,
      0x32, 0x37, 0x1a, 0x93, 0x78, 0x48, 0x78, 0x82, 0xde, 0xad, 0x5c, 0xf2,
      0xcf, 0xf2, 0xbb, 0x2c, 0x44, 0x05, 0x7f, 0x4a, 0xf9, 0xb1, 0x2b, 0xdd,
      0x49, 0xf6, 0x7e, 0xd0, 0x42, 0xaa, 0x14, 0x3c, 0x24, 0x77, 0xb4};
  static const unsigned char openssl_pub_key[] = {
      0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
      0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
      0x42, 0x00, 0x04, 0xb9, 0xda, 0x0d, 0x71, 0x60, 0xb3, 0x63, 0x28, 0x22,
      0x67, 0xe7, 0xe0, 0xa3, 0xf8, 0x00, 0x8e, 0x4c, 0x89, 0xed, 0x31, 0x34,
      0xf6, 0xdb, 0xc4, 0xfe, 0x0b, 0x5d, 0xe1, 0x11, 0x39, 0x49, 0xa6, 0x50,
      0xa8, 0xe3, 0x4a, 0xc0, 0x40, 0x88, 0xb8, 0x38, 0x3f, 0x56, 0xfb, 0x33,
      0x8d, 0xd4, 0x64, 0x91, 0xd6, 0x15, 0x77, 0x42, 0x27, 0xc5, 0xaa, 0x44,
      0xff, 0xab, 0x4d, 0xb5, 0x7e, 0x25, 0x3d};

  scoped_ptr<crypto::ECPrivateKey> keypair_openssl(
      crypto::ECPrivateKey::CreateFromEncryptedPrivateKeyInfo(
          "",
          std::vector<uint8>(openssl_key, openssl_key + arraysize(openssl_key)),
          std::vector<uint8>(openssl_pub_key,
                             openssl_pub_key + arraysize(openssl_pub_key))));

  EXPECT_TRUE(keypair_openssl.get());
}

// The Android code writes out Channel IDs differently from the NSS
// implementation; the empty password is converted to "\0\0". The OpenSSL port
// should support either.
#if defined(USE_OPENSSL)
TEST(ECPrivateKeyUnitTest, LoadOldOpenSSLKeyTest) {
  static const unsigned char openssl_key[] = {
      0x30, 0x82, 0x01, 0xa1, 0x30, 0x1b, 0x06, 0x0a, 0x2a, 0x86, 0x48, 0x86,
      0xf7, 0x0d, 0x01, 0x0c, 0x01, 0x03, 0x30, 0x0d, 0x04, 0x08, 0x86, 0xaa,
      0xd7, 0xdf, 0x3b, 0x91, 0x97, 0x60, 0x02, 0x01, 0x01, 0x04, 0x82, 0x01,
      0x80, 0xcb, 0x2a, 0x14, 0xaa, 0x4f, 0x38, 0x4c, 0xe1, 0x49, 0x00, 0xe2,
      0x1a, 0x3a, 0x75, 0x87, 0x7e, 0x3d, 0xea, 0x4d, 0x53, 0xd4, 0x46, 0x47,
      0x23, 0x8f, 0xa1, 0x72, 0x51, 0x92, 0x86, 0x8b, 0xeb, 0x53, 0xe6, 0x6a,
      0x0a, 0x6b, 0xb6, 0xa0, 0xdc, 0x0f, 0xdc, 0x20, 0xc3, 0x45, 0x85, 0xf1,
      0x95, 0x90, 0x5c, 0xf4, 0xfa, 0xee, 0x47, 0xaf, 0x35, 0xd0, 0xd0, 0xd3,
      0x14, 0xde, 0x0d, 0xca, 0x1b, 0xd3, 0xbb, 0x20, 0xec, 0x9d, 0x6a, 0xd4,
      0xc1, 0xce, 0x60, 0x81, 0xab, 0x0c, 0x72, 0x10, 0xfa, 0x28, 0x3c, 0xac,
      0x87, 0x7b, 0x82, 0x85, 0x00, 0xb8, 0x58, 0x9c, 0x07, 0xc4, 0x7d, 0xa9,
      0xc5, 0x94, 0x95, 0xf7, 0x23, 0x93, 0x3f, 0xed, 0xef, 0x92, 0x55, 0x25,
      0x74, 0xbb, 0xd3, 0xd1, 0x67, 0x3b, 0x3d, 0x5a, 0xfe, 0x84, 0xf8, 0x97,
      0x7d, 0x7c, 0x01, 0xc7, 0xd7, 0x0d, 0xf8, 0xc3, 0x6d, 0xd6, 0xf1, 0xaa,
      0x9d, 0x1f, 0x69, 0x97, 0x45, 0x06, 0xc4, 0x1c, 0x95, 0x3c, 0xe0, 0xef,
      0x11, 0xb2, 0xb3, 0x72, 0x91, 0x9e, 0x7d, 0x0f, 0x7f, 0xc8, 0xf6, 0x64,
      0x49, 0x5e, 0x3c, 0x53, 0x37, 0x79, 0x03, 0x1c, 0x3f, 0x29, 0x6c, 0x6b,
      0xea, 0x4c, 0x35, 0x9b, 0x6d, 0x1b, 0x59, 0x43, 0x4c, 0x14, 0x47, 0x2a,
      0x36, 0x39, 0x2a, 0xd8, 0x96, 0x90, 0xdc, 0xfc, 0xd2, 0xdd, 0x23, 0x0e,
      0x2c, 0xb3, 0x83, 0xf9, 0xf2, 0xe3, 0xe6, 0x99, 0x53, 0x57, 0x33, 0xc5,
      0x5f, 0xf9, 0xfd, 0x56, 0x0b, 0x32, 0xd4, 0xf3, 0x9d, 0x5b, 0x34, 0xe5,
      0x94, 0xbf, 0xb6, 0xc0, 0xce, 0xe1, 0x73, 0x5c, 0x02, 0x7a, 0x4c, 0xed,
      0xde, 0x23, 0x38, 0x89, 0x9f, 0xcd, 0x51, 0xf3, 0x90, 0x80, 0xd3, 0x4b,
      0x83, 0xd3, 0xee, 0xf2, 0x9e, 0x35, 0x91, 0xa5, 0xa3, 0xc0, 0x5c, 0xce,
      0xdb, 0xaa, 0x70, 0x1e, 0x1d, 0xc1, 0x44, 0xea, 0x3b, 0xa7, 0x5a, 0x11,
      0xd1, 0xf3, 0xf3, 0xd0, 0xf4, 0x5a, 0xc4, 0x99, 0xaf, 0x8d, 0xe2, 0xbc,
      0xa2, 0xb9, 0x3d, 0x86, 0x5e, 0xba, 0xa0, 0xdf, 0x78, 0x81, 0x7c, 0x54,
      0x31, 0xe3, 0x98, 0xb5, 0x46, 0xcb, 0x4d, 0x26, 0x4b, 0xf8, 0xac, 0x3a,
      0x54, 0x1b, 0x77, 0x5a, 0x18, 0xa5, 0x43, 0x0e, 0x14, 0xde, 0x7b, 0xb7,
      0x4e, 0x45, 0x99, 0x03, 0xd1, 0x3d, 0x18, 0xb2, 0x36, 0x00, 0x48, 0x07,
      0x72, 0xbb, 0x4f, 0x21, 0x25, 0x3e, 0xda, 0x25, 0x24, 0x5b, 0xc8, 0xa0,
      0x28, 0xd5, 0x9b, 0x96, 0x87, 0x07, 0x77, 0x84, 0xff, 0xd7, 0xac, 0x71,
      0xf6, 0x61, 0x63, 0x0b, 0xfb, 0x42, 0xfd, 0x52, 0xf4, 0xc4, 0x35, 0x0c,
      0xc2, 0xc1, 0x55, 0x22, 0x42, 0x2f, 0x13, 0x7d, 0x93, 0x27, 0xc8, 0x11,
      0x35, 0xc5, 0xe3, 0xc5, 0xaa, 0x15, 0x3c, 0xac, 0x30, 0xbc, 0x45, 0x16,
      0xed};
  static const unsigned char openssl_pub_key[] = {
      0x30, 0x82, 0x01, 0x4b, 0x30, 0x82, 0x01, 0x03, 0x06, 0x07, 0x2a, 0x86,
      0x48, 0xce, 0x3d, 0x02, 0x01, 0x30, 0x81, 0xf7, 0x02, 0x01, 0x01, 0x30,
      0x2c, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01, 0x01, 0x02, 0x21,
      0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x30, 0x5b, 0x04,
      0x20, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x04, 0x20, 0x5a,
      0xc6, 0x35, 0xd8, 0xaa, 0x3a, 0x93, 0xe7, 0xb3, 0xeb, 0xbd, 0x55, 0x76,
      0x98, 0x86, 0xbc, 0x65, 0x1d, 0x06, 0xb0, 0xcc, 0x53, 0xb0, 0xf6, 0x3b,
      0xce, 0x3c, 0x3e, 0x27, 0xd2, 0x60, 0x4b, 0x03, 0x15, 0x00, 0xc4, 0x9d,
      0x36, 0x08, 0x86, 0xe7, 0x04, 0x93, 0x6a, 0x66, 0x78, 0xe1, 0x13, 0x9d,
      0x26, 0xb7, 0x81, 0x9f, 0x7e, 0x90, 0x04, 0x41, 0x04, 0x6b, 0x17, 0xd1,
      0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6, 0xe5, 0x63, 0xa4, 0x40,
      0xf2, 0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb, 0x33, 0xa0, 0xf4, 0xa1, 0x39,
      0x45, 0xd8, 0x98, 0xc2, 0x96, 0x4f, 0xe3, 0x42, 0xe2, 0xfe, 0x1a, 0x7f,
      0x9b, 0x8e, 0xe7, 0xeb, 0x4a, 0x7c, 0x0f, 0x9e, 0x16, 0x2b, 0xce, 0x33,
      0x57, 0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68, 0x37, 0xbf, 0x51,
      0xf5, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbc, 0xe6, 0xfa, 0xad,
      0xa7, 0x17, 0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc, 0x63, 0x25, 0x51,
      0x02, 0x01, 0x01, 0x03, 0x42, 0x00, 0x04, 0xde, 0x09, 0x08, 0x07, 0x03,
      0x2e, 0x8f, 0x37, 0x9a, 0xd5, 0xad, 0xe5, 0xc6, 0x9d, 0xd4, 0x63, 0xc7,
      0x4a, 0xe7, 0x20, 0xcb, 0x90, 0xa0, 0x1f, 0x18, 0x18, 0x72, 0xb5, 0x21,
      0x88, 0x38, 0xc0, 0xdb, 0xba, 0xf6, 0x99, 0xd8, 0xa5, 0x3b, 0x83, 0xe9,
      0xe3, 0xd5, 0x61, 0x99, 0x73, 0x42, 0xc6, 0x6c, 0xe8, 0x0a, 0x95, 0x40,
      0x41, 0x3b, 0x0d, 0x10, 0xa7, 0x4a, 0x93, 0xdb, 0x5a, 0xe7, 0xec};

  scoped_ptr<crypto::ECPrivateKey> keypair_openssl(
      crypto::ECPrivateKey::CreateFromEncryptedPrivateKeyInfo(
          "",
          std::vector<uint8>(openssl_key, openssl_key + arraysize(openssl_key)),
          std::vector<uint8>(openssl_pub_key,
                             openssl_pub_key + arraysize(openssl_pub_key))));

  EXPECT_TRUE(keypair_openssl.get());
}
#endif  // defined(USE_OPENSSL)
