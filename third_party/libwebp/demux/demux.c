// Copyright 2012 Google Inc. All Rights Reserved.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
//  WebP container demux.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/utils.h"
#include "../webp/decode.h"     // WebPGetFeatures
#include "../webp/demux.h"
#include "../webp/format_constants.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define DMUX_MAJ_VERSION 0
#define DMUX_MIN_VERSION 1
#define DMUX_REV_VERSION 0

typedef struct {
  size_t start_;        // start location of the data
  size_t end_;          // end location
  size_t riff_end_;     // riff chunk end location, can be > end_.
  size_t buf_size_;     // size of the buffer
  const uint8_t* buf_;
} MemBuffer;

typedef struct {
  size_t offset_;
  size_t size_;
} ChunkData;

typedef struct Frame {
  int x_offset_, y_offset_;
  int width_, height_;
  int duration_;
  WebPMuxAnimDispose dispose_method_;
  int is_fragment_;  // this is a frame fragment (and not a full frame).
  int frame_num_;  // the referent frame number for use in assembling fragments.
  int complete_;   // img_components_ contains a full image.
  ChunkData img_components_[2];  // 0=VP8{,L} 1=ALPH
  struct Frame* next_;
} Frame;

typedef struct Chunk {
  ChunkData data_;
  struct Chunk* next_;
} Chunk;

struct WebPDemuxer {
  MemBuffer mem_;
  WebPDemuxState state_;
  int is_ext_format_;
  uint32_t feature_flags_;
  int canvas_width_, canvas_height_;
  int loop_count_;
  uint32_t bgcolor_;
  int num_frames_;
  Frame* frames_;
  Chunk* chunks_;  // non-image chunks
};

typedef enum {
  PARSE_OK,
  PARSE_NEED_MORE_DATA,
  PARSE_ERROR
} ParseStatus;

typedef struct ChunkParser {
  uint8_t id[4];
  ParseStatus (*parse)(WebPDemuxer* const dmux);
  int (*valid)(const WebPDemuxer* const dmux);
} ChunkParser;

static ParseStatus ParseSingleImage(WebPDemuxer* const dmux);
static ParseStatus ParseVP8X(WebPDemuxer* const dmux);
static int IsValidSimpleFormat(const WebPDemuxer* const dmux);
static int IsValidExtendedFormat(const WebPDemuxer* const dmux);

static const ChunkParser kMasterChunks[] = {
  { { 'V', 'P', '8', ' ' }, ParseSingleImage, IsValidSimpleFormat },
  { { 'V', 'P', '8', 'L' }, ParseSingleImage, IsValidSimpleFormat },
  { { 'V', 'P', '8', 'X' }, ParseVP8X,        IsValidExtendedFormat },
  { { '0', '0', '0', '0' }, NULL,             NULL },
};

//------------------------------------------------------------------------------

int WebPGetDemuxVersion(void) {
  return (DMUX_MAJ_VERSION << 16) | (DMUX_MIN_VERSION << 8) | DMUX_REV_VERSION;
}

// -----------------------------------------------------------------------------
// MemBuffer

static int RemapMemBuffer(MemBuffer* const mem,
                          const uint8_t* data, size_t size) {
  if (size < mem->buf_size_) return 0;  // can't remap to a shorter buffer!

  mem->buf_ = data;
  mem->end_ = mem->buf_size_ = size;
  return 1;
}

static int InitMemBuffer(MemBuffer* const mem,
                         const uint8_t* data, size_t size) {
  memset(mem, 0, sizeof(*mem));
  return RemapMemBuffer(mem, data, size);
}

// Return the remaining data size available in 'mem'.
static WEBP_INLINE size_t MemDataSize(const MemBuffer* const mem) {
  return (mem->end_ - mem->start_);
}

// Return true if 'size' exceeds the end of the RIFF chunk.
static WEBP_INLINE int SizeIsInvalid(const MemBuffer* const mem, size_t size) {
  return (size > mem->riff_end_ - mem->start_);
}

static WEBP_INLINE void Skip(MemBuffer* const mem, size_t size) {
  mem->start_ += size;
}

static WEBP_INLINE void Rewind(MemBuffer* const mem, size_t size) {
  mem->start_ -= size;
}

static WEBP_INLINE const uint8_t* GetBuffer(MemBuffer* const mem) {
  return mem->buf_ + mem->start_;
}

// Read from 'mem' and skip the read bytes.
static WEBP_INLINE uint8_t ReadByte(MemBuffer* const mem) {
  const uint8_t byte = mem->buf_[mem->start_];
  Skip(mem, 1);
  return byte;
}

