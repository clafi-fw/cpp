export module ClaFi.PathArt.TimberMillSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the mill stands and how far its moving parts have gone.
    export struct TimberMillPose
    {
        FloatPoint base; // the middle of the front wall's foot
        float size; // pixels per unit of the mill, whose tower is 116 units across at its foot
        float wingTurns; // clockwise
        float wheelTurns; // counterclockwise
        float cycleTime; // the smoke's and the falling water's, 0 to 1
    };

    export struct TimberMillColors
    {
        Color bodyTop;
        Color bodyBottom;
        Color roofTop;
        Color roofBottom;
        Color timber; // the window and door frames, the trough and the wings' stocks
        Color bark; // the door
        Color barkShade; // the door's planks and the wheel's hub
        Color stoneLight; // the plinth and the chimney
        Color stoneDark;
        Color stoneCrack;
        Color sunlight; // what warms the wheel's spokes
        Color glass; // a window with no lamp behind it
        Color lamp; // behind the windows, as strong as its opacity
        Color shadow; // under the eaves
        Color wingFabric;
        Color wheel;
        Color water; // what the trough pours
        Color foam;
        Color smoke;
    };

    // The chimney's smoke, which the house stands in front of.
    export void drawTimberMillSmoke(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
    // The barrel of a tower on its stone plinth, the chimney behind its cone of a roof.
    export void drawTimberMillHouse(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
    // The trough and the water wheel left of the house, as much as the house and the river show.
    export void drawTimberMillWheel(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
    // The water the trough pours, the steam where it lands, and the foam at the wheel's foot.
    export void drawTimberMillWater(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
    // The four sails on their hub, 126 units above the foot.
    export void drawTimberMillWings(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
}
