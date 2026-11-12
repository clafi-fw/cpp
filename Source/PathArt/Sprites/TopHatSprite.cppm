export module ClaFi.PathArt.TopHatSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // Where a top hat is, how it is turned and which way up.
    export struct TopHatPose
    {
        FloatPoint brim; // the middle of its brim
        float turn{ 0.0f };
        float size{ 1.0f }; // pixels per unit of the hat, whose brim is 76 units across
        bool upsideDown{ false }; // its crown below the brim and the inside showing
    };

    // The hat's own frame in its units, the middle of its brim at 0, 0 and its crown above.
    export [[nodiscard]] SceneFrame topHatFrame(const SceneFrame&, const TopHatPose&);
    // The hat but the near lip of its brim. See PathArt
    export void drawTopHat(const SceneFrame&, const TopHatPose&);
    // The near lip of the brim, over the head the hat sits on or what comes up out of it.
    export void drawTopHatBrimLip(const SceneFrame&, const TopHatPose&);
}
