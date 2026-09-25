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
        if (colors.indicatorFactor > 0.0f)
        {
            // The line's outer edge around the peak - its centerline grown by half its width
            const float uOuterStart = uTabStart - halfWidth;
            const float uOuterEnd = uTabEnd + halfWidth;
            const float vOuter = vTop + halfWidth;
            const float outerRadius = actualRadius > 0.0f ? actualRadius + halfWidth : 0.0f;

            PixelPath outerPath;
            outerPath.moveTo(toWorld(uOuterStart, 0.0f));
            outerPath.lineTo(toWorld(uOuterStart, vOuter - outerRadius));
            if (outerRadius > 0.0f)
            {
                outerPath.quadTo(
                    toWorld(uOuterStart, vOuter),
                    toWorld(uOuterStart + outerRadius, vOuter)
                );
            }
            outerPath.lineTo(toWorld(uOuterEnd - outerRadius, vOuter));
            if (outerRadius > 0.0f)
            {
                outerPath.quadTo(
                    toWorld(uOuterEnd, vOuter),
                    toWorld(uOuterEnd, vOuter - outerRadius)
                );
            }
            outerPath.lineTo(toWorld(uOuterEnd, 0.0f));
            outerPath.close();

            // Compute horizontal span of the indicator based on the scaling factor
            const float tabWidth = uOuterEnd - uOuterStart;
            const float indWidth = tabWidth * colors.indicatorFactor;
            const float uIndCenter = (uTabStart + uTabEnd) / 2.0f;
            const float uIndStart = uIndCenter - indWidth / 2.0f;
            const float uIndEnd = uIndCenter + indWidth / 2.0f;

            // Geometry levels
            const float vBottomInd = vOuter - geometry.indicatorHeight;
            const float vTopIndOver = vOuter + halfWidth; // past the edge the clip draws

            // Calculate the corner radius for the indicator (capsule-bottom appearance)
            const float indRadius = std::clamp(geometry.indicatorHeight * 0.5f, 0.0f, indWidth * 0.5f);

            // Construct the bottom-rounded indicator path
            PixelPath indPath;
            indPath.moveTo(toWorld(uIndStart, vTopIndOver));
            indPath.lineTo(toWorld(uIndEnd, vTopIndOver));

            if (indRadius > 0.0f)
            {
                // Line down to bottom-right corner start
                indPath.lineTo(toWorld(uIndEnd, vBottomInd + indRadius));
                // Round bottom-right corner
                indPath.quadTo(
                    toWorld(uIndEnd, vBottomInd),
                    toWorld(uIndEnd - indRadius, vBottomInd)
                );
                // Line across to bottom-left corner start
                indPath.lineTo(toWorld(uIndStart + indRadius, vBottomInd));
                // Round bottom-left corner
                indPath.quadTo(
                    toWorld(uIndStart, vBottomInd),
                    toWorld(uIndStart, vBottomInd + indRadius)
                );
            }
            else
            {
                // Fallback straight lines if radius is zero
                indPath.lineTo(toWorld(uIndEnd, vBottomInd));
                indPath.lineTo(toWorld(uIndStart, vBottomInd));
            }

            indPath.close(); // Automatically connects back to top-left (uIndStart, vTopIndOver)

            // Push the clip mask, paint the capsule indicator, and restore
            canvas.pushClip(outerPath);
            canvas.fillPath(indPath, colors.indicator);
            canvas.popClip();
        }
    }

}
