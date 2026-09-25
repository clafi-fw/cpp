module ThisApp.AppIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ThisApp::AppIcon
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    namespace
    {
        using Corners = std::array<FloatPoint, 4>;
        using CornerRadii = std::array<float, 4>;

        // WhatsClip's leaf: each corner carries a fixed offset, so no two sides are quite parallel.
        struct Leaf
        {
            float left{};
            float top{};
            float right{};
            float bottom{};
            float radius{};
            Corners offsets{};
        };

        // From a fraction of the leaf's height to its foot, so bands overlap and no seam shows.
        struct Band
        {
            float from{};
            Color color{};
        };

        struct Drop
        {
            FloatPoint center{};
            float radius{};
        };

        struct Extent
        {
            float left{};
            float top{};
            float width{};
            float height{};
        };

        // A colour as a lightness and two opponent axes, over which a straight mix looks even.
        struct Oklab
        {
            float lightness{};
            float a{};
            float b{};
        };

        constexpr float k_epsilon{ 0.0001f };

        constexpr Color k_outlineColor{ 0xFF9B9BA2 };
        constexpr Color k_edgeColor{ 0xFFF4F5F8 };
        constexpr Color k_keylineColor{ 0xFF3A3A44 };
        constexpr float k_outlineWidth{ 0.020f };   // the line round each leaf
        constexpr float k_edgeWidth{ 0.020f };      // the light line round the blot
        constexpr float k_keylineWidth{ 0.020f };   // the dark line inside the blot's light one

        // Greys on the line from the keyline colour to the edge colour, lightest at the top band.
        constexpr Color k_backUpper{ 0xFFF4F5F8 };
        constexpr Color k_backLower{ 0xFFE1E2E6 };
        constexpr Color k_frontUpper{ 0xFFECEDF0 };
        constexpr Color k_frontMiddle{ 0xFFD9D9DE };
        constexpr Color k_frontLower{ 0xFFC6C6CB };

        constexpr Leaf k_backLeaf{
            .left = 0.05f,
            .top = 0.07f,
            .right = 0.55f,
            .bottom = 0.75f,
            .radius = 0.085f,
            .offsets = { FloatPoint{ 0.008f, -0.005f }, FloatPoint{ -0.006f, 0.008f },
                         FloatPoint{ 0.005f, 0.006f }, FloatPoint{ -0.007f, -0.004f } }
        };

        constexpr Leaf k_frontLeaf{
            .left = 0.40f,
            .top = 0.27f,
            .right = 0.95f,
            .bottom = 0.90f,
            .radius = 0.095f,
            .offsets = { FloatPoint{ -0.007f, 0.006f }, FloatPoint{ 0.009f, -0.005f },
                         FloatPoint{ -0.005f, -0.008f }, FloatPoint{ 0.006f, 0.005f } }
        };

        constexpr std::array<Band, 2> k_backBands{
            Band{ .from = 0.00f, .color = k_backUpper },
            Band{ .from = 0.45f, .color = k_backLower }
        };

        constexpr std::array<Band, 3> k_frontBands{
            Band{ .from = 0.00f, .color = k_frontUpper },
            Band{ .from = 0.34f, .color = k_frontMiddle },
            Band{ .from = 0.67f, .color = k_frontLower }
        };

        // WhatsClip's box, tail included, so the leaves land on the same pixels in every mark.
        constexpr Extent k_markBox{ .left = 0.033f, .top = 0.055f, .width = 0.936f, .height = 0.8834f };

        // The blot, as the controls of a closed quadratic B-spline.
        constexpr std::array<FloatPoint, 48> k_outline{
            FloatPoint{ 0.3614f, 0.2417f }, FloatPoint{ 0.4003f, 0.2602f }, FloatPoint{ 0.4403f, 0.2603f },
            FloatPoint{ 0.4809f, 0.2505f }, FloatPoint{ 0.5219f, 0.2568f }, FloatPoint{ 0.5607f, 0.2681f },
            FloatPoint{ 0.5960f, 0.2929f }, FloatPoint{ 0.6366f, 0.2894f }, FloatPoint{ 0.6785f, 0.2899f },
            FloatPoint{ 0.7041f, 0.3280f }, FloatPoint{ 0.6931f, 0.3674f }, FloatPoint{ 0.6922f, 0.4106f },
            FloatPoint{ 0.7258f, 0.4363f }, FloatPoint{ 0.7647f, 0.4515f }, FloatPoint{ 0.7745f, 0.4969f },
            FloatPoint{ 0.7442f, 0.5259f }, FloatPoint{ 0.7052f, 0.5420f }, FloatPoint{ 0.6907f, 0.5828f },
            FloatPoint{ 0.6682f, 0.6160f }, FloatPoint{ 0.6452f, 0.6506f }, FloatPoint{ 0.6479f, 0.6935f },
            FloatPoint{ 0.6155f, 0.7253f }, FloatPoint{ 0.5742f, 0.7128f }, FloatPoint{ 0.5393f, 0.6956f },
            FloatPoint{ 0.4937f, 0.7025f }, FloatPoint{ 0.4713f, 0.7347f }, FloatPoint{ 0.4614f, 0.7768f },
            FloatPoint{ 0.4228f, 0.7977f }, FloatPoint{ 0.3821f, 0.7856f }, FloatPoint{ 0.3616f, 0.7465f },
            FloatPoint{ 0.3770f, 0.7067f }, FloatPoint{ 0.3806f, 0.6669f }, FloatPoint{ 0.3489f, 0.6350f },
            FloatPoint{ 0.3161f, 0.6146f }, FloatPoint{ 0.2711f, 0.6174f }, FloatPoint{ 0.2480f, 0.5768f },
            FloatPoint{ 0.2746f, 0.5410f }, FloatPoint{ 0.2743f, 0.5007f }, FloatPoint{ 0.2459f, 0.4674f },
            FloatPoint{ 0.2054f, 0.4586f }, FloatPoint{ 0.1722f, 0.4325f }, FloatPoint{ 0.1664f, 0.3895f },
            FloatPoint{ 0.1922f, 0.3551f }, FloatPoint{ 0.2333f, 0.3472f }, FloatPoint{ 0.2742f, 0.3636f },
            FloatPoint{ 0.3114f, 0.3417f }, FloatPoint{ 0.3042f, 0.2962f }, FloatPoint{ 0.3207f, 0.2589f }
        };

        constexpr std::array<Drop, 3> k_drops{
            Drop{ .center = FloatPoint{ 0.2379f, 0.2485f }, .radius = 0.0327f },
            Drop{ .center = FloatPoint{ 0.6560f, 0.7976f }, .radius = 0.0436f },
            Drop{ .center = FloatPoint{ 0.4942f, 0.1689f }, .radius = 0.0417f }
        };

        // The sweep's anchors, a sixth of the circle apart, each as saturated as sRGB allows.
        constexpr std::array<Color, 6> k_rainbow{
            Color{ 0xFFFC0035 },
            Color{ 0xFFF27700 },
            Color{ 0xFFF4C100 },
            Color{ 0xFF00B959 },
            Color{ 0xFF00AAEB },
            Color{ 0xFF9654FF }
        };

        constexpr std::size_t k_sweepSteps{ 72 };
        constexpr std::size_t k_stepsPerAnchor{ k_sweepSteps / k_rainbow.size() };
        constexpr float k_wedgeAngle{ k_2Pi / static_cast<float>(k_sweepSteps) };
        // The first wedge, red, centred straight up.
        constexpr float k_sweepStart{ -k_2Pi * 0.25f - k_wedgeAngle * 0.5f };
        constexpr FloatPoint k_sweepCenter{ 0.4775f, 0.4851f };   // the outline's centroid
        // Each wedge reaches under the next one, so no seam of the leaves shows between two fills.
        constexpr float k_wedgeOverlap{ 0.02f };
        constexpr float k_wedgeReach{ 2.0f };   // past the blot, whose outline clips the wedges

        using SweepColors = std::array<Color, k_sweepSteps>;

        [[nodiscard]] FloatPoint mix(FloatPoint from, FloatPoint to, float amount);
        [[nodiscard]] FloatPoint towards(FloatPoint from, FloatPoint to, float distance);
        [[nodiscard]] Corners cornersOf(const Leaf&);
        [[nodiscard]] Corners bandCorners(const Corners& leaf, const Band&);
        [[nodiscard]] FloatPoint sweepPoint(float angle);
        [[nodiscard]] Matrix3x2 fitTo(const FloatRect& bounds);

        [[nodiscard]] float toLinear(ColorByte channel);
        [[nodiscard]] ColorByte toChannel(float linear);
        [[nodiscard]] Oklab toOklab(Color);
        [[nodiscard]] Color fromOklab(const Oklab&);
        [[nodiscard]] Color mixOklab(Color from, Color to, float amount);
        [[nodiscard]] SweepColors mixSweep();
        [[nodiscard]] const SweepColors& sweepColors();

        void addRoundedQuad(PixelPath& path, const Corners&, const CornerRadii& radii);
        void addCircle(PixelPath& path, FloatPoint center, float radius);
        void addOutline(PixelPath& path);

        void fillBand(Canvas&, const Matrix3x2& transform, const Leaf&, const Band&);
        void strokeLeaf(Canvas&, const Matrix3x2& transform, const Leaf&);
        void paintLeaves(Canvas&, const Matrix3x2& transform);
        void paintBlot(Canvas&, const Matrix3x2& transform);
        void paintSweep(Canvas&, const Matrix3x2& transform);
    }
}

