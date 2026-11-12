export module ClaFi.PathArt.GatorSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How a whole gator lies on the floor, facing right, its head lifted and its jaws apart.
    export struct GatorPose
    {
        FloatPoint neck; // the floor under its neck
        float size; // pixels per unit of the gator, which is 460 units from tail to snout
        float headLift{ 0.0f }; // the head's turn up about the neck, in radians
        float gape{ 0.0f }; // the upper jaw's turn up about the back of the mouth, in radians
        float tailDrop{ 0.0f }; // how far the tip of its tail hangs below its rest, in units
        float glance{ 0.0f }; // how far its pupil looks toward its snout, in units
        float lid{ 0.0f }; // 0 its eye open, 1 shut
    };

    // A point of the head, in the gator's units as it lies with its head down, placed in the frame.
    export [[nodiscard]] FloatPoint gatorHeadToFrame(const GatorPose&, FloatPoint headPoint);
    // A point of the upper jaw as it lies shut, placed in the frame. See PathArt
    export [[nodiscard]] FloatPoint gatorUpperJawToFrame(const GatorPose&, FloatPoint jawPoint);
    // A point midway between the open jaws, `depth` units out from the back of the mouth.
    export [[nodiscard]] FloatPoint gatorMouthPoint(const GatorPose&, float depth);
    // The gator but its upper jaw: what lies behind a head put in its mouth.
    export void drawGator(const SceneFrame&, const GatorPose&);
    // The upper jaw with the eye, and both rows of teeth, over a head put in the mouth.
    export void drawGatorUpperJaw(const SceneFrame&, const GatorPose&);
}
