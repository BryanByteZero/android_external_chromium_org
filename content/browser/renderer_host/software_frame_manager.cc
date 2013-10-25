// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/software_frame_manager.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/sys_info.h"
#include "content/browser/renderer_host/dip_util.h"
#include "content/public/browser/user_metrics.h"

namespace {

void ReleaseMailbox(scoped_refptr<content::SoftwareFrame> frame,
                    unsigned sync_point,
                    bool lost_resource) {}

}  // namespace

namespace content {

////////////////////////////////////////////////////////////////////////////////
// SoftwareFrame

class CONTENT_EXPORT SoftwareFrame : public base::RefCounted<SoftwareFrame> {
 private:
  friend class base::RefCounted<SoftwareFrame>;
  friend class SoftwareFrameManager;

  SoftwareFrame(
    base::WeakPtr<SoftwareFrameManagerClient> frame_manager_client,
    uint32 output_surface_id,
    unsigned frame_id,
    gfx::Size frame_size_dip,
    gfx::Size frame_size_pixels,
    scoped_ptr<base::SharedMemory> shared_memory);
  ~SoftwareFrame();

  base::WeakPtr<SoftwareFrameManagerClient> frame_manager_client_;
  const uint32 output_surface_id_;
  const unsigned frame_id_;
  const gfx::Size frame_size_dip_;
  const gfx::Size frame_size_pixels_;
  scoped_ptr<base::SharedMemory> shared_memory_;

