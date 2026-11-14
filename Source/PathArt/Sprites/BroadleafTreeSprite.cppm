export module ClaFi.PathArt.BroadleafTreeSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    export struct BroadleafTreeColors
    {
        Color crown; // the lowest clumps
        Color crownMiddle;
        Color crownLight; // the clumps the sun reaches
        Color crownShade; // under the lowest clumps
        Color trunk;
        Color trunkShade;
        Color shadow;
    };

    // A broadleaf tree of nine clumps, 129 units tall, standing on `foot` over its shadow.
    export void drawBroadleafTree(const SceneFrame&, FloatPoint foot, float size,
        const BroadleafTreeColors&);
}
