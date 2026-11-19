export module ClaFi.PathArt.TutuSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How one tutu differs from another of the same cut: its color, its size, its set.
    export struct TutuLook
    {
        Color tint{ 0xFFFFFFFF }; // the color every part of it leans toward, by `tintShare`
        float tintShare{ 0.0f };
        float size{ 1.0f };
        float tilt{ 0.0f }; // in radians, about the middle of the hips
    };

    // A classical tutu in units about the middle of the hips, its pleats turned by `turn` radians.
    export void drawTutu(const SceneFrame& hips, float turn, const TutuLook& = {});
}
