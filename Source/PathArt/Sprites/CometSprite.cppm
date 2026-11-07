export module ClaFi.PathArt.CometSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The curve a comet falls along: a quadratic Bezier from where it sets off to where it lands.
    export struct CometCourse
    {
        FloatPoint start;
        FloatPoint control;
        FloatPoint end;
    };

    // A comet at one moment of a pass along its course. See PathArt
    export struct CometPose
    {
        CometCourse course;
        float passTime; // 0 to 1 through the pass
        float fallShare; // the share of the pass it falls for; it lands then and its tails fade
        float cycleTime; // the time its sparks twinkle by
    };

    // The point `along` the course, 0 at its start and 1 at its end.
    export [[nodiscard]] FloatPoint pointOnCourse(const CometCourse&, float along);
    // The comet's head, its dust and ion tails and the sparks it sheds.
    export void drawComet(const SceneFrame&, const CometPose&);
}
