// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/nacl/loader/nacl_ipc_adapter.h"

#include <limits.h>
#include <string.h>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/shared_memory.h"
#include "build/build_config.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_platform_file.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_custom.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nacl_desc_sync_socket.h"
#include "native_client/src/trusted/desc/nacl_desc_wrapper.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "ppapi/c/ppb_file_io.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_handle.h"

namespace {

enum BufferSizeStatus {
  // The buffer contains a full message with no extra bytes.
  MESSAGE_IS_COMPLETE,

  // The message doesn't fit and the buffer contains only some of it.
  MESSAGE_IS_TRUNCATED,

  // The buffer contains a full message + extra data.
  MESSAGE_HAS_EXTRA_DATA
};

BufferSizeStatus GetBufferStatus(const char* data, size_t len) {
  if (len < sizeof(NaClIPCAdapter::NaClMessageHeader))
    return MESSAGE_IS_TRUNCATED;

  const NaClIPCAdapter::NaClMessageHeader* header =
      reinterpret_cast<const NaClIPCAdapter::NaClMessageHeader*>(data);
  uint32 message_size =
      sizeof(NaClIPCAdapter::NaClMessageHeader) + header->payload_size;

  if (len == message_size)
    return MESSAGE_IS_COMPLETE;
  if (len > message_size)
    return MESSAGE_HAS_EXTRA_DATA;
  return MESSAGE_IS_TRUNCATED;
}

// This object allows the NaClDesc to hold a reference to a NaClIPCAdapter and
// forward calls to it.
struct DescThunker {
  explicit DescThunker(NaClIPCAdapter* adapter_param)
      : adapter(adapter_param) {
  }
  scoped_refptr<NaClIPCAdapter> adapter;
};

NaClIPCAdapter* ToAdapter(void* handle) {
  return static_cast<DescThunker*>(handle)->adapter.get();
}

// NaClDescCustom implementation.
void NaClDescCustomDestroy(void* handle) {
  delete static_cast<DescThunker*>(handle);
}

ssize_t NaClDescCustomSendMsg(void* handle, const NaClImcTypedMsgHdr* msg,
                              int /* flags */) {
  return static_cast<ssize_t>(ToAdapter(handle)->Send(msg));
}

ssize_t NaClDescCustomRecvMsg(void* handle, NaClImcTypedMsgHdr* msg,
                              int /* flags */) {
  return static_cast<ssize_t>(ToAdapter(handle)->BlockingReceive(msg));
}

NaClDesc* MakeNaClDescCustom(NaClIPCAdapter* adapter) {
  NaClDescCustomFuncs funcs = NACL_DESC_CUSTOM_FUNCS_INITIALIZER;
  funcs.Destroy = NaClDescCustomDestroy;
  funcs.SendMsg = NaClDescCustomSendMsg;
  funcs.RecvMsg = NaClDescCustomRecvMsg;
  // NaClDescMakeCustomDesc gives us a reference on the returned NaClDesc.
  return NaClDescMakeCustomDesc(new DescThunker(adapter), &funcs);
}

void DeleteChannel(IPC::Channel* channel) {
  delete channel;
}

// Translates Pepper's read/write open flags into the NaCl equivalents.
// Since the host has already opened the file, flags such as O_CREAT, O_TRUNC,
// and O_EXCL don't make sense, so we filter those out. If no read or write
// flags are set, the function returns NACL_ABI_O_RDONLY as a safe fallback.
int TranslatePepperFileReadWriteOpenFlags(int32_t pp_open_flags) {
  bool read = (pp_open_flags & PP_FILEOPENFLAG_READ) != 0;
  bool write = (pp_open_flags & PP_FILEOPENFLAG_WRITE) != 0;
  bool append = (pp_open_flags & PP_FILEOPENFLAG_APPEND) != 0;

  int nacl_open_flag = NACL_ABI_O_RDONLY;  // NACL_ABI_O_RDONLY == 0.
  if (read && (write || append)) {
    nacl_open_flag = NACL_ABI_O_RDWR;
  } else if (write || append) {
    nacl_open_flag = NACL_ABI_O_WRONLY;
  } else if (!read) {
    DLOG(WARNING) << "One of PP_FILEOPENFLAG_READ, PP_FILEOPENFLAG_WRITE, "
                  << "or PP_FILEOPENFLAG_APPEND should be set.";
  }
  if (append)
    nacl_open_flag |= NACL_ABI_O_APPEND;

  return nacl_open_flag;
}

class NaClDescWrapper {
 public:
  explicit NaClDescWrapper(NaClDesc* desc): desc_(desc) {}
  ~NaClDescWrapper() {
    NaClDescUnref(desc_);
  }

