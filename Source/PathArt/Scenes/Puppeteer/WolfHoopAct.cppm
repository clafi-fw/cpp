export module ClaFi.PathArt.WolfHoopAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.WolfSprite;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The Chase wolf leaps through a hoop the android holds, then trips on its rim. See PathArt
    export class WolfHoopAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        WolfPose m_wolf{};
        bool m_wolfFacesLeft{ false };
        FloatPoint m_hoop{}; // its middle, in the frame
        float m_dazeTurns{ -1.0f }; // how far the stars have gone round his head, below 0 for none
        float m_spotlightX{ 0.0f };
    };
}
