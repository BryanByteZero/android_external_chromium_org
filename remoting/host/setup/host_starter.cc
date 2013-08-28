// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/setup/host_starter.h"

#include "base/guid.h"
#include "base/thread_task_runner_handle.h"
#include "base/values.h"
#include "google_apis/google_api_keys.h"
#include "remoting/host/pin_hash.h"
#include "remoting/host/setup/oauth_helper.h"

namespace {
const int kMaxGetTokensRetries = 3;
}  // namespace

namespace remoting {

HostStarter::HostStarter(
    scoped_ptr<gaia::GaiaOAuthClient> oauth_client,
    scoped_ptr<remoting::ServiceClient> service_client,
    scoped_ptr<remoting::DaemonController> daemon_controller)
    : oauth_client_(oauth_client.Pass()),
      service_client_(service_client.Pass()),
      daemon_controller_(daemon_controller.Pass()),
      consent_to_data_collection_(false),
      unregistering_host_(false),
      weak_ptr_factory_(this),
      weak_ptr_(weak_ptr_factory_.GetWeakPtr()) {
  main_task_runner_ = base::ThreadTaskRunnerHandle::Get();
}

HostStarter::~HostStarter() {
}

scoped_ptr<HostStarter> HostStarter::Create(
    const std::string& chromoting_hosts_url,
    net::URLRequestContextGetter* url_request_context_getter) {
  scoped_ptr<gaia::GaiaOAuthClient> oauth_client(
      new gaia::GaiaOAuthClient(url_request_context_getter));
  scoped_ptr<remoting::ServiceClient> service_client(
      new remoting::ServiceClient(
          chromoting_hosts_url, url_request_context_getter));
  scoped_ptr<remoting::DaemonController> daemon_controller(
      remoting::DaemonController::Create());
  return scoped_ptr<HostStarter>(
      new HostStarter(oauth_client.Pass(), service_client.Pass(),
                      daemon_controller.Pass()));
}

void HostStarter::StartHost(
    const std::string& host_name,
    const std::string& host_pin,
    bool consent_to_data_collection,
    const std::string& auth_code,
    const std::string& redirect_url,
    CompletionCallback on_done) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(on_done_.is_null());

  host_name_ = host_name;
  host_pin_ = host_pin;
  consent_to_data_collection_ = consent_to_data_collection;
  on_done_ = on_done;
  oauth_client_info_.client_id =
      google_apis::GetOAuth2ClientID(google_apis::CLIENT_REMOTING);
  oauth_client_info_.client_secret =
      google_apis::GetOAuth2ClientSecret(google_apis::CLIENT_REMOTING);
  oauth_client_info_.redirect_uri = redirect_url;
  // Map the authorization code to refresh and access tokens.
  oauth_client_->GetTokensFromAuthCode(oauth_client_info_, auth_code,
                                       kMaxGetTokensRetries, this);
}

void HostStarter::OnGetTokensResponse(
    const std::string& refresh_token,
    const std::string& access_token,
    int expires_in_seconds) {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(FROM_HERE, base::Bind(
        &HostStarter::OnGetTokensResponse, weak_ptr_,
        refresh_token, access_token, expires_in_seconds));
    return;
  }
  refresh_token_ = refresh_token;
  access_token_ = access_token;
  // Get the email corresponding to the access token.
  oauth_client_->GetUserEmail(access_token_, 1, this);
}

void HostStarter::OnRefreshTokenResponse(
    const std::string& access_token,
    int expires_in_seconds) {
  // We never request a refresh token, so this call is not expected.
  NOTREACHED();
}

// This function is called twice: once with the host owner credentials, and once
// with the service account credentials.
void HostStarter::OnGetUserEmailResponse(const std::string& user_email) {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(FROM_HERE, base::Bind(
        &HostStarter::OnGetUserEmailResponse, weak_ptr_, user_email));
    return;
  }

  if (host_owner_.empty()) {
    // This is the first callback, with the host owner credentials. Store the
    // owner's email, and register the host.
    host_owner_ = user_email;
    host_id_ = base::GenerateGUID();
    key_pair_ = RsaKeyPair::Generate();

    std::string host_client_id;
    host_client_id = google_apis::GetOAuth2ClientID(
        google_apis::CLIENT_REMOTING_HOST);

    service_client_->RegisterHost(
        host_id_, host_name_, key_pair_->GetPublicKey(), host_client_id,
        access_token_, this);
  } else {
    // This is the second callback, with the service account credentials.
    // This email is the service account's email, used to login to XMPP.
    xmpp_login_ = user_email;
    StartHostProcess();
  }
}

