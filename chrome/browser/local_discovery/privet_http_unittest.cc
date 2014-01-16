// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/local_discovery/privet_http_impl.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::StrictMock;
using testing::NiceMock;

namespace local_discovery {

namespace {

const char kSampleInfoResponse[] = "{"
    "       \"version\": \"1.0\","
    "       \"name\": \"Common printer\","
    "       \"description\": \"Printer connected through Chrome connector\","
    "       \"url\": \"https://www.google.com/cloudprint\","
    "       \"type\": ["
    "               \"printer\""
    "       ],"
    "       \"id\": \"\","
    "       \"device_state\": \"idle\","
    "       \"connection_state\": \"online\","
    "       \"manufacturer\": \"Google\","
    "       \"model\": \"Google Chrome\","
    "       \"serial_number\": \"1111-22222-33333-4444\","
    "       \"firmware\": \"24.0.1312.52\","
    "       \"uptime\": 600,"
    "       \"setup_url\": \"http://support.google.com/\","
    "       \"support_url\": \"http://support.google.com/cloudprint/?hl=en\","
    "       \"update_url\": \"http://support.google.com/cloudprint/?hl=en\","
    "       \"x-privet-token\": \"SampleTokenForTesting\","
    "       \"api\": ["
    "               \"/privet/accesstoken\","
    "               \"/privet/capabilities\","
    "               \"/privet/printer/submitdoc\","
    "       ]"
    "}";

const char kSampleInfoResponseRegistered[] = "{"
    "       \"version\": \"1.0\","
    "       \"name\": \"Common printer\","
    "       \"description\": \"Printer connected through Chrome connector\","
    "       \"url\": \"https://www.google.com/cloudprint\","
    "       \"type\": ["
    "               \"printer\""
    "       ],"
    "       \"id\": \"MyDeviceID\","
    "       \"device_state\": \"idle\","
    "       \"connection_state\": \"online\","
    "       \"manufacturer\": \"Google\","
    "       \"model\": \"Google Chrome\","
    "       \"serial_number\": \"1111-22222-33333-4444\","
    "       \"firmware\": \"24.0.1312.52\","
    "       \"uptime\": 600,"
    "       \"setup_url\": \"http://support.google.com/\","
    "       \"support_url\": \"http://support.google.com/cloudprint/?hl=en\","
    "       \"update_url\": \"http://support.google.com/cloudprint/?hl=en\","
    "       \"x-privet-token\": \"SampleTokenForTesting\","
    "       \"api\": ["
    "               \"/privet/accesstoken\","
    "               \"/privet/capabilities\","
    "               \"/privet/printer/submitdoc\","
    "       ]"
    "}";

const char kSampleInfoResponseWithCreatejob[] = "{"
    "       \"version\": \"1.0\","
    "       \"name\": \"Common printer\","
    "       \"description\": \"Printer connected through Chrome connector\","
    "       \"url\": \"https://www.google.com/cloudprint\","
    "       \"type\": ["
    "               \"printer\""
    "       ],"
    "       \"id\": \"\","
    "       \"device_state\": \"idle\","
    "       \"connection_state\": \"online\","
    "       \"manufacturer\": \"Google\","
    "       \"model\": \"Google Chrome\","
    "       \"serial_number\": \"1111-22222-33333-4444\","
    "       \"firmware\": \"24.0.1312.52\","
    "       \"uptime\": 600,"
    "       \"setup_url\": \"http://support.google.com/\","
    "       \"support_url\": \"http://support.google.com/cloudprint/?hl=en\","
    "       \"update_url\": \"http://support.google.com/cloudprint/?hl=en\","
    "       \"x-privet-token\": \"SampleTokenForTesting\","
    "       \"api\": ["
    "               \"/privet/accesstoken\","
    "               \"/privet/capabilities\","
    "               \"/privet/printer/createjob\","
    "               \"/privet/printer/submitdoc\","
    "       ]"
    "}";

const char kSampleRegisterStartResponse[] = "{"
    "\"user\": \"example@google.com\","
    "\"action\": \"start\""
    "}";

const char kSampleRegisterGetClaimTokenResponse[] = "{"
    "       \"action\": \"getClaimToken\","
    "       \"user\": \"example@google.com\","
    "       \"token\": \"MySampleToken\","
    "       \"claim_url\": \"https://domain.com/SoMeUrL\""
    "}";

const char kSampleRegisterCompleteResponse[] = "{"
    "\"user\": \"example@google.com\","
    "\"action\": \"complete\","
    "\"device_id\": \"MyDeviceID\""
    "}";

const char kSampleXPrivetErrorResponse[] =
    "{ \"error\": \"invalid_x_privet_token\" }";

const char kSampleRegisterErrorTransient[] =
    "{ \"error\": \"device_busy\", \"timeout\": 1}";

const char kSampleRegisterErrorPermanent[] =
    "{ \"error\": \"user_cancel\" }";

const char kSampleInfoResponseBadJson[] = "{";

const char kSampleRegisterCancelResponse[] = "{"
    "\"user\": \"example@google.com\","
    "\"action\": \"cancel\""
    "}";

const char kSampleLocalPrintResponse[] = "{"
    "\"job_id\": \"123\","
    "\"expires_in\": 500,"
    "\"job_type\": \"application/pdf\","
    "\"job_size\": 16,"
    "\"job_name\": \"Sample job name\","
    "}";

const char kSampleCapabilitiesResponse[] = "{"
    "\"version\" : \"1.0\","
    "\"printer\" : {"
    "  \"supported_content_type\" : ["
    "   { \"content_type\" : \"application/pdf\" },"
    "   { \"content_type\" : \"image/pwg-raster\" }"
    "  ]"
    "}"
    "}";

const char kSampleCapabilitiesResponsePWGOnly[] = "{"
    "\"version\" : \"1.0\","
    "\"printer\" : {"
    "  \"supported_content_type\" : ["
    "   { \"content_type\" : \"image/pwg-raster\" }"
    "  ]"
    "}"
    "}";

const char kSampleCapabilitiesResponseWithAnyMimetype[] = "{"
    "\"version\" : \"1.0\","
    "\"printer\" : {"
    "  \"supported_content_type\" : ["
    "   { \"content_type\" : \"*/*\" },"
    "   { \"content_type\" : \"image/pwg-raster\" }"
    "  ]"
    "}"
    "}";

const char kSampleErrorResponsePrinterBusy[] = "{"
    "\"error\": \"invalid_print_job\","
    "\"timeout\": 1 "
    "}";

const char kSampleInvalidDocumentTypeResponse[] = "{"
    "\"error\" : \"invalid_document_type\""
    "}";

const char kSampleCreatejobResponse[] = "{ \"job_id\": \"1234\" }";

const char kSampleEmptyJSONResponse[] = "{}";

class MockTestURLFetcherFactoryDelegate
    : public net::TestURLFetcher::DelegateForTests {
 public:
  // Callback issued correspondingly to the call to the |Start()| method.
  MOCK_METHOD1(OnRequestStart, void(int fetcher_id));

  // Callback issued correspondingly to the call to |AppendChunkToUpload|.
  // Uploaded chunks can be retrieved with the |upload_chunks()| getter.
  MOCK_METHOD1(OnChunkUpload, void(int fetcher_id));

  // Callback issued correspondingly to the destructor.
  MOCK_METHOD1(OnRequestEnd, void(int fetcher_id));
};

class PrivetHTTPTest : public ::testing::Test {
 public:
  PrivetHTTPTest() {
    request_context_= new net::TestURLRequestContextGetter(
        base::MessageLoopProxy::current());
    privet_client_.reset(new PrivetHTTPClientImpl(
        "sampleDevice._privet._tcp.local",
        net::HostPortPair("10.0.0.8", 6006),
        request_context_.get()));
    fetcher_factory_.SetDelegateForTests(&fetcher_delegate_);
  }

