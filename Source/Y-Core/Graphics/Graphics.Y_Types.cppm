export module ClaFi.Core.Graphics.Types;

export import :PixelView;
export import :Bitmap;

import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    // A stretch along a line that ink covers, left edge first.
    export struct InkExtent
    {
        float left{ 0.0f };
        float right{ 0.0f };
    };

    // Which backend draws.
    export enum class BackendType
    {
        Cpu,
        Direct2D
    };

    // How a glyph's coverage is computed. See Graphics-Types
    export enum class TextRasterizationMode
    {
        Cached,
        Outline
    };

    // Whether coverage is computed per colour channel or once for the pixel.
    export enum class TextAntialiasingMode
    {
        Subpixel,
        Grayscale
    };

    // Everything about rasterizing a run of text that a backend has to be told. See Graphics-Types
    export struct TextRasterizationParams
    {
        TextAntialiasingMode antialias{ TextAntialiasingMode::Subpixel };
        // Fit the glyph outlines to the pixel grid. Crisper, at the cost of the shape depending on
        // sub pixel position.
        bool gridFit{ true };
        // Put each run's baseline origin on a whole device pixel.
        bool snapOrigins{ true };
        // The platform's contrast boost, which thickens light glyphs on a dark ground. Off makes a
        // cached run weigh the same as an outlined one.
        bool enhancedContrast{ true };
        TextRasterizationMode rasterizationMode{ TextRasterizationMode::Cached };

        bool operator==(const TextRasterizationParams&) const = default;
    };

    // Opaque stand-in for a backend's own text antialiasing mode. Only the backend that produced
    // one may interpret it.
    export using TextAntialiasToken = std::uint32_t;

    // How a stroke ends.
    export enum class StrokeCap {
        Round,
        Butt,
        Square
    };

    export class PainterBase
    {
    public:
        // Stays in the interface deliberately: a painter is constructed once per primitive, so
        // this runs thousands of times a frame. A body in the BMI can be inlined by importers;
        // a body in the implementation unit cannot, and there is no whole-program optimisation
        // to recover it.
        PainterBase(const PixelView& pixelView)
            :
            m_pixelView{ pixelView }
        {
        }
        const PixelView& pixelView() const { return m_pixelView; }
    protected:
        const PixelView m_pixelView;
    };

    export struct GradientStop
    {
        float position;
        Color color;
        bool operator==(const GradientStop&) const = default;
    };

    export struct LinearGradient
    {
        FloatPoint startPoint;
        FloatPoint endPoint;
        std::vector<GradientStop> stops;
        static LinearGradient simple(FloatPoint startPt, FloatPoint endPt, Color startColor, Color endColor)
        {
            return {
                startPt,
                endPt,
                { { 0.0f, startColor }, { 1.0f, endColor } }
            };
        }
        bool operator==(const LinearGradient&) const = default;
    };

    export struct RadialGradient
    {
        FloatPoint center;
        FloatPoint offset;
        float radiusX;
        float radiusY;
        std::vector<GradientStop> stops;
        bool operator==(const RadialGradient&) const = default;
    };

    // The shape a glow is cast from.
    export enum class GlowShape {
        Circle,         // Euclidean distance
        SoftRectangle,  // L4 Norm
        SharpRectangle  // Chebyshev distance (max)
    };

    export struct PointGlow
    {
        FloatPoint lightPos;
        Color lightColor;
        float lightSpread;
        float xRatio{ 1.0f };
        GlowShape shape{ GlowShape::Circle };
        float opacity{ 1.0f };
        bool operator==(const PointGlow&) const = default;
    };

    export struct SolidColor
    {
        Color color;
        bool operator==(const SolidColor&) const = default;
    };

    export using Brush = std::variant<SolidColor, LinearGradient, RadialGradient, PointGlow>;

    // What a path is drawn as.
    export enum class PathRenderMode
    {
        Fill,
        Stroke,
        OuterGlow, // the ramp both ways from the outline, as far in as out. See Graphics-Types
        Shadow // the shape's own coverage, solid, and the ramp outward from it. See Graphics-Types
    };

    // How a glow thins out with distance from the shape casting it.
    export enum class GlowFalloff
    {
        Linear, // straight to nothing. See Graphics-Types
        Smooth // steep at the shape and level by the time it ends. See Graphics-Types
    };

    // How much of a glow stands at `t` of the way out, 1 against the shape and 0 at the reach.
    // The one statement of each curve: the mask that rasterizes a glow computes this eight
    // pixels at a time and cannot call it, so anything that changes here changes there too.
    export constexpr float falloffAt(GlowFalloff falloff, float t)
    {
        const float remaining = 1.0f - std::clamp(t, 0.0f, 1.0f);
        return falloff == GlowFalloff::Smooth ? remaining * remaining : remaining;
    }

    // What one ring of a stacked glow has to be given to leave `target` standing, when `laid` is
    // already down under it and the brush covers `alpha` of a pixel at full opacity. Rings are
    // laid outside in and each composites over the last, so a ring handed its target directly
    // would darken what is there rather than reach it - and a translucent brush handed an opaque
    // one's share compounds past the target the same way, since what is under it is alpha * laid.
    export constexpr float ringOpacity(float target, float laid, float alpha = 1.0f)
    {
        const float under = alpha * laid;
        if (under >= 1.0f)
            return 0.0f;
        return std::max(0.0f, (target - laid) / (1.0f - under));
    }

    export struct PathGeometryOptions {
        PathRenderMode mode = PathRenderMode::Fill;
        float strokeWidth = 1.0f;
        StrokeCap strokeCap = StrokeCap::Round;
        // Read for OuterGlow and Shadow and ignored by the other modes.
        GlowFalloff falloff = GlowFalloff::Linear;
    };

    // A shadow cast by a shape onto whatever is already drawn behind it.
    export struct ShadowParams
    {
        // The shadow where it is darkest, against the shape's own edge. Everything further out
        // is a share of this, so its opacity is the whole of how heavy the shadow reads.
        Color color{};
        // How far the shadow reaches out past the shape.
        float blur{ 0.0f };
        // Which way the light falls, as the distance the shadow is carried. Zero is a light
        // straight in front of the shape, which casts an even ring around it.
        FloatPoint offset{};
        // How much the shape is grown before its shadow is taken. It moves the whole ramp out
        // rather than stretching it, which is what lets a shadow sit wider than what casts it
        // without also softening.
        float spread{ 0.0f };
        GlowFalloff falloff{ GlowFalloff::Smooth };
        bool operator==(const ShadowParams&) const = default;
    };

    export struct PathDrawLayer {
        PathGeometryOptions geometry;
        Brush brush;
        static PathDrawLayer fill(Color color) { return { .geometry{}, .brush{ SolidColor{color} } }; }
        static PathDrawLayer stroke(Color color, float strokeWidth)
        {
            return {
                .geometry{
                    .mode = PathRenderMode::Stroke,
                    .strokeWidth = strokeWidth
                },
                .brush{
                    SolidColor{color}
                }
            };
        }
        static PathDrawLayer linearGradient(FloatPoint startPt, FloatPoint endPt, Color startColor, Color endColor)
        {
            return { .geometry{}, .brush{ LinearGradient::simple(startPt, endPt, startColor, endColor) } };
        }
        static PathDrawLayer pointGlow(const PointGlow& pointGlow) { return { .geometry{}, .brush{ pointGlow } }; }

        static PathDrawLayer outerGlow(Color c, float radius, StrokeCap cap = StrokeCap::Round,
            GlowFalloff falloff = GlowFalloff::Linear)
        {
            return {
                .geometry{ .mode = PathRenderMode::OuterGlow, .strokeWidth = radius, .strokeCap = cap, .falloff = falloff },
                .brush{ SolidColor{ c } }
            };
        }

        static PathDrawLayer shadow(Color c, float reach, GlowFalloff falloff = GlowFalloff::Smooth)
        {
            return {
                .geometry{ .mode = PathRenderMode::Shadow, .strokeWidth = reach, .falloff = falloff },
                .brush{ SolidColor{ c } }
            };
        }
    };

    export struct Matrix3x2
    {
        // dont change anything here - it's binary compatible with D2D1_MATRIX_3X2_F

        float a{ 1.0f }, b{ 0.0f }; // m11, m12
        float c{ 0.0f }, d{ 1.0f }; // m21, m22
        float e{ 0.0f }, f{ 0.0f }; // dx,  dy

        inline constexpr FloatPoint transform(FloatPoint p) const {
            return { p.x * a + p.y * c + e, p.x * b + p.y * d + f };
        }

        inline constexpr FloatPoint transformVector(FloatPoint v) const {
            return { v.x * a + v.y * c, v.x * b + v.y * d };
        }

        float getScaleFactor() const {
            float sx = std::sqrt(a * a + b * b);
            float sy = std::sqrt(c * c + d * d);
            return std::max(sx, sy);
        }

        static constexpr Matrix3x2 identity() { return {}; }
        static constexpr Matrix3x2 translation(float dx, float dy) { return { 1.0f, 0.0f, 0.0f, 1.0f, dx, dy }; }
        static constexpr Matrix3x2 translation(FloatPoint p) { return { 1.0f, 0.0f, 0.0f, 1.0f, p.x, p.y }; }
        static constexpr Matrix3x2 scale(float s) { return { s, 0.0f, 0.0f, s, 0.0f, 0.0f }; }
        static constexpr Matrix3x2 scale(float sx, float sy) { return { sx, 0.0f, 0.0f, sy, 0.0f, 0.0f }; }
        static Matrix3x2 rotation(float rad) {
            float s = std::sin(rad), c = std::cos(rad);
            return { c, s, -s, c, 0.0f, 0.0f };
        }
        static Matrix3x2 rotationDeg(float deg) { return rotation(deg * 0.0174533f); }

        constexpr Matrix3x2 operator*(const Matrix3x2& o) const {
            return {
                a * o.a + c * o.b,
                b * o.a + d * o.b,
                a * o.c + c * o.d,
                b * o.c + d * o.d,
                a * o.e + c * o.f + e,
                b * o.e + d * o.f + f
            };
        }
        // Maps a rect by its two opposite corners. That is exact while the matrix has no rotation
        // and no shear - true of a scale about an origin, which is all the press animation produces.
        // Under a rotation this yields the bounding box rather than the rotated shape, so anything
        // that has to survive a rotation belongs in a path, not a rect.
        [[nodiscard]] FloatRect mapRect(const FloatRect& rect) const
        {
            FloatPoint topLeft = transform({ rect.left, rect.top });
            FloatPoint bottomRight = transform({ rect.right, rect.bottom });
            return {
                (std::min)(topLeft.x, bottomRight.x),
                (std::min)(topLeft.y, bottomRight.y),
                (std::max)(topLeft.x, bottomRight.x),
                (std::max)(topLeft.y, bottomRight.y)
            };
        }

        bool operator==(const Matrix3x2& o) const = default;
    };

    // One step of a path, absolute or relative.
    export enum class PathCommandType : std::uint8_t {
        MoveTo,      // M
        MoveBy,      // m
        LineTo,      // L
        LineBy,      // l
        HLineTo,     // H
        HLineBy,     // h
        VLineTo,     // V
        VLineBy,     // v
        QuadTo,      // Q
        QuadBy,      // q
        CubicTo,     // C
        CubicBy,     // c
        Close        // Z/z
    };

    export struct PathCommand {
        PathCommandType type;
        FloatPoint p1{ 0.0f, 0.0f };
        FloatPoint p2{ 0.0f, 0.0f };
        FloatPoint p3{ 0.0f, 0.0f };
    };

    export class PixelPath {
    public:
        void moveTo(FloatPoint point) { m_commands.push_back({ PathCommandType::MoveTo, point }); m_currentPoint = point; }
        void moveTo(float x, float y) { moveTo({ x, y }); }
        void moveBy(FloatPoint delta) { m_commands.push_back({ PathCommandType::MoveBy, delta }); m_currentPoint = { m_currentPoint.x + delta.x, m_currentPoint.y + delta.y }; }
        void moveBy(float dx, float dy) { moveBy({ dx, dy }); }

        void lineTo(FloatPoint p) { m_commands.push_back({ PathCommandType::LineTo, p }); m_currentPoint = p; }
        void lineTo(float x, float y) { lineTo({ x, y }); }
        void lineBy(FloatPoint delta) { m_commands.push_back({ PathCommandType::LineBy, delta }); m_currentPoint = { m_currentPoint.x + delta.x, m_currentPoint.y + delta.y }; }
        void lineBy(float dx, float dy) { lineBy({ dx, dy }); }

        void hLineTo(float x) { m_commands.push_back({ PathCommandType::HLineTo, { x, 0.0f } }); m_currentPoint.x = x; }
        void vLineTo(float y) { m_commands.push_back({ PathCommandType::VLineTo, { 0.0f, y } }); m_currentPoint.y = y; }
        void hLineBy(float dx) { m_commands.push_back({ PathCommandType::HLineBy, { dx, 0.0f } }); m_currentPoint.x += dx; }
        void vLineBy(float dy) { m_commands.push_back({ PathCommandType::VLineBy, { 0.0f, dy } }); m_currentPoint.y += dy; }

        void quadTo(FloatPoint ctrl, FloatPoint end) { m_commands.push_back({ PathCommandType::QuadTo, ctrl, end }); m_currentPoint = end; }
        void bezierBy(FloatPoint dCtrl, FloatPoint dEnd) { m_commands.push_back({ PathCommandType::QuadBy, dCtrl, dEnd }); m_currentPoint = { m_currentPoint.x + dEnd.x, m_currentPoint.y + dEnd.y }; }

        void cubicTo(FloatPoint ctrl1, FloatPoint ctrl2, FloatPoint end) { m_commands.push_back({ PathCommandType::CubicTo, ctrl1, ctrl2, end }); m_currentPoint = end; }
        void cubicBy(FloatPoint dCtrl1, FloatPoint dCtrl2, FloatPoint dEnd) { m_commands.push_back({ PathCommandType::CubicBy, dCtrl1, dCtrl2, dEnd }); m_currentPoint = { m_currentPoint.x + dEnd.x, m_currentPoint.y + dEnd.y }; }

        void close() { m_commands.push_back({ PathCommandType::Close }); }
        void clear() { m_commands.clear(); m_currentPoint = { 0.0f, 0.0f }; }

        // Appends a closed polygon with its corners rounded: each corner is cut back along both
        // of its edges and the two cuts joined by a quadratic through the corner. How far back a
        // corner is cut is the radius, or half of its shorter edge where that is less, so the two
        // corners sharing an edge cannot cross and a corner's round stays symmetric about it.
        //
        // Adds rather than draws: the draw... calls below start a fresh path, which is what stops
        // one of them being combined with any other geometry. Fewer than three points is not a
        // polygon and appends nothing.
        void addRoundedPolygon(std::span<const FloatPoint> points, float radius);

        void drawRoundedRect(float w, float h, float r);
        void drawCircle(FloatPoint center, float radius);

        void transform(const Matrix3x2& matrix);

        bool isEmpty() const { return m_commands.empty(); }
        const std::vector<PathCommand>& commands() const { return m_commands; }
        FloatPoint currentPoint() const { return m_currentPoint; }

    private:
        std::vector<PathCommand> m_commands;
        FloatPoint m_currentPoint{ 0.0f, 0.0f };
    };

    // What a graphics backend provides, every stroke laid inside its bounds. See Graphics-Types
    export class IBackend
    {
    public:
        virtual ~IBackend() = default;
        virtual BackendType type() const = 0;
        virtual void resize(IntPoint) = 0;
        virtual IntSize size() const = 0;
        virtual void pushClip(const FloatRect& exactClipRect) = 0;
        virtual void pushClip(const PixelPath& path, const Matrix3x2* transform = nullptr) = 0;
        virtual void popClip() = 0;
        // Everything drawn until the matching pop reaches the surface as one image at this
        // opacity, rather than primitive by primitive. Two shapes that overlap inside a layer do
        // not show each other through where they cross, and a shape drawn over another inside one
        // covers it as completely as it would at full strength.
        //
        // That is the whole reason a layer exists here rather than an opacity multiplied into
        // every brush: a control tree painted at half strength through its brushes would show its
        // own surface through its own text. Pair these through ScopedCanvasOpacity rather than
        // by hand.
        virtual void pushOpacity(float opacity) = 0;
        virtual void popOpacity() = 0;

        // A KEPT IMAGE the size of the surface, named by index. Everything drawn between pushImage
        // and the matching pop goes into it instead of the surface, and drawImage puts it back at
        // an opacity. Unlike an opacity layer it outlives the frame that drew it, which is what
        // makes it worth having: a crossfade builds its next image out of its last one, so a
        // caller holds two and trades them over.
        //
        // An image spans the surface and is drawn into under the surface's own transform, so a
        // pixel keeps the address it would have had without one. A newly built image is
        // transparent; what a caller does not cover, it does not get back.
        //
        // The store belongs to the surface, so a resize and anything that discards the surface
        // empty it. hasImage is how a caller finds out that what it drew is gone and has to be
        // drawn again.
        virtual void pushImage(int index) = 0;
        virtual void popImage() = 0;
        virtual void drawImage(int index, float opacity) = 0;
        [[nodiscard]] virtual bool hasImage(int index) const = 0;

        // WHETHER A FRAME STARTS FROM NOTHING. Off by default: a frame otherwise keeps whatever
        // the last one left, which costs nothing and is invisible for as long as the children
        // cover every pixel. A window whose alpha is read by whoever shows it cannot rely on that
        // - the pixels outside a rounded corner are exactly the ones nothing covers.
        virtual void setTransparentBase(bool) {}
        virtual void beginPaint(void* nativeContext, const IntRect* dirtyRect) = 0;
        virtual void endPaint(void* nativeContext, const IntRect* dirtyRect, Bitmap*& outData) = 0;
        virtual void* getNativeRenderTarget() const = 0;
        virtual void* getNativeBrush(const Brush& brush) = 0;

        // Rasterize text this way until the scope is closed. Returns what the antialiasing mode was
        // so the caller can hand it straight back. The token is the backend's own value rather than
        // anything this header names, so restoring cannot flatten a mode the shared code has no
        // word for. Use ScopedTextRaster rather than pairing these by hand.
        [[nodiscard]] virtual TextAntialiasToken beginTextRaster(const TextRasterizationParams&) = 0;
        virtual void endTextRaster(TextAntialiasToken) = 0;
        // The snapOrigins of the params currently in force. Read per glyph run, deep inside the
        // platform's own draw callback, which is too far from the caller to be passed a flag.
        [[nodiscard]] virtual bool snapTextOrigins() const = 0;

        // Transforms
        virtual void setTransform(const Matrix3x2& matrix) = 0;
        virtual void resetTransform() = 0;

        // Thin Abstracted Drawing Primitives
        virtual PixelView stagingView(const FloatRect& requestedRect) = 0;
        virtual void commitStagingView(const PixelView& view) = 0;
        virtual void drawPixelView(const PixelView& view) = 0;
        virtual void drawLine(FloatPoint pt1, FloatPoint pt2, const Brush& brush, float strokeWidth) = 0;

        virtual void drawRectangle(const FloatRect& rect, const Brush& brush, float strokeWidth) = 0;
        virtual void fillRectangle(const FloatRect& rect, const Brush& brush) = 0;

        virtual void drawRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush, float strokeWidth) = 0;
        virtual void fillRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush) = 0;

        virtual void drawEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush, float strokeWidth) = 0;
        virtual void fillEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush) = 0;

        virtual bool fillPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush) = 0;
        virtual bool drawPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush, float strokeWidth) = 0;

        // The shadow of `parts`, and not the shape itself. Answers false to be given the shared
        // construction instead - Canvas::castShadowDef, which asks for a Shadow layer on the
        // shape's silhouette and so arrives back at this backend's own glow.
        //
        // A backend claims it for a shadow it can answer in one piece, where handing the work
        // over beats laying the ramp down through the glow. Direct2D does: a rounded rectangle's
        // distance field is rectangles filled with gradients, stated exactly.
        virtual bool castShadow(const RoundedRectangleParts& parts, const ShadowParams&) = 0;

        virtual void fillPath(const PixelPath& path, const Brush& brush, const Matrix3x2* transform = nullptr) = 0;
        virtual void drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform = nullptr) = 0;
    };

    // Names the raster params one text run wants. See Graphics-Types
    export class ScopedTextRaster
    {
    public:
        // Both bodies stay in the interface: one guard is built per text run, so with a screen
        // full of labels this runs hundreds of times a frame.
        ScopedTextRaster(IBackend& backend, const TextRasterizationParams& params)
            : m_backend{ &backend }
            , m_restoreTo{ backend.beginTextRaster(params) }
        {
        }
        ~ScopedTextRaster()
        {
            m_backend->endTextRaster(m_restoreTo);
        }
        ScopedTextRaster(const ScopedTextRaster&) = delete;
        ScopedTextRaster& operator=(const ScopedTextRaster&) = delete;
    private:
        IBackend* m_backend;
        TextAntialiasToken m_restoreTo;
    };

}