//-----------------------------------------------------------------------------

namespace ThisApp::AppIcon
{
    void paint(Canvas& canvas, const FloatRect& bounds, const Opacity opacity)
    {
        // One layer for the whole mark: its parts overlap, and faded one by one they would show.
        const ScopedCanvasOpacity layer{ canvas, opacity };
        const Matrix3x2 transform = fitTo(bounds);

        paintLeaves(canvas, transform);
        paintBlot(canvas, transform);
    }

    void paintIcon(PaintIconEvent& event)
    {
        paint(event.canvas(), event.iconRect(), 1.0f - event.disabledAmount());
    }

    namespace
    {
        FloatPoint mix(const FloatPoint from, const FloatPoint to, const float amount)
        {
            return {
                from.x + (to.x - from.x) * amount,
                from.y + (to.y - from.y) * amount
            };
        }

        FloatPoint towards(const FloatPoint from, const FloatPoint to, const float distance)
        {
            const float dx = to.x - from.x;
            const float dy = to.y - from.y;
            const float length = std::hypot(dx, dy);
            if (length < k_epsilon)
                return from;

            return {
                from.x + dx / length * distance,
                from.y + dy / length * distance
            };
        }

        Corners cornersOf(const Leaf& leaf)
        {
            const Corners base = {
                FloatPoint{ leaf.left, leaf.top },
                FloatPoint{ leaf.right, leaf.top },
                FloatPoint{ leaf.right, leaf.bottom },
                FloatPoint{ leaf.left, leaf.bottom }
            };

            Corners result = {};
            for (std::size_t index = 0; index < base.size(); ++index)
            {
                result[index] = {
                    base[index].x + leaf.offsets[index].x,
                    base[index].y + leaf.offsets[index].y
                };
            }

            return result;
        }

