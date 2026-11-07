export module ClaFi.PathArt.JackOLanternSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the jack-o'-lantern sits and when its candle is drawn.
    export struct JackOLanternPose
    {
        FloatPoint base; // the middle of its bottom
        float cycleTime; // the time its candle flickers by
    };

    // A carved pumpkin lit by a flickering candle.
    export void drawJackOLantern(const SceneFrame&, const JackOLanternPose&);
}