static WEBP_INLINE int ReadLE16s(MemBuffer* const mem) {
  const uint8_t* const data = mem->buf_ + mem->start_;
  const int val = GetLE16(data);
  Skip(mem, 2);
  return val;
}

static WEBP_INLINE int ReadLE24s(MemBuffer* const mem) {
  const uint8_t* const data = mem->buf_ + mem->start_;
  const int val = GetLE24(data);
  Skip(mem, 3);
  return val;
}

static WEBP_INLINE uint32_t ReadLE32(MemBuffer* const mem) {
  const uint8_t* const data = mem->buf_ + mem->start_;
  const uint32_t val = GetLE32(data);
  Skip(mem, 4);
  return val;
}

// -----------------------------------------------------------------------------
// Secondary chunk parsing

static void AddChunk(WebPDemuxer* const dmux, Chunk* const chunk) {
  Chunk** c = &dmux->chunks_;
  while (*c != NULL) c = &(*c)->next_;
  *c = chunk;
  chunk->next_ = NULL;
}

// Add a frame to the end of the list, ensuring the last frame is complete.
// Returns true on success, false otherwise.
static int AddFrame(WebPDemuxer* const dmux, Frame* const frame) {
  const Frame* last_frame = NULL;
  Frame** f = &dmux->frames_;
  while (*f != NULL) {
    last_frame = *f;
    f = &(*f)->next_;
  }
  if (last_frame != NULL && !last_frame->complete_) return 0;
  *f = frame;
  frame->next_ = NULL;
  return 1;
}

// Store image bearing chunks to 'frame'.
// If 'has_vp8l_alpha' is not NULL, it will be set to true if the frame is a
// lossless image with alpha.
static ParseStatus StoreFrame(int frame_num, uint32_t min_size,
                              MemBuffer* const mem, Frame* const frame,
                              int* const has_vp8l_alpha) {
  int alpha_chunks = 0;
  int image_chunks = 0;
  int done = (MemDataSize(mem) < min_size);
  ParseStatus status = PARSE_OK;

  if (has_vp8l_alpha != NULL) *has_vp8l_alpha = 0;  // Default.

  if (done) return PARSE_NEED_MORE_DATA;

  do {
    const size_t chunk_start_offset = mem->start_;
    const uint32_t fourcc = ReadLE32(mem);
    const uint32_t payload_size = ReadLE32(mem);
    const uint32_t payload_size_padded = payload_size + (payload_size & 1);
    const size_t payload_available = (payload_size_padded > MemDataSize(mem))
                                   ? MemDataSize(mem) : payload_size_padded;
    const size_t chunk_size = CHUNK_HEADER_SIZE + payload_available;

    if (payload_size > MAX_CHUNK_PAYLOAD) return PARSE_ERROR;
    if (SizeIsInvalid(mem, payload_size_padded)) return PARSE_ERROR;
    if (payload_size_padded > MemDataSize(mem)) status = PARSE_NEED_MORE_DATA;

    switch (fourcc) {
      case MKFOURCC('A', 'L', 'P', 'H'):
        if (alpha_chunks == 0) {
          ++alpha_chunks;
          frame->img_components_[1].offset_ = chunk_start_offset;
          frame->img_components_[1].size_ = chunk_size;
          frame->frame_num_ = frame_num;
          Skip(mem, payload_available);
        } else {
          goto Done;
        }
        break;
      case MKFOURCC('V', 'P', '8', 'L'):
        if (alpha_chunks > 0) return PARSE_ERROR;  // VP8L has its own alpha
        // fall through
      case MKFOURCC('V', 'P', '8', ' '):
        if (image_chunks == 0) {
          // Extract the bitstream features, tolerating failures when the data
          // is incomplete.
          WebPBitstreamFeatures features;
          const VP8StatusCode vp8_status =
              WebPGetFeatures(mem->buf_ + chunk_start_offset, chunk_size,
                              &features);
          if (status == PARSE_NEED_MORE_DATA &&
              vp8_status == VP8_STATUS_NOT_ENOUGH_DATA) {
            return PARSE_NEED_MORE_DATA;
          } else if (vp8_status != VP8_STATUS_OK) {
            // We have enough data, and yet WebPGetFeatures() failed.
            return PARSE_ERROR;
          }
          ++image_chunks;
          frame->img_components_[0].offset_ = chunk_start_offset;
          frame->img_components_[0].size_ = chunk_size;
          frame->width_ = features.width;
          frame->height_ = features.height;
          if (has_vp8l_alpha != NULL) *has_vp8l_alpha = features.has_alpha;
          frame->frame_num_ = frame_num;
          frame->complete_ = (status == PARSE_OK);
          Skip(mem, payload_available);
        } else {
          goto Done;
        }
        break;
 Done:
      default:
        // Restore fourcc/size when moving up one level in parsing.
        Rewind(mem, CHUNK_HEADER_SIZE);
        done = 1;
        break;
    }

    if (mem->start_ == mem->riff_end_) {
      done = 1;
    } else if (MemDataSize(mem) < CHUNK_HEADER_SIZE) {
      status = PARSE_NEED_MORE_DATA;
    }
  } while (!done && status == PARSE_OK);

  return status;
}