  virtual ~PrivetHTTPTest() {
  }

  bool SuccessfulResponseToURL(const GURL& url,
                               const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    EXPECT_TRUE(fetcher);
    EXPECT_EQ(url, fetcher->GetOriginalURL());

    if (!fetcher || url != fetcher->GetOriginalURL())
      return false;

    fetcher->SetResponseString(response);
    fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                              net::OK));
    fetcher->set_response_code(200);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    return true;
  }

  bool SuccessfulResponseToURLAndData(const GURL& url,
                                      const std::string& data,
                                      const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    EXPECT_TRUE(fetcher);
    EXPECT_EQ(url, fetcher->GetOriginalURL());

    if (!fetcher) return false;

    EXPECT_EQ(data, fetcher->upload_data());
    if (data != fetcher->upload_data()) return false;

    return SuccessfulResponseToURL(url, response);
  }

  bool SuccessfulResponseToURLAndFilePath(const GURL& url,
                                          const base::FilePath& file_path,
                                          const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    EXPECT_TRUE(fetcher);
    EXPECT_EQ(url, fetcher->GetOriginalURL());

    if (!fetcher) return false;

    EXPECT_EQ(file_path, fetcher->upload_file_path());
    if (file_path != fetcher->upload_file_path()) return false;

    return SuccessfulResponseToURL(url, response);
  }


