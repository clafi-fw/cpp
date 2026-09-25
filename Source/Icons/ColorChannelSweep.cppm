export module ClaFi.Icons.ColorChannelSweep;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Icons::ColorChannelSweep
{
    using namespace ::ClaFi::Graphics;

    // Neither backend has a conic gradient, so the sweep is built from segments, each carrying a
    // two-stop linear gradient between the channel values at its two ends. Twelve is where the
    // banding at the joins stops being visible on a monotonic ramp - six is enough for hue, where
    // the primaries land on segment boundaries, but not for a gray to white one.
    export constexpr int k_segmentCount = 12;

    // Screen coordinates run y downwards, so a positive quarter turn points at 6 o'clock. Every
    // icon in the set starts and ends its sweep there.
    export constexpr float k_seamAngle = k_2Pi * 0.25f;

    // Which shape the sweep is drawn as. See Icons
    export enum class ChannelForm
    {
        Ring,
        Pie
    };

    export constexpr ChannelForm k_channelForm = ChannelForm::Pie;

    // Metrics as fractions of the icon size. For the ring, mid-band radius plus half the band puts
    // the outer edge on the rect the caller reserved, so those two have to change together; the pie
    // carries no outline, so its radius is simply half the rect.
    export constexpr float k_ringRadiusRatio = 0.42f;
    export constexpr float k_ringBandRatio = 0.16f;
    export constexpr float k_pieRadiusRatio = 0.5f;

    // Whether the wedges of a pie meet, or stop short and let the backdrop through. See Icons
    export enum class WedgeJoint
    {
        Sealed, // wedges overlap, so the sweep is one continuous disc
        // Wedges stop short, leaving the surface behind the icon showing as spokes. See Icons
        Spoked
    };

    export constexpr WedgeJoint k_wedgeJoint = WedgeJoint::Sealed;

    // Widths in pixels, at the rim, converted to an angle against whatever radius is in play. Both
    // of these were angles once, which was the bug that produced the spokes by accident: a constant
    // angle is a shrinking distance as the icon gets smaller, and 0.005 rad came to 0.07px at icon
    // size, far too little to close a join. Anything measured against the rasterizer belongs in
    // pixels.
    export constexpr float k_spokeWidth = 0.4f;
    export constexpr float k_sealOverlap = 0.75f;

    // What happens where the sweep closes on itself.
    export enum class SweepEnds
    {
        Cyclic, // the channel wraps: the colour at the seam is the same from both sides. See Icons
        Stepped // the channel runs from one end to the other. See Icons
    };

    // One colour per segment boundary, so k_segmentCount + 1 of them: entry i is the channel at
    // i / k_segmentCount of the way round, clockwise from the seam. For a Cyclic sweep the first
    // and last entry are the same colour.
    export using BoundaryColors = std::array<Color, k_segmentCount + 1>;

    export float boundaryAngle(int boundaryIndex);

    // Lays the sweep out inside the icon rect in whichever form k_channelForm selects. The whole
    // trio goes through here, so the three land on identical metrics without repeating the
    // arithmetic, and the form switch reaches all of them at once.
    export void drawChannelSweep(Canvas&, const FloatRect& iconRect, const BoundaryColors&, SweepEnds);

    export void drawRingSweep(Canvas&, FloatPoint center, float radius, float bandWidth,
        const BoundaryColors&, SweepEnds, const Matrix3x2& transform);

    // Carries no outline: the wedges reach the edge of the rect on their own.
    export void drawPieSweep(Canvas&, FloatPoint center, float radius,
        const BoundaryColors&, SweepEnds, const Matrix3x2& transform);


    //----------------------------------------------------------------------------


    namespace
    {
        // The angular span a segment covers. Boundaries are shared between neighbours, so what
        // separates them is how far each one over- or under-shoots.
        struct SegmentSpan
        {
            float from;
            float to;
        };

        // A width at the rim, as the angle that subtends it. Guards a degenerate radius so a
        // zero-sized icon cannot divide by zero on its way to drawing nothing.
        float angleForWidth(float width, float radius)
        {
            return width / std::max(radius, 1.0f);
        }

        FloatPoint pointOn(FloatPoint center, float radius, float angle)
        {
            return { center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius };
        }

        // One segment as a single quadratic. The control point sits where the two end tangents
        // meet, which puts both ends exactly on the circle. Good to about a quarter turn - past
        // that the middle of the arc starts to sag, so k_segmentCount must stay above 4.
        void addArc(PixelPath& path, FloatPoint center, float radius, float from, float to)
        {
            float ctrlRadius = radius / std::cos((to - from) * 0.5f);
            path.quadTo(pointOn(center, ctrlRadius, (from + to) * 0.5f), pointOn(center, radius, to));
        }

        // Overshoots the end boundary so the next segment covers the join, rather than the two
        // meeting on a shared antialiased edge that shows as a hairline. The last segment of a
        // Stepped sweep closes on the seam, and that is the one join that must stay hard.
        SegmentSpan sealedSpan(int segmentIndex, float radius, SweepEnds ends)
        {
            bool closesTheSeam = (segmentIndex + 1 == k_segmentCount) && (ends == SweepEnds::Stepped);
            float overlap = closesTheSeam ? 0.0f : angleForWidth(k_sealOverlap, radius);
            return {
                boundaryAngle(segmentIndex),
                boundaryAngle(segmentIndex + 1) + overlap
            };
        }

        // Undershoots both boundaries by half a spoke, so the gap is centred on the boundary and
        // neighbours contribute equally to it. The gap is angular, so it tapers away towards the
        // centre - which is what makes it read as a spoke rather than a slice, and why the width is
        // given for the rim where it is widest.
        SegmentSpan spokedSpan(int segmentIndex, float radius)
        {
            float halfGap = angleForWidth(k_spokeWidth, radius) * 0.5f;
            return {
                boundaryAngle(segmentIndex) + halfGap,
                boundaryAngle(segmentIndex + 1) - halfGap
            };
        }

        LinearGradient segmentBrush(FloatPoint startPt, FloatPoint endPt, Color startColor, Color endColor)
        {
            // Brush points are path-local: the CPU painter maps them through the draw transform and
            // D2D sets the same matrix on the render target, so both agree without a second matrix.
            return LinearGradient::simple(startPt, endPt, startColor, endColor);
        }
    }

    float boundaryAngle(int boundaryIndex)
    {
        return k_seamAngle + k_2Pi * boundaryIndex / k_segmentCount;
    }

    void drawChannelSweep(Canvas& canvas, const FloatRect& iconRect, const BoundaryColors& colors, SweepEnds ends)
    {
        float size = std::min(iconRect.width(), iconRect.height());

        if constexpr (k_channelForm == ChannelForm::Ring)
        {
            // The band is centred on the radius, so half of it hangs outside: the rect has to give
            // that half back before the radius is taken, and the origin shifts by the same half.
            float bandWidth = std::max(1.0f, size * k_ringBandRatio);
            size -= bandWidth;

            FloatPoint center = { size * 0.5f, size * 0.5f };
            Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft() + FloatPoint{ bandWidth * 0.5f });

            drawRingSweep(canvas, center, size * k_ringRadiusRatio, bandWidth, colors, ends, transform);
        }
        else
        {
            FloatPoint center = { size * 0.5f, size * 0.5f };
            Matrix3x2 transform = Matrix3x2::translation(iconRect.topLeft());

            drawPieSweep(canvas, center, size * k_pieRadiusRatio, colors, ends, transform);
        }
    }

    void drawRingSweep(Canvas& canvas, FloatPoint center, float radius, float bandWidth,
        const BoundaryColors& colors, SweepEnds ends, const Matrix3x2& transform)
    {
        PixelPath path;

        for (int i = 0; i < k_segmentCount; ++i)
        {
            // Always sealed: the ring's joins are along the band, and a gap there would read as a
            // dashed arc rather than as a spoke.
            SegmentSpan span = sealedSpan(i, radius, ends);
            FloatPoint startPt = pointOn(center, radius, span.from);
            FloatPoint endPt = pointOn(center, radius, span.to);

            path.clear();
            path.moveTo(startPt);
            addArc(path, center, radius, span.from, span.to);

            // Butt caps, not round: a round cap on a band this wide bulges half the band width past
            // the end of its arc, and twelve of those turn the outer edge into a scallop. A butt cap
            // ends on a radius, so the angular overlap above closes the join flat - and leaves the
            // seam of a Stepped sweep as a clean radial edge.
            PathDrawLayer layer{
                .geometry{
                    .mode = PathRenderMode::Stroke,
                    .strokeWidth = bandWidth,
                    .strokeCap = StrokeCap::Butt
                },
                .brush{ segmentBrush(startPt, endPt, colors[i], colors[i + 1]) }
            };
            canvas.drawPath(path, { layer }, &transform);
        }
    }

    void drawPieSweep(Canvas& canvas, FloatPoint center, float radius,
        const BoundaryColors& colors, SweepEnds ends, const Matrix3x2& transform)
    {
        PixelPath path;

        for (int i = 0; i < k_segmentCount; ++i)
        {
            SegmentSpan span = k_wedgeJoint == WedgeJoint::Spoked
                ? spokedSpan(i, radius)
                : sealedSpan(i, radius, ends);
            FloatPoint startPt = pointOn(center, radius, span.from);
            FloatPoint endPt = pointOn(center, radius, span.to);

            path.clear();
            path.moveTo(center);
            path.lineTo(startPt);
            addArc(path, center, radius, span.from, span.to);
            path.close();

            PathDrawLayer layer{
                .geometry{},
                .brush{ segmentBrush(startPt, endPt, colors[i], colors[i + 1]) }
            };
            canvas.drawPath(path, { layer }, &transform);
        }
    }

}
