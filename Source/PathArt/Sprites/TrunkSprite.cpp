module ClaFi.PathArt.TrunkSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // Drawn to a stump's measures, 11 units either side of its middle, and scaled from them.
    void drawTrunk(const SceneFrame& frame, const FloatPoint foot, const float halfWidth,
        const float height, const Color bark, const Color barkShade)
    {
        const SceneFrame onFoot = frame.within(Matrix3x2::Translation(foot)
            * Matrix3x2::Scale(halfWidth / 11.0f));
        const float top = -height * 11.0f / halfWidth;
        PixelPath trunk;
        trunk.moveTo(-16.0f, 0.5f);
        trunk.quadTo({ -11.0f, -1.0f }, { -11.0f, -6.0f });
        trunk.lineTo(-11.0f, top);
        trunk.lineTo(11.0f, top);
        trunk.lineTo(11.0f, -6.0f);
        trunk.quadTo({ 11.0f, -1.0f }, { 16.0f, 0.5f });
        trunk.close();
        fillShape(onFoot, trunk, LinearGradient::Simple({ -11.0f, 0.0f }, { 11.0f, 0.0f }, bark,
            barkShade));
        // Seeded by lane and segment, never by place, so a trunk's grooves hold as it scrolls.
        const SolidColor groove{ faded(barkShade, 0.3f) };
        int laneIndex = 0;
        for (const float laneX : { -6.0f, -0.5f, 5.0f })
        {
            float segmentBottom = -2.0f - 3.0f * pseudoRandom(laneIndex, 321.0f);
            int segmentIndex = 0;
            while (segmentBottom > top + 3.0f)
            {
                const int seed = laneIndex * 89 + segmentIndex * 17;
                const float segmentLength = 10.0f + 10.0f * pseudoRandom(seed, 322.0f);
                const float segmentTop = std::max(segmentBottom - segmentLength, top + 1.5f);
                if (segmentBottom - segmentTop < 3.0f)
                    break;
                const float segmentX = std::clamp(laneX + 5.0f * (pseudoRandom(seed, 323.0f)
                    - 0.5f), -9.0f, 9.0f);
                const float swaySide = pseudoRandom(seed, 325.0f) < 0.5f ? -1.0f : 1.0f;
                const float sway = (0.6f + 0.6f * pseudoRandom(seed, 324.0f)) * swaySide;
                const float segmentHeight = segmentBottom - segmentTop;
                PixelPath segment;
                segment.moveTo(segmentX, segmentBottom);
                segment.cubicTo({ segmentX + sway, segmentBottom - segmentHeight * 0.33f },
                    { segmentX - sway, segmentBottom - segmentHeight * 0.66f },
                    { segmentX + 0.3f * sway, segmentTop });
                strokeShape(onFoot, segment, groove, 1.0f);
                segmentBottom = segmentTop - (2.5f + 3.5f * pseudoRandom(seed, 326.0f));
                ++segmentIndex;
            }
            ++laneIndex;
        }
    }
}