  NaClDesc* desc() { return desc_; }

 private:
  NaClDesc* desc_;
  DISALLOW_COPY_AND_ASSIGN(NaClDescWrapper);
};

}  // namespace

class NaClIPCAdapter::RewrittenMessage
    : public base::RefCounted<RewrittenMessage> {
 public:
  RewrittenMessage();

  bool is_consumed() const { return data_read_cursor_ == data_len_; }

  void SetData(const NaClIPCAdapter::NaClMessageHeader& header,
               const void* payload, size_t payload_length);

  int Read(NaClImcTypedMsgHdr* msg);

  void AddDescriptor(NaClDescWrapper* desc) { descs_.push_back(desc); }

  size_t desc_count() const { return descs_.size(); }

 private:
  friend class base::RefCounted<RewrittenMessage>;
  ~RewrittenMessage() {}

  scoped_ptr<char[]> data_;
  size_t data_len_;

  // Offset into data where the next read will happen. This will be equal to
  // data_len_ when all data has been consumed.
  size_t data_read_cursor_;

  // Wrapped descriptors for transfer to untrusted code.
  ScopedVector<NaClDescWrapper> descs_;
};

NaClIPCAdapter::RewrittenMessage::RewrittenMessage()
    : data_len_(0),
      data_read_cursor_(0) {
}

void NaClIPCAdapter::RewrittenMessage::SetData(
    const NaClIPCAdapter::NaClMessageHeader& header,
    const void* payload,
    size_t payload_length) {
  DCHECK(!data_.get() && data_len_ == 0);
  size_t header_len = sizeof(NaClIPCAdapter::NaClMessageHeader);
  data_len_ = header_len + payload_length;
  data_.reset(new char[data_len_]);

  memcpy(data_.get(), &header, sizeof(NaClIPCAdapter::NaClMessageHeader));
  memcpy(&data_[header_len], payload, payload_length);
}

int NaClIPCAdapter::RewrittenMessage::Read(NaClImcTypedMsgHdr* msg) {
  CHECK(data_len_ >= data_read_cursor_);
  char* dest_buffer = static_cast<char*>(msg->iov[0].base);
  size_t dest_buffer_size = msg->iov[0].length;
  size_t bytes_to_write = std::min(dest_buffer_size,
                                   data_len_ - data_read_cursor_);
  if (bytes_to_write == 0)
    return 0;

  memcpy(dest_buffer, &data_[data_read_cursor_], bytes_to_write);
  data_read_cursor_ += bytes_to_write;

  // Once all data has been consumed, transfer any file descriptors.
  if (is_consumed()) {
    nacl_abi_size_t desc_count = static_cast<nacl_abi_size_t>(descs_.size());
    CHECK(desc_count <= msg->ndesc_length);
    msg->ndesc_length = desc_count;
    for (nacl_abi_size_t i = 0; i < desc_count; i++) {
      // Copy the NaClDesc to the buffer and add a ref so it won't be freed
      // when we clear our ScopedVector.
      msg->ndescv[i] = descs_[i]->desc();
      NaClDescRef(descs_[i]->desc());
    }
    descs_.clear();
  } else {
    msg->ndesc_length = 0;
  }
  return static_cast<int>(bytes_to_write);
}

NaClIPCAdapter::LockedData::LockedData()
    : channel_closed_(false) {
}

NaClIPCAdapter::LockedData::~LockedData() {
}

NaClIPCAdapter::IOThreadData::IOThreadData() {
}

NaClIPCAdapter::IOThreadData::~IOThreadData() {
}

NaClIPCAdapter::NaClIPCAdapter(const IPC::ChannelHandle& handle,
                               base::TaskRunner* runner)
    : lock_(),
      cond_var_(&lock_),
      task_runner_(runner),
      locked_data_() {
  io_thread_data_.channel_.reset(
      new IPC::Channel(handle, IPC::Channel::MODE_SERVER, this));
  // Note, we can not PostTask for ConnectChannelOnIOThread here. If we did,
  // and that task ran before this constructor completes, the reference count
  // would go to 1 and then to 0 because of the Task, before we've been returned
  // to the owning scoped_refptr, which is supposed to give us our first
  // ref-count.
}

