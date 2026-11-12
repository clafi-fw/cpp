export module ClaFi.PathArt.KettlebellAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The android presses a kettlebell overhead and drops it, which jolts the stage. See PathArt
    export class KettlebellAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        [[nodiscard]] float joltAge() const override { return m_landedAge; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        FloatPoint m_bell{}; // the middle of its ball, in the frame
        float m_bellTurn{ 0.0f };
        float m_landedAge{ -1.0f }; // seconds since the bell landed, below 0 before
        bool m_straining{ false };
        bool m_bellHeld{ false };
        float m_restY{ 0.0f }; // the bell's middle as it stands on the floor
        float m_floorY{ 0.0f };
        float m_spotlightX{ 0.0f };
        float m_actTime{ 0.0f };
    };
}