  DISALLOW_COPY_AND_ASSIGN(SoftwareFrame);
};

SoftwareFrame::SoftwareFrame(
  base::WeakPtr<SoftwareFrameManagerClient> frame_manager_client,
  uint32 output_surface_id,
  unsigned frame_id,
  gfx::Size frame_size_dip,
  gfx::Size frame_size_pixels,
  scoped_ptr<base::SharedMemory> shared_memory)
    : frame_manager_client_(frame_manager_client),
      output_surface_id_(output_surface_id),
      frame_id_(frame_id),
      frame_size_dip_(frame_size_dip),
      frame_size_pixels_(frame_size_pixels),
      shared_memory_(shared_memory.Pass()) {}

SoftwareFrame::~SoftwareFrame() {
  if (frame_manager_client_) {
    frame_manager_client_->SoftwareFrameWasFreed(
        output_surface_id_, frame_id_);
  }
}

////////////////////////////////////////////////////////////////////////////////
// SoftwareFrameManager

SoftwareFrameManager::SoftwareFrameManager(
    base::WeakPtr<SoftwareFrameManagerClient> client)
      : client_(client) {}

SoftwareFrameManager::~SoftwareFrameManager() {
  DiscardCurrentFrame();
}

bool SoftwareFrameManager::SwapToNewFrame(
    uint32 output_surface_id,
    const cc::SoftwareFrameData* frame_data,
    float frame_device_scale_factor,
    base::ProcessHandle process_handle) {

#ifdef OS_WIN
  scoped_ptr<base::SharedMemory> shared_memory(
      new base::SharedMemory(frame_data->handle, true,
                             process_handle));
#else
  scoped_ptr<base::SharedMemory> shared_memory(
      new base::SharedMemory(frame_data->handle, true));
#endif

  // The NULL handle is used in testing.
  if (base::SharedMemory::IsHandleValid(shared_memory->handle())) {
    const size_t size_in_bytes = 4 * frame_data->size.GetArea();
#ifdef OS_WIN
    if (!shared_memory->Map(0)) {
      DLOG(ERROR) << "Unable to map renderer memory.";
      RecordAction(UserMetricsAction(
          "BadMessageTerminate_SharedMemoryManager1"));
      return false;
    }

    if (shared_memory->mapped_size() < size_in_bytes) {
      DLOG(ERROR) << "Shared memory too small for given rectangle";
      RecordAction(UserMetricsAction(
          "BadMessageTerminate_SharedMemoryManager2"));
      return false;
    }
#else
    if (!shared_memory->Map(size_in_bytes)) {
      DLOG(ERROR) << "Unable to map renderer memory.";
      RecordAction(UserMetricsAction(
          "BadMessageTerminate_SharedMemoryManager1"));
      return false;
    }
#endif
  }

  scoped_refptr<SoftwareFrame> next_frame(new SoftwareFrame(
      client_,
      output_surface_id,
      frame_data->id,
      ConvertSizeToDIP(frame_device_scale_factor, frame_data->size),
      frame_data->size,
      shared_memory.Pass()));
  current_frame_.swap(next_frame);
  return true;
}

bool SoftwareFrameManager::HasCurrentFrame() const {
  return current_frame_.get() ? true : false;
}

void SoftwareFrameManager::DiscardCurrentFrame() {
  if (!HasCurrentFrame())
    return;
  current_frame_ = NULL;
  SoftwareFrameMemoryManager::GetInstance()->RemoveFrame(this);
}

void SoftwareFrameManager::SwapToNewFrameComplete(bool visible) {
  DCHECK(HasCurrentFrame());
  SoftwareFrameMemoryManager::GetInstance()->AddFrame(this, visible);
}

void SoftwareFrameManager::SetVisibility(bool visible) {
  if (HasCurrentFrame()) {
    SoftwareFrameMemoryManager::GetInstance()->SetFrameVisibility(this,
                                                                  visible);
  }
}

void SoftwareFrameManager::GetCurrentFrameMailbox(
    cc::TextureMailbox* mailbox,
    scoped_ptr<cc::SingleReleaseCallback>* callback) {
  DCHECK(HasCurrentFrame());
  *mailbox = cc::TextureMailbox(
      current_frame_->shared_memory_.get(), current_frame_->frame_size_pixels_);
  *callback = cc::SingleReleaseCallback::Create(
      base::Bind(ReleaseMailbox, current_frame_));
}

const void* SoftwareFrameManager::GetCurrentFramePixels() const {
  DCHECK(HasCurrentFrame());
  DCHECK(base::SharedMemory::IsHandleValid(
      current_frame_->shared_memory_->handle()));
  return current_frame_->shared_memory_->memory();
}

gfx::Size SoftwareFrameManager::GetCurrentFrameSizeInPixels() const {
  DCHECK(HasCurrentFrame());
  return current_frame_->frame_size_pixels_;
}

gfx::Size SoftwareFrameManager::GetCurrentFrameSizeInDIP() const {
  DCHECK(HasCurrentFrame());
  return current_frame_->frame_size_dip_;
}

void SoftwareFrameManager::EvictCurrentFrame() {
  DCHECK(HasCurrentFrame());
  DiscardCurrentFrame();
  if (client_)
    client_->ReleaseReferencesToSoftwareFrame();
}

////////////////////////////////////////////////////////////////////////////////
// SoftwareFrameMemoryManager

SoftwareFrameMemoryManager* SoftwareFrameMemoryManager::GetInstance() {
  return Singleton<SoftwareFrameMemoryManager>::get();
}

void SoftwareFrameMemoryManager::AddFrame(SoftwareFrameManager* frame,
                                          bool visible) {
  RemoveFrame(frame);
  if (visible)
    visible_frames_.insert(frame);
  else
    hidden_frames_.push_front(frame);
  CullHiddenFrames();
}

void SoftwareFrameMemoryManager::RemoveFrame(SoftwareFrameManager* frame) {
  visible_frames_.erase(frame);
  hidden_frames_.remove(frame);
}

void SoftwareFrameMemoryManager::SetFrameVisibility(SoftwareFrameManager* frame,
                                                    bool visible) {
  if (visible) {
    hidden_frames_.remove(frame);
    visible_frames_.insert(frame);
  } else {
    visible_frames_.erase(frame);
    hidden_frames_.push_front(frame);
    CullHiddenFrames();
  }
}

SoftwareFrameMemoryManager::SoftwareFrameMemoryManager()
  : max_number_of_saved_frames_(
        std::min(5, 2 + (base::SysInfo::AmountOfPhysicalMemoryMB() / 256))) {}

SoftwareFrameMemoryManager::~SoftwareFrameMemoryManager() {}

void SoftwareFrameMemoryManager::CullHiddenFrames() {
  while (!hidden_frames_.empty() &&
         hidden_frames_.size() + visible_frames_.size() >
             max_number_of_saved_frames()) {
    size_t old_size = hidden_frames_.size();
    // Should remove self from list.
    hidden_frames_.back()->EvictCurrentFrame();
    DCHECK_EQ(hidden_frames_.size() + 1, old_size);
  }
}

}  // namespace content