void HostStarter::OnHostRegistered(const std::string& authorization_code) {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(FROM_HERE, base::Bind(
        &HostStarter::OnHostRegistered, weak_ptr_, authorization_code));
    return;
  }

  if (authorization_code.empty()) {
    // No service account code, start the host with the owner's credentials.
    xmpp_login_ = host_owner_;
    StartHostProcess();
    return;
  }

  // Received a service account authorization code, update oauth_client_info_
  // to use the service account client keys, and get service account tokens.
  oauth_client_info_.client_id =
      google_apis::GetOAuth2ClientID(
          google_apis::CLIENT_REMOTING_HOST);
  oauth_client_info_.client_secret =
      google_apis::GetOAuth2ClientSecret(
          google_apis::CLIENT_REMOTING_HOST);
  oauth_client_info_.redirect_uri = "oob";
  oauth_client_->GetTokensFromAuthCode(
      oauth_client_info_, authorization_code, kMaxGetTokensRetries, this);
}

void HostStarter::StartHostProcess() {
  // Start the host.
  std::string host_secret_hash = remoting::MakeHostPinHash(host_id_, host_pin_);
  scoped_ptr<base::DictionaryValue> config(new base::DictionaryValue());
  if (host_owner_ != xmpp_login_) {
    config->SetString("host_owner", host_owner_);
  }
  config->SetString("xmpp_login", xmpp_login_);
  config->SetString("oauth_refresh_token", refresh_token_);
  config->SetString("host_id", host_id_);
  config->SetString("host_name", host_name_);
  config->SetString("private_key", key_pair_->ToString());
  config->SetString("host_secret_hash", host_secret_hash);
  daemon_controller_->SetConfigAndStart(
      config.Pass(), consent_to_data_collection_,
      base::Bind(&HostStarter::OnHostStarted, base::Unretained(this)));
}

void HostStarter::OnHostStarted(DaemonController::AsyncResult result) {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(FROM_HERE, base::Bind(
        &HostStarter::OnHostStarted, weak_ptr_, result));
    return;
  }
  if (result != DaemonController::RESULT_OK) {
    unregistering_host_ = true;
    service_client_->UnregisterHost(host_id_, access_token_, this);
    return;
  }
  CompletionCallback cb = on_done_;
  on_done_.Reset();
  cb.Run(START_COMPLETE);
}

void HostStarter::OnOAuthError() {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(FROM_HERE, base::Bind(
        &HostStarter::OnOAuthError, weak_ptr_));
    return;
  }
  CompletionCallback cb = on_done_;
  on_done_.Reset();
  if (unregistering_host_) {
    LOG(ERROR) << "OAuth error occurred when unregistering host.";
    cb.Run(START_ERROR);
  } else {
    cb.Run(OAUTH_ERROR);
  }
}

void HostStarter::OnNetworkError(int response_code) {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(FROM_HERE, base::Bind(
        &HostStarter::OnNetworkError, weak_ptr_, response_code));
    return;
  }
  CompletionCallback cb = on_done_;
  on_done_.Reset();
  if (unregistering_host_) {
    LOG(ERROR) << "Network error occurred when unregistering host.";
    cb.Run(START_ERROR);
  } else {
    cb.Run(NETWORK_ERROR);
  }
}

void HostStarter::OnHostUnregistered() {
  if (!main_task_runner_->BelongsToCurrentThread()) {
    main_task_runner_->PostTask(FROM_HERE, base::Bind(
        &HostStarter::OnHostUnregistered, weak_ptr_));
    return;
  }
  CompletionCallback cb = on_done_;
  on_done_.Reset();
  cb.Run(START_ERROR);
}

}  // namespace remoting
