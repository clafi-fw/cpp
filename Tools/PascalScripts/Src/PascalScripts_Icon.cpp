module ThisApp.AppIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp::AppIcon
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // A line of text on the sheet, from one column to another on one row.
        struct Bar
        {
            float left;
            float right;
            float top;
        };

        // The mark is drawn in a square of this many units and scaled onto the rect it is given.
        constexpr float k_designSize{ 32.0f };
        constexpr float k_sheetLeft{ 6.0f };
        constexpr float k_sheetTop{ 2.0f };
        constexpr float k_sheetRight{ 26.0f };
        constexpr float k_sheetBottom{ 30.0f };
        constexpr float k_fold{ 6.0f };   // the side of the square the folded corner stands in
        constexpr float k_barHeight{ 2.5f };
        constexpr float k_barLeft{ 10.0f };
        // The first bar is the keyword line, and it is the short one.
        constexpr std::array<Bar, 3> k_bars{
            Bar{ k_barLeft, 16.0f, 12.0f },
            Bar{ k_barLeft, 22.0f, 17.0f },
            Bar{ k_barLeft, 19.0f, 22.0f }
        };
        constexpr float k_outlineWidth{ 1.5f };

        constexpr Color k_paper{ 0xFFF4F6FA };
        constexpr Color k_paperEdge{ 0xFF56637D };
        constexpr Color k_foldShade{ 0xFFD3DAE8 };
        constexpr Color k_keywordBar{ 0xFF3D7BD9 };
        constexpr Color k_textBar{ 0xFF9AA7BF };

        [[nodiscard]] float scaleFor(const FloatRect& bounds)
        {
            return std::min(bounds.width(), bounds.height()) / k_designSize;
        }

        // Scales the design square onto the rect, centred in it.
        [[nodiscard]] Matrix3x2 fitTo(const FloatRect& bounds)
        {
            const float scale = scaleFor(bounds);
            const FloatPoint origin = bounds.topLeft();
            const float left = origin.x + (bounds.width() - k_designSize * scale) * 0.5f;
            const float top = origin.y + (bounds.height() - k_designSize * scale) * 0.5f;
            return { scale, 0.0f, 0.0f, scale, left, top };
        }

        void addSheet(PixelPath& path)
        {
            path.moveTo(k_sheetLeft, k_sheetTop);
            path.lineTo(k_sheetRight - k_fold, k_sheetTop);
            path.lineTo(k_sheetRight, k_sheetTop + k_fold);
            path.lineTo(k_sheetRight, k_sheetBottom);
            path.lineTo(k_sheetLeft, k_sheetBottom);
            path.close();
        }

        void addFold(PixelPath& path)
        {
            path.moveTo(k_sheetRight - k_fold, k_sheetTop);
            path.lineTo(k_sheetRight - k_fold, k_sheetTop + k_fold);
            path.lineTo(k_sheetRight, k_sheetTop + k_fold);
            path.close();
        }

        void addBar(PixelPath& path, const Bar& bar)
        {
            path.moveTo(bar.left, bar.top);
            path.lineTo(bar.right, bar.top);
            path.lineTo(bar.right, bar.top + k_barHeight);
            path.lineTo(bar.left, bar.top + k_barHeight);
            path.close();
        }

        void paintMark(Canvas& canvas, const Matrix3x2& transform, const float outlineWidth,
            const Color paper, const Color edge, const Color fold, const Color keywordBar,
            const Color textBar)
        {
            PixelPath path;
            addSheet(path);
            canvas.drawPath(path, {
                PathDrawLayer::fill(paper),
                PathDrawLayer::stroke(edge, outlineWidth)
            }, &transform);

            path.clear();
            addFold(path);
            canvas.drawPath(path, {
                PathDrawLayer::fill(fold),
                PathDrawLayer::stroke(edge, outlineWidth)
            }, &transform);

            path.clear();
            addBar(path, k_bars[0]);
            canvas.drawPath(path, { PathDrawLayer::fill(keywordBar) }, &transform);

            path.clear();
            addBar(path, k_bars[1]);
            addBar(path, k_bars[2]);
            canvas.drawPath(path, { PathDrawLayer::fill(textBar) }, &transform);
        }
    }

    void paint(Canvas& canvas, const FloatRect& bounds, const Opacity opacity)
    {
        // One layer for the whole mark: its parts overlap, and faded one by one they would show.
        const ScopedCanvasOpacity layer{ canvas, opacity };
        paintMark(canvas, fitTo(bounds), k_outlineWidth,
            k_paper, k_paperEdge, k_foldShade, k_keywordBar, k_textBar);
    }

    void paintIcon(PaintIconEvent& event)
    {
        paint(event.canvas(), event.iconRect(), 1.0f - event.disabledAmount());
    }

    void paintScriptIcon(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        // The outline is a stroke of the form's own weight, so it reads the same at a crumb's
        // size and a tile's; stated in design units because the transform scales it.
        const float outlineWidth = event.scaledStrokeWidth(Thickness::Thin) / scaleFor(iconRect);
        paintMark(event.canvas(), fitTo(iconRect), outlineWidth,
            event.surfaceRgb(),
            event.textRgb(InkGrade::Strong),
            event.textRgb(InkGrade::Subtle),
            event.accentRgb(InkGrade::Strong),
            event.textRgb(InkGrade::Muted));
    }
}
