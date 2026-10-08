module SeeDocs_App.Studio.Icons;

import SeeDocs_App.Database;
import SeeDocs_App.Surface;

import ClaFi.Icons.FolderIcon;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // Every icon is drawn on this grid and scaled to the rect it is given, stroke and all.
        constexpr float k_grid = 14.0f;
        constexpr float k_rowIconSize = 14.0f;
        constexpr float k_stroke = 1.1f;
        constexpr float k_softFill = 0.35f; // a face tinted rather than filled
        constexpr float k_kindGap = 12.0f;  // the least room before a row's kind word

        // Strong ink holds the body, the accent marks the part that names the kind.
        struct Inks
        {
            Color body{};
            Color accent{};
            Color accentSoft{};
        };

        // How one icon is drawn: the canvas, its inks and the grid's place in the control.
        struct Drawing
        {
            Canvas& canvas;
            Inks inks{};
            Matrix3x2 transform{};

            void stroke(const PixelPath& path, Color color) const
            {
                canvas.drawPath(path, { PathDrawLayer::stroke(color, k_stroke) }, &transform);
            }

            void fill(const PixelPath& path, Color color) const
            {
                canvas.drawPath(path, { PathDrawLayer::fill(color) }, &transform);
            }
        };

        [[nodiscard]] Inks inksOf(const PaintIconEvent& event)
        {
            const Color accent = event.accentRgb(InkGrade::Strongest);
            return { event.textRgb(InkGrade::Strong), accent, accent.withOpacity(k_softFill) };
        }

        // PixelPath::drawCircle starts the path over, so a circle joining others is drawn by hand.
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

        void paintChapter(const Drawing& drawing)
        {
            PixelPath folder;
            Icons::FolderIcon::addOutline(folder, { 1.5f, 3.5f, 12.5f, 11.5f });
            drawing.stroke(folder, drawing.inks.body);
        }

        // A cube with its top face lit.
        void paintModule(const Drawing& drawing)
        {
            PixelPath top;
            top.moveTo(7.0f, 1.6f);
            top.lineTo(12.4f, 4.3f);
            top.lineTo(7.0f, 7.0f);
            top.lineTo(1.6f, 4.3f);
            top.close();
            drawing.fill(top, drawing.inks.accentSoft);

            PixelPath cube;
            cube.moveTo(7.0f, 1.6f);
            cube.lineTo(12.4f, 4.3f);
            cube.lineTo(12.4f, 9.7f);
            cube.lineTo(7.0f, 12.4f);
            cube.lineTo(1.6f, 9.7f);
            cube.lineTo(1.6f, 4.3f);
            cube.close();
            cube.moveTo(1.6f, 4.3f);
            cube.lineTo(7.0f, 7.0f);
            cube.lineTo(12.4f, 4.3f);
            addLine(cube, { 7.0f, 7.0f }, { 7.0f, 12.4f });
            drawing.stroke(cube, drawing.inks.body);
        }

        // A window: a title band over a body.
        void paintControl(const Drawing& drawing)
        {
            PixelPath band;
            band.moveTo(1.5f, 5.2f);
            band.lineTo(1.5f, 3.5f);
            band.quadTo({ 1.5f, 2.0f }, { 3.0f, 2.0f });
            band.lineTo(11.0f, 2.0f);
            band.quadTo({ 12.5f, 2.0f }, { 12.5f, 3.5f });
            band.lineTo(12.5f, 5.2f);
            band.close();
            drawing.fill(band, drawing.inks.accent);

            PixelPath frame;
            frame.drawRoundedRect({ 1.5f, 2.0f, 12.5f, 12.0f }, 1.5f, 1.5f);
            addLine(frame, { 1.5f, 5.2f }, { 12.5f, 5.2f });
            drawing.stroke(frame, drawing.inks.body);
        }

        // A box with a header band and two member lines.
        void paintClass(const Drawing& drawing)
        {
            PixelPath box;
            box.drawRoundedRect({ 2.0f, 1.5f, 12.0f, 12.5f }, 1.0f, 1.0f);
            addLine(box, { 4.0f, 7.3f }, { 10.0f, 7.3f });
            addLine(box, { 4.0f, 9.8f }, { 8.2f, 9.8f });
            drawing.stroke(box, drawing.inks.body);

            PixelPath header;
            addLine(header, { 2.5f, 4.6f }, { 11.5f, 4.6f });
            drawing.stroke(header, drawing.inks.accent);
        }

        // Data laid out in cells.
        void paintStruct(const Drawing& drawing)
        {
            PixelPath cell;
            addRect(cell, { 2.0f, 2.0f, 7.0f, 7.0f });
            drawing.fill(cell, drawing.inks.accentSoft);

            PixelPath cells;
            addRect(cells, { 2.0f, 2.0f, 12.0f, 12.0f });
            addLine(cells, { 7.0f, 2.0f }, { 7.0f, 12.0f });
            addLine(cells, { 2.0f, 7.0f }, { 12.0f, 7.0f });
            drawing.stroke(cells, drawing.inks.body);
        }

        // Two views of one storage, the shared part lit.
        void paintUnion(const Drawing& drawing)
        {
            PixelPath shared;
            addRect(shared, { 4.5f, 4.5f, 8.5f, 8.5f });
            drawing.fill(shared, drawing.inks.accentSoft);

            PixelPath views;
            addRect(views, { 1.5f, 1.5f, 8.5f, 8.5f });
            addRect(views, { 5.5f, 5.5f, 12.5f, 12.5f });
            drawing.stroke(views, drawing.inks.body);
        }

        // A list of named values, one of them current.
        void paintEnum(const Drawing& drawing)
        {
            PixelPath current;
            addCircle(current, { 3.2f, 3.4f }, 1.4f);
            drawing.fill(current, drawing.inks.accent);

            PixelPath list;
            addCircle(list, { 3.2f, 7.0f }, 1.1f);
            addCircle(list, { 3.2f, 10.6f }, 1.1f);
            addLine(list, { 6.3f, 3.4f }, { 12.0f, 3.4f });
            addLine(list, { 6.3f, 7.0f }, { 12.0f, 7.0f });
            addLine(list, { 6.3f, 10.6f }, { 12.0f, 10.6f });
            drawing.stroke(list, drawing.inks.body);
        }

        // A name standing for a type.
        void paintAlias(const Drawing& drawing)
        {
            PixelPath name;
            addLine(name, { 1.8f, 5.0f }, { 6.5f, 5.0f });
            addLine(name, { 1.8f, 9.0f }, { 6.5f, 9.0f });
            drawing.stroke(name, drawing.inks.body);

            PixelPath arrow;
            addLine(arrow, { 7.5f, 7.0f }, { 12.2f, 7.0f });
            arrow.moveTo(9.8f, 4.6f);
            arrow.lineTo(12.2f, 7.0f);
            arrow.lineTo(9.8f, 9.4f);
            drawing.stroke(arrow, drawing.inks.accent);
        }

        // A constraint on a template argument.
        void paintConcept(const Drawing& drawing)
        {
            PixelPath brackets;
            brackets.moveTo(4.2f, 2.5f);
            brackets.lineTo(1.5f, 7.0f);
            brackets.lineTo(4.2f, 11.5f);
            brackets.moveTo(9.8f, 2.5f);
            brackets.lineTo(12.5f, 7.0f);
            brackets.lineTo(9.8f, 11.5f);
            drawing.stroke(brackets, drawing.inks.body);

            PixelPath check;
            check.moveTo(5.3f, 7.2f);
            check.lineTo(6.6f, 8.6f);
            check.lineTo(8.8f, 5.4f);
            drawing.stroke(check, drawing.inks.accent);
        }
    }

    void paintRowIcon(PaintIconEvent& event, const RowIcon icon)
    {
        const FloatRect& rect = event.iconRect();
        const float scale = std::min(rect.width(), rect.height()) / k_grid;
        const Drawing drawing = {
            event.canvas(),
            inksOf(event),
            Matrix3x2::translation(rect.topLeft()) * Matrix3x2::scale(scale)
        };
        switch (icon)
        {
            case RowIcon::Chapter:
                paintChapter(drawing);
                break;
            case RowIcon::Module:
                paintModule(drawing);
                break;
            case RowIcon::Control:
                paintControl(drawing);
                break;
            case RowIcon::Class:
                paintClass(drawing);
                break;
            case RowIcon::Struct:
                paintStruct(drawing);
                break;
            case RowIcon::Union:
                paintUnion(drawing);
                break;
            case RowIcon::Enum:
                paintEnum(drawing);
                break;
            case RowIcon::Alias:
                paintAlias(drawing);
                break;
            case RowIcon::Concept:
                paintConcept(drawing);
                break;
        }
    }

    RowIcon rowIconOf(const Type& type)
    {
        if (type.isControl)
            return RowIcon::Control;
        switch (type.kind)
        {
            case TypeKind::Struct:
                return RowIcon::Struct;
            case TypeKind::Union:
                return RowIcon::Union;
            case TypeKind::Enum:
                return RowIcon::Enum;
            case TypeKind::Alias:
                return RowIcon::Alias;
            case TypeKind::Concept:
                return RowIcon::Concept;
            case TypeKind::Class:
                break;
        }
        return RowIcon::Class;
    }

    InTextIcon rowIcon(const RowIcon icon)
    {
        return InTextIcon{ k_rowIconSize, [icon](PaintIconEvent& event) {
            paintRowIcon(event, icon);
        } };
    }

    void writeRowKind(Text& text, const Type& type)
    {
        if (type.isControl)
            return;
        text << FlexSpace{ k_kindGap } << TextStyleId::SubBody << InkGrade::Muted
            << kindWord(type.kind) << PopColor{} << PopTextStyle{};
    }
}
