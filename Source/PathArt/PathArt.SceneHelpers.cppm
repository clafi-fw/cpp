export module ClaFi.PathArt.SceneHelpers;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // Where a scene draws, in the scene's own coordinates: 0, 0 at the top left. See PathArt
    export struct SceneFrame
    {
        Canvas& canvas;
        float width;
        float height;
        float unit; // pixels per scene unit - 600 units span the shorter side
        Matrix3x2 toCanvas; // from the scene's coordinates onto the canvas

        [[nodiscard]] static SceneFrame FromBounds(Canvas&, const FloatRect& bounds);
        // A frame whose shorter side spans `units`, not 600 - an icon framed to its figure.
        [[nodiscard]] static SceneFrame Spanning(Canvas&, const FloatRect& bounds, float units);
        // The same frame with `local` applied ahead of its own move onto the canvas.
        [[nodiscard]] SceneFrame within(const Matrix3x2& local) const;
    };

    // Where points lie on a sphere's map, and the ground each stands for. See PathArt#mapquery
    export struct MapQuery
    {
        std::span<const float> longitudes; // in radians, any number of turns either way
        std::span<const float> latitudes; // in radians
        std::span<const float> longitudeSpans; // the longitudes a point's ground spans, in radians
        std::span<const float> latitudeSpans; // the latitudes it spans, in radians
    };

    // Which way a two-part limb bends when it can bend either way.
    export enum class BendToward
    {
        Left,
        Right,
        Down
    };

    // A two-part limb: where it bends and where it ends.
    export struct Limb
    {
        FloatPoint joint;
        FloatPoint end;
    };

    // Shapes that stand apart in one brush, drawn as the backend draws them fastest. See PathArt
    export class ScatteredShapes
    {
    public:
        // A stroke width of 0 fills the shapes.
        ScatteredShapes(const SceneFrame&, const Brush&, float strokeWidth = 0.0f,
            StrokeCap = StrokeCap::Round);
    public:
        // The path the next shape goes into; on the CPU backend the shape before it is drawn first.
        [[nodiscard]] PixelPath& next();
        void draw(); // what is still waiting to be drawn
    private:
        SceneFrame m_frame;
        Brush m_brush;
        float m_strokeWidth;
        StrokeCap m_cap;
        bool m_oneByOne;
        PixelPath m_path;
    };

    // MillScene's pseudo random numbers, 0 to 1: one index and shift always give one number.
    export [[nodiscard]] float pseudoRandom(int index, float shift = 0.0f);
    // The part of `value` past its whole number, 0 to 1 for negative values too.
    export [[nodiscard]] float fractionalPart(float value);
    // 0 below edge0, 1 above edge1 and an S-curve between; edge1 below edge0 runs the other way.
    export [[nodiscard]] constexpr float smoothStep(const float edge0, const float edge1,
        const float value)
    {
        const float share = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return share * share * (3.0f - 2.0f * share);
    }
    // A bell, 1 at `center`, falling to a third at `width` either side.
    export [[nodiscard]] float bump(float value, float center, float width);
    export [[nodiscard]] FloatPoint rotated(FloatPoint, float angle);
    export [[nodiscard]] FloatPoint pointBetween(FloatPoint from, FloatPoint to, float share);
    // Color::blend with the share held to 0..1.
    export [[nodiscard]] Color colorBetween(Color from, Color to, float share);
    export [[nodiscard]] Color faded(Color, float share); // the colour at a share of its own alpha
    // MillScene's spin in turns, which comes round with a shape's symmetry each cycle. See PathArt
    export [[nodiscard]] float calculateSyncRotation(float cycleTime, int index,
        float symmetryFactor);
    // A limb from `root` toward `target`, its end held within reach. See PathArt
    export [[nodiscard]] Limb reachToward(FloatPoint root, FloatPoint target, float upperLength,
        float lowerLength, BendToward);

    // An ellipse appended to the path, where PixelPath::drawEllipse starts a path of its own.
    export void addEllipse(PixelPath&, FloatPoint center, float radiusX, float radiusY);
    // MillScene's four-point star appended to the path, its points `radius` from the center.
    export void addSparkle(PixelPath&, FloatPoint center, float radius);

    export void fillShape(const SceneFrame&, const PixelPath&, const Brush&,
        const Matrix3x2& local = Matrix3x2::Identity());
    export void strokeShape(const SceneFrame&, const PixelPath&, const Brush&, float width,
        StrokeCap = StrokeCap::Round, const Matrix3x2& local = Matrix3x2::Identity());
    export void drawSegment(const SceneFrame&, FloatPoint from, FloatPoint to, const Brush&,
        float width, StrokeCap = StrokeCap::Round);
    export void fillDisc(const SceneFrame&, FloatPoint center, float radius, const Brush&);
    export void fillOval(const SceneFrame&, FloatPoint center, float radiusX, float radiusY,
        const Brush&, float angle = 0.0f);
    export void fillBox(const SceneFrame&, const FloatRect&, const Brush&);
    export void fillRoundedBox(const SceneFrame&, const FloatRect&, float radius, const Brush&);
    // A PointGlow around `center`; an xRatio below 1 widens it.
    export void drawGlow(const SceneFrame&, FloatPoint center, float spread, Color,
        float xRatio = 1.0f);
    // MillScene's four-point star.
    export void drawSparkle(const SceneFrame&, FloatPoint center, float radius, Color);
}
