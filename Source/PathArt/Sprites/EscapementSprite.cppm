export module ClaFi.PathArt.EscapementSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // A pendulum escapement at one moment: its escape wheel, its anchor and its pendulum.
    export struct EscapementPose
    {
        FloatPoint wheelCenter;
        float size; // pixels per unit of the escapement, whose wheel is 62 units in radius
        float wheelPhase; // the angle the wheel's tooth 0 points along
        float swing; // the pendulum's angle off the vertical, in radians
    };

    export constexpr int k_escapeWheelTeeth{ 20 };

    // The escapement `swings` swings into its run: each crossing of the middle lets one tooth go.
    export [[nodiscard]] EscapementPose escapementAt(FloatPoint wheelCenter, float size,
        float swings);
    // The pendulum's rod and bob, which hang behind the wheel.
    export void drawPendulum(const SceneFrame&, const EscapementPose&);
    // The escape wheel and the anchor rocking over it.
    export void drawEscapement(const SceneFrame&, const EscapementPose&);
}
