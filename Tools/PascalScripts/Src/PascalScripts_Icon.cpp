module ThisApp.AppIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp::AppIcon
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    namespace
    {
        using Corners = std::array<FloatPoint, 4>;
        using CornerRadii = std::array<float, 4>;
        using Triangle = std::array<FloatPoint, 3>;

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

        // From a fraction of the shape's height to its foot, so bands overlap and no seam shows.
        struct Band
        {
            float from{};
            Color color{};
        };

        struct Extent
        {
            float left{};
            float top{};
            float width{};
            float height{};
        };

        // A line of text on the sheet, from one column to another on one row.
        struct Bar
        {
            float left{};
            float right{};
            float top{};
        };

        // Where the Run arrow stands on the sheet: its left side's top, and that side's length.
        struct ArrowPlace
        {
            FloatPoint topLeft{};
            float side{};
        };

        constexpr float k_epsilon{ 0.0001f };

        constexpr Color k_outlineColor{ 0xFF9B9BA2 };
        constexpr Color k_edgeColor{ 0xFFF4F5F8 };
        constexpr Color k_keylineColor{ 0xFF3A3A44 };
        constexpr float k_outlineWidth{ 0.020f };   // the line round each leaf
        constexpr float k_edgeWidth{ 0.020f };      // the light line round the arrow
        constexpr float k_keylineWidth{ 0.020f };   // the dark line inside the arrow's light one

        // Greys on the line from the keyline colour to the edge colour, lightest at the top band.
        constexpr Color k_backUpper{ 0xFFF4F5F8 };
        constexpr Color k_backLower{ 0xFFE1E2E6 };
        constexpr Color k_frontUpper{ 0xFFECEDF0 };
        constexpr Color k_frontMiddle{ 0xFFD9D9DE };
        constexpr Color k_frontLower{ 0xFFC6C6CB };

        // The Themes sweep's green, lightest at the top band as the leaves are.
        constexpr Color k_arrowUpper{ 0xFF2CC968 };
        constexpr Color k_arrowMiddle{ 0xFF00B959 };
        constexpr Color k_arrowLower{ 0xFF00A94A };

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

        constexpr std::array<Band, 3> k_arrowBands{
            Band{ .from = 0.00f, .color = k_arrowUpper },
            Band{ .from = 0.34f, .color = k_arrowMiddle },
            Band{ .from = 0.67f, .color = k_arrowLower }
        };

        // WhatsClip's box, tail included, so the leaves land on the same pixels in every mark.
        constexpr Extent k_markBox{ .left = 0.033f, .top = 0.055f, .width = 0.936f, .height = 0.8834f };

        // The Run arrow's corners before they are rounded, an equilateral triangle pointing right.
        constexpr Triangle k_arrow{
            FloatPoint{ 0.2710f, 0.1550f },
            FloatPoint{ 0.8686f, 0.5000f },
            FloatPoint{ 0.2710f, 0.8450f }
        };

        constexpr float k_arrowRadius{ 0.069f };   // each corner's round, an arc of this radius
        // A 60-degree corner's round meets each of its edges this far from the corner.
        constexpr float k_arrowCut{ k_arrowRadius * 1.7320508f };
        // How far a cubic's controls lean into a 60-degree corner for the cubic to trace a circle.
        constexpr float k_arcPull{ 4.0f / 9.0f };

        // The script's sheet is drawn in a square of this many units and scaled onto its rect.
        constexpr float k_designSize{ 32.0f };
        constexpr float k_sheetLeft{ 6.0f };
        constexpr float k_sheetTop{ 2.0f };
        constexpr float k_sheetRight{ 26.0f };
        constexpr float k_sheetBottom{ 30.0f };
        constexpr float k_fold{ 6.0f };   // the side of the square the folded corner stands in
        constexpr float k_cornerRadius{ 1.5f };   // every rounded corner, the fold's included
        constexpr float k_foldShare{ gradeOf(InkGrade::Faint) * 0.5f };   // the fold's fill
        constexpr float k_barHeight{ 1.5f };
        // A begin and an end in the keyword ink, the body between them indented.
        constexpr std::array<Bar, 2> k_keywordBars{
            Bar{ 10.0f, 16.5f, 8.75f },
            Bar{ 10.0f, 15.0f, 20.75f }
        };
        constexpr std::array<Bar, 2> k_bodyBars{
            Bar{ 13.0f, 22.0f, 12.75f },
            Bar{ 13.0f, 22.0f, 16.75f }
        };
        constexpr ArrowPlace k_tileArrow{ .topLeft{ 18.0f, 20.5f }, .side = 7.5f };   // lower right
        // Alone on the sheet, its centroid on the page's middle below the fold.
        constexpr ArrowPlace k_smallArrow{ .topLeft{ 12.6f, 11.0f }, .side = 12.0f };
        constexpr float k_smallIconSize{ 32.0f };   // logical pixels; below it, no lines

        [[nodiscard]] FloatPoint mix(FloatPoint from, FloatPoint to, float amount);
        [[nodiscard]] FloatPoint towards(FloatPoint from, FloatPoint to, float distance);
        [[nodiscard]] Corners cornersOf(const Leaf&);
        [[nodiscard]] Corners bandCorners(const Corners& leaf, const Band&);
        [[nodiscard]] Matrix3x2 fitTo(const FloatRect& bounds);
        [[nodiscard]] float scaleFor(const FloatRect& bounds);
        [[nodiscard]] Matrix3x2 fitSheetTo(const FloatRect& bounds);
        [[nodiscard]] Matrix3x2 fitArrowTo(const Matrix3x2& sheet, const ArrowPlace&);
        [[nodiscard]] ControlPaintContext paperContext(const ControlPaintContext& host);

        void addRoundedQuad(PixelPath& path, const Corners&, const CornerRadii& radii);
        void addArrow(PixelPath& path);
        void addRoundedPolygon(PixelPath& path, std::span<const FloatPoint> corners,
            std::span<const float> radii);
        void addSheet(PixelPath& path);
        void addFold(PixelPath& path);
        void addBar(PixelPath& path, const Bar&);

        void fillBand(Canvas&, const Matrix3x2& transform, const Leaf&, const Band&);
        void strokeLeaf(Canvas&, const Matrix3x2& transform, const Leaf&);
        void paintLeaves(Canvas&, const Matrix3x2& transform);
        void fillArrowBand(Canvas&, const Matrix3x2& transform, const Band&);
        void paintArrow(Canvas&, const Matrix3x2& transform);
        void paintSheet(Canvas&, const Matrix3x2& transform, float lineWidth, Color paper,
            Color frame, Color fold);
        void paintBars(Canvas&, const Matrix3x2& transform, std::span<const Bar> bars, Color);
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
        paintArrow(canvas, transform);
    }

    void paintIcon(PaintIconEvent& event)
    {
        paint(event.canvas(), event.iconRect(), 1.0f - event.disabledAmount());
    }

    void paintScriptIcon(PaintIconEvent& event)
    {
        Canvas& canvas = event.canvas();
        const FloatRect& iconRect = event.iconRect();
        const Matrix3x2 transform = fitSheetTo(iconRect);
        // The form's own stroke weight, in design units because the transform scales it.
        const float lineWidth = event.scaledStrokeWidth(Thickness::Thin) / scaleFor(iconRect);
        const ControlPaintContext paper = paperContext(event.controlContext());
        paintSheet(canvas, transform, lineWidth,
            paper.surfaceRgb(),
            paper.textRgb(InkGrade::Subtle),
            paper.inkRgb(InkWell::textInk(k_foldShare)));

        const float iconSide = std::min(iconRect.width(), iconRect.height());
        if (iconSide / event.controlContext().scaleFactor() < k_smallIconSize)
        {
            paintArrow(canvas, fitArrowTo(transform, k_smallArrow));
            return;
        }

        // The keywords keep the hue CodeBox gives them, softened to sit with the body lines.
        const Color keywordInk = paper.inkRgb(InkWell::blueInk(InkGrade::Muted));
        paintBars(canvas, transform, k_keywordBars, keywordInk);
        paintBars(canvas, transform, k_bodyBars, paper.textRgb(InkGrade::Subtle));
        paintArrow(canvas, fitArrowTo(transform, k_tileArrow));
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

        float scaleFor(const FloatRect& bounds)
        {
            return std::min(bounds.width(), bounds.height()) / k_designSize;
        }

        // Scales the sheet's design square onto the rect, centred in it.
        Matrix3x2 fitSheetTo(const FloatRect& bounds)
        {
            const float scale = scaleFor(bounds);
            const FloatPoint origin = bounds.topLeft();
            const float left = origin.x + (bounds.width() - k_designSize * scale) * 0.5f;
            const float top = origin.y + (bounds.height() - k_designSize * scale) * 0.5f;
            return { scale, 0.0f, 0.0f, scale, left, top };
        }

        // The app mark's Run arrow laid onto the sheet's design square at the place given.
        Matrix3x2 fitArrowTo(const Matrix3x2& sheet, const ArrowPlace& place)
        {
            const float scale = place.side / (k_arrow[2].y - k_arrow[0].y);
            const float left = place.topLeft.x - k_arrow[0].x * scale;
            const float top = place.topLeft.y - k_arrow[0].y * scale;
            return {
                sheet.a * scale, 0.0f,
                0.0f, sheet.d * scale,
                sheet.e + left * sheet.a,
                sheet.f + top * sheet.d
            };
        }

        // The sheet is white whatever the theme, so its inks are the theme's light side on white.
        ControlPaintContext paperContext(const ControlPaintContext& host)
        {
            ControlPaintContext result = host;
            result.lightness = k_lightLightness;
            result.surfaceHsl = {
                host.textHsl.hue,
                0.0f,
                luminosityOf(0.0f, k_lightLightness)
            };
            result.textHsl = {
                host.textHsl.hue,
                host.textHsl.saturation,
                luminosityOf(1.0f, k_lightLightness)
            };
            result.surface = result.surfaceHsl.toColor();
            return result;
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

        void addArrow(PixelPath& path)
        {
            for (std::size_t index = 0; index < k_arrow.size(); ++index)
            {
                const FloatPoint previous = k_arrow[(index + k_arrow.size() - 1) % k_arrow.size()];
                const FloatPoint corner = k_arrow[index];
                const FloatPoint next = k_arrow[(index + 1) % k_arrow.size()];

                const FloatPoint entry = towards(corner, previous, k_arrowCut);
                const FloatPoint exit = towards(corner, next, k_arrowCut);

                if (index == 0)
                    path.moveTo(entry);
                else
                    path.lineTo(entry);

                path.cubicTo(mix(entry, corner, k_arcPull), mix(exit, corner, k_arcPull), exit);
            }

            path.close();
        }

        // Each corner turned on a circular arc of its radius; a radius of zero leaves it square.
        void addRoundedPolygon(PixelPath& path, const std::span<const FloatPoint> corners,
            const std::span<const float> radii)
        {
            const std::size_t count = corners.size();
            for (std::size_t index = 0; index < count; ++index)
            {
                const FloatPoint previous = corners[(index + count - 1) % count];
                const FloatPoint corner = corners[index];
                const FloatPoint next = corners[(index + 1) % count];
                const float previousLength = std::hypot(previous.x - corner.x,
                    previous.y - corner.y);
                const float nextLength = std::hypot(next.x - corner.x, next.y - corner.y);
                const bool square = radii[index] < k_epsilon
                    || previousLength < k_epsilon
                    || nextLength < k_epsilon;
                if (square)
                {
                    if (index == 0)
                        path.moveTo(corner);
                    else
                        path.lineTo(corner);
                    continue;
                }

                const FloatPoint toPrevious = {
                    (previous.x - corner.x) / previousLength,
                    (previous.y - corner.y) / previousLength
                };
                const FloatPoint toNext = {
                    (next.x - corner.x) / nextLength,
                    (next.y - corner.y) / nextLength
                };
                const float cosine = std::clamp(toPrevious.x * toNext.x + toPrevious.y * toNext.y,
                    -1.0f, 1.0f);
                const float angle = std::acos(cosine);
                const float halfTangent = std::tan(angle * 0.5f);
                // Where the arc meets each side, never past the side's middle.
                const float room = std::min(previousLength, nextLength) * 0.5f;
                const float cut = std::min(radii[index] / halfTangent, room);
                const float radius = cut * halfTangent;
                // How far each control leans back along its side for one cubic to trace the arc.
                const float turn = std::numbers::pi_v<float> - angle;
                const float pull = 4.0f / 3.0f * std::tan(turn * 0.25f) * radius;

                const FloatPoint entry = {
                    corner.x + toPrevious.x * cut,
                    corner.y + toPrevious.y * cut
                };
                const FloatPoint exit = {
                    corner.x + toNext.x * cut,
                    corner.y + toNext.y * cut
                };
                const FloatPoint entryControl = {
                    entry.x - toPrevious.x * pull,
                    entry.y - toPrevious.y * pull
                };
                const FloatPoint exitControl = {
                    exit.x - toNext.x * pull,
                    exit.y - toNext.y * pull
                };
                if (index == 0)
                    path.moveTo(entry);
                else
                    path.lineTo(entry);
                path.cubicTo(entryControl, exitControl, exit);
            }

            path.close();
        }

        void addSheet(PixelPath& path)
        {
            constexpr std::array<FloatPoint, 5> corners = {
                FloatPoint{ k_sheetLeft, k_sheetTop },
                FloatPoint{ k_sheetRight - k_fold, k_sheetTop },
                FloatPoint{ k_sheetRight, k_sheetTop + k_fold },
                FloatPoint{ k_sheetRight, k_sheetBottom },
                FloatPoint{ k_sheetLeft, k_sheetBottom }
            };
            constexpr std::array<float, 5> radii = {
                k_cornerRadius,
                k_cornerRadius,
                k_cornerRadius,
                k_cornerRadius,
                k_cornerRadius
            };
            addRoundedPolygon(path, corners, radii);
        }

        void addFold(PixelPath& path)
        {
            constexpr std::array<FloatPoint, 3> corners = {
                FloatPoint{ k_sheetRight - k_fold, k_sheetTop },
                FloatPoint{ k_sheetRight - k_fold, k_sheetTop + k_fold },
                FloatPoint{ k_sheetRight, k_sheetTop + k_fold }
            };
            constexpr std::array<float, 3> radii = { 0.0f, k_cornerRadius, 0.0f };
            addRoundedPolygon(path, corners, radii);
        }

        void addBar(PixelPath& path, const Bar& bar)
        {
            path.moveTo(bar.left, bar.top);
            path.lineTo(bar.right, bar.top);
            path.lineTo(bar.right, bar.top + k_barHeight);
            path.lineTo(bar.left, bar.top + k_barHeight);
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

        // The band runs to the mark box on three sides, so the arrow's clip alone draws its edge.
        void fillArrowBand(Canvas& canvas, const Matrix3x2& transform, const Band& band)
        {
            const float top = k_arrow[0].y + (k_arrow[2].y - k_arrow[0].y) * band.from;
            const float left = k_markBox.left;
            const float right = k_markBox.left + k_markBox.width;
            const float bottom = k_markBox.top + k_markBox.height;

            PixelPath path;
            path.moveTo(left, top);
            path.lineTo(right, top);
            path.lineTo(right, bottom);
            path.lineTo(left, bottom);
            path.close();
            canvas.drawPath(path, { PathDrawLayer::fill(band.color) }, &transform);
        }

        // The frame is stroked first, so the bands cover the half of each line that falls inside.
        void paintArrow(Canvas& canvas, const Matrix3x2& transform)
        {
            PixelPath arrow;
            addArrow(arrow);

            const float edgeStroke = (k_keylineWidth + k_edgeWidth) * 2.0f;
            const float keylineStroke = k_keylineWidth * 2.0f;
            canvas.drawPath(arrow, { PathDrawLayer::stroke(k_edgeColor, edgeStroke) }, &transform);
            canvas.drawPath(arrow, { PathDrawLayer::stroke(k_keylineColor, keylineStroke) }, &transform);

            canvas.pushClip(arrow, &transform);
            for (const Band& band : k_arrowBands)
                fillArrowBand(canvas, transform, band);

            canvas.popClip();
        }

        void paintSheet(Canvas& canvas, const Matrix3x2& transform, const float lineWidth,
            const Color paper, const Color frame, const Color fold)
        {
            PixelPath outline;
            addSheet(outline);
            canvas.fillPath(outline, paper, &transform);

            // Held inside the page, the fold's tips stop at the softened junctions.
            PixelPath flap;
            addFold(flap);
            canvas.pushClip(outline, &transform);
            canvas.drawPath(flap, {
                PathDrawLayer::fill(fold),
                PathDrawLayer::stroke(frame, lineWidth)
            }, &transform);
            canvas.popClip();

            canvas.drawPath(outline, { PathDrawLayer::stroke(frame, lineWidth) }, &transform);
        }

        void paintBars(Canvas& canvas, const Matrix3x2& transform, const std::span<const Bar> bars,
            const Color color)
        {
            PixelPath path;
            for (const Bar& bar : bars)
                addBar(path, bar);

            canvas.fillPath(path, color, &transform);
        }
    }
}
