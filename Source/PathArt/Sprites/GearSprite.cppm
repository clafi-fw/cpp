export module ClaFi.PathArt.GearSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A gear's metal, lit from the top left.
    export struct GearMetal
    {
        Color light;
        Color shade;
    };

    // A gear where it stands and how far it has turned.
    export struct Gear
    {
        int teeth;
        FloatPoint center;
        float phase; // the angle tooth 0 points along
        int spokes; // with none the gear is a plain disc with a ring
        GearMetal metal;
        float pitchRadius;
    };

    // A gear meshing with `driver`, placed at `angle` from it, its teeth in the driver's gaps.
    export [[nodiscard]] Gear meshWith(const Gear& driver, int teeth, float angle, int spokes,
        GearMetal, float toothSize);
    export void drawGear(const SceneFrame&, const Gear&, float toothSize);
}
