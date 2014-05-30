// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc_rtp_dump_writer.h"

#include "base/big_endian.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/zlib/zlib.h"

using content::BrowserThread;

namespace {

static const size_t kMinimumGzipOutputBufferSize = 256;  // In bytes.

const unsigned char kRtpDumpFileHeaderFirstLine[] = "#!rtpplay1.0 0.0.0.0/0\n";
static const size_t kRtpDumpFileHeaderSize = 16;  // In bytes.

// A helper for writing the header of the dump file.
void WriteRtpDumpFileHeaderBigEndian(base::TimeTicks start,
                                     std::vector<uint8>* output) {
  size_t buffer_start_pos = output->size();
  output->resize(output->size() + kRtpDumpFileHeaderSize);

  char* buffer = reinterpret_cast<char*>(&(*output)[buffer_start_pos]);

  base::TimeDelta delta = start - base::TimeTicks();
  uint32 start_sec = delta.InSeconds();
  base::WriteBigEndian(buffer, start_sec);
  buffer += sizeof(start_sec);

  uint32 start_usec =
      delta.InMilliseconds() * base::Time::kMicrosecondsPerMillisecond;
  base::WriteBigEndian(buffer, start_usec);
  buffer += sizeof(start_usec);

  // Network source, always 0.
  base::WriteBigEndian(buffer, uint32(0));
  buffer += sizeof(uint32);

  // UDP port, always 0.
  base::WriteBigEndian(buffer, uint16(0));
  buffer += sizeof(uint16);

  // 2 bytes padding.
  base::WriteBigEndian(buffer, uint16(0));
}

// The header size for each packet dump.
static const size_t kPacketDumpHeaderSize = 8;  // In bytes.

// A helper for writing the header for each packet dump.
// |start| is the time when the recording is started.
// |dump_length| is the length of the packet dump including this header.
// |packet_length| is the length of the RTP packet header.
void WritePacketDumpHeaderBigEndian(const base::TimeTicks& start,
                                    uint16 dump_length,
                                    uint16 packet_length,
                                    std::vector<uint8>* output) {
  size_t buffer_start_pos = output->size();
  output->resize(output->size() + kPacketDumpHeaderSize);

  char* buffer = reinterpret_cast<char*>(&(*output)[buffer_start_pos]);

  base::WriteBigEndian(buffer, dump_length);
  buffer += sizeof(dump_length);

  base::WriteBigEndian(buffer, packet_length);
  buffer += sizeof(packet_length);

  base::WriteBigEndian(buffer,
                       (base::TimeTicks::Now() - start).InMilliseconds());
}

// Append |src_len| bytes from |src| to |dest|.
void AppendToBuffer(const uint8* src,
                    size_t src_len,
                    std::vector<uint8>* dest) {
  size_t old_dest_size = dest->size();
  dest->resize(old_dest_size + src_len);
  memcpy(&(*dest)[old_dest_size], src, src_len);
}

}  // namespace

// This class is running on the FILE thread for compressing and writing the
// dump buffer to disk.
class WebRtcRtpDumpWriter::FileThreadWorker {
 public:
  explicit FileThreadWorker(const base::FilePath& dump_path)
      : dump_path_(dump_path) {
    thread_checker_.DetachFromThread();

    memset(&stream_, 0, sizeof(stream_));
    int result = deflateInit2(&stream_,
                              Z_DEFAULT_COMPRESSION,
                              Z_DEFLATED,
                              // windowBits = 15 is default, 16 is added to
                              // produce a gzip header + trailer.
                              15 + 16,
                              8,  // memLevel = 8 is default.
                              Z_DEFAULT_STRATEGY);
    DCHECK_EQ(Z_OK, result);
  }

  ~FileThreadWorker() {
    DCHECK(thread_checker_.CalledOnValidThread());
  }

