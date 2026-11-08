export module ClaFi.PathArt.SatelliteScene;

import ClaFi.PathArt.CloudMap;
import ClaFi.PathArt.EarthMap;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
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
        [[nodiscard]] bool offersLooks() const override { return true; }
    private:
        // A direction from the Earth's center: x across the screen, y up it, z toward the viewer.
        struct Direction
        {
            float x;
            float y;
            float z;
        };
        // How the Earth stands turned this frame, worked out once for the coasts' many points.
        struct EarthTurn
        {
            float spinCosine;
            float spinSine;
            float tipCosine; // of the tip that leans the north pole toward the viewer
            float tipSine;

            // A direction fixed to the Earth, as it stands this frame.
            [[nodiscard]] Direction applied(Direction onEarth) const;
        };
        // The Earth as drawn this frame: where it stands, how far it has turned, where the Sun is.
        struct Globe
        {
            FloatPoint center;
            float radius;
            float tilt; // how far north is turned from straight up on the screen
            float spin;
            Direction sun;

            // The direction to a longitude and latitude in degrees, fixed to the Earth.
            [[nodiscard]] static Direction onEarthAt(FloatPoint lonLat);
            [[nodiscard]] EarthTurn turn() const;
            // The point at a longitude and latitude, in degrees.
            [[nodiscard]] Direction surfaceAt(FloatPoint lonLat) const;
            [[nodiscard]] FloatPoint onScreen(Direction) const;
            // The map from a direction's x and y to the screen.
            [[nodiscard]] Graphics::Matrix3x2 toScreen() const;
            // The near side of a closed outline as directions' x and y, in loops closed at the rim.
            void nearSideOf(std::span<const Direction> outline, bool counterClockwise,
                std::vector<FloatPoint>& points, std::vector<std::size_t>& loopEnds) const;
            // Adds the near side of a closed outline to a path, answering whether it is in view.
            bool addNearSide(std::span<const Direction> outline, bool counterClockwise,
                const FloatRect& view, Graphics::PixelPath&) const;
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
            std::vector<FloatPoint> lonLat;
            std::vector<Direction> onEarth; // each point's direction, fixed to the Earth
            bool counterClockwise; // the way its points run, seen from outside the Earth
            bool iceSheet; // Greenland's, drawn white until the land's map is built, and townless
        };
        struct City
        {
            FloatPoint lonLat;
            float brightness;
        };
        // The coastlines and the towns spread evenly over them, built together off the paint.
        struct BuiltCoasts
        {
            std::vector<Coastline> coastlines;
            std::vector<City> towns;
        };
        // The land's map and the towns placed by it, built together off the paint.
        struct BuiltLand
        {
            // By pointer for the same reason as the clouds' maps.
            std::unique_ptr<EarthMap> map;
            std::vector<City> towns;
        };
        // A band of rows of a picture worked out by pixel, and where it stands in the canvas's.
        struct PixelBand
        {
            Graphics::Bitmap pixels;
            FloatPoint at;
        };
        // The map of each layer of cloud, in the order the layers float up from the ground.
        using CloudMaps = std::array<CloudMap, 3>;
    private:
        [[nodiscard]] static BuiltCoasts buildCoasts();
        // Towns spread evenly over the land, until the land's map places them.
        [[nodiscard]] static std::vector<City> evenTowns(std::span<const Coastline>);
        // Towns where the land's map makes them likely. See PathArt#satellitescene
        [[nodiscard]] static std::vector<City> townsBy(const EarthMap&,
            std::span<const Coastline>);
        static void drawSpace(const SceneFrame&);
        static void drawOcean(const SceneFrame&);
        [[nodiscard]] static Terminator terminatorOf(const SceneFrame&, const Globe&);
        // The land in one flat colour inside each coastline, until the land's map is built.
        void drawFlatLand(const SceneFrame&, const Globe&) const;
        // The land worked out pixel by pixel inside the coastlines. See PathArt#satellitescene
        void drawLand(const SceneFrame&, const Globe&);
        [[nodiscard]] static Graphics::LinearGradient nightOf(const Globe&, const Terminator&);
        void drawCityLights(const SceneFrame&, const Globe&, const Terminator&,
            float cycleTime) const;
        void drawClouds(const SceneFrame&, const Globe&, const Graphics::LinearGradient& night,
            float cycleTime);
        static void drawAtmosphere(const SceneFrame&, const Globe&);
        // Lays the bands in at the canvas's pixels, whatever its transform, cut to `cut` there.
        static void layBands(const SceneFrame&, std::span<const PixelBand>, const FloatRect& cut);
    private:
        // The Sun goes round once a cycle, so a cycle is one day over the satellite.
        static constexpr std::chrono::seconds k_cycle{ 240 };
        std::vector<Coastline> m_coastlines{};
        std::vector<City> m_cities{};
        // Started by the constructor, and taken by the first frame drawn.
        std::future<BuiltCoasts> m_coastsBuilt{};
        SceneLayer m_space{ drawSpace };
        SceneLayer m_ocean{ drawOcean };
        // By pointer, as MSVC's std::future builds what it carries with no arguments first.
        std::future<std::unique_ptr<CloudMaps>> m_cloudMapsBuilt{};
        // Built off the paint from the first frame drawn on, and kept. See PathArt#satellitescene
        std::unique_ptr<CloudMaps> m_cloudMaps{};
        // Kept from one frame to the next, so a frame of the same size allocates nothing.
        std::vector<PixelBand> m_cloudBands{};
        std::future<BuiltLand> m_landBuilt{};
        // Built off the paint from the first frame drawn on, and kept. See PathArt#satellitescene
        std::unique_ptr<EarthMap> m_earthMap{};
        // Kept from one frame to the next, as the clouds' are.
        std::vector<PixelBand> m_landBands{};
    };
}