  void RunFor(base::TimeDelta time_period) {
    base::CancelableCallback<void()> callback(base::Bind(
        &PrivetHTTPTest::Stop, base::Unretained(this)));
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE, callback.callback(), time_period);

    base::MessageLoop::current()->Run();
    callback.Cancel();
  }

  void Stop() {
    base::MessageLoop::current()->Quit();
  }

 protected:
  base::MessageLoop loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  net::TestURLFetcherFactory fetcher_factory_;
  scoped_ptr<PrivetHTTPClient> privet_client_;
  NiceMock<MockTestURLFetcherFactoryDelegate> fetcher_delegate_;
};

class MockJSONCallback{
 public:
  MockJSONCallback() {}
  ~MockJSONCallback() {}

  void OnPrivetJSONDone(const base::DictionaryValue* value) {
    if (!value) {
      value_.reset();
    } else {
      value_.reset(value->DeepCopy());
    }

    OnPrivetJSONDoneInternal();
  }

  MOCK_METHOD0(OnPrivetJSONDoneInternal, void());

  const base::DictionaryValue* value() { return value_.get(); }
  PrivetJSONOperation::ResultCallback callback() {
    return base::Bind(&MockJSONCallback::OnPrivetJSONDone,
                      base::Unretained(this));
  }
 protected:
  scoped_ptr<base::DictionaryValue> value_;
};

class MockRegisterDelegate : public PrivetRegisterOperation::Delegate {
 public:
  MockRegisterDelegate() {
  }
  ~MockRegisterDelegate() {
  }

  virtual void OnPrivetRegisterClaimToken(
      PrivetRegisterOperation* operation,
      const std::string& token,
      const GURL& url) OVERRIDE {
    OnPrivetRegisterClaimTokenInternal(token, url);
  }

  MOCK_METHOD2(OnPrivetRegisterClaimTokenInternal, void(
      const std::string& token,
      const GURL& url));

  virtual void OnPrivetRegisterError(
      PrivetRegisterOperation* operation,
      const std::string& action,
      PrivetRegisterOperation::FailureReason reason,
      int printer_http_code,
      const base::DictionaryValue* json) OVERRIDE {
    // TODO(noamsml): Save and test for JSON?
    OnPrivetRegisterErrorInternal(action, reason, printer_http_code);
  }

  MOCK_METHOD3(OnPrivetRegisterErrorInternal,
               void(const std::string& action,
                    PrivetRegisterOperation::FailureReason reason,
                    int printer_http_code));

