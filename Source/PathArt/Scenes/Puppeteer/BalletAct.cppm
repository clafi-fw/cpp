export module ClaFi.PathArt.BalletAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // In a tutu and pointe shoes, the android dances turns, a penche and a leap. See PathArt
    export class BalletAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_android.pelvis.x; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
    };
}
