export module ClaFi.PathArt.AccordionAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // The android squeezes an accordion, swaying and tapping a foot, notes drifting up.
    export class AccordionAct : public BotTalentAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        float m_bellowsHalfWidth{ 30.0f }; // in units
        float m_tilt{ 0.0f };
        float m_spotlightX{ 0.0f };
        float m_actTime{ 0.0f };
    };
}