  virtual void OnPrivetRegisterDone(
      PrivetRegisterOperation* operation,
      const std::string& device_id) OVERRIDE {
    OnPrivetRegisterDoneInternal(device_id);
  }

  MOCK_METHOD1(OnPrivetRegisterDoneInternal,
               void(const std::string& device_id));
};

class MockLocalPrintDelegate : public PrivetLocalPrintOperation::Delegate {
 public:
  MockLocalPrintDelegate() {}
  ~MockLocalPrintDelegate() {}

  virtual void OnPrivetPrintingDone(
      const PrivetLocalPrintOperation* print_operation) {
    OnPrivetPrintingDoneInternal();
  }

  MOCK_METHOD0(OnPrivetPrintingDoneInternal, void());

  virtual void OnPrivetPrintingError(
      const PrivetLocalPrintOperation* print_operation, int http_code) {
    OnPrivetPrintingErrorInternal(http_code);
  }

  MOCK_METHOD1(OnPrivetPrintingErrorInternal, void(int http_code));
};

// A note on PWG raster conversion: The PWG raster converter used simply
// converts strings to file paths based on them by appending "test.pdf", since
// it's easier to test that way. Instead of using a mock, we simply check if the
// request is uploading a file that is based on this pattern.
class FakePWGRasterConverter : public PWGRasterConverter {
 public:
  FakePWGRasterConverter() {
  }

  virtual ~FakePWGRasterConverter() {
  }

  virtual void Start(base::RefCountedMemory* data,
                     const printing::PdfRenderSettings& conversion_settings,
                     const ResultCallback& callback) OVERRIDE {
    std::string data_str((const char*)data->front(), data->size());
    callback.Run(true, base::FilePath().AppendASCII(data_str + "test.pdf"));
  }
};

TEST_F(PrivetHTTPTest, CreatePrivetStorageList) {
  MockJSONCallback mock_callback;
  scoped_ptr<PrivetJSONOperation> storage_list_operation =
      privet_client_->CreateStorageListOperation(
          "/path/to/nothing",
          mock_callback.callback());
  storage_list_operation->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(GURL("http://10.0.0.8:6006/privet/info"),
                                      kSampleInfoResponse));

  EXPECT_CALL(mock_callback, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/storage/list?path=/path/to/nothing"),
      kSampleEmptyJSONResponse));
}

class PrivetInfoTest : public PrivetHTTPTest {
 public:
  PrivetInfoTest() {}

  virtual ~PrivetInfoTest() {}

  virtual void SetUp() OVERRIDE {
    info_operation_ = privet_client_->CreateInfoOperation(
        info_callback_.callback());
  }

 protected:
  scoped_ptr<PrivetJSONOperation> info_operation_;
  StrictMock<MockJSONCallback> info_callback_;
};

TEST_F(PrivetInfoTest, SuccessfulInfo) {
  info_operation_->Start();

  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher != NULL);
  EXPECT_EQ(GURL("http://10.0.0.8:6006/privet/info"),
            fetcher->GetOriginalURL());

  fetcher->SetResponseString(kSampleInfoResponse);
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                            net::OK));
  fetcher->set_response_code(200);

  EXPECT_CALL(info_callback_, OnPrivetJSONDoneInternal());
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  std::string name;

  privet_client_->GetCachedInfo()->GetString("name", &name);
  EXPECT_EQ("Common printer", name);
};

TEST_F(PrivetInfoTest, InfoSaveToken) {
  info_operation_->Start();

  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher != NULL);
  fetcher->SetResponseString(kSampleInfoResponse);
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                            net::OK));
  fetcher->set_response_code(200);

  EXPECT_CALL(info_callback_, OnPrivetJSONDoneInternal());
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  info_operation_ =
      privet_client_->CreateInfoOperation(info_callback_.callback());
  info_operation_->Start();

  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher != NULL);
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);
  std::string header_token;
  ASSERT_TRUE(headers.GetHeader("X-Privet-Token", &header_token));
  EXPECT_EQ("SampleTokenForTesting", header_token);
};