// Creates a new Frame if 'actual_size' is within bounds and 'mem' contains
// enough data ('min_size') to parse the payload.
// Returns PARSE_OK on success with *frame pointing to the new Frame.
// Returns PARSE_NEED_MORE_DATA with insufficient data, PARSE_ERROR otherwise.
static ParseStatus NewFrame(const MemBuffer* const mem,
                            uint32_t min_size, uint32_t actual_size,
                            Frame** frame) {
  if (SizeIsInvalid(mem, min_size)) return PARSE_ERROR;
  if (actual_size < min_size) return PARSE_ERROR;
  if (MemDataSize(mem) < min_size)  return PARSE_NEED_MORE_DATA;

  *frame = (Frame*)calloc(1, sizeof(**frame));
  return (*frame == NULL) ? PARSE_ERROR : PARSE_OK;
}

// Parse a 'ANMF' chunk and any image bearing chunks that immediately follow.
// 'frame_chunk_size' is the previously validated, padded chunk size.
static ParseStatus ParseAnimationFrame(
    WebPDemuxer* const dmux, uint32_t frame_chunk_size) {
  const int has_frames = !!(dmux->feature_flags_ & ANIMATION_FLAG);
  const uint32_t anmf_payload_size = frame_chunk_size - ANMF_CHUNK_SIZE;
  int added_frame = 0;
  MemBuffer* const mem = &dmux->mem_;
  Frame* frame;
  ParseStatus status =
      NewFrame(mem, ANMF_CHUNK_SIZE, frame_chunk_size, &frame);
  if (status != PARSE_OK) return status;

  frame->x_offset_       = 2 * ReadLE24s(mem);
  frame->y_offset_       = 2 * ReadLE24s(mem);
  frame->width_          = 1 + ReadLE24s(mem);
  frame->height_         = 1 + ReadLE24s(mem);
  frame->duration_       = ReadLE24s(mem);
  frame->dispose_method_ = (WebPMuxAnimDispose)(ReadByte(mem) & 1);
  if (frame->width_ * (uint64_t)frame->height_ >= MAX_IMAGE_AREA) {
    return PARSE_ERROR;
  }

  // Store a frame only if the animation flag is set there is some data for
  // this frame is available.
  status = StoreFrame(dmux->num_frames_ + 1, anmf_payload_size, mem, frame,
                      NULL);
  if (status != PARSE_ERROR && has_frames && frame->frame_num_ > 0) {
    added_frame = AddFrame(dmux, frame);
    if (added_frame) {
      ++dmux->num_frames_;
    } else {
      status = PARSE_ERROR;
    }
  }

  if (!added_frame) free(frame);
  return status;
}

#ifdef WEBP_EXPERIMENTAL_FEATURES
// Parse a 'FRGM' chunk and any image bearing chunks that immediately follow.
// 'fragment_chunk_size' is the previously validated, padded chunk size.
static ParseStatus ParseFragment(WebPDemuxer* const dmux,
                                 uint32_t fragment_chunk_size) {
  const int frame_num = 1;  // All fragments belong to the 1st (and only) frame.
  const int has_fragments = !!(dmux->feature_flags_ & FRAGMENTS_FLAG);
  const uint32_t frgm_payload_size = fragment_chunk_size - FRGM_CHUNK_SIZE;
  int added_fragment = 0;
  MemBuffer* const mem = &dmux->mem_;
  Frame* frame;
  ParseStatus status =
      NewFrame(mem, FRGM_CHUNK_SIZE, fragment_chunk_size, &frame);
  if (status != PARSE_OK) return status;

  frame->is_fragment_ = 1;
  frame->x_offset_ = 2 * ReadLE24s(mem);
  frame->y_offset_ = 2 * ReadLE24s(mem);

  // Store a fragment only if the fragments flag is set there is some data for
  // this fragment is available.
  status = StoreFrame(frame_num, frgm_payload_size, mem, frame, NULL);
  if (status != PARSE_ERROR && has_fragments && frame->frame_num_ > 0) {
    added_fragment = AddFrame(dmux, frame);
    if (!added_fragment) {
      status = PARSE_ERROR;
    } else {
      dmux->num_frames_ = 1;
    }
  }

  if (!added_fragment) free(frame);
  return status;
}
#endif  // WEBP_EXPERIMENTAL_FEATURES

