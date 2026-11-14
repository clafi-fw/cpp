export module ClaFi.PathArt.WolfHeadSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the wolf's head is and how it shows.
    export struct WolfHeadPose
    {
        FloatPoint at; // the middle of its head, a little below its eyes
        float size; // pixels per unit of the head, which is 55 units from nose to nape
        bool eyesOpen;
        float opacity;
    };

    export struct WolfHeadColors
    {
        Color head;
        Color ears; // the ears and the nose
        Color eyes; // and the glow round them
    };

    // A wolf's head looking left, its eyes glowing.
    export void drawWolfHead(const SceneFrame&, const WolfHeadPose&, const WolfHeadColors&);
}
