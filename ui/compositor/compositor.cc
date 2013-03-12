// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/compositor.h"

#include <algorithm>
#include <deque>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "cc/context_provider.h"
#include "cc/input_handler.h"
#include "cc/layer.h"
#include "cc/layer_tree_host.h"
#include "cc/output_surface.h"
#include "cc/thread_impl.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/compositor/compositor_observer.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/compositor/dip_util.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/test_web_graphics_context_3d.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_switches.h"
#include "webkit/gpu/grcontext_for_webgraphicscontext3d.h"
#include "webkit/gpu/webgraphicscontext3d_in_process_impl.h"

#if defined(OS_CHROMEOS)
#include "base/chromeos/chromeos_version.h"
#endif

namespace {

const double kDefaultRefreshRate = 60.0;
const double kTestRefreshRate = 100.0;

enum SwapType {
  DRAW_SWAP,
  READPIXELS_SWAP,
};

base::Thread* g_compositor_thread = NULL;

bool g_test_compositor_enabled = false;

ui::ContextFactory* g_context_factory = NULL;

const int kCompositorLockTimeoutMs = 67;

class PendingSwap {
 public:
  PendingSwap(SwapType type, ui::PostedSwapQueue* posted_swaps);
  ~PendingSwap();

  SwapType type() const { return type_; }
  bool posted() const { return posted_; }

 private:
  friend class ui::PostedSwapQueue;

  SwapType type_;
  bool posted_;
  ui::PostedSwapQueue* posted_swaps_;

  DISALLOW_COPY_AND_ASSIGN(PendingSwap);
};

class NullContextProvider : public cc::ContextProvider {
 public:
  virtual bool InitializeOnMainThread() OVERRIDE { return false; }
  virtual bool BindToCurrentThread() OVERRIDE { return false; }
  virtual WebKit::WebGraphicsContext3D* Context3d() OVERRIDE { return NULL; }
  virtual class GrContext* GrContext() OVERRIDE { return NULL; }
  virtual void VerifyContexts() OVERRIDE {}
  virtual bool DestroyedOnMainThread() OVERRIDE { return false; }

 protected:
  virtual ~NullContextProvider() {}
};

struct MainThreadNullContextProvider {
  scoped_refptr<NullContextProvider> provider;

  static MainThreadNullContextProvider* GetInstance() {
    return Singleton<MainThreadNullContextProvider>::get();
  }
};

struct CompositorThreadNullContextProvider {
  scoped_refptr<NullContextProvider> provider;

  static CompositorThreadNullContextProvider* GetInstance() {
    return Singleton<CompositorThreadNullContextProvider>::get();
  }
};

}  // namespace

namespace ui {

// static
ContextFactory* ContextFactory::GetInstance() {
  // We leak the shared resources so that we don't race with
  // the tear down of the gl_bindings.
  if (!g_context_factory) {
    DVLOG(1) << "Using DefaultSharedResource";
    scoped_ptr<DefaultContextFactory> instance(
        new DefaultContextFactory());
    if (instance->Initialize())
      g_context_factory = instance.release();
  }
  return g_context_factory;
}

// static
void ContextFactory::SetInstance(ContextFactory* instance) {
  g_context_factory = instance;
}

DefaultContextFactory::DefaultContextFactory() {
}

DefaultContextFactory::~DefaultContextFactory() {
}

bool DefaultContextFactory::Initialize() {
  // The following line of code exists soley to disable IO restrictions
  // on this thread long enough to perform the GL bindings.
  // TODO(wjmaclean) Remove this when GL initialisation cleaned up.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (!gfx::GLSurface::InitializeOneOff() ||
      gfx::GetGLImplementation() == gfx::kGLImplementationNone) {
    LOG(ERROR) << "Could not load the GL bindings";
    return false;
  }
  return true;
}

cc::OutputSurface* DefaultContextFactory::CreateOutputSurface(
    Compositor* compositor) {
  return new cc::OutputSurface(
      make_scoped_ptr(CreateContextCommon(compositor, false)));
}

WebKit::WebGraphicsContext3D* DefaultContextFactory::CreateOffscreenContext() {
  return CreateContextCommon(NULL, true);
}

