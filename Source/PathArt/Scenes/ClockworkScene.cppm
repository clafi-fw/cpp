export module ClaFi.PathArt.ClockworkScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A pendulum escapement ticks a train of brass and copper gears. See PathArt
    export class ClockworkScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Clockwork"; }
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        static void drawPlate(const SceneFrame&);
        static void drawPipes(const SceneFrame&);
        static void drawGauge(const SceneFrame&);
        static void drawGaugeNeedle(const SceneFrame&, float cycleTime);
    private:
        SceneLayer m_plate{ drawPlate };
        SceneLayer m_pipes{ drawPipes };
        SceneLayer m_gauge{ drawGauge };
    };
}
