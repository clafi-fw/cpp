export module ClaFi.Icons.RowIcons;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

// Icons that lead a tree's or a list's row, drawn on one grid in one stroke. See Icons#rowicons
namespace ClaFi::Icons::RowIcons
{
    export constexpr float GridSize = 14.0f;    // the grid row icons are drawn on
    export constexpr float StrokeWidth = 1.1f;  // in grid units, scaling with the shape
    export constexpr float TintOpacity = 0.35f; // the accent's opacity on a face tinted, not filled
    export constexpr float Size = 14.0f;        // how wide a row icon stands in the row's text
    export constexpr float Gap = 5.0f;          // between a row icon and the row's words
    export constexpr float KindGap = 12.0f;     // the least room before a row's kind word

    // The inks a row icon is drawn in, read off the control it is drawn for.
    export struct Inks
    {
        Color body{};       // strong text, for the outline
        Color accent{};     // the part that names the kind
        Color accentTint{}; // the accent at TintOpacity
        Color spot{};
        Color surface{};    // clears a band round a part that crosses another
    };

    // One row icon being drawn: the canvas, its inks and the grid laid over the icon's rect.
    export struct Drawing
    {
        explicit Drawing(DrawIconEvent&);

        Graphics::Canvas& canvas;
        Inks inks;
        Graphics::Matrix3x2 transform; // from the grid to the canvas

        void stroke(const Graphics::PixelPath&, Color) const; // in StrokeWidth
        void fill(const Graphics::PixelPath&, Color) const;
    };

    // PixelPath::drawCircle starts the path over, so a circle joining other shapes is added here.
    export void addCircle(Graphics::PixelPath&, FloatPoint center, float radius);
    export void addRect(Graphics::PixelPath&, const FloatRect&);
    export void addLine(Graphics::PixelPath&, FloatPoint from, FloatPoint to);

    // The box a folder's outline fills on the grid.
    export [[nodiscard]] FloatRect folderBox();
    // A folder: FolderIcon's outline across folderBox, in the body ink.
    export void drawFolder(const Drawing&);

    // Writes a row's kind word at the row's far end, in SubBody and muted. See Icons#writekindword
    export void writeKindWord(Text&, std::wstring_view kindWord);
}
