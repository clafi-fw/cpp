module ClaFi.PathArt.FallingLeavesEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // Each leaf falls the frame's height two or three times a cycle, swaying as it goes.
    void drawFallingLeaves(const SceneFrame& frame, const float cycleTime,
        const FallingLeaves& leaves)
    {
        const float opacity = leaves.opacity;
        if (opacity <= 0.001f)
            return;

        const float t = cycleTime;
        const float w = frame.width;
        const float h = frame.height;
        const float unit = frame.unit;
        PixelPath leaf;
        leaf.moveTo(0.0f, -10.0f);
        leaf.lineTo(8.0f, 0.0f);
        leaf.lineTo(0.0f, 10.0f);
        leaf.lineTo(-8.0f, 0.0f);
        leaf.close();
        const float s = unit * 0.65f;
        for (int i = 0; i < leaves.count; ++i)
        {
            const float xBase = pseudoRandom(i) * w;
            const float loopCount = static_cast<float>(2 + (i % 2));
            const float y = std::fmod(pseudoRandom(i, 4.0f) * h + t * (loopCount * h), h);
            const float drift = std::sin(t * k_2Pi * 1.0f + static_cast<float>(i)) * 60.0f * unit;
            const Color color = i % 2 == 0 ? leaves.color1 : leaves.color2;
            const Matrix3x2 tx = Matrix3x2::Translation(xBase + drift, y)
                * Matrix3x2::Rotation(calculateSyncRotation(t, i, 2.0f) * k_2Pi)
                * Matrix3x2::Scale(s - leaves.petals * 0.5f);
            fillShape(frame, leaf, SolidColor{ color.withOpacity(opacity) }, tx);
        }
    }
}
