export module ClaFi.PathArt.MillScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.ThemeBlend;
import ClaFi.PathArt.Types;
import ClaFi.PathArt.WaterReflection;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    // A watermill by a pond in a valley, in a choice of themes. See PathArt
    export class MillScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Mill"; }
        // **What the scene is painting**, which between two picks is a blend of them and not any
        // theme in the list.
        [[nodiscard]] const SceneTheme& theme() const { return m_themes.worn(); }
        // The theme it was told to wear, blend or no blend - what a caller taking colors or a
        // name off the scene means.
        [[nodiscard]] const SceneTheme& pickedTheme() const { return m_themes.picked(); }
        [[nodiscard]] bool offersThemes() const override { return true; }
        void setTheme(std::size_t index) override { m_themes.pick(index); }
        void wearTheme(std::size_t index) override { m_themes.wear(index); }
        void animationTick() override { m_themes.tick(); }

        // Exact target bounds passed explicitly to anchor layout geometry
        void draw(Canvas& canvas, const FloatRect& bounds, SceneTime time) override;
        void drawIcon(Canvas& canvas, const FloatRect& bounds) const override;
        [[nodiscard]] SceneReadoutColors readoutColors() const override;

    private:
        // Draws m_sky and m_land for the bounds' size in the worn theme.
        void keepStillLayers(const FloatRect& bounds);

        // Render Helpers (Dynamic elements render directly via Canvas&)
        void drawCloud(Canvas& canvas, const Matrix3x2& place, Color color);
    private:
        ThemeBlend m_themes{};
        WaterReflection m_reflection{}; // what the pond mirrors
        Bitmap m_sky{}; // the sky and the sun or the moon
        Bitmap m_land{}; // the mountains, the valley, the river and what stands still on them
        ScaledSize m_stillLayersSize{};
        std::uint64_t m_stillLayersThemeVersion{}; // the worn theme the layers were drawn in
        FloatPoint m_sunPos{};
        FloatPoint m_offset{};
        float m_objectsScale{};
    };
}
