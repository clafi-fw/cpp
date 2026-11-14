export module ClaFi.PathArt.BucketWheelSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where the water wheel stands and how far it has turned.
    export struct BucketWheelPose
    {
        FloatPoint center;
        float size; // pixels per unit of the wheel, whose rim is 92 units across
        float turns; // counterclockwise
    };

    export struct BucketWheelColors
    {
        Color wood; // the rims, and darker the buckets and the far rim
        Color spokes;
        Color hub;
        Color axle;
    };

    // Two rims on eight spokes and eighteen buckets round the outer one, the far rim behind.
    export void drawBucketWheel(const SceneFrame&, const BucketWheelPose&,
        const BucketWheelColors&);
}
