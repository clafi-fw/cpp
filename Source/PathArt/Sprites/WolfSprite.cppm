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
        float leap; // 0 on its feet, 1 stretched out in a leap, forelegs reaching
        float sprawl; // 0 on its feet, 1 sprawled on its belly
        float onBack; // 0 on its feet, 1 on its back with its legs in the air
        float dangle; // 0 as the rest has it, 1 its legs and tail hanging straight down
        float dazed; // past a half, its eyes are crosses
        float headTurn;
        float tongueTurn;
        float jawOpen; // 0 shut, 1 gaping, its teeth out
        float glance; // how far its eye looks down
        float cycleTime; // the time its legs twitch by while it lies on its back
    };

    // The colours of a wolf's fur.
    export struct WolfCoat
    {
        Color light; // its head and snout, and its back
        Color shade; // its ears and lower jaw, and its flank
        Color coat; // its near legs and its tail
        Color far; // its far legs, and the pads of the near ones
        Color dark; // its tail's tip, and the pads of the far legs
        Color belly;
    };

    // A wolf running to the right, in the coat given.
    export void drawWolf(const SceneFrame&, const WolfPose&, const WolfCoat&);
}