  // Compresses the data in |buffer| and write to the dump file. If |end_stream|
  // is true, the compression stream will be ended and the dump file cannot be
  // written to any more.
  void CompressAndWriteToFileOnFileThread(
      scoped_ptr<std::vector<uint8> > buffer,
      bool end_stream,
      FlushResult* result,
      size_t* bytes_written) {
    DCHECK(thread_checker_.CalledOnValidThread());

    // This is called either when the in-memory buffer is full or the dump
    // should be ended.
    DCHECK(!buffer->empty() || end_stream);

    *result = FLUSH_RESULT_SUCCESS;
    *bytes_written = 0;

    // There may be nothing to compress/write if there is no RTP packet since
    // the last flush.
    if (!buffer->empty()) {
      *bytes_written = CompressAndWriteBufferToFile(buffer.get(), result);
    } else if (!base::PathExists(dump_path_)) {
      // If the dump does not exist, it means there is no RTP packet recorded.
      // Return FLUSH_RESULT_NO_DATA to indicate no dump file created.
      *result = FLUSH_RESULT_NO_DATA;
    }

    if (end_stream && !EndDumpFile())
      *result = FLUSH_RESULT_FAILURE;
  }

 private:
  // Helper for CompressAndWriteToFileOnFileThread to compress and write one
  // dump.
  size_t CompressAndWriteBufferToFile(std::vector<uint8>* buffer,
                                      FlushResult* result) {
    DCHECK(thread_checker_.CalledOnValidThread());
    DCHECK(buffer->size());

    *result = FLUSH_RESULT_SUCCESS;

    std::vector<uint8> compressed_buffer;
    if (!Compress(buffer, &compressed_buffer)) {
      DVLOG(2) << "Compressing buffer failed.";
      *result = FLUSH_RESULT_FAILURE;
      return 0;
    }

    int bytes_written = -1;

    if (base::PathExists(dump_path_)) {
      bytes_written = base::AppendToFile(
          dump_path_,
          reinterpret_cast<const char*>(&compressed_buffer[0]),
          compressed_buffer.size());
    } else {
      bytes_written = base::WriteFile(
          dump_path_,
          reinterpret_cast<const char*>(&compressed_buffer[0]),
          compressed_buffer.size());
    }

    if (bytes_written == -1) {
      DVLOG(2) << "Writing file failed: " << dump_path_.value();
      *result = FLUSH_RESULT_FAILURE;
      return 0;
    }

    DCHECK_EQ(static_cast<size_t>(bytes_written), compressed_buffer.size());
    return bytes_written;
  }

  // Compresses |input| into |output|.
  bool Compress(std::vector<uint8>* input, std::vector<uint8>* output) {
    DCHECK(thread_checker_.CalledOnValidThread());
    int result = Z_OK;

    output->resize(std::max(kMinimumGzipOutputBufferSize, input->size()));

    stream_.next_in = &(*input)[0];
    stream_.avail_in = input->size();
    stream_.next_out = &(*output)[0];
    stream_.avail_out = output->size();

    result = deflate(&stream_, Z_SYNC_FLUSH);
    DCHECK_EQ(Z_OK, result);
    DCHECK_EQ(0U, stream_.avail_in);

    output->resize(output->size() - stream_.avail_out);

    stream_.next_in = NULL;
    stream_.next_out = NULL;
    stream_.avail_out = 0;
    return true;
  }

  // Ends the compression stream and completes the dump file.
  bool EndDumpFile() {
    DCHECK(thread_checker_.CalledOnValidThread());

    std::vector<uint8> output_buffer;
    output_buffer.resize(kMinimumGzipOutputBufferSize);

    stream_.next_in = NULL;
    stream_.avail_in = 0;
    stream_.next_out = &output_buffer[0];
    stream_.avail_out = output_buffer.size();

    int result = deflate(&stream_, Z_FINISH);
    DCHECK_EQ(Z_STREAM_END, result);

    result = deflateEnd(&stream_);
    DCHECK_EQ(Z_OK, result);

    output_buffer.resize(output_buffer.size() - stream_.avail_out);

    memset(&stream_, 0, sizeof(z_stream));

    DCHECK(!output_buffer.empty());
    int bytes_written =
        base::AppendToFile(dump_path_,
                           reinterpret_cast<const char*>(&output_buffer[0]),
                           output_buffer.size());

    return bytes_written > 0;
  }

