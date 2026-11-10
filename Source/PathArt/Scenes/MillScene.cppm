export module ClaFi.PathArt.MillScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.ThemeBlend;
import ClaFi.PathArt.Types;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Cpu_PathPainter;
import ClaFi.Core.Graphics.Types;
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
        [[nodiscard]] std::wstring_view name() const override { return L"Mill"; }
        // **What the scene is painting**, which between two picks is a blend of them and not any
        // theme in the list.
        [[nodiscard]] const SceneTheme& theme() const { return m_themes.worn(); }
        // The theme it was told to wear, blend or no blend - what a caller taking colours or a
        // name off the scene means.
        [[nodiscard]] const SceneTheme& pickedTheme() const { return m_themes.picked(); }
        [[nodiscard]] bool offersThemes() const override { return true; }
        void setTheme(std::size_t index) override { m_themes.pick(index); }
        void animationTick() override { m_themes.tick(); }

        // Exact target bounds passed explicitly to anchor layout geometry
        void draw(Canvas& canvas, const FloatRect& bounds, SceneTime time) override;
        void drawIcon(Canvas& canvas, const FloatRect& bounds) const override;
        [[nodiscard]] SceneReadoutColors readoutColors() const override;

    private:
        void prepareBackground(float w, float h);

        // Render Helpers (Dynamic elements render directly via Canvas&)
        void drawWaterFoam(Canvas& canvas, float x, float y, float mS, float t) const;
        void drawSunCorona(Canvas& canvas, float x, float y, float r, float t);
        void drawCloud(Canvas& canvas, float x, float y, float s);
        float drawRiver(Canvas& canvas, FloatPoint off, float w, float h, float t);

        // Background Static Helper (Accepts PixelPathPainter for offline bitmap caching)
        void drawHills(PixelPathPainter& p, FloatPoint off, float w, float h);
    private:
        ThemeBlend m_themes{};
        Bitmap m_background{};
        ScaledSize m_backgroundDimensions{};
        std::uint64_t m_backgroundThemeVersion{}; // the worn theme the background was drawn in
        FloatPoint m_sunPos;
        float m_sunRadius;
        FloatPoint m_offset;
        float m_objectsScale;
    };
}
