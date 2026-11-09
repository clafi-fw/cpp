export module ClaFi.PathArt.OrreryScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // The Sun, six planets and an asteroid belt on a tilted orbit plane. See PathArt
    export class OrreryScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Orrery"; }
        [[nodiscard]] std::chrono::seconds cycleDuration() const override { return k_cycle; }
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
        void drawIcon(Graphics::Canvas&, const FloatRect& bounds) const override;
    private:
        static void drawSpace(const SceneFrame&);
        static void drawFarOrbits(const SceneFrame&);
        static void drawNearOrbits(const SceneFrame&);
        static void drawOrbits(const SceneFrame&, bool frontHalf);
    private:
        // Saturn's one orbit a cycle sets the pace of all of them.
        static constexpr std::chrono::seconds k_cycle{ 240 };
        SceneLayer m_space{ drawSpace };
        SceneLayer m_farOrbits{ drawFarOrbits };
        SceneLayer m_nearOrbits{ drawNearOrbits };
    };
}