  const base::FilePath dump_path_;

  z_stream stream_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(FileThreadWorker);
};

WebRtcRtpDumpWriter::WebRtcRtpDumpWriter(
    const base::FilePath& incoming_dump_path,
    const base::FilePath& outgoing_dump_path,
    size_t max_dump_size,
    const base::Closure& max_dump_size_reached_callback)
    : max_dump_size_(max_dump_size),
      max_dump_size_reached_callback_(max_dump_size_reached_callback),
      total_dump_size_on_disk_(0),
      incoming_file_thread_worker_(new FileThreadWorker(incoming_dump_path)),
      outgoing_file_thread_worker_(new FileThreadWorker(outgoing_dump_path)),
      weak_ptr_factory_(this) {
}

WebRtcRtpDumpWriter::~WebRtcRtpDumpWriter() {
  DCHECK(thread_checker_.CalledOnValidThread());

  bool success = BrowserThread::DeleteSoon(
      BrowserThread::FILE, FROM_HERE, incoming_file_thread_worker_.release());
  DCHECK(success);

  success = BrowserThread::DeleteSoon(
      BrowserThread::FILE, FROM_HERE, outgoing_file_thread_worker_.release());
  DCHECK(success);
}

void WebRtcRtpDumpWriter::WriteRtpPacket(const uint8* packet_header,
                                         size_t header_length,
                                         size_t packet_length,
                                         bool incoming) {
  DCHECK(thread_checker_.CalledOnValidThread());

  static const size_t kMaxInMemoryBufferSize = 65536;

  std::vector<uint8>* dest_buffer =
      incoming ? &incoming_buffer_ : &outgoing_buffer_;

  // We use the capacity of the buffer to indicate if the buffer has been
  // initialized and if the dump file header has been created.
  if (!dest_buffer->capacity()) {
    dest_buffer->reserve(std::min(kMaxInMemoryBufferSize, max_dump_size_));

    start_time_ = base::TimeTicks::Now();

    // Writes the dump file header.
    AppendToBuffer(kRtpDumpFileHeaderFirstLine,
                   arraysize(kRtpDumpFileHeaderFirstLine) - 1,
                   dest_buffer);
    WriteRtpDumpFileHeaderBigEndian(start_time_, dest_buffer);
  }

  size_t packet_dump_length = kPacketDumpHeaderSize + header_length;

  // Flushes the buffer to disk if the buffer is full.
  if (dest_buffer->size() + packet_dump_length > dest_buffer->capacity())
    FlushBuffer(incoming, false, FlushDoneCallback());

  WritePacketDumpHeaderBigEndian(
      start_time_, packet_dump_length, packet_length, dest_buffer);

  // Writes the actual RTP packet header.
  AppendToBuffer(packet_header, header_length, dest_buffer);
}

void WebRtcRtpDumpWriter::EndDump(RtpDumpType type,
                                  const EndDumpCallback& finished_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(type == RTP_DUMP_OUTGOING || incoming_file_thread_worker_ != NULL);
  DCHECK(type == RTP_DUMP_INCOMING || outgoing_file_thread_worker_ != NULL);

  bool incoming = (type == RTP_DUMP_BOTH || type == RTP_DUMP_INCOMING);
  EndDumpContext context(type, finished_callback);

  // End the incoming dump first if required. OnDumpEnded will continue to end
  // the outgoing dump if necessary.
  FlushBuffer(incoming,
              true,
              base::Bind(&WebRtcRtpDumpWriter::OnDumpEnded,
                         weak_ptr_factory_.GetWeakPtr(),
                         context,
                         incoming));
}

size_t WebRtcRtpDumpWriter::max_dump_size() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return max_dump_size_;
}

