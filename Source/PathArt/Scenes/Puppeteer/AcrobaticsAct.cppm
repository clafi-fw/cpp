export module ClaFi.PathArt.AcrobaticsAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // A side somersault, then over onto the antenna for a headspin and back up. See PathArt
    export class AcrobaticsAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_android.pelvis.x; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        float m_floorY{ 0.0f };
        float m_spinTurns{ 0.0f };
        float m_sparks{ 0.0f }; // how strongly sparks fly from the antenna, 0 for none
    };
}