// General chunk storage, starting with the header at 'start_offset', allowing
// the user to request the payload via a fourcc string. 'size' includes the
// header and the unpadded payload size.
// Returns true on success, false otherwise.
static int StoreChunk(WebPDemuxer* const dmux,
                      size_t start_offset, uint32_t size) {
  Chunk* const chunk = (Chunk*)calloc(1, sizeof(*chunk));
  if (chunk == NULL) return 0;

  chunk->data_.offset_ = start_offset;
  chunk->data_.size_ = size;
  AddChunk(dmux, chunk);
  return 1;
}

// -----------------------------------------------------------------------------
// Primary chunk parsing

static int ReadHeader(MemBuffer* const mem) {
  const size_t min_size = RIFF_HEADER_SIZE + CHUNK_HEADER_SIZE;
  uint32_t riff_size;

  // Basic file level validation.
  if (MemDataSize(mem) < min_size) return 0;
  if (memcmp(GetBuffer(mem), "RIFF", CHUNK_SIZE_BYTES) ||
      memcmp(GetBuffer(mem) + CHUNK_HEADER_SIZE, "WEBP", CHUNK_SIZE_BYTES)) {
    return 0;
  }

  riff_size = GetLE32(GetBuffer(mem) + TAG_SIZE);
  if (riff_size < CHUNK_HEADER_SIZE) return 0;
  if (riff_size > MAX_CHUNK_PAYLOAD) return 0;

  // There's no point in reading past the end of the RIFF chunk
  mem->riff_end_ = riff_size + CHUNK_HEADER_SIZE;
  if (mem->buf_size_ > mem->riff_end_) {
    mem->buf_size_ = mem->end_ = mem->riff_end_;
  }

  Skip(mem, RIFF_HEADER_SIZE);
  return 1;
}

static ParseStatus ParseSingleImage(WebPDemuxer* const dmux) {
  const size_t min_size = CHUNK_HEADER_SIZE;
  MemBuffer* const mem = &dmux->mem_;
  Frame* frame;
  ParseStatus status;
  int has_vp8l_alpha = 0;  // Frame contains a lossless image with alpha.

  if (dmux->frames_ != NULL) return PARSE_ERROR;
  if (SizeIsInvalid(mem, min_size)) return PARSE_ERROR;
  if (MemDataSize(mem) < min_size) return PARSE_NEED_MORE_DATA;

  frame = (Frame*)calloc(1, sizeof(*frame));
  if (frame == NULL) return PARSE_ERROR;

  // For the single image case we allow parsing of a partial frame, but we need
  // at least CHUNK_HEADER_SIZE for parsing.
  status = StoreFrame(1, CHUNK_HEADER_SIZE, &dmux->mem_, frame,
                      &has_vp8l_alpha);
  if (status != PARSE_ERROR) {
    const int has_alpha = !!(dmux->feature_flags_ & ALPHA_FLAG);
    // Clear any alpha when the alpha flag is missing.
    if (!has_alpha && frame->img_components_[1].size_ > 0) {
      frame->img_components_[1].offset_ = 0;
      frame->img_components_[1].size_ = 0;
    }

    // Use the frame width/height as the canvas values for non-vp8x files.
    // Also, set ALPHA_FLAG if this is a lossless image with alpha.
    if (!dmux->is_ext_format_ && frame->width_ > 0 && frame->height_ > 0) {
      dmux->state_ = WEBP_DEMUX_PARSED_HEADER;
      dmux->canvas_width_ = frame->width_;
      dmux->canvas_height_ = frame->height_;
      dmux->feature_flags_ |= has_vp8l_alpha ? ALPHA_FLAG : 0;
    }
    AddFrame(dmux, frame);
    dmux->num_frames_ = 1;
  } else {
    free(frame);
  }

  return status;
}

