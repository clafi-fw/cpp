export module ClaFi.PathArt.JugglingAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // The android juggles three balls in a cascade, its eyes on the highest. See PathArt
    export class JugglingAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_android.pelvis.x; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        std::array<FloatPoint, 3> m_balls{}; // in units about the pelvis
    };
}
