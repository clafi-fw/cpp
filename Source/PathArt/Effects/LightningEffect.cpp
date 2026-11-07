module ClaFi.PathArt.LightningEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // A strike lasts 0.15 of its third of the cycle, flickering as it fades; each strike's bolt
    // takes its own path, picked by the strike's number.
    void drawLightning(const SceneFrame& frame, const float cycleTime, const Lightning& lightning)
    {
        if (lightning.strength <= 0.001f)
            return;
        const float strikePhase = cycleTime * 3.0f;
        const float flashT = std::fmod(strikePhase, 1.0f);
        if (flashT > 0.15f)
            return;

        const float w = frame.width;
        const float h = frame.height;
        const float unit = frame.unit;
        const float seed = std::floor(strikePhase);
        const float flicker = (std::sin(flashT * 150.0f) > 0.0f) ? 1.0f : 0.0f;
        const float fade = 1.0f - (flashT / 0.15f);

        // Soft sky flash
        const float opacityFill = lightning.strength * flicker * fade * 0.15f;
        fillBox(frame, { 0.0f, 0.0f, w, h },
            SolidColor{ lightning.color.withOpacity(opacityFill) });

        // Lightning bolt path generation
        const float opacityBolt = lightning.strength * flicker * fade;
        PixelPath path;
        float lx = w * (0.3f + 0.4f * pseudoRandom(1, seed));
        float ly = 0.0f;
        path.moveTo(lx, ly);
        for (int i = 1; i <= 6; ++i)
        {
            lx += (pseudoRandom(i, seed) * 120.0f - 60.0f) * unit;
            ly += h * 0.1f + pseudoRandom(i + 10, seed) * h * 0.1f;
            path.lineTo(lx, ly);

            if (pseudoRandom(i + 20, seed) > 0.5f)
            {
                const float bx = lx + (pseudoRandom(i + 30, seed) * 100.0f - 50.0f) * unit;
                const float by = ly + h * 0.15f + pseudoRandom(i + 40, seed) * h * 0.05f;
                path.lineTo(bx, by);
                path.moveTo(lx, ly);
            }
        }

        const Color boltColor = lightning.color.withOpacity(opacityBolt);
        frame.canvas.drawPath(
            path,
            {
                PathDrawLayer::Stroke(boltColor, 3.0f * unit),
                PathDrawLayer::OuterGlow(boltColor, 20.0f * unit)
            },
            &frame.toCanvas
        );
    }
}
