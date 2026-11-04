module ClaFi.Icons.RowIcons;

import ClaFi.Icons.FolderIcon;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::RowIcons
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        [[nodiscard]] Inks inksOf(const DrawIconEvent& event)
        {
            const Color accent = event.accentRgb(InkGrade::Strongest);
            return {
                event.textRgb(InkGrade::Strong),
                accent,
                accent.withOpacity(TintOpacity),
                event.spotRgb(InkGrade::Strongest),
                event.surfaceRgb()
            };
        }

        // The grid is scaled to the shorter side of the rect and stands at its top left.
        [[nodiscard]] Matrix3x2 gridTransform(const FloatRect& iconRect)
        {
            const float scale = std::min(iconRect.width(), iconRect.height()) / GridSize;
            return Matrix3x2::Translation(iconRect.topLeft()) * Matrix3x2::Scale(scale);
        }
    }

    // Drawing

    Drawing::Drawing(DrawIconEvent& event)
        :
        canvas{ event.canvas() },
        inks{ inksOf(event) },
        transform{ gridTransform(event.iconRect()) }
    {
    }

    void Drawing::stroke(const PixelPath& path, const Color color) const
    {
        canvas.drawPath(path, { PathDrawLayer::Stroke(color, StrokeWidth) }, &transform);
    }

    void Drawing::fill(const PixelPath& path, const Color color) const
    {
        canvas.drawPath(path, { PathDrawLayer::Fill(color) }, &transform);
    }

    // Shapes

    void addCircle(PixelPath& path, const FloatPoint center, const float radius)
    {
        const FloatPoint radii = { radius, radius };
        const FloatPoint top = { center.x, center.y - radius };
        const FloatPoint bottom = { center.x, center.y + radius };
        path.moveTo(top);
        path.arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise, bottom);
        path.arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise, top);
        path.close();
    }

    void addRect(PixelPath& path, const FloatRect& rect)
    {
        path.moveTo(rect.left, rect.top);
        path.lineTo(rect.right, rect.top);
        path.lineTo(rect.right, rect.bottom);
        path.lineTo(rect.left, rect.bottom);
        path.close();
    }

    void addLine(PixelPath& path, const FloatPoint from, const FloatPoint to)
    {
        path.moveTo(from);
        path.lineTo(to);
    }

    // Folder

    FloatRect folderBox()
    {
        return { 1.5f, 3.5f, 12.5f, 11.5f };
    }

    void drawFolder(const Drawing& drawing)
    {
        PixelPath folder;
        FolderIcon::addOutline(folder, folderBox());
        drawing.stroke(folder, drawing.inks.body);
    }

    // Kind word

    void writeKindWord(Text& text, const std::wstring_view kindWord)
    {
        text << FlexSpace{ KindGap };
        text << StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, kindWord } };
    }
}
