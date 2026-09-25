export module ClaFi.Icons.OjIcon;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Graphics;

    export class OjIcon
    {
    public:
        // Takes the rect it should fill rather than reading one off a buffer, so the icon is
        // sized by its caller and the canvas transform reaches it through the canvas.
        void paint(Canvas&, const FloatRect& bounds, const Color c) const;
    };


    void OjIcon::paint(Canvas& canvas, const FloatRect& rect, const Color iconColor) const
    {
        // 1. Calculate relative scaling to fit a virtual 100x100 logical grid
        float size = std::min(rect.right - rect.left, rect.bottom - rect.top);
        FloatPoint center = rect.center();

        float scale = size / 100.0f;
        // Direct matrix initialization: uniform scale with origin shift to center
        Matrix3x2 transform = { scale, 0.0f, 0.0f, scale, center.x, center.y };

        float strokeWidth = 9.0f;

        // 2. Draw Outer Rounded Rectangle
        PixelPath outerPath;
        // 76x76 allows the 9.0f stroke to comfortably sit within the 100x100 grid bounds
        outerPath.drawRoundedRect(91.0f, 91.0f, 20.0f);

        PathDrawLayer effects =
            PathDrawLayer{
                {
                    .mode = PathRenderMode::Stroke,
                    .strokeWidth = strokeWidth
                },
                //Graphics::SolidColor{ iconColor }
                Graphics::LinearGradient::simple(
                    { -45, -45 }, { 45, 45 },
                    iconColor, 0xffAA2BDD
                )
            };


        canvas.drawPath(outerPath, { effects }, &transform);

        // 3. Draw Inner 'J'
        // Designed so the exact *ink* bounds (factoring in the StrokeCap::Butt flat cuts)
        // perfectly center visually at X: [-15.5, 15.5] and Y: [-28.0, 28.0].
        PixelPath jPath;
        jPath.moveTo(11.0f, -28.0f);    // Top right flat cut
        jPath.lineTo(11.0f, -3.0f);     // Vertical stem descending

        // Quad curve hooks down and left, creating a flat vertical cut at the tail
        jPath.quadTo({ 11.0f, 23.5f }, { -15.5f, 23.5f });

        effects.geometry.strokeCap = StrokeCap::Butt;
        canvas.drawPath(jPath, { effects }, &transform);
    }
}