static ParseStatus ParseVP8X(WebPDemuxer* const dmux) {
  MemBuffer* const mem = &dmux->mem_;
  int anim_chunks = 0;
  uint32_t vp8x_size;
  ParseStatus status = PARSE_OK;

  if (MemDataSize(mem) < CHUNK_HEADER_SIZE) return PARSE_NEED_MORE_DATA;

  dmux->is_ext_format_ = 1;
  Skip(mem, TAG_SIZE);  // VP8X
  vp8x_size = ReadLE32(mem);
  if (vp8x_size > MAX_CHUNK_PAYLOAD) return PARSE_ERROR;
  if (vp8x_size < VP8X_CHUNK_SIZE) return PARSE_ERROR;
  vp8x_size += vp8x_size & 1;
  if (SizeIsInvalid(mem, vp8x_size)) return PARSE_ERROR;
  if (MemDataSize(mem) < vp8x_size) return PARSE_NEED_MORE_DATA;

  dmux->feature_flags_ = ReadByte(mem);
  Skip(mem, 3);  // Reserved.
  dmux->canvas_width_  = 1 + ReadLE24s(mem);
  dmux->canvas_height_ = 1 + ReadLE24s(mem);
  if (dmux->canvas_width_ * (uint64_t)dmux->canvas_height_ >= MAX_IMAGE_AREA) {
    return PARSE_ERROR;  // image final dimension is too large
  }
  Skip(mem, vp8x_size - VP8X_CHUNK_SIZE);  // skip any trailing data.
  dmux->state_ = WEBP_DEMUX_PARSED_HEADER;

  if (SizeIsInvalid(mem, CHUNK_HEADER_SIZE)) return PARSE_ERROR;
  if (MemDataSize(mem) < CHUNK_HEADER_SIZE) return PARSE_NEED_MORE_DATA;

  do {
    int store_chunk = 1;
    const size_t chunk_start_offset = mem->start_;
    const uint32_t fourcc = ReadLE32(mem);
    const uint32_t chunk_size = ReadLE32(mem);
    const uint32_t chunk_size_padded = chunk_size + (chunk_size & 1);

    if (chunk_size > MAX_CHUNK_PAYLOAD) return PARSE_ERROR;
    if (SizeIsInvalid(mem, chunk_size_padded)) return PARSE_ERROR;

    switch (fourcc) {
      case MKFOURCC('V', 'P', '8', 'X'): {
        return PARSE_ERROR;
      }
      case MKFOURCC('A', 'L', 'P', 'H'):
      case MKFOURCC('V', 'P', '8', ' '):
      case MKFOURCC('V', 'P', '8', 'L'): {
        // check that this isn't an animation (all frames should be in an ANMF).
        if (anim_chunks > 0) return PARSE_ERROR;

        Rewind(mem, CHUNK_HEADER_SIZE);
        status = ParseSingleImage(dmux);
        break;
      }
      case MKFOURCC('A', 'N', 'I', 'M'): {
        if (chunk_size_padded < ANIM_CHUNK_SIZE) return PARSE_ERROR;

        if (MemDataSize(mem) < chunk_size_padded) {
          status = PARSE_NEED_MORE_DATA;
        } else if (anim_chunks == 0) {
          ++anim_chunks;
          dmux->bgcolor_ = ReadLE32(mem);
          dmux->loop_count_ = ReadLE16s(mem);
          Skip(mem, chunk_size_padded - ANIM_CHUNK_SIZE);
        } else {
          store_chunk = 0;
          goto Skip;
        }
        break;
      }
      case MKFOURCC('A', 'N', 'M', 'F'): {
        if (anim_chunks == 0) return PARSE_ERROR;  // 'ANIM' precedes frames.
        status = ParseAnimationFrame(dmux, chunk_size_padded);
        break;
      }
#ifdef WEBP_EXPERIMENTAL_FEATURES
      case MKFOURCC('F', 'R', 'G', 'M'): {
        status = ParseFragment(dmux, chunk_size_padded);
        break;
      }
#endif
      case MKFOURCC('I', 'C', 'C', 'P'): {
        store_chunk = !!(dmux->feature_flags_ & ICCP_FLAG);
        goto Skip;
      }
      case MKFOURCC('X', 'M', 'P', ' '): {
        store_chunk = !!(dmux->feature_flags_ & XMP_FLAG);
        goto Skip;
      }
      case MKFOURCC('E', 'X', 'I', 'F'): {
        store_chunk = !!(dmux->feature_flags_ & EXIF_FLAG);
        goto Skip;
      }
 Skip:
      default: {
        if (chunk_size_padded <= MemDataSize(mem)) {
          if (store_chunk) {
            // Store only the chunk header and unpadded size as only the payload
            // will be returned to the user.
            if (!StoreChunk(dmux, chunk_start_offset,
                            CHUNK_HEADER_SIZE + chunk_size)) {
              return PARSE_ERROR;
            }
          }
          Skip(mem, chunk_size_padded);
        } else {
          status = PARSE_NEED_MORE_DATA;
        }
      }
    }

    if (mem->start_ == mem->riff_end_) {
      break;
    } else if (MemDataSize(mem) < CHUNK_HEADER_SIZE) {
      status = PARSE_NEED_MORE_DATA;
    }
  } while (status == PARSE_OK);

  return status;
}

// -----------------------------------------------------------------------------
// Format validation

