// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_MESSAGE_PROCESS_HOST_H_
#define CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_MESSAGE_PROCESS_HOST_H_

#include <queue>
#include <string>

#include "base/file_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop.h"
#include "base/process.h"
#include "content/public/browser/browser_thread.h"

namespace net {

class DrainableIOBuffer;
class FileStream;
class IOBuffer;
class IOBufferWithSize;

}  // namespace net

namespace extensions {

class NativeProcessLauncher;

// Manages the native side of a connection between an extension and a native
// process.
//
// This class must only be created, called, and deleted on the IO thread.
// Public methods typically accept callbacks which will be invoked on the UI
// thread.
class NativeMessageProcessHost
#if defined(OS_POSIX)
    : public MessageLoopForIO::Watcher
#endif  // !defined(OS_POSIX)
 {
 public:
  // Interface for the object that receives messages from the native process.
  class Client {
   public:
    virtual ~Client() {}
    // Called on the UI thread.
    virtual void PostMessageFromNativeProcess(int port_id,
                                              const std::string& message) = 0;
    virtual void CloseChannel(int port_id, bool error) = 0;
  };

  virtual ~NativeMessageProcessHost();

  static scoped_ptr<NativeMessageProcessHost> Create(
      base::WeakPtr<Client> weak_client_ui,
      const std::string& source_extension_id,
      const std::string& native_host_name,
      int destination_port);

  // Create using specified |launcher|. Used in tests.
  static scoped_ptr<NativeMessageProcessHost> CreateWithLauncher(
      base::WeakPtr<Client> weak_client_ui,
      const std::string& source_extension_id,
      const std::string& native_host_name,
      int destination_port,
      scoped_ptr<NativeProcessLauncher> launcher);

  // Send a message with the specified payload.
  void Send(const std::string& json);

#if defined(OS_POSIX)
  // MessageLoopForIO::Watcher interface
  virtual void OnFileCanReadWithoutBlocking(int fd) OVERRIDE;
  virtual void OnFileCanWriteWithoutBlocking(int fd) OVERRIDE;
#endif  // !defined(OS_POSIX)

  // Try and read a single message from |read_file_|. This should only be called
  // in unittests when you know there is data in the file.
  void ReadNowForTesting();

 private:
  NativeMessageProcessHost(base::WeakPtr<Client> weak_client_ui,
                           const std::string& source_extension_id,
                           const std::string& native_host_name,
                           int destination_port,
                           scoped_ptr<NativeProcessLauncher> launcher);

  // Starts the host process.
  void LaunchHostProcess();

  // Callback for NativeProcessLauncher::Launch().
  void OnHostProcessLaunched(bool result,
                             base::PlatformFile read_file,
                             base::PlatformFile write_file);

  // Helper methods to read incoming messages.
  void WaitRead();
  void DoRead();
  void OnRead(int result);
  void HandleReadResult(int result);
  void ProcessIncomingData(const char* data, int data_size);

  // Helper methods to write outgoing messages.
  void DoWrite();
  void HandleWriteResult(int result);
  void OnWritten(int result);

  // Called when we've failed to start the native host or failed to read or
  // write to/from it. Closes IO pipes and schedules CloseChannel() call.
  void OnError();

  // Closes the connection. Called from OnError() and destructor.
  void Close();

  // The Client messages will be posted to. Should only be accessed from the
  // UI thread.
  base::WeakPtr<Client> weak_client_ui_;

  // ID of the calling extension.
  std::string source_extension_id_;

  // Name of the native messaging host.
  std::string native_host_name_;

  // The id of the port on the other side of this connection. This is passed to
  // |weak_client_ui_| when posting messages.
  int destination_port_;

  // Launcher used to launch the native process.
  scoped_ptr<NativeProcessLauncher> launcher_;

  // Set to true after the native messaging connection has been stopped, e.g.
  // due to an error.
  bool closed_;

  // Input stream handle and reader.
  base::PlatformFile read_file_;
  scoped_ptr<net::FileStream> read_stream_;

#if defined(OS_POSIX)
  MessageLoopForIO::FileDescriptorWatcher read_watcher_;
#endif  // !defined(OS_POSIX)

  // Write stream.
  scoped_ptr<net::FileStream> write_stream_;

  // Read buffer passed to FileStream::Read().
  scoped_refptr<net::IOBuffer> read_buffer_;

  // Set to true when a read is pending.
  bool read_pending_;

  // Set to true once we've read EOF from the child process.
  bool read_eof_;

  // Buffer for incomplete incoming messages.
  std::string incoming_data_;

  // Queue for outgoing messages.
  std::queue<scoped_refptr<net::IOBufferWithSize> > write_queue_;

  // The message that's currently being sent.
  scoped_refptr<net::DrainableIOBuffer> current_write_buffer_;

  // Set to true when a write is pending.
  bool write_pending_;

  DISALLOW_COPY_AND_ASSIGN(NativeMessageProcessHost);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_MESSAGE_PROCESS_HOST_H_
