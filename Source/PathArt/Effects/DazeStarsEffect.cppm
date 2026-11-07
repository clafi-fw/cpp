export module ClaFi.PathArt.DazeStarsEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Three stars circling over a dazed head.
    export struct DazeStars
    {
        FloatPoint center; // the middle of their circle
        float turns; // how far round they have gone, in turns
        Color color{ 0xFFFFD34D };
    };

    export void drawDazeStars(const SceneFrame&, const DazeStars&);
}
