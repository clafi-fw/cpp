export module ClaFi.PathArt.GatorAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.GatorSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // The android puts its head in a gator's mouth and is out as the jaws snap. See PathArt
    export class GatorAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        GatorPose m_gator{};
        float m_chomp{ -1.0f }; // 0 to 1 over the half second after the snap, below 0 before it
        float m_spotlightX{ 0.0f };
    };
}
