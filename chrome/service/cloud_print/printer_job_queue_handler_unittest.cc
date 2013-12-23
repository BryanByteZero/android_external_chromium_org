// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/cloud_print/printer_job_queue_handler.h"

#include <set>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Return;
using ::testing::AtLeast;

namespace cloud_print {

const char kJobListResponse[] =
    "{"
    " \"success\" : true, "
    " \"jobs\" : ["
    "{"
    "  \"tags\" : [ \"^own\", \"\"], "
    "  \"title\" : \"test1\","
    "  \"ticketUrl\" : \"http://example.com/job1ticket\","
    "  \"fileUrl\" : \"http://example.com/job1data\","
    "  \"id\" : \"__testjob1\""
    "},"
    "{"
    "  \"tags\" : [ \"^own\", \"\"], "
    "  \"title\" : \"test2\","
    "  \"ticketUrl\" : \"http://example.com/job2ticket\","
    "  \"fileUrl\" : \"http://example.com/job2data\","
    "  \"id\" : \"__testjob2\""
    "},"
    "{"
    "  \"tags\" : [ \"^own\", \"\"], "
    "  \"title\" : \"test3\","
    "  \"ticketUrl\" : \"http://example.com/job3ticket\","
    "  \"fileUrl\" : \"http://example.com/job3data\","
    "  \"id\" : \"__testjob3\""
    "}"
    "]"
    "}";

class TimeProviderMock : public PrinterJobQueueHandler::TimeProvider {
 public:
  MOCK_METHOD0(GetNow, base::Time());
};

class PrinterJobQueueHandlerTest : public ::testing::Test {
 protected:
  base::Value* data_;
  base::DictionaryValue* json_data_;
  virtual void SetUp() {
    base::JSONReader json_reader;
    data_ = json_reader.Read(kJobListResponse);
    data_->GetAsDictionary(&json_data_);
  }

  virtual void TearDown() {
    delete data_;
  }
};

TEST_F(PrinterJobQueueHandlerTest, BasicJobReadTest) {
  PrinterJobQueueHandler job_queue_handler;
  std::vector<JobDetails> jobs;

  job_queue_handler.GetJobsFromQueue(json_data_, &jobs);

  ASSERT_EQ((size_t)3, jobs.size());

  EXPECT_EQ(std::string("__testjob1"), jobs[0].job_id_);
  EXPECT_EQ(std::string("test1"), jobs[0].job_title_);
  EXPECT_EQ(std::string("http://example.com/job1ticket"),
            jobs[0].print_ticket_url_);
  EXPECT_EQ(std::string("http://example.com/job1data"),
            jobs[0].print_data_url_);

  std::set<std::string> expected_tags;
  expected_tags.insert("^own");
  expected_tags.insert(std::string());
  std::set<std::string> actual_tags;
  actual_tags.insert(jobs[0].tags_.begin(), jobs[0].tags_.end());

  EXPECT_EQ(expected_tags, actual_tags);
  EXPECT_EQ(base::TimeDelta(), jobs[0].time_remaining_);
}

TEST_F(PrinterJobQueueHandlerTest, PreferNonFailureTest) {
  TimeProviderMock* time_mock = new TimeProviderMock();
  PrinterJobQueueHandler job_queue_handler(time_mock);
  EXPECT_CALL((*time_mock), GetNow())
      .Times(AtLeast(2))
      .WillRepeatedly(Return(base::Time::UnixEpoch()));

  // must fail twice for backoff to kick in
  job_queue_handler.JobFetchFailed("__testjob1");
  job_queue_handler.JobFetchFailed("__testjob1");

  std::vector<JobDetails> jobs;
  job_queue_handler.GetJobsFromQueue(json_data_, &jobs);

  EXPECT_EQ(std::string("__testjob2"), jobs[0].job_id_);
  EXPECT_EQ(base::TimeDelta(), jobs[0].time_remaining_);
}

TEST_F(PrinterJobQueueHandlerTest, PreferNoTimeTest) {
  TimeProviderMock* time_mock = new TimeProviderMock();
  PrinterJobQueueHandler job_queue_handler(time_mock);
  EXPECT_CALL((*time_mock), GetNow()).
      Times(AtLeast(8));

  ON_CALL((*time_mock), GetNow())
      .WillByDefault(Return(base::Time::UnixEpoch()));

  for (int i = 0; i < 4; i++)
    job_queue_handler.JobFetchFailed("__testjob1");

  ON_CALL((*time_mock), GetNow())
      .WillByDefault(Return(base::Time::UnixEpoch() +
                            base::TimeDelta::FromMinutes(4)));

  for (int i = 0; i < 2; i++)
    job_queue_handler.JobFetchFailed("__testjob2");

  for (int i = 0; i < 2; i++)
    job_queue_handler.JobFetchFailed("__testjob3");

  std::vector<JobDetails> jobs;
  job_queue_handler.GetJobsFromQueue(json_data_, &jobs);

  EXPECT_EQ(base::TimeDelta(), jobs[0].time_remaining_);
  EXPECT_EQ(std::string("__testjob1"), jobs[0].job_id_);
}

TEST_F(PrinterJobQueueHandlerTest, PreferLowerTimeTest) {
  TimeProviderMock* time_mock = new TimeProviderMock();
  PrinterJobQueueHandler job_queue_handler(time_mock);

  EXPECT_CALL((*time_mock), GetNow()).
      Times(AtLeast(8));

  ON_CALL((*time_mock), GetNow())
      .WillByDefault(Return(base::Time::UnixEpoch()));

  for (int i = 0; i < 4; i++)
    job_queue_handler.JobFetchFailed("__testjob1");

  ON_CALL((*time_mock), GetNow())
      .WillByDefault(Return(base::Time::UnixEpoch() +
                            base::TimeDelta::FromSeconds(4)));

  for (int i = 0; i < 2; i++)
    job_queue_handler.JobFetchFailed("__testjob2");

  for (int i = 0; i < 2; i++)
    job_queue_handler.JobFetchFailed("__testjob3");

  std::vector<JobDetails> jobs;
  job_queue_handler.GetJobsFromQueue(json_data_,
                                                   &jobs);

  base::TimeDelta time_to_wait = jobs[0].time_remaining_;
  EXPECT_NE(base::TimeDelta(), time_to_wait);

  jobs.clear();
  ON_CALL((*time_mock), GetNow())
      .WillByDefault(Return(base::Time::UnixEpoch() +
                            base::TimeDelta::FromSeconds(4) + time_to_wait));

  job_queue_handler.GetJobsFromQueue(json_data_,
                                     &jobs);

  EXPECT_EQ(base::TimeDelta(), jobs[0].time_remaining_);
  EXPECT_EQ(std::string("__testjob2"),  jobs[0].job_id_);
}

}  // namespace cloud_print