TEST_F(PrivetInfoTest, InfoFailureHTTP) {
  info_operation_->Start();

  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher != NULL);
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                            net::OK));
  fetcher->set_response_code(404);

  EXPECT_CALL(info_callback_, OnPrivetJSONDoneInternal());
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(NULL, privet_client_->GetCachedInfo());
};

class PrivetRegisterTest : public PrivetHTTPTest {
 public:
  PrivetRegisterTest() {
  }
  virtual ~PrivetRegisterTest() {
  }

  virtual void SetUp() OVERRIDE {
    info_operation_ = privet_client_->CreateInfoOperation(
        info_callback_.callback());
    register_operation_ =
        privet_client_->CreateRegisterOperation("example@google.com",
                                                &register_delegate_);
  }

 protected:
  bool SuccessfulResponseToURL(const GURL& url,
                               const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    EXPECT_TRUE(fetcher);
    EXPECT_EQ(url, fetcher->GetOriginalURL());
    if (!fetcher || url != fetcher->GetOriginalURL())
      return false;

    fetcher->SetResponseString(response);
    fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                              net::OK));
    fetcher->set_response_code(200);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    return true;
  }

  scoped_ptr<PrivetJSONOperation> info_operation_;
  NiceMock<MockJSONCallback> info_callback_;
  scoped_ptr<PrivetRegisterOperation> register_operation_;
  StrictMock<MockRegisterDelegate> register_delegate_;
};

TEST_F(PrivetRegisterTest, RegisterSuccessSimple) {
  // Start with info request first to populate XSRF token.
  info_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  register_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=start&user=example%40google.com"),
      kSampleRegisterStartResponse));

  EXPECT_CALL(register_delegate_, OnPrivetRegisterClaimTokenInternal(
      "MySampleToken",
      GURL("https://domain.com/SoMeUrL")));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=getClaimToken&user=example%40google.com"),
      kSampleRegisterGetClaimTokenResponse));

  register_operation_->CompleteRegistration();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=complete&user=example%40google.com"),
      kSampleRegisterCompleteResponse));

  EXPECT_CALL(register_delegate_, OnPrivetRegisterDoneInternal(
      "MyDeviceID"));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponseRegistered));
}

TEST_F(PrivetRegisterTest, RegisterNoInfoCall) {
  register_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=start&user=example%40google.com"),
      kSampleRegisterStartResponse));
}

TEST_F(PrivetRegisterTest, RegisterXSRFFailure) {
  register_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=start&user=example%40google.com"),
      kSampleRegisterStartResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=getClaimToken&user=example%40google.com"),
      kSampleXPrivetErrorResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_CALL(register_delegate_, OnPrivetRegisterClaimTokenInternal(
      "MySampleToken", GURL("https://domain.com/SoMeUrL")));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=getClaimToken&user=example%40google.com"),
      kSampleRegisterGetClaimTokenResponse));
}

TEST_F(PrivetRegisterTest, TransientFailure) {
  register_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=start&user=example%40google.com"),
      kSampleRegisterErrorTransient));

  EXPECT_CALL(fetcher_delegate_, OnRequestStart(0));

  RunFor(base::TimeDelta::FromSeconds(2));

  testing::Mock::VerifyAndClearExpectations(&fetcher_delegate_);

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=start&user=example%40google.com"),
      kSampleRegisterStartResponse));
}

TEST_F(PrivetRegisterTest, PermanentFailure) {
  register_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=start&user=example%40google.com"),
      kSampleRegisterStartResponse));

  EXPECT_CALL(register_delegate_,
              OnPrivetRegisterErrorInternal(
                  "getClaimToken",
                  PrivetRegisterOperation::FAILURE_JSON_ERROR,
                  200));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=getClaimToken&user=example%40google.com"),
      kSampleRegisterErrorPermanent));
}

