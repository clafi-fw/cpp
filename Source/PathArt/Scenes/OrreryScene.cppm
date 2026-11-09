export module ClaFi.PathArt.OrreryScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // The Sun, six planets and an asteroid belt on a tilted orbit plane. See PathArt
    export class OrreryScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return k_name; }
        [[nodiscard]] std::chrono::seconds cycleDuration() const override { return k_cycle; }
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
        void drawIcon(Graphics::Canvas&, const FloatRect& bounds) const override;
        [[nodiscard]] std::span<const std::wstring_view> switches() const override;
        [[nodiscard]] bool isSwitchOn(std::size_t index) const override;
        void setSwitch(std::size_t index, bool on) override;
        void storeView(Dom::Section& view) const override;
        void restoreView(const Dom::Section& view) override;
        // The config section the switch is kept in, under the scene's name. See PathArt
        [[nodiscard]] static Dom::Dt::Section viewSchema();
    private:
        static void drawSpace(const SceneFrame&);
        static void drawFarOrbits(const SceneFrame&);
        static void drawNearOrbits(const SceneFrame&);
        static void drawOrbits(const SceneFrame&, bool frontHalf);
    private:
        static constexpr std::wstring_view k_name{ L"Orrery" };
        // Saturn's one orbit a cycle sets the pace of all of them.
        static constexpr std::chrono::seconds k_cycle{ 240 };
        SceneLayer m_space{ drawSpace };
        SceneLayer m_farOrbits{ drawFarOrbits };
        SceneLayer m_nearOrbits{ drawNearOrbits };
        bool m_orbitsShown{ true };
    };
}
