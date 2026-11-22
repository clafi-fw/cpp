export module ClaFi.PathArt.RobotDanceAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The android moonwalks and shuffles under concert lighting. See PathArt
    export class RobotDanceAct : public BotTalentAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_android.pelvis.x; }
        [[nodiscard]] StandPose standPose() const override { return { .shown = 0.0f }; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        FloatPoint m_smokeMachineFoot{};
        float m_floorY{};
        float m_backdropY{};
        float m_blackout{};
        float m_closingLight{};
    };
}
