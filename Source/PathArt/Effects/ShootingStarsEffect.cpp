module ClaFi.PathArt.ShootingStarsEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // Each flies 160 units with a tail of 50, brightening and fading over its flight.
    void drawShootingStars(const SceneFrame& frame, const float cycleTime,
        const ShootingStars& stars)
    {
        const float unit = frame.unit;
        const FloatPoint heading = stars.heading;
        for (int i = 0; i != stars.count; ++i)
        {
            const float startTime = (static_cast<float>(i) + 0.35f)
                / static_cast<float>(stars.count) + 0.05f * pseudoRandom(i, 9.0f);
            const float flight = (cycleTime - startTime) / stars.flightShare;
            if (flight < 0.0f || flight > 1.0f)
                continue;
            const FloatPoint from = {
                frame.width * (0.15f + 0.7f * pseudoRandom(i, 7.0f)),
                std::lerp(stars.top, stars.bottom, pseudoRandom(i, 8.0f))
            };
            const FloatPoint head = from + heading * (160.0f * unit * flight);
            const FloatPoint tail = head - heading * (50.0f * unit);
            const float brightness = 0.9f * std::sin(std::numbers::pi_v<float> * flight);
            drawSegment(frame, head, tail, LinearGradient::Simple(head, tail,
                stars.color.withOpacity(brightness), stars.color.withOpacity(0.0f)), 1.6f * unit);
        }
    }
}