        Corners bandCorners(const Corners& leaf, const Band& band)
        {
            return {
                mix(leaf[0], leaf[3], band.from),
                mix(leaf[1], leaf[2], band.from),
                leaf[2],
                leaf[3]
            };
        }

        FloatPoint sweepPoint(const float angle)
        {
            return {
                k_sweepCenter.x + std::cos(angle) * k_wedgeReach,
                k_sweepCenter.y + std::sin(angle) * k_wedgeReach
            };
        }

        Matrix3x2 fitTo(const FloatRect& bounds)
        {
            const float scaleX = bounds.width() / k_markBox.width;
            const float scaleY = bounds.height() / k_markBox.height;
            const FloatPoint origin = bounds.topLeft();

            return {
                scaleX, 0.0f,
                0.0f, scaleY,
                origin.x - k_markBox.left * scaleX,
                origin.y - k_markBox.top * scaleY
            };
        }

        float toLinear(const ColorByte channel)
        {
            const float value = static_cast<float>(channel) / 255.0f;
            if (value <= 0.04045f)
                return value / 12.92f;

            return std::pow((value + 0.055f) / 1.055f, 2.4f);
        }

        ColorByte toChannel(const float linear)
        {
            const float clamped = std::clamp(linear, 0.0f, 1.0f);
            const float value = clamped <= 0.0031308f
                ? clamped * 12.92f
                : 1.055f * std::pow(clamped, 1.0f / 2.4f) - 0.055f;

            return static_cast<ColorByte>(std::lround(value * 255.0f));
        }

