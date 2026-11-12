export module ClaFi.PathArt.StringsCutAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // In a top hat, the android waves the strings away and the marionette dances on. See PathArt
    export class StringsCutAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_marionetteX; }
        [[nodiscard]] bool usesMarionette() const override { return true; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        MarionettePose m_marionette{};
        float m_marionetteX{ 0.0f };
        float m_twirl{ 1.0f }; // the marionette's facing as it twirls, 1 to -1 and back
        FloatPoint m_wandTip{}; // in units about the pelvis
        float m_actTime{ 0.0f };
    };
}
