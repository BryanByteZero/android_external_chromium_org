// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/vpx_video_decoder.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "base/string_number_conversions.h"
#include "base/sys_byteorder.h"
#include "media/base/bind_to_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"

namespace media {

// Always try to use three threads for video decoding.  There is little reason
// not to since current day CPUs tend to be multi-core and we measured
// performance benefits on older machines such as P4s with hyperthreading.
static const int kDecodeThreads = 2;
static const int kMaxDecodeThreads = 16;

// Returns the number of threads.
static int GetThreadCount() {
  // TODO(scherkus): De-duplicate this function and the one used by
  // FFmpegVideoDecoder.

  // Refer to http://crbug.com/93932 for tsan suppressions on decoding.
  int decode_threads = kDecodeThreads;

  const CommandLine* cmd_line = CommandLine::ForCurrentProcess();
  std::string threads(cmd_line->GetSwitchValueASCII(switches::kVideoThreads));
  if (threads.empty() || !base::StringToInt(threads, &decode_threads))
    return decode_threads;

  decode_threads = std::max(decode_threads, 0);
  decode_threads = std::min(decode_threads, kMaxDecodeThreads);
  return decode_threads;
}

VpxVideoDecoder::VpxVideoDecoder(
    const scoped_refptr<base::MessageLoopProxy>& message_loop)
    : message_loop_(message_loop),
      state_(kUninitialized) {
}

VpxVideoDecoder::~VpxVideoDecoder() {
  DCHECK_EQ(kUninitialized, state_);
}

void VpxVideoDecoder::Initialize(
    const scoped_refptr<DemuxerStream>& stream,
    const PipelineStatusCB& status_cb,
    const StatisticsCB& statistics_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(!demuxer_stream_) << "Already initialized.";

  if (!stream) {
    status_cb.Run(PIPELINE_ERROR_DECODE);
    return;
  }

  demuxer_stream_ = stream;
  statistics_cb_ = statistics_cb;

  if (!ConfigureDecoder()) {
    status_cb.Run(DECODER_ERROR_NOT_SUPPORTED);
    return;
  }

  // Success!
  state_ = kNormal;
  status_cb.Run(PIPELINE_OK);
}

static scoped_ptr<vpx_codec_ctx, VpxDeleter> InitializeVpxContext(
    scoped_ptr<vpx_codec_ctx, VpxDeleter> context,
    const VideoDecoderConfig& config) {
  context.reset(new vpx_codec_ctx());
  vpx_codec_dec_cfg_t vpx_config = {0};
  vpx_config.w = config.coded_size().width();
  vpx_config.h = config.coded_size().height();
  vpx_config.threads = GetThreadCount();

  vpx_codec_err_t status = vpx_codec_dec_init(context.get(),
                                              config.codec() == kCodecVP9 ?
                                                  vpx_codec_vp9_dx() :
                                                  vpx_codec_vp8_dx(),
                                              &vpx_config,
                                              0);
  if (status != VPX_CODEC_OK) {
    LOG(ERROR) << "vpx_codec_dec_init failed, status=" << status;
    context.reset();
  }
  return context.Pass();
}

bool VpxVideoDecoder::ConfigureDecoder() {
  const VideoDecoderConfig& config = demuxer_stream_->video_decoder_config();
  if (!config.IsValidConfig()) {
    DLOG(ERROR) << "Invalid video stream config: "
                << config.AsHumanReadableString();
    return false;
  }

  const CommandLine* cmd_line = CommandLine::ForCurrentProcess();
  bool can_handle = false;
  if (cmd_line->HasSwitch(switches::kEnableVp9Playback) &&
      config.codec() == kCodecVP9) {
    can_handle = true;
  }
  if (cmd_line->HasSwitch(switches::kEnableVp8AlphaPlayback) &&
      config.codec() == kCodecVP8 && config.format() == VideoFrame::YV12A) {
    can_handle = true;
  }
  if (!can_handle)
    return false;

  vpx_codec_ = InitializeVpxContext(vpx_codec_.Pass(), config);
  if (!vpx_codec_.get())
    return false;

  if (config.format() == VideoFrame::YV12A) {
    vpx_codec_alpha_ = InitializeVpxContext(vpx_codec_alpha_.Pass(), config);
    if (!vpx_codec_alpha_.get())
      return false;
  }

  return true;
}

void VpxVideoDecoder::Read(const ReadCB& read_cb) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(!read_cb.is_null());
  CHECK_NE(state_, kUninitialized);
  CHECK(read_cb_.is_null()) << "Overlapping decodes are not supported.";
  read_cb_ = BindToCurrentLoop(read_cb);

