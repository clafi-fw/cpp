module ClaFi.Showcase.MillScene.Icon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Showcase::AppIcon
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        using Corners = std::array<FloatPoint, 4>;
        using CornerRadii = std::array<float, 4>;

        // The Themes app's leaf: each corner carries an offset, so no two sides are quite parallel.
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

        // One curve of an outline, running on from where the one before it ended.
        struct Cubic
        {
            FloatPoint control1{};
            FloatPoint control2{};
            FloatPoint end{};
        };

        struct Extent
        {
            float left{};
            float top{};
            float width{};
            float height{};
        };

        constexpr float k_epsilon{ 0.0001f };

        constexpr Color k_outlineColor{ 0xFFF4F5F8 };
        constexpr float k_outlineWidth{ 0.020f }; // the light line round each leaf

        // Five steps from the sky's blue to the meadow's green, mixed in Oklab, one per band.
        constexpr Color k_backUpper{ 0xFF78C8FF };
        constexpr Color k_backLower{ 0xFF67C0D1 };
        constexpr Color k_frontUpper{ 0xFF57B6A4 };
        constexpr Color k_frontMiddle{ 0xFF49AC74 };
        constexpr Color k_frontLower{ 0xFF3CA03C };

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

        // Where the cow's origin stands and how long its unit is, as fractions of the icon's side.
        constexpr FloatPoint k_cowOrigin{ 0.4895f, 0.5021f };
        constexpr float k_cowScale{ 0.9355f };

        constexpr Color k_keylineColor{ 0xFF3A3A44 };
        constexpr Color k_edgeColor{ 0xFFF4F5F8 };
        constexpr Color k_faceColor{ 0xFFFFFFFF };
        constexpr Color k_hornColor{ 0xFFF0E6C8 };
        constexpr Color k_patchColor{ 0xFF1E1E1E };
        constexpr Color k_snoutColor{ 0xFFFFC0CB };
        constexpr Color k_nostrilColor{ 0xFF3C1E1E };
        constexpr Color k_grassColor{ 0xFF50DC50 };

        constexpr float k_edgeWidth{ 0.0242f }; // the light line round the cow
        constexpr float k_nostrilRadius{ 1.0f / 32.0f };
        constexpr float k_snoutY{ 0.22f };
        constexpr float k_snoutWidth{ 0.22f };
        constexpr float k_snoutHeight{ 0.15f };
        // How far a quarter circle's handles reach, as a share of its radius.
        constexpr float k_arcHandle{ 0.5523f };

        // The cow's silhouette, grown 0.0242 all round: filled dark, it rings the parts.
        constexpr FloatPoint k_frameStart{ 0.2300f, 0.1720f };
        constexpr std::array<Cubic, 60> k_frame{
            Cubic{ { 0.2386f, 0.1437f }, { 0.2429f, 0.1143f }, { 0.2439f, 0.0848f } },
            Cubic{ { 0.2888f, 0.0950f }, { 0.3337f, 0.1052f }, { 0.3785f, 0.1154f } },
            Cubic{ { 0.3871f, 0.1173f }, { 0.4126f, 0.1241f }, { 0.4196f, 0.1242f } },
            Cubic{ { 0.4239f, 0.1243f }, { 0.4282f, 0.1232f }, { 0.4320f, 0.1210f } },
            Cubic{ { 0.4367f, 0.1184f }, { 0.4403f, 0.1142f }, { 0.4424f, 0.1092f } },
            Cubic{ { 0.4456f, 0.1014f }, { 0.4465f, 0.0789f }, { 0.4469f, 0.0696f } },
            Cubic{ { 0.4493f, 0.0214f }, { 0.4442f, -0.0350f }, { 0.4079f, -0.0707f } },
            Cubic{ { 0.3566f, -0.1213f }, { 0.2731f, -0.1022f }, { 0.2130f, -0.0808f } },
            Cubic{ { 0.2210f, -0.1495f }, { 0.2290f, -0.2182f }, { 0.2370f, -0.2869f } },
            Cubic{ { 0.2389f, -0.3035f }, { 0.2409f, -0.3200f }, { 0.2428f, -0.3366f } },
            Cubic{ { 0.2439f, -0.3460f }, { 0.2457f, -0.3552f }, { 0.2400f, -0.3636f } },
            Cubic{ { 0.2370f, -0.3681f }, { 0.2325f, -0.3714f }, { 0.2274f, -0.3730f } },
            Cubic{ { 0.2202f, -0.3753f }, { 0.1932f, -0.3742f }, { 0.1842f, -0.3742f } },
            Cubic{ { 0.1883f, -0.3825f }, { 0.1925f, -0.3908f }, { 0.1966f, -0.3992f } },
            Cubic{ { 0.1987f, -0.4033f }, { 0.2012f, -0.4074f }, { 0.2027f, -0.4117f } },
            Cubic{ { 0.2035f, -0.4139f }, { 0.2040f, -0.4162f }, { 0.2042f, -0.4186f } },
            Cubic{ { 0.2051f, -0.4341f }, { 0.1907f, -0.4467f }, { 0.1755f, -0.4438f } },
            Cubic{ { 0.1706f, -0.4428f }, { 0.1658f, -0.4410f }, { 0.1611f, -0.4395f } },
            Cubic{ { 0.1509f, -0.4364f }, { 0.1407f, -0.4332f }, { 0.1305f, -0.4301f } },
            Cubic{ { 0.1012f, -0.4211f }, { 0.0717f, -0.4126f }, { 0.0426f, -0.4030f } },
            Cubic{ { 0.0306f, -0.3991f }, { 0.0236f, -0.3865f }, { 0.0265f, -0.3742f } },
            Cubic{ { 0.0088f, -0.3742f }, { -0.0088f, -0.3742f }, { -0.0265f, -0.3742f } },
            Cubic{ { -0.0236f, -0.3864f }, { -0.0305f, -0.3990f }, { -0.0423f, -0.4030f } },
            Cubic{ { -0.0518f, -0.4061f }, { -0.0615f, -0.4088f }, { -0.0710f, -0.4118f } },
            Cubic{ { -0.0898f, -0.4176f }, { -0.1086f, -0.4234f }, { -0.1274f, -0.4291f } },
            Cubic{ { -0.1383f, -0.4325f }, { -0.1491f, -0.4358f }, { -0.1599f, -0.4391f } },
            Cubic{ { -0.1650f, -0.4407f }, { -0.1701f, -0.4427f }, { -0.1752f, -0.4437f } },
            Cubic{ { -0.1905f, -0.4468f }, { -0.2049f, -0.4343f }, { -0.2042f, -0.4188f } },
            Cubic{ { -0.2041f, -0.4165f }, { -0.2036f, -0.4142f }, { -0.2028f, -0.4120f } },
            Cubic{ { -0.2013f, -0.4076f }, { -0.1988f, -0.4035f }, { -0.1967f, -0.3994f } },
            Cubic{ { -0.1925f, -0.3910f }, { -0.1883f, -0.3826f }, { -0.1842f, -0.3742f } },
            Cubic{ { -0.1932f, -0.3742f }, { -0.2200f, -0.3753f }, { -0.2272f, -0.3731f } },
            Cubic{ { -0.2323f, -0.3715f }, { -0.2368f, -0.3682f }, { -0.2399f, -0.3638f } },
            Cubic{ { -0.2457f, -0.3555f }, { -0.2439f, -0.3462f }, { -0.2428f, -0.3368f } },
            Cubic{ { -0.2409f, -0.3202f }, { -0.2390f, -0.3037f }, { -0.2370f, -0.2871f } },
            Cubic{ { -0.2290f, -0.2183f }, { -0.2210f, -0.1496f }, { -0.2130f, -0.0808f } },
            Cubic{ { -0.2730f, -0.1022f }, { -0.3565f, -0.1212f }, { -0.4078f, -0.0709f } },
            Cubic{ { -0.4441f, -0.0352f }, { -0.4493f, 0.0211f }, { -0.4469f, 0.0693f } },
            Cubic{ { -0.4465f, 0.0787f }, { -0.4456f, 0.1011f }, { -0.4425f, 0.1090f } },
            Cubic{ { -0.4405f, 0.1140f }, { -0.4368f, 0.1182f }, { -0.4322f, 0.1209f } },
            Cubic{ { -0.4285f, 0.1231f }, { -0.4241f, 0.1242f }, { -0.4198f, 0.1242f } },
            Cubic{ { -0.4127f, 0.1241f }, { -0.3767f, 0.1150f }, { -0.3670f, 0.1128f } },
            Cubic{ { -0.3097f, 0.0997f }, { -0.2523f, 0.0867f }, { -0.1949f, 0.0737f } },
            Cubic{ { -0.1931f, 0.0891f }, { -0.1913f, 0.1046f }, { -0.1895f, 0.1200f } },
            Cubic{ { -0.2117f, 0.1361f }, { -0.2301f, 0.1578f }, { -0.2386f, 0.1841f } },
            Cubic{ { -0.2412f, 0.1920f }, { -0.2429f, 0.2002f }, { -0.2437f, 0.2085f } },
            Cubic{ { -0.2449f, 0.2214f }, { -0.2442f, 0.2346f }, { -0.2442f, 0.2475f } },
            Cubic{ { -0.2442f, 0.2702f }, { -0.2442f, 0.2928f }, { -0.2442f, 0.3155f } },
            Cubic{ { -0.2442f, 0.3262f }, { -0.2453f, 0.3703f }, { -0.2430f, 0.3774f } },
            Cubic{ { -0.2411f, 0.3835f }, { -0.2367f, 0.3886f }, { -0.2311f, 0.3915f } },
            Cubic{ { -0.2227f, 0.3958f }, { -0.2098f, 0.3942f }, { -0.2005f, 0.3941f } },
            Cubic{ { -0.1782f, 0.3937f }, { -0.1558f, 0.3928f }, { -0.1336f, 0.3911f } },
            Cubic{ { -0.0353f, 0.3835f }, { 0.0720f, 0.3613f }, { 0.1490f, 0.2957f } },
            Cubic{ { 0.2173f, 0.2890f }, { 0.2748f, 0.3068f }, { 0.3202f, 0.3599f } },
            Cubic{ { 0.3317f, 0.3733f }, { 0.3416f, 0.3880f }, { 0.3504f, 0.4033f } },
            Cubic{ { 0.3559f, 0.4129f }, { 0.3618f, 0.4267f }, { 0.3674f, 0.4351f } },
            Cubic{ { 0.3809f, 0.4554f }, { 0.4056f, 0.4663f }, { 0.4298f, 0.4625f } },
            Cubic{ { 0.4644f, 0.4571f }, { 0.4882f, 0.4241f }, { 0.4824f, 0.3896f } },
            Cubic{ { 0.4796f, 0.3726f }, { 0.4643f, 0.3471f }, { 0.4552f, 0.3320f } },
            Cubic{ { 0.4044f, 0.2473f }, { 0.3290f, 0.1875f }, { 0.2300f, 0.1720f } }
        };

        // The grass as the outline of a round-ended stroke, which both backends draw alike.
        constexpr FloatPoint k_grassStart{ -0.0391f, 0.2702f };
        constexpr std::array<Cubic, 15> k_grass{
            Cubic{ { -0.0390f, 0.2933f }, { -0.0181f, 0.3116f }, { 0.0048f, 0.3088f } },
            Cubic{ { 0.0138f, 0.3077f }, { 0.0312f, 0.2998f }, { 0.0407f, 0.2964f } },
            Cubic{ { 0.0511f, 0.2928f }, { 0.0616f, 0.2894f }, { 0.0721f, 0.2863f } },
            Cubic{ { 0.1264f, 0.2707f }, { 0.1857f, 0.2628f }, { 0.2408f, 0.2790f } },
            Cubic{ { 0.2900f, 0.2935f }, { 0.3293f, 0.3275f }, { 0.3578f, 0.3695f } },
            Cubic{ { 0.3651f, 0.3802f }, { 0.3717f, 0.3915f }, { 0.3778f, 0.4030f } },
            Cubic{ { 0.3810f, 0.4091f }, { 0.3836f, 0.4158f }, { 0.3874f, 0.4216f } },
            Cubic{ { 0.3897f, 0.4249f }, { 0.3924f, 0.4279f }, { 0.3955f, 0.4304f } },
            Cubic{ { 0.4188f, 0.4492f }, { 0.4542f, 0.4354f }, { 0.4586f, 0.4058f } },
            Cubic{ { 0.4606f, 0.3925f }, { 0.4546f, 0.3814f }, { 0.4487f, 0.3699f } },
            Cubic{ { 0.4379f, 0.3492f }, { 0.4257f, 0.3292f }, { 0.4117f, 0.3106f } },
            Cubic{ { 0.3340f, 0.2072f }, { 0.2238f, 0.1761f }, { 0.0996f, 0.1995f } },
            Cubic{ { 0.0646f, 0.2061f }, { 0.0303f, 0.2165f }, { -0.0030f, 0.2292f } },
            Cubic{ { -0.0086f, 0.2314f }, { -0.0144f, 0.2332f }, { -0.0196f, 0.2362f } },
            Cubic{ { -0.0316f, 0.2432f }, { -0.0391f, 0.2563f }, { -0.0391f, 0.2702f } }
        };

        [[nodiscard]] FloatPoint mix(FloatPoint from, FloatPoint to, float amount);
        [[nodiscard]] FloatPoint towards(FloatPoint from, FloatPoint to, float distance);
        [[nodiscard]] Corners cornersOf(const Leaf&);
        [[nodiscard]] Corners bandCorners(const Corners& leaf, const Band&);
        [[nodiscard]] Matrix3x2 fitTo(const FloatRect& bounds);
        [[nodiscard]] Matrix3x2 cowTransform(const FloatRect& bounds);

        void addRoundedQuad(PixelPath& path, const Corners&, const CornerRadii& radii);
        void addOutline(PixelPath& path, FloatPoint start, std::span<const Cubic> curves);
        void addCapsule(PixelPath& path, FloatPoint from, FloatPoint to, float radius);

        void fillBand(Canvas&, const Matrix3x2& transform, const Leaf&, const Band&);
        void strokeLeaf(Canvas&, const Matrix3x2& transform, const Leaf&);
        void paintLeaves(Canvas&, const Matrix3x2& transform);
        void paintCow(Canvas&, const Matrix3x2& transform);
    }
}

