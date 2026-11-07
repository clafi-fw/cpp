export module ClaFi.PathArt.OrreryScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
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
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        static void drawSky(const SceneFrame&, float cycleTime);
        static void drawOrbits(const SceneFrame&, FloatPoint center, float outerRadius,
            bool frontHalf);
    private:
        // Saturn's one orbit a cycle sets the pace of all of them.
        static constexpr std::chrono::seconds k_cycle{ 240 };
    };
}