  // Return empty frames if decoding has finished.
  if (state_ == kDecodeFinished) {
    read_cb.Run(kOk, VideoFrame::CreateEmptyFrame());
    return;
  }

  ReadFromDemuxerStream();
}

void VpxVideoDecoder::Reset(const base::Closure& closure) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK(reset_cb_.is_null());
  reset_cb_ = BindToCurrentLoop(closure);

  // Defer the reset if a read is pending.
  if (!read_cb_.is_null())
    return;

  DoReset();
}

void VpxVideoDecoder::Stop(const base::Closure& closure) {
  DCHECK(message_loop_->BelongsToCurrentThread());

  if (state_ == kUninitialized) {
    closure.Run();
    return;
  }

  if (!read_cb_.is_null())
    base::ResetAndReturn(&read_cb_).Run(kOk, NULL);

  state_ = kUninitialized;
  closure.Run();
}

void VpxVideoDecoder::ReadFromDemuxerStream() {
  DCHECK_NE(state_, kUninitialized);
  DCHECK_NE(state_, kDecodeFinished);
  DCHECK(!read_cb_.is_null());

  demuxer_stream_->Read(base::Bind(
      &VpxVideoDecoder::DoDecryptOrDecodeBuffer, this));
}

void VpxVideoDecoder::DoDecryptOrDecodeBuffer(
    DemuxerStream::Status status,
    const scoped_refptr<DecoderBuffer>& buffer) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK_NE(state_, kDecodeFinished);
  DCHECK_EQ(status != DemuxerStream::kOk, !buffer) << status;

  if (state_ == kUninitialized)
    return;

  DCHECK(!read_cb_.is_null());

  if (!reset_cb_.is_null()) {
    base::ResetAndReturn(&read_cb_).Run(kOk, NULL);
    DoReset();
    return;
  }

  if (status == DemuxerStream::kAborted) {
    base::ResetAndReturn(&read_cb_).Run(kOk, NULL);
    return;
  }

  if (status == DemuxerStream::kConfigChanged) {
    if (!ConfigureDecoder()) {
      base::ResetAndReturn(&read_cb_).Run(kDecodeError, NULL);
      return;
    }

    ReadFromDemuxerStream();
    return;
  }

  DCHECK_EQ(status, DemuxerStream::kOk);
  DecodeBuffer(buffer);
}

void VpxVideoDecoder::DecodeBuffer(
    const scoped_refptr<DecoderBuffer>& buffer) {
  DCHECK(message_loop_->BelongsToCurrentThread());
  DCHECK_NE(state_, kUninitialized);
  DCHECK_NE(state_, kDecodeFinished);
  DCHECK(reset_cb_.is_null());
  DCHECK(!read_cb_.is_null());
  DCHECK(buffer);

  // Transition to kDecodeFinished on the first end of stream buffer.
  if (state_ == kNormal && buffer->IsEndOfStream()) {
    state_ = kDecodeFinished;
    base::ResetAndReturn(&read_cb_).Run(kOk, VideoFrame::CreateEmptyFrame());
    return;
  }

  scoped_refptr<VideoFrame> video_frame;
  if (!Decode(buffer, &video_frame)) {
    state_ = kDecodeFinished;
    base::ResetAndReturn(&read_cb_).Run(kDecodeError, NULL);
    return;
  }

  // Any successful decode counts!
  if (buffer->GetDataSize() && buffer->GetSideDataSize()) {
    PipelineStatistics statistics;
    statistics.video_bytes_decoded = buffer->GetDataSize();
    statistics_cb_.Run(statistics);
  }

  // If we didn't get a frame we need more data.
  if (!video_frame) {
    ReadFromDemuxerStream();
    return;
  }

  base::ResetAndReturn(&read_cb_).Run(kOk, video_frame);
}

