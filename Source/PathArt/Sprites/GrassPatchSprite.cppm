export module ClaFi.PathArt.GrassPatchSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The three shades of a grass patch's blades, back to front.
    export struct GrassPatchColors
    {
        Color dark;
        Color middle;
        Color light;
    };

    // A clump of tall grass rooted on the origin, 33 units tall, bent `bladeLean` radians right.
    export void drawGrassPatch(const SceneFrame&, const GrassPatchColors&, float bladeLean);
}
