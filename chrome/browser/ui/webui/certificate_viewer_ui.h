// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CERTIFICATE_VIEWER_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CERTIFICATE_VIEWER_UI_H_
#pragma once

#include "chrome/browser/ui/webui/constrained_html_ui.h"

// The WebUI for chrome://view-cert
class CertificateViewerUI : public ConstrainedHtmlUI {
 public:
  explicit CertificateViewerUI(content::WebUI* web_ui);
  virtual ~CertificateViewerUI();

  DISALLOW_COPY_AND_ASSIGN(CertificateViewerUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_CERTIFICATE_VIEWER_UI_H_
