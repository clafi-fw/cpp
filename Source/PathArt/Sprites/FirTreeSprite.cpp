module ClaFi.PathArt.FirTreeSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawFirTree(const SceneFrame& frame, const FloatPoint base, const float size,
        const FirTreeColors& colors)
    {
        drawGlow(frame, base, 24.0f * size, faded(colors.shadow, 0.3f), 0.5f);
        const Matrix3x2 tx = Matrix3x2::Translation(base) * Matrix3x2::Scale(size);

        PixelPath path;
        path.moveTo(-10.0f, 0.0f);
        path.lineTo(10.0f, 0.0f);
        path.lineTo(10.0f, -30.0f);
        path.lineTo(-10.0f, -30.0f);
        path.close();
        fillShape(frame, path, SolidColor{ colors.trunk }, tx);

        Hsl hsl{ colors.leaves };
        const float satK = 0.9f;
        const float lumK = 1.0f + std::min(0.16f, (0.95f - hsl.luminosity) * 0.5f);
        for (int tier = 0; tier != 3; ++tier)
        {
            const float i = static_cast<float>(tier);
            const float yO = -20.0f - (i * 30.0f);
            path.clear();
            path.moveTo(-40.0f + (i * 5.0f), yO);
            path.lineTo(40.0f - (i * 5.0f), yO);
            path.lineTo(0.0f, yO - 50.0f);
            path.close();
            fillShape(frame, path, SolidColor{ hsl.toColor() }, tx);
            hsl.scaleSaturation(satK);
            hsl.scaleLuminosity(lumK);
        }
    }
}
