export module ClaFi.PathArt.ChaseScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.ThemeBlend;
import ClaFi.PathArt.Types;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A wolf chases a hare across a scrolling landscape, and keeps falling over. See PathArt
    export class ChaseScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return k_name; }
        [[nodiscard]] std::chrono::seconds cycleDuration() const override { return k_cycle; }
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
        void drawIcon(Graphics::Canvas&, const FloatRect& bounds) const override;
        [[nodiscard]] SceneReadoutColors readoutColors() const override;
        [[nodiscard]] bool offersThemes() const override { return true; }
        void setTheme(std::size_t index) override { m_themes.pick(index); }
        void animationTick() override { m_themes.tick(); }
        void storeView(Dom::Section& view) const override;
        void restoreView(const Dom::Section& view) override;
        // The config section the next run's first cycle is kept in, under the scene's name.
        [[nodiscard]] static Dom::Dt::Section viewSchema();
    private:
        using TileDrawer = std::function<void(float tileLeft, float tileWidth)>;
    private:
        [[nodiscard]] static float travelAt(float cycleTime);
        // A period that the whole cycle's travel divides, near the one wanted. See PathArt
        [[nodiscard]] static float periodNear(float parallax, float wanted);
        static void forEachTile(const SceneFrame&, float parallax, float period, float travel,
            const TileDrawer&);
        // The sky with the theme's stars, the sun, the clouds, the mountains, the far hills and the
        // theme's fireflies over them.
        static void drawBackdrop(const SceneFrame&, const SceneTheme&, float cycleTime,
            float travel, double journey, float groundY);
        static void drawGround(const SceneFrame&, const SceneTheme&, float travel, float groundY);
        static void drawFrontGrass(const SceneFrame&, const SceneTheme&, float travel);
        // The near edge's row, or the far edge's between its runs' own, evenly spaced. See PathArt
        static void drawVergeRow(const SceneFrame&, const SceneTheme&, SceneTime, float groundY,
            float closeness, bool nearEdge);
    private:
        static constexpr std::wstring_view k_name{ L"Chase" };
        // Three 20-second runs: two over obstacles, then the one into the branch.
        static constexpr std::chrono::seconds k_cycle{ 60 };
        static constexpr std::wstring_view k_nextCycleNode{ L"NextCycle" };
        ThemeBlend m_themes{}; // what the scene is drawn in
        // Cycles the story stands ahead of the host's, where the last run left it. See PathArt
        std::int64_t m_cyclesAhead{};
        std::int64_t m_nextCycle{}; // the story's cycle after the last one drawn
    };
}