class DefaultContextFactory::DefaultContextProvider
    : public cc::ContextProvider {
 public:
  DefaultContextProvider(ContextFactory* factory)
      : factory_(factory),
        destroyed_(false) {}

  virtual bool InitializeOnMainThread() OVERRIDE {
    if (context3d_)
      return true;
    context3d_.reset(factory_->CreateOffscreenContext());
    return !!context3d_;
  }

  virtual bool BindToCurrentThread() {
    return context3d_->makeContextCurrent();
  }

  virtual WebKit::WebGraphicsContext3D* Context3d() { return context3d_.get(); }

  virtual class GrContext* GrContext() {
    if (!gr_context_) {
      gr_context_.reset(
          new webkit::gpu::GrContextForWebGraphicsContext3D(context3d_.get()));
    }
    return gr_context_->get();
  }

  virtual void VerifyContexts() OVERRIDE {
    if (context3d_ && !context3d_->isContextLost())
      return;
    base::AutoLock lock(destroyed_lock_);
    destroyed_ = true;
  }

  bool DestroyedOnMainThread() {
    base::AutoLock lock(destroyed_lock_);
    return destroyed_;
  }

 protected:
  virtual ~DefaultContextProvider() {}

 private:
  ContextFactory* factory_;
  base::Lock destroyed_lock_;
  bool destroyed_;
  scoped_ptr<WebKit::WebGraphicsContext3D> context3d_;
  scoped_ptr<webkit::gpu::GrContextForWebGraphicsContext3D> gr_context_;
};

scoped_refptr<cc::ContextProvider>
DefaultContextFactory::OffscreenContextProviderForMainThread() {
  if (!offscreen_contexts_main_thread_ ||
      !offscreen_contexts_main_thread_->DestroyedOnMainThread()) {
    offscreen_contexts_main_thread_ = new DefaultContextProvider(this);
  }
  return offscreen_contexts_main_thread_;
}

scoped_refptr<cc::ContextProvider>
DefaultContextFactory::OffscreenContextProviderForCompositorThread() {
  if (!offscreen_contexts_compositor_thread_ ||
      !offscreen_contexts_compositor_thread_->DestroyedOnMainThread()) {
    offscreen_contexts_compositor_thread_ = new DefaultContextProvider(this);
  }
  return offscreen_contexts_compositor_thread_;
}

void DefaultContextFactory::RemoveCompositor(Compositor* compositor) {
}

WebKit::WebGraphicsContext3D* DefaultContextFactory::CreateContextCommon(
    Compositor* compositor,
    bool offscreen) {
  DCHECK(offscreen || compositor);
  WebKit::WebGraphicsContext3D::Attributes attrs;
  attrs.depth = false;
  attrs.stencil = false;
  attrs.antialias = false;
  attrs.shareResources = true;
  WebKit::WebGraphicsContext3D* context =
      offscreen ?
      webkit::gpu::WebGraphicsContext3DInProcessImpl::CreateForWebView(
          attrs, false) :
      webkit::gpu::WebGraphicsContext3DInProcessImpl::CreateForWindow(
          attrs, compositor->widget(), share_group_.get());
  if (!context)
    return NULL;

  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (!offscreen) {
    context->makeContextCurrent();
    gfx::GLContext* gl_context = gfx::GLContext::GetCurrent();
    bool vsync = !command_line->HasSwitch(switches::kDisableGpuVsync);
    gl_context->SetSwapInterval(vsync ? 1 : 0);
    gl_context->ReleaseCurrent(NULL);
  }
  return context;
}

Texture::Texture(bool flipped, const gfx::Size& size, float device_scale_factor)
    : size_(size),
      flipped_(flipped),
      device_scale_factor_(device_scale_factor) {
}

Texture::~Texture() {
}

std::string Texture::Produce() {
  return EmptyString();
}

CompositorLock::CompositorLock(Compositor* compositor)
    : compositor_(compositor) {
  MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&CompositorLock::CancelLock, AsWeakPtr()),
      base::TimeDelta::FromMilliseconds(kCompositorLockTimeoutMs));
}

CompositorLock::~CompositorLock() {
  CancelLock();
}

void CompositorLock::CancelLock() {
  if (!compositor_)
    return;
  compositor_->UnlockCompositor();
  compositor_ = NULL;
}

class PostedSwapQueue {
 public:
  PostedSwapQueue() : pending_swap_(NULL) {
  }

  ~PostedSwapQueue() {
    DCHECK(!pending_swap_);
  }

  SwapType NextPostedSwap() const {
    return queue_.front();
  }

  bool AreSwapsPosted() const {
    return !queue_.empty();
  }

