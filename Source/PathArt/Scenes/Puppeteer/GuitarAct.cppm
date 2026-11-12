export module ClaFi.PathArt.GuitarAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;

namespace ClaFi::PathArt
{
    // The android strums a guitar through four chords, the neck raised for the last bars.
    export class GuitarAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        float m_neckAngle{ 0.0f };
        float m_ringing{ 0.0f }; // how far the strings still shake from the last stroke, 0 to 1
        float m_spotlightX{ 0.0f };
        float m_actTime{ 0.0f };
    };
}
