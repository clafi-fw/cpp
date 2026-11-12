export module ClaFi.PathArt.TutuSprite;

import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // A classical tutu in units about the middle of the hips, its pleats turned by `turn` radians.
    export void drawTutu(const SceneFrame& hips, float turn);
}
