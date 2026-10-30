export module ClaFi.Core.Graphics.TabRenderer;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{
    export void drawTab(Canvas& canvas, const TabGeometry& geometry, const TabColors& colors)
    {
        // 1. Establish pixel-perfect offsets to account for stroke line width
        const float halfWidth = geometry.lineWidth / 2.0f;

        // Path centerline vertical positions
        const float vBase = halfWidth;                                  // Center of the baseline stroke
        const float vTop = geometry.tabProtrusion - halfWidth;          // Center of the tab's peak stroke

        // Path centerline horizontal positions
        const float uStart = geometry.lineStart + halfWidth;            // Start of the line path (inside left cap)
        const float uEnd = std::max(geometry.lineEnd,
            geometry.tabEnd /*preventing later std::clamp crash*/) - halfWidth; // End of the line path (inside right cap)
        const float uTabStart = geometry.tabStart + halfWidth;          // Centerline of the tab's left edge
        const float uTabEnd = std::max(geometry.tabEnd - halfWidth, uTabStart); // Centerline of the tab's right edge

        // 2. Define the local-to-world coordinate transformer based on strip orientation
        auto toWorld = [&](float u, float v) -> FloatPoint
            {
                switch (geometry.orientation)
                {
                case TabsOrientation::HorizontalTop:
                    return { geometry.origin.x + u, geometry.origin.y - v };

                case TabsOrientation::HorizontalBottom:
                    return { geometry.origin.x + u, geometry.origin.y + v };

                case TabsOrientation::VerticalLeft:
                    return { geometry.origin.x - v, geometry.origin.y + u };

                case TabsOrientation::VerticalRight:
                    return { geometry.origin.x + v, geometry.origin.y + u };
                }
                return geometry.origin;
            };

        // 3. Prevent corner overlaps by calculating maximum safe corner radius limits
        float maxH = (vTop - vBase) / 2.0f;
        float maxW = (uTabEnd - uTabStart) / 2.0f;
        float maxL = uTabStart - uStart;
        float maxR = uEnd - uTabEnd;

        const float actualRadius = std::clamp(
            geometry.cornerRadius,
            0.0f,
            std::min({ maxH, maxW, maxL, maxR })
        );

        // 4. Build the continuous stroke path with rounded peak and base corners
        PixelPath strokePath;
        strokePath.moveTo(toWorld(uStart, vBase));
        strokePath.lineTo(toWorld(uTabStart - actualRadius, vBase));

        if (actualRadius > 0.0f)
        {
            // Base-Left Outside Corner
            strokePath.quadTo(
                toWorld(uTabStart, vBase),
                toWorld(uTabStart, vBase + actualRadius)
            );

            // Vertical Left Wall
            strokePath.lineTo(toWorld(uTabStart, vTop - actualRadius));

            // Top-Left Peak Corner
            strokePath.quadTo(
                toWorld(uTabStart, vTop),
                toWorld(uTabStart + actualRadius, vTop)
            );

            // Horizontal Top Cap
            strokePath.lineTo(toWorld(uTabEnd - actualRadius, vTop));

            // Top-Right Peak Corner
            strokePath.quadTo(
                toWorld(uTabEnd, vTop),
                toWorld(uTabEnd, vTop - actualRadius)
            );

            // Vertical Right Wall
            strokePath.lineTo(toWorld(uTabEnd, vBase + actualRadius));

            // Base-Right Outside Corner
            strokePath.quadTo(
                toWorld(uTabEnd, vBase),
                toWorld(uTabEnd + actualRadius, vBase)
            );
        }
        else
        {
            strokePath.lineTo(toWorld(uTabStart, vBase));
            strokePath.lineTo(toWorld(uTabStart, vTop));
            strokePath.lineTo(toWorld(uTabEnd, vTop));
            strokePath.lineTo(toWorld(uTabEnd, vBase));
        }

        strokePath.lineTo(toWorld(uEnd, vBase));

        // 5. Fill the active tab surface
        PixelPath fillPath = strokePath;
        fillPath.lineTo(toWorld(uEnd, 0.0f));   // Drop down to the absolute bottom of the right stroke edge
        fillPath.lineTo(toWorld(uStart, 0.0f)); // Draw horizontal line to the absolute bottom of the left stroke edge
        fillPath.close();                       // Close the path vertically back up to (uStart, vBase)
        canvas.fillPath(fillPath, colors.surface);

        // 6. Calculate the dynamic center ratio of the tab relative to the line strip
        const float lineLength = geometry.lineEnd - geometry.lineStart;
        const float tabCenter = (geometry.tabStart + geometry.tabEnd) / 2.0f;
        float centerRatio = (lineLength > 0.0f) ? (tabCenter - geometry.lineStart) / lineLength : 0.5f;
        centerRatio = std::clamp(centerRatio, 0.0f, 1.0f);

        // 7. Draw the main tab strip stroke
        LinearGradient lineGrad{
            .startPoint = toWorld(geometry.lineStart, vBase),
            .endPoint = toWorld(geometry.lineEnd, vBase),
            .stops = {
                { 0.0f, colors.lineCaps },
                { centerRatio, colors.tabLine },
                { 1.0f, colors.lineCaps }
            }
        };
        canvas.drawPath(
            strokePath,
            lineGrad,
            geometry.lineWidth
        );

        // 8. Draw the indicator over the line, flush with its outer edge
        // The line's outer edge around the peak - its centerline grown by half its width
        const float uOuterStart = uTabStart - halfWidth;
        const float uOuterEnd = uTabEnd + halfWidth;
        const float vOuter = vTop + halfWidth;
        const float outerRadius = actualRadius > 0.0f ? actualRadius + halfWidth : 0.0f;

        const FloatPoint baseCorner = toWorld(uOuterStart, 0.0f);
        const FloatPoint peakCorner = toWorld(uOuterEnd, vOuter);
        RoundedRectangleParts peakOuterShape = {
            .bounds = {
                std::min(baseCorner.x, peakCorner.x),
                std::min(baseCorner.y, peakCorner.y),
                std::max(baseCorner.x, peakCorner.x),
                std::max(baseCorner.y, peakCorner.y)
            }
        };
        RectSide peakSide = RectSide::Top;
        switch (geometry.orientation)
        {
            case TabsOrientation::HorizontalTop:
                peakSide = RectSide::Top;
                peakOuterShape.radii.topLeft = outerRadius;
                peakOuterShape.radii.topRight = outerRadius;
                break;
            case TabsOrientation::HorizontalBottom:
                peakSide = RectSide::Bottom;
                peakOuterShape.radii.bottomRight = outerRadius;
                peakOuterShape.radii.bottomLeft = outerRadius;
                break;
            case TabsOrientation::VerticalLeft:
                peakSide = RectSide::Left;
                peakOuterShape.radii.topLeft = outerRadius;
                peakOuterShape.radii.bottomLeft = outerRadius;
                break;
            case TabsOrientation::VerticalRight:
                peakSide = RectSide::Right;
                peakOuterShape.radii.topRight = outerRadius;
                peakOuterShape.radii.bottomRight = outerRadius;
                break;
        }

        canvas.fillEdgeIndicator(
            peakOuterShape,
            peakSide,
            geometry.indicatorHeight,
            colors.indicator,
            colors.indicatorFactor
        );
    }

}
