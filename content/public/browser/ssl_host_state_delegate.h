// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_SSL_HOST_STATE_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_SSL_HOST_STATE_DELEGATE_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "content/common/content_export.h"
#include "net/cert/x509_certificate.h"

namespace content {

// The SSLHostStateDelegate encapulates the host-specific state for SSL errors.
// For example, SSLHostStateDelegate remembers whether the user has whitelisted
// a particular broken cert for use with particular host.  We separate this
// state from the SSLManager because this state is shared across many navigation
// controllers.
//
// SSLHostStateDelegate may be implemented by the embedder to provide a storage
// strategy for certificate decisions or it may be left unimplemented to use a
// default strategy of not remembering decisions at all.
class SSLHostStateDelegate {
 public:
  // Records that |cert| is not permitted to be used for |host| in the future,
  // for a specified |error| type.
  virtual void DenyCert(const std::string& host,
                        net::X509Certificate* cert,
                        net::CertStatus error) = 0;

  // Records that |cert| is permitted to be used for |host| in the future, for
  // a specified |error| type.
  virtual void AllowCert(const std::string&,
                         net::X509Certificate* cert,
                         net::CertStatus error) = 0;

  // Clear all allow/deny preferences.
  virtual void Clear() = 0;

  // Queries whether |cert| is allowed or denied for |host| and |error|. Returns
  // true in |expired_previous_decision| if a previous user decision expired
  // immediately prior to this query, otherwise false.
  virtual net::CertPolicy::Judgment QueryPolicy(
      const std::string& host,
      net::X509Certificate* cert,
      net::CertStatus error,
      bool* expired_previous_decision) = 0;

  // Records that a host has run insecure content.
  virtual void HostRanInsecureContent(const std::string& host, int pid) = 0;

  // Returns whether the specified host ran insecure content.
  virtual bool DidHostRunInsecureContent(const std::string& host,
                                         int pid) const = 0;

 protected:
  virtual ~SSLHostStateDelegate() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_SSL_HOST_STATE_DELEGATE_H_
