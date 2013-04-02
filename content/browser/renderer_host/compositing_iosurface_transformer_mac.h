// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_COMPOSITING_IOSURFACE_TRANSFORMER_MAC_H_
#define CONTENT_BROWSER_RENDERER_HOST_COMPOSITING_IOSURFACE_TRANSFORMER_MAC_H_

#include <OpenGL/gl.h>

#include "base/basictypes.h"
#include "content/browser/renderer_host/compositing_iosurface_shader_programs_mac.h"

namespace gfx {
class Rect;
class Size;
}  // namespace gfx

namespace content {

// Provides useful image filtering operations that are implemented efficiently
// using OpenGL shader programs.
//
// Note: All methods assume to be called within an active OpenGL context.
class CompositingIOSurfaceTransformer {
 public:
  // Construct a transformer that always uses the given parameters for texture
  // bindings.  |texture_target| is one of the valid enums to use with
  // glBindTexture().
  // |src_texture_needs_y_flip| is true when the |src_texture| argument to any
  // of the methods below uses upside-down Y coordinates.
  // |shader_program_cache| is not owned by this instance.
  CompositingIOSurfaceTransformer(
      GLenum texture_target, bool src_texture_needs_y_flip,
      CompositingIOSurfaceShaderPrograms* shader_program_cache);

  ~CompositingIOSurfaceTransformer();

  // Resize using bilinear interpolation.  Returns false on error.  Otherwise,
  // the |texture| argument will point to the result and the caller is
  // responsible for calling glDeleteTexture(*texture).
  //
  // If the src and dst sizes are identical, this becomes a simple copy into a
  // new texture.
  //
  // Note: This implementation is faulty in that minifications by more than 2X
  // will undergo aliasing.
  bool ResizeBilinear(GLuint src_texture, const gfx::Rect& src_subrect,
                      const gfx::Size& dst_size, GLuint* texture);

  // Color format conversion from RGB to planar YV12 (also known as YUV420).
  //
  // YV12 is effectively a twelve bit per pixel format consisting of a full-
  // size y (luminance) plane and half-width, half-height u and v (blue and
  // red chrominance) planes.  This method will allocate three off-screen
  // textures, one for each plane, and return them via the output arguments
  // |texture_y|, |texture_u|, and |texture_v|.  While the textures are in
  // GL_RGBA format, they should be interpreted as the appropriate single-byte,
  // planar format after reading the pixel data.  The output arguments
  // |packed_y_size| and |packed_uv_size| follow from these special semantics:
  // They represent the size of their corresponding texture, if it was to be
  // treated like RGBA pixel data.  That means their widths are in terms of
  // "quads," where one quad contains 4 Y (or U or V) pixels.
  //
  // If |src_subrect|'s size does not match |dst_size|, the source will be
  // bilinearly interpolated during conversion.
  //
  // Returns true if successful, and the caller is responsible for deleting the
  // output textures.
  bool TransformRGBToYV12(
      GLuint src_texture, const gfx::Rect& src_subrect,
      const gfx::Size& dst_size,
      GLuint* texture_y, GLuint* texture_u, GLuint* texture_v,
      gfx::Size* packed_y_size, gfx::Size* packed_uv_size);

 private:
  // Target to bind all input and output textures to (which defines the type of
  // textures being created and read).  Generally, this is
  // GL_TEXTURE_RECTANGLE_ARB.
  const GLenum texture_target_;
  const bool src_texture_needs_y_flip_;
  CompositingIOSurfaceShaderPrograms* const shader_program_cache_;

  // Auto-detected and set once in the constructor.
  bool system_supports_multiple_draw_buffers_;

  DISALLOW_COPY_AND_ASSIGN(CompositingIOSurfaceTransformer);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_COMPOSITING_IOSURFACE_TRANSFORMER_MAC_H_
