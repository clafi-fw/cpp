export module ClaFi.PathArt.BallBalanceAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The android rides a circus ball across the stage, arms out, and ends on one leg. See PathArt
    export class BallBalanceAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_ball.x; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        FloatPoint m_ball{}; // its middle, in the frame
        float m_ballTurn{ 0.0f };
        float m_floorY{ 0.0f };
    };
}
