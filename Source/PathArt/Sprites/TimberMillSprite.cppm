export module ClaFi.PathArt.TimberMillSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the mill stands and how far its moving parts have gone.
    export struct TimberMillPose
    {
        FloatPoint base; // the middle of the front wall's foot
        float size; // pixels per unit of the mill, whose front wall is 112 units wide
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
        Color timber; // the frame, the trough and the wings' stocks
        Color bark; // the door
        Color barkShade; // the door's planks and the wheel's hub
        Color stoneLight; // the plinth and the chimney
        Color stoneDark;
        Color stoneCrack;
        Color sunlight; // what warms the side wall and the spokes
        Color glass; // a window with no lamp behind it
        Color lamp; // behind the windows, as strong as its opacity
        Color shadow; // under the eaves and the barge boards
        Color wingFabric;
        Color wheel;
        Color water; // what the trough pours
        Color river; // over the wheel's foot
        Color foam;
        Color smoke;
    };

    // The chimney's smoke, which the house stands in front of.
    export void drawTimberMillSmoke(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
    // The timber framed house on its stone plinth, its right side in view.
    export void drawTimberMillHouse(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
    // The water wheel at the house's left corner, its foot in the river.
    export void drawTimberMillWheel(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
    // The trough over the wheel, the water it pours, and the river churning round the wheel.
    export void drawTimberMillWater(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
    // The four lattice wings on their hub, 126 units above the foot.
    export void drawTimberMillWings(const SceneFrame&, const TimberMillPose&,
        const TimberMillColors&);
}