bool VpxVideoDecoder::Decode(
    const scoped_refptr<DecoderBuffer>& buffer,
    scoped_refptr<VideoFrame>* video_frame) {
  DCHECK(video_frame);
  DCHECK(!buffer->IsEndOfStream());

  // Pass |buffer| to libvpx.
  int64 timestamp = buffer->GetTimestamp().InMicroseconds();
  void* user_priv = reinterpret_cast<void*>(&timestamp);
  vpx_codec_err_t status = vpx_codec_decode(vpx_codec_.get(),
                                            buffer->GetData(),
                                            buffer->GetDataSize(),
                                            user_priv,
                                            0);
  if (status != VPX_CODEC_OK) {
    LOG(ERROR) << "vpx_codec_decode() failed, status=" << status;
    return false;
  }

  // Gets pointer to decoded data.
  vpx_codec_iter_t iter = NULL;
  const vpx_image_t* vpx_image = vpx_codec_get_frame(vpx_codec_.get(), &iter);
  if (!vpx_image) {
    *video_frame = NULL;
    return true;
  }

  if (vpx_image->user_priv != reinterpret_cast<void*>(&timestamp)) {
    LOG(ERROR) << "Invalid output timestamp.";
    return false;
  }

  const vpx_image_t* vpx_image_alpha = NULL;
  if (vpx_codec_alpha_.get() && buffer->GetSideDataSize() >= 8) {
    // Pass alpha data to libvpx.
    int64 timestamp_alpha = buffer->GetTimestamp().InMicroseconds();
    void* user_priv_alpha = reinterpret_cast<void*>(&timestamp_alpha);

    // First 8 bytes of side data is side_data_id in big endian.
    const uint64 side_data_id = base::NetToHost64(
        *(reinterpret_cast<const uint64*>(buffer->GetSideData())));
    if (side_data_id == 1) {
      status = vpx_codec_decode(vpx_codec_alpha_.get(),
                                buffer->GetSideData() + 8,
                                buffer->GetSideDataSize() - 8,
                                user_priv_alpha,
                                0);

      if (status != VPX_CODEC_OK) {
        LOG(ERROR) << "vpx_codec_decode() failed on alpha, status=" << status;
        return false;
      }

      // Gets pointer to decoded data.
      vpx_codec_iter_t iter_alpha = NULL;
      vpx_image_alpha = vpx_codec_get_frame(vpx_codec_alpha_.get(),
                                            &iter_alpha);
      if (!vpx_image_alpha) {
        *video_frame = NULL;
        return true;
      }

      if (vpx_image_alpha->user_priv !=
          reinterpret_cast<void*>(&timestamp_alpha)) {
        LOG(ERROR) << "Invalid output timestamp on alpha.";
        return false;
      }
    }
  }

  CopyVpxImageTo(vpx_image, vpx_image_alpha, video_frame);
  (*video_frame)->SetTimestamp(base::TimeDelta::FromMicroseconds(timestamp));
  return true;
}

void VpxVideoDecoder::DoReset() {
  DCHECK(read_cb_.is_null());

  state_ = kNormal;
  reset_cb_.Run();
  reset_cb_.Reset();
}

void VpxVideoDecoder::CopyVpxImageTo(
    const struct vpx_image* vpx_image,
    const struct vpx_image* vpx_image_alpha,
    scoped_refptr<VideoFrame>* video_frame) {
  CHECK(vpx_image);
  CHECK_EQ(vpx_image->d_w % 2, 0U);
  CHECK_EQ(vpx_image->d_h % 2, 0U);
  CHECK(vpx_image->fmt == VPX_IMG_FMT_I420 ||
        vpx_image->fmt == VPX_IMG_FMT_YV12);

  gfx::Size size(vpx_image->d_w, vpx_image->d_h);
  gfx::Size natural_size =
      demuxer_stream_->video_decoder_config().natural_size();

  *video_frame = VideoFrame::CreateFrame(vpx_codec_alpha_.get() ?
                                             VideoFrame::YV12A :
                                             VideoFrame::YV12,
                                         size,
                                         gfx::Rect(size),
                                         natural_size,
                                         kNoTimestamp());

  CopyYPlane(vpx_image->planes[VPX_PLANE_Y],
             vpx_image->stride[VPX_PLANE_Y],
             vpx_image->d_h,
             *video_frame);
  CopyUPlane(vpx_image->planes[VPX_PLANE_U],
             vpx_image->stride[VPX_PLANE_U],
             vpx_image->d_h / 2,
             *video_frame);
  CopyVPlane(vpx_image->planes[VPX_PLANE_V],
             vpx_image->stride[VPX_PLANE_V],
             vpx_image->d_h / 2,
             *video_frame);
  if (!vpx_codec_alpha_.get())
    return;
  if (!vpx_image_alpha) {
    MakeOpaqueAPlane(vpx_image->stride[VPX_PLANE_Y], vpx_image->d_h,
                     *video_frame);
    return;
  }
  CopyAPlane(vpx_image_alpha->planes[VPX_PLANE_Y],
             vpx_image->stride[VPX_PLANE_Y],
             vpx_image->d_h,
             *video_frame);
}

}  // namespace media
