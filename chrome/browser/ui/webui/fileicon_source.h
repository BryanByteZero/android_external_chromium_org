// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_FILEICON_SOURCE_H_
#define CHROME_BROWSER_UI_WEBUI_FILEICON_SOURCE_H_

#include <string>

#include "base/files/file_path.h"
#include "chrome/browser/icon_manager.h"
#include "chrome/common/cancelable_task_tracker.h"
#include "content/public/browser/url_data_source.h"
#include "ui/base/layout.h"

namespace gfx {
class Image;
}

// FileIconSource is the gateway between network-level chrome:
// requests for favicons and the history backend that serves these.
class FileIconSource : public content::URLDataSource {
 public:
  explicit FileIconSource();

  // content::URLDataSource implementation.
  virtual std::string GetSource() const OVERRIDE;
  virtual void StartDataRequest(
      const std::string& path,
      int render_process_id,
      int render_frame_id,
      const content::URLDataSource::GotDataCallback& callback) OVERRIDE;
  virtual std::string GetMimeType(const std::string&) const OVERRIDE;

 protected:
  virtual ~FileIconSource();

  // Once the |path| and |icon_size| has been determined from the request, this
  // function is called to perform the actual fetch. Declared as virtual for
  // testing.
  virtual void FetchFileIcon(
      const base::FilePath& path,
      ui::ScaleFactor scale_factor,
      IconLoader::IconSize icon_size,
      const content::URLDataSource::GotDataCallback& callback);

 private:
  // Contains the necessary information for completing an icon fetch request.
  struct IconRequestDetails {
    IconRequestDetails();
    ~IconRequestDetails();

    // The callback to run with the response.
    content::URLDataSource::GotDataCallback callback;

    // The requested scale factor to respond with.
    ui::ScaleFactor scale_factor;
  };

  // Called when favicon data is available from the history backend.
  void OnFileIconDataAvailable(const IconRequestDetails& details,
                               gfx::Image* icon);

  // Tracks tasks requesting file icons.
  CancelableTaskTracker cancelable_task_tracker_;

  DISALLOW_COPY_AND_ASSIGN(FileIconSource);
};
#endif  // CHROME_BROWSER_UI_WEBUI_FILEICON_SOURCE_H_
