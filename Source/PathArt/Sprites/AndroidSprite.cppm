export module ClaFi.PathArt.AndroidSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How the android stands at one moment of its dance.
    export struct AndroidPose
    {
        FloatPoint feet; // between its feet, on the floor
        float sway; // the dance's sway in radians; the body dips twice a sway, the head turns once
        FloatPoint grip; // where its left hand holds what it works
        float cycleTime; // the time its eyes blink and its lights flash by
    };

    // The android and its left upper arm, reaching for the grip.
    export void drawAndroid(const SceneFrame&, const AndroidPose&);
    // The android's left forearm and hand, drawn over what it holds.
    export void drawAndroidHand(const SceneFrame&, const AndroidPose&);
}
