// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/android/net_jni_registrar.h"

#include "base/basictypes.h"
#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "net/android/android_private_key.h"
#include "net/android/gurl_utils.h"
#include "net/android/keystore.h"
#include "net/android/network_change_notifier_android.h"
#include "net/android/network_library.h"
#include "net/cert/x509_util_android.h"
#include "net/proxy/proxy_config_service_android.h"

namespace net {
namespace android {

static base::android::RegistrationMethod kNetRegisteredMethods[] = {
  { "AndroidCertVerifyResult", net::android::RegisterCertVerifyResult },
  { "AndroidPrivateKey", net::android::RegisterAndroidPrivateKey},
  { "AndroidKeyStore", net::android::RegisterKeyStore },
  { "AndroidNetworkLibrary", net::android::RegisterNetworkLibrary },
  { "GURLUtils", net::RegisterGURLUtils },
  { "NetworkChangeNotifierAndroid",
    net::NetworkChangeNotifierAndroid::Register },
  { "ProxyConfigService", net::ProxyConfigServiceAndroid::Register },
  { "X509Util", net::RegisterX509Util },
};

bool RegisterJni(JNIEnv* env) {
  return base::android::RegisterNativeMethods(
      env, kNetRegisteredMethods, arraysize(kNetRegisteredMethods));
}

}  // namespace android
}  // namespace net
