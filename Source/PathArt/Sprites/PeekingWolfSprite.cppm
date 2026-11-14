export module ClaFi.PathArt.PeekingWolfSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the wolf peeks from and when it is drawn.
    export struct PeekingWolfPose
    {
        FloatPoint at; // the top of its snout
        FloatPoint hip; // where its hind leg starts, reaching 25 units right and 16 down from it
        float size; // pixels per unit of the wolf
        float cycleTime; // it breathes twice a cycle
        bool eyesOpen;
        float earTurn; // how far its left ear is turned up from its rest, in turns
        float opacity;
    };

    export struct PeekingWolfColors
    {
        Color body;
        Color ear;
        Color eye; // and the glow round the eyes
    };

    // A wolf's head and a hind leg, peeking out from behind cover, its eyes glowing.
    export void drawPeekingWolf(const SceneFrame&, const PeekingWolfPose&,
        const PeekingWolfColors&);
}