//-----------------------------------------------------------------------------

namespace ClaFi::Showcase::AppIcon
{
    void paint(Canvas& canvas, const FloatRect& bounds)
    {
        paintLeaves(canvas, fitTo(bounds));
        paintCow(canvas, cowTransform(bounds));
    }

    void paintIcon(PaintIconEvent& event)
    {
        paint(event.canvas(), event.iconRect());
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

        // The cow keeps its own proportions whatever the rect: one scale serves both axes.
        Matrix3x2 cowTransform(const FloatRect& bounds)
        {
            const float scale = std::min(bounds.width(), bounds.height()) * k_cowScale;
            const FloatPoint origin = bounds.topLeft();

            return {
                scale, 0.0f,
                0.0f, scale,
                origin.x + bounds.width() * k_cowOrigin.x,
                origin.y + bounds.height() * k_cowOrigin.y
            };
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

        void addOutline(PixelPath& path, const FloatPoint start, const std::span<const Cubic> curves)
        {
            path.moveTo(start);
            for (const Cubic& cubic : curves)
                path.cubicTo(cubic.control1, cubic.control2, cubic.end);

            path.close();
        }

        // The outline a round-ended stroke from one point to the other would cover.
        void addCapsule(PixelPath& path, const FloatPoint from, const FloatPoint to, const float radius)
        {
            const float length = std::hypot(to.x - from.x, to.y - from.y);
            const FloatPoint along = {
                (to.x - from.x) / length,
                (to.y - from.y) / length
            };
            const FloatPoint side = { -along.y, along.x };
            const FloatPoint reach = along * radius;
            const FloatPoint offset = side * radius;
            const FloatPoint handleAlong = along * (radius * k_arcHandle);
            const FloatPoint handleSide = side * (radius * k_arcHandle);

            path.moveTo(from + offset);
            path.lineTo(to + offset);
            path.cubicTo(to + offset + handleAlong, to + reach + handleSide, to + reach);
            path.cubicTo(to + reach - handleSide, to - offset + handleAlong, to - offset);
            path.lineTo(from - offset);
            path.cubicTo(from - offset - handleAlong, from - reach - handleSide, from - reach);
            path.cubicTo(from - reach + handleSide, from + offset - handleAlong, from + offset);
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

        // MillScene's cow, part for part, over a frame that holds it off the leaves.
        void paintCow(Canvas& canvas, const Matrix3x2& transform)
        {
            PixelPath path;

            // The edge is stroked first, so the keyline covers the half of it that falls inside.
            const float edgeStroke = k_edgeWidth * 2.0f;
            addOutline(path, k_frameStart, k_frame);
            canvas.drawPath(
                path,
                {
                    PathDrawLayer::stroke(k_edgeColor, edgeStroke),
                    PathDrawLayer::fill(k_keylineColor)
                },
                &transform);

            // Ears, which reach outside the face and are drawn under it.
            path.clear();
            path.moveTo({ -0.2f, -0.05f });
            path.quadTo({ -0.45f, -0.15f }, { -0.42f, 0.1f });
            path.lineTo({ -0.2f, 0.05f });
            path.close();
            path.moveTo({ 0.2f, -0.05f });
            path.quadTo({ 0.45f, -0.15f }, { 0.42f, 0.1f });
            path.lineTo({ 0.2f, 0.05f });
            path.close();
            canvas.drawPath(path, { PathDrawLayer::fill(k_faceColor) }, &transform);

            // Horns.
            path.clear();
            path.moveTo({ -0.12f, -0.3f });
            path.lineTo({ -0.18f, -0.42f });
            path.lineTo({ -0.05f, -0.38f });
            path.close();
            path.moveTo({ 0.12f, -0.3f });
            path.lineTo({ 0.18f, -0.42f });
            path.lineTo({ 0.05f, -0.38f });
            path.close();
            canvas.drawPath(path, { PathDrawLayer::fill(k_hornColor) }, &transform);

            // The tapered face.
            path.clear();
            path.moveTo({ -0.22f, -0.35f });
            path.lineTo({ 0.22f, -0.35f });
            path.lineTo({ 0.15f, 0.25f });
            path.lineTo({ -0.15f, 0.25f });
            path.close();
            canvas.drawPath(path, { PathDrawLayer::fill(k_faceColor) }, &transform);

            // The patch over one eye, which is what makes it read as a cow at 16 units.
            path.clear();
            path.moveTo({ -0.22f, -0.35f });
            path.quadTo({ -0.05f, -0.3f }, { 0.0f, -0.1f });
            path.lineTo({ -0.2f, 0.05f });
            path.close();
            canvas.drawPath(path, { PathDrawLayer::fill(k_patchColor) }, &transform);

            // The snout, and the grass being munched out of the side of it.
            const float snoutTop = k_snoutY - k_snoutHeight;
            const float snoutBottom = k_snoutY + k_snoutHeight;
            path.clear();
            path.moveTo({ -k_snoutWidth, k_snoutY });
            path.quadTo({ -k_snoutWidth, snoutTop }, { k_snoutWidth, snoutTop });
            path.quadTo({ k_snoutWidth, snoutBottom }, { -k_snoutWidth, snoutBottom });
            path.close();
            canvas.drawPath(path, { PathDrawLayer::fill(k_snoutColor) }, &transform);

            path.clear();
            for (const float x : { -0.07f, 0.07f })
                addCapsule(path, { x, k_snoutY + 0.02f }, { x, k_snoutY + 0.05f }, k_nostrilRadius);

            canvas.drawPath(path, { PathDrawLayer::fill(k_nostrilColor) }, &transform);

            path.clear();
            addOutline(path, k_grassStart, k_grass);
            canvas.drawPath(path, { PathDrawLayer::fill(k_grassColor) }, &transform);
        }
    }
}
