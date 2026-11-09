export module ClaFi.PathArt.GhostScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
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
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
        void drawIcon(Graphics::Canvas&, const FloatRect& bounds) const override;
        [[nodiscard]] SceneReadoutColors readoutColors() const override;
    private:
        // The near ground's top edge at x.
        [[nodiscard]] static float groundY(const SceneFrame&, float x);
        static void drawSky(const SceneFrame&);
        static void drawMoon(const SceneFrame&);
        static void drawClouds(const SceneFrame&, float cycleTime);
        static void drawBats(const SceneFrame&, float cycleTime);
        static void drawLand(const SceneFrame&);
        static void drawBareTree(const SceneFrame&, FloatPoint root);
        static void drawFence(const SceneFrame&);
        static void drawTombstones(const SceneFrame&);
    private:
        SceneLayer m_sky{ drawSky };
        SceneLayer m_land{ drawLand };
        SceneLayer m_moon{ drawMoon };
        SceneLayer m_tombstones{ drawTombstones };
    };
}