NaClIPCAdapter::NaClIPCAdapter(scoped_ptr<IPC::Channel> channel,
                               base::TaskRunner* runner)
    : lock_(),
      cond_var_(&lock_),
      task_runner_(runner),
      locked_data_() {
  io_thread_data_.channel_ = channel.Pass();
}

void NaClIPCAdapter::ConnectChannel() {
  task_runner_->PostTask(FROM_HERE,
      base::Bind(&NaClIPCAdapter::ConnectChannelOnIOThread, this));
}

// Note that this message is controlled by the untrusted code. So we should be
// skeptical of anything it contains and quick to give up if anything is fishy.
int NaClIPCAdapter::Send(const NaClImcTypedMsgHdr* msg) {
  if (msg->iov_length != 1)
    return -1;

  base::AutoLock lock(lock_);

  const char* input_data = static_cast<char*>(msg->iov[0].base);
  size_t input_data_len = msg->iov[0].length;
  if (input_data_len > IPC::Channel::kMaximumMessageSize) {
    ClearToBeSent();
    return -1;
  }

  // current_message[_len] refers to the total input data received so far.
  const char* current_message;
  size_t current_message_len;
  bool did_append_input_data;
  if (locked_data_.to_be_sent_.empty()) {
    // No accumulated data, we can avoid a copy by referring to the input
    // buffer (the entire message fitting in one call is the common case).
    current_message = input_data;
    current_message_len = input_data_len;
    did_append_input_data = false;
  } else {
    // We've already accumulated some data, accumulate this new data and
    // point to the beginning of the buffer.

    // Make sure our accumulated message size doesn't overflow our max. Since
    // we know that data_len < max size (checked above) and our current
    // accumulated value is also < max size, we just need to make sure that
    // 2x max size can never overflow.
    COMPILE_ASSERT(IPC::Channel::kMaximumMessageSize < (UINT_MAX / 2),
                   MaximumMessageSizeWillOverflow);
    size_t new_size = locked_data_.to_be_sent_.size() + input_data_len;
    if (new_size > IPC::Channel::kMaximumMessageSize) {
      ClearToBeSent();
      return -1;
    }

    locked_data_.to_be_sent_.append(input_data, input_data_len);
    current_message = &locked_data_.to_be_sent_[0];
    current_message_len = locked_data_.to_be_sent_.size();
    did_append_input_data = true;
  }

  // Check the total data we've accumulated so far to see if it contains a full
  // message.
  switch (GetBufferStatus(current_message, current_message_len)) {
    case MESSAGE_IS_COMPLETE: {
      // Got a complete message, can send it out. This will be the common case.
      bool success = SendCompleteMessage(current_message, current_message_len);
      ClearToBeSent();
      return success ? static_cast<int>(input_data_len) : -1;
    }
    case MESSAGE_IS_TRUNCATED:
      // For truncated messages, just accumulate the new data (if we didn't
      // already do so above) and go back to waiting for more.
      if (!did_append_input_data)
        locked_data_.to_be_sent_.append(input_data, input_data_len);
      return static_cast<int>(input_data_len);
    case MESSAGE_HAS_EXTRA_DATA:
    default:
      // When the plugin gives us too much data, it's an error.
      ClearToBeSent();
      return -1;
  }
}

int NaClIPCAdapter::BlockingReceive(NaClImcTypedMsgHdr* msg) {
  if (msg->iov_length != 1)
    return -1;

  int retval = 0;
  {
    base::AutoLock lock(lock_);
    while (locked_data_.to_be_received_.empty() &&
           !locked_data_.channel_closed_)
      cond_var_.Wait();
    if (locked_data_.channel_closed_) {
      retval = -1;
    } else {
      retval = LockedReceive(msg);
      DCHECK(retval > 0);
    }
  }
  cond_var_.Signal();
  return retval;
}

void NaClIPCAdapter::CloseChannel() {
  {
    base::AutoLock lock(lock_);
    locked_data_.channel_closed_ = true;
  }
  cond_var_.Signal();

  task_runner_->PostTask(FROM_HERE,
      base::Bind(&NaClIPCAdapter::CloseChannelOnIOThread, this));
}

NaClDesc* NaClIPCAdapter::MakeNaClDesc() {
  return MakeNaClDescCustom(this);
}

#if defined(OS_POSIX)
int NaClIPCAdapter::TakeClientFileDescriptor() {
  return io_thread_data_.channel_->TakeClientFileDescriptor();
}
#endif

