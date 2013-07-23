// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <string>
#include <vector>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "chrome/test/chromedriver/chrome/log.h"
#include "chrome/test/chromedriver/chrome/version.h"
#include "chrome/test/chromedriver/server/http_handler.h"
#include "chrome/test/chromedriver/server/http_response.h"
#include "net/server/http_server_request_info.h"
#include "third_party/mongoose/mongoose.h"

#if defined(OS_POSIX)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

void SendHttpResponse(bool shutdown,
                      const HttpResponseSenderFunc& send_response_func,
                      scoped_ptr<HttpResponse> response) {
  send_response_func.Run(response.Pass());
  if (shutdown)
    base::MessageLoop::current()->QuitWhenIdle();
}

void HandleHttpRequest(HttpHandler* handler,
                       const net::HttpServerRequestInfo& request,
                       const HttpResponseSenderFunc& send_response_func) {
  handler->Handle(request,
                  base::Bind(&SendHttpResponse,
                             handler->ShouldShutdown(request),
                             send_response_func));
}

void ReadRequestBody(const struct mg_request_info* const request_info,
                     struct mg_connection* const connection,
                     std::string* request_body) {
  int content_length = 0;
  // 64 maximum header count hard-coded in mongoose.h
  for (int header_index = 0; header_index < 64; ++header_index) {
    if (request_info->http_headers[header_index].name == NULL) {
      break;
    }
    if (LowerCaseEqualsASCII(request_info->http_headers[header_index].name,
                             "content-length")) {
      base::StringToInt(
          request_info->http_headers[header_index].value, &content_length);
      break;
    }
  }
  if (content_length > 0) {
    request_body->resize(content_length);
    int bytes_read = 0;
    while (bytes_read < content_length) {
      bytes_read += mg_read(connection,
                            &(*request_body)[bytes_read],
                            content_length - bytes_read);
    }
  }
}

typedef base::Callback<
    void(const net::HttpServerRequestInfo&, const HttpResponseSenderFunc&)>
    HttpRequestHandlerFunc;

struct MongooseUserData {
  base::SingleThreadTaskRunner* cmd_task_runner;
  HttpRequestHandlerFunc* handler_func;
};

void DoneProcessing(base::WaitableEvent* event,
                    scoped_ptr<HttpResponse>* response_to_set,
                    scoped_ptr<HttpResponse> response) {
  *response_to_set = response.Pass();
  event->Signal();
}

void* ProcessHttpRequest(mg_event event_raised,
                         struct mg_connection* connection,
                         const struct mg_request_info* request_info) {
  if (event_raised != MG_NEW_REQUEST)
    return reinterpret_cast<void*>(false);
  MongooseUserData* user_data =
      reinterpret_cast<MongooseUserData*>(request_info->user_data);

  net::HttpServerRequestInfo request;
  request.method = request_info->request_method;
  request.path = request_info->uri;
  ReadRequestBody(request_info, connection, &request.data);

  base::WaitableEvent event(false, false);
  scoped_ptr<HttpResponse> response;
  user_data->cmd_task_runner
      ->PostTask(FROM_HERE,
                 base::Bind(*user_data->handler_func,
                            request,
                            base::Bind(&DoneProcessing, &event, &response)));
  event.Wait();

  // Don't allow HTTP keep alive.
  response->AddHeader("connection", "close");
  std::string data;
  response->GetData(&data);
  mg_write(connection, data.data(), data.length());
  return reinterpret_cast<void*>(true);
}

void MakeMongooseOptions(const std::string& port,
                         int http_threads,
                         std::vector<std::string>* out_options) {
  out_options->push_back("listening_ports");
  out_options->push_back(port);
  out_options->push_back("enable_keep_alive");
  out_options->push_back("no");
  out_options->push_back("num_threads");
  out_options->push_back(base::IntToString(http_threads));
}

}  // namespace

