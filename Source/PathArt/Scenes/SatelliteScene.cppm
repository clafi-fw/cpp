export module ClaFi.PathArt.SatelliteScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A satellite over a large turning Earth, through day, dusk and night. See PathArt
    export class SatelliteScene : public Scene
    {
    public:
        SatelliteScene();
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Satellite"; }
        [[nodiscard]] std::chrono::seconds cycleDuration() const override { return k_cycle; }
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        // A direction from the Earth's center: x across the screen, y up it, z toward the viewer.
        struct Direction
        {
            float x;
            float y;
            float z;
        };
        // The Earth as drawn this frame: where it stands, how far it has turned, where the Sun is.
        struct Globe
        {
            FloatPoint center;
            float radius;
            float tilt; // how far north is turned from straight up on the screen
            float spin;
            Direction sun;

            // The point at a longitude and latitude, in degrees.
            [[nodiscard]] Direction surfaceAt(FloatPoint lonLat) const;
            [[nodiscard]] FloatPoint onScreen(Direction) const;
            // The point on the outline at an angle, counterclockwise from across the screen.
            [[nodiscard]] FloatPoint onRim(float angle) const;
            // The near-side part of a closed outline, in loops closed along the rim.
            void nearSideOf(std::span<const Direction> outline, bool counterClockwise,
                std::vector<FloatPoint>& points, std::vector<std::size_t>& loopEnds) const;
            // How much sunlight falls where the surface faces this way: above 0 by day.
            [[nodiscard]] float sunlightAt(Direction) const;
        };
        // The line the day ends at, across the part of the globe in view. See PathArt
        struct Terminator
        {
            FloatPoint through;
            FloatPoint along;
            FloatPoint dayward; // points away from the line into the day

            // Where a point stands from the line: above 0 on the day side.
            [[nodiscard]] float distanceTo(FloatPoint) const;
        };
        struct Coastline
        {
            Color color;
            std::vector<FloatPoint> lonLat;
            bool counterClockwise; // the way its points run, seen from outside the Earth
        };
        struct City
        {
            FloatPoint lonLat;
            float brightness;
        };
        // A lump of cloud; some stand alone and some overlap another of their cluster.
        struct Cloud
        {
            FloatPoint lonLat;
            FloatPoint halfSize; // across and up, in degrees
            float tilt; // off the line of latitude
            std::array<float, 4> phases; // where each of the edge's waves starts
            float driftPhase; // shared by the lumps of one cluster, which drift together
        };
    private:
        static void drawSpace(const SceneFrame&);
        static void drawOcean(const SceneFrame&);
        [[nodiscard]] static Terminator terminatorOf(const SceneFrame&, const Globe&);
        void drawSurface(const SceneFrame&, const Globe&, float cycleTime) const;
        static void drawNight(const SceneFrame&, const Globe&, const Terminator&);
        void drawCityLights(const SceneFrame&, const Globe&, const Terminator&,
            float cycleTime) const;
        static void drawAtmosphere(const SceneFrame&, const Globe&);
    private:
        // The Sun goes round once a cycle, so a cycle is one day over the satellite.
        static constexpr std::chrono::seconds k_cycle{ 240 };
        std::vector<Coastline> m_coastlines{};
        std::vector<City> m_cities{};
        std::vector<Cloud> m_clouds{};
        SceneLayer m_space{ drawSpace };
        SceneLayer m_ocean{ drawOcean };
    };
}
