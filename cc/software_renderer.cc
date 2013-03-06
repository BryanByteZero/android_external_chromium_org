// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/software_renderer.h"

#include "base/debug/trace_event.h"
#include "cc/compositor_frame.h"
#include "cc/compositor_frame_ack.h"
#include "cc/compositor_frame_metadata.h"
#include "cc/debug_border_draw_quad.h"
#include "cc/math_util.h"
#include "cc/output_surface.h"
#include "cc/render_pass_draw_quad.h"
#include "cc/software_output_device.h"
#include "cc/solid_color_draw_quad.h"
#include "cc/texture_draw_quad.h"
#include "cc/tile_draw_quad.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebImage.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkDevice.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkShader.h"
#include "third_party/skia/include/effects/SkLayerRasterizer.h"
#include "ui/gfx/rect_conversions.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/transform.h"

namespace cc {

namespace {

void toSkMatrix(SkMatrix* flattened, const gfx::Transform& m)
{
    // Convert from 4x4 to 3x3 by dropping the third row and column.
    flattened->set(0, SkDoubleToScalar(m.matrix().getDouble(0, 0)));
    flattened->set(1, SkDoubleToScalar(m.matrix().getDouble(0, 1)));
    flattened->set(2, SkDoubleToScalar(m.matrix().getDouble(0, 3)));
    flattened->set(3, SkDoubleToScalar(m.matrix().getDouble(1, 0)));
    flattened->set(4, SkDoubleToScalar(m.matrix().getDouble(1, 1)));
    flattened->set(5, SkDoubleToScalar(m.matrix().getDouble(1, 3)));
    flattened->set(6, SkDoubleToScalar(m.matrix().getDouble(3, 0)));
    flattened->set(7, SkDoubleToScalar(m.matrix().getDouble(3, 1)));
    flattened->set(8, SkDoubleToScalar(m.matrix().getDouble(3, 3)));
}

bool isScaleAndTranslate(const SkMatrix& matrix)
{
    return SkScalarNearlyZero(matrix[SkMatrix::kMSkewX]) &&
           SkScalarNearlyZero(matrix[SkMatrix::kMSkewY]) &&
           SkScalarNearlyZero(matrix[SkMatrix::kMPersp0]) &&
           SkScalarNearlyZero(matrix[SkMatrix::kMPersp1]) &&
           SkScalarNearlyZero(matrix[SkMatrix::kMPersp2] - 1.0f);
}

} // anonymous namespace

scoped_ptr<SoftwareRenderer> SoftwareRenderer::create(RendererClient* client, OutputSurface* outputSurface, ResourceProvider* resourceProvider)
{
    return make_scoped_ptr(new SoftwareRenderer(client, outputSurface, resourceProvider));
}

SoftwareRenderer::SoftwareRenderer(RendererClient* client,
                                   OutputSurface* outputSurface,
                                   ResourceProvider* resourceProvider)
    : DirectRenderer(client, resourceProvider)
    , m_outputSurface(outputSurface)
    , m_visible(true)
    , m_isScissorEnabled(false)
    , m_outputDevice(outputSurface->software_device())
    , m_skCurrentCanvas(0)
{
    m_resourceProvider->setDefaultResourceType(ResourceProvider::Bitmap);

    m_capabilities.maxTextureSize = m_resourceProvider->maxTextureSize();
    m_capabilities.bestTextureFormat = m_resourceProvider->bestTextureFormat();
    m_capabilities.usingSetVisibility = true;
    // The updater can access bitmaps while the SoftwareRenderer is using them.
    m_capabilities.allowPartialTextureUpdates = true;
    m_capabilities.usingPartialSwap = true;
    if (m_client->hasImplThread())
      m_capabilities.usingSwapCompleteCallback = true;
    m_compositorFrame.software_frame_data.reset(new SoftwareFrameData());

    viewportChanged();
}

SoftwareRenderer::~SoftwareRenderer()
{
}

const RendererCapabilities& SoftwareRenderer::capabilities() const
{
    return m_capabilities;
}

void SoftwareRenderer::viewportChanged()
{
    m_outputDevice->Resize(viewportSize());
}

void SoftwareRenderer::beginDrawingFrame(DrawingFrame& frame)
{
    TRACE_EVENT0("cc", "SoftwareRenderer::beginDrawingFrame");
    m_skRootCanvas = m_outputDevice->BeginPaint(
        gfx::ToEnclosingRect(frame.rootDamageRect));
}

void SoftwareRenderer::finishDrawingFrame(DrawingFrame& frame)
{
    TRACE_EVENT0("cc", "SoftwareRenderer::finishDrawingFrame");
    m_currentFramebufferLock.reset();
    m_skCurrentCanvas = NULL;
    m_skRootCanvas = NULL;
    if (settings().compositorFrameMessage) {
        m_compositorFrame.metadata = m_client->makeCompositorFrameMetadata();
        m_outputDevice->EndPaint(m_compositorFrame.software_frame_data.get());
    } else {
        m_outputDevice->EndPaint();
    }
}

bool SoftwareRenderer::swapBuffers()
{
    if (settings().compositorFrameMessage)
        m_outputSurface->SendFrameToParentCompositor(&m_compositorFrame);
    return true;
}

void SoftwareRenderer::receiveCompositorFrameAck(const CompositorFrameAck& ack)
{
    if (m_client->hasImplThread())
        m_client->onSwapBuffersComplete();
    m_outputDevice->ReclaimDIB(ack.last_content_dib);
}

bool SoftwareRenderer::flippedFramebuffer() const
{
    return false;
}

void SoftwareRenderer::ensureScissorTestEnabled()
{
    m_isScissorEnabled = true;
    setClipRect(m_scissorRect);
}

void SoftwareRenderer::ensureScissorTestDisabled()
{
    // There is no explicit notion of enabling/disabling scissoring in software
    // rendering, but the underlying effect we want is to clear any existing
    // clipRect on the current SkCanvas. This is done by setting clipRect to
    // the viewport's dimensions.
    m_isScissorEnabled = false;
    SkDevice* device = m_skCurrentCanvas->getDevice();
    setClipRect(gfx::Rect(device->width(), device->height()));
}

void SoftwareRenderer::finish()
{
}

void SoftwareRenderer::bindFramebufferToOutputSurface(DrawingFrame& frame)
{
    m_currentFramebufferLock.reset();
    m_skCurrentCanvas = m_skRootCanvas;
}

bool SoftwareRenderer::bindFramebufferToTexture(DrawingFrame& frame, const ScopedResource* texture, const gfx::Rect& framebufferRect)
{
    m_currentFramebufferLock = make_scoped_ptr(new ResourceProvider::ScopedWriteLockSoftware(m_resourceProvider, texture->id()));
    m_skCurrentCanvas = m_currentFramebufferLock->skCanvas();
    initializeMatrices(frame, framebufferRect, false);
    setDrawViewportSize(framebufferRect.size());

    return true;
}

void SoftwareRenderer::setScissorTestRect(const gfx::Rect& scissorRect)
{
    m_isScissorEnabled = true;
    m_scissorRect = scissorRect;
    setClipRect(scissorRect);
}

void SoftwareRenderer::setClipRect(const gfx::Rect& rect)
{
    // Skia applies the current matrix to clip rects so we reset it temporary.
    SkMatrix currentMatrix = m_skCurrentCanvas->getTotalMatrix();
    m_skCurrentCanvas->resetMatrix();
    m_skCurrentCanvas->clipRect(gfx::RectToSkRect(rect), SkRegion::kReplace_Op);
    m_skCurrentCanvas->setMatrix(currentMatrix);
}

void SoftwareRenderer::clearCanvas(SkColor color)
{
    // SkCanvas::clear doesn't respect the current clipping region
    // so we SkCanvas::drawColor instead if scissoring is active.
    if (m_isScissorEnabled)
        m_skCurrentCanvas->drawColor(color, SkXfermode::kSrc_Mode);
    else
        m_skCurrentCanvas->clear(color);
}

void SoftwareRenderer::clearFramebuffer(DrawingFrame& frame)
{
    if (frame.currentRenderPass->has_transparent_background) {
        clearCanvas(SkColorSetARGB(0, 0, 0, 0));
    } else {
#ifndef NDEBUG
        // On DEBUG builds, opaque render passes are cleared to blue to easily see regions that were not drawn on the screen.
        clearCanvas(SkColorSetARGB(255, 0, 0, 255));
#endif
    }
}

void SoftwareRenderer::setDrawViewportSize(const gfx::Size& viewportSize)
{
}

bool SoftwareRenderer::isSoftwareResource(ResourceProvider::ResourceId id) const
{
    switch (m_resourceProvider->resourceType(id)) {
    case ResourceProvider::GLTexture:
        return false;
    case ResourceProvider::Bitmap:
        return true;
    }

    LOG(FATAL) << "Invalid resource type.";
    return false;
}

void SoftwareRenderer::drawQuad(DrawingFrame& frame, const DrawQuad* quad)
{
    TRACE_EVENT0("cc", "SoftwareRenderer::drawQuad");
    gfx::Transform quadRectMatrix;
    quadRectTransform(&quadRectMatrix, quad->quadTransform(), quad->rect);
    gfx::Transform contentsDeviceTransform = frame.windowMatrix * frame.projectionMatrix * quadRectMatrix;
    contentsDeviceTransform.FlattenTo2d();
    SkMatrix skDeviceMatrix;
    toSkMatrix(&skDeviceMatrix, contentsDeviceTransform);
    m_skCurrentCanvas->setMatrix(skDeviceMatrix);

    m_skCurrentPaint.reset();
    if (!isScaleAndTranslate(skDeviceMatrix)) {
      m_skCurrentPaint.setAntiAlias(true);
      m_skCurrentPaint.setFilterBitmap(true);
    }

    if (quad->ShouldDrawWithBlending()) {
        m_skCurrentPaint.setAlpha(quad->opacity() * 255);
        m_skCurrentPaint.setXfermodeMode(SkXfermode::kSrcOver_Mode);
    } else {
        m_skCurrentPaint.setXfermodeMode(SkXfermode::kSrc_Mode);
    }

    switch (quad->material) {
    case DrawQuad::DEBUG_BORDER:
        drawDebugBorderQuad(frame, DebugBorderDrawQuad::MaterialCast(quad));
        break;
    case DrawQuad::SOLID_COLOR:
        drawSolidColorQuad(frame, SolidColorDrawQuad::MaterialCast(quad));
        break;
    case DrawQuad::TEXTURE_CONTENT:
        drawTextureQuad(frame, TextureDrawQuad::MaterialCast(quad));
        break;
    case DrawQuad::TILED_CONTENT:
        drawTileQuad(frame, TileDrawQuad::MaterialCast(quad));
        break;
    case DrawQuad::RENDER_PASS:
        drawRenderPassQuad(frame, RenderPassDrawQuad::MaterialCast(quad));
        break;
    default:
        drawUnsupportedQuad(frame, quad);
        break;
    }

    m_skCurrentCanvas->resetMatrix();
}

void SoftwareRenderer::drawDebugBorderQuad(const DrawingFrame& frame, const DebugBorderDrawQuad* quad)
{
    // We need to apply the matrix manually to have pixel-sized stroke width.
    SkPoint vertices[4];
    gfx::RectFToSkRect(quadVertexRect()).toQuad(vertices);
    SkPoint transformedVertices[4];
    m_skCurrentCanvas->getTotalMatrix().mapPoints(transformedVertices, vertices, 4);
    m_skCurrentCanvas->resetMatrix();

    m_skCurrentPaint.setColor(quad->color);
    m_skCurrentPaint.setAlpha(quad->opacity() * SkColorGetA(quad->color));
    m_skCurrentPaint.setStyle(SkPaint::kStroke_Style);
    m_skCurrentPaint.setStrokeWidth(quad->width);
    m_skCurrentCanvas->drawPoints(SkCanvas::kPolygon_PointMode, 4, transformedVertices, m_skCurrentPaint);
}

void SoftwareRenderer::drawSolidColorQuad(const DrawingFrame& frame, const SolidColorDrawQuad* quad)
{
    m_skCurrentPaint.setColor(quad->color);
    m_skCurrentPaint.setAlpha(quad->opacity() * SkColorGetA(quad->color));
    m_skCurrentCanvas->drawRect(gfx::RectFToSkRect(quadVertexRect()), m_skCurrentPaint);
}

void SoftwareRenderer::drawTextureQuad(const DrawingFrame& frame, const TextureDrawQuad* quad)
{
    if (!isSoftwareResource(quad->resource_id)) {
        drawUnsupportedQuad(frame, quad);
        return;
    }

    // FIXME: Add support for non-premultiplied alpha.
    ResourceProvider::ScopedReadLockSoftware lock(m_resourceProvider, quad->resource_id);
    const SkBitmap* bitmap = lock.skBitmap();
    gfx::RectF uvRect = gfx::ScaleRect(gfx::BoundingRect(quad->uv_top_left, quad->uv_bottom_right),
                                       bitmap->width(),
                                       bitmap->height());
    SkRect skUvRect = gfx::RectFToSkRect(uvRect);
    if (quad->flipped)
        m_skCurrentCanvas->scale(1, -1);
    m_skCurrentCanvas->drawBitmapRectToRect(*bitmap, &skUvRect,
                                            gfx::RectFToSkRect(quadVertexRect()),
                                            &m_skCurrentPaint);
}

void SoftwareRenderer::drawTileQuad(const DrawingFrame& frame, const TileDrawQuad* quad)
{
    DCHECK(isSoftwareResource(quad->resource_id));
    ResourceProvider::ScopedReadLockSoftware lock(m_resourceProvider, quad->resource_id);

    SkRect uvRect = gfx::RectFToSkRect(quad->tex_coord_rect);
    m_skCurrentPaint.setFilterBitmap(true);
    m_skCurrentCanvas->drawBitmapRectToRect(*lock.skBitmap(), &uvRect,
                                            gfx::RectFToSkRect(quadVertexRect()),
                                            &m_skCurrentPaint);
}

void SoftwareRenderer::drawRenderPassQuad(const DrawingFrame& frame, const RenderPassDrawQuad* quad)
{
    CachedResource* contentTexture = m_renderPassTextures.get(quad->render_pass_id);
    if (!contentTexture || !contentTexture->id())
        return;

    DCHECK(isSoftwareResource(contentTexture->id()));
    ResourceProvider::ScopedReadLockSoftware lock(m_resourceProvider, contentTexture->id());

    SkRect destRect = gfx::RectFToSkRect(quadVertexRect());
    SkRect contentRect = SkRect::MakeWH(quad->rect.width(), quad->rect.height());

    SkMatrix contentMat;
    contentMat.setRectToRect(contentRect, destRect, SkMatrix::kFill_ScaleToFit);

    const SkBitmap* content = lock.skBitmap();
    skia::RefPtr<SkShader> shader = skia::AdoptRef(
        SkShader::CreateBitmapShader(*content,
                                     SkShader::kClamp_TileMode,
                                     SkShader::kClamp_TileMode));
    shader->setLocalMatrix(contentMat);
    m_skCurrentPaint.setShader(shader.get());

    SkImageFilter* filter = quad->filter.get();
    if (filter)
        m_skCurrentPaint.setImageFilter(filter);

    if (quad->mask_resource_id) {
        ResourceProvider::ScopedReadLockSoftware maskLock(m_resourceProvider, quad->mask_resource_id);

        const SkBitmap* mask = maskLock.skBitmap();

        SkRect maskRect = SkRect::MakeXYWH(
            quad->mask_uv_rect.x() * mask->width(),
            quad->mask_uv_rect.y() * mask->height(),
            quad->mask_uv_rect.width() * mask->width(),
            quad->mask_uv_rect.height() * mask->height());

        SkMatrix maskMat;
        maskMat.setRectToRect(maskRect, destRect, SkMatrix::kFill_ScaleToFit);

        skia::RefPtr<SkShader> maskShader = skia::AdoptRef(
            SkShader::CreateBitmapShader(*mask,
                                         SkShader::kClamp_TileMode,
                                         SkShader::kClamp_TileMode));
        maskShader->setLocalMatrix(maskMat);

        SkPaint maskPaint;
        maskPaint.setShader(maskShader.get());

        skia::RefPtr<SkLayerRasterizer> maskRasterizer = skia::AdoptRef(new SkLayerRasterizer);
        maskRasterizer->addLayer(maskPaint);

        m_skCurrentPaint.setRasterizer(maskRasterizer.get());
        m_skCurrentCanvas->drawRect(destRect, m_skCurrentPaint);
    } else {
        // FIXME: Apply background filters and blend with content
        m_skCurrentCanvas->drawRect(destRect, m_skCurrentPaint);
    }
}

void SoftwareRenderer::drawUnsupportedQuad(const DrawingFrame& frame, const DrawQuad* quad)
{
    m_skCurrentPaint.setColor(SK_ColorMAGENTA);
    m_skCurrentPaint.setAlpha(quad->opacity() * 255);
    m_skCurrentCanvas->drawRect(gfx::RectFToSkRect(quadVertexRect()), m_skCurrentPaint);
}

void SoftwareRenderer::getFramebufferPixels(void *pixels, const gfx::Rect& rect)
{
    TRACE_EVENT0("cc", "SoftwareRenderer::getFramebufferPixels");
    SkBitmap subsetBitmap;
    m_outputDevice->CopyToBitmap(rect, &subsetBitmap);
    subsetBitmap.copyPixelsTo(pixels,
                              4 * rect.width() * rect.height(),
                              4 * rect.width());
}

void SoftwareRenderer::setVisible(bool visible)
{
    if (m_visible == visible)
        return;
    m_visible = visible;
}

}  // namespace cc
