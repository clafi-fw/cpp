export module ClaFi.PathArt.RobotDanceAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // The android dances through robot poses, sends a wave along its arms and spins. See PathArt
    export class RobotDanceAct : public BotTalentAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_android.pelvis.x; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
    };
}
