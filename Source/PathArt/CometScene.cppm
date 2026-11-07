export module ClaFi.PathArt.CometScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A comet falls across the night sky and lands behind the mountains. See PathArt
    export class CometScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Comet"; }
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        // One pass of the comet: the curve it falls along and how far along it is.
        struct Pass
        {
            FloatPoint start;
            FloatPoint control;
            FloatPoint end;
            // Where the pass stands, 0 to 1: the fall, then the glow behind the ridge.
            float passTime;
            float along; // how far along the curve the head is; 1 once it has landed
            // 0 until the comet lands, then 0 to 1 as the glow behind the ridge fades.
            float afterglow;
        };
    private:
        [[nodiscard]] static float farRidgeY(const SceneFrame&, float x);
        [[nodiscard]] static float nearRidgeY(const SceneFrame&, float x);
        [[nodiscard]] static FloatPoint pointOnCurve(const Pass&, float along);
        [[nodiscard]] static Pass passAt(const SceneFrame&, float cycleTime);
        // Everything above the lake, which the lake shows again upside down.
        static void drawAboveLake(const SceneFrame&, float cycleTime, float lakeY, bool reflection);
        static void drawShootingStars(const SceneFrame&, float cycleTime);
        static void drawCometAndTails(const SceneFrame&, const Pass&, float cycleTime);
        static void drawLake(const SceneFrame&, float cycleTime, float lakeY);
    };
}
