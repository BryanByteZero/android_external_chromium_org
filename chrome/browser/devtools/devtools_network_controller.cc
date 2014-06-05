// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/devtools_network_controller.h"

#include "chrome/browser/devtools/devtools_network_conditions.h"
#include "chrome/browser/devtools/devtools_network_transaction.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"

using content::BrowserThread;

namespace {

const char kDevToolsRequestInitiator[] = "X-DevTools-Request-Initiator";

}  // namespace

DevToolsNetworkController::DevToolsNetworkController()
    : weak_ptr_factory_(this) {
}

DevToolsNetworkController::~DevToolsNetworkController() {
}

void DevToolsNetworkController::AddTransaction(
    DevToolsNetworkTransaction* transaction) {
  DCHECK(thread_checker_.CalledOnValidThread());
  transactions_.insert(transaction);
}

void DevToolsNetworkController::RemoveTransaction(
    DevToolsNetworkTransaction* transaction) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(transactions_.find(transaction) != transactions_.end());
  transactions_.erase(transaction);
}

void DevToolsNetworkController::SetNetworkState(
    const std::string& client_id,
    const scoped_refptr<DevToolsNetworkConditions> conditions) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(
          &DevToolsNetworkController::SetNetworkStateOnIO,
          weak_ptr_factory_.GetWeakPtr(),
          client_id,
          conditions));
}

void DevToolsNetworkController::SetNetworkStateOnIO(
    const std::string& client_id,
    const scoped_refptr<DevToolsNetworkConditions> conditions) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!conditions) {
    if (client_id == active_client_id_) {
      conditions_ = NULL;
      active_client_id_ = std::string();
    }
    return;
  }
  conditions_ = conditions;
  active_client_id_ = client_id;

  // Iterate over a copy of set, because failing of transaction could result in
  // creating a new one, or (theoretically) destroying one.
  Transactions old_transactions(transactions_);
  for (Transactions::iterator it = old_transactions.begin();
       it != old_transactions.end(); ++it) {
    if (transactions_.find(*it) == transactions_.end())
      continue;
    if (!(*it)->request() || (*it)->failed())
      continue;
    if (ShouldFail((*it)->request()))
      (*it)->Fail();
  }
}

bool DevToolsNetworkController::ShouldFail(
    const net::HttpRequestInfo* request) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(request);
  if (!conditions_ || !conditions_->IsOffline())
    return false;

  if (!conditions_->HasMatchingDomain(request->url))
    return false;

  return !request->extra_headers.HasHeader(kDevToolsRequestInitiator);
}
