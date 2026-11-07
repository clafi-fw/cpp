export module ClaFi.PathArt.WolfSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How the wolf stands, runs or lies at one moment.
    export struct WolfPose
    {
        FloatPoint center;
        float size; // pixels per unit of the wolf, which is about 150 units from nose to tail
        float turn; // the whole wolf's turn about its middle, in radians
        float stride; // where the gallop is, in radians; the legs and the tail follow it
        float sprawl; // 0 on its feet, 1 sprawled on its belly
        float onBack; // 0 on its feet, 1 on its back with its legs in the air
        float dazed; // past a half, its eyes are crosses
        float headTurn;
        float tongueTurn;
        float glance; // how far its eye looks down
        float cycleTime; // the time its legs twitch by while it lies on its back
    };

    export void drawWolf(const SceneFrame&, const WolfPose&); // a grey wolf running to the right
}
