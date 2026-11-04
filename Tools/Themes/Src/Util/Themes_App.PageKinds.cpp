module Themes_App.PageKinds;

import ClaFi.Icons.ColorChannelSweep;
import ClaFi.Icons.RowIcons;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi::Graphics;
    using namespace ::ClaFi::Icons::RowIcons;
    namespace Sweep = ::ClaFi::Icons::ColorChannelSweep;

    namespace
    {
        constexpr float k_rainbowSaturation = 0.8f; // the hues are the icon's own, in any theme
        constexpr float k_rainbowLuminosity = 0.55f;
        constexpr float k_rainbowRadius = 6.6f;     // past the farthest point of the outline

        // An artist's palette, its thumb notch at the lower right.
        void addPaletteOutline(PixelPath& path)
        {
            path.moveTo(7.0f, 1.6f);
            path.cubicTo({ 10.4f, 1.6f }, { 12.6f, 3.7f }, { 12.6f, 6.3f });
            path.cubicTo({ 12.6f, 8.3f }, { 11.3f, 8.7f }, { 10.3f, 8.7f });
            path.cubicTo({ 9.1f, 8.7f }, { 8.7f, 9.7f }, { 9.2f, 10.6f });
            path.cubicTo({ 9.8f, 11.7f }, { 8.9f, 12.4f }, { 7.3f, 12.4f });
            path.cubicTo({ 3.9f, 12.4f }, { 1.4f, 10.0f }, { 1.4f, 7.0f });
            path.cubicTo({ 1.4f, 4.0f }, { 3.8f, 1.6f }, { 7.0f, 1.6f });
            path.close();
        }

        // The palette filled with the hues swept round its centre, its outline muted over them.
        // The hues take the event's fade by hand, being none of the theme's inks.
        void drawPalette(const DrawIconEvent& event, const Drawing& drawing)
        {
            Sweep::BoundaryColors colors{};
            for (int i = 0; i <= Sweep::k_segmentCount; ++i)
            {
                const float hue = static_cast<float>(i) / Sweep::k_segmentCount;
                const Hsl color = { hue, k_rainbowSaturation, k_rainbowLuminosity };
                colors[i] = event.applyDisabledFactor(color.toColor());
            }
            PixelPath palette;
            addPaletteOutline(palette);
            drawing.canvas.pushClip(palette, &drawing.transform);
            Sweep::drawPieSweep(drawing.canvas, { 7.0f, 7.0f }, k_rainbowRadius, colors,
                Sweep::SweepEnds::Cyclic, drawing.transform);
            drawing.canvas.popClip();
            drawing.stroke(palette, event.textRgb(InkGrade::Muted));
        }

        // A drop of paint, tinted, beside the lines of its list.
        void drawRules(const Drawing& drawing)
        {
            PixelPath drop;
            drop.moveTo(4.2f, 2.2f);
            drop.cubicTo({ 4.2f, 2.2f }, { 1.4f, 5.9f }, { 1.4f, 8.0f });
            drop.arcTo({ 2.8f, 2.8f }, 0.0f, ArcSize::Small, ArcSweep::CounterClockwise,
                { 7.0f, 8.0f });
            drop.cubicTo({ 7.0f, 5.9f }, { 4.2f, 2.2f }, { 4.2f, 2.2f });
            drop.close();
            drawing.fill(drop, drawing.inks.accentTint);
            drawing.stroke(drop, drawing.inks.accent);

            PixelPath lines;
            addLine(lines, { 9.0f, 4.2f }, { 12.6f, 4.2f });
            addLine(lines, { 9.0f, 7.4f }, { 12.6f, 7.4f });
            addLine(lines, { 9.0f, 10.6f }, { 12.6f, 10.6f });
            drawing.stroke(lines, drawing.inks.body);
        }

        // A box, and in the accent the lengths across and down it.
        void drawSizes(const Drawing& drawing)
        {
            PixelPath box;
            box.drawRoundedRect({ 5.0f, 5.0f, 12.5f, 12.5f }, 1.1f, 1.1f);
            drawing.stroke(box, drawing.inks.body);

            PixelPath lengths;
            addLine(lengths, { 5.0f, 2.4f }, { 12.5f, 2.4f });
            addLine(lengths, { 5.0f, 1.3f }, { 5.0f, 3.5f });
            addLine(lengths, { 12.5f, 1.3f }, { 12.5f, 3.5f });
            addLine(lengths, { 2.4f, 5.0f }, { 2.4f, 12.5f });
            addLine(lengths, { 1.3f, 5.0f }, { 3.5f, 5.0f });
            addLine(lengths, { 1.3f, 12.5f }, { 3.5f, 12.5f });
            drawing.stroke(lengths, drawing.inks.accent);
        }
    }

    void drawPageIcon(DrawIconEvent& event, const PageKind kind)
    {
        const Drawing drawing{ event };
        switch (kind)
        {
            case PageKind::Palette:
                drawPalette(event, drawing);
                break;
            case PageKind::Folder:
                drawFolder(drawing);
                break;
            case PageKind::Rules:
                drawRules(drawing);
                break;
            case PageKind::Sizes:
                drawSizes(drawing);
                break;
        }
    }

    // **The kind is the icon's tag**: the layout cache tells icons apart by size and tag alone, so
    // a rules row and a sizes row of one name would otherwise be drawn from one layout.
    InTextIcon pageIcon(const PageKind kind, const float size)
    {
        const DrawIconFunc draw = [kind](DrawIconEvent& event) {
            drawPageIcon(event, kind);
        };
        return InTextIcon{ size, draw, Tag{ kind } };
    }

    std::wstring_view kindWordOf(const PageKind kind)
    {
        switch (kind)
        {
            case PageKind::Rules:
                return L"rules";
            case PageKind::Sizes:
                return L"sizes";
            case PageKind::Palette:
            case PageKind::Folder:
                break;
        }
        return {};
    }

    void writePageTitle(Text& text, const PageKind kind, const std::wstring_view name)
    {
        text << pageIcon(kind, k_titleIconSize) << Space{ k_titleIconGap };
        text << StyledText{ TextStyles::SubTitle, name };
        const std::wstring_view kindWord = kindWordOf(kind);
        if (!kindWord.empty())
        {
            text << L"  ";
            text << StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, kindWord } };
        }
    }
}
