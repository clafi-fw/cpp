export module ClaFi.Core.Graphics.Canvas;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{

    export class Canvas
    {
    public:
        using StagingDrawFunc = std::function<void(PixelView, const Matrix3x2& canvasTransform)>;
    public:
        explicit Canvas(std::unique_ptr<IBackend>);
        ~Canvas() = default;
    public:
        // THE BACKEND THIS CANVAS DRAWS THROUGH, PUT IN AFTER IT WAS BUILT. Everything the old one
        // held goes with it - its brushes, its geometries, its kept images - so whoever calls this
        // paints whole afterwards, and calls it where no paint is open. See FormBase::stateBackend
        void setBackend(std::unique_ptr<IBackend>);
        void resize(IntSize);
        void pushClip(const FloatRect& exactClipRect);
        void pushClip(const PixelPath& path, const Matrix3x2* transform = nullptr);
        void popClip();
        // Everything drawn until the matching pop reaches the surface as one image at this
        // opacity. See IBackend::pushOpacity for what that buys over an opacity applied to each
        // brush, and prefer ScopedCanvasOpacity to pairing these by hand.
        void pushOpacity(float opacity);
        void popOpacity();
        // A kept image the size of the surface. See IBackend::pushImage; prefer ScopedCanvasImage
        // to pairing these by hand.
        void pushImage(int index);
        void popImage();
        void drawImage(int index, float opacity);
        [[nodiscard]] bool hasImage(int index) const;
        FloatRect clipBox() const;

        int width() const;
        int height() const;

        void beginPaint(void* nativeContext, const IntRect& dirtyRect);
        void endPaint(void* nativeContext, const IntRect& dirtyRect, Bitmap*& outData);

        // Transform States
        [[nodiscard]] const Matrix3x2& transform() const { return m_transform; }
        void setTransform(const Matrix3x2& matrix);
        void resetTransform();

        // --- Decoupled Shapes (Pairs ANY shape with ANY brush!) ---
        void drawLine(FloatPoint pt1, FloatPoint pt2, const Brush& brush, float strokeWidth = 1.0f);
        void drawLine(FloatPoint pt1, FloatPoint pt2, Color color, float strokeWidth = 1.0f);
        void drawRectangle(const FloatRect& rect, const Brush& brush, float strokeWidth = 1.0f);
        void drawRectangle(const FloatRect& rect, Color color, float strokeWidth = 1.0f);
        void fillRectangle(const FloatRect& rect, const Brush& brush);
        void fillRectangle(const FloatRect& rect, Color color);
        void drawRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush, float strokeWidth = 1.0f);
        void drawRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, Color color, float strokeWidth = 1.0f);
        void fillRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush);
        void fillRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, Color color);
        void drawEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush, float strokeWidth = 1.0f);
        void drawEllipse(FloatPoint center, float radiusX, float radiusY, Color color, float strokeWidth = 1.0f);
        void fillEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush);
        void fillEllipse(FloatPoint center, float radiusX, float radiusY, Color color);
        void drawCircle(FloatPoint center, float radius, const Brush& brush, float strokeWidth = 1.0f);
        void drawCircle(FloatPoint center, float radius, Color color, float strokeWidth = 1.0f);
        void fillCircle(FloatPoint center, float radius, const Brush& brush);
        void fillCircle(FloatPoint center, float radius, Color color);
        void drawCorner(const CornerNook& nook, const Brush& brush, float strokeWidth = 1.0f);
        void drawCorner(const CornerNook& nook, Color color, float strokeWidth = 1.0f);
        void fillCorner(const CornerNook& nook, const Brush& brush);
        void fillCorner(const CornerNook& nook, Color color);
        void fillDustyCorner(const CornerNook& nook, const Brush& brush);
        void fillDustyCorner(const CornerNook& nook, Color color);
        void drawPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush, float strokeWidth = 1.0f);
        void drawPartialRoundedRectangle(const RoundedRectangleParts&, Color, float strokeWidth);
        void fillPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush);
        void fillPartialRoundedRectangle(const RoundedRectangleParts&, Color);
        void drawPartialRoundedRectangleDef(const RoundedRectangleParts& parts, const Brush& brush, float strokeWidth = 1.0f);
        void fillPartialRoundedRectangleDef(const RoundedRectangleParts& parts, const Brush& brush);

        // The shadow a shape casts on what is already drawn, and not the shape itself: whatever
        // casts a shadow covers it, so drawing the shape here would be work nobody sees. A caller
        // that wants the soft ring without anything standing in it is asking for a glow, and
        // should say so through a path layer.
        //
        // Call this BEFORE drawing the shape. The ramp is measured from the silhouette's edge and
        // reaches inward as far as it reaches out, and what covers the inward half is the shape.
        // A shape that does not cover its own silhouette - one drawn with a translucent fill -
        // will show that half through, and wants the shadow clipped to the outside instead.
        void castShadow(const RoundedRectangleParts& parts, const ShadowParams&);
        void castShadowDef(const RoundedRectangleParts& parts, const ShadowParams&);

        void drawPath(const PixelPath& path, const Brush& brush, float strokeWidth = 1.0f, const Matrix3x2* transform = nullptr) const;
        void drawPath(const PixelPath& path, Color color, float strokeWidth = 1.0f, const Matrix3x2* transform = nullptr);
        void fillPath(const PixelPath& path, const Brush& brush, const Matrix3x2* transform = nullptr) const;
        void fillPath(const PixelPath& path, Color color, const Matrix3x2* transform = nullptr) const;

        void drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform = nullptr) const;
        void drawPath(const PixelPath& path, std::initializer_list<PathDrawLayer> layers, const Matrix3x2* transform = nullptr) const;

        void fillLinearGradientRectangle(const FloatRect& rect, FloatPoint startPoint, FloatPoint endPoint, const std::vector<GradientStop>& stops);
        void fillRadialGradientRectangle(const FloatRect& rect, FloatPoint center, FloatPoint offset, float radiusX, float radiusY, const std::vector<GradientStop>& stops);
        void fillTopBottomGradientRectangle(const FloatRect& rect, Color topColor, Color bottomColor);
        void fillLeftRightGradientRectangle(const FloatRect& rect, Color leftColor, Color rightColor);
        // A band along one side of the rect, opaque at the edge and clear fadeSize in. The band's
        // two corners on that edge take the rect's radii there, so a fade over a rounded view
        // ends on the view's arc.
        void fadeEdge(const FloatRect& rect, const CornerRadii& radii, RectSide side, float fadeSize, Color opaqueColor);

        // --- Software Rendering & Bitmap Drawing ---
        // Handing out a raw pixel buffer is an escape hatch from the canvas, so whatever the caller
        // draws into it is outside the canvas transform by construction - it will not animate with
        // its control unless it asks to. The matrix passed alongside is that transform, in the same
        // coordinates the caller already works in.
        //
        // A caller that builds geometry in its own local space composes onto it rather than
        // replacing it, which is a one token change to the matrix it already builds:
        //     Matrix3x2 transform = canvasTransform * Matrix3x2::translation(iconRect.topLeft());
        // A caller that works in canvas coordinates maps its points through it directly.
        void stagingDraw(const FloatRect& requestedRect, const StagingDrawFunc&);
        void drawPixelView(const PixelView& view) const;
        void drawBitmap(const Bitmap& bitmap, const FloatPoint& position) const;

        IBackend* backend() const;
    private:
        PixelView stagingView(const FloatRect& requestedRect);
    private:
        int m_width{ 0 };
        int m_height{ 0 };
        BitmapData m_stagingBuffer;
        std::vector<FloatRect> m_clipStack;
        // Mirrors what the backend was last told, so a transform can be composed onto it and put
        // back. The backend interface only sets, it does not report.
        Matrix3x2 m_transform{ Matrix3x2::identity() };
        std::unique_ptr<IBackend> m_backend;
    };

    // Composes a transform onto the canvas for as long as it is alive. See Graphics-Types
    export class ScopedCanvasTransform
    {
    public:
        ScopedCanvasTransform(Canvas&, const Matrix3x2& local);
        ~ScopedCanvasTransform();
        ScopedCanvasTransform(const ScopedCanvasTransform&) = delete;
        ScopedCanvasTransform& operator=(const ScopedCanvasTransform&) = delete;
    private:
        // Null when the matrix was identity and nothing was changed.
        Canvas* m_canvas{ nullptr };
        Matrix3x2 m_restoreTo{ Matrix3x2::identity() };
    };

    // Holds an opacity layer open for as long as it is alive. See Graphics-Types
    export class ScopedCanvasOpacity
    {
    public:
        ScopedCanvasOpacity(Canvas&, float opacity);
        ~ScopedCanvasOpacity();
        ScopedCanvasOpacity(const ScopedCanvasOpacity&) = delete;
        ScopedCanvasOpacity& operator=(const ScopedCanvasOpacity&) = delete;
    private:
        // Null when the opacity was full and no layer was opened.
        Canvas* m_canvas{ nullptr };
    };

    // Draws into the kept image at this index for as long as it is alive. See Graphics-Types
    export class ScopedCanvasImage
    {
    public:
        ScopedCanvasImage(Canvas&, int index);
        ~ScopedCanvasImage();
        ScopedCanvasImage(const ScopedCanvasImage&) = delete;
        ScopedCanvasImage& operator=(const ScopedCanvasImage&) = delete;
    private:
        Canvas& m_canvas;
    };

}
