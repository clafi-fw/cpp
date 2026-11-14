module ClaFi.PathArt.SpruceSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // Each tier is a jagged skirt, every other point drooping, lit on the right and shaded on the
    // left by a wedge from its tip clipped to the skirt.
    void drawSpruce(const SceneFrame& frame, const FloatPoint foot, const float size,
        const SpruceColors& colors)
    {
        constexpr int k_tiers{ 5 };
        constexpr int k_skirtPoints{ 7 };
        const SceneFrame tree = frame.within(Matrix3x2::Translation(foot)
            * Matrix3x2::Scale(size));

        drawSpruceShadow(frame, foot, size, colors.shadow);
        fillBox(tree, { -6.0f, -26.0f, 6.0f, 0.0f }, SolidColor{ colors.trunk });
        fillBox(tree, { -6.0f, -26.0f, -1.0f, 0.0f }, SolidColor{ colors.trunkShade });

        const SolidColor needles{ colors.needles };
        const SolidColor sunlit{ faded(colors.sunlit, 0.75f) };
        const SolidColor shaded{ faded(colors.shaded, 0.8f) };
        const Matrix3x2 toCanvas = tree.toCanvas;
        for (int i = 0; i != k_tiers; ++i)
        {
            const float height = static_cast<float>(i) / static_cast<float>(k_tiers - 1);
            const float bottom = -18.0f - static_cast<float>(i) * 24.0f;
            const float top = bottom - 46.0f + height * 6.0f;
            const float half = 46.0f + (16.0f - 46.0f) * height;

            PixelPath skirt;
            skirt.moveTo(0.0f, top);
            for (int k = 0; k != k_skirtPoints; ++k)
            {
                const float across = static_cast<float>(k)
                    / static_cast<float>(k_skirtPoints - 1);
                const float x = half - 2.0f * half * across;
                const float droop = k % 2 == 0 ? 6.0f : 0.0f;
                skirt.lineTo(x, bottom + droop - std::abs(x) * 0.05f);
            }
            skirt.close();
            fillShape(tree, skirt, needles);

            frame.canvas.pushClip(skirt, &toCanvas);
            PixelPath wedge;
            wedge.moveTo(0.0f, top);
            wedge.lineTo(half + 4.0f, bottom + 8.0f);
            wedge.lineTo(half * 0.25f, bottom + 8.0f);
            wedge.close();
            fillShape(tree, wedge, sunlit);
            wedge.clear();
            wedge.moveTo(0.0f, top);
            wedge.lineTo(-half - 4.0f, bottom + 8.0f);
            wedge.lineTo(-half * 0.45f, bottom + 8.0f);
            wedge.close();
            fillShape(tree, wedge, shaded);
            frame.canvas.popClip();
        }
    }

    void drawSpruceShadow(const SceneFrame& frame, const FloatPoint foot, const float size,
        const Color shadow)
    {
        if (shadow.alpha == 0)
            return;
        const SceneFrame tree = frame.within(Matrix3x2::Translation(foot)
            * Matrix3x2::Scale(size));
        fillOval(tree, { 4.0f, 0.0f }, 40.0f, 7.0f, SolidColor{ shadow });
    }
}
