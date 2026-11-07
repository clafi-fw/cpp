export module ClaFi.PathArt.GhostScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A ghost loops a figure eight over a moonlit graveyard. See PathArt
    export class GhostScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Ghost"; }
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        // The near ground's top edge at x.
        [[nodiscard]] static float groundY(const SceneFrame&, float x);
        static void drawSky(const SceneFrame&, float cycleTime);
        static void drawBat(const SceneFrame&, FloatPoint center, float size, float flap);
        static void drawBareTree(const SceneFrame&, FloatPoint root);
        static void drawFence(const SceneFrame&);
        static void drawTombstones(const SceneFrame&);
        static void drawPumpkin(const SceneFrame&, float cycleTime);
        static void drawGhost(const SceneFrame&, FloatPoint center, float size, float cycleTime);
    };
}
