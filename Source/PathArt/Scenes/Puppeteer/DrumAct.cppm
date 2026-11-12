export module ClaFi.PathArt.DrumAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // The android plays a snare drum at its waist: a stick twirl, a roll and a crash on a cymbal.
    export class DrumAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        float m_leftStick{ 0.0f }; // the sticks' angles, in the upper body's frame
        float m_rightStick{ 0.0f };
        float m_struck{ 0.0f }; // 1 the moment a stick strikes the head, falling to 0
        float m_crashAge{ -1.0f }; // seconds since the cymbal was struck, below 0 before
        float m_floorY{ 0.0f };
        float m_spotlightX{ 0.0f };
        float m_actTime{ 0.0f };
    };
}
