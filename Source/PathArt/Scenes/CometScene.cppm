export module ClaFi.PathArt.CometScene;

import ClaFi.PathArt.CometSprite;
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
        // One pass of the comet: the curve it falls along and how far into the pass it is.
        struct Pass
        {
            CometCourse course;
            // Where the pass stands, 0 to 1: the fall, then the glow behind the ridge.
            float passTime;
            // 0 until the comet lands, then 0 to 1 as the glow behind the ridge fades.
            float afterglow;
        };
    private:
        [[nodiscard]] static float farRidgeY(const SceneFrame&, float x);
        [[nodiscard]] static float nearRidgeY(const SceneFrame&, float x);
        [[nodiscard]] static Pass passAt(const SceneFrame&, float cycleTime);
        // Everything above the lake, which the lake shows again upside down.
        static void drawAboveLake(const SceneFrame&, float cycleTime, float lakeY, bool reflection);
        static void drawLake(const SceneFrame&, float cycleTime, float lakeY);
    };
}
