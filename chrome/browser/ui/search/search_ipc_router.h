// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_SEARCH_SEARCH_IPC_ROUTER_H_
#define CHROME_BROWSER_UI_SEARCH_SEARCH_IPC_ROUTER_H_

#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/common/instant_types.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}

class SearchIPCRouterTest;

// SearchIPCRouter is responsible for receiving and sending IPC messages between
// the browser and the Instant page.
class SearchIPCRouter : public content::WebContentsObserver {
 public:
  // SearchIPCRouter calls its delegate in response to messages received from
  // the page.
  class Delegate {
   public:
    // Called upon determination of Instant API support in response to the page
    // load event.
    virtual void OnInstantSupportDetermined(bool supports_instant) = 0;

    // Called upon determination of voice search API support.
    virtual void OnSetVoiceSearchSupport(bool supports_voice_search) = 0;
  };

  // An interface to be implemented by consumers of SearchIPCRouter objects to
  // decide whether to process the message received from the page, and vice
  // versa (decide whether to send messages to the page).
  class Policy {
   public:
    virtual ~Policy() {}

    // SearchIPCRouter calls these functions before sending/receiving messages
    // to/from the page.
    virtual bool ShouldProcessSetVoiceSearchSupport() = 0;
    virtual bool ShouldSendSetPromoInformation() = 0;
    virtual bool ShouldSendSetDisplayInstantResults() = 0;
    virtual bool ShouldSendSetSuggestionToPrefetch() = 0;
    virtual bool ShouldSendMostVisitedItems() = 0;
    virtual bool ShouldSendThemeBackgroundInfo() = 0;
    virtual bool ShouldSubmitQuery() = 0;
  };

  SearchIPCRouter(content::WebContents* web_contents, Delegate* delegate,
                  scoped_ptr<Policy> policy);
  virtual ~SearchIPCRouter();

  // Tells the renderer to determine if the page supports the Instant API, which
  // results in a call to OnInstantSupportDetermined() when the reply is
  // received.
  void DetermineIfPageSupportsInstant();

  // Tells the renderer information it needs to display promos.
  void SetPromoInformation(bool is_app_launcher_enabled);

  // Tells the renderer whether to display the Instant results.
  void SetDisplayInstantResults();

  // Tells the renderer about the most visited items.
  void SendMostVisitedItems(const std::vector<InstantMostVisitedItem>& items);

  // Tells the renderer about the current theme background.
  void SendThemeBackgroundInfo(const ThemeBackgroundInfo& theme_info);

  // Tells the page the suggestion to be prefetched if any.
  void SetSuggestionToPrefetch(const InstantSuggestion& suggestion);

  // Tells the page that the user pressed Enter in the omnibox.
  void Submit(const string16& text);

 private:
  friend class SearchIPCRouterTest;
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest,
                           DetermineIfPageSupportsInstant_Local);
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest,
                           DetermineIfPageSupportsInstant_NonLocal);
  FRIEND_TEST_ALL_PREFIXES(SearchTabHelperTest,
                           PageURLDoesntBelongToInstantRenderer);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest,
                           ProcessVoiceSearchSupportMsg);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest,
                           SendSetDisplayInstantResults);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest,
                           SendSetSuggestionToPrefetch);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest,
                           DoNotSendSetMessagesForIncognitoPage);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest, SendMostVisitedItems);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest,
                           DoNotSendMostVisitedItems);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest, SendThemeBackgroundInfo);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest,
                           DoNotSendThemeBackgroundInfo);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest, SubmitQuery);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest,
                           AppropriateMessagesSentToIncognitoPages);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest, SendSetPromoInformation);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterPolicyTest,
                           DoNotSendSetPromoInformation);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterTest, ProcessVoiceSearchSupportMsg);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterTest, IgnoreVoiceSearchSupportMsg);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterTest, SendSetPromoInformationMsg);
  FRIEND_TEST_ALL_PREFIXES(SearchIPCRouterTest,
                           DoNotSendSetPromoInformationMsg);

  // Overridden from contents::WebContentsObserver:
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // Handler for when Instant support has been determined.
  void OnInstantSupportDetermined(int page_id, bool supports_instant) const;

  // Handler for when voice search support has been determined.
  void OnVoiceSearchSupportDetermined(int page_id,
                                      bool supports_voice_search) const;

  // Used by unit tests to set a fake delegate.
  void set_delegate(Delegate* delegate);

  // Used by unit tests.
  void set_policy(scoped_ptr<Policy> policy);

  // Used by unit tests.
  Policy* policy() const { return policy_.get(); }

  Delegate* delegate_;
  scoped_ptr<Policy> policy_;

  DISALLOW_COPY_AND_ASSIGN(SearchIPCRouter);
};

#endif  // CHROME_BROWSER_UI_SEARCH_SEARCH_IPC_ROUTER_H_
