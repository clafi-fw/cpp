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
        // A gear's metal, lit from the top left.
        struct Metal
        {
            Color light;
            Color shade;
        };
        struct Gear
        {
            int teeth;
            FloatPoint center;
            float phase; // the angle tooth 0 points along
            int spokes;
            Metal metal;
            float pitchRadius;
        };
    private:
        // A gear meshing with `driver`, placed at `angle` from it, its teeth in the driver's gaps.
        [[nodiscard]] static Gear meshWith(const Gear& driver, int teeth, float angle, int spokes,
            Metal, float toothSize);
        static void drawPlate(const SceneFrame&, FloatPoint lampAt);
        static void drawGear(const SceneFrame&, const Gear&, float toothSize);
        static void drawEscapeWheel(const SceneFrame&, FloatPoint center, float size, float phase);
        static void drawAnchor(const SceneFrame&, FloatPoint pivot, float swing, float size);
        static void drawPipes(const SceneFrame&, FloatPoint center, float size, float cycleTime);
    };
}