  int NumSwapsPosted(SwapType type) const {
    int count = 0;
    for (std::deque<SwapType>::const_iterator it = queue_.begin();
         it != queue_.end(); ++it) {
      if (*it == type)
        count++;
    }
    return count;
  }

  void PostSwap() {
    DCHECK(pending_swap_);
    queue_.push_back(pending_swap_->type());
    pending_swap_->posted_ = true;
  }

  void EndSwap() {
    queue_.pop_front();
  }

 private:
  friend class ::PendingSwap;

  PendingSwap* pending_swap_;
  std::deque<SwapType> queue_;

  DISALLOW_COPY_AND_ASSIGN(PostedSwapQueue);
};

}  // namespace ui

namespace {

PendingSwap::PendingSwap(SwapType type, ui::PostedSwapQueue* posted_swaps)
    : type_(type), posted_(false), posted_swaps_(posted_swaps) {
  // Only one pending swap in flight.
  DCHECK_EQ(static_cast<PendingSwap*>(NULL), posted_swaps_->pending_swap_);
  posted_swaps_->pending_swap_ = this;
}

PendingSwap::~PendingSwap() {
  DCHECK_EQ(this, posted_swaps_->pending_swap_);
  posted_swaps_->pending_swap_ = NULL;
}

}  // namespace