bool NaClIPCAdapter::OnMessageReceived(const IPC::Message& msg) {
  {
    base::AutoLock lock(lock_);

    scoped_refptr<RewrittenMessage> rewritten_msg(new RewrittenMessage);

    typedef std::vector<ppapi::proxy::SerializedHandle> Handles;
    Handles handles;
    scoped_ptr<IPC::Message> new_msg;
    if (!locked_data_.nacl_msg_scanner_.ScanMessage(msg, &handles, &new_msg))
      return false;

    // Now add any descriptors we found to rewritten_msg. |handles| is usually
    // empty, unless we read a message containing a FD or handle.
    for (Handles::const_iterator iter = handles.begin();
         iter != handles.end();
         ++iter) {
      scoped_ptr<NaClDescWrapper> nacl_desc;
      switch (iter->type()) {
        case ppapi::proxy::SerializedHandle::SHARED_MEMORY: {
          const base::SharedMemoryHandle& shm_handle = iter->shmem();
          uint32_t size = iter->size();
          nacl_desc.reset(new NaClDescWrapper(NaClDescImcShmMake(
#if defined(OS_WIN)
              shm_handle,
#else
              shm_handle.fd,
#endif
              static_cast<size_t>(size))));
          break;
        }
        case ppapi::proxy::SerializedHandle::SOCKET: {
          nacl_desc.reset(new NaClDescWrapper(NaClDescSyncSocketMake(
#if defined(OS_WIN)
              iter->descriptor()
#else
              iter->descriptor().fd
#endif
          )));
          break;
        }
        case ppapi::proxy::SerializedHandle::CHANNEL_HANDLE: {
          // Check that this came from a PpapiMsg_CreateNaClChannel message.
          // This code here is only appropriate for that message.
          DCHECK(msg.type() == PpapiMsg_CreateNaClChannel::ID);
          IPC::ChannelHandle channel_handle =
              IPC::Channel::GenerateVerifiedChannelID("nacl");
          scoped_refptr<NaClIPCAdapter> ipc_adapter(
              new NaClIPCAdapter(channel_handle, task_runner_.get()));
          ipc_adapter->ConnectChannel();
#if defined(OS_POSIX)
          channel_handle.socket = base::FileDescriptor(
              ipc_adapter->TakeClientFileDescriptor(), true);
#endif
          nacl_desc.reset(new NaClDescWrapper(ipc_adapter->MakeNaClDesc()));
          // Send back a message that the channel was created.
          scoped_ptr<IPC::Message> response(
              new PpapiHostMsg_ChannelCreated(channel_handle));
          task_runner_->PostTask(FROM_HERE,
              base::Bind(&NaClIPCAdapter::SendMessageOnIOThread, this,
                         base::Passed(&response)));
          break;
        }
        case ppapi::proxy::SerializedHandle::FILE:
          // IMPORTANT: The NaClDescIoDescFromHandleAllocCtor function creates
          // a NaClDesc that checks file flags before reading and writing. This
          // is essential since PPB_FileIO now sends a file descriptor to the
          // plugin which may have write capabilities. We can't allow the plugin
          // to write with it since it could bypass quota checks, which still
          // happen in the host.
          nacl_desc.reset(new NaClDescWrapper(NaClDescIoDescFromHandleAllocCtor(
#if defined(OS_WIN)
              iter->descriptor(),
#else
              iter->descriptor().fd,
#endif
              TranslatePepperFileReadWriteOpenFlags(iter->open_flag()))));
          break;
        case ppapi::proxy::SerializedHandle::INVALID: {
          // Nothing to do. TODO(dmichael): Should we log this? Or is it
          // sometimes okay to pass an INVALID handle?
          break;
        }
        // No default, so the compiler will warn us if new types get added.
      }
      if (nacl_desc.get())
        rewritten_msg->AddDescriptor(nacl_desc.release());
    }
    if (new_msg)
      SaveMessage(*new_msg, rewritten_msg.get());
    else
      SaveMessage(msg, rewritten_msg.get());
  }
  cond_var_.Signal();
  return true;
}

void NaClIPCAdapter::OnChannelConnected(int32 peer_pid) {
}

void NaClIPCAdapter::OnChannelError() {
  CloseChannel();
}

NaClIPCAdapter::~NaClIPCAdapter() {
  // Make sure the channel is deleted on the IO thread.
  task_runner_->PostTask(FROM_HERE,
      base::Bind(&DeleteChannel, io_thread_data_.channel_.release()));
}

