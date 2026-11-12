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
        float headTilt{ 0.0f }; // the head's tilt beyond the lean, as a limp puppet's droops
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
    // The marionette hung limp by its control from `hook`, swung `swing` radians. See PathArt
    export [[nodiscard]] MarionettePose hangMarionette(float unit, FloatPoint hook, float swing);
    // The marionette on its own feet at `feet`, its strings gone, `time` seconds in. See PathArt
    export [[nodiscard]] MarionetteJoints standingMarionette(float unit, FloatPoint feet,
        float curtsy, float wave, float time);
    // The strings, the control and the puppet hanging from it.
    export void drawMarionette(const SceneFrame&, const MarionettePose&);
    // The strings alone, at a share of their own opacity.
    export void drawMarionetteStrings(const SceneFrame&, const MarionettePose&, float opacity);
    // The control alone, at a share of its opacity.
    export void drawMarionetteControl(const SceneFrame&, const MarionetteControl&, float opacity);
    // The puppet alone, without its strings or control.
    export void drawMarionetteFigure(const SceneFrame&, const MarionetteJoints&);
}
