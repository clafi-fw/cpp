export module ClaFi.PathArt.BouquetSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The colour of the roses in a bouquet.
    export enum class BouquetRoses
    {
        Red,
        Pink,
        Yellow,
        White
    };

    // A bouquet of roses in a paper cone tied with a ribbon; a turn of 0 holds it flowers up.
    export struct BouquetPose
    {
        FloatPoint middle;
        float turn{ 0.0f };
        float size{ 1.0f }; // pixels per unit of the bouquet, which is about 50 units long
        BouquetRoses roses{ BouquetRoses::Red };
    };

    export void drawBouquet(const SceneFrame&, const BouquetPose&);
}
