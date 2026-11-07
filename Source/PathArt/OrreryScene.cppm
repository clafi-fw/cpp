export module ClaFi.PathArt.OrreryScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // The Sun, six planets and an asteroid belt on a tilted orbit plane. See PathArt
    export class OrreryScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Orrery"; }
        [[nodiscard]] std::chrono::seconds cycleDuration() const override { return k_cycle; }
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        enum class Planet
        {
            Mercury,
            Venus,
            Earth,
            Mars,
            Jupiter,
            Saturn
        };
        struct Orbit
        {
            Planet planet;
            float radiusShare; // the orbit's radius as a share of the outermost one's
            float orbitsPerCycle;
            float startTurn; // where the planet stands at the start of the cycle, in turns
            float planetRadius; // the planet's radius in scene units
            Color color;
        };
        // Which way the light comes from: across the screen, down it, and toward the viewer.
        struct LightDirection
        {
            float x;
            float y;
            float toViewer;
        };
        // A planet where it stands at the moment drawn.
        struct PlacedPlanet
        {
            const Orbit* orbit;
            FloatPoint center;
            float depth; // nearer the viewer above 0, beyond the Sun below it
            float radius;
            LightDirection light;
        };
    private:
        static void drawSky(const SceneFrame&, float cycleTime);
        static void drawSun(const SceneFrame&, FloatPoint center, float cycleTime);
        static void drawOrbits(const SceneFrame&, FloatPoint center, float outerRadius,
            bool frontHalf);
        static void drawAsteroids(const SceneFrame&, FloatPoint center, float outerRadius,
            bool frontHalf, float cycleTime);
        static void drawPlanet(const SceneFrame&, const PlacedPlanet&, float cycleTime);
        static void drawSaturnRings(const SceneFrame&, const PlacedPlanet&, bool frontHalf);
        static void drawMoon(const SceneFrame&, const PlacedPlanet& earth, float cycleTime);
        // Darkens the half of a ball turned from the light. See PathArt
        static void shadeBall(const SceneFrame&, FloatPoint center, float radius, LightDirection,
            Color shade);
    private:
        // Saturn's one orbit a cycle sets the pace of all of them.
        static constexpr std::chrono::seconds k_cycle{ 240 };
    };
}
