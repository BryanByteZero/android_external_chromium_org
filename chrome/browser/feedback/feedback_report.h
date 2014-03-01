// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEEDBACK_FEEDBACK_REPORT_H_
#define CHROME_BROWSER_FEEDBACK_FEEDBACK_REPORT_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"

namespace base {
class SequencedTaskRunner;
}

namespace content {
class BrowserContext;
}

namespace feedback {

typedef base::Callback<void(const std::string&)> QueueCallback;

// This class holds a feedback report. Once a report is created, a disk backup
// for it is created automatically. This backup needs to explicitly be
// deleted by calling DeleteReportOnDisk.
class FeedbackReport : public base::RefCounted<FeedbackReport> {
 public:
  FeedbackReport(content::BrowserContext* context,
                 const base::Time& upload_at,
                 const std::string& data);

  // Stops the disk write of the report and deletes the report file if already
  // written.
  void DeleteReportOnDisk();

  const base::Time& upload_at() const { return upload_at_; }
  const std::string& data() const { return data_; }

  // Loads the reports still on disk and queues then using the given callback.
  // This call blocks on the file reads.
  static void LoadReportsAndQueue(const base::FilePath& user_dir,
                                  QueueCallback callback);

 private:
  friend class base::RefCounted<FeedbackReport>;
  virtual ~FeedbackReport();

  // Name of the file corresponding to this report.
  base::FilePath file_;

  base::FilePath reports_path_;
  base::Time upload_at_;  // Upload this report at or after this time.
  std::string data_;

  scoped_refptr<base::SequencedTaskRunner> reports_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(FeedbackReport);
};

}  // namespace feedback

#endif  // CHROME_BROWSER_FEEDBACK_FEEDBACK_REPORT_H_
