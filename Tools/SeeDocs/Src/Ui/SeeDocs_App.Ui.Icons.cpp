module SeeDocs_App.Ui.Icons;

import SeeDocs_App.Database;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.Icons.FolderIcon;
import ClaFi.Icons.RowIcons;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App::Ui
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;
    using namespace ::ClaFi::Icons::RowIcons;

    namespace
    {
        // **The menu's icons are drawn on a grid of their own**, the size the menu shows them at,
        // and in the theme's Thin stroke rather than the rows' one, which is how the framework's
        // own menu icons beside them are drawn.
        constexpr float k_menuGrid = 32.0f;
        constexpr float k_flapFill = 0.2f;   // the open folder's flap, tinted rather than filled
        constexpr float k_haloWidth = 3.2f;  // the band cleared around the spark, in strokes

        // A menu icon's grid in its rect, and the Thin stroke in grid units: the transform scales
        // the stroke with the shape.
        struct MenuDrawing
        {
            Canvas& canvas;
            Matrix3x2 transform{};
            float stroke{};
        };

        [[nodiscard]] MenuDrawing menuDrawing(DrawIconEvent& event)
        {
            const FloatRect& rect = event.iconRect();
            const float size = std::min(rect.width(), rect.height());
            const float scale = size / k_menuGrid;
            return {
                event.canvas(),
                Matrix3x2::Translation(rect.center() - FloatPoint{ size * 0.5f })
                    * Matrix3x2::Scale(scale),
                event.scaledStrokeWidth(Thickness::Thin) / scale
            };
        }

        // **The back of an open folder**, open where the flap stands in front of it: up the left
        // side, over the tab, and down the right side as far as the flap's top.
        void addFolderBack(PixelPath& path)
        {
            const float left = 2.0f;
            const float top = 5.5f;
            const float right = 27.0f;
            const float bottom = 26.0f;
            const float width = right - left;
            const float body = top + (bottom - top) * 0.18f;
            const float radius = width * 0.10f;
            const float flapTop = 13.0f;
            path.moveTo(left, bottom - radius);
            path.lineTo(left, top + radius);
            path.quadTo({ left, top }, { left + radius, top });
            path.lineTo(left + width * 0.40f - radius * 0.6f, top);
            path.lineTo(left + width * 0.53f, body);
            path.lineTo(right - radius, body);
            path.quadTo({ right, body }, { right, body + radius });
            path.lineTo(right, flapTop);
        }

        // A four-pointed star, its long points up, down and to the sides.
        void addSpark(PixelPath& path, const FloatPoint center, const float outer,
            const float inner)
        {
            std::array<FloatPoint, 8ull> points{};
            for (std::size_t index = 0; index < points.size(); ++index)
            {
                const float angle = (-90.0f + 45.0f * static_cast<float>(index))
                    * std::numbers::pi_v<float> / 180.0f;
                const float radius = index % 2ull == 0ull ? outer : inner;
                points[index] = center + FloatPoint{ std::cos(angle), std::sin(angle) } * radius;
            }
            path.moveTo(points[0ull]);
            for (std::size_t index = 1; index < points.size(); ++index)
                path.lineTo(points[index]);
            path.close();
        }

        // A house, its door in the spot.
        void drawHome(const Drawing& drawing)
        {
            PixelPath door;
            door.moveTo(5.8f, 12.0f);
            door.lineTo(5.8f, 8.4f);
            door.lineTo(8.2f, 8.4f);
            door.lineTo(8.2f, 12.0f);
            door.close();
            drawing.fill(door, drawing.inks.spot);

            PixelPath house;
            house.moveTo(1.8f, 7.0f);
            house.lineTo(7.0f, 2.2f);
            house.lineTo(12.2f, 7.0f);
            house.moveTo(3.0f, 6.0f);
            house.lineTo(3.0f, 12.0f);
            house.lineTo(11.0f, 12.0f);
            house.lineTo(11.0f, 6.0f);
            drawing.stroke(house, drawing.inks.body);
        }

        // The folder with its tab and the band under it lit: the root of the folders.
        // The band follows the outline's tab and slant - see FolderIcon - with sharp corners,
        // which lie within the outline's rounded stroke.
        void drawProject(const Drawing& drawing)
        {
            const FloatRect box = folderBox();
            const float width = box.width();
            const float bodyTop = box.top + box.height() * 0.18f;
            const float bandBottom = 6.8f;
            PixelPath band;
            band.moveTo(box.left, box.top);
            band.lineTo(box.left + width * 0.40f, box.top);
            band.lineTo(box.left + width * 0.53f, bodyTop);
            band.lineTo(box.right, bodyTop);
            band.lineTo(box.right, bandBottom);
            band.lineTo(box.left, bandBottom);
            band.close();
            drawing.fill(band, drawing.inks.spot);

            PixelPath folder;
            Icons::FolderIcon::addOutline(folder, box);
            drawing.stroke(folder, drawing.inks.body);
        }

        // Three sliders, their knobs in the accent, each knob clearing the line it rides.
        void drawOptions(const Drawing& drawing)
        {
            const std::array<FloatPoint, 3ull> knobs{
                FloatPoint{ 9.2f, 3.5f },
                FloatPoint{ 4.8f, 7.0f },
                FloatPoint{ 7.8f, 10.5f }
            };
            PixelPath lines;
            for (const FloatPoint& knob : knobs)
                addLine(lines, { 2.0f, knob.y }, { 12.0f, knob.y });
            drawing.stroke(lines, drawing.inks.body);

            PixelPath halos;
            PixelPath dots;
            for (const FloatPoint& knob : knobs)
            {
                addCircle(halos, knob, 2.2f);
                addCircle(dots, knob, 1.5f);
            }
            drawing.fill(halos, drawing.inks.surface);
            drawing.fill(dots, drawing.inks.accent);
        }

        // A window, rows in the accent standing in its left pane.
        void drawSidebar(const Drawing& drawing)
        {
            PixelPath frame;
            frame.drawRoundedRect({ 1.5f, 2.5f, 12.5f, 11.5f }, 1.4f, 1.4f);
            addLine(frame, { 6.0f, 2.5f }, { 6.0f, 11.5f });
            drawing.stroke(frame, drawing.inks.body);

            PixelPath rows;
            addLine(rows, { 3.2f, 5.0f }, { 4.4f, 5.0f });
            addLine(rows, { 3.2f, 7.0f }, { 4.4f, 7.0f });
            addLine(rows, { 3.2f, 9.0f }, { 4.4f, 9.0f });
            drawing.stroke(rows, drawing.inks.accent);
        }

        // A section: a header band in the accent, two rows under it.
        void drawCategory(const Drawing& drawing)
        {
            PixelPath band;
            band.drawRoundedRect({ 1.5f, 2.2f, 12.5f, 4.8f }, 1.0f, 1.0f);
            drawing.fill(band, drawing.inks.accent);

            PixelPath rows;
            addLine(rows, { 1.5f, 8.0f }, { 12.5f, 8.0f });
            addLine(rows, { 1.5f, 11.3f }, { 12.5f, 11.3f });
            drawing.stroke(rows, drawing.inks.body);
        }

        // A sheet, its corner folded and the fold tinted, with two lines of its text.
        void drawContent(const Drawing& drawing)
        {
            PixelPath fold;
            fold.moveTo(8.5f, 1.5f);
            fold.lineTo(8.5f, 4.5f);
            fold.lineTo(11.5f, 4.5f);
            fold.close();
            drawing.fill(fold, drawing.inks.accentTint);

            PixelPath sheet;
            sheet.moveTo(3.0f, 1.5f);
            sheet.lineTo(8.5f, 1.5f);
            sheet.lineTo(11.5f, 4.5f);
            sheet.lineTo(11.5f, 12.5f);
            sheet.lineTo(3.0f, 12.5f);
            sheet.close();
            sheet.moveTo(8.5f, 1.5f);
            sheet.lineTo(8.5f, 4.5f);
            sheet.lineTo(11.5f, 4.5f);
            addLine(sheet, { 5.0f, 7.5f }, { 9.5f, 7.5f });
            addLine(sheet, { 5.0f, 10.0f }, { 9.5f, 10.0f });
            drawing.stroke(sheet, drawing.inks.body);
        }

        // Three layers, the top one tinted.
        void drawAllTheApis(const Drawing& drawing)
        {
            PixelPath top;
            top.moveTo(7.0f, 2.0f);
            top.lineTo(12.3f, 4.6f);
            top.lineTo(7.0f, 7.2f);
            top.lineTo(1.7f, 4.6f);
            top.close();
            drawing.fill(top, drawing.inks.accentTint);
            drawing.stroke(top, drawing.inks.body);

            PixelPath layers;
            layers.moveTo(1.7f, 7.3f);
            layers.lineTo(7.0f, 9.9f);
            layers.lineTo(12.3f, 7.3f);
            layers.moveTo(1.7f, 10.0f);
            layers.lineTo(7.0f, 12.6f);
            layers.lineTo(12.3f, 10.0f);
            drawing.stroke(layers, drawing.inks.body);
        }

        // A cube with its top face lit.
        void drawModule(const Drawing& drawing)
        {
            PixelPath top;
            top.moveTo(7.0f, 1.6f);
            top.lineTo(12.4f, 4.3f);
            top.lineTo(7.0f, 7.0f);
            top.lineTo(1.6f, 4.3f);
            top.close();
            drawing.fill(top, drawing.inks.accentTint);

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
        void drawControl(const Drawing& drawing)
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
        void drawClass(const Drawing& drawing)
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
        void drawStruct(const Drawing& drawing)
        {
            PixelPath cell;
            addRect(cell, { 2.0f, 2.0f, 7.0f, 7.0f });
            drawing.fill(cell, drawing.inks.accentTint);

            PixelPath cells;
            addRect(cells, { 2.0f, 2.0f, 12.0f, 12.0f });
            addLine(cells, { 7.0f, 2.0f }, { 7.0f, 12.0f });
            addLine(cells, { 2.0f, 7.0f }, { 12.0f, 7.0f });
            drawing.stroke(cells, drawing.inks.body);
        }

        // Two views of one storage, the shared part lit.
        void drawUnion(const Drawing& drawing)
        {
            PixelPath shared;
            addRect(shared, { 4.5f, 4.5f, 8.5f, 8.5f });
            drawing.fill(shared, drawing.inks.accentTint);

            PixelPath views;
            addRect(views, { 1.5f, 1.5f, 8.5f, 8.5f });
            addRect(views, { 5.5f, 5.5f, 12.5f, 12.5f });
            drawing.stroke(views, drawing.inks.body);
        }

        // A list of named values, one of them current.
        void drawEnum(const Drawing& drawing)
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
        void drawAlias(const Drawing& drawing)
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
        void drawConcept(const Drawing& drawing)
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

        // A lens over what is searched, its handle in the accent.
        void drawSearch(const Drawing& drawing)
        {
            PixelPath lens;
            addCircle(lens, { 6.0f, 6.0f }, 3.9f);
            drawing.stroke(lens, drawing.inks.body);

            PixelPath handle;
            addLine(handle, { 8.9f, 8.9f }, { 12.3f, 12.3f });
            drawing.stroke(handle, drawing.inks.accent);
        }
    }

    void drawRowIcon(DrawIconEvent& event, const RowIcon icon)
    {
        const Drawing drawing{ event };
        switch (icon)
        {
            case RowIcon::Home:
                drawHome(drawing);
                break;
            case RowIcon::Project:
                drawProject(drawing);
                break;
            case RowIcon::Options:
                drawOptions(drawing);
                break;
            case RowIcon::Sidebar:
                drawSidebar(drawing);
                break;
            case RowIcon::Category:
                drawCategory(drawing);
                break;
            case RowIcon::Content:
                drawContent(drawing);
                break;
            case RowIcon::AllTheApis:
                drawAllTheApis(drawing);
                break;
            case RowIcon::Folder:
                drawFolder(drawing);
                break;
            case RowIcon::Module:
                drawModule(drawing);
                break;
            case RowIcon::Control:
                drawControl(drawing);
                break;
            case RowIcon::Class:
                drawClass(drawing);
                break;
            case RowIcon::Struct:
                drawStruct(drawing);
                break;
            case RowIcon::Union:
                drawUnion(drawing);
                break;
            case RowIcon::Enum:
                drawEnum(drawing);
                break;
            case RowIcon::Alias:
                drawAlias(drawing);
                break;
            case RowIcon::Concept:
                drawConcept(drawing);
                break;
            case RowIcon::Search:
                drawSearch(drawing);
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
            case TypeKind::Defined:
                return RowIcon::Alias;
            case TypeKind::Concept:
                return RowIcon::Concept;
            case TypeKind::Class:
            case TypeKind::Interface:
                break;
        }
        return RowIcon::Class;
    }

    std::optional<RowIcon> rowIconOf(const SidebarKind kind)
    {
        switch (kind)
        {
            case SidebarKind::Category:
                return RowIcon::Category;
            case SidebarKind::Content:
                return RowIcon::Content;
            case SidebarKind::AllTheApis:
                return RowIcon::AllTheApis;
            case SidebarKind::Divider:
                break;
        }
        return std::nullopt;
    }

    // **The kind is the icon's tag**: the layout cache tells icons apart by size and tag alone, so
    // a folder's row and a module's row of one name would otherwise be drawn from one layout.
    InTextIcon rowIcon(const RowIcon icon)
    {
        const DrawIconFunc paint = [icon](DrawIconEvent& event) {
            drawRowIcon(event, icon);
        };
        return InTextIcon{ Icons::RowIcons::Size, paint, Tag{ icon } };
    }

    InTextIcon titleIcon(const RowIcon icon)
    {
        const DrawIconFunc paint = [icon](DrawIconEvent& event) {
            drawRowIcon(event, icon);
        };
        return InTextIcon{ k_titleIconSize, paint, Tag{ icon } };
    }

    void writeRowKind(Text& text, const Type& type)
    {
        if (!type.isControl)
            writeKindWord(text, kindWord(type.kind));
    }

    void drawOpenIcon(DrawIconEvent& event)
    {
        const MenuDrawing drawing = menuDrawing(event);
        PixelPath back;
        addFolderBack(back);
        drawing.canvas.drawPath(back,
            { PathDrawLayer::Stroke(event.textRgb(InkGrade::Strong), drawing.stroke) },
            &drawing.transform);

        const std::array<FloatPoint, 4ull> corners{
            FloatPoint{ 7.2f, 13.0f },
            FloatPoint{ 30.6f, 13.0f },
            FloatPoint{ 26.0f, 26.0f },
            FloatPoint{ 2.0f, 26.0f }
        };
        PixelPath flap;
        flap.addRoundedPolygon(corners, 1.6f);
        const Color accent = event.accentRgb(InkGrade::Strongest);
        drawing.canvas.drawPath(flap,
            {
                PathDrawLayer::Fill(accent.withOpacity(k_flapFill)),
                PathDrawLayer::Stroke(accent, drawing.stroke)
            },
            &drawing.transform);
    }

    // The spark crosses the folder's corner, so a band in the surface's colour is cleared round
    // it first, the way the framework's Open in Explorer arrow crosses its folder.
    void drawNewProjectIcon(DrawIconEvent& event)
    {
        const MenuDrawing drawing = menuDrawing(event);
        PixelPath folder;
        Icons::FolderIcon::addOutline(folder, { 2.0f, 7.5f, 26.5f, 27.5f });
        drawing.canvas.drawPath(folder,
            { PathDrawLayer::Stroke(event.textRgb(InkGrade::Strong), drawing.stroke) },
            &drawing.transform);

        PixelPath spark;
        addSpark(spark, { 24.5f, 8.0f }, 7.0f, 2.1f);
        drawing.canvas.drawPath(spark,
            {
                PathDrawLayer::Stroke(event.surfaceRgb(), drawing.stroke * k_haloWidth),
                PathDrawLayer::Fill(event.accentRgb(InkGrade::Strongest))
            },
            &drawing.transform);
    }
}
