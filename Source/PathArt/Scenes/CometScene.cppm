export module ClaFi.PathArt.CometScene;

import ClaFi.PathArt.CometSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
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
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
        void drawIcon(Graphics::Canvas&, const FloatRect& bounds) const override;
        [[nodiscard]] SceneReadoutColors readoutColors() const override;
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
        // The frame turned over about the lake's edge, as the lake shows what is above it.
        [[nodiscard]] static SceneFrame reflectionOf(const SceneFrame&);
        // Everything above the lake, which the lake shows again upside down.
        void drawAboveLake(const SceneFrame& upright, float cycleTime, bool reflection) const;
        void drawLake(const SceneFrame&, float cycleTime) const;
        static void drawSky(const SceneFrame&);
        static void drawFarRidge(const SceneFrame&);
        static void drawNearRidge(const SceneFrame&);
        static void drawMirroredFarRidge(const SceneFrame&);
        static void drawMirroredNearRidge(const SceneFrame&);
        static void drawLakeWater(const SceneFrame&);
        static void drawLakeShade(const SceneFrame&);
        static void drawShore(const SceneFrame&);
    private:
        SceneLayer m_farRidge{ drawFarRidge };
        SceneLayer m_nearRidge{ drawNearRidge };
        SceneLayer m_lakeWater{ drawLakeWater };
        SceneLayer m_mirroredFarRidge{ drawMirroredFarRidge };
        SceneLayer m_mirroredNearRidge{ drawMirroredNearRidge };
        SceneLayer m_lakeShade{ drawLakeShade };
        SceneLayer m_shore{ drawShore };
    };
}
