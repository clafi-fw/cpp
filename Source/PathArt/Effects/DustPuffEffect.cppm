export module ClaFi.PathArt.DustPuffEffect;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A puff of dust thrown up where something lands, spreading and fading as it settles.
    export struct DustPuff
    {
        FloatPoint ground; // the middle of where it landed
        float settle; // 0 as it lands to 1 once the dust has gone
        Color color{ 0xFFEADBB8 };
    };

    export void drawDustPuff(const SceneFrame&, const DustPuff&);
}
