module ClaFi.Showcase.TextEngine.Icon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Showcase::TextEngine::AppIcon
{
    using namespace ::ClaFi;
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

        // One curve of the letter's outline, running on from where the one before it ended.
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

        constexpr Color k_outlineColor{ 0xFF9B9BA2 };
        constexpr Color k_edgeColor{ 0xFFF4F5F8 };
        constexpr Color k_inkColor{ 0xFF1D1D26 };
        constexpr float k_outlineWidth{ 0.020f };   // the line round each leaf
        constexpr float k_edgeWidth{ 0.020f };      // the light line round the letter

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

        // Eagle Lake's T (SIL Open Font License), grown 0.021 all round and fitted to the mark.
        constexpr FloatPoint k_letterStart{ 0.5324f, 0.7242f };
        constexpr std::array<Cubic, 39> k_letter{
            Cubic{ { 0.5371f, 0.5731f }, { 0.5419f, 0.4219f }, { 0.5466f, 0.2707f } },
            Cubic{ { 0.5695f, 0.2681f }, { 0.5924f, 0.2657f }, { 0.6153f, 0.2636f } },
            Cubic{ { 0.6300f, 0.2623f }, { 0.6446f, 0.2607f }, { 0.6593f, 0.2609f } },
            Cubic{ { 0.6600f, 0.2642f }, { 0.6603f, 0.2673f }, { 0.6620f, 0.2703f } },
            Cubic{ { 0.6639f, 0.2738f }, { 0.6668f, 0.2768f }, { 0.6704f, 0.2787f } },
            Cubic{ { 0.6777f, 0.2827f }, { 0.6841f, 0.2814f }, { 0.6915f, 0.2786f } },
            Cubic{ { 0.6950f, 0.2773f }, { 0.6984f, 0.2759f }, { 0.7017f, 0.2744f } },
            Cubic{ { 0.7292f, 0.2626f }, { 0.7726f, 0.2393f }, { 0.7934f, 0.2182f } },
            Cubic{ { 0.8030f, 0.2085f }, { 0.8106f, 0.1953f }, { 0.8052f, 0.1814f } },
            Cubic{ { 0.8034f, 0.1768f }, { 0.8002f, 0.1727f }, { 0.7961f, 0.1697f } },
            Cubic{ { 0.7840f, 0.1607f }, { 0.7662f, 0.1610f }, { 0.7517f, 0.1608f } },
            Cubic{ { 0.7194f, 0.1604f }, { 0.6871f, 0.1621f }, { 0.6549f, 0.1648f } },
            Cubic{ { 0.6326f, 0.1667f }, { 0.6104f, 0.1688f }, { 0.5881f, 0.1711f } },
            Cubic{ { 0.5364f, 0.1763f }, { 0.4846f, 0.1819f }, { 0.4326f, 0.1834f } },
            Cubic{ { 0.4044f, 0.1842f }, { 0.3706f, 0.1846f }, { 0.3427f, 0.1811f } },
            Cubic{ { 0.3331f, 0.1799f }, { 0.3236f, 0.1781f }, { 0.3141f, 0.1758f } },
            Cubic{ { 0.3154f, 0.1721f }, { 0.3168f, 0.1694f }, { 0.3169f, 0.1654f } },
            Cubic{ { 0.3171f, 0.1524f }, { 0.3051f, 0.1421f }, { 0.2923f, 0.1443f } },
            Cubic{ { 0.2861f, 0.1454f }, { 0.2658f, 0.1569f }, { 0.2592f, 0.1606f } },
            Cubic{ { 0.2343f, 0.1744f }, { 0.2048f, 0.1926f }, { 0.1868f, 0.2149f } },
            Cubic{ { 0.1768f, 0.2274f }, { 0.1694f, 0.2432f }, { 0.1748f, 0.2592f } },
            Cubic{ { 0.1823f, 0.2814f }, { 0.2129f, 0.2884f }, { 0.2334f, 0.2910f } },
            Cubic{ { 0.2542f, 0.2937f }, { 0.2753f, 0.2939f }, { 0.2962f, 0.2937f } },
            Cubic{ { 0.3333f, 0.2932f }, { 0.3702f, 0.2906f }, { 0.4070f, 0.2869f } },
            Cubic{ { 0.4046f, 0.3789f }, { 0.4020f, 0.4709f }, { 0.3996f, 0.5629f } },
            Cubic{ { 0.3985f, 0.6032f }, { 0.3973f, 0.6436f }, { 0.3966f, 0.6839f } },
            Cubic{ { 0.3961f, 0.7086f }, { 0.3953f, 0.7332f }, { 0.3949f, 0.7579f } },
            Cubic{ { 0.3946f, 0.7761f }, { 0.3935f, 0.7920f }, { 0.4031f, 0.8084f } },
            Cubic{ { 0.4066f, 0.8145f }, { 0.4113f, 0.8198f }, { 0.4169f, 0.8240f } },
            Cubic{ { 0.4342f, 0.8371f }, { 0.4567f, 0.8371f }, { 0.4774f, 0.8349f } },
            Cubic{ { 0.5222f, 0.8299f }, { 0.5678f, 0.8074f }, { 0.6045f, 0.7820f } },
            Cubic{ { 0.6144f, 0.7752f }, { 0.6241f, 0.7681f }, { 0.6335f, 0.7607f } },
            Cubic{ { 0.6377f, 0.7574f }, { 0.6440f, 0.7531f }, { 0.6470f, 0.7489f } },
            Cubic{ { 0.6538f, 0.7396f }, { 0.6517f, 0.7263f }, { 0.6424f, 0.7195f } },
            Cubic{ { 0.6375f, 0.7160f }, { 0.6312f, 0.7147f }, { 0.6253f, 0.7161f } },
            Cubic{ { 0.6224f, 0.7168f }, { 0.6196f, 0.7179f }, { 0.6167f, 0.7187f } },
            Cubic{ { 0.6122f, 0.7200f }, { 0.6077f, 0.7212f }, { 0.6031f, 0.7222f } },
            Cubic{ { 0.5847f, 0.7264f }, { 0.5645f, 0.7292f }, { 0.5456f, 0.7269f } },
            Cubic{ { 0.5411f, 0.7263f }, { 0.5368f, 0.7253f }, { 0.5324f, 0.7242f } }
        };