int NaClIPCAdapter::LockedReceive(NaClImcTypedMsgHdr* msg) {
  lock_.AssertAcquired();

  if (locked_data_.to_be_received_.empty())
    return 0;
  scoped_refptr<RewrittenMessage> current =
      locked_data_.to_be_received_.front();

  int retval = current->Read(msg);

  // When a message is entirely consumed, remove if from the waiting queue.
  if (current->is_consumed())
    locked_data_.to_be_received_.pop();

  return retval;
}

bool NaClIPCAdapter::SendCompleteMessage(const char* buffer,
                                         size_t buffer_len) {
  lock_.AssertAcquired();
  // The message will have already been validated, so we know it's large enough
  // for our header.
  const NaClMessageHeader* header =
      reinterpret_cast<const NaClMessageHeader*>(buffer);

  // Length of the message not including the body. The data passed to us by the
  // plugin should match that in the message header. This should have already
  // been validated by GetBufferStatus.
  int body_len = static_cast<int>(buffer_len - sizeof(NaClMessageHeader));
  DCHECK(body_len == static_cast<int>(header->payload_size));

  // We actually discard the flags and only copy the ones we care about. This
  // is just because message doesn't have a constructor that takes raw flags.
  scoped_ptr<IPC::Message> msg(
      new IPC::Message(header->routing, header->type,
                       IPC::Message::PRIORITY_NORMAL));
  if (header->flags & IPC::Message::SYNC_BIT)
    msg->set_sync();
  if (header->flags & IPC::Message::REPLY_BIT)
    msg->set_reply();
  if (header->flags & IPC::Message::REPLY_ERROR_BIT)
    msg->set_reply_error();
  if (header->flags & IPC::Message::UNBLOCK_BIT)
    msg->set_unblock(true);

  msg->WriteBytes(&buffer[sizeof(NaClMessageHeader)], body_len);

  // Technically we didn't have to do any of the previous work in the lock. But
  // sometimes our buffer will point to the to_be_sent_ string which is
  // protected by the lock, and it's messier to factor Send() such that it can
  // unlock for us. Holding the lock for the message construction, which is
  // just some memcpys, shouldn't be a big deal.
  lock_.AssertAcquired();
  if (locked_data_.channel_closed_) {
    // If we ever pass handles from the plugin to the host, we should close them
    // here before we drop the message.
    return false;
  }

  // Scan all untrusted messages.
  scoped_ptr<IPC::Message> new_msg;
  locked_data_.nacl_msg_scanner_.ScanUntrustedMessage(*msg, &new_msg);
  if (new_msg)
    msg.reset(new_msg.release());

  // Actual send must be done on the I/O thread.
  task_runner_->PostTask(FROM_HERE,
      base::Bind(&NaClIPCAdapter::SendMessageOnIOThread, this,
                 base::Passed(&msg)));
  return true;
}

void NaClIPCAdapter::ClearToBeSent() {
  lock_.AssertAcquired();

  // Don't let the string keep its buffer behind our back.
  std::string empty;
  locked_data_.to_be_sent_.swap(empty);
}

void NaClIPCAdapter::ConnectChannelOnIOThread() {
  if (!io_thread_data_.channel_->Connect())
    NOTREACHED();
}

void NaClIPCAdapter::CloseChannelOnIOThread() {
  io_thread_data_.channel_->Close();
}

void NaClIPCAdapter::SendMessageOnIOThread(scoped_ptr<IPC::Message> message) {
  io_thread_data_.channel_->Send(message.release());
}

void NaClIPCAdapter::SaveMessage(const IPC::Message& msg,
                                 RewrittenMessage* rewritten_msg) {
  lock_.AssertAcquired();
  // There is some padding in this structure (the "padding" member is 16
  // bits but this then gets padded to 32 bits). We want to be sure not to
  // leak data to the untrusted plugin, so zero everything out first.
  NaClMessageHeader header;
  memset(&header, 0, sizeof(NaClMessageHeader));

  header.payload_size = static_cast<uint32>(msg.payload_size());
  header.routing = msg.routing_id();
  header.type = msg.type();
  header.flags = msg.flags();
  header.num_fds = static_cast<int>(rewritten_msg->desc_count());

  rewritten_msg->SetData(header, msg.payload(), msg.payload_size());
  locked_data_.to_be_received_.push(rewritten_msg);
}

int TranslatePepperFileReadWriteOpenFlagsForTesting(int32_t pp_open_flags) {
  return TranslatePepperFileReadWriteOpenFlags(pp_open_flags);
}
