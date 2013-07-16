// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/in_process_view_renderer.h"

#include <android/bitmap.h>

#include "android_webview/browser/scoped_app_gl_state_restore.h"
#include "android_webview/public/browser/draw_gl.h"
#include "android_webview/public/browser/draw_sw.h"
#include "base/android/jni_android.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "content/public/browser/web_contents.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkDevice.h"
#include "third_party/skia/include/core/SkGraphics.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/transform.h"
#include "ui/gfx/vector2d_conversions.h"
#include "ui/gfx/vector2d_f.h"

using base::android::AttachCurrentThread;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace android_webview {

namespace {


const void* kUserDataKey = &kUserDataKey;

class UserData : public content::WebContents::Data {
 public:
  UserData(InProcessViewRenderer* ptr) : instance_(ptr) {}
  virtual ~UserData() {
    instance_->WebContentsGone();
  }

  static InProcessViewRenderer* GetInstance(content::WebContents* contents) {
    if (!contents)
      return NULL;
    UserData* data = reinterpret_cast<UserData*>(
        contents->GetUserData(kUserDataKey));
    return data ? data->instance_ : NULL;
  }

 private:
  InProcessViewRenderer* instance_;
};

typedef base::Callback<bool(SkCanvas*)> RenderMethod;

bool RasterizeIntoBitmap(JNIEnv* env,
                         const JavaRef<jobject>& jbitmap,
                         int scroll_x,
                         int scroll_y,
                         const RenderMethod& renderer) {
  DCHECK(jbitmap.obj());

  AndroidBitmapInfo bitmap_info;
  if (AndroidBitmap_getInfo(env, jbitmap.obj(), &bitmap_info) < 0) {
    LOG(ERROR) << "Error getting java bitmap info.";
    return false;
  }

  void* pixels = NULL;
  if (AndroidBitmap_lockPixels(env, jbitmap.obj(), &pixels) < 0) {
    LOG(ERROR) << "Error locking java bitmap pixels.";
    return false;
  }

  bool succeeded;
  {
    SkBitmap bitmap;
    bitmap.setConfig(SkBitmap::kARGB_8888_Config,
                     bitmap_info.width,
                     bitmap_info.height,
                     bitmap_info.stride);
    bitmap.setPixels(pixels);

    SkDevice device(bitmap);
    SkCanvas canvas(&device);
    canvas.translate(-scroll_x, -scroll_y);
    succeeded = renderer.Run(&canvas);
  }

  if (AndroidBitmap_unlockPixels(env, jbitmap.obj()) < 0) {
    LOG(ERROR) << "Error unlocking java bitmap pixels.";
    return false;
  }

  return succeeded;
}

bool RenderPictureToCanvas(SkPicture* picture, SkCanvas* canvas) {
  canvas->drawPicture(*picture);
  return true;
}

// TODO(boliu): Remove this when hardware mode is ready.
bool HardwareEnabled() {
 return CommandLine::ForCurrentProcess()->HasSwitch("testing-webview-gl-mode");
}

// Provides software rendering functions from the Android glue layer.
// Allows preventing extra copies of data when rendering.
AwDrawSWFunctionTable* g_sw_draw_functions = NULL;

// Tells if the Skia library versions in Android and Chromium are compatible.
// If they are then it's possible to pass Skia objects like SkPictures to the
// Android glue layer via the SW rendering functions.
// If they are not, then additional copies and rasterizations are required
// as a fallback mechanism, which will have an important performance impact.
bool g_is_skia_version_compatible = false;

const int64 kFallbackTickTimeoutInMilliseconds = 500;

}  // namespace

// static
void BrowserViewRenderer::SetAwDrawSWFunctionTable(
    AwDrawSWFunctionTable* table) {
  g_sw_draw_functions = table;
  g_is_skia_version_compatible =
      g_sw_draw_functions->is_skia_version_compatible(&SkGraphics::GetVersion);
  LOG_IF(WARNING, !g_is_skia_version_compatible)
      << "Skia versions are not compatible, rendering performance will suffer.";
}

// static
AwDrawSWFunctionTable* BrowserViewRenderer::GetAwDrawSWFunctionTable() {
  return g_sw_draw_functions;
}

// static
bool BrowserViewRenderer::IsSkiaVersionCompatible() {
  DCHECK(g_sw_draw_functions);
  return g_is_skia_version_compatible;
}

InProcessViewRenderer::InProcessViewRenderer(
    BrowserViewRenderer::Client* client,
    JavaHelper* java_helper,
    content::WebContents* web_contents)
    : client_(client),
      java_helper_(java_helper),
      web_contents_(web_contents),
      compositor_(NULL),
      visible_(false),
      dip_scale_(0.0),
      page_scale_factor_(1.0),
      continuous_invalidate_(false),
      block_invalidates_(false),
      do_ensure_continuous_invalidation_task_pending_(false),
      weak_factory_(this),
      width_(0),
      height_(0),
      attached_to_window_(false),
      hardware_initialized_(false),
      hardware_failed_(false),
      last_egl_context_(NULL) {
  CHECK(web_contents_);
  web_contents_->SetUserData(kUserDataKey, new UserData(this));
  content::SynchronousCompositor::SetClientForWebContents(web_contents_, this);

  // Currently the logic in this class relies on |compositor_| remaining NULL
  // until the DidInitializeCompositor() call, hence it is not set here.
}

InProcessViewRenderer::~InProcessViewRenderer() {
  CHECK(web_contents_);
  content::SynchronousCompositor::SetClientForWebContents(web_contents_, NULL);
  web_contents_->SetUserData(kUserDataKey, NULL);
  DCHECK(web_contents_ == NULL);  // WebContentsGone should have been called.
}

// static
InProcessViewRenderer* InProcessViewRenderer::FromWebContents(
    content::WebContents* contents) {
  return UserData::GetInstance(contents);
}

void InProcessViewRenderer::WebContentsGone() {
  web_contents_ = NULL;
  compositor_ = NULL;
}

bool InProcessViewRenderer::OnDraw(jobject java_canvas,
                                   bool is_hardware_canvas,
                                   const gfx::Vector2d& scroll,
                                   const gfx::Rect& clip) {
  fallback_tick_.Cancel();
  scroll_at_start_of_frame_  = scroll;
  if (is_hardware_canvas && attached_to_window_ && HardwareEnabled()) {
    // We should be performing a hardware draw here. If we don't have the
    // comositor yet or if RequestDrawGL fails, it means we failed this draw and
    // thus return false here to clear to background color for this draw.
    return compositor_ && client_->RequestDrawGL(java_canvas);
  }
  // Perform a software draw
  block_invalidates_ = true;
  bool result = DrawSWInternal(java_canvas, clip);
  block_invalidates_ = false;
  EnsureContinuousInvalidation(NULL);
  return result;
}

void InProcessViewRenderer::DrawGL(AwDrawGLInfo* draw_info) {
  TRACE_EVENT0("android_webview", "InProcessViewRenderer::DrawGL");
  DCHECK(visible_);

  // We need to watch if the current Android context has changed and enforce
  // a clean-up in the compositor.
  EGLContext current_context = eglGetCurrentContext();
  if (!current_context) {
    TRACE_EVENT_INSTANT0(
        "android_webview", "EarlyOut_NullEGLContext", TRACE_EVENT_SCOPE_THREAD);
    return;
  }

  ScopedAppGLStateRestore state_restore;

  if (attached_to_window_ && compositor_ && !hardware_initialized_) {
    TRACE_EVENT0("android_webview", "InitializeHwDraw");
    hardware_failed_ = !compositor_->InitializeHwDraw();
    hardware_initialized_ = true;
    last_egl_context_ = current_context;

    if (hardware_failed_)
      return;
  }

  if (draw_info->mode == AwDrawGLInfo::kModeProcess)
    return;

  if (last_egl_context_ != current_context) {
    // TODO(boliu): Handle context lost
    TRACE_EVENT_INSTANT0(
        "android_webview", "EGLContextChanged", TRACE_EVENT_SCOPE_THREAD);
  }
  last_egl_context_ = current_context;

  if (!compositor_) {
    TRACE_EVENT_INSTANT0(
        "android_webview", "EarlyOut_NoCompositor", TRACE_EVENT_SCOPE_THREAD);
    return;
  }


  gfx::Transform transform;
  transform.matrix().setColMajorf(draw_info->transform);
  transform.Translate(scroll_at_start_of_frame_.x(),
                      scroll_at_start_of_frame_.y());
  // TODO(joth): Check return value.
  block_invalidates_ = true;
  compositor_->DemandDrawHw(
      gfx::Size(draw_info->width, draw_info->height),
      transform,
      gfx::Rect(draw_info->clip_left,
                draw_info->clip_top,
                draw_info->clip_right - draw_info->clip_left,
                draw_info->clip_bottom - draw_info->clip_top));
  block_invalidates_ = false;

  EnsureContinuousInvalidation(draw_info);
}

bool InProcessViewRenderer::DrawSWInternal(jobject java_canvas,
                                           const gfx::Rect& clip) {
  TRACE_EVENT0("android_webview", "InProcessViewRenderer::DrawSW");

  if (clip.IsEmpty()) {
    TRACE_EVENT_INSTANT0(
        "android_webview", "EarlyOut_EmptyClip", TRACE_EVENT_SCOPE_THREAD);
    return true;
  }

  if (!compositor_) {
    TRACE_EVENT_INSTANT0(
        "android_webview", "EarlyOut_NoCompositor", TRACE_EVENT_SCOPE_THREAD);
    return false;
  }

  JNIEnv* env = AttachCurrentThread();

  AwDrawSWFunctionTable* sw_functions = GetAwDrawSWFunctionTable();
  AwPixelInfo* pixels = sw_functions ?
      sw_functions->access_pixels(env, java_canvas) : NULL;
  // Render into an auxiliary bitmap if pixel info is not available.
  ScopedJavaLocalRef<jobject> jcanvas(env, java_canvas);
  if (pixels == NULL) {
    TRACE_EVENT0("android_webview", "RenderToAuxBitmap");
    ScopedJavaLocalRef<jobject> jbitmap(java_helper_->CreateBitmap(
        env, clip.width(), clip.height(), jcanvas, web_contents_));
    if (!jbitmap.obj()) {
      TRACE_EVENT_INSTANT0("android_webview",
                           "EarlyOut_BitmapAllocFail",
                           TRACE_EVENT_SCOPE_THREAD);
      return false;
    }

    if (!RasterizeIntoBitmap(env, jbitmap,
                             clip.x() - scroll_at_start_of_frame_.x(),
                             clip.y() - scroll_at_start_of_frame_.y(),
                             base::Bind(&InProcessViewRenderer::CompositeSW,
                                        base::Unretained(this)))) {
      TRACE_EVENT_INSTANT0("android_webview",
                           "EarlyOut_RasterizeFail",
                           TRACE_EVENT_SCOPE_THREAD);
      return false;
    }

    java_helper_->DrawBitmapIntoCanvas(env, jbitmap, jcanvas,
                                       clip.x(), clip.y());
    return true;
  }

  // Draw in a SkCanvas built over the pixel information.
  bool succeeded = false;
  {
    SkBitmap bitmap;
    bitmap.setConfig(static_cast<SkBitmap::Config>(pixels->config),
                     pixels->width,
                     pixels->height,
                     pixels->row_bytes);
    bitmap.setPixels(pixels->pixels);
    SkDevice device(bitmap);
    SkCanvas canvas(&device);
    SkMatrix matrix;
    for (int i = 0; i < 9; i++)
      matrix.set(i, pixels->matrix[i]);
    canvas.setMatrix(matrix);

    if (pixels->clip_region_size) {
      SkRegion clip_region;
      size_t bytes_read = clip_region.readFromMemory(pixels->clip_region);
      DCHECK_EQ(pixels->clip_region_size, bytes_read);
      canvas.setClipRegion(clip_region);
    } else {
      canvas.clipRect(gfx::RectToSkRect(clip));
    }
    canvas.translate(scroll_at_start_of_frame_.x(),
                     scroll_at_start_of_frame_.y());

    succeeded = CompositeSW(&canvas);
  }

  sw_functions->release_pixels(pixels);
  return succeeded;
}

base::android::ScopedJavaLocalRef<jobject>
InProcessViewRenderer::CapturePicture() {
  if (!compositor_ || !GetAwDrawSWFunctionTable()) {
    TRACE_EVENT_INSTANT0(
        "android_webview", "EarlyOut_CapturePicture", TRACE_EVENT_SCOPE_THREAD);
    return ScopedJavaLocalRef<jobject>();
  }

  gfx::Size record_size(width_, height_);

  // Return empty Picture objects for empty SkPictures.
  JNIEnv* env = AttachCurrentThread();
  if (record_size.width() <= 0 || record_size.height() <= 0) {
    return java_helper_->RecordBitmapIntoPicture(
        env, ScopedJavaLocalRef<jobject>());
  }

  skia::RefPtr<SkPicture> picture = skia::AdoptRef(new SkPicture);
  SkCanvas* rec_canvas = picture->beginRecording(record_size.width(),
                                                 record_size.height(),
                                                 0);
  if (!CompositeSW(rec_canvas))
    return ScopedJavaLocalRef<jobject>();
  picture->endRecording();

  if (IsSkiaVersionCompatible()) {
    // Add a reference that the create_picture() will take ownership of.
    picture->ref();
    return ScopedJavaLocalRef<jobject>(env,
        GetAwDrawSWFunctionTable()->create_picture(env, picture.get()));
  }

  // If Skia versions are not compatible, workaround it by rasterizing the
  // picture into a bitmap and drawing it into a new Java picture. Pass null
  // for |canvas| as we don't have java canvas at this point (and it would be
  // software anyway).
  ScopedJavaLocalRef<jobject> jbitmap(java_helper_->CreateBitmap(
      env, picture->width(), picture->height(), ScopedJavaLocalRef<jobject>(),
      NULL));
  if (!jbitmap.obj())
    return ScopedJavaLocalRef<jobject>();

  if (!RasterizeIntoBitmap(env, jbitmap, 0, 0,
      base::Bind(&RenderPictureToCanvas,
                 base::Unretained(picture.get())))) {
    return ScopedJavaLocalRef<jobject>();
  }

  return java_helper_->RecordBitmapIntoPicture(env, jbitmap);
}

void InProcessViewRenderer::EnableOnNewPicture(bool enabled) {
}

void InProcessViewRenderer::OnVisibilityChanged(bool visible) {
  TRACE_EVENT_INSTANT1("android_webview",
                       "InProcessViewRenderer::OnVisibilityChanged",
                       TRACE_EVENT_SCOPE_THREAD,
                       "visible",
                       visible);
  visible_ = visible;
}

void InProcessViewRenderer::OnSizeChanged(int width, int height) {
  TRACE_EVENT_INSTANT2("android_webview",
                       "InProcessViewRenderer::OnSizeChanged",
                       TRACE_EVENT_SCOPE_THREAD,
                       "width",
                       width,
                       "height",
                       height);
  width_ = width;
  height_ = height;
}

void InProcessViewRenderer::OnAttachedToWindow(int width, int height) {
  TRACE_EVENT2("android_webview",
               "InProcessViewRenderer::OnAttachedToWindow",
               "width",
               width,
               "height",
               height);
  attached_to_window_ = true;
  width_ = width;
  height_ = height;
}

void InProcessViewRenderer::OnDetachedFromWindow() {
  TRACE_EVENT0("android_webview",
               "InProcessViewRenderer::OnDetachedFromWindow");

  if (hardware_initialized_) {
    DCHECK(compositor_);

    ScopedAppGLStateRestore state_restore;
    compositor_->ReleaseHwDraw();
    hardware_initialized_ = false;
  }

  attached_to_window_ = false;
}

bool InProcessViewRenderer::IsAttachedToWindow() {
  return attached_to_window_;
}

bool InProcessViewRenderer::IsViewVisible() {
  return visible_;
}

gfx::Rect InProcessViewRenderer::GetScreenRect() {
  return gfx::Rect(client_->GetLocationOnScreen(), gfx::Size(width_, height_));
}

void InProcessViewRenderer::DidInitializeCompositor(
    content::SynchronousCompositor* compositor) {
  TRACE_EVENT0("android_webview",
               "InProcessViewRenderer::DidInitializeCompositor");
  DCHECK(compositor && compositor_ == NULL);
  compositor_ = compositor;
  hardware_initialized_ = false;
  hardware_failed_ = false;
}

void InProcessViewRenderer::DidDestroyCompositor(
    content::SynchronousCompositor* compositor) {
  TRACE_EVENT0("android_webview",
               "InProcessViewRenderer::DidDestroyCompositor");
  DCHECK(compositor_ == compositor);

  // This can fail if Apps call destroy while the webview is still attached
  // to the view tree. This is an illegal operation that will lead to leaks.
  // Log for now. Consider a proper fix if this becomes a problem.
  LOG_IF(ERROR, hardware_initialized_)
      << "Destroy called before OnDetachedFromWindow. May Leak GL resources";
  compositor_ = NULL;
}

void InProcessViewRenderer::SetContinuousInvalidate(bool invalidate) {
  if (continuous_invalidate_ == invalidate)
    return;

  TRACE_EVENT_INSTANT1("android_webview",
                       "InProcessViewRenderer::SetContinuousInvalidate",
                       TRACE_EVENT_SCOPE_THREAD,
                       "invalidate",
                       invalidate);
  continuous_invalidate_ = invalidate;
  EnsureContinuousInvalidation(NULL);
}

void InProcessViewRenderer::SetDipScale(float dip_scale) {
  dip_scale_ = dip_scale;
  CHECK(dip_scale_ > 0);
}

void InProcessViewRenderer::SetPageScaleFactor(float page_scale_factor) {
  page_scale_factor_ = page_scale_factor;
  CHECK(page_scale_factor_ > 0);
}

void InProcessViewRenderer::ScrollTo(gfx::Vector2d new_value) {
  DCHECK(dip_scale_ > 0);
  // In general we don't guarantee that the scroll offset transforms are
  // symmetrical. That is if scrolling from JS to offset1 results in a native
  // offset2 then scrolling from UI to offset2 results in JS being scrolled to
  // offset1 again.
  // The reason we explicitly do rounding here is that it seems to yeld the
  // most stabile transformation.
  gfx::Vector2dF new_value_css = gfx::ToRoundedVector2d(
      gfx::ScaleVector2d(new_value, 1.0f / (dip_scale_ * page_scale_factor_)));

  DCHECK(scroll_offset_css_ != new_value_css);

  scroll_offset_css_ = new_value_css;

  if (compositor_)
    compositor_->DidChangeRootLayerScrollOffset();
}

void InProcessViewRenderer::SetTotalRootLayerScrollOffset(
    gfx::Vector2dF new_value_css) {
  previous_accumulated_overscroll_ = gfx::Vector2dF();

  // TOOD(mkosiba): Add a DCHECK to say that this does _not_ get called during
  // DrawGl when http://crbug.com/249972 is fixed.
  if (scroll_offset_css_ == new_value_css)
    return;

  scroll_offset_css_ = new_value_css;

  DCHECK(dip_scale_ > 0);
  DCHECK(page_scale_factor_ > 0);

  gfx::Vector2d scroll_offset = gfx::ToRoundedVector2d(
      gfx::ScaleVector2d(new_value_css, dip_scale_ * page_scale_factor_));

  client_->ScrollContainerViewTo(scroll_offset);
}

gfx::Vector2dF InProcessViewRenderer::GetTotalRootLayerScrollOffset() {
  return scroll_offset_css_;
}

void InProcessViewRenderer::DidOverscroll(
    gfx::Vector2dF accumulated_overscroll,
    gfx::Vector2dF current_fling_velocity) {
  // TODO(mkosiba): Enable this once flinging is handled entirely Java-side.
  // DCHECK(current_fling_velocity.IsZero());
  const float physical_pixel_scale = dip_scale_ * page_scale_factor_;
  gfx::Vector2d overscroll_delta = gfx::ToRoundedVector2d(gfx::ScaleVector2d(
      accumulated_overscroll - previous_accumulated_overscroll_,
      physical_pixel_scale));
  previous_accumulated_overscroll_ +=
      gfx::ScaleVector2d(overscroll_delta, 1.0f / physical_pixel_scale);
  client_->DidOverscroll(overscroll_delta);
}

void InProcessViewRenderer::EnsureContinuousInvalidation(
    AwDrawGLInfo* draw_info) {
  if (continuous_invalidate_ && !block_invalidates_ &&
      !do_ensure_continuous_invalidation_task_pending_) {
    do_ensure_continuous_invalidation_task_pending_ = true;

    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&InProcessViewRenderer::DoEnsureContinuousInvalidation,
                   weak_factory_.GetWeakPtr(),
                   static_cast<AwDrawGLInfo*>(NULL)));
  }
}

