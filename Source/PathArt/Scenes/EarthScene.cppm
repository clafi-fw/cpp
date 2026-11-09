export module ClaFi.PathArt.EarthScene;

import ClaFi.PathArt.CloudMap;
import ClaFi.PathArt.EarthMap;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A large Earth turning under a satellite, through day, dusk and night. See PathArt
    export class EarthScene : public Scene
    {
    public:
        EarthScene();
    public:
        [[nodiscard]] std::wstring_view name() const override { return k_name; }
        [[nodiscard]] std::chrono::seconds cycleDuration() const override { return k_cycle; }
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
        void drawIcon(Graphics::Canvas&, const FloatRect& bounds) const override;
        [[nodiscard]] bool offersLooks() const override { return true; }
        [[nodiscard]] bool offersView() const override { return true; }
        [[nodiscard]] bool dragMovesViewFrom(FloatPoint start,
            const FloatRect& bounds) const override;
        void dragView(FloatPoint by, const FloatRect& bounds) override;
        void scrollView(float notches) override;
        void holdView(bool held) override;
        void letGoView(FloatPoint speed, const FloatRect& bounds) override;
        void clickView(const FloatRect&) override;
        void doubleClickView(const FloatRect&) override;
        [[nodiscard]] std::span<const SceneGesture> gestures() const override;
        void storeView(Dom::Section& view) const override;
        void restoreView(const Dom::Section& view) override;
        // The config section the view is kept in, under the scene's name. See PathArt#earthscene
        [[nodiscard]] static Dom::Dt::Section viewSchema();
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
        // A turn by `angle` about an axis of length 1, anticlockwise seen from its tip.
        struct AxisTurn
        {
            Direction axis;
            float angle;
        };
        // A rotation, as where it takes each of the three axes.
        struct Rotation
        {
            Direction x;
            Direction y;
            Direction z;

            // The rotation by `angle` about an axis of length 1, anticlockwise seen from its tip.
            [[nodiscard]] static Rotation About(Direction axis, float angle);
            [[nodiscard]] Direction applied(Direction) const;
            // This rotation, turning after `first`.
            [[nodiscard]] Rotation after(const Rotation& first) const;
            // The axes set square and of length 1 again, as rounding wears them with each turn.
            [[nodiscard]] Rotation orthonormal() const;
            // The axis and the angle About would take to make this rotation.
            [[nodiscard]] AxisTurn axisTurn() const;
            // The rotation that undoes this one.
            [[nodiscard]] Rotation inverse() const;
        };
        // The spin about the Earth's axis, its tip toward the viewer, then the tilt in the screen.
        struct ViewAngles
        {
            float tilt;
            float tip;
            float spin;

            // The angles of a turn from the Earth's axes to the view's. See PathArt#earthscene
            [[nodiscard]] static ViewAngles Of(const Rotation& fromEarth);
        };
        // Where the view stands besides the orbit, moved by drags and the wheel.
        struct View
        {
            // How the drags have turned the orbit about the Earth, among the stars and the Sun.
            Rotation orbitTurn{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
            float altitude{ 0.4f }; // 0 at the lowest, 1 with the whole globe in the frame
            // The orbits the satellite stands at beyond those the globe's time makes; all, hanging.
            double orbitsAhead{};
            // How far the view is turned about the vertical from the track running across it.
            float trackAngle{};
            // Whether the satellite hangs over one point of the ground rather than orbiting.
            bool hanging{};
            // Where the view stands on the Earth while it hangs, before it leans back.
            Rotation hangingCamera{
                { 1.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f }
            };
        };
        // The Earth as drawn this frame: where it stands, how far it has turned, where the Sun is.
        struct Globe
        {
            FloatPoint center;
            float radius;
            float tilt; // how far north is turned from straight up on the screen
            float tip; // how far the north pole leans toward the viewer
            float spin;
            Direction sun;
            Rotation fromEarth; // where the Earth's axes stand in the view

            // The turned Earth and Sun as the view's axes see them. See PathArt#earthscene
            [[nodiscard]] static Globe FromCamera(FloatPoint center, float radius,
                const Rotation& camera, double earthTurns, double sunTurns);
            // Where the view's axes stand among the stars at a point of the orbit, before any drag.
            [[nodiscard]] static Rotation orbitAxesAt(double orbits, float altitude,
                float trackAngle);
            // Where the Earth's axes stand among the stars after a number of turns.
            [[nodiscard]] static Rotation earthAxesAt(double earthTurns);
            // The direction to a longitude and latitude in degrees, fixed to the Earth.
            [[nodiscard]] static Direction onEarthAt(FloatPoint lonLat);
            [[nodiscard]] EarthTurn turn() const;
            [[nodiscard]] FloatPoint onScreen(Direction) const;
            // The map from a direction's x and y to the screen.
            [[nodiscard]] Graphics::Matrix3x2 toScreen() const;
            // A closed outline's near side as directions' x and y, in loops closed at `rimRadius`.
            void nearSideOf(std::span<const Direction> outline, bool counterClockwise,
                float rimRadius, std::vector<FloatPoint>& points,
                std::vector<std::size_t>& loopEnds) const;
            // Adds the near side of a closed outline to a path, answering whether it is in view.
            bool addNearSide(std::span<const Direction> outline, bool counterClockwise,
                const FloatRect& view, Graphics::PixelPath&) const;
            // How much sunlight falls where the surface faces this way: above 0 by day.
            [[nodiscard]] float sunlightAt(Direction) const;
        };
        struct Coastline
        {
            std::vector<FloatPoint> lonLat;
            std::vector<Direction> onEarth; // each point's direction, fixed to the Earth
            bool counterClockwise; // the way its points run, seen from outside the Earth
            bool iceSheet; // Greenland's or Antarctica's: white until the map is built, townless
        };
        struct City
        {
            Direction onEarth; // where it stands, fixed to the Earth
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
        // Towns where the land's map makes them likely. See PathArt#earthscene
        [[nodiscard]] static std::vector<City> townsBy(const EarthMap&,
            std::span<const Coastline>);
        static void drawSpace(const SceneFrame&);
        static void drawOcean(const SceneFrame&);
        // The land in one flat colour inside each coastline, until the land's map is built.
        void drawFlatLand(const SceneFrame&, const Globe&) const;
        // The land worked out pixel by pixel inside the coastlines. See PathArt#earthscene
        void drawLand(const SceneFrame&, const Globe&);
        // The sea coloured pixel by pixel by its depth, once the land's map is built.
        void drawSea(const SceneFrame&, const Globe&);
        // The night worked out pixel by pixel from the sunlight. See PathArt#earthscene
        void drawNight(const SceneFrame&, const Globe&);
        void drawCityLights(const SceneFrame&, const Globe&, float cycleTime) const;
        // The clouds drift on the scene's time, so they go on drifting over a globe held still.
        void drawClouds(const SceneFrame&, const Globe&, double sceneSeconds);
        static void drawAtmosphere(const SceneFrame&, const Globe&);
        // Lays the bands in at the canvas's pixels, whatever its transform, cut to `cut` there.
        static void layBands(const SceneFrame&, std::span<const PixelBand>, const FloatRect& cut);
        // The scene's seconds since the last frame, 0 for the first.
        [[nodiscard]] double secondsSinceLastFrame(double sceneSeconds);
        // How long the globe has stood still by a frame, while a press held the view.
        [[nodiscard]] double stillSecondsAfter(double frameSeconds);
        // Glides the Sun as drawn toward where the double clicks have moved it.
        void moveShownSun(double frameSeconds);
        // How many turns the Earth has made by the globe's time, counted in orbits.
        [[nodiscard]] double earthTurnsAt(double timeInOrbits) const;
        // Where the view's axes stand among the stars: on the orbit, or hanging over the ground.
        [[nodiscard]] Rotation cameraAt(double orbits, double earthTurns) const;
        // The globe under the view's held point moved by `offset`, as a direction in the view.
        [[nodiscard]] Direction heldPointAt(FloatPoint offset, const FloatRect& bounds) const;
        // The track angle that carries the ground under the held point the way `speed` goes.
        [[nodiscard]] float trackAngleFor(FloatPoint speed, const FloatRect& bounds) const;
        // Hangs the satellite over the ground in the view the last frame showed.
        void hangSatellite();
        // Flies the satellite on at `trackAngle`, its orbit laid on the view the last frame showed.
        void flySatelliteOn(float trackAngle);
        // A rotation's config section under `key`: an axis and an angle in degrees, `start`'s.
        [[nodiscard]] static Dom::Dt::Section rotationSchema(std::wstring_view key,
            const Rotation& start);
        static void storeRotation(Dom::Section& section, const Rotation& rotation);
        // The rotation a section holds, or `fallback` where its axis has no length.
        [[nodiscard]] static Rotation restoredRotation(const Dom::Section& section,
            const Rotation& fallback);
    private:
        // The twinkling and blinking come round each cycle; the orbit and the cloud drift never do.
        static constexpr std::chrono::seconds k_cycle{ 240 };
        static constexpr std::wstring_view k_name{ L"Earth" };
        std::vector<Coastline> m_coastlines{};
        std::vector<City> m_cities{};
        // Started by the constructor, and taken by the first frame drawn.
        std::future<BuiltCoasts> m_coastsBuilt{};
        SceneLayer m_space{ drawSpace };
        SceneLayer m_ocean{ drawOcean };
        // By pointer, as MSVC's std::future builds what it carries with no arguments first.
        std::future<std::unique_ptr<CloudMaps>> m_cloudMapsBuilt{};
        // Built off the paint from the first frame drawn on, and kept. See PathArt#earthscene
        std::unique_ptr<CloudMaps> m_cloudMaps{};
        // Kept from one frame to the next, so a frame of the same size allocates nothing.
        std::vector<PixelBand> m_cloudBands{};
        std::future<BuiltLand> m_landBuilt{};
        // Built off the paint from the first frame drawn on, and kept. See PathArt#earthscene
        std::unique_ptr<EarthMap> m_earthMap{};
        // Kept from one frame to the next, as the clouds' are.
        std::vector<PixelBand> m_landBands{};
        std::vector<PixelBand> m_seaBands{};
        std::vector<PixelBand> m_nightBands{};
        View m_view{};
        // The orbits the last frame was drawn after, which a drag moves the view from.
        double m_shownOrbits{};
        double m_shownTimeInOrbits{}; // the globe's time of the last frame, counted in orbits
        // The turns the Earth stands ahead of its time's count, by a restore.
        double m_earthTurnsAhead{};
        bool m_viewHeld{}; // by a press
        std::optional<double> m_lastFrameAt{}; // in seconds of the scene's time
        // How long the globe has stood still in all, which its motion runs that far behind.
        double m_stillSeconds{};
        // How far round the Earth's axis the double clicks have moved the Sun, in turns.
        double m_sunTurns{};
        double m_shownSunTurns{}; // where the last frame drew the Sun, gliding toward m_sunTurns
    };
}
