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
        // The same frame with `local` applied ahead of its own move onto the canvas.
        [[nodiscard]] SceneFrame within(const Matrix3x2& local) const;
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

    // A field of twinkling stars across a band of the frame.
    export struct StarField
    {
        int count{ 100 };
        float top{ 0.0f };
        float bottom{ 0.0f };
        float seed{ 0.0f }; // picks the stars - two fields with one seed are one field
        Color color{ 0xFFEEF0FF };
        float opacity{ 1.0f };
        // How often the slowest star twinkles in a cycle; a long cycle raises it to keep the pace.
        float twinklesPerCycle{ 1.0f };
    };

    // MillScene's pseudo random numbers, 0 to 1: one index and shift always give one number.
    export [[nodiscard]] float pseudoRandom(int index, float shift = 0.0f);
    // The part of `value` past its whole number, 0 to 1 for negative values too.
    export [[nodiscard]] float fractionalPart(float value);
    // 0 below edge0, 1 above edge1 and an S-curve between; edge1 below edge0 runs the other way.
    export [[nodiscard]] float smoothStep(float edge0, float edge1, float value);
    // A bell, 1 at `center`, falling to a third at `width` either side.
    export [[nodiscard]] float bump(float value, float center, float width);
    export [[nodiscard]] FloatPoint rotated(FloatPoint, float angle);
    export [[nodiscard]] FloatPoint pointBetween(FloatPoint from, FloatPoint to, float share);
    // Color::blend with the share held to 0..1.
    export [[nodiscard]] Color colorBetween(Color from, Color to, float share);
    // A limb from `root` toward `target`, its end held within reach. See PathArt
    export [[nodiscard]] Limb reachToward(FloatPoint root, FloatPoint target, float upperLength,
        float lowerLength, BendToward);

    // An ellipse appended to the path, where PixelPath::drawEllipse starts a path of its own.
    export void addEllipse(PixelPath&, FloatPoint center, float radiusX, float radiusY);

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
    export void drawStars(const SceneFrame&, float cycleTime, const StarField&);
}
