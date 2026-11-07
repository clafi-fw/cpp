export module ClaFi.PathArt.ClockworkScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
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
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        static void drawPlate(const SceneFrame&, FloatPoint lampAt);
        static void drawPipes(const SceneFrame&, FloatPoint center, float size, float cycleTime);
    };
}
