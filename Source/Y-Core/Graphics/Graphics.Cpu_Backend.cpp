module;
#include <immintrin.h>
module ClaFi.Core.Graphics.Cpu.Canvas;

import ClaFi.Core.Graphics.Cpu_Painter_Circle;
import ClaFi.Core.Graphics.Cpu_Painter_Rect;
import ClaFi.Core.Graphics.Cpu_Painter_RoundedRect;

import ClaFi.Core.Graphics.Cpu_PathPainter;
import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{

    CpuBackend::CpuBackend(IntSize size)
        :
        m_size{ size }
    {
        CpuBackend::resize(size);
    }

    FloatRect CpuBackend::bounds() const
    {
        return {
            0.0f,
            0.0f,
            static_cast<float>(m_size.x),
            static_cast<float>(m_size.y),
        };
    }

    void CpuBackend::resize(IntSize size)
    {
        m_size = size;
        m_backBuffer.resize(size);
        // An image is the size of the surface it was drawn for, so none of them means anything
        // now. Emptied rather than resized: what a caller drew at the old size is gone either way,
        // and hasImage is how it finds that out.
        m_images.clear();
        updateActiveView();
    }

    void CpuBackend::beginPaint(void* /* nativeContext */, const IntRect* dirtyRect)
    {
        m_clipStack.clear();
        m_transform = Matrix3x2::identity();
        m_poolIndex = 0;
        m_opacityLayers.clear();
        m_layerBufferIndex = 0;
        m_imageTargets.clear();
        m_target = &m_backBuffer;

        SavedClip baseClip;
        baseClip.bounds = dirtyRect ? dirtyRect->toFloat() : bounds();
        m_clipStack.push_back(baseClip);

        applyClipStateToGlobal();
        updateActiveView();

        // A FRAME THAT CARRIES ALPHA STARTS FROM NOTHING. Every other frame keeps what the last
        // one left and relies on the children covering every pixel, which costs nothing and is
        // invisible for as long as the covering holds. It does not hold where a window has a
        // rounded corner: the pixels outside the curve are the ones nothing draws, and they have
        // to read as absent rather than as whatever stood there two frames ago.
        //
        // ONLY THE DIRTY RECT, because that is what this pass will paint and the clip already
        // states it.
        if (m_transparentBase)
            m_activeView.fill(k_noColor);
    }

    void CpuBackend::endPaint(void* /* nativeContext */, const IntRect* /* dirtyRect */, Bitmap*& outData)
    {
        m_clipStack.clear();
        m_poolIndex = 0;
        m_opacityLayers.clear();
        m_layerBufferIndex = 0;
        m_imageTargets.clear();
        m_target = &m_backBuffer;
        applyClipStateToGlobal();
        updateActiveView();
        outData = &m_backBuffer;
    }

    void CpuBackend::pushClip(const FloatRect& exactClipRect)
    {
        // The rect arrives in the caller's coordinates, the same ones the primitives below are
        // given. Direct2D reads PushAxisAlignedClip in the render target's current transform
        // space, so mapping here is what keeps a child clipped to the same pixels under either
        // backend while an ancestor is mid-animation.
        FloatRect currentBounds = m_clipStack.empty() ? bounds() : m_clipStack.back().bounds;
        currentBounds.intersectWith(mapRect(exactClipRect));

        SavedClip newClip;
        newClip.bounds = currentBounds;
        newClip.prevPoolIndex = m_poolIndex;

        if (!m_clipStack.empty() && m_clipStack.back().hasGeometricClip)
        {
            const SavedClip& parent = m_clipStack.back();
            newClip.hasGeometricClip = true;
            newClip.clipMask = parent.clipMask;
            newClip.clipStride = parent.clipStride;
            newClip.clipBox = parent.clipBox;
        }

        m_clipStack.push_back(newClip);
        applyClipStateToGlobal();
        updateActiveView();
    }

    void CpuBackend::pushClip(const PixelPath& path, const Matrix3x2* transform)
    {
        bool wasActive = g_rasterBuffers.hasActiveClip;
        g_rasterBuffers.hasActiveClip = false;

        // Same reasoning as the rect overload, except the path rasterizer can take the matrix
        // directly, so it is composed rather than baked in beforehand.
        Matrix3x2 finalTransform = transform ? (m_transform * (*transform)) : m_transform;

        PixelPathPainter painter{ m_target->pixelView() };
        painter.setClipPath(path, &finalTransform);

        g_rasterBuffers.hasActiveClip = wasActive;

        SavedClip newClip;
        FloatRect currentBounds = m_clipStack.empty() ? bounds() : m_clipStack.back().bounds;

        newClip.hasGeometricClip = true;
        newClip.clipBox = g_rasterBuffers.clipBox;
        newClip.clipStride = g_rasterBuffers.m_clipStride;
        newClip.prevPoolIndex = m_poolIndex;

        NoAllocFloatVector* rentedMask = rentMask();
        std::swap(*rentedMask, g_rasterBuffers.clipMask);
        newClip.clipMask = rentedMask;

        newClip.bounds = currentBounds;
        newClip.bounds.intersectWith(newClip.clipBox);

        if (!m_clipStack.empty() && m_clipStack.back().hasGeometricClip)
        {
            const SavedClip& parent = m_clipStack.back();
            FloatRect combinedBox = FloatRect::intersection(parent.clipBox, newClip.clipBox);

            if (combinedBox.empty())
            {
                newClip.clipMask = nullptr;
                newClip.clipStride = 0;
                newClip.clipBox = {};
            }
            else
            {
                int startX = static_cast<int>(std::floor(combinedBox.left));
                int endX = static_cast<int>(std::ceil(combinedBox.right));
                int startY = static_cast<int>(std::floor(combinedBox.top));
                int endY = static_cast<int>(std::ceil(combinedBox.bottom));

                int width = endX - startX;
                int height = endY - startY;
                int stride = (width + 7) & ~7;

                NoAllocFloatVector* combinedMask = rentMask();
                combinedMask->resize(static_cast<std::size_t>(stride) * height + 8);

                int parentLeft = static_cast<int>(std::floor(parent.clipBox.left));
                int parentTop = static_cast<int>(std::floor(parent.clipBox.top));
                int parentStride = parent.clipStride;
                const NoAllocFloatVector& parentVec = *parent.clipMask;

                int newLeft = static_cast<int>(std::floor(newClip.clipBox.left));
                int newTop = static_cast<int>(std::floor(newClip.clipBox.top));
                int newStride = newClip.clipStride;
                const NoAllocFloatVector& newVec = *newClip.clipMask;

                for (int y = startY; y < endY; ++y)
                {
                    float* dstRow = &(*combinedMask)[(y - startY) * stride];
                    int parentY = y - parentTop;
                    int newY = y - newTop;

                    const float* parentRow = &parentVec[parentY * parentStride + (startX - parentLeft)];
                    const float* newRow = &newVec[newY * newStride + (startX - newLeft)];

                    int x = 0;
                    for (; x <= width - 8; x += 8)
                    {
                        __m256 vParent = _mm256_loadu_ps(parentRow + x);
                        __m256 vNew = _mm256_loadu_ps(newRow + x);
                        _mm256_storeu_ps(dstRow + x, _mm256_mul_ps(vParent, vNew));
                    }

                    for (; x < width; ++x)
                    {
                        dstRow[x] = parentRow[x] * newRow[x];
                    }
                }

                newClip.clipMask = combinedMask;
                newClip.clipStride = stride;
                newClip.clipBox = combinedBox;
            }
        }

        m_clipStack.push_back(newClip);
        applyClipStateToGlobal();
        updateActiveView();
    }

    void CpuBackend::popClip()
    {
        if (!m_clipStack.empty())
        {
            m_poolIndex = m_clipStack.back().prevPoolIndex;
            m_clipStack.pop_back();
        }

        applyClipStateToGlobal();
        updateActiveView();
    }

    // The layer starts as a copy of the surface it will be composited back onto, not as empty
    // pixels. Every painter here mixes toward its own colour by coverage, which is the arithmetic
    // of drawing on something opaque: on empty pixels the same arithmetic leaves colours already
    // multiplied by their coverage, and mixing those back in as ordinary pixels darkens every
    // antialiased edge in the layer. Starting from what is underneath keeps the surface under the
    // painters opaque, so a colour written inside a layer means what it means outside one, and the
    // composite in popOpacity is a plain mix of two opaque images.
    //
    // A layer buffer spans the whole surface rather than the clip it will be used within, so a
    // pixel keeps the address it would have had without the layer. Everything downstream - the
    // painters, the glyph renderer taking a staging view, the geometric clip mask in
    // g_rasterBuffers - works in absolute coordinates, and a buffer placed at the clip's corner
    // would move all of them.
    void CpuBackend::pushOpacity(float opacity)
    {
        Bitmap* buffer = rentLayerBuffer();
        FloatRect layerBounds = m_clipStack.empty() ? bounds() : m_clipStack.back().bounds;

        // Only the part that may be drawn into is copied, and it is the same part popOpacity
        // mixes back. What a previous layer left in the rest of the buffer is never read.
        buffer->pixelView().subView(layerBounds).copyFrom(getActiveView());

        m_opacityLayers.push_back({
            .buffer = buffer,
            .previousTarget = m_target,
            .bounds = layerBounds,
            .opacity = opacity,
        });
        m_target = buffer;
        updateActiveView();
    }

    void CpuBackend::popOpacity()
    {
        if (m_opacityLayers.empty())
        {
            return;
        }

        OpacityLayer layer = m_opacityLayers.back();
        m_opacityLayers.pop_back();
        --m_layerBufferIndex;

        // The surface underneath is what the layer is composited onto, so the target and the view
        // over it are put back first. The clip stack is where the push left it - a layer is held
        // open across a passage that balances its own clips - so the view is the one the push saw.
        m_target = layer.previousTarget;
        updateActiveView();

        PixelView source = layer.buffer->pixelView().subView(layer.bounds);
        getActiveView().blendSurface(source, source.topLeft(), layer.opacity);
    }

    // The same redirection an opacity layer makes, and for the same reason - an image spans the
    // whole surface so a pixel keeps the address it would have had without it. What it does not
    // do is clear: an image is written by a caller that covers it, and clearing would cost a
    // window's worth of writes on every frame of a crossfade.
    void CpuBackend::pushImage(int index)
    {
        // The target to go back to is recorded whether or not there is an image to redirect to, so
        // a push that could not build one still balances its pop and draws to the surface
        // meanwhile rather than losing what it was about to draw.
        m_imageTargets.push_back(m_target);
        if (Bitmap* target = image(index))
        {
            m_target = target;
            updateActiveView();
        }
    }

    void CpuBackend::popImage()
    {
        if (m_imageTargets.empty())
        {
            return;
        }

        m_target = m_imageTargets.back();
        m_imageTargets.pop_back();
        updateActiveView();
    }

    void CpuBackend::drawImage(int index, float opacity)
    {
        if (!hasImage(index))
        {
            return;
        }

        PixelView source = m_images[static_cast<std::size_t>(index)]->pixelView().subView(m_activeView.bounds());
        // A surface blend knows nothing of a path clip, and an image is the one thing drawn here
        // that is not a painter: a form's corner pass composites a crossing's images under the
        // corner's clip, and what the images hold outside the arc has to stay out.
        if (g_rasterBuffers.hasActiveClip && g_rasterBuffers.activeClipMaskPtr)
        {
            blendImageThroughClip(source, opacity);
            return;
        }
        m_activeView.blendSurface(source, source.topLeft(), opacity);
    }

    bool CpuBackend::hasImage(int index) const
    {
        if (index < 0 || static_cast<std::size_t>(index) >= m_images.size())
        {
            return false;
        }

        const Bitmap& stored = *m_images[static_cast<std::size_t>(index)];
        return stored.width() == m_size.x && stored.height() == m_size.y;
    }

    TextAntialiasToken CpuBackend::beginTextRaster(const TextRasterizationParams& params)
    {
        m_snapTextOrigins = params.snapOrigins;
        return {};
    }

    void CpuBackend::drawLine(FloatPoint pt1, FloatPoint pt2, const Brush& brush, float strokeWidth)
    {
        m_scratchPath.clear();
        m_scratchPath.moveTo(pt1);
        m_scratchPath.lineTo(pt2);
        drawPathWithBrush(m_scratchPath, brush, strokeWidth);
    }

    void CpuBackend::fillRectangle(const FloatRect& rect, const Brush& brush)
    {
        RectPainter painter{ getActiveView() };
        painter.paintSolid(mapRect(rect), brush, 1.0f, &m_transform);
    }

    void CpuBackend::drawRectangle(const FloatRect& rect, const Brush& brush, float strokeWidth)
    {
        RectPainter painter{ getActiveView() };
        painter.paintBorder(mapRect(rect), brush, mapLength(strokeWidth), 1.0f, &m_transform);
    }

    void CpuBackend::fillRoundedRectangle(const FloatRect& rect, float rx, float ry, const Brush& brush)
    {
        RoundedRectangleParts parts{
            .bounds = rect,
            .radii = uniformCorners((std::max)(rx, ry)),
            .sides = k_allRectSidesTrue
        };
        fillPartialRoundedRectangle(parts, brush);
    }

    void CpuBackend::drawRoundedRectangle(const FloatRect& rect, float rx, float ry, const Brush& brush, float strokeWidth)
    {
        RoundedRectangleParts parts{
            .bounds = rect,
            .radii = uniformCorners((std::max)(rx, ry)),
            .sides = k_allRectSidesTrue
        };
        drawPartialRoundedRectangle(parts, brush, strokeWidth);
    }

    void CpuBackend::fillEllipse(FloatPoint center, float rx, float ry, const Brush& brush)
    {
        if (rx == ry)
        {
            CirclePainter painter{ getActiveView() };
            painter.paintSolid(m_transform.transform(center), brush, mapLength(rx), &m_transform);
            return;
        }

        m_scratchPath.clear();
        m_scratchPath.drawCircle(center, (std::max)(rx, ry));
        fillPath(m_scratchPath, brush);
    }

    void CpuBackend::drawEllipse(FloatPoint center, float rx, float ry, const Brush& brush, float strokeWidth)
    {
        if (rx == ry)
        {
            CirclePainter painter{ getActiveView() };
            painter.paintRing(m_transform.transform(center), brush, mapLength(rx), mapLength(strokeWidth), &m_transform);
            return;
        }

        m_scratchPath.clear();
        m_scratchPath.drawCircle(center, (std::max)(rx, ry));
        drawPathWithBrush(m_scratchPath, brush, strokeWidth);
    }

    bool CpuBackend::fillPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush)
    {
        RoundedRectPainter painter{ getActiveView() };
        painter.fillPartial(mapParts(parts), brush, 1.0f, &m_transform);
        return true;
    }

    bool CpuBackend::drawPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush, float strokeWidth)
    {
        RoundedRectPainter painter{ getActiveView() };
        painter.drawPartial(mapParts(parts), brush, mapLength(strokeWidth), 1.0f, &m_transform);
        return true;
    }

    void CpuBackend::fillPath(const PixelPath& path, const Brush& brush, const Matrix3x2* transform)
    {
        PixelView targetView = getActiveView();
        PixelPathPainter painter{ targetView };

        PathDrawLayer layer{
            .geometry{.mode = PathRenderMode::Fill },
            .brush = brush
        };

        Matrix3x2 finalTransform = transform ? (m_transform * (*transform)) : m_transform;
        painter.drawPath(path, layer, finalTransform);
    }

    void CpuBackend::drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform)
    {
        PixelView targetView = getActiveView();
        PixelPathPainter painter{ targetView };

        Matrix3x2 finalTransform = transform ? (m_transform * (*transform)) : m_transform;
        painter.drawPath(path, layers, finalTransform);
    }

    void CpuBackend::drawPathWithBrush(const PixelPath& path, const Brush& brush, float strokeWidth, const Matrix3x2* transform) const
    {
        PixelView targetView = getActiveView();
        PixelPathPainter painter{ targetView };

        PathDrawLayer layer{
            .geometry{.mode = PathRenderMode::Stroke, .strokeWidth = strokeWidth },
            .brush = brush
        };

        Matrix3x2 finalTransform = transform ? (m_transform * (*transform)) : m_transform;
        painter.drawPath(path, layer, finalTransform);
    }

    RoundedRectangleParts CpuBackend::mapParts(const RoundedRectangleParts& parts) const
    {
        RoundedRectangleParts result = parts;
        result.bounds = mapRect(parts.bounds);
        for (float& radius : result.radii)
            radius = mapLength(radius);
        return result;
    }

    void CpuBackend::applyClipStateToGlobal()
    {
        if (m_clipStack.empty())
        {
            g_rasterBuffers.hasActiveClip = false;
            g_rasterBuffers.activeClipMaskPtr = nullptr;
            return;
        }

        const SavedClip& active = m_clipStack.back();
        if (!active.hasGeometricClip)
        {
            g_rasterBuffers.hasActiveClip = false;
            g_rasterBuffers.activeClipMaskPtr = nullptr;
            return;
        }

        g_rasterBuffers.hasActiveClip = true;
        g_rasterBuffers.clipBox = active.clipBox;
        g_rasterBuffers.m_clipStride = active.clipStride;
        g_rasterBuffers.activeClipMaskPtr = active.clipMask ? active.clipMask->data() : nullptr;
    }

    void CpuBackend::updateActiveView()
    {
        FloatRect clip = m_clipStack.empty() ? bounds() : m_clipStack.back().bounds;
        m_activeView = m_target->pixelView().subView(clip);
    }

    // An image and the view share their coordinates - both span the whole surface - so the same
    // absolute pixel is read from one and written to the other. The mask is indexed as the
    // rasterizer indexes it: from the clip box's floored corner, at the clip stride.
    void CpuBackend::blendImageThroughClip(const PixelView& source, float opacity) const
    {
        IntRect area = m_activeView.ownedPixels();
        if (!area.intersectWith(source.ownedPixels()))
            return;
        if (!area.intersectWith(m_activeView.clippedBounds().roundedOut()))
            return;

        const RasterBuffers& buffers = g_rasterBuffers;
        const int clipLeft = static_cast<int>(std::floor(buffers.clipBox.left));
        const int clipTop = static_cast<int>(std::floor(buffers.clipBox.top));
        const int clipRight = static_cast<int>(std::ceil(buffers.clipBox.right));
        const int clipBottom = static_cast<int>(std::ceil(buffers.clipBox.bottom));
        const float amount = std::clamp(opacity, 0.0f, 1.0f);

        for (int y = area.top; y < area.bottom; ++y)
        {
            Color* destRow = m_activeView.scanLineAbs(static_cast<float>(area.left), static_cast<float>(y));
            const Color* srcRow = source.scanLineAbs(static_cast<float>(area.left), static_cast<float>(y));
            const bool inClipRows = y >= clipTop && y < clipBottom;
            const float* clipRow = inClipRows
                ? buffers.activeClipMaskPtr + static_cast<std::ptrdiff_t>(y - clipTop) * buffers.m_clipStride
                : nullptr;
            for (int x = area.left; x < area.right; ++x)
            {
                const bool inClip = clipRow && x >= clipLeft && x < clipRight;
                const float coverage = inClip ? clipRow[x - clipLeft] : 0.0f;
                const int mix = static_cast<int>(amount * coverage * 256.0f);
                destRow[x - area.left].blend_fixed(srcRow[x - area.left], mix);
            }
        }
    }

    NoAllocFloatVector* CpuBackend::rentMask()
    {
        if (m_poolIndex >= m_clipMaskPool.size())
        {
            m_clipMaskPool.push_back(std::make_unique<NoAllocFloatVector>());
        }

        return m_clipMaskPool[m_poolIndex++].get();
    }

    Bitmap* CpuBackend::rentLayerBuffer()
    {
        if (m_layerBufferIndex >= m_layerBufferPool.size())
        {
            m_layerBufferPool.push_back(std::make_unique<Bitmap>());
        }

        Bitmap* buffer = m_layerBufferPool[m_layerBufferIndex++].get();
        buffer->resize(m_size);
        return buffer;
    }

    Bitmap* CpuBackend::image(int index)
    {
        if (index < 0)
        {
            return nullptr;
        }

        const std::size_t slot = static_cast<std::size_t>(index);
        while (m_images.size() <= slot)
        {
            m_images.push_back(std::make_unique<Bitmap>());
        }

        Bitmap& stored = *m_images[slot];
        if (stored.width() != m_size.x || stored.height() != m_size.y)
        {
            stored.resize(m_size);
            // A bitmap's pixels come from an allocator that leaves them as they were found, so
            // what a caller does not cover would be whatever the heap held. Cleared where the
            // storage is built and not on every push, which is where a window's worth of writes a
            // frame would go.
            stored.pixelView().fill(Color{});
        }
        return &stored;
    }

}