static int IsValidSimpleFormat(const WebPDemuxer* const dmux) {
  const Frame* const frame = dmux->frames_;
  if (dmux->state_ == WEBP_DEMUX_PARSING_HEADER) return 1;

  if (dmux->canvas_width_ <= 0 || dmux->canvas_height_ <= 0) return 0;
  if (dmux->state_ == WEBP_DEMUX_DONE && frame == NULL) return 0;

  if (frame->width_ <= 0 || frame->height_ <= 0) return 0;
  return 1;
}

static int IsValidExtendedFormat(const WebPDemuxer* const dmux) {
  const int has_fragments = !!(dmux->feature_flags_ & FRAGMENTS_FLAG);
  const int has_frames = !!(dmux->feature_flags_ & ANIMATION_FLAG);
  const Frame* f;

  if (dmux->state_ == WEBP_DEMUX_PARSING_HEADER) return 1;

  if (dmux->canvas_width_ <= 0 || dmux->canvas_height_ <= 0) return 0;
  if (dmux->loop_count_ < 0) return 0;
  if (dmux->state_ == WEBP_DEMUX_DONE && dmux->frames_ == NULL) return 0;

  for (f = dmux->frames_; f != NULL; f = f->next_) {
    const int cur_frame_set = f->frame_num_;
    int frame_count = 0, fragment_count = 0;

    // Check frame properties and if the image is composed of fragments that
    // each fragment came from a fragment.
    for (; f != NULL && f->frame_num_ == cur_frame_set; f = f->next_) {
      const ChunkData* const image = f->img_components_;
      const ChunkData* const alpha = f->img_components_ + 1;

      if (!has_fragments && f->is_fragment_) return 0;
      if (!has_frames && f->frame_num_ > 1) return 0;
      if (f->x_offset_ < 0 || f->y_offset_ < 0) return 0;
      if (f->complete_) {
        if (alpha->size_ == 0 && image->size_ == 0) return 0;
        // Ensure alpha precedes image bitstream.
        if (alpha->size_ > 0 && alpha->offset_ > image->offset_) {
          return 0;
        }

        if (f->width_ <= 0 || f->height_ <= 0) return 0;
      } else {
        // There shouldn't be a partial frame in a complete file.
        if (dmux->state_ == WEBP_DEMUX_DONE) return 0;

        // Ensure alpha precedes image bitstream.
        if (alpha->size_ > 0 && image->size_ > 0 &&
            alpha->offset_ > image->offset_) {
          return 0;
        }
        // There shouldn't be any frames after an incomplete one.
        if (f->next_ != NULL) return 0;
      }

      fragment_count += f->is_fragment_;
      ++frame_count;
    }
    if (!has_fragments && frame_count > 1) return 0;
    if (fragment_count > 0 && frame_count != fragment_count) return 0;
    if (f == NULL) break;
  }
  return 1;
}

// -----------------------------------------------------------------------------
// WebPDemuxer object

static void InitDemux(WebPDemuxer* const dmux, const MemBuffer* const mem) {
  dmux->state_ = WEBP_DEMUX_PARSING_HEADER;
  dmux->loop_count_ = 1;
  dmux->bgcolor_ = 0xFFFFFFFF;  // White background by default.
  dmux->canvas_width_ = -1;
  dmux->canvas_height_ = -1;
  dmux->mem_ = *mem;
}

WebPDemuxer* WebPDemuxInternal(const WebPData* data, int allow_partial,
                               WebPDemuxState* state, int version) {
  const ChunkParser* parser;
  int partial;
  ParseStatus status = PARSE_ERROR;
  MemBuffer mem;
  WebPDemuxer* dmux;

  if (WEBP_ABI_IS_INCOMPATIBLE(version, WEBP_DEMUX_ABI_VERSION)) return NULL;
  if (data == NULL || data->bytes == NULL || data->size == 0) return NULL;

  if (!InitMemBuffer(&mem, data->bytes, data->size)) return NULL;
  if (!ReadHeader(&mem)) return NULL;

  partial = (mem.buf_size_ < mem.riff_end_);
  if (!allow_partial && partial) return NULL;

  dmux = (WebPDemuxer*)calloc(1, sizeof(*dmux));
  if (dmux == NULL) return NULL;
  InitDemux(dmux, &mem);

  for (parser = kMasterChunks; parser->parse != NULL; ++parser) {
    if (!memcmp(parser->id, GetBuffer(&dmux->mem_), TAG_SIZE)) {
      status = parser->parse(dmux);
      if (status == PARSE_OK) dmux->state_ = WEBP_DEMUX_DONE;
      if (status == PARSE_NEED_MORE_DATA && !partial) status = PARSE_ERROR;
      if (status != PARSE_ERROR && !parser->valid(dmux)) status = PARSE_ERROR;
      break;
    }
  }
  if (state) *state = dmux->state_;

  if (status == PARSE_ERROR) {
    WebPDemuxDelete(dmux);
    return NULL;
  }
  return dmux;
}

