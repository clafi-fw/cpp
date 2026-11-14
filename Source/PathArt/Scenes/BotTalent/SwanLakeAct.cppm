export module ClaFi.PathArt.SwanLakeAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // Four androids in tutus dance the little swans, hands joined, almost together. See PathArt
    export class SwanLakeAct : public BotTalentAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        [[nodiscard]] StandPose standPose() const override { return { .shown = 0.0f }; }
        void draw(const SceneFrame&) const override;
    private:
        using Dancers = std::array<AndroidPose, 4>;
    private:
        Dancers m_dancers{}; // from the audience's left
        float m_spotlightX{ 0.0f };
    };
}
