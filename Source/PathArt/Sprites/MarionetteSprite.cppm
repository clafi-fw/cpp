export module ClaFi.PathArt.MarionetteSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The marionette's joints at one moment of its dance.
    export struct MarionetteJoints
    {
        FloatPoint pelvis;
        FloatPoint neck;
        FloatPoint head;
        float lean;
        FloatPoint shoulderLeft;
        FloatPoint shoulderRight;
        FloatPoint hipLeft;
        FloatPoint hipRight;
        Limb legLeft;
        Limb legRight;
        Limb armLeft;
        Limb armRight;
    };

    // The wooden control the strings hang from: a main bar and a leg bar hooked under it.
    export struct MarionetteControl
    {
        FloatPoint center;
        FloatPoint endLeft;
        FloatPoint endRight;
        FloatPoint legBarCenter;
        FloatPoint kneeEndLeft;
        FloatPoint kneeEndRight;
    };

    // A marionette and its control at one moment of its dance.
    export struct MarionettePose
    {
        MarionetteJoints joints;
        MarionetteControl control;
    };

    // The marionette dancing, `sway` radians into the dance, its feet about `feet`. See PathArt
    export [[nodiscard]] MarionettePose danceMarionette(float unit, FloatPoint feet, float sway);
    // The strings, the control and the puppet hanging from it.
    export void drawMarionette(const SceneFrame&, const MarionettePose&);
}