int main(int argc, char *argv[]) {
  CommandLine::Init(argc, argv);

  base::AtExitManager at_exit;
  CommandLine* cmd_line = CommandLine::ForCurrentProcess();

  // Parse command line flags.
  std::string port = "9515";
  std::string url_base;
  int http_threads = 4;
  base::FilePath log_path;
  Log::Level log_level = Log::kError;
  if (cmd_line->HasSwitch("h") || cmd_line->HasSwitch("help")) {
    std::string options;
    const char* kOptionAndDescriptions[] = {
        "port=PORT", "port to listen on",
        "log-path=FILE", "write server log to file instead of stderr, "
            "increases log level to INFO",
        "verbose", "log verbosely",
        "silent", "log nothing",
        "url-base", "base URL path prefix for commands, e.g. wd/url",
        "http-threads=THREAD_COUNT", "number of HTTP threads to spawn",
    };
    for (size_t i = 0; i < arraysize(kOptionAndDescriptions) - 1; i += 2) {
      options += base::StringPrintf(
          "  --%-30s%s\n",
          kOptionAndDescriptions[i], kOptionAndDescriptions[i + 1]);
    }
    printf("Usage: %s [OPTIONS]\n\nOptions\n%s", argv[0], options.c_str());
    return 0;
  }
  if (cmd_line->HasSwitch("port"))
    port = cmd_line->GetSwitchValueASCII("port");
  if (cmd_line->HasSwitch("url-base"))
    url_base = cmd_line->GetSwitchValueASCII("url-base");
  if (url_base.empty() || url_base[0] != '/')
    url_base = "/" + url_base;
  if (url_base[url_base.length() - 1] != '/')
    url_base = url_base + "/";
  if (cmd_line->HasSwitch("http-threads")) {
    if (!base::StringToInt(cmd_line->GetSwitchValueASCII("http-threads"),
                           &http_threads)) {
      printf("'http-threads' option must be an integer\n");
      return 1;
    }
  }
  if (cmd_line->HasSwitch("log-path")) {
    log_level = Log::kLog;
    log_path = cmd_line->GetSwitchValuePath("log-path");
#if defined(OS_WIN)
    FILE* redir_stderr = _wfreopen(log_path.value().c_str(), L"w", stderr);
#else
    FILE* redir_stderr = freopen(log_path.value().c_str(), "w", stderr);
#endif
    if (!redir_stderr) {
      printf("Failed to redirect stderr to log file. Exiting...\n");
      return 1;
    }
  }
  if (cmd_line->HasSwitch("verbose"))
    log_level = Log::kDebug;

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  bool success = logging::InitLogging(settings);
  if (!success) {
    PLOG(ERROR) << "Unable to initialize logging";
  }
  logging::SetLogItems(false,  // enable_process_id
                       false,  // enable_thread_id
                       false,  // enable_timestamp
                       false); // enable_tickcount
  if (!cmd_line->HasSwitch("verbose"))
    logging::SetMinLogLevel(logging::LOG_FATAL);

  base::Thread io_thread("ChromeDriver IO");
  CHECK(io_thread.StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0)));

  scoped_ptr<Log> log(new Logger(log_level));
  HttpHandler handler(io_thread.message_loop_proxy(), log.get(), url_base);
  base::MessageLoop cmd_loop;
  HttpRequestHandlerFunc handler_func =
      base::Bind(&HandleHttpRequest, &handler);
  MongooseUserData user_data = { cmd_loop.message_loop_proxy(), &handler_func };

  std::vector<std::string> args;
  MakeMongooseOptions(port, http_threads, &args);
  scoped_ptr<const char*[]> options(new const char*[args.size() + 1]);
  for (size_t i = 0; i < args.size(); ++i) {
    options[i] = args[i].c_str();
  }
  options[args.size()] = NULL;

  struct mg_context* ctx = mg_start(&ProcessHttpRequest,
                                    &user_data,
                                    options.get());
  if (ctx == NULL) {
    printf("Port not available. Exiting...\n");
    return 1;
  }

  if (!cmd_line->HasSwitch("silent")) {
    printf("Started ChromeDriver (v%s) on port %s\n",
           kChromeDriverVersion,
           port.c_str());
    fflush(stdout);
  }

#if defined(OS_POSIX)
  if (!cmd_line->HasSwitch("verbose")) {
    // Close stderr on exec, so that Chrome log spew doesn't confuse users.
    fcntl(STDERR_FILENO, F_SETFD, FD_CLOEXEC);
  }
#endif

  base::RunLoop cmd_run_loop;
  cmd_run_loop.Run();
  // Don't run destructors for objects passed via MongooseUserData,
  // because ProcessHttpRequest may be accessing them.
  // TODO(kkania): Fix when switching to net::HttpServer.
  exit(0);
}
