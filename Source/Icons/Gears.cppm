export module ClaFi.Icons.Gears;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Icons
{
    using namespace Graphics;

    // Canvas::drawPath has no whole-path opacity the way PixelPathPainter did, so the half
    // opacity these two were drawn at is carried by the layer colours instead. That double
    // blends where the stroke overlaps the fill, which is what a single composite avoided.
    export void drawSettingsGear(Canvas& canvas, FloatPoint center, float rotation, int numTeeth = 8)
    {
        const float innerRadius = 30.0f;
        const float outerRadius = 45.0f;
        const float toothWidthFactor = 0.2f;

        PixelPath path{};

        path.clear();

        const float angleStep = (2.0f * 3.14159265f) / static_cast<float>(numTeeth);

        for (int i = 0; i < numTeeth; ++i) {
            float baseAngle = i * angleStep;
            float a1 = baseAngle;
            float a2 = baseAngle + angleStep * toothWidthFactor;
            float a3 = baseAngle + angleStep * (0.5f - toothWidthFactor);
            float a4 = baseAngle + angleStep * 0.5f;
            path.lineTo({ innerRadius * std::cos(a1), innerRadius * std::sin(a1) });
            path.lineTo({ outerRadius * std::cos(a2), outerRadius * std::sin(a2) });
            path.lineTo({ outerRadius * std::cos(a3), outerRadius * std::sin(a3) });
            path.lineTo({ innerRadius * std::cos(a4), innerRadius * std::sin(a4) });
        }
        path.close();

        Matrix3x2 transform =
            Matrix3x2::translation(center)
            * Matrix3x2::scale(2.0f)
            * Matrix3x2::rotation(rotation * k_2Pi);
        canvas.drawPath(
            path,
            {
                PathDrawLayer::fill(Color{ 0xFF445566 }.withOpacity(0.5f)), // Sleek blue-grey
                PathDrawLayer::stroke(Color{ 0xFFAABBCC }.withOpacity(0.5f), 3.5f) // Steel highlight
            },
            &transform
        );
    }

    // There's a error in the path painter - it doesn't close the path properly
    export void drawBezierGear(Canvas& canvas, FloatPoint center, float scaleFactor, float rotation, int numTeeth = 6)
    {
        PixelPath path{};
        path.clear();

        const float innerR = 30.0f;
        const float outerR = 45.0f;
        const float holeR = 15.0f;
        const float angleStep = k_2Pi / numTeeth;

        // 1. Draw the gear teeth with curves
        for (int i = 0; i < numTeeth; ++i) {
            float a1 = i * angleStep;
            float a3 = a1 + angleStep * 0.3f;  // tooth top-left
            float a4 = a1 + angleStep * 0.7f;  // tooth top-right
            float a5 = a1 + angleStep * 0.8f;  // tooth ends

            // Move along the inner radius
            path.lineTo({ innerR * std::cos(a1), innerR * std::sin(a1) });

            // Curve UP to the tooth top
            path.quadTo(
                { outerR * std::cos(a1), outerR * std::sin(a1) }, // Control point at corner
                { outerR * std::cos(a3), outerR * std::sin(a3) }  // Target top edge
            );

            // Flat top of tooth
            path.lineTo({ outerR * std::cos(a4), outerR * std::sin(a4) });

            // Curve DOWN back to the inner circle
            path.quadTo(
                { outerR * std::cos(a5), outerR * std::sin(a5) }, // Control point at corner
                { innerR * std::cos(a5), innerR * std::sin(a5) }  // Target inner edge
            );
        }

        // 2. Add the center hole (XOR magic)
        // Adding points in a circle inside the path will "cut" it out
        // 2. The Hole (lift the pen!)
        for (int i = 0; i <= 32; ++i) {
            float alpha = i * (k_2Pi / 32.0f);
            FloatPoint p = { holeR * std::cos(alpha), holeR * std::sin(alpha) };

            if (i == 0) path.moveTo(p); // LIFT THE PEN
            else path.lineTo(p);
        }

        path.close();

        Matrix3x2 transform =
            Matrix3x2::translation(center)
            * Matrix3x2::scale(2.0f * scaleFactor)
            * Matrix3x2::rotation(rotation * k_2Pi);
        canvas.drawPath(
            path,
            {
                PathDrawLayer::fill(Color{ 0xFF445566 }.withOpacity(0.5f)), // Sleek blue-grey
                PathDrawLayer::stroke(Color{ 0xFFAABBCC }.withOpacity(0.5f), 3.5f) // Steel highlight
            },
            &transform);
    }

}
