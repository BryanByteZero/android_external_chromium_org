// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/ct_serialization.h"

#include "base/basictypes.h"
#include "base/logging.h"

namespace net {

namespace ct {

namespace {

// Note: length is always specified in bytes.
// Signed Certificate Timestamp (SCT) Version length
const size_t kVersionLength = 1;

// Members of a V1 SCT
const size_t kLogIdLength = 32;
const size_t kTimestampLength = 8;
const size_t kExtensionsLengthBytes = 2;
const size_t kHashAlgorithmLength = 1;
const size_t kSigAlgorithmLength = 1;
const size_t kSignatureLengthBytes = 2;

// Members of the digitally-signed struct of a V1 SCT
const size_t kSignatureTypeLength = 1;
const size_t kLogEntryTypeLength = 2;
const size_t kAsn1CertificateLengthBytes = 3;
const size_t kTbsCertificateLengthBytes = 3;

const size_t kSCTListLengthBytes = 2;
const size_t kSerializedSCTLengthBytes = 2;

enum SignatureType {
  SIGNATURE_TYPE_CERTIFICATE_TIMESTAMP = 0,
  TREE_HASH = 1,
};

// Reads a TLS-encoded variable length unsigned integer from |in|.
// The integer is expected to be in big-endian order, which is used by TLS.
// The bytes read from |in| are discarded (i.e. |in|'s prefix removed)
// |length| indicates the size (in bytes) of the integer. On success, returns
// true and stores the result in |*out|.
template <typename T>
bool ReadUint(size_t length, base::StringPiece* in, T* out) {
  if (in->size() < length)
    return false;
  DCHECK_LE(length, sizeof(T));

  T result = 0;
  for (size_t i = 0; i < length; ++i) {
    result = (result << 8) | static_cast<unsigned char>((*in)[i]);
  }
  in->remove_prefix(length);
  *out = result;
  return true;
}

// Reads a TLS-encoded field length from |in|.
// The bytes read from |in| are discarded (i.e. |in|'s prefix removed)
// |prefix_length| indicates the bytes needed to represent the length (e.g. 3)
// success, returns true and stores the result in |*out|.
bool ReadLength(size_t prefix_length, base::StringPiece* in, size_t* out) {
  size_t length;
  if (!ReadUint(prefix_length, in, &length))
    return false;
  *out = length;
  return true;
}

// Reads |length| bytes from |*in|. If |*in| is too small, returns false.
// The bytes read from |in| are discarded (i.e. |in|'s prefix removed)
bool ReadFixedBytes(size_t length,
                    base::StringPiece* in,
                    base::StringPiece* out) {
  if (in->length() < length)
    return false;
  out->set(in->data(), length);
  in->remove_prefix(length);
  return true;
}

// Reads a length-prefixed variable amount of bytes from |in|, updating |out|
// on success. |prefix_length| indicates the number of bytes needed to represent
// the length.
// The bytes read from |in| are discarded (i.e. |in|'s prefix removed)
bool ReadVariableBytes(size_t prefix_length,
                       base::StringPiece* in,
                       base::StringPiece* out) {
  size_t length;
  if (!ReadLength(prefix_length, in, &length))
    return false;
  return ReadFixedBytes(length, in, out);
}

// Reads a variable-length list that has been TLS encoded.
// The bytes read from |in| are discarded (i.e. |in|'s prefix removed)
// |max_list_length| contains the overall length of the encoded list.
// |max_item_length| contains the maximum length of a single item.
// On success, returns true and updates |*out| with the encoded list.
bool ReadList(size_t max_list_length,
              size_t max_item_length,
              base::StringPiece* in,
              std::vector<base::StringPiece>* out) {
  std::vector<base::StringPiece> result;

  base::StringPiece list_data;
  if (!ReadVariableBytes(max_list_length, in, &list_data))
    return false;

  while (!list_data.empty()) {
    base::StringPiece list_item;
    if (!ReadVariableBytes(max_item_length, &list_data, &list_item)) {
      DVLOG(1) << "Failed to read item in list.";
      return false;
    }
    if (list_item.empty()) {
      DVLOG(1) << "Empty item in list";
      return false;
    }
    result.push_back(list_item);
  }

  result.swap(*out);
  return true;
}

// Checks and converts a hash algorithm.
// |in| is the numeric representation of the algorithm.
// If the hash algorithm value is in a set of known values, fills in |out| and
// returns true. Otherwise, returns false.
bool ConvertHashAlgorithm(unsigned in, DigitallySigned::HashAlgorithm* out) {
  switch (in) {
    case DigitallySigned::HASH_ALGO_NONE:
    case DigitallySigned::HASH_ALGO_MD5:
    case DigitallySigned::HASH_ALGO_SHA1:
    case DigitallySigned::HASH_ALGO_SHA224:
    case DigitallySigned::HASH_ALGO_SHA256:
    case DigitallySigned::HASH_ALGO_SHA384:
    case DigitallySigned::HASH_ALGO_SHA512:
      break;
    default:
      return false;
  }
  *out = static_cast<DigitallySigned::HashAlgorithm>(in);
  return true;
}

// Checks and converts a signing algorithm.
// |in| is the numeric representation of the algorithm.
// If the signing algorithm value is in a set of known values, fills in |out|
// and returns true. Otherwise, returns false.
bool ConvertSignatureAlgorithm(
    unsigned in,
    DigitallySigned::SignatureAlgorithm* out) {
  switch (in) {
    case DigitallySigned::SIG_ALGO_ANONYMOUS:
    case DigitallySigned::SIG_ALGO_RSA:
    case DigitallySigned::SIG_ALGO_DSA:
    case DigitallySigned::SIG_ALGO_ECDSA:
      break;
    default:
      return false;
  }
  *out = static_cast<DigitallySigned::SignatureAlgorithm>(in);
  return true;
}

// Writes a TLS-encoded variable length unsigned integer to |output|.
// |length| indicates the size (in bytes) of the integer.
// |value| the value itself to be written.
template <typename T>
void WriteUint(size_t length, T value, std::string* output) {
  DCHECK_LE(length, sizeof(T));
  DCHECK(length == sizeof(T) || value >> (length * 8) == 0);

  for (; length > 0; --length) {
    output->push_back((value >> ((length - 1)* 8)) & 0xFF);
  }
}

// Writes an array to |output| from |input|.
// Should be used in one of two cases:
// * The length of |input| has already been encoded into the |output| stream.
// * The length of |input| is fixed and the reader is expected to specify that
// length when reading.
// If the length of |input| is dynamic and data is expected to follow it,
// WriteVariableBytes must be used.
void WriteEncodedBytes(const base::StringPiece& input, std::string* output) {
  input.AppendToString(output);
}

// Writes a variable-length array to |output|.
// |prefix_length| indicates the number of bytes needed to represnt the length.
// |input| is the array itself.
// If the size of |input| is less than 2^|prefix_length| - 1, encode the
// length and data and return true. Otherwise, return false.
bool WriteVariableBytes(size_t prefix_length,
                        const base::StringPiece& input,
                        std::string* output) {
  size_t input_size = input.size();
  size_t max_allowed_input_size =
      static_cast<size_t>(((1 << (prefix_length * 8)) - 1));
  if (input_size > max_allowed_input_size)
    return false;

  WriteUint(prefix_length, input.size(), output);
  WriteEncodedBytes(input, output);

  return true;
}

// Writes a LogEntry of type X.509 cert to |output|.
// |input| is the LogEntry containing the certificate.
// Returns true if the leaf_certificate in the LogEntry does not exceed
// kMaxAsn1CertificateLength and so can be written to |output|.
bool EncodeAsn1CertLogEntry(const LogEntry& input, std::string* output) {
  return WriteVariableBytes(kAsn1CertificateLengthBytes,
                            input.leaf_certificate, output);
}

// Writes a LogEntry of type PreCertificate to |output|.
// |input| is the LogEntry containing the TBSCertificate and issuer key hash.
// Returns true if the TBSCertificate component in the LogEntry does not
// exceed kMaxTbsCertificateLength and so can be written to |output|.
bool EncodePrecertLogEntry(const LogEntry& input, std::string* output) {
  WriteEncodedBytes(
      base::StringPiece(
          reinterpret_cast<const char*>(input.issuer_key_hash.data),
          kLogIdLength),
      output);
  return WriteVariableBytes(kTbsCertificateLengthBytes,
                            input.tbs_certificate, output);
}

}  // namespace

bool EncodeDigitallySigned(const DigitallySigned& input,
                           std::string* output) {
  WriteUint(kHashAlgorithmLength, input.hash_algorithm, output);
  WriteUint(kSigAlgorithmLength, input.signature_algorithm,
            output);
  return WriteVariableBytes(kSignatureLengthBytes, input.signature_data,
                            output);
}

bool DecodeDigitallySigned(base::StringPiece* input,
                           DigitallySigned* output) {
  unsigned hash_algo;
  unsigned sig_algo;
  base::StringPiece sig_data;

  if (!ReadUint(kHashAlgorithmLength, input, &hash_algo) ||
      !ReadUint(kSigAlgorithmLength, input, &sig_algo) ||
      !ReadVariableBytes(kSignatureLengthBytes, input, &sig_data)) {
    return false;
  }

  DigitallySigned result;
  if (!ConvertHashAlgorithm(hash_algo, &result.hash_algorithm)) {
    DVLOG(1) << "Invalid hash algorithm " << hash_algo;
    return false;
  }
  if (!ConvertSignatureAlgorithm(sig_algo, &result.signature_algorithm)) {
    DVLOG(1) << "Invalid signature algorithm " << sig_algo;
    return false;
  }
  sig_data.CopyToString(&result.signature_data);

  *output = result;
  return true;
}

bool EncodeLogEntry(const LogEntry& input, std::string* output) {
  WriteUint(kLogEntryTypeLength, input.type, output);
  switch (input.type) {
    case LogEntry::LOG_ENTRY_TYPE_X509:
      return EncodeAsn1CertLogEntry(input, output);
    case LogEntry::LOG_ENTRY_TYPE_PRECERT:
      return EncodePrecertLogEntry(input, output);
  }
  return false;
}

bool EncodeV1SCTSignedData(const base::Time& timestamp,
                           const std::string& serialized_log_entry,
                           const std::string& extensions,
                           std::string* output) {
  WriteUint(kVersionLength, SignedCertificateTimestamp::SCT_VERSION_1,
            output);
  WriteUint(kSignatureTypeLength, SIGNATURE_TYPE_CERTIFICATE_TIMESTAMP,
            output);
  base::TimeDelta time_since_epoch = timestamp - base::Time::UnixEpoch();
  WriteUint(kTimestampLength, time_since_epoch.InMilliseconds(),
            output);
  // NOTE: serialized_log_entry must already be serialized and contain the
  // length as the prefix.
  WriteEncodedBytes(serialized_log_entry, output);
  return WriteVariableBytes(kExtensionsLengthBytes, extensions, output);
}

bool DecodeSCTList(base::StringPiece* input,
                   std::vector<base::StringPiece>* output) {
  std::vector<base::StringPiece> result;
  if (!ReadList(kSCTListLengthBytes, kSerializedSCTLengthBytes,
                input, &result)) {
    return false;
  }

  if (!input->empty() || result.empty())
    return false;
  output->swap(result);
  return true;
}

bool DecodeSignedCertificateTimestamp(
    base::StringPiece* input,
    scoped_refptr<SignedCertificateTimestamp>* output) {
  scoped_refptr<SignedCertificateTimestamp> result(
      new SignedCertificateTimestamp());
  unsigned version;
  if (!ReadUint(kVersionLength, input, &version))
    return false;
  if (version != SignedCertificateTimestamp::SCT_VERSION_1) {
    DVLOG(1) << "Unsupported/invalid version " << version;
    return false;
  }

  result->version = SignedCertificateTimestamp::SCT_VERSION_1;
  uint64 timestamp;
  base::StringPiece log_id;
  base::StringPiece extensions;
  if (!ReadFixedBytes(kLogIdLength, input, &log_id) ||
      !ReadUint(kTimestampLength, input, &timestamp) ||
      !ReadVariableBytes(kExtensionsLengthBytes, input,
                         &extensions) ||
      !DecodeDigitallySigned(input, &result->signature)) {
    return false;
  }

  if (timestamp > static_cast<uint64>(kint64max)) {
    DVLOG(1) << "Timestamp value too big to cast to int64: " << timestamp;
    return false;
  }

  log_id.CopyToString(&result->log_id);
  extensions.CopyToString(&result->extensions);
  result->timestamp =
      base::Time::UnixEpoch() +
      base::TimeDelta::FromMilliseconds(static_cast<int64>(timestamp));

  output->swap(result);
  return true;
}

bool EncodeSCTListForTesting(const base::StringPiece& sct,
                             std::string* output) {
  std::string encoded_sct;
  return WriteVariableBytes(kSerializedSCTLengthBytes, sct, &encoded_sct) &&
      WriteVariableBytes(kSCTListLengthBytes, encoded_sct, output);
}

}  // namespace ct

}  // namespace net