void WebPDemuxDelete(WebPDemuxer* dmux) {
  Chunk* c;
  Frame* f;
  if (dmux == NULL) return;

  for (f = dmux->frames_; f != NULL;) {
    Frame* const cur_frame = f;
    f = f->next_;
    free(cur_frame);
  }
  for (c = dmux->chunks_; c != NULL;) {
    Chunk* const cur_chunk = c;
    c = c->next_;
    free(cur_chunk);
  }
  free(dmux);
}

// -----------------------------------------------------------------------------

uint32_t WebPDemuxGetI(const WebPDemuxer* dmux, WebPFormatFeature feature) {
  if (dmux == NULL) return 0;

  switch (feature) {
    case WEBP_FF_FORMAT_FLAGS:     return dmux->feature_flags_;
    case WEBP_FF_CANVAS_WIDTH:     return (uint32_t)dmux->canvas_width_;
    case WEBP_FF_CANVAS_HEIGHT:    return (uint32_t)dmux->canvas_height_;
    case WEBP_FF_LOOP_COUNT:       return (uint32_t)dmux->loop_count_;
    case WEBP_FF_BACKGROUND_COLOR: return dmux->bgcolor_;
    case WEBP_FF_FRAME_COUNT:      return (uint32_t)dmux->num_frames_;
  }
  return 0;
}

// -----------------------------------------------------------------------------
// Frame iteration

// Find the first 'frame_num' frame. There may be multiple such frames in a
// fragmented frame.
static const Frame* GetFrame(const WebPDemuxer* const dmux, int frame_num) {
  const Frame* f;
  for (f = dmux->frames_; f != NULL; f = f->next_) {
    if (frame_num == f->frame_num_) break;
  }
  return f;
}

// Returns fragment 'fragment_num' and the total count.
static const Frame* GetFragment(
    const Frame* const frame_set, int fragment_num, int* const count) {
  const int this_frame = frame_set->frame_num_;
  const Frame* f = frame_set;
  const Frame* fragment = NULL;
  int total;

  for (total = 0; f != NULL && f->frame_num_ == this_frame; f = f->next_) {
    if (++total == fragment_num) fragment = f;
  }
  *count = total;
  return fragment;
}

static const uint8_t* GetFramePayload(const uint8_t* const mem_buf,
                                      const Frame* const frame,
                                      size_t* const data_size) {
  *data_size = 0;
  if (frame != NULL) {
    const ChunkData* const image = frame->img_components_;
    const ChunkData* const alpha = frame->img_components_ + 1;
    size_t start_offset = image->offset_;
    *data_size = image->size_;

    // if alpha exists it precedes image, update the size allowing for
    // intervening chunks.
    if (alpha->size_ > 0) {
      const size_t inter_size = (image->offset_ > 0)
                              ? image->offset_ - (alpha->offset_ + alpha->size_)
                              : 0;
      start_offset = alpha->offset_;
      *data_size  += alpha->size_ + inter_size;
    }
    return mem_buf + start_offset;
  }
  return NULL;
}

// Create a whole 'frame' from VP8 (+ alpha) or lossless.
static int SynthesizeFrame(const WebPDemuxer* const dmux,
                           const Frame* const first_frame,
                           int fragment_num, WebPIterator* const iter) {
  const uint8_t* const mem_buf = dmux->mem_.buf_;
  int num_fragments;
  size_t payload_size = 0;
  const Frame* const fragment =
      GetFragment(first_frame, fragment_num, &num_fragments);
  const uint8_t* const payload =
      GetFramePayload(mem_buf, fragment, &payload_size);
  if (payload == NULL) return 0;
  assert(first_frame != NULL);

  iter->frame_num      = first_frame->frame_num_;
  iter->num_frames     = dmux->num_frames_;
  iter->fragment_num   = fragment_num;
  iter->num_fragments  = num_fragments;
  iter->x_offset       = fragment->x_offset_;
  iter->y_offset       = fragment->y_offset_;
  iter->width          = fragment->width_;
  iter->height         = fragment->height_;
  iter->duration       = fragment->duration_;
  iter->dispose_method = fragment->dispose_method_;
  iter->complete       = fragment->complete_;
  iter->fragment.bytes = payload;
  iter->fragment.size  = payload_size;
  // TODO(jzern): adjust offsets for 'FRGM's embedded in 'ANMF's
  return 1;
}

static int SetFrame(int frame_num, WebPIterator* const iter) {
  const Frame* frame;
  const WebPDemuxer* const dmux = (WebPDemuxer*)iter->private_;
  if (dmux == NULL || frame_num < 0) return 0;
  if (frame_num > dmux->num_frames_) return 0;
  if (frame_num == 0) frame_num = dmux->num_frames_;

  frame = GetFrame(dmux, frame_num);
  if (frame == NULL) return 0;

  return SynthesizeFrame(dmux, frame, 1, iter);
}