        [[nodiscard]] FloatPoint mix(FloatPoint from, FloatPoint to, float amount);
        [[nodiscard]] FloatPoint towards(FloatPoint from, FloatPoint to, float distance);
        [[nodiscard]] Corners cornersOf(const Leaf&);
        [[nodiscard]] Corners bandCorners(const Corners& leaf, const Band&);
        [[nodiscard]] Matrix3x2 fitTo(const FloatRect& bounds);

        void addRoundedQuad(PixelPath& path, const Corners&, const CornerRadii& radii);
        void addLetter(PixelPath& path);

        void fillBand(Canvas&, const Matrix3x2& transform, const Leaf&, const Band&);
        void strokeLeaf(Canvas&, const Matrix3x2& transform, const Leaf&);
        void paintLeaves(Canvas&, const Matrix3x2& transform);
        void paintLetter(Canvas&, const Matrix3x2& transform);
    }
}

//-----------------------------------------------------------------------------

namespace ClaFi::Showcase::TextEngine::AppIcon
{
    void paint(Canvas& canvas, const FloatRect& bounds, const Opacity opacity)
    {
        // One layer for the whole mark: its parts overlap, and faded one by one they would show.
        const ScopedCanvasOpacity layer{ canvas, opacity };
        const Matrix3x2 transform = fitTo(bounds);

        paintLeaves(canvas, transform);
        paintLetter(canvas, transform);
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

        void addLetter(PixelPath& path)
        {
            path.moveTo(k_letterStart);
            for (const Cubic& cubic : k_letter)
                path.cubicTo(cubic.control1, cubic.control2, cubic.end);

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

        // The edge is stroked first, so the ink covers the half of it that falls inside.
        void paintLetter(Canvas& canvas, const Matrix3x2& transform)
        {
            PixelPath letter;
            addLetter(letter);

            const float edgeStroke = k_edgeWidth * 2.0f;
            canvas.drawPath(letter,
                { PathDrawLayer::stroke(k_edgeColor, edgeStroke), PathDrawLayer::fill(k_inkColor) },
                &transform);
        }
    }
}