void InProcessViewRenderer::DoEnsureContinuousInvalidation(
    AwDrawGLInfo* draw_info) {
  do_ensure_continuous_invalidation_task_pending_ = false;
  if (continuous_invalidate_ && !block_invalidates_) {
    if (draw_info) {
      draw_info->dirty_left = draw_info->clip_left;
      draw_info->dirty_top = draw_info->clip_top;
      draw_info->dirty_right = draw_info->clip_right;
      draw_info->dirty_bottom = draw_info->clip_bottom;
      draw_info->status_mask |= AwDrawGLInfo::kStatusMaskDraw;
    } else {
      client_->PostInvalidate();
    }

    // Unretained here is safe because the callback is cancelled when
    // |fallback_tick_| is destroyed.
    fallback_tick_.Reset(base::Bind(&InProcessViewRenderer::FallbackTickFired,
                                    base::Unretained(this)));
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        fallback_tick_.callback(),
        base::TimeDelta::FromMilliseconds(kFallbackTickTimeoutInMilliseconds));

    block_invalidates_ = true;
  }
}

void InProcessViewRenderer::FallbackTickFired() {
  TRACE_EVENT1("android_webview",
               "InProcessViewRenderer::FallbackTickFired",
               "continuous_invalidate_",
               continuous_invalidate_);
  if (continuous_invalidate_ && compositor_) {
    SkDevice device(SkBitmap::kARGB_8888_Config, 1, 1);
    SkCanvas canvas(&device);
    block_invalidates_ = true;
    CompositeSW(&canvas);
  }
  block_invalidates_ = false;
  EnsureContinuousInvalidation(NULL);
}

bool InProcessViewRenderer::CompositeSW(SkCanvas* canvas) {
  DCHECK(compositor_);
  return compositor_->DemandDrawSw(canvas);
}

}  // namespace android_webview