namespace ui {

Compositor::Compositor(CompositorDelegate* delegate,
                       gfx::AcceleratedWidget widget)
    : delegate_(delegate),
      root_layer_(NULL),
      widget_(widget),
      posted_swaps_(new PostedSwapQueue()),
      device_scale_factor_(0.0f),
      last_started_frame_(0),
      last_ended_frame_(0),
      disable_schedule_composite_(false),
      compositor_lock_(NULL) {
  root_web_layer_ = cc::Layer::Create();
  root_web_layer_->SetAnchorPoint(gfx::PointF(0.f, 0.f));

  CommandLine* command_line = CommandLine::ForCurrentProcess();
  cc::LayerTreeSettings settings;
  settings.initialDebugState.showFPSCounter =
      command_line->HasSwitch(switches::kUIShowFPSCounter);
  settings.initialDebugState.showPlatformLayerTree =
      command_line->HasSwitch(switches::kUIShowLayerTree);
  settings.refreshRate =
      g_test_compositor_enabled ? kTestRefreshRate : kDefaultRefreshRate;
  settings.initialDebugState.showDebugBorders =
      command_line->HasSwitch(switches::kUIShowLayerBorders);
  settings.partialSwapEnabled =
      command_line->HasSwitch(switches::kUIEnablePartialSwap);
  settings.perTilePaintingEnabled =
      command_line->HasSwitch(switches::kUIEnablePerTilePainting);

  scoped_ptr<cc::Thread> thread;
  if (g_compositor_thread) {
    thread = cc::ThreadImpl::createForDifferentThread(
        g_compositor_thread->message_loop_proxy());
  }

  host_ = cc::LayerTreeHost::create(this, settings, thread.Pass());
  host_->setRootLayer(root_web_layer_);
  host_->setSurfaceReady();
}

Compositor::~Compositor() {
  CancelCompositorLock();
  DCHECK(!compositor_lock_);

  // Don't call |CompositorDelegate::ScheduleDraw| from this point.
  delegate_ = NULL;
  if (root_layer_)
    root_layer_->SetCompositor(NULL);

  // Stop all outstanding draws before telling the ContextFactory to tear
  // down any contexts that the |host_| may rely upon.
  host_.reset();

  if (!g_test_compositor_enabled)
    ContextFactory::GetInstance()->RemoveCompositor(this);
}

void Compositor::Initialize(bool use_thread) {
  if (use_thread) {
    g_compositor_thread = new base::Thread("Browser Compositor");
    g_compositor_thread->Start();
  }
}

void Compositor::Terminate() {
  if (g_compositor_thread) {
    g_compositor_thread->Stop();
    delete g_compositor_thread;
    g_compositor_thread = NULL;
  }
}

void Compositor::ScheduleDraw() {
  if (g_compositor_thread)
    host_->composite();
  else if (delegate_)
    delegate_->ScheduleDraw();
}

void Compositor::SetRootLayer(Layer* root_layer) {
  if (root_layer_ == root_layer)
    return;
  if (root_layer_)
    root_layer_->SetCompositor(NULL);
  root_layer_ = root_layer;
  if (root_layer_ && !root_layer_->GetCompositor())
    root_layer_->SetCompositor(this);
  root_web_layer_->RemoveAllChildren();
  if (root_layer_)
    root_web_layer_->AddChild(root_layer_->cc_layer());
}

void Compositor::SetHostHasTransparentBackground(
    bool host_has_transparent_background) {
  host_->setHasTransparentBackground(host_has_transparent_background);
}

void Compositor::Draw(bool force_clear) {
  DCHECK(!g_compositor_thread);

  if (!root_layer_)
    return;

  last_started_frame_++;
  PendingSwap pending_swap(DRAW_SWAP, posted_swaps_.get());
  if (!IsLocked()) {
    // TODO(nduca): Temporary while compositor calls
    // compositeImmediately() directly.
    layout();
    host_->composite();
  }
  if (!pending_swap.posted())
    NotifyEnd();
}

void Compositor::ScheduleFullDraw() {
  host_->setNeedsRedraw();
}

bool Compositor::ReadPixels(SkBitmap* bitmap,
                            const gfx::Rect& bounds_in_pixel) {
  if (bounds_in_pixel.right() > size().width() ||
      bounds_in_pixel.bottom() > size().height())
    return false;
  bitmap->setConfig(SkBitmap::kARGB_8888_Config,
                    bounds_in_pixel.width(), bounds_in_pixel.height());
  bitmap->allocPixels();
  SkAutoLockPixels lock_image(*bitmap);
  unsigned char* pixels = static_cast<unsigned char*>(bitmap->getPixels());
  CancelCompositorLock();
  PendingSwap pending_swap(READPIXELS_SWAP, posted_swaps_.get());
  return host_->compositeAndReadback(pixels, bounds_in_pixel);
}

void Compositor::SetScaleAndSize(float scale, const gfx::Size& size_in_pixel) {
  DCHECK_GT(scale, 0);
  if (!size_in_pixel.IsEmpty()) {
    size_ = size_in_pixel;
    host_->setViewportSize(size_in_pixel, size_in_pixel);
    root_web_layer_->SetBounds(size_in_pixel);
  }
  if (device_scale_factor_ != scale) {
    device_scale_factor_ = scale;
    if (root_layer_)
      root_layer_->OnDeviceScaleFactorChanged(scale);
  }
}

void Compositor::AddObserver(CompositorObserver* observer) {
  observer_list_.AddObserver(observer);
}

void Compositor::RemoveObserver(CompositorObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

bool Compositor::HasObserver(CompositorObserver* observer) {
  return observer_list_.HasObserver(observer);
}

void Compositor::OnSwapBuffersPosted() {
  DCHECK(!g_compositor_thread);
  posted_swaps_->PostSwap();
}

void Compositor::OnSwapBuffersComplete() {
  DCHECK(!g_compositor_thread);
  DCHECK(posted_swaps_->AreSwapsPosted());
  DCHECK_GE(1, posted_swaps_->NumSwapsPosted(DRAW_SWAP));
  if (posted_swaps_->NextPostedSwap() == DRAW_SWAP)
    NotifyEnd();
  posted_swaps_->EndSwap();
}

void Compositor::OnSwapBuffersAborted() {
  DCHECK(!g_compositor_thread);
  DCHECK_GE(1, posted_swaps_->NumSwapsPosted(DRAW_SWAP));

  // We've just lost the context, so unwind all posted_swaps.
  while (posted_swaps_->AreSwapsPosted()) {
    if (posted_swaps_->NextPostedSwap() == DRAW_SWAP)
      NotifyEnd();
    posted_swaps_->EndSwap();
  }

  FOR_EACH_OBSERVER(CompositorObserver,
                    observer_list_,
                    OnCompositingAborted(this));
}

void Compositor::OnUpdateVSyncParameters(base::TimeTicks timebase,
                                         base::TimeDelta interval) {
  FOR_EACH_OBSERVER(CompositorObserver,
                    observer_list_,
                    OnUpdateVSyncParameters(this, timebase, interval));
}

void Compositor::willBeginFrame() {
}

void Compositor::didBeginFrame() {
}

void Compositor::animate(double frameBeginTime) {
}

void Compositor::layout() {
  // We're sending damage that will be addressed during this composite
  // cycle, so we don't need to schedule another composite to address it.
  disable_schedule_composite_ = true;
  if (root_layer_)
    root_layer_->SendDamagedRects();
  disable_schedule_composite_ = false;
}

void Compositor::applyScrollAndScale(gfx::Vector2d scrollDelta,
                                     float pageScale) {
}

scoped_ptr<cc::OutputSurface> Compositor::createOutputSurface() {
  if (g_test_compositor_enabled) {
    scoped_ptr<ui::TestWebGraphicsContext3D> context3d(
        new ui::TestWebGraphicsContext3D);
    context3d->Initialize();
    return make_scoped_ptr(new cc::OutputSurface(
        context3d.PassAs<WebKit::WebGraphicsContext3D>()));
  } else {
    return make_scoped_ptr(
        ContextFactory::GetInstance()->CreateOutputSurface(this));
  }
}

void Compositor::didRecreateOutputSurface(bool success) {
}

scoped_ptr<cc::InputHandler> Compositor::createInputHandler() {
  return scoped_ptr<cc::InputHandler>();
}

void Compositor::willCommit() {
}

void Compositor::didCommit() {
  DCHECK(!IsLocked());
  FOR_EACH_OBSERVER(CompositorObserver,
                    observer_list_,
                    OnCompositingDidCommit(this));
}

void Compositor::didCommitAndDrawFrame() {
  base::TimeTicks start_time = base::TimeTicks::Now();
  FOR_EACH_OBSERVER(CompositorObserver,
                    observer_list_,
                    OnCompositingStarted(this, start_time));
}

void Compositor::didCompleteSwapBuffers() {
  DCHECK(g_compositor_thread);
  NotifyEnd();
}

void Compositor::scheduleComposite() {
  if (!disable_schedule_composite_)
    ScheduleDraw();
}

scoped_refptr<cc::ContextProvider>
Compositor::OffscreenContextProviderForMainThread() {
  if (g_test_compositor_enabled) {
    if (!MainThreadNullContextProvider::GetInstance()->provider) {
      MainThreadNullContextProvider::GetInstance()->provider =
          new NullContextProvider;
    }
    return MainThreadNullContextProvider::GetInstance()->provider;
  }
  return ContextFactory::GetInstance()->OffscreenContextProviderForMainThread();
}

scoped_refptr<cc::ContextProvider>
Compositor::OffscreenContextProviderForCompositorThread() {
  if (g_test_compositor_enabled) {
    if (!CompositorThreadNullContextProvider::GetInstance()->provider) {
      CompositorThreadNullContextProvider::GetInstance()->provider =
          new NullContextProvider;
    }
    return CompositorThreadNullContextProvider::GetInstance()->provider;
  }
  return ContextFactory::GetInstance()->
      OffscreenContextProviderForCompositorThread();
}

scoped_refptr<CompositorLock> Compositor::GetCompositorLock() {
  if (!compositor_lock_) {
    compositor_lock_ = new CompositorLock(this);
    if (g_compositor_thread)
      host_->setDeferCommits(true);
    FOR_EACH_OBSERVER(CompositorObserver,
                      observer_list_,
                      OnCompositingLockStateChanged(this));
  }
  return compositor_lock_;
}

void Compositor::UnlockCompositor() {
  DCHECK(compositor_lock_);
  compositor_lock_ = NULL;
  if (g_compositor_thread)
    host_->setDeferCommits(false);
  FOR_EACH_OBSERVER(CompositorObserver,
                    observer_list_,
                    OnCompositingLockStateChanged(this));
}

void Compositor::CancelCompositorLock() {
  if (compositor_lock_)
    compositor_lock_->CancelLock();
}

void Compositor::NotifyEnd() {
  last_ended_frame_++;
  FOR_EACH_OBSERVER(CompositorObserver,
                    observer_list_,
                    OnCompositingEnded(this));
}

COMPOSITOR_EXPORT void SetupTestCompositor() {
  if (!CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableTestCompositor)) {
    g_test_compositor_enabled = true;
  }
#if defined(OS_CHROMEOS)
  // If the test is running on the chromeos envrionment (such as
  // device or vm bots), use the real compositor.
  if (base::chromeos::IsRunningOnChromeOS())
    g_test_compositor_enabled = false;
#endif
}

COMPOSITOR_EXPORT void DisableTestCompositor() {
  g_test_compositor_enabled = false;
}

COMPOSITOR_EXPORT bool IsTestCompositorEnabled() {
  return g_test_compositor_enabled;
}

}  // namespace ui