TEST_F(PrivetRegisterTest, InfoFailure) {
  register_operation_->Start();

  EXPECT_CALL(register_delegate_,
              OnPrivetRegisterErrorInternal(
                  "start",
                  PrivetRegisterOperation::FAILURE_TOKEN,
                  -1));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponseBadJson));
}

TEST_F(PrivetRegisterTest, RegisterCancel) {
  // Start with info request first to populate XSRF token.
  info_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  register_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=start&user=example%40google.com"),
      kSampleRegisterStartResponse));

  register_operation_->Cancel();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/register?"
           "action=cancel&user=example%40google.com"),
      kSampleRegisterCancelResponse));

  // Must keep mocks alive for 3 seconds so the cancelation object can be
  // deleted.
  RunFor(base::TimeDelta::FromSeconds(3));
}

class PrivetCapabilitiesTest : public PrivetHTTPTest {
 public:
  PrivetCapabilitiesTest() {}

  virtual ~PrivetCapabilitiesTest() {}

  virtual void SetUp() OVERRIDE {
    capabilities_operation_ = privet_client_->CreateCapabilitiesOperation(
        capabilities_callback_.callback());
  }

 protected:
  scoped_ptr<PrivetJSONOperation> capabilities_operation_;
  StrictMock<MockJSONCallback> capabilities_callback_;
};

TEST_F(PrivetCapabilitiesTest, SuccessfulCapabilities) {
  capabilities_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_CALL(capabilities_callback_, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponse));

  std::string version;
  EXPECT_TRUE(capabilities_callback_.value()->GetString("version", &version));
  EXPECT_EQ("1.0", version);
};

TEST_F(PrivetCapabilitiesTest, CacheToken) {
  capabilities_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_CALL(capabilities_callback_, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponse));

  capabilities_operation_ = privet_client_->CreateCapabilitiesOperation(
      capabilities_callback_.callback());

  capabilities_operation_->Start();

  EXPECT_CALL(capabilities_callback_, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponse));
};

TEST_F(PrivetCapabilitiesTest, BadToken) {
  capabilities_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleXPrivetErrorResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_CALL(capabilities_callback_, OnPrivetJSONDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponse));
};

class PrivetLocalPrintTest : public PrivetHTTPTest {
 public:
  PrivetLocalPrintTest() {}

  virtual ~PrivetLocalPrintTest() {}

  virtual void SetUp() OVERRIDE {
    local_print_operation_ = privet_client_->CreateLocalPrintOperation(
        &local_print_delegate_);

    local_print_operation_->SetPWGRasterConverterForTesting(
        scoped_ptr<PWGRasterConverter>(new FakePWGRasterConverter));
  }

  scoped_refptr<base::RefCountedBytes> RefCountedBytesFromString(
      std::string str) {
    std::vector<unsigned char> str_vec;
    str_vec.insert(str_vec.begin(), str.begin(), str.end());
    return scoped_refptr<base::RefCountedBytes>(
        base::RefCountedBytes::TakeVector(&str_vec));
  }

 protected:
  scoped_ptr<PrivetLocalPrintOperation> local_print_operation_;
  StrictMock<MockLocalPrintDelegate> local_print_delegate_;
};

TEST_F(PrivetLocalPrintTest, SuccessfulLocalPrint) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(RefCountedBytesFromString(
      "Sample print data"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  // TODO(noamsml): Is encoding spaces as pluses standard?
  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GURL("http://10.0.0.8:6006/privet/printer/submitdoc?"
           "user=sample%40gmail.com&jobname=Sample+job+name"),
      "Sample print data",
      kSampleLocalPrintResponse));
};

