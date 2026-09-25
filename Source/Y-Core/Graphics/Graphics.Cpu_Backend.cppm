export module ClaFi.Core.Graphics.Cpu.Canvas;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{
    struct SavedClip
    {
        FloatRect bounds;
        bool hasGeometricClip{ false };
        const NoAllocFloatVector* clipMask{ nullptr };
        int clipStride{ 0 };
        FloatRect clipBox;
        std::size_t prevPoolIndex{ 0 };
    };

    // An opacity layer standing open: the buffer the painters are writing into, the part of it
    // they may reach, the surface it is composited back onto and the strength that composite is
    // taken at.
    struct OpacityLayer
    {
        Bitmap* buffer{ nullptr };
        Bitmap* previousTarget{ nullptr };
        FloatRect bounds{};
        float opacity{ 1.0f };
    };

    export class CpuBackend : public IBackend
    {
    public:
        explicit CpuBackend(IntSize);
        ~CpuBackend() override = default;

        [[nodiscard]] BackendType type() const override { return BackendType::Cpu; }
        [[nodiscard]] IntSize size() const override { return m_size; }
        [[nodiscard]] FloatRect bounds() const;

        void resize(IntSize) override;
        void setTransparentBase(bool value) override { m_transparentBase = value; }

        void beginPaint(void* nativeContext, const IntRect* dirtyRect) override;
        void endPaint(void* nativeContext, const IntRect* dirtyRect, Bitmap*& outData) override;

        void pushClip(const FloatRect& exactClipRect) override;
        void pushClip(const PixelPath& path, const Matrix3x2* transform = nullptr) override;
        void popClip() override;

        void pushOpacity(float opacity) override;
        void popOpacity() override;

        void pushImage(int index) override;
        void popImage() override;
        void drawImage(int index, float opacity) override;
        [[nodiscard]] bool hasImage(int index) const override;

        void* getNativeRenderTarget() const override { return nullptr; }
        void* getNativeBrush(const Brush&) override { return nullptr; }

        // Only snapOrigins is honoured. This path averages DirectWrite's 3x1 ClearType texture down
        // to a single coverage value per pixel whatever is asked of it, so it is grayscale already
        // and an antialias of Subpixel costs it nothing to ignore - drawing subpixel here would mean
        // keeping the three channels apart and compositing them separately, which the rasterizer's
        // blend loop does not do yet. Grid fit, contrast and the rasterizer belong to DirectWrite's
        // own rasterization, which this path replaces rather than configures.
        [[nodiscard]] TextAntialiasToken beginTextRaster(const TextRasterizationParams& params) override;
        void endTextRaster(TextAntialiasToken) override { m_snapTextOrigins = true; }
        [[nodiscard]] bool snapTextOrigins() const override { return m_snapTextOrigins; }

        void setTransform(const Matrix3x2& matrix) override { m_transform = matrix; }
        void resetTransform() override { m_transform = Matrix3x2::identity(); }

        // The glyph route reaches the canvas transform on its own: FadeTextRenderer maps each
        // glyph's pen position through it and resolves the fraction by sampling the cached
        // coverage, so a run follows its control without this needing to move the rect. A glyph
        // itself is not resized - the cached coverage is a fixed size - which leaves a glyph body's
        // worth of unscaled width and nothing more.
        //
        // The view is a live window onto the back buffer, not a cleared scratch buffer as it is
        // under Direct2D. Painters here write through to the surface, which is why the commit below
        // has nothing to do - and why FadeTextRenderer can take a view straight off the backend and
        // never commit it. It is the only reader.
        PixelView stagingView(const FloatRect& requestedRect) override { return getActiveView().subView(requestedRect); }
        void commitStagingView(const PixelView&) override {}
        void drawPixelView(const PixelView& view) override { getActiveView().drawSurface(view, view.topLeft()); }

        // ====================================================================
        // CLEAN ROUTING PASS-THROUGHS TO RE-DECOUPLED MODULAR PAINTERS
        // ====================================================================
        // Direct2D carries the canvas transform on the render target, so every primitive it issues
        // is mapped for free. These painters rasterize into a pixel buffer with no such stage, so
        // the transform is folded into the geometry here, once per entry point. The matrix is still
        // handed to the painters afterwards, but only so they can place brush coordinates - they
        // must not apply it to geometry as well, or the shape moves twice.
        //
        // Everything routed through drawPath or fillPath is exempt: PixelPathPainter bakes the
        // matrix into the points itself and scales stroke widths with it, so those paths are mapped
        // exactly once already.

        void drawLine(FloatPoint pt1, FloatPoint pt2, const Brush& brush, float strokeWidth) override;
        void fillRectangle(const FloatRect& rect, const Brush& brush) override;
        void drawRectangle(const FloatRect& rect, const Brush& brush, float strokeWidth) override;
        void fillRoundedRectangle(const FloatRect& rect, float rx, float ry, const Brush& brush) override;
        void drawRoundedRectangle(const FloatRect& rect, float rx, float ry, const Brush& brush, float strokeWidth) override;
        void fillEllipse(FloatPoint center, float rx, float ry, const Brush& brush) override;
        void drawEllipse(FloatPoint center, float rx, float ry, const Brush& brush, float strokeWidth) override;
        bool fillPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush) override;
        bool drawPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush, float strokeWidth) override;
        // The shared construction asks for a glow, and a glow here is one pass over a distance
        // field. There is nothing a shadow drawn in one piece would improve on.
        bool castShadow(const RoundedRectangleParts&, const ShadowParams&) override { return false; }

        // ====================================================================
        // STANDARD VECTOR FALLBACKS
        // ====================================================================

        void fillPath(const PixelPath& path, const Brush& brush, const Matrix3x2* transform = nullptr) override;
        void drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform = nullptr) override;

    private:
        void drawPathWithBrush(const PixelPath& path, const Brush& brush, float strokeWidth, const Matrix3x2* transform = nullptr) const;
        // Maps a rect out of the caller's coordinates and into the buffer's. The painters
        // downstream rasterize axis-aligned spans and cannot express a rotated edge either way, so
        // Matrix3x2::mapRect losing rotation costs nothing that was reachable here - a rotated
        // control has to go through drawPath.
        //
        // An identity matrix maps every coordinate back to itself bit for bit - the multiplications
        // are by exactly 1.0 and 0.0 - so the untransformed case keeps hitting the integer-snapped
        // fast path in RectPainter.
        [[nodiscard]] FloatRect mapRect(const FloatRect& rect) const { return m_transform.mapRect(rect); }

        // Radii, stroke widths and any other distance have to follow the same scale, or a shrinking
        // control keeps a border that grows relative to it.
        [[nodiscard]] float mapLength(float value) const { return value * m_transform.getScaleFactor(); }

        [[nodiscard]] RoundedRectangleParts mapParts(const RoundedRectangleParts& parts) const;

        void applyClipStateToGlobal();
        void updateActiveView();
        // drawImage under a path clip: the image mixed in by the clip's coverage, pixel by pixel.
        void blendImageThroughClip(const PixelView& source, float opacity) const;

        [[nodiscard]] PixelView getActiveView() const { return m_activeView; }

        [[nodiscard]] NoAllocFloatVector* rentMask();
        [[nodiscard]] Bitmap* rentLayerBuffer();
        // The kept image at this index, built at the surface's size if it is not there at that
        // size already. Null for an index that names none, which a caller answers by drawing to
        // the surface instead.
        [[nodiscard]] Bitmap* image(int index);

    private:
        IntSize m_size;
        // Whether each frame is cleared before it is painted - see beginPaint.
        bool m_transparentBase{ false };
        Bitmap m_backBuffer;
        // What the painters write into: the back buffer, or the innermost opacity layer's own
        // buffer while one is open. Every painter reaches its pixels through getActiveView, so
        // this is the whole of the redirection.
        Bitmap* m_target{ &m_backBuffer };
        bool m_snapTextOrigins{ true };
        Matrix3x2 m_transform{ Matrix3x2::identity() };
        std::vector<SavedClip> m_clipStack;
        PixelPath m_scratchPath;
        PixelView m_activeView{ {}, {}, {} };

        std::vector<std::unique_ptr<NoAllocFloatVector>> m_clipMaskPool;
        std::size_t m_poolIndex{ 0 };

        // A layer buffer is the size of the whole surface and is held for reuse, the same way a
        // clip mask is: a window's worth of pixels is too much to allocate per frame, and a
        // layer is opened at most a handful of times in one.
        std::vector<std::unique_ptr<Bitmap>> m_layerBufferPool;
        std::size_t m_layerBufferIndex{ 0 };
        std::vector<OpacityLayer> m_opacityLayers;

        // The kept images, and the targets the pushes that opened them were redirected from. Held
        // by unique_ptr so that growing the store cannot move an image an open push is writing
        // into. A resize empties the store: an image is the size of the surface it was drawn for.
        std::vector<std::unique_ptr<Bitmap>> m_images;
        std::vector<Bitmap*> m_imageTargets;
    };
}
