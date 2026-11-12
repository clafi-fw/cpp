export module ClaFi.PathArt.TopHatAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The android shows a top hat empty, taps it three times with a wand, and a rabbit comes out.
    export class TopHatAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        FloatPoint m_brim{}; // the middle of the hat's brim, in units about the pelvis
        FloatPoint m_brimEdge{}; // where the wand taps, in units about the pelvis
        float m_hatTurn{ 0.0f };
        FloatPoint m_wandTip{}; // in units about the pelvis
        float m_rabbitRise{ 0.0f };
        float m_rabbitLook{ 0.0f };
        float m_spotlightX{ 0.0f };
        float m_actTime{ 0.0f };
    };
}
