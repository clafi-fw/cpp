export module ClaFi.PathArt.PuppeteerScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // An android works a marionette on a small stage. See PathArt
    export class PuppeteerScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Puppeteer"; }
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        static void drawStage(const SceneFrame&);
        static void drawSpotlight(const SceneFrame&, float lampX, float puppetX, float floorY);
        static void drawCurtains(const SceneFrame&, float cycleTime);
    };
}
