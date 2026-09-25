export module ClaFi.Icons.HomeIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::HomeIcon
{
    using namespace ::ClaFi::Graphics;

    // A pitched roof over a door - home as the place you live.
    export void paintHouse(PaintIconEvent&);
    // A block of windows - home as the building, and as a grid of tiles. The one shape carries
    // both readings, which is what a home that opens onto a list of things wants.
    export void paintBlock(PaintIconEvent&);


    //----------------------------------------------------------------------------


    namespace
    {
        // The house in a unit square. The roof is the widest part and the only one that reaches the
        // edges, so everything else is placed against it rather than against the icon rect.
        constexpr float k_apexY = 0.07f;
        constexpr float k_eavesY = 0.47f;

        // The walls start above the roof line: their top ends are covered by it, so the eaves meet
        // solid wall rather than leaving a notch where two strokes almost touch.
        constexpr float k_wallTop = 0.38f;
        constexpr float k_wallLeft = 0.19f;
        constexpr float k_wallRight = 0.81f;
        constexpr float k_floorY = 0.94f;

        // A door rather than a window: standing on the floor line, it is three strokes instead of
        // four, and it still reads once the fourth would have closed up.
        constexpr float k_doorLeft = 0.39f;
        constexpr float k_doorRight = 0.61f;
        constexpr float k_doorTop = 0.62f;

        // The block, also in a unit square. Square rather than tall, and rounded rather than
        // square-cornered, because it has to be legible as a grid of tiles as well as as a
        // building - a tall box with windows in it can only be the one thing.
        constexpr float k_blockRadiusRatio = 0.18f;

        // Two across and three down. More columns and the rows stop reading as storeys; fewer
        // rows and the grid stops reading as tiles.
        constexpr int k_windowColumns = 2;
        constexpr int k_windowRows = 3;
        constexpr float k_windowWidth = 0.20f;
        constexpr float k_windowHeight = 0.14f;
        constexpr float k_windowLeft = 0.26f;
        constexpr float k_windowTop = 0.18f;
        constexpr float k_windowStepX = 0.28f;
        constexpr float k_windowStepY = 0.25f;

        constexpr float k_strokeRatio = 0.09f;
        constexpr float k_minStrokeWidth = 1.0f;

        // The square the drawing is laid out in: the icon rect's shorter side, less the stroke
        // that straddles the outline, centred in whatever the caller gave.
        [[nodiscard]] FloatRect drawingBounds(const FloatRect& iconRect, float strokeWidth)
        {
            float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
            FloatPoint center = iconRect.center();
            return {
                center.x - size * 0.5f,
                center.y - size * 0.5f,
                center.x + size * 0.5f,
                center.y + size * 0.5f,
            };
        }

        [[nodiscard]] float strokeWidthFor(const FloatRect& iconRect)
        {
            return std::max(k_minStrokeWidth, std::min(iconRect.width(), iconRect.height()) * k_strokeRatio);
        }
    }

    void paintHouse(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const Color strokeColor = event.textRgb(InkGrade::Strongest);

        const float strokeWidth = strokeWidthFor(iconRect);
        const FloatRect bounds = drawingBounds(iconRect, strokeWidth);
        const float size = bounds.width();
        const Matrix3x2 transform = Matrix3x2::translation(bounds.topLeft());

        PixelPath path;

        path.moveTo(0.0f, size * k_eavesY);
        path.lineTo(size * 0.5f, size * k_apexY);
        path.lineTo(size, size * k_eavesY);

        path.moveTo(size * k_wallLeft, size * k_wallTop);
        path.lineTo(size * k_wallLeft, size * k_floorY);
        path.lineTo(size * k_wallRight, size * k_floorY);
        path.lineTo(size * k_wallRight, size * k_wallTop);

        path.moveTo(size * k_doorLeft, size * k_floorY);
        path.lineTo(size * k_doorLeft, size * k_doorTop);
        path.lineTo(size * k_doorRight, size * k_doorTop);
        path.lineTo(size * k_doorRight, size * k_floorY);

        event.canvas().drawPath(path, { PathDrawLayer::stroke(strokeColor, strokeWidth) }, &transform);
    }

    void paintBlock(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        Color blockColor = event.textRgb(InkGrade::Muted);
        Color windowColor = event.surfaceRgb();

        float strokeWidth = strokeWidthFor(iconRect);
        FloatRect bounds = drawingBounds(iconRect, strokeWidth);
        float size = bounds.width();


        float radius = size * k_blockRadiusRatio;
        event.canvas().fillRoundedRectangle(bounds, radius, radius, blockColor);

        for (int row = 0; row != k_windowRows; ++row)
        {
            for (int column = 0; column != k_windowColumns; ++column)
            {
                float left = bounds.left + size * (k_windowLeft + k_windowStepX * column);
                float top = bounds.top + size * (k_windowTop + k_windowStepY * row);
                FloatRect window = {
                    left,
                    top,
                    left + size * k_windowWidth,
                    top + size * k_windowHeight,
                };
                event.canvas().fillRectangle(window, windowColor);
            }
        }
    }

}
