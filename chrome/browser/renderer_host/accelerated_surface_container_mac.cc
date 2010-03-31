// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/accelerated_surface_container_mac.h"

#include "app/surface/io_surface_support_mac.h"
#include "base/logging.h"
#include "chrome/browser/renderer_host/accelerated_surface_container_manager_mac.h"
#include "webkit/glue/plugins/webplugin.h"

AcceleratedSurfaceContainerMac::AcceleratedSurfaceContainerMac(
    AcceleratedSurfaceContainerManagerMac* manager)
    : manager_(manager),
      x_(0),
      y_(0),
      surface_(NULL),
      width_(0),
      height_(0),
      texture_(0),
      texture_needs_upload_(true) {
}

AcceleratedSurfaceContainerMac::~AcceleratedSurfaceContainerMac() {
  EnqueueTextureForDeletion();
  ReleaseIOSurface();
}

void AcceleratedSurfaceContainerMac::ReleaseIOSurface() {
  if (surface_) {
    CFRelease(surface_);
    surface_ = NULL;
  }
}

void AcceleratedSurfaceContainerMac::SetSizeAndIOSurface(
    int32 width,
    int32 height,
    uint64 io_surface_identifier) {
  ReleaseIOSurface();
  IOSurfaceSupport* io_surface_support = IOSurfaceSupport::Initialize();
  if (io_surface_support) {
    surface_ = io_surface_support->IOSurfaceLookup(
        static_cast<uint32>(io_surface_identifier));
    EnqueueTextureForDeletion();
    width_ = width;
    height_ = height;
  }
}

void AcceleratedSurfaceContainerMac::SetSizeAndTransportDIB(
    int32 width,
    int32 height,
    TransportDIB::Handle transport_dib) {
  if (TransportDIB::is_valid(transport_dib)) {
    transport_dib_.reset(TransportDIB::Map(transport_dib));
    EnqueueTextureForDeletion();
    width_ = width;
    height_ = height;
  }
}

void AcceleratedSurfaceContainerMac::MoveTo(
    const webkit_glue::WebPluginGeometry& geom) {
  x_ = geom.window_rect.x();
  y_ = geom.window_rect.y();
  // TODO(kbr): may need to pay attention to cutout rects.
  clipRect_ = geom.clip_rect;
}

void AcceleratedSurfaceContainerMac::Draw(CGLContextObj context) {
  IOSurfaceSupport* io_surface_support = IOSurfaceSupport::Initialize();
  GLenum target = GL_TEXTURE_RECTANGLE_ARB;
  if (!texture_) {
    if ((io_surface_support && !surface_) ||
        (!io_surface_support && !transport_dib_.get()))
      return;
    glGenTextures(1, &texture_);
    glBindTexture(target, texture_);
    glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (io_surface_support) {
      texture_needs_upload_ = true;
    } else {
      // Reserve space on the card for the actual texture upload, done with the
      // glTexSubImage2D() call, below.
      glTexImage2D(target,
                   0,  // mipmap level 0
                   GL_RGBA,  // internal format
                   width_,
                   height_,
                   0,  // no border
                   GL_BGRA,  // The GPU plugin read BGRA pixels
                   GL_UNSIGNED_INT_8_8_8_8_REV,
                   NULL);  // No data, this call just reserves room.
    }
  }

  // When using an IOSurface, the texture does not need to be repeatedly
  // uploaded, just when we've been told we have to.
  if (io_surface_support && texture_needs_upload_) {
    DCHECK(surface_);
    glBindTexture(target, texture_);
    // Don't think we need to identify a plane.
    GLuint plane = 0;
    io_surface_support->CGLTexImageIOSurface2D(context,
                                               target,
                                               GL_RGBA,
                                               width_,
                                               height_,
                                               GL_BGRA,
                                               GL_UNSIGNED_INT_8_8_8_8_REV,
                                               surface_,
                                               plane);
    texture_needs_upload_ = false;
  }
  // If using TransportDIBs, the texture needs to be uploaded every frame.
  if (transport_dib_.get() != NULL) {
    void* pixel_memory = transport_dib_->memory();
    if (pixel_memory) {
      glBindTexture(target, texture_);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Needed for NPOT textures.
      glTexSubImage2D(target,
                      0,  // mipmap level 0
                      0,  // x-offset
                      0,  // y-offset
                      width_,
                      height_,
                      GL_BGRA,  // The GPU plugin gave us BGRA pixels
                      GL_UNSIGNED_INT_8_8_8_8_REV,
                      pixel_memory);
    }
  }

  if (texture_) {
    // TODO(kbr): convert this to use only OpenGL ES 2.0 functionality
    glBindTexture(target, texture_);
    glEnable(target);
    glBegin(GL_TRIANGLE_STRIP);
    // TODO(kbr): may need to pay attention to cutout rects.
    int clipX = clipRect_.x();
    int clipY = clipRect_.y();
    int clipWidth = clipRect_.width();
    int clipHeight = clipRect_.height();
    int x = x_ + clipX;
    int y = y_ + clipY;
    glTexCoord2f(clipX, height_ - clipY);
    glVertex3f(x, y, 0);
    glTexCoord2f(clipX + clipWidth, height_ - clipY);
    glVertex3f(x + clipWidth, y, 0);
    glTexCoord2f(clipX, height_ - clipY - clipHeight);
    glVertex3f(x, y + clipHeight, 0);
    glTexCoord2f(clipX + clipWidth, height_ - clipY - clipHeight);
    glVertex3f(x + clipWidth, y + clipHeight, 0);
    glEnd();
    glDisable(target);
  }
}

void AcceleratedSurfaceContainerMac::EnqueueTextureForDeletion() {
  if (texture_) {
    manager_->EnqueueTextureForDeletion(texture_);
    texture_ = 0;
  }
}