        Oklab toOklab(const Color color)
        {
            const float red = toLinear(color.red);
            const float green = toLinear(color.green);
            const float blue = toLinear(color.blue);
            const float longCone = std::cbrt(0.4122214708f * red + 0.5363325363f * green + 0.0514459929f * blue);
            const float mediumCone = std::cbrt(0.2119034982f * red + 0.6806995451f * green + 0.1073969566f * blue);
            const float shortCone = std::cbrt(0.0883024619f * red + 0.2817188376f * green + 0.6299787005f * blue);

            return {
                0.2104542553f * longCone + 0.7936177850f * mediumCone - 0.0040720468f * shortCone,
                1.9779984951f * longCone - 2.4285922050f * mediumCone + 0.4505937099f * shortCone,
                0.0259040371f * longCone + 0.7827717662f * mediumCone - 0.8086757660f * shortCone
            };
        }

        Color fromOklab(const Oklab& oklab)
        {
            const float longRoot = oklab.lightness + 0.3963377774f * oklab.a + 0.2158037573f * oklab.b;
            const float mediumRoot = oklab.lightness - 0.1055613458f * oklab.a - 0.0638541728f * oklab.b;
            const float shortRoot = oklab.lightness - 0.0894841775f * oklab.a - 1.2914855480f * oklab.b;
            const float longCone = longRoot * longRoot * longRoot;
            const float mediumCone = mediumRoot * mediumRoot * mediumRoot;
            const float shortCone = shortRoot * shortRoot * shortRoot;

            return {
                toChannel(4.0767416621f * longCone - 3.3077115913f * mediumCone + 0.2309699292f * shortCone),
                toChannel(-1.2684380046f * longCone + 2.6097574011f * mediumCone - 0.3413193965f * shortCone),
                toChannel(-0.0041960863f * longCone - 0.7034186147f * mediumCone + 1.7076147010f * shortCone)
            };
        }

        Color mixOklab(const Color from, const Color to, const float amount)
        {
            const Oklab start = toOklab(from);
            const Oklab end = toOklab(to);

            return fromOklab({
                start.lightness + (end.lightness - start.lightness) * amount,
                start.a + (end.a - start.a) * amount,
                start.b + (end.b - start.b) * amount
            });
        }

        SweepColors mixSweep()
        {
            SweepColors colors = {};
            for (std::size_t step = 0; step < colors.size(); ++step)
            {
                const std::size_t anchor = step / k_stepsPerAnchor;
                const std::size_t within = step % k_stepsPerAnchor;
                const float amount = static_cast<float>(within) / static_cast<float>(k_stepsPerAnchor);
                colors[step] = mixOklab(k_rainbow[anchor], k_rainbow[(anchor + 1) % k_rainbow.size()], amount);
            }

            return colors;
        }

        const SweepColors& sweepColors()
        {
            // Mixed once: the anchors are constants, so the colours are the same for every paint.
            static const SweepColors s_colors = mixSweep();
            return s_colors;
        }

        void addRoundedQuad(PixelPath& path, const Corners& corners, const CornerRadii& radii)
        {
            bool started = false;
            for (std::size_t index = 0; index < corners.size(); ++index)
            {
                const FloatPoint previous = corners[(index + 3) % corners.size()];
                const FloatPoint current = corners[index];
                const FloatPoint next = corners[(index + 1) % corners.size()];

                const float toPrevious = std::hypot(current.x - previous.x, current.y - previous.y);
                const float toNext = std::hypot(next.x - current.x, next.y - current.y);
                const float radius = std::min({ radii[index], toPrevious * 0.5f, toNext * 0.5f });

                const FloatPoint entry = towards(current, previous, radius);
                const FloatPoint exit = towards(current, next, radius);

                if (started)
                    path.lineTo(entry);
                else
                {
                    path.moveTo(entry);
                    started = true;
                }

                if (radius > k_epsilon)
                    path.quadTo(current, exit);
            }

            path.close();
        }