TEST_F(PrivetLocalPrintTest, SuccessfulLocalPrintWithAnyMimetype) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(
      RefCountedBytesFromString("Sample print data"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponseWithAnyMimetype));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  // TODO(noamsml): Is encoding spaces as pluses standard?
  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GURL("http://10.0.0.8:6006/privet/printer/submitdoc?"
           "user=sample%40gmail.com&jobname=Sample+job+name"),
      "Sample print data",
      kSampleLocalPrintResponse));
};

TEST_F(PrivetLocalPrintTest, SuccessfulPWGLocalPrint) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetData(
      RefCountedBytesFromString("path/to/"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponse));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponsePWGOnly));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  // TODO(noamsml): Is encoding spaces as pluses standard?
  EXPECT_TRUE(SuccessfulResponseToURLAndFilePath(
      GURL("http://10.0.0.8:6006/privet/printer/submitdoc?"
           "user=sample%40gmail.com&jobname=Sample+job+name"),
      base::FilePath(FILE_PATH_LITERAL("path/to/test.pdf")),
      kSampleLocalPrintResponse));
};

TEST_F(PrivetLocalPrintTest, SuccessfulLocalPrintWithCreatejob) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetTicket("Sample print ticket");
  local_print_operation_->SetData(
      RefCountedBytesFromString("Sample print data"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GURL("http://10.0.0.8:6006/privet/printer/createjob"),
      "Sample print ticket",
      kSampleCreatejobResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  // TODO(noamsml): Is encoding spaces as pluses standard?
  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GURL("http://10.0.0.8:6006/privet/printer/submitdoc?"
           "user=sample%40gmail.com&jobname=Sample+job+name&job_id=1234"),
      "Sample print data",
      kSampleLocalPrintResponse));
};

TEST_F(PrivetLocalPrintTest, PDFPrintInvalidDocumentTypeRetry) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetTicket("Sample print ticket");
  local_print_operation_->SetData(
      RefCountedBytesFromString("sample/path/"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GURL("http://10.0.0.8:6006/privet/printer/createjob"),
      "Sample print ticket",
      kSampleCreatejobResponse));

  // TODO(noamsml): Is encoding spaces as pluses standard?
  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GURL("http://10.0.0.8:6006/privet/printer/submitdoc?"
           "user=sample%40gmail.com&jobname=Sample+job+name&job_id=1234"),
      "sample/path/",
      kSampleInvalidDocumentTypeResponse));

  EXPECT_CALL(local_print_delegate_, OnPrivetPrintingDoneInternal());

  EXPECT_TRUE(SuccessfulResponseToURLAndFilePath(
      GURL("http://10.0.0.8:6006/privet/printer/submitdoc?"
           "user=sample%40gmail.com&jobname=Sample+job+name&job_id=1234"),
      base::FilePath(FILE_PATH_LITERAL("sample/path/test.pdf")),
      kSampleLocalPrintResponse));
};

TEST_F(PrivetLocalPrintTest, LocalPrintRetryOnInvalidJobID) {
  local_print_operation_->SetUsername("sample@gmail.com");
  local_print_operation_->SetJobname("Sample job name");
  local_print_operation_->SetTicket("Sample print ticket");
  local_print_operation_->SetData(
      RefCountedBytesFromString("Sample print data"));
  local_print_operation_->Start();

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/info"),
      kSampleInfoResponseWithCreatejob));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/capabilities"),
      kSampleCapabilitiesResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GURL("http://10.0.0.8:6006/privet/printer/createjob"),
      "Sample print ticket",
      kSampleCreatejobResponse));

  EXPECT_TRUE(SuccessfulResponseToURLAndData(
      GURL("http://10.0.0.8:6006/privet/printer/submitdoc?"
           "user=sample%40gmail.com&jobname=Sample+job+name&job_id=1234"),
      "Sample print data",
      kSampleErrorResponsePrinterBusy));

  RunFor(base::TimeDelta::FromSeconds(3));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL("http://10.0.0.8:6006/privet/printer/createjob"),
      kSampleCreatejobResponse));
};


}  // namespace

}  // namespace local_discovery
