module ClaFi.PathArt.FogEffect;

import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // Each puff crosses the frame and 300 units either side of it, where its glow fades out.
    void drawFog(const SceneFrame& frame, const float cycleTime, const FogBank& bank)
    {
        const float unit = frame.unit;
        for (int i = 0; i != bank.puffs; ++i)
        {
            const float start = static_cast<float>(i) / static_cast<float>(bank.puffs) + bank.start;
            const float drift = fractionalPart(cycleTime * bank.driftsPerCycle + start);
            const float x = drift * (frame.width + 600.0f * unit) - 300.0f * unit;
            drawGlow(frame, { x, bank.y }, 70.0f * unit, bank.color, 0.22f);
        }
    }
}
