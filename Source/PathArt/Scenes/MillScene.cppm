export module ClaFi.PathArt.MillScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Cpu_PathPainter;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;
    using namespace ::ClaFi::Graphics::Cpu;

    // A watermill landscape in a choice of themes, crossfading from one to the next.
    export class MillScene : public Scene
    {
    public:
        MillScene();
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Mill"; }
        std::size_t themeIndex() const { return m_themeIndex; }
        // **What the scene is painting**, which between two picks is a blend of them and not any
        // theme in the list.
        const SceneTheme& theme() const { return m_theme; }
        // The theme it was told to wear, blend or no blend - what a caller taking colours or a
        // name off the scene means.
        const SceneTheme& pickedTheme() const { return Themes::allThemes[m_themeIndex]; }
        void setTheme(std::size_t index);
        void animationTick();

        // Exact target bounds passed explicitly to anchor layout geometry
        void draw(Canvas& canvas, const FloatRect& bounds, float t) override;

    private:
        void crossFadeToPickedTheme();
        void prepareBackground(float w, float h);

        // Render Helpers (Dynamic elements render directly via Canvas&)
        void drawWaterFoam(Canvas& canvas, float x, float y, float mS, float t) const;
        void drawSunCorona(Canvas& canvas, float x, float y, float r, float t);
        void drawCloud(Canvas& canvas, float x, float y, float s);
        float drawRiver(Canvas& canvas, FloatPoint off, float w, float h, float t);

        // Background Static Helper (Accepts PixelPathPainter for offline bitmap caching)
        void drawHills(PixelPathPainter& p, FloatPoint off, float w, float h);
    private:
        std::size_t m_themeIndex{ 0 };
        // **NULL until the constructor fades the first theme in** - see MillScene::MillScene.
        SceneTheme m_theme{};
        SceneTheme m_prevTheme{};
        const SceneTheme* m_nextTheme{};
        Bitmap m_background{};
        ScaledSize m_backgroundDimensions{};
        FloatPoint m_sunPos;
        float m_sunRadius;
        FloatPoint m_offset;
        float m_objectsScale;
        AnimationController m_animator{};
    };
}
