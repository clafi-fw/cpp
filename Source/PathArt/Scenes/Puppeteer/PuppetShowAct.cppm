export module ClaFi.PathArt.PuppetShowAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // The android works the marionette through its dance. See PathArt
    export class PuppetShowAct : public PuppeteerAct
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
    };
}
