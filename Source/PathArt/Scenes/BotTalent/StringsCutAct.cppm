export module ClaFi.PathArt.StringsCutAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.TopHatSprite;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // In a top hat, the android waves away the strings, the stand, then itself. See PathArt
    export class StringsCutAct : public BotTalentAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_marionetteX; }
        [[nodiscard]] bool usesMarionette() const override { return true; }
        [[nodiscard]] StandPose standPose() const override;
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        MarionettePose m_marionette{};
        MarionetteStand m_stand{};
        float m_marionetteX{ 0.0f };
        float m_twirl{ 1.0f }; // the marionette's facing as it twirls, 1 to -1 and back
        FloatPoint m_wandTip{}; // in units about the pelvis
        TopHatPose m_wornHat{}; // the hat on the head, where the android leaves it in the air
        float m_floorY{ 0.0f };
        float m_actTime{ 0.0f };
    };
}