int WebPDemuxGetFrame(const WebPDemuxer* dmux, int frame, WebPIterator* iter) {
  if (iter == NULL) return 0;

  memset(iter, 0, sizeof(*iter));
  iter->private_ = (void*)dmux;
  return SetFrame(frame, iter);
}

int WebPDemuxNextFrame(WebPIterator* iter) {
  if (iter == NULL) return 0;
  return SetFrame(iter->frame_num + 1, iter);
}

int WebPDemuxPrevFrame(WebPIterator* iter) {
  if (iter == NULL) return 0;
  if (iter->frame_num <= 1) return 0;
  return SetFrame(iter->frame_num - 1, iter);
}

int WebPDemuxSelectFragment(WebPIterator* iter, int fragment_num) {
  if (iter != NULL && iter->private_ != NULL && fragment_num > 0) {
    const WebPDemuxer* const dmux = (WebPDemuxer*)iter->private_;
    const Frame* const frame = GetFrame(dmux, iter->frame_num);
    if (frame == NULL) return 0;

    return SynthesizeFrame(dmux, frame, fragment_num, iter);
  }
  return 0;
}

void WebPDemuxReleaseIterator(WebPIterator* iter) {
  (void)iter;
}

// -----------------------------------------------------------------------------
// Chunk iteration

static int ChunkCount(const WebPDemuxer* const dmux, const char fourcc[4]) {
  const uint8_t* const mem_buf = dmux->mem_.buf_;
  const Chunk* c;
  int count = 0;
  for (c = dmux->chunks_; c != NULL; c = c->next_) {
    const uint8_t* const header = mem_buf + c->data_.offset_;
    if (!memcmp(header, fourcc, TAG_SIZE)) ++count;
  }
  return count;
}

static const Chunk* GetChunk(const WebPDemuxer* const dmux,
                             const char fourcc[4], int chunk_num) {
  const uint8_t* const mem_buf = dmux->mem_.buf_;
  const Chunk* c;
  int count = 0;
  for (c = dmux->chunks_; c != NULL; c = c->next_) {
    const uint8_t* const header = mem_buf + c->data_.offset_;
    if (!memcmp(header, fourcc, TAG_SIZE)) ++count;
    if (count == chunk_num) break;
  }
  return c;
}

static int SetChunk(const char fourcc[4], int chunk_num,
                    WebPChunkIterator* const iter) {
  const WebPDemuxer* const dmux = (WebPDemuxer*)iter->private_;
  int count;

  if (dmux == NULL || fourcc == NULL || chunk_num < 0) return 0;
  count = ChunkCount(dmux, fourcc);
  if (count == 0) return 0;
  if (chunk_num == 0) chunk_num = count;

  if (chunk_num <= count) {
    const uint8_t* const mem_buf = dmux->mem_.buf_;
    const Chunk* const chunk = GetChunk(dmux, fourcc, chunk_num);
    iter->chunk.bytes = mem_buf + chunk->data_.offset_ + CHUNK_HEADER_SIZE;
    iter->chunk.size  = chunk->data_.size_ - CHUNK_HEADER_SIZE;
    iter->num_chunks  = count;
    iter->chunk_num   = chunk_num;
    return 1;
  }
  return 0;
}

int WebPDemuxGetChunk(const WebPDemuxer* dmux,
                      const char fourcc[4], int chunk_num,
                      WebPChunkIterator* iter) {
  if (iter == NULL) return 0;

  memset(iter, 0, sizeof(*iter));
  iter->private_ = (void*)dmux;
  return SetChunk(fourcc, chunk_num, iter);
}

int WebPDemuxNextChunk(WebPChunkIterator* iter) {
  if (iter != NULL) {
    const char* const fourcc =
        (const char*)iter->chunk.bytes - CHUNK_HEADER_SIZE;
    return SetChunk(fourcc, iter->chunk_num + 1, iter);
  }
  return 0;
}

int WebPDemuxPrevChunk(WebPChunkIterator* iter) {
  if (iter != NULL && iter->chunk_num > 1) {
    const char* const fourcc =
        (const char*)iter->chunk.bytes - CHUNK_HEADER_SIZE;
    return SetChunk(fourcc, iter->chunk_num - 1, iter);
  }
  return 0;
}

void WebPDemuxReleaseChunkIterator(WebPChunkIterator* iter) {
  (void)iter;
}

#if defined(__cplusplus) || defined(c_plusplus)
}  // extern "C"
#endif
