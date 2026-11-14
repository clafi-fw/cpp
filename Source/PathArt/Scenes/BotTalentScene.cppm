export module ClaFi.PathArt.BotTalentScene;

import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // An android puts on a show on a small stage, a curtain closing between its acts. See PathArt
    export class BotTalentScene : public Scene
    {
    public:
        BotTalentScene();
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Bot Talent"; }
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
        void drawIcon(Graphics::Canvas&, const FloatRect& bounds) const override;
        [[nodiscard]] SceneReadoutColors readoutColors() const override;
    private:
        using Acts = std::vector<std::unique_ptr<BotTalentAct>>;
    private:
        static void drawStage(const SceneFrame&);
        static void drawSpotlight(const SceneFrame&, float lampX, float targetX, float floorY);
        static void drawCurtain(const SceneFrame&, float closure, float closingSpeed,
            float cycleTime);
        static void drawDrapes(const SceneFrame&, float cycleTime, float swing);
        static void drawValance(const SceneFrame&);
    private:
        SceneLayer m_stage{ drawStage };
        SceneLayer m_valance{ drawValance };
        Acts m_acts{};
    };
}
