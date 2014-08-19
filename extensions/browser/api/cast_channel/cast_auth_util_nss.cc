// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/cast_channel/cast_auth_util.h"

#include <cert.h>
#include <cryptohi.h>
#include <pk11pub.h>
#include <seccomon.h>
#include <string>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "crypto/nss_util.h"
#include "crypto/scoped_nss_types.h"
#include "extensions/browser/api/cast_channel/cast_channel.pb.h"
#include "extensions/browser/api/cast_channel/cast_message_util.h"
#include "net/base/hash_value.h"
#include "net/cert/x509_certificate.h"

namespace extensions {
namespace core_api {
namespace cast_channel {

namespace {

// Fingerprints and public keys of the allowed / trusted ICAs.
static const net::SHA1HashValue kFingerprintICA1 = { {
    0x57,0x16,0xE2,0xAD,0x73,0x2E,0xBE,0xDA,0xEB,0x18,
    0xE8,0x47,0x15,0xA8,0xDE,0x90,0x3B,0x5E,0x2A,0xF4
} };
static const unsigned char kPublicKeyICA1[] = {
    0x30,0x82,0x01,0x0A,0x02,0x82,0x01,0x01,0x00,0xBC,0x22,0x80,
    0xBD,0x80,0xF6,0x3A,0x21,0x00,0x3B,0xAE,0x76,0x5E,0x35,0x7F,
    0x3D,0xC3,0x64,0x5C,0x55,0x94,0x86,0x34,0x2F,0x05,0x87,0x28,
    0xCD,0xF7,0x69,0x8C,0x17,0xB3,0x50,0xA7,0xB8,0x82,0xFA,0xDF,
    0xC7,0x43,0x2D,0xD6,0x7E,0xAB,0xA0,0x6F,0xB7,0x13,0x72,0x80,
    0xA4,0x47,0x15,0xC1,0x20,0x99,0x50,0xCD,0xEC,0x14,0x62,0x09,
    0x5B,0xA4,0x98,0xCD,0xD2,0x41,0xB6,0x36,0x4E,0xFF,0xE8,0x2E,
    0x32,0x30,0x4A,0x81,0xA8,0x42,0xA3,0x6C,0x9B,0x33,0x6E,0xCA,
    0xB2,0xF5,0x53,0x66,0xE0,0x27,0x53,0x86,0x1A,0x85,0x1E,0xA7,
    0x39,0x3F,0x4A,0x77,0x8E,0xFB,0x54,0x66,0x66,0xFB,0x58,0x54,
    0xC0,0x5E,0x39,0xC7,0xF5,0x50,0x06,0x0B,0xE0,0x8A,0xD4,0xCE,
    0xE1,0x6A,0x55,0x1F,0x8B,0x17,0x00,0xE6,0x69,0xA3,0x27,0xE6,
    0x08,0x25,0x69,0x3C,0x12,0x9D,0x8D,0x05,0x2C,0xD6,0x2E,0xA2,
    0x31,0xDE,0xB4,0x52,0x50,0xD6,0x20,0x49,0xDE,0x71,0xA0,0xF9,
    0xAD,0x20,0x40,0x12,0xF1,0xDD,0x25,0xEB,0xD5,0xE6,0xB8,0x36,
    0xF4,0xD6,0x8F,0x7F,0xCA,0x43,0xDC,0xD7,0x10,0x5B,0xE6,0x3F,
    0x51,0x8A,0x85,0xB3,0xF3,0xFF,0xF6,0x03,0x2D,0xCB,0x23,0x4F,
    0x9C,0xAD,0x18,0xE7,0x93,0x05,0x8C,0xAC,0x52,0x9A,0xF7,0x4C,
    0xE9,0x99,0x7A,0xBE,0x6E,0x7E,0x4D,0x0A,0xE3,0xC6,0x1C,0xA9,
    0x93,0xFA,0x3A,0xA5,0x91,0x5D,0x1C,0xBD,0x66,0xEB,0xCC,0x60,
    0xDC,0x86,0x74,0xCA,0xCF,0xF8,0x92,0x1C,0x98,0x7D,0x57,0xFA,
    0x61,0x47,0x9E,0xAB,0x80,0xB7,0xE4,0x48,0x80,0x2A,0x92,0xC5,
    0x1B,0x02,0x03,0x01,0x00,0x01
};

static const net::SHA1HashValue kFingerprintICA2 = { {
    0x1B,0xA2,0x9E,0xC9,0x8E,0x4E,0xB3,0x80,0xEE,0x55,
    0xB2,0x97,0xFD,0x2E,0x2B,0x2C,0xB6,0x8E,0x0B,0x2F
} };
static const unsigned char kPublicKeyICA2[] = {
    0x30,0x82,0x01,0x0A,0x02,0x82,0x01,0x01,0x00,0xBC,0x22,0x80,
    0xBD,0x80,0xF6,0x3A,0x21,0x00,0x3B,0xAE,0x76,0x5E,0x35,0x7F,
    0x3D,0xC3,0x64,0x5C,0x55,0x94,0x86,0x34,0x2F,0x05,0x87,0x28,
    0xCD,0xF7,0x69,0x8C,0x17,0xB3,0x50,0xA7,0xB8,0x82,0xFA,0xDF,
    0xC7,0x43,0x2D,0xD6,0x7E,0xAB,0xA0,0x6F,0xB7,0x13,0x72,0x80,
    0xA4,0x47,0x15,0xC1,0x20,0x99,0x50,0xCD,0xEC,0x14,0x62,0x09,
    0x5B,0xA4,0x98,0xCD,0xD2,0x41,0xB6,0x36,0x4E,0xFF,0xE8,0x2E,
    0x32,0x30,0x4A,0x81,0xA8,0x42,0xA3,0x6C,0x9B,0x33,0x6E,0xCA,
    0xB2,0xF5,0x53,0x66,0xE0,0x27,0x53,0x86,0x1A,0x85,0x1E,0xA7,
    0x39,0x3F,0x4A,0x77,0x8E,0xFB,0x54,0x66,0x66,0xFB,0x58,0x54,
    0xC0,0x5E,0x39,0xC7,0xF5,0x50,0x06,0x0B,0xE0,0x8A,0xD4,0xCE,
    0xE1,0x6A,0x55,0x1F,0x8B,0x17,0x00,0xE6,0x69,0xA3,0x27,0xE6,
    0x08,0x25,0x69,0x3C,0x12,0x9D,0x8D,0x05,0x2C,0xD6,0x2E,0xA2,
    0x31,0xDE,0xB4,0x52,0x50,0xD6,0x20,0x49,0xDE,0x71,0xA0,0xF9,
    0xAD,0x20,0x40,0x12,0xF1,0xDD,0x25,0xEB,0xD5,0xE6,0xB8,0x36,
    0xF4,0xD6,0x8F,0x7F,0xCA,0x43,0xDC,0xD7,0x10,0x5B,0xE6,0x3F,
    0x51,0x8A,0x85,0xB3,0xF3,0xFF,0xF6,0x03,0x2D,0xCB,0x23,0x4F,
    0x9C,0xAD,0x18,0xE7,0x93,0x05,0x8C,0xAC,0x52,0x9A,0xF7,0x4C,
    0xE9,0x99,0x7A,0xBE,0x6E,0x7E,0x4D,0x0A,0xE3,0xC6,0x1C,0xA9,
    0x93,0xFA,0x3A,0xA5,0x91,0x5D,0x1C,0xBD,0x66,0xEB,0xCC,0x60,
    0xDC,0x86,0x74,0xCA,0xCF,0xF8,0x92,0x1C,0x98,0x7D,0x57,0xFA,
    0x61,0x47,0x9E,0xAB,0x80,0xB7,0xE4,0x48,0x80,0x2A,0x92,0xC5,
    0x1B,0x02,0x03,0x01,0x00,0x01
};

static const net::SHA1HashValue kFingerprintICA3 = { {
    0x97,0x05,0xCE,0xF6,0x3F,0xA9,0x5E,0x0F,0xE7,0x61,
    0xFB,0x08,0x44,0x31,0xBE,0xDE,0x01,0xB8,0xFB,0xEB
} };
static const unsigned char kPublicKeyICA3[] = {
    0x30,0x82,0x01,0x0A,0x02,0x82,0x01,0x01,0x00,0xB7,0xE8,0xC3,
    0xE4,0x2C,0xDE,0x74,0x53,0xF2,0x49,0x95,0x6D,0xD1,0xDA,0x69,
    0x57,0x0D,0x86,0xE5,0xED,0xB4,0xB9,0xE6,0x73,0x9F,0x6C,0xAD,
    0x3B,0x64,0x85,0x03,0x0D,0x08,0x44,0xAF,0x18,0x69,0x82,0xAD,
    0xA9,0x74,0x64,0x37,0x47,0xE1,0xE7,0x26,0x19,0x33,0x3C,0xE2,
    0xD0,0xB5,0x84,0x3C,0xD7,0xAC,0x63,0xAE,0xC4,0x32,0x23,0xF6,
    0xDC,0x14,0x10,0x4B,0x95,0x7F,0xE8,0x98,0xD7,0x7A,0x9E,0x43,
    0x3D,0x68,0x8B,0x2A,0x70,0xF7,0x1E,0x43,0x70,0xBA,0xA5,0xA5,
    0x93,0xAD,0x8A,0xD4,0x9F,0xAC,0x83,0x16,0xF3,0x48,0x5F,0xC5,
    0xE0,0xA5,0x44,0xB8,0x4F,0xD9,0xD8,0x75,0x90,0x25,0x8B,0xE3,
    0x1C,0x6C,0xDA,0x88,0xFF,0x09,0x2B,0xCA,0x1E,0x48,0xDD,0x76,
    0x0F,0x68,0x56,0x7B,0x15,0x9D,0xCA,0x6B,0x1C,0xF7,0x48,0xC2,
    0x89,0xC6,0x93,0x0A,0x31,0xF2,0x78,0x27,0x45,0x3D,0xF1,0x0D,
    0x5B,0x6E,0x55,0x32,0xEF,0x49,0xA0,0xD6,0xAF,0xA6,0x30,0x91,
    0xF2,0x21,0x2F,0xDB,0xA4,0x29,0xB9,0x9B,0x22,0xBC,0xCD,0x0B,
    0xA6,0x8B,0xA6,0x22,0x79,0xFD,0xCF,0x95,0x93,0x96,0xB3,0x23,
    0xC9,0xC6,0x30,0x8E,0xC0,0xE9,0x1F,0xEC,0xFB,0xF5,0x88,0xDD,
    0x97,0x72,0x16,0x29,0x08,0xFA,0x42,0xE7,0x4F,0xCA,0xAE,0xD7,
    0x0F,0x23,0x48,0x9B,0x82,0xA7,0x37,0x4A,0xDD,0x60,0x04,0x75,
    0xDC,0xDE,0x09,0x98,0xD2,0x16,0x23,0x04,0x70,0x4D,0x99,0x9F,
    0x4A,0x82,0x28,0xE6,0xBE,0x8F,0x9D,0xBF,0xA1,0x4B,0xA2,0xBA,
    0xF5,0xB2,0x51,0x1E,0x4E,0xE7,0x80,0x9E,0x7A,0x38,0xA1,0xC7,
    0x09,0x02,0x03,0x01,0x00,0x01
};

static const net::SHA1HashValue kFingerprintICA4 = { {
    0x01,0xF5,0x28,0x56,0x33,0x80,0x9B,0x31,0xE7,0xD9,
    0xF7,0x4E,0xAA,0xDD,0x97,0x37,0xA0,0x28,0xE7,0x24
} };
static const unsigned char kPublicKeyICA4[] = {
    0x30,0x82,0x01,0x0A,0x02,0x82,0x01,0x01,0x00,0xB0,0x0E,0x5E,
    0x07,0x3A,0xDF,0xA4,0x5F,0x68,0xF7,0x21,0xC7,0x64,0xDB,0xB6,
    0x76,0xEF,0xEE,0x8B,0x93,0xF8,0xF6,0x1B,0x88,0xE1,0x93,0xB7,
    0x17,0xF0,0x15,0x1E,0x7E,0x52,0x55,0x77,0x3C,0x02,0x8D,0x7B,
    0x4A,0x6C,0xD3,0xBD,0xD6,0xC1,0x9C,0x72,0xC8,0xB3,0x15,0xCF,
    0x11,0xC1,0xF5,0x46,0xC4,0xD5,0x20,0x47,0xFB,0x30,0xF4,0xE4,
    0x61,0x0C,0x68,0xF0,0x5E,0xAB,0x37,0x8E,0x9B,0xE1,0xBC,0x81,
    0xC3,0x70,0x8A,0x78,0xD6,0x83,0x34,0x32,0x9C,0x19,0x62,0xEB,
    0xE4,0x9C,0xED,0xE3,0x64,0x6C,0x41,0x1D,0x9C,0xD2,0x8B,0x48,
    0x4C,0x23,0x90,0x95,0xB3,0xE7,0x52,0xEA,0x05,0x57,0xCC,0x60,
    0xB3,0xBA,0x14,0xE4,0xBA,0x00,0x39,0xE4,0x46,0x55,0x74,0xCE,
    0x5A,0x8E,0x7A,0x67,0x23,0xDA,0x68,0x0A,0xFA,0xC4,0x84,0x1E,
    0xB4,0xC5,0xA1,0xA2,0x6A,0x73,0x1F,0x6E,0xC8,0x2E,0x2F,0x9A,
    0x9E,0xA8,0xB1,0x0E,0xFD,0x87,0xA6,0x8F,0x4D,0x3D,0x4B,0x05,
    0xD5,0x35,0x5A,0x74,0x4D,0xBC,0x8E,0x82,0x44,0x96,0xF4,0xB5,
    0x95,0x60,0x4E,0xA5,0xDF,0x27,0x3D,0x41,0x5C,0x07,0xA3,0xB4,
    0x35,0x5A,0xB3,0x9E,0xF2,0x05,0x24,0xCA,0xCD,0x31,0x5A,0x0D,
    0x26,0x4C,0xD4,0xD3,0xFD,0x50,0xE1,0x34,0xE9,0x4C,0x81,0x58,
    0x30,0xB2,0xC7,0x7A,0xDD,0x81,0x89,0xA6,0xD4,0x3A,0x38,0x84,
    0x03,0xB7,0x34,0x9E,0x77,0x3F,0xFF,0x78,0x07,0x5B,0x99,0xC1,
    0xB2,0x1F,0x35,0x56,0x6E,0x3A,0x3C,0x0C,0x25,0xE1,0x57,0xF6,
    0x8A,0x7E,0x49,0xC0,0xCC,0x83,0x11,0x35,0xE7,0x91,0x6D,0x2E,
    0x65,0x02,0x03,0x01,0x00,0x01
};

// Info for trusted ICA certs.
struct ICACertInfo {
  const net::SHA1HashValue* fingerprint;
  SECItem public_key;
};

// List of allowed / trusted ICAs.
static const ICACertInfo kAllowedICAs[] = {
  { &kFingerprintICA1,
    { siDERCertBuffer,
      const_cast<unsigned char*>(kPublicKeyICA1),
      sizeof(kPublicKeyICA1) } },
  { &kFingerprintICA2,
    { siDERCertBuffer,
      const_cast<unsigned char*>(kPublicKeyICA2),
      sizeof(kPublicKeyICA2) } },
  { &kFingerprintICA3,
    { siDERCertBuffer,
      const_cast<unsigned char*>(kPublicKeyICA3),
      sizeof(kPublicKeyICA3) } },
  { &kFingerprintICA4,
    { siDERCertBuffer,
      const_cast<unsigned char*>(kPublicKeyICA4),
      sizeof(kPublicKeyICA4) } },
};

typedef scoped_ptr<
    CERTCertificate,
    crypto::NSSDestroyer<CERTCertificate, CERT_DestroyCertificate> >
        ScopedCERTCertificate;

// Returns the index of the ICA whose fingerprint matches |fingerprint|.
// Returns -1, if no such ICA is found.
static int GetICAWithFingerprint(const net::SHA1HashValue& fingerprint) {
  for (size_t i = 0; i < arraysize(kAllowedICAs); ++i) {
    if (kAllowedICAs[i].fingerprint->Equals(fingerprint))
      return static_cast<int>(i);
  }
  return -1;
}

// Parses out DeviceAuthMessage from CastMessage
static AuthResult ParseAuthMessage(const CastMessage& challenge_reply,
                                   DeviceAuthMessage* auth_message) {
  const std::string kErrorPrefix("Failed to parse auth message: ");
  if (challenge_reply.payload_type() != CastMessage_PayloadType_BINARY) {
    return AuthResult::Create(
        kErrorPrefix + "Wrong payload type in challenge reply",
        AuthResult::ERROR_WRONG_PAYLOAD_TYPE);
  }
  if (!challenge_reply.has_payload_binary()) {
    return AuthResult::Create(
        kErrorPrefix +
            "Payload type is binary but payload_binary field not set",
        AuthResult::ERROR_NO_PAYLOAD);
  }
  if (!auth_message->ParseFromString(challenge_reply.payload_binary())) {
    return AuthResult::Create(
        kErrorPrefix + "Cannot parse binary payload into DeviceAuthMessage",
        AuthResult::ERROR_PAYLOAD_PARSING_FAILED);
  }

  VLOG(1) << "Auth message: " << AuthMessageToString(*auth_message);

  if (auth_message->has_error()) {
    std::string error_format_str = kErrorPrefix + "Auth message error: %d";
    return AuthResult::Create(
        base::StringPrintf(error_format_str.c_str(),
                           auth_message->error().error_type()),
        AuthResult::ERROR_MESSAGE_ERROR);
  }
  if (!auth_message->has_response()) {
    return AuthResult::Create(
        kErrorPrefix + "Auth message has no response field",
        AuthResult::ERROR_NO_RESPONSE);
  }
  return AuthResult();
}

// Authenticates the given credentials:
// 1. |signature| verification of |data| using |certificate|.
// 2. |certificate| is signed by a trusted CA.
AuthResult VerifyCredentials(const AuthResponse& response,
                             const std::string& data) {
  const std::string kErrorPrefix("Failed to verify credentials: ");
  const std::string& certificate = response.client_auth_certificate();
  const std::string& signature = response.signature();

  const SECItem* trusted_ca_key_der;

  // If the list of intermediates is empty then use kPublicKeyICA1 as
  // the trusted CA (legacy case).
  // Otherwise, use the first intermediate in the list as long as it
  // is in the allowed list of intermediates.
  int num_intermediates = response.intermediate_certificate_size();

  VLOG(1) << "Response has " << num_intermediates << " intermediates";

  if (num_intermediates <= 0) {
    trusted_ca_key_der = &kAllowedICAs[0].public_key;
  } else {
    const std::string& ica = response.intermediate_certificate(0);
    scoped_refptr<net::X509Certificate> ica_cert
        = net::X509Certificate::CreateFromBytes(ica.data(), ica.length());
    int index = GetICAWithFingerprint(ica_cert->fingerprint());
    if (index == -1) {
      return AuthResult::Create(kErrorPrefix + "Disallowed intermediate cert",
                                AuthResult::ERROR_FINGERPRINT_NOT_FOUND);
    }
    trusted_ca_key_der = &kAllowedICAs[index].public_key;
  }

  crypto::EnsureNSSInit();
  SECItem der_cert;
  der_cert.type = siDERCertBuffer;
  // Make a copy of certificate string so it is safe to type cast.
  der_cert.data = reinterpret_cast<unsigned char*>(const_cast<char*>(
      certificate.data()));
  der_cert.len = certificate.length();

  // Parse into a certificate structure.
  ScopedCERTCertificate cert(CERT_NewTempCertificate(
      CERT_GetDefaultCertDB(), &der_cert, NULL, PR_FALSE, PR_TRUE));
  if (!cert.get()) {
    return AuthResult::CreateWithNSSError(
        kErrorPrefix + "Failed to parse certificate.",
        AuthResult::ERROR_NSS_CERT_PARSING_FAILED,
        PORT_GetError());
  }

  // Check that the certificate is signed by trusted CA.
  // NOTE: We const_cast trusted_ca_key_der since on some platforms
  // SECKEY_ImportDERPublicKey API takes in SECItem* and not const
  // SECItem*.
  crypto::ScopedSECKEYPublicKey ca_public_key(
      SECKEY_ImportDERPublicKey(
          const_cast<SECItem*>(trusted_ca_key_der), CKK_RSA));
  SECStatus verified = CERT_VerifySignedDataWithPublicKey(
      &cert->signatureWrap, ca_public_key.get(), NULL);
  if (verified != SECSuccess) {
    return AuthResult::CreateWithNSSError(
        kErrorPrefix + "Cert not signed by trusted CA",
        AuthResult::ERROR_NSS_CERT_NOT_SIGNED_BY_TRUSTED_CA,
        PORT_GetError());
  }

  VLOG(1) << "Cert signed by trusted CA";

  // Verify that the |signature| matches |data|.
  crypto::ScopedSECKEYPublicKey public_key(CERT_ExtractPublicKey(cert.get()));
  if (!public_key.get()) {
    return AuthResult::CreateWithNSSError(
        kErrorPrefix + "Unable to extract public key from certificate",
        AuthResult::ERROR_NSS_CANNOT_EXTRACT_PUBLIC_KEY,
        PORT_GetError());
  }
  SECItem signature_item;
  signature_item.type = siBuffer;
  signature_item.data = reinterpret_cast<unsigned char*>(
      const_cast<char*>(signature.data()));
  signature_item.len = signature.length();
  verified = VFY_VerifyDataDirect(
      reinterpret_cast<unsigned char*>(const_cast<char*>(data.data())),
      data.size(),
      public_key.get(),
      &signature_item,
      SEC_OID_PKCS1_RSA_ENCRYPTION,
      SEC_OID_SHA1, NULL, NULL);

  if (verified != SECSuccess) {
    return AuthResult::CreateWithNSSError(
        kErrorPrefix + "Signed blobs did not match",
        AuthResult::ERROR_NSS_SIGNED_BLOBS_MISMATCH,
        PORT_GetError());
  }

  VLOG(1) << "Signature verification succeeded";

  return AuthResult();
}

}  // namespace

AuthResult AuthenticateChallengeReply(const CastMessage& challenge_reply,
                                      const std::string& peer_cert) {
  if (peer_cert.empty()) {
    AuthResult result = AuthResult::Create("Peer cert was empty.",
                                           AuthResult::ERROR_PEER_CERT_EMPTY);
    VLOG(1) << result.error_message;
    return result;
  }

  VLOG(1) << "Challenge reply: " << CastMessageToString(challenge_reply);
  DeviceAuthMessage auth_message;
  AuthResult result = ParseAuthMessage(challenge_reply, &auth_message);
  if (!result.success()) {
    VLOG(1) << result.error_message;
    return result;
  }

  const AuthResponse& response = auth_message.response();
  result = VerifyCredentials(response, peer_cert);
  if (!result.success()) {
    VLOG(1) << result.error_message
            << ", NSS error code: " << result.nss_error_code;
    return result;
  }

  return AuthResult();
}

}  // namespace cast_channel
}  // namespace core_api
}  // namespace extensions