        void addCircle(PixelPath& path, const FloatPoint center, const float radius)
        {
            const float control = radius * 0.4142f;
            const float diagonal = radius * 0.7071f;

            path.moveTo({ center.x, center.y - radius });
            path.quadTo({ center.x + control, center.y - radius }, { center.x + diagonal, center.y - diagonal });
            path.quadTo({ center.x + radius, center.y - control }, { center.x + radius, center.y });
            path.quadTo({ center.x + radius, center.y + control }, { center.x + diagonal, center.y + diagonal });
            path.quadTo({ center.x + control, center.y + radius }, { center.x, center.y + radius });
            path.quadTo({ center.x - control, center.y + radius }, { center.x - diagonal, center.y + diagonal });
            path.quadTo({ center.x - radius, center.y + control }, { center.x - radius, center.y });
            path.quadTo({ center.x - radius, center.y - control }, { center.x - diagonal, center.y - diagonal });
            path.quadTo({ center.x - control, center.y - radius }, { center.x, center.y - radius });
            path.close();
        }

        // Each quad runs from the midpoint before its control to the midpoint after it.
        void addOutline(PixelPath& path)
        {
            const std::size_t count = k_outline.size();
            path.moveTo(mix(k_outline[count - 1], k_outline[0], 0.5f));
            for (std::size_t index = 0; index < count; ++index)
                path.quadTo(k_outline[index], mix(k_outline[index], k_outline[(index + 1) % count], 0.5f));

            path.close();
        }

        void fillBand(Canvas& canvas, const Matrix3x2& transform, const Leaf& leaf, const Band& band)
        {
            const float top = band.from > 0.0f ? 0.0f : leaf.radius;
            const CornerRadii radii = { top, top, leaf.radius, leaf.radius };

            PixelPath path;
            addRoundedQuad(path, bandCorners(cornersOf(leaf), band), radii);
            canvas.drawPath(path, { PathDrawLayer::fill(band.color) }, &transform);
        }

        void strokeLeaf(Canvas& canvas, const Matrix3x2& transform, const Leaf& leaf)
        {
            const CornerRadii radii = { leaf.radius, leaf.radius, leaf.radius, leaf.radius };

            PixelPath path;
            addRoundedQuad(path, cornersOf(leaf), radii);
            canvas.drawPath(path, { PathDrawLayer::stroke(k_outlineColor, k_outlineWidth) }, &transform);
        }

        // A leaf takes its outline before the next covers it, so the front one draws the seam.
        void paintLeaves(Canvas& canvas, const Matrix3x2& transform)
        {
            for (const Band& band : k_backBands)
                fillBand(canvas, transform, k_backLeaf, band);

            strokeLeaf(canvas, transform, k_backLeaf);

            for (const Band& band : k_frontBands)
                fillBand(canvas, transform, k_frontLeaf, band);

            strokeLeaf(canvas, transform, k_frontLeaf);
        }

        // Both edges go down before the sweep, which covers their inner halves.
        void paintBlot(Canvas& canvas, const Matrix3x2& transform)
        {
            PixelPath outline;
            addOutline(outline);
            for (const Drop& drop : k_drops)
                addCircle(outline, drop.center, drop.radius);

            const float edgeStroke = (k_keylineWidth + k_edgeWidth) * 2.0f;
            const float keylineStroke = k_keylineWidth * 2.0f;
            canvas.drawPath(outline, { PathDrawLayer::stroke(k_edgeColor, edgeStroke) }, &transform);
            canvas.drawPath(outline, { PathDrawLayer::stroke(k_keylineColor, keylineStroke) }, &transform);

            canvas.pushClip(outline, &transform);
            paintSweep(canvas, transform);
            canvas.popClip();
        }

        void paintSweep(Canvas& canvas, const Matrix3x2& transform)
        {
            const SweepColors& colors = sweepColors();
            for (std::size_t step = 0; step < colors.size(); ++step)
            {
                const float from = k_sweepStart + k_wedgeAngle * static_cast<float>(step);

                PixelPath wedge;
                wedge.moveTo(k_sweepCenter);
                wedge.lineTo(sweepPoint(from));
                wedge.lineTo(sweepPoint(from + k_wedgeAngle + k_wedgeOverlap));
                wedge.close();
                canvas.drawPath(wedge, { PathDrawLayer::fill(colors[step]) }, &transform);
            }
        }
    }
}
