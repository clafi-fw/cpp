export module ClaFi.Icons.ChannelTile;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Icons::ChannelTile
{
    using namespace ::ClaFi::Graphics;

    // A rounded square split three ways by a Y, each part one flat colour. Three regions hold up at
    // 16px, where a segmented gradient sweep has nothing left to say - that legibility is what this
    // form buys.
    export constexpr int k_partCount = 3;

    // Screen coordinates run y downwards, so a quarter turn points at 6 o'clock: the stem goes down
    // and the two arms up-left and up-right. Region 0 is then the left face, 1 the top, 2 the right
    // - the order every channel fills from the low end of its ramp.
    export constexpr float k_stemAngle = k_2Pi * 0.25f;

    export constexpr float k_cornerRatio = 0.24f;   // Corner radius, as a fraction of the tile size

    // The divider's width, as a fraction of the tile size. The channel icons carry a wide Y so the
    // three parts read as separate things rather than as faces of one solid.
    export constexpr float k_channelDividerRatio = 0.09f;
    export constexpr float k_minDividerWidth = 1.0f;    // Pixels, so the Y survives a 16px tile

    export using TileColors = std::array<Color, k_partCount>;
    export using ArmAngles = std::array<float, k_partCount>;

    // The three corners a part is left with where the cut meets the shape it was cut from.
    export constexpr int k_cutCornersPerPart = 3;

    // How far each of those corners is drawn into a curve, as a fraction of the tile size, three to a
    // part in the order the parts are drawn.
    export using CutSoftening = std::array<float, k_partCount * k_cutCornersPerPart>;

    // The same three parts cut out of a rounded square inset from the icon by the margin, each corner
    // the cut makes drawn into a curve. The inset shape's arcs are concentric with the icon's, so what
    // is left showing around the parts is the margin the whole way round, corners included - which is
    // what lets the parts sit on a ground rather than cover it. The arms are given rather than taken
    // from a joint: this tile carries no offset, the arms meeting at the centre.
    export void drawBlobTile(Canvas&, const FloatRect& iconRect, const TileColors&, const ArmAngles&,
        float marginRatio, float dividerRatio, const CutSoftening&);


    //----------------------------------------------------------------------------


    namespace
    {
        FloatPoint pointOn(FloatPoint origin, float angle, float distance)
        {
            return { origin.x + std::cos(angle) * distance, origin.y + std::sin(angle) * distance };
        }

        // The span a part covers. Arms are given in turn order, so only the wrap needs handling.
        float partEndAngle(const ArmAngles& arms, int partIndex)
        {
            float from = arms[partIndex];
            float to = arms[(partIndex + 1) % k_partCount];
            return to <= from ? to + k_2Pi : to;
        }

        // Which side of the line through origin along direction a point falls on, positive being the
        // side a part keeps.
        [[nodiscard]] float sideOf(FloatPoint origin, FloatPoint direction, FloatPoint point)
        {
            return direction.x * (point.y - origin.y) - direction.y * (point.x - origin.x);
        }

        // The inset shape is walked as a run of chords. Ten a quarter turn holds the deviation under a
        // tenth of a pixel at any size an icon is drawn, and keeps the shape the same at every size
        // rather than having it follow the pixel count.
        constexpr int k_arcSteps = 10;

        // Four arcs of one chord more than their steps, and a cut can add one point per pass.
        constexpr int k_maxOutlinePoints = 4 * (k_arcSteps + 1) + 2;

        struct Outline
        {
            std::array<FloatPoint, k_maxOutlinePoints> points{};
            std::array<bool, k_maxOutlinePoints> cut{};
            int count{ 0 };
        };

        using OutlineLengths = std::array<float, k_maxOutlinePoints>;

        // The rounded square the parts are cut from: the icon's own shape brought in by the margin on
        // every side, its radius brought in by the same, which is what makes the two concentric.
        void buildInnerSquare(Outline& outline, float size, float margin)
        {
            float half = size * 0.5f - margin;
            float radius = std::max(0.0f, size * k_cornerRatio - margin);

            struct ArcCentre
            {
                FloatPoint at{ 0.0f, 0.0f };
                float from{ 0.0f };
            };

            std::array<ArcCentre, 4> centres = {
                ArcCentre{ { half - radius, half - radius }, 0.0f },
                ArcCentre{ { -half + radius, half - radius }, 0.25f },
                ArcCentre{ { -half + radius, -half + radius }, 0.5f },
                ArcCentre{ { half - radius, -half + radius }, 0.75f },
            };

            outline.count = 0;
            for (const ArcCentre& centre : centres)
            {
                for (int step = 0; step <= k_arcSteps; ++step)
                {
                    float turn = centre.from + 0.25f * static_cast<float>(step) / k_arcSteps;
                    outline.points[outline.count] = pointOn(centre.at, turn * k_2Pi, radius);
                    outline.cut[outline.count] = false;
                    ++outline.count;
                }
            }
        }

        // What survives a cut keeps its mark; a point the cut itself makes is a corner to be softened.
        void clipOutline(Outline& outline, FloatPoint origin, FloatPoint direction)
        {
            Outline kept;
            for (int i = 0; i < outline.count; ++i)
            {
                const FloatPoint& current = outline.points[i];
                const FloatPoint& next = outline.points[(i + 1) % outline.count];
                float currentSide = sideOf(origin, direction, current);
                float nextSide = sideOf(origin, direction, next);

                if (currentSide >= 0.0f)
                {
                    kept.points[kept.count] = current;
                    kept.cut[kept.count] = outline.cut[i];
                    ++kept.count;
                }

                if (currentSide * nextSide >= 0.0f)
                    continue;

                float along = currentSide / (currentSide - nextSide);
                kept.points[kept.count] = {
                    current.x + (next.x - current.x) * along,
                    current.y + (next.y - current.y) * along,
                };
                kept.cut[kept.count] = true;
                ++kept.count;
            }
            outline = kept;
        }

        void measureOutline(const Outline& outline, OutlineLengths& lengths)
        {
            for (int i = 0; i < outline.count; ++i)
            {
                const FloatPoint& from = outline.points[i];
                const FloatPoint& to = outline.points[(i + 1) % outline.count];
                lengths[i] = std::hypot(to.x - from.x, to.y - from.y);
            }
        }

        // A position on the outline is the index of a point plus how far along the edge leaving it the
        // position stands, as a fraction of that edge.
        [[nodiscard]] FloatPoint pointAtPosition(const Outline& outline, float position)
        {
            int index = static_cast<int>(std::floor(position)) % outline.count;
            float fraction = position - std::floor(position);
            const FloatPoint& from = outline.points[index];
            const FloatPoint& to = outline.points[(index + 1) % outline.count];
            return {
                from.x + (to.x - from.x) * fraction,
                from.y + (to.y - from.y) * fraction,
            };
        }

        [[nodiscard]] float walkForward(const Outline& outline, const OutlineLengths& lengths,
            float position, float distance)
        {
            int index = static_cast<int>(std::floor(position));
            float fraction = position - static_cast<float>(index);
            float left = distance;

            for (int step = 0; step <= outline.count; ++step)
            {
                float remaining = lengths[index] * (1.0f - fraction);
                if (lengths[index] <= 0.0f)
                {
                    index = (index + 1) % outline.count;
                    fraction = 0.0f;
                    continue;
                }
                if (remaining >= left)
                    return static_cast<float>(index) + std::min(fraction + left / lengths[index], 0.999999f);
                left -= remaining;
                index = (index + 1) % outline.count;
                fraction = 0.0f;
            }
            return static_cast<float>(index);
        }

        [[nodiscard]] float walkBack(const Outline& outline, const OutlineLengths& lengths,
            float position, float distance)
        {
            int index = static_cast<int>(std::floor(position));
            float fraction = position - static_cast<float>(index);
            float left = distance;

            for (int step = 0; step <= outline.count; ++step)
            {
                float covered = lengths[index] * fraction;
                if (covered >= left and lengths[index] > 0.0f)
                    return static_cast<float>(index) + std::max(fraction - left / lengths[index], 0.0f);
                left -= covered;
                index = (index - 1 + outline.count) % outline.count;
                fraction = 1.0f;
            }
            return static_cast<float>(index);
        }

        // How much outline lies between two positions, walking the way the points are wound.
        [[nodiscard]] float spanBetween(const Outline& outline, const OutlineLengths& lengths,
            float from, float to)
        {
            float total = 0.0f;
            int index = static_cast<int>(std::floor(from));
            float fraction = from - static_cast<float>(index);

            for (int step = 0; step <= outline.count; ++step)
            {
                if (static_cast<int>(std::floor(to)) == index and to - static_cast<float>(index) >= fraction)
                    return total + lengths[index] * (to - static_cast<float>(index) - fraction);
                total += lengths[index] * (1.0f - fraction);
                index = (index + 1) % outline.count;
                fraction = 0.0f;
            }
            return total;
        }

        // The corners are softened by walking the outline rather than by taking a bite out of the two
        // edges either side. A corner standing on an arc has a run of short chords next to it, and an
        // edge-by-edge fillet would be held to the length of the first one.
        void traceSoftened(PixelPath& path, const Outline& outline, const float* softening, float size)
        {
            OutlineLengths lengths{};
            measureOutline(outline, lengths);

            std::array<int, k_cutCornersPerPart> corners{};
            std::array<float, k_cutCornersPerPart> wanted{};
            int cornerCount = 0;
            for (int i = 0; i < outline.count and cornerCount < k_cutCornersPerPart; ++i)
            {
                if (!outline.cut[i])
                    continue;
                corners[cornerCount] = i;
                wanted[cornerCount] = softening[cornerCount] * size;
                ++cornerCount;
            }

            if (cornerCount == 0)
            {
                path.clear();
                path.moveTo(outline.points[0]);
                for (int i = 1; i < outline.count; ++i)
                    path.lineTo(outline.points[i]);
                path.close();
                return;
            }

            // Two corners sharing a stretch of outline cannot take more of it than it holds.
            for (int i = 0; i < cornerCount; ++i)
            {
                int next = (i + 1) % cornerCount;
                float span = spanBetween(outline, lengths,
                    static_cast<float>(corners[i]), static_cast<float>(corners[next]));
                float asked = wanted[i] + wanted[next];
                if (asked <= span or asked <= 0.0f)
                    continue;
                float scale = span / asked;
                wanted[i] *= scale;
                wanted[next] *= scale;
            }

            std::array<float, k_cutCornersPerPart> entering{};
            std::array<float, k_cutCornersPerPart> leaving{};
            for (int i = 0; i < cornerCount; ++i)
            {
                entering[i] = walkBack(outline, lengths, static_cast<float>(corners[i]), wanted[i]);
                leaving[i] = walkForward(outline, lengths, static_cast<float>(corners[i]), wanted[i]);
            }

            path.clear();
            path.moveTo(pointAtPosition(outline, leaving[0]));

            for (int i = 0; i < cornerCount; ++i)
            {
                int next = (i + 1) % cornerCount;
                float stop = entering[next] < leaving[i]
                    ? entering[next] + static_cast<float>(outline.count)
                    : entering[next];

                for (int index = static_cast<int>(std::ceil(leaving[i]));
                    static_cast<float>(index) < stop; ++index)
                {
                    path.lineTo(outline.points[index % outline.count]);
                }

                path.lineTo(pointAtPosition(outline, entering[next]));
                path.quadTo(outline.points[corners[next]], pointAtPosition(outline, leaving[next]));
            }
            path.close();
        }
    }

    void drawBlobTile(Canvas& canvas, const FloatRect& iconRect, const TileColors& colors,
        const ArmAngles& arms, float marginRatio, float dividerRatio, const CutSoftening& softening)
    {
        float size = std::min(iconRect.width(), iconRect.height());
        Matrix3x2 transform = Matrix3x2::translation(iconRect.center());
        float margin = size * marginRatio;
        float halfDivider = std::max(k_minDividerWidth, size * dividerRatio) * 0.5f;

        PixelPath path;

        for (int i = 0; i < k_partCount; ++i)
        {
            float from = arms[i];
            float to = partEndAngle(arms, i);
            float halfSpan = (to - from) * 0.5f;
            FloatPoint vertex = pointOn({ 0.0f, 0.0f }, from + halfSpan, halfDivider / std::sin(halfSpan));

            Outline outline;
            buildInnerSquare(outline, size, margin);
            clipOutline(outline, vertex, { std::cos(from), std::sin(from) });
            clipOutline(outline, vertex, { -std::cos(to), -std::sin(to) });
            if (outline.count < 3)
                continue;

            traceSoftened(path, outline, softening.data() + i * k_cutCornersPerPart, size);
            canvas.fillPath(path, SolidColor{ colors[i] }, &transform);
        }
    }

}