WebRtcRtpDumpWriter::EndDumpContext::EndDumpContext(
    RtpDumpType type,
    const EndDumpCallback& callback)
    : type(type),
      incoming_succeeded(false),
      outgoing_succeeded(false),
      callback(callback) {
}

WebRtcRtpDumpWriter::EndDumpContext::~EndDumpContext() {
}

void WebRtcRtpDumpWriter::FlushBuffer(bool incoming,
                                      bool end_stream,
                                      const FlushDoneCallback& callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  scoped_ptr<std::vector<uint8> > new_buffer(new std::vector<uint8>());

  if (incoming) {
    new_buffer->reserve(incoming_buffer_.capacity());
    new_buffer->swap(incoming_buffer_);
  } else {
    new_buffer->reserve(outgoing_buffer_.capacity());
    new_buffer->swap(outgoing_buffer_);
  }

  scoped_ptr<FlushResult> result(new FlushResult(FLUSH_RESULT_FAILURE));

  scoped_ptr<size_t> bytes_written(new size_t(0));

  FileThreadWorker* worker = incoming ? incoming_file_thread_worker_.get()
                                      : outgoing_file_thread_worker_.get();

  // Using "Unretained(worker)" because |worker| is owner by this object and it
  // guaranteed to be deleted on the FILE thread before this object goes away.
  base::Closure task =
      base::Bind(&FileThreadWorker::CompressAndWriteToFileOnFileThread,
                 base::Unretained(worker),
                 Passed(&new_buffer),
                 end_stream,
                 result.get(),
                 bytes_written.get());

  // OnFlushDone is necessary to avoid running the callback after this
  // object is gone.
  base::Closure reply = base::Bind(&WebRtcRtpDumpWriter::OnFlushDone,
                                   weak_ptr_factory_.GetWeakPtr(),
                                   callback,
                                   Passed(&result),
                                   Passed(&bytes_written));

  // Define the task and reply outside the method call so that getting and
  // passing the scoped_ptr does not depend on the argument evaluation order.
  BrowserThread::PostTaskAndReply(BrowserThread::FILE, FROM_HERE, task, reply);

  if (end_stream) {
    bool success = BrowserThread::DeleteSoon(
        BrowserThread::FILE,
        FROM_HERE,
        incoming ? incoming_file_thread_worker_.release()
                 : outgoing_file_thread_worker_.release());
    DCHECK(success);
  }
}

void WebRtcRtpDumpWriter::OnFlushDone(const FlushDoneCallback& callback,
                                      const scoped_ptr<FlushResult>& result,
                                      const scoped_ptr<size_t>& bytes_written) {
  DCHECK(thread_checker_.CalledOnValidThread());

  total_dump_size_on_disk_ += *bytes_written;

  if (total_dump_size_on_disk_ >= max_dump_size_ &&
      !max_dump_size_reached_callback_.is_null()) {
    max_dump_size_reached_callback_.Run();
  }

  // Returns success for FLUSH_RESULT_MAX_SIZE_REACHED since the dump is still
  // valid.
  if (!callback.is_null()) {
    callback.Run(*result != FLUSH_RESULT_FAILURE &&
                 *result != FLUSH_RESULT_NO_DATA);
  }
}

void WebRtcRtpDumpWriter::OnDumpEnded(EndDumpContext context,
                                      bool incoming,
                                      bool success) {
  DCHECK(thread_checker_.CalledOnValidThread());

  DVLOG(2) << "Dump ended, incoming = " << incoming
           << ", succeeded = " << success;

  if (incoming)
    context.incoming_succeeded = success;
  else
    context.outgoing_succeeded = success;

  // End the outgoing dump if needed.
  if (incoming && context.type == RTP_DUMP_BOTH) {
    FlushBuffer(false,
                true,
                base::Bind(&WebRtcRtpDumpWriter::OnDumpEnded,
                           weak_ptr_factory_.GetWeakPtr(),
                           context,
                           false));
    return;
  }

  context.callback.Run(context.incoming_succeeded, context.outgoing_succeeded);
}
