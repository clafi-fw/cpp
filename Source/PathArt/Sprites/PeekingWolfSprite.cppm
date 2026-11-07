export module ClaFi.PathArt.PeekingWolfSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the wolf peeks from and when it is drawn.
    export struct PeekingWolfPose
    {
        FloatPoint at; // the top of its snout
        float size; // pixels per unit of the wolf
        float cycleTime; // it breathes twice a cycle; its ears twitch and its eyes blink by it
        float opacity;
    };

    export struct PeekingWolfColors
    {
        Color body;
        Color ear;
        Color eye;
    };

    // A wolf's head and a hind leg, peeking out from behind cover.
    export void drawPeekingWolf(const SceneFrame&, const PeekingWolfPose&,
        const PeekingWolfColors&);
}
