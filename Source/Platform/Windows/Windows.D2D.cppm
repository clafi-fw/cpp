module;
#include "Windows.Headers.h"
export module ClaFi.Platform.Windows.D2D;

import ClaFi.Platform.Windows.Composition;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Windows
{
    using namespace Graphics;
    using Microsoft::WRL::ComPtr;

    // --- UNIFIED BRUSH HASHING ---
    struct BrushHash
    {
        std::size_t operator()(const Brush&) const;
    };

    // Satisfies the GPU backend contract: it renders into a window the platform owns. See Platform
    export class Direct2DBackend : public IBackend
    {
    public:
        Direct2DBackend(IPlatformWindow&, IntSize);
        ~Direct2DBackend() override = default;

    public:
        [[nodiscard]] BackendType type() const override { return BackendType::Direct2D; }
        [[nodiscard]] IntSize size() const override { return m_size; }
        void resize(IntSize) override;
        void pushClip(const FloatRect& exactClipRect) override;
        void pushClip(const PixelPath& path, const Matrix3x2* transform = nullptr) override;
        void popClip() override;
        void pushOpacity(float opacity) override;
        void popOpacity() override;
        void pushImage(int index) override;
        void popImage() override;
        void drawImage(int index, float opacity) override;
        [[nodiscard]] bool hasImage(int index) const override;
        void setTransparentBase(bool value) override { m_transparentBase = value; }
        void beginPaint(void* nativeContext, const IntRect* dirtyRect) override;
        void endPaint(void* nativeContext, const IntRect* dirtyRect, Bitmap*&) override;
        void* getNativeRenderTarget() const override { return m_renderTarget.Get(); }

        void* getNativeBrush(const Brush& brush) override;
        [[nodiscard]] TextAntialiasToken beginTextRaster(const TextRasterizationParams&) override;
        void endTextRaster(TextAntialiasToken) override;
        [[nodiscard]] bool snapTextOrigins() const override { return m_snapTextOrigins; }

        // Transforms
        void setTransform(const Matrix3x2& matrix) override;
        void resetTransform() override;

        // Decoupled primitives
        PixelView stagingView(const FloatRect&) override { return { {}, {}, {} }; }

        // Staged content is rasterized by the CPU painters, which are handed the canvas transform
        // as a placement matrix and bake it into their geometry. Uploading that through a
        // transformed render target would scale it a second time, so the blit goes down
        // untransformed. This is also why it stays sharp: the paths are rasterized at the animated
        // size rather than resampled from a bitmap drawn at the resting one.
        //
        // drawPixelView keeps following the transform - a bitmap handed to the canvas is content,
        // not pre-rendered output, and should animate with its control.
        void commitStagingView(const PixelView& view) override;

        void drawPixelView(const PixelView& view) override;
        void drawLine(FloatPoint pt1, FloatPoint pt2, const Brush& brush, float strokeWidth) override;
        void drawRectangle(const FloatRect& rect, const Brush& brush, float strokeWidth) override;
        void fillRectangle(const FloatRect& rect, const Brush& brush) override;
        void drawRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush, float strokeWidth) override;
        void fillRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush) override;
        bool fillPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush) override;
        bool drawPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush, float strokeWidth) override;
        void drawEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush, float strokeWidth) override;
        void fillEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush) override;

        // A SHADOW IS THE DISTANCE FIELD OF ITS SILHOUETTE, and a rounded rectangle's is made of
        // pieces a gradient states exactly - see the definition. Rectangles and brushes, no
        // geometry, no layer, no ring; what it costs does not grow with the silhouette.
        bool castShadow(const RoundedRectangleParts&, const ShadowParams&) override;

        void fillPath(const PixelPath& path, const Brush& brush, const Matrix3x2* transform = nullptr) override;
        void drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform = nullptr) override;

    private:
        struct CachedRenderingParams
        {
            TextRasterizationParams key;
            ComPtr<IDWriteRenderingParams> value;
        };
        struct ImagePush
        {
            ComPtr<ID2D1RenderTarget> previousTarget;
            // What the target being left was carrying. Text raster state belongs to the target, so
            // what m_installedRaster names while an image is open is the image's own.
            TextRasterizationParams installedRaster{};
            ComPtr<ID2D1BitmapRenderTarget> begun;
        };

        enum class ClipType { AxisAligned, Layer };

        struct ClipState
        {
            ClipType type;
            ComPtr<ID2D1PathGeometry> maskGeometry;
        };

    private:
        void drawPathWithBrush(const PixelPath& path, const Brush& brush, float strokeWidth, const Matrix3x2* transform = nullptr);

        // Lays a glow down as strokes of the same geometry, widest first, each one falling inside
        // the last. Direct2D offers no distance field, so the ramp is built out of the widths it
        // does offer.
        void strokeGlow(ID2D1Geometry& geometry, const PathDrawLayer& layer, const Brush& brush);
        // The Shadow layer: the glow kept outside the silhouette, and the silhouette filled solid.
        void fillShadow(ID2D1Geometry& silhouette, const PathDrawLayer& layer);
        // The ramp of a shadow from a distance `from` out to its reach, as the stops of a gradient
        // whose whole length is `extent`: full up to `from`, the falloff from there to the reach.
        [[nodiscard]] ComPtr<ID2D1GradientStopCollection> shadowRamp(const ShadowParams&, float from, float extent);

        // Builds what the presenter's device does not have yet - the Direct2D device and context
        // on a new device, the target bitmap over a frame texture that moved or grew. Answers
        // whether there is a target to draw into.
        bool ensureTarget(CompositionPresenter&);
        // Releases everything the device owns and asks for another frame, which is where the
        // rebuild is attempted. What the device does not own stays: the factory, and the
        // DirectWrite rendering params.
        void discardTarget();
        // The shared upload surface the CPU painters stage into. Belongs to the device and to the
        // current size, so it is made again whenever either of those changes, and kept when
        // neither has.
        void createStagingBitmap();
        // The kept image at this index, built at the surface's size if it is not there at that
        // size already. Null when the device would not build one, which a caller answers by
        // drawing to the surface instead.
        [[nodiscard]] ID2D1BitmapRenderTarget* image(int index);

        ID2D1Brush* getNativeBrushInternal(const Brush& brush);
        ComPtr<ID2D1GradientStopCollection> createStopCollection(const std::vector<GradientStop>& stops);
        ComPtr<ID2D1PathGeometry> buildPathGeometry(const PixelPath& path, bool isFilled);
        ComPtr<IDWriteRenderingParams> renderingParams(const TextRasterizationParams&);
        static ComPtr<IDWriteRenderingParams> makeRenderingParams(DWRITE_RENDERING_MODE1, DWRITE_GRID_FIT_MODE, bool zeroContrast);

    private:
        // The window this backend draws for, asked for a whole repaint whenever the target is
        // new. Non-owning, and outlives this backend - a form declares its window before its
        // canvas, so the canvas is destroyed first.
        IPlatformWindow* m_window;
        IntSize m_size;
        bool m_isPainting{ false };
        bool m_snapTextOrigins{ true };
        // A frame that carries alpha starts from nothing - see IBackend::setTransparentBase - and
        // whether the frame's dirty rect is standing as a clip on the context.
        bool m_transparentBase{ false };
        bool m_baseClip{ false };
        // Which of the presenter's devices the Direct2D device below was built on, and which
        // frame surface the target bitmap wraps - both non-owning identities, compared to what
        // the presenter answers at each paint.
        unsigned m_generation{ 0 };
        IDXGISurface* m_targetSurface{ nullptr };
        // Built on demand and kept, because the parameters describe how to rasterize rather than
        // what, so one object serves every run that asks for the same thing. Bounded by the number
        // of distinct params any caller asks for, which is a handful, so it needs no eviction - a
        // linear scan over that is cheaper than hashing.
        std::vector<CachedRenderingParams> m_renderingParamsCache;
        // What the target carries now, and what it carried when it was made. A run asking for what
        // is already installed reaches the target with nothing, so a list of movable items pays for
        // one change and a page of static text pays for none.
        //
        // The created state is recorded rather than inferred, because returning to it means handing
        // DirectWrite's own params back. A custom object built to say the same thing is not the
        // same object, and would leave the target off the settings it starts on.
        TextRasterizationParams m_createdRaster{};
        TextRasterizationParams m_installedRaster{};
        // The window's own target - the device context, drawing into the target bitmap over the
        // presenter's frame texture - and the one being drawn into. They are the same object
        // except while a kept image is open, when the second is that image's target. Every draw
        // goes through m_renderTarget so that pushImage redirects all of them at once, the glyph
        // renderer included - it reaches the target through getNativeRenderTarget. Only what a
        // window target alone can answer for - being built, being the surface an image is
        // measured against - names m_deviceContext.
        ComPtr<ID2D1Device> m_d2dDevice;
        ComPtr<ID2D1DeviceContext> m_deviceContext;
        ComPtr<ID2D1Bitmap1> m_targetBitmap;
        ComPtr<ID2D1RenderTarget> m_renderTarget;
        ComPtr<ID2D1Bitmap> m_d2dBitmap;

        // A kept image is a bitmap render target built from the window's, so it shares the device
        // and a brush or a geometry made for either is good on both.
        std::vector<ComPtr<ID2D1BitmapRenderTarget>> m_images;

        // One entry per pushImage that has not been popped. The target to go back to is recorded
        // rather than derived, and the image is recorded only when its own BeginDraw was actually
        // made - a push that could not build its image still balances its pop, and draws to the
        // surface meanwhile rather than losing what was drawn.
        std::vector<ImagePush> m_imageStack;

        // Maintain active geometry pointers so D2D layer masks don't access freed memory
        std::vector<ClipState> m_clipTypeStack;

        // Opacity layers standing open. Counted rather than stacked because a layer carries
        // nothing that has to be kept alive to be popped, and kept apart from m_clipTypeStack so
        // that a clip popped between a push and its pop cannot take the layer's entry with it.
        // A push that reached no render target counts as nothing, which is what the pop reads.
        int m_opacityLayers{ 0 };

        // --- SINGLE UNIFIED CACHE FOR ALL BRUSHES ---
        std::unordered_map<Brush, ComPtr<ID2D1Brush>, BrushHash> m_brushCache;
    };
}
