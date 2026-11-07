export module ClaFi.PathArt.HareSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How the hare runs or sits at one moment.
    export struct HarePose
    {
        FloatPoint foot; // where its hind foot is
        float size; // pixels per unit of the hare, which is about 70 units long
        float facing; // 1 facing right, -1 facing left; between them it turns
        float air; // 0 on the ground, 1 at the top of a hop
        float stretch;
        float bodyTurn;
        float sitting;
        float earTurn;
        float earWiggle;
        float footLift;
    };

    export void drawHare(const SceneFrame&, const HarePose&);
}
