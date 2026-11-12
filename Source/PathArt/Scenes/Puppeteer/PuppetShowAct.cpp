module ClaFi.PathArt.PuppetShowAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // The android bobs on each step and holds the control from the left, its arm reaching up.
    void PuppetShowAct::pose(const ActStage& stage, const float, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float sway = danceSway(time);
        m_marionetteX = marionetteX(stage, sway);
        m_marionette = danceMarionette(unit, { m_marionetteX, stage.floorY }, sway);

        const float bob = -2.5f * std::abs(std::sin(2.0f * sway));
        m_android = standingAndroid(frame, { puppeteerX(stage), stage.floorY }, time.cycleTime,
            bob);
        m_android.headTurn = -0.1f + 0.06f * std::sin(sway);
        m_android.look = -5.0f + 2.0f * std::sin(sway);
        const FloatPoint grip = m_marionette.control.center - FloatPoint{ 0.0f, 9.0f * unit };
        AndroidSide& holding = m_android.left;
        holding.hand = androidFrameToUpper(frame, m_android, grip);
        holding.upperArm = 87.0f;
        holding.forearm = 87.0f;
        holding.elbow = ElbowBend::Down;
        holding.handShape = HandShape::Fist;
        holding.forearmOnTop = true;
    }

    void PuppetShowAct::draw(const SceneFrame& frame) const
    {
        drawAndroid(frame, m_android);
        drawMarionette(frame, m_marionette);
        drawAndroidForearm(frame, m_android, AudienceSide::Left);
    }
}
