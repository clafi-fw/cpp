module ClaFi.PathArt.BroadleafTreeSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // One of the crown's clumps, and which of the crown's tones it wears.
        struct Clump
        {
            FloatPoint center;
            float radius;
            int tone; // 0 the crown, 1 its middle, 2 its light
        };

        // Bottom to top, so the lighter clumps lie over the darker.
        constexpr std::array<Clump, 9> k_clumps{ {
            { { -26.0f, -58.0f }, 26.0f, 0 },
            { { 24.0f, -60.0f }, 25.0f, 0 },
            { { 0.0f, -70.0f }, 30.0f, 0 },
            { { -14.0f, -92.0f }, 26.0f, 1 },
            { { 18.0f, -88.0f }, 24.0f, 1 },
            { { 2.0f, -108.0f }, 22.0f, 1 },
            { { 12.0f, -100.0f }, 15.0f, 2 },
            { { 26.0f, -76.0f }, 13.0f, 2 },
            { { -4.0f, -118.0f }, 11.0f, 2 }
        } };
    }

    void drawBroadleafTree(const SceneFrame& frame, const FloatPoint foot, const float size,
        const BroadleafTreeColors& colors)
    {
        const SceneFrame tree = frame.within(Matrix3x2::Translation(foot)
            * Matrix3x2::Scale(size));
        fillOval(tree, { 4.0f, 0.0f }, 46.0f, 8.0f, SolidColor{ colors.shadow });

        PixelPath trunk;
        trunk.moveTo(-6.0f, 0.0f);
        trunk.lineTo(-4.0f, -46.0f);
        trunk.lineTo(4.0f, -46.0f);
        trunk.lineTo(7.0f, 0.0f);
        trunk.close();
        fillShape(tree, trunk, SolidColor{ colors.trunk });
        fillBox(tree, { -6.0f, -46.0f, -2.0f, 0.0f }, SolidColor{ colors.trunkShade });

        const std::array<Color, 3> tones{ colors.crown, colors.crownMiddle, colors.crownLight };
        for (const Clump& clump : k_clumps)
        {
            const Color tone = tones[static_cast<std::size_t>(clump.tone)];
            fillDisc(tree, clump.center, clump.radius, SolidColor{ tone });
        }

        // One path for both, so where they overlap the shade is no darker.
        PixelPath shade;
        addEllipse(shade, { -30.0f, -52.0f }, 16.0f, 16.0f);
        addEllipse(shade, { -6.0f, -50.0f }, 14.0f, 14.0f);
        fillShape(tree, shade, SolidColor{ colors.crownShade });
    }
}
