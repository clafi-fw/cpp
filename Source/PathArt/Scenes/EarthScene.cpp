module;
#include <immintrin.h>
module ClaFi.PathArt.EarthScene;

import ClaFi.PathArt.CloudMap;
import ClaFi.PathArt.EarthMap;
import ClaFi.PathArt.PlanetSprite;
import ClaFi.PathArt.SatelliteSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.PathArt.StarsEffect;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace EarthSceneColors
    {
        constexpr Color SpaceTop{ 0xFF02030A };
        constexpr Color SpaceBottom{ 0xFF0A1230 };
        constexpr Color Star{ 0xFFF0F3FF };
        constexpr Color SunGlare{ 0xFFFFF1D6 };
        constexpr Color OceanLimb{ 0xFF3F8EE0 };
        constexpr Color Haze{ 0xFF3F8EE0 }; // what the air's haze draws the ground toward
        constexpr Color OceanDeep{ 0xFF0C3D84 }; // the abyss, 4000 to 6000 metres down
        constexpr Color SeaShelf{ 0xFF1E6880 }; // over the shelf, less than 200 metres deep
        constexpr Color SeaSlope{ 0xFF185C8C }; // down the continental slope, 200 to 1000 metres
        constexpr Color SeaRise{ 0xFF13508F }; // the rise below the slope, 1000 to 2000 metres
        constexpr Color SeaRidge{ 0xFF0F4689 }; // the ridges, 2000 to 4000 metres down
        constexpr Color SeaTrench{ 0xFF082C66 }; // the trenches, past 8000 metres
        constexpr Color FlatSeaShelf{ 0xFF4398DE }; // the stylized look's
        constexpr Color FlatSeaSlope{ 0xFF2B78C4 };
        constexpr Color FlatSeaRidge{ 0xFF1A5BA6 };
        constexpr Color FlatSeaTrench{ 0xFF082C66 };
        constexpr Color Cloud{ 0xCAFFFFFF }; // where a layer covers the sky whole
        constexpr Color Moonlight{ 0xFF46536E }; // the night's light on the clouds and the ground
        constexpr Color Night{ 0xFF020614 };
        constexpr Color NightBlue{ 0xFF030818 };
        constexpr Color SeaGlint{ 0xFFCFE8FF };
        constexpr Color CityGlow{ 0xFFFFB84D };
        constexpr Color CityLight{ 0xFFFFD27A };
        constexpr Color Day{ 0xFF6CB8FF };
        constexpr Color Dusk{ 0xFFFF8A3D };
        constexpr Color DuskDeep{ 0xFFFF7040 };
        constexpr Color NightAir{ 0xFF1B2A5C };
        constexpr Color Grass{ 0xFF507B38 };
        constexpr Color GrassLight{ 0xFF61853F };
        constexpr Color Steppe{ 0xFF929355 };
        constexpr Color Sand{ 0xFFAB8D5C };
        constexpr Color SandLight{ 0xFFBBA377 };
        constexpr Color Taiga{ 0xFF3B6633 };
        constexpr Color TaigaDeep{ 0xFF2F5A2E };
        constexpr Color Tundra{ 0xFF8C8E6C };
        constexpr Color TundraMoss{ 0xFF7D8462 };
        constexpr Color Rock{ 0xFF85765C };
        constexpr Color RockHigh{ 0xFF9C9387 };
        constexpr Color Snow{ 0xFFF3F6FA };
        constexpr Color SlopeShadow{ 0xFF141C24 };
        constexpr Color SlopeLight{ 0xFFFFF6E2 };
        constexpr Color FlatGrass{ 0xFF66973F }; // the stylized look's
        constexpr Color FlatSteppe{ 0xFFA6A65A };
        constexpr Color FlatSand{ 0xFFD3B477 };
        constexpr Color FlatTaiga{ 0xFF3F6D36 };
        constexpr Color FlatTundra{ 0xFF8E9070 };
        constexpr Color FlatRock{ 0xFF9A8565 };
        constexpr Color FlatRockHigh{ 0xFFB9AE9E };
        constexpr Color IconOcean{ 0xFF2F6FB8 }; // the planet the scene's icon shows
    }

    namespace
    {
        constexpr float k_degree{ std::numbers::pi_v<float> / 180.0f };
        // How long the satellite takes to go round, over both poles.
        constexpr std::chrono::seconds k_orbit{ 180 };
        // The orbits each turn of the Earth, 3 and the golden ratio. See PathArt#earthscene
        constexpr double k_orbitsPerEarthTurn{ 3.0 + std::numbers::phi };
        // Where on its orbit the satellite starts, from the equator northward.
        constexpr float k_orbitStart{ 81.0f * k_degree };
        // How far the Earth has turned at the start.
        constexpr float k_earthTurnAtStart{ 72.0f * k_degree };
        // How far from the satellite, away from the horizon, the view looks straight down.
        constexpr float k_viewBehind{ 36.0f * k_degree };
        // How far it looks with the whole globe in the frame, so never down the Earth's axis.
        constexpr float k_leastViewBehind{ 1.0f * k_degree };
        // The share of the way to the whole globe a wheel notch climbs or comes down by.
        constexpr float k_altitudePerNotch{ 0.1f };
        // The globe's radius at the lowest altitude, of the frame's longer side.
        constexpr float k_lowestGlobe{ 1.15f };
        // The globe's radius with the whole globe in the frame, of the frame's shorter side.
        constexpr float k_wholeGlobe{ 0.4f };
        // How far round the stars the Sun stands from where the orbit crosses the equator.
        constexpr float k_sunAround{ 35.0f * k_degree };
        // How far north of the equator the Sun stands.
        constexpr float k_sunNorth{ 10.0f * k_degree };
        // How much of a day a double click moves the light on by.
        constexpr double k_dayPerDoubleClick{ 0.25 };
        // The seconds the Sun takes to glide two thirds of the way a double click moves it.
        constexpr double k_sunGliding{ 0.25 };
        // How near the Sun has to glide before it is drawn where it was moved, in turns.
        constexpr double k_sunGlideEnd{ 0.0005 };
        // The frame widths the stars drift each time the satellite goes round.
        constexpr float k_skyWidthsPerOrbit{ 0.2f };
        // The pace things keep at a 40-second cycle, in this scene's 240-second one.
        constexpr float k_lively{ 6.0f };
        // How many opacities the city lights are drawn in, one fill each on Direct2D.
        constexpr int k_cityOpacitySteps{ 6 };
        // The brightness above which a town glows, unless a glow stands near: some 190 of 7300.
        constexpr float k_glowingBrightness{ 0.97f };
        // How far apart the points are where a shape's edge follows the rim.
        constexpr float k_rimStep{ 2.0f * k_degree };
        // How far a point of cloud faces the viewer when it starts fading into the haze.
        constexpr float k_cloudHazeDepth{ 0.12f };
        // The rows of a picture worked out together on one core, and laid in as one.
        constexpr int k_bandRows{ 32 };
        // The shades of the clouds kept from day to deep night.
        constexpr int k_cloudShadeCount{ 1024 };
        // The night's premultiplied colours kept from day to deep night.
        constexpr int k_nightShadeCount{ 256 };
        // The realistic sea's colours kept from the coast to the deepest trench.
        constexpr int k_seaShadeCount{ 256 };
        // The sunlight at which the night starts to fall.
        constexpr float k_duskSunlight{ 0.03f };
        // How much less sunlight it takes from there for the night to fall whole.
        constexpr float k_nightSunlightSpan{ 0.16f };
        // The moonlight's share of the night over the ground: deep in it, an eighth of the view.
        constexpr float k_moonlitShareOfNight{ 0.125f / 0.9f };
        // The globe's radius in pixels from which the land is worked out every 4th pixel.
        constexpr float k_coarseRadius{ 3200.0f };
        // The globe's radius in pixels from which the land is worked out every 2nd pixel.
        constexpr float k_secondPixelRadius{ 1300.0f };
        // The cover the stylized look's lighter tone starts from, showing nothing below it.
        constexpr float k_stylizedLeastCover{ 0.28f };
        // The cover over which a stylized tone of cloud fades in, where the cover changes slowly.
        constexpr float k_cloudToneFade{ 0.06f };
        // The cover at which the stylized look's denser tone of cloud is halfway in.
        constexpr float k_stylizedDenseCover{ 0.65f };
        // How much of the sky the stylized look's lighter tone of cloud hides.
        constexpr float k_lighterCloudShare{ 0.55f };
        // How high a height of 1 on the land's map stands, as a share of the Earth's radius.
        constexpr float k_reliefHeight{ 0.014f };
        // A Sun lower than this lights the slopes as from this. See PathArt#earthscene
        constexpr float k_lowestSunForRelief{ 0.25f };
        // The height the stylized look's lowest band starts at.
        constexpr float k_lowestBand{ 0.08f };
        // The light below which the stylized look shows a slope's shaded tone.
        constexpr float k_shadedBelow{ 0.925f };
        // The light above which it shows a slope's lit tone.
        constexpr float k_litAbove{ 1.055f };
        // The light over which a tone of slope fades in, where the light changes slowly.
        constexpr float k_toneFade{ 0.05f };
        // The dryness, forest or tundra over which a zone fades into the next, where it changes
        // slowly.
        constexpr float k_zoneFade{ 0.03f };
        // The depth over which a tone of the stylized sea fades in, where the depth changes slowly.
        constexpr float k_seaToneFade{ 0.02f };
        // The air looked through, past the air straight down, that veils half of what lies behind.
        constexpr float k_halfHazeAir{ 7.0f };
        // How fast the air thins with height: a third of it is left over the Himalaya's crest.
        constexpr float k_airThinning{ 1.1f };
        // How far to either side an edge cut at the pixel is softened, in what the pixel spans.
        constexpr float k_edgeSoftening{ 0.75f };
        // The least depth the ground a point stands for is worked out at, short of the rim.
        constexpr float k_leastSpanDepth{ 0.02f };
        // How much wider than the points stand apart the ground about one is taken: the land is
        // read half a level coarser, as detail that fine shimmers when the globe turns.
        constexpr float k_spanWidening{ 1.4142135f };
        // The realistic coast's blur per pixel between the land's points. See PathArt#earthscene
        constexpr float k_coastBlurPerStep{ 0.5f };

        // What the viewer may do to the scene, as the host's hints show it.
        constexpr std::array<SceneGesture, 4> k_gestures{
            SceneGesture{ L"Click", L"to stop and resume" },
            SceneGesture{ L"Double click", L"to advance time" },
            SceneGesture{ L"Drag", L"to choose position" },
            SceneGesture{ L"Drag and fling", L"to make an orbit" }
        };

        // The names the view is kept under in the config.
        namespace ViewNodes
        {
            constexpr std::wstring_view Altitude = L"Altitude";
            constexpr std::wstring_view Orbits = L"Orbits";
            constexpr std::wstring_view EarthTurns = L"EarthTurns";
            constexpr std::wstring_view SunTurns = L"SunTurns";
            constexpr std::wstring_view TrackAngle = L"TrackAngle";
            constexpr std::wstring_view Hanging = L"Hanging";
            constexpr std::wstring_view HangingCamera = L"HangingCamera";
            constexpr std::wstring_view OrbitTurn = L"OrbitTurn";
            constexpr std::wstring_view AxisX = L"AxisX";
            constexpr std::wstring_view AxisY = L"AxisY";
            constexpr std::wstring_view AxisZ = L"AxisZ";
            constexpr std::wstring_view Degrees = L"Degrees";
        }

        // A layer of cloud: what its map holds, and how it floats. See PathArt#earthscene
        struct CloudLayer
        {
            CloudKind kind;
            std::uint32_t seed;
            float cover; // the share of the sphere its map covers
            float height; // above the ground, as a share of the Earth's radius
            float axisTilt; // how far its axis leans from the Earth's, in degrees
            float driftPerSecond; // how far it drifts east against the ground, in degrees
            float opacity; // its share of the cloud colour's opacity
        };

        // Low heaps, a middle sheet and high streaks, each drifting faster than the one below.
        constexpr std::array<CloudLayer, 3> k_cloudLayers{
            CloudLayer{
                .kind = CloudKind::Puffy,
                .seed = 11,
                .cover = 0.32f,
                .height = 0.010f,
                .axisTilt = 0.0f,
                .driftPerSecond = 0.4f,
                .opacity = 0.9f
            },
            CloudLayer{
                .kind = CloudKind::Patchy,
                .seed = 21,
                .cover = 0.45f,
                .height = 0.014f,
                .axisTilt = 5.0f,
                .driftPerSecond = 0.6f,
                .opacity = 0.55f
            },
            CloudLayer{
                .kind = CloudKind::Streaky,
                .seed = 31,
                .cover = 0.30f,
                .height = 0.018f,
                .axisTilt = -6.0f,
                .driftPerSecond = 0.9f,
                .opacity = 0.45f
            }
        };

        // A sphere as this frame shows it, placed by the canvas's pixels.
        struct SphereView
        {
            FloatPoint fromPixel; // across and up from the sphere's center, in its radii, per pixel
            FloatPoint atOrigin; // the same at the canvas's pixel 0, 0
            float tiltCosine;
            float tiltSine;
            float tipCosine; // of how far its axis tips back from the screen's up
            float tipSine;
            float spin;
        };

        // A layer of cloud as this frame shows it.
        struct CloudLayerView
        {
            const CloudMap* map;
            SphereView sphere;
            float opacity; // where its map's cover is whole
        };

        // Where the points of a row fall on a layer's map, and how far the ground about each spans.
        struct MapPoints
        {
            std::vector<float> longitudes;
            std::vector<float> latitudes;
            std::vector<float> longitudeSpans;
            std::vector<float> latitudeSpans;
        };

        // What a layer shows at the points of a row.
        struct LayerPoints
        {
            std::vector<float> covers;
            std::vector<float> clearness; // how clear of the haze, 0 off the sphere
        };

        // Each layer's points along one row, in the order of k_cloudLayers.
        using CloudPoints = std::array<LayerPoints, k_cloudLayers.size()>;

        // A sphere in the canvas's pixels and the Sun as the screen stands, for how deep in the
        // night each pixel lies.
        struct NightView
        {
            FloatPoint center; // in pixels
            FloatPoint inverseRadii; // across and up, per pixel
            FloatPoint sunAcross; // the Sun across and up the screen
            float sunToward; // and toward the viewer
        };

        // What every band of the clouds' picture is worked out from, this frame.
        struct CloudPicture
        {
            std::span<const CloudLayerView> layers;
            SceneLook look;
            int step; // the pixels between the points the layers are worked out at, 2, 4 or 8
            FloatPoint sphereCenter; // the highest layer's, in pixels
            FloatPoint sphereRadii; // the highest layer's, across and up, in pixels
            int left; // the first column in view
            int right; // the column past the last in view
            std::span<const Color> shades; // the clouds' colour from day to deep night
            NightView night;
        };

        // How 8 pixels in a row stand between its points at a step: the point before each, the
        // point after it, and the share of the way from the one to the other.
        struct PointSpread
        {
            __m256i before;
            __m256i after;
            __m256 shares;
        };

        // A value at 8 pixels, and how much it changes over each pixel.
        struct SpreadLanes
        {
            __m256 values;
            __m256 change;
        };

        // The points about each of 8 pixels: its own and the next, in the rows above and below it.
        struct PointCorners
        {
            __m256 upperLeft;
            __m256 upperRight;
            __m256 lowerLeft;
            __m256 lowerRight;
        };

        // A city's light where it shows this frame, and the opacity step it is drawn at.
        struct LitCity
        {
            FloatPoint at;
            float radius;
            int step;
        };

        // The side of a cell of the grid the towns are placed on, in degrees.
        constexpr float k_townCellSize{ 0.25f };
        constexpr float k_townGridNorth{ 85.0f }; // the latitude its first row starts at
        constexpr int k_townGridColumns{ 1440 }; // round the whole circle of latitude
        constexpr int k_townGridRows{ 572 }; // down to 58 degrees south
        constexpr float k_townSouth{ -56.0f }; // the latitudes the towns are tried between
        constexpr float k_townNorth{ 72.0f };
        constexpr int k_landTownCount{ 1500 }; // tried over the land
        constexpr int k_coastlineTownCount{ 5800 }; // tried on the coastline itself
        // The least arc between two towns, in degrees, so their lights stand apart.
        constexpr float k_leastTownSpacing{ 0.2f };
        // The least arc between two glowing towns, in degrees, so their glows stand apart.
        constexpr float k_leastGlowSpacing{ 1.5f };

        // Whether an outline lies under Greenland's or Antarctica's ice, by its points' middle.
        [[nodiscard]] bool isIceSheet(const std::span<const FloatPoint> outline)
        {
            FloatPoint sum = { 0.0f, 0.0f };
            for (const FloatPoint point : outline)
                sum = { sum.x + point.x, sum.y + point.y };
            const float count = static_cast<float>(outline.size());
            const FloatPoint middle = { sum.x / count, sum.y / count };
            const bool greenland = middle.y > 59.0f && middle.x > -60.0f && middle.x < -30.0f;
            return greenland || middle.y < -60.0f;
        }

        // The towns' grid's column for a column counted past either end, round the circle.
        [[nodiscard]] std::size_t wrappedTownColumn(const int column)
        {
            return static_cast<std::size_t>((column % k_townGridColumns + k_townGridColumns)
                % k_townGridColumns);
        }

        // The towns' grid's row a latitude falls in, counted on past either end.
        [[nodiscard]] int townRowAt(const float latitude)
        {
            return static_cast<int>(std::floor((k_townGridNorth - latitude) / k_townCellSize));
        }

        // The towns' grid's column a longitude falls in, counted on past either end.
        [[nodiscard]] int townColumnAt(const float longitude)
        {
            return static_cast<int>(std::floor((longitude + 180.0f) / k_townCellSize));
        }

        // The cell of the towns' grid a point of the map lies in, if the grid reaches it.
        [[nodiscard]] std::optional<std::size_t> townCellAt(const FloatPoint lonLat)
        {
            const int row = townRowAt(lonLat.y);
            if (row < 0 || row >= k_townGridRows)
                return std::nullopt;
            return static_cast<std::size_t>(row) * k_townGridColumns
                + wrappedTownColumn(townColumnAt(lonLat.x));
        }

        // **The cells of the towns' grid within `arc` degrees of a point**: the rows the arc spans
        // either side, and the columns it spans in longitude where the farther of those rows lies,
        // as a degree of longitude shortens toward the poles.
        [[nodiscard]] std::vector<std::size_t> townCellsNear(const FloatPoint lonLat,
            const float arc)
        {
            const int row = townRowAt(lonLat.y);
            const int column = townColumnAt(lonLat.x);
            const int rowsAcross = static_cast<int>(std::ceil(arc / k_townCellSize));
            const float farthestLatitude = std::abs(lonLat.y) + arc;
            const int columnsAcross = static_cast<int>(std::ceil(
                arc / std::cos(farthestLatitude * k_degree) / k_townCellSize));
            std::vector<std::size_t> cells;
            const int firstRow = std::max(row - rowsAcross, 0);
            const int lastRow = std::min(row + rowsAcross, k_townGridRows - 1);
            for (int nearRow = firstRow; nearRow <= lastRow; ++nearRow)
            {
                for (int nearColumn = column - columnsAcross; nearColumn <= column + columnsAcross;
                    ++nearColumn)
                {
                    cells.push_back(static_cast<std::size_t>(nearRow) * k_townGridColumns
                        + wrappedTownColumn(nearColumn));
                }
            }
            return cells;
        }

        // **Which cells of the towns' grid have their middle on land**, 1 for land. Each edge marks
        // the rows whose middle latitude it spans, half open so that a point two edges share counts
        // once. A row's crossings, sorted, bound the land between each pair, so a sea inside the
        // land, its outline the other way round, stays sea.
        [[nodiscard]] std::vector<std::uint8_t> landCellsOf(
            const std::span<const std::span<const FloatPoint>> outlines)
        {
            std::vector<std::vector<float>> crossings(static_cast<std::size_t>(k_townGridRows));
            for (const std::span<const FloatPoint> outline : outlines)
            {
                for (std::size_t i = 0; i != outline.size(); ++i)
                {
                    const FloatPoint from = outline[i];
                    const FloatPoint to = outline[(i + 1) % outline.size()];
                    const float lower = std::min(from.y, to.y);
                    const float upper = std::max(from.y, to.y);
                    const int firstRow = std::max(0, static_cast<int>(
                        std::floor((k_townGridNorth - upper) / k_townCellSize - 0.5f)) + 1);
                    const int lastRow = std::min(k_townGridRows - 1, static_cast<int>(
                        std::floor((k_townGridNorth - lower) / k_townCellSize - 0.5f)));
                    for (int row = firstRow; row <= lastRow; ++row)
                    {
                        const float latitude = k_townGridNorth
                            - (static_cast<float>(row) + 0.5f) * k_townCellSize;
                        const float share = (latitude - from.y) / (to.y - from.y);
                        crossings[static_cast<std::size_t>(row)].push_back(
                            from.x + (to.x - from.x) * share);
                    }
                }
            }
            std::vector<std::uint8_t> land(
                static_cast<std::size_t>(k_townGridColumns) * k_townGridRows, 0);
            for (int row = 0; row != k_townGridRows; ++row)
            {
                std::vector<float>& rowCrossings = crossings[static_cast<std::size_t>(row)];
                std::ranges::sort(rowCrossings);
                const std::size_t rowStart = static_cast<std::size_t>(row) * k_townGridColumns;
                for (std::size_t i = 0; i + 1 < rowCrossings.size(); i += 2)
                {
                    const int firstColumn = static_cast<int>(
                        std::ceil((rowCrossings[i] + 180.0f) / k_townCellSize - 0.5f));
                    const int lastColumn = static_cast<int>(
                        std::floor((rowCrossings[i + 1] + 180.0f) / k_townCellSize - 0.5f));
                    for (int column = firstColumn; column <= lastColumn; ++column)
                        land[rowStart + wrappedTownColumn(column)] = 1;
                }
            }
            return land;
        }

        // **How far each cell's middle of the towns' grid stands from a coast**, in degrees of arc.
        // The coasts' points start at 0, and two sweeps, one forward and one back, carry each
        // cell's distance on to its neighbours, a cell's width shrinking with the latitude.
        [[nodiscard]] std::vector<float> coastDistancesOf(
            const std::span<const std::span<const FloatPoint>> outlines)
        {
            std::vector<float> distances(static_cast<std::size_t>(k_townGridColumns)
                * k_townGridRows, std::numeric_limits<float>::max());
            for (const std::span<const FloatPoint> outline : outlines)
            {
                for (const FloatPoint point : outline)
                {
                    if (const std::optional<std::size_t> cell = townCellAt(point))
                        distances[*cell] = 0.0f;
                }
            }
            const auto at = [&](int row, int column) -> float& {
                return distances[static_cast<std::size_t>(row) * k_townGridColumns
                    + wrappedTownColumn(column)];
            };
            for (const int direction : { 1, -1 })
            {
                for (int sweptRow = 0; sweptRow != k_townGridRows; ++sweptRow)
                {
                    const int row = direction == 1 ? sweptRow : k_townGridRows - 1 - sweptRow;
                    const float latitude = k_townGridNorth
                        - (static_cast<float>(row) + 0.5f) * k_townCellSize;
                    const float across = k_townCellSize * std::cos(latitude * k_degree);
                    const float diagonal = std::hypot(across, k_townCellSize);
                    const int previousRow = row - direction;
                    const bool hasPreviousRow = previousRow >= 0 && previousRow < k_townGridRows;
                    for (int sweptColumn = 0; sweptColumn != k_townGridColumns; ++sweptColumn)
                    {
                        const int column = direction == 1
                            ? sweptColumn : k_townGridColumns - 1 - sweptColumn;
                        float& distance = at(row, column);
                        distance = std::min(distance, at(row, column - direction) + across);
                        if (!hasPreviousRow)
                            continue;
                        distance = std::min(distance, at(previousRow, column) + k_townCellSize);
                        distance = std::min(distance, at(previousRow, column - 1) + diagonal);
                        distance = std::min(distance, at(previousRow, column + 1) + diagonal);
                    }
                }
            }
            return distances;
        }

        // A point to try a town at, as likely anywhere between the towns' latitudes.
        [[nodiscard]] FloatPoint townSiteAt(const int seed)
        {
            const float longitude = -180.0f + 360.0f * pseudoRandom(seed, 71.0f);
            const float southSine = std::sin(k_townSouth * k_degree);
            const float northSine = std::sin(k_townNorth * k_degree);
            const float sine = std::lerp(southSine, northSine, pseudoRandom(seed, 72.0f));
            return { longitude, std::asin(sine) / k_degree };
        }

        // **A point on the coastline to try a town at**, about as likely anywhere along it, as the
        // outlines' points stand about evenly apart. It is moved 0.03 to 0.12 degrees of arc
        // inland, to the left of the way the outline runs, so its light stands on the land.
        [[nodiscard]] FloatPoint coastlineSiteAt(
            const std::span<const std::span<const FloatPoint>> outlines,
            const std::size_t pointCount, const int seed)
        {
            const float picked = pseudoRandom(seed, 81.0f) * static_cast<float>(pointCount);
            std::size_t point = std::min(pointCount - 1, static_cast<std::size_t>(picked));
            std::size_t outlineIndex = 0;
            while (point >= outlines[outlineIndex].size())
            {
                point -= outlines[outlineIndex].size();
                ++outlineIndex;
            }
            const std::span<const FloatPoint> outline = outlines[outlineIndex];
            const FloatPoint from = outline[point];
            const FloatPoint to = outline[point + 1 == outline.size() ? 0 : point + 1];
            // The edge in degrees of arc, a degree of longitude shortened by the latitude's cosine.
            const float across = std::cos(from.y * k_degree);
            const FloatPoint along = { (to.x - from.x) * across, to.y - from.y };
            const float inland = std::lerp(0.03f, 0.12f, pseudoRandom(seed, 82.0f))
                / std::hypot(along.x, along.y);
            const float share = pseudoRandom(seed, 83.0f);
            return {
                from.x + (to.x - from.x) * share - along.y * inland / across,
                from.y + (to.y - from.y) * share + along.x * inland
            };
        }

        // Where the globe stands in a frame.
        struct GlobeDisc
        {
            FloatPoint center;
            float radius;
        };

        // How far the view leans back from looking straight down, at an altitude.
        [[nodiscard]] float viewBehindAt(const float altitude)
        {
            return std::lerp(k_viewBehind, k_leastViewBehind, altitude);
        }

        // The part of a count past its last whole number, taken in double to keep a long run exact.
        [[nodiscard]] float fractionOf(const double count)
        {
            return static_cast<float>(count - std::floor(count));
        }

        // **Where the globe stands in a frame at an altitude**: at the lowest a cap shows, and at
        // the highest the whole globe, in the middle. See PathArt#earthscene
        [[nodiscard]] GlobeDisc globeDiscAt(const float width, const float height,
            const float altitude)
        {
            const float lowest = k_lowestGlobe * std::max(width, height);
            const float whole = k_wholeGlobe * std::min(width, height);
            const float radius = lowest * std::pow(whole / lowest, altitude);
            const float top = std::lerp(height * 0.44f, height * 0.5f - whole, altitude);
            return { { width * std::lerp(0.4f, 0.5f, altitude), top + radius }, radius };
        }

        // Where the limb crosses both sides of the frame, the ocean covers everything below the
        // lower crossing, so space stops a little past it.
        void fillSpace(const SceneFrame& frame, const GlobeDisc& disc)
        {
            const float farthest = std::max(disc.center.x, frame.width - disc.center.x);
            if (farthest >= disc.radius)
            {
                fillBox(frame, { 0.0f, 0.0f, frame.width, frame.height },
                    LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, frame.height },
                        EarthSceneColors::SpaceTop, EarthSceneColors::SpaceBottom));
                return;
            }
            const float limbLowest = disc.center.y
                - std::sqrt(disc.radius * disc.radius - farthest * farthest);
            const float bottom = std::min(frame.height, limbLowest + 2.0f * frame.unit);
            fillBox(frame, { 0.0f, 0.0f, frame.width, bottom },
                LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, frame.height },
                    EarthSceneColors::SpaceTop, EarthSceneColors::SpaceBottom));
        }

        void fillOcean(const SceneFrame& frame, const GlobeDisc& disc)
        {
            PixelPath outline;
            outline.drawCircle(disc.center, disc.radius);
            frame.canvas.pushClip(outline, &frame.toCanvas);
            const float top = disc.center.y - disc.radius;
            fillBox(frame, { 0.0f, 0.0f, frame.width, frame.height },
                LinearGradient::Simple({ 0.0f, top }, { 0.0f, top + disc.radius * 0.45f },
                EarthSceneColors::OceanLimb, EarthSceneColors::OceanDeep));
            frame.canvas.popClip();
        }

        // The atmosphere's colour at a point of the limb, from the sunlight there.
        [[nodiscard]] Color atmosphereColor(const float sunlight)
        {
            if (sunlight > 0.18f)
                return EarthSceneColors::Day.withOpacity(0.9f);
            if (sunlight > 0.0f)
            {
                const Color tint = colorBetween(EarthSceneColors::Dusk,
                    EarthSceneColors::Day, sunlight / 0.18f);
                return tint.withOpacity(0.92f);
            }
            if (sunlight > -0.16f)
            {
                const float share = 1.0f + sunlight / 0.16f;
                const Color tint = colorBetween(EarthSceneColors::NightAir,
                    EarthSceneColors::DuskDeep, share);
                return tint.withOpacity(std::lerp(0.3f, 0.92f, share));
            }
            return EarthSceneColors::NightAir.withOpacity(0.3f);
        }

        // The pixels a row's buffers hold, a whole number of the 8 one AVX2 step works on.
        [[nodiscard]] std::size_t groupedLength(const int pixels)
        {
            return (static_cast<std::size_t>(pixels) + 7) & ~std::size_t{ 7 };
        }

        // The pixel offsets of the 8 lanes.
        [[nodiscard]] __m256 laneOffsets()
        {
            return _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
        }

        // The sphere and the Sun for the night, the Sun turned from the view's axes to the screen.
        [[nodiscard]] NightView nightViewOf(const Matrix3x2& toPixels, const FloatPoint center,
            const float radius, const float tilt, const float sunX, const float sunY,
            const float sunZ)
        {
            return {
                .center = toPixels.transform(center),
                .inverseRadii = { 1.0f / (radius * toPixels.a), 1.0f / (radius * toPixels.d) },
                .sunAcross = rotated({ sunX, sunY }, tilt),
                .sunToward = sunZ
            };
        }

        // **How deep in the night 8 pixels of a row lie**, 0 on the day side of the dusk and 1
        // where the night has fallen whole, from the sunlight on the sphere under each. Past its
        // edge the sunlight is the limb's.
        [[nodiscard]] __m256 nightDepths(const NightView& night, const __m256 pixelX,
            const float pixelY)
        {
            const __m256 across = _mm256_mul_ps(
                _mm256_sub_ps(pixelX, _mm256_set1_ps(night.center.x)),
                _mm256_set1_ps(night.inverseRadii.x));
            const float up = (night.center.y - pixelY) * night.inverseRadii.y;
            const __m256 rest = _mm256_fnmadd_ps(across, across, _mm256_set1_ps(1.0f - up * up));
            const __m256 toward = _mm256_sqrt_ps(_mm256_max_ps(rest, _mm256_setzero_ps()));
            const __m256 sunlight = _mm256_fmadd_ps(across, _mm256_set1_ps(night.sunAcross.x),
                _mm256_fmadd_ps(toward, _mm256_set1_ps(night.sunToward),
                    _mm256_set1_ps(up * night.sunAcross.y)));
            const __m256 depth = _mm256_mul_ps(_mm256_sub_ps(_mm256_set1_ps(k_duskSunlight),
                sunlight), _mm256_set1_ps(1.0f / k_nightSunlightSpan));
            return _mm256_min_ps(_mm256_max_ps(depth, _mm256_setzero_ps()), _mm256_set1_ps(1.0f));
        }

        // 0 below edge0, 1 above edge1 and an S-curve between, in each lane.
        [[nodiscard]] __m256 smoothStep8(const float edge0, const float edge1, const __m256 value)
        {
            const __m256 scaled = _mm256_mul_ps(_mm256_sub_ps(value, _mm256_set1_ps(edge0)),
                _mm256_set1_ps(1.0f / (edge1 - edge0)));
            const __m256 share = _mm256_min_ps(_mm256_max_ps(scaled, _mm256_setzero_ps()),
                _mm256_set1_ps(1.0f));
            return _mm256_mul_ps(_mm256_mul_ps(share, share),
                _mm256_fnmadd_ps(_mm256_set1_ps(2.0f), share, _mm256_set1_ps(3.0f)));
        }

        // How far an edge is softened to either side, with the reciprocal of the width it softens
        // over, so edges that soften alike divide once.
        struct SoftEdge
        {
            __m256 softness;
            __m256 perWidth;
        };

        [[nodiscard]] SoftEdge softEdgeOf(const __m256 softness)
        {
            return { softness, _mm256_div_ps(_mm256_set1_ps(0.5f), softness) };
        }

        // 0 below `edge` less the softness, 1 above `edge` plus it, and an S-curve between.
        [[nodiscard]] __m256 smoothStepAround(const __m256 edge, const SoftEdge& soft,
            const __m256 value)
        {
            const __m256 scaled = _mm256_mul_ps(_mm256_add_ps(_mm256_sub_ps(value, edge),
                soft.softness), soft.perWidth);
            const __m256 share = _mm256_min_ps(_mm256_max_ps(scaled, _mm256_setzero_ps()),
                _mm256_set1_ps(1.0f));
            return _mm256_mul_ps(_mm256_mul_ps(share, share),
                _mm256_fnmadd_ps(_mm256_set1_ps(2.0f), share, _mm256_set1_ps(3.0f)));
        }

        // The angle of each point x, y from the x axis, to within two millionths of a radian.
        [[nodiscard]] __m256 quickAngle(const __m256 y, const __m256 x)
        {
            const __m256 signBit = _mm256_set1_ps(-0.0f);
            const __m256 absoluteX = _mm256_andnot_ps(signBit, x);
            const __m256 absoluteY = _mm256_andnot_ps(signBit, y);
            const __m256 ratio = _mm256_div_ps(_mm256_min_ps(absoluteX, absoluteY),
                _mm256_max_ps(absoluteX, absoluteY));
            const __m256 square = _mm256_mul_ps(ratio, ratio);
            __m256 series = _mm256_set1_ps(-0.0117212f);
            series = _mm256_fmadd_ps(series, square, _mm256_set1_ps(0.05265332f));
            series = _mm256_fmadd_ps(series, square, _mm256_set1_ps(-0.11643287f));
            series = _mm256_fmadd_ps(series, square, _mm256_set1_ps(0.19354346f));
            series = _mm256_fmadd_ps(series, square, _mm256_set1_ps(-0.33262347f));
            series = _mm256_fmadd_ps(series, square, _mm256_set1_ps(0.99997726f));
            __m256 angle = _mm256_mul_ps(ratio, series);
            const __m256 steep = _mm256_cmp_ps(absoluteY, absoluteX, _CMP_GT_OQ);
            angle = _mm256_blendv_ps(angle,
                _mm256_sub_ps(_mm256_set1_ps(std::numbers::pi_v<float> / 2.0f), angle), steep);
            const __m256 leftward = _mm256_cmp_ps(x, _mm256_setzero_ps(), _CMP_LT_OQ);
            angle = _mm256_blendv_ps(angle,
                _mm256_sub_ps(_mm256_set1_ps(std::numbers::pi_v<float>), angle), leftward);
            return _mm256_or_ps(angle, _mm256_and_ps(y, signBit));
        }

        // A row of a sphere's points `step` pixels apart, ready to be worked out 8 at a time.
        struct SphereRow
        {
            __m256 up;
            __m256 upSquared;
            __m256 upTiltCosine;
            __m256 upTiltSine;
            __m256 tiltCosine;
            __m256 tiltSine;
            __m256 tipCosine;
            __m256 tipSine;
            __m256 acrossStart;
            __m256 acrossPerPoint;
        };

        // 8 points of a sphere with its axis upright, in its radii: x across, y up the axis and z
        // toward the viewer.
        struct SphereLanes
        {
            __m256 across; // how far across the screen each stands from the center
            __m256 depth; // how far each faces the viewer, 0 off the sphere
            __m256 x;
            __m256 y;
            __m256 z;
            __m256 aroundAxis; // how far each is from the axis
        };

        // How far the ground about 8 points spans, in radians.
        struct PointSpans
        {
            __m256 longitudes;
            __m256 latitudes;
        };

        // A sphere at `center` with `radius` in the scene, placed by the canvas's pixels.
        [[nodiscard]] SphereView sphereViewOf(const Matrix3x2& toPixels, const FloatPoint center,
            const float radius, const float tilt, const float tip, const float spin)
        {
            const FloatPoint pixelScale = { toPixels.a, toPixels.d };
            const FloatPoint pixelShift = { toPixels.e, toPixels.f };
            return {
                .fromPixel = { 1.0f / (pixelScale.x * radius), -1.0f / (pixelScale.y * radius) },
                .atOrigin = {
                    -(pixelShift.x / pixelScale.x + center.x) / radius,
                    (center.y + pixelShift.y / pixelScale.y) / radius
                },
                .tiltCosine = std::cos(tilt),
                .tiltSine = std::sin(tilt),
                .tipCosine = std::cos(tip),
                .tipSine = std::sin(tip),
                .spin = spin
            };
        }

        [[nodiscard]] SphereRow sphereRowAt(const SphereView& sphere, const float pixelY,
            const float firstPixelX, const int step)
        {
            const float up = sphere.atOrigin.y + sphere.fromPixel.y * pixelY;
            return {
                .up = _mm256_set1_ps(up),
                .upSquared = _mm256_set1_ps(up * up),
                .upTiltCosine = _mm256_set1_ps(up * sphere.tiltCosine),
                .upTiltSine = _mm256_set1_ps(up * sphere.tiltSine),
                .tiltCosine = _mm256_set1_ps(sphere.tiltCosine),
                .tiltSine = _mm256_set1_ps(sphere.tiltSine),
                .tipCosine = _mm256_set1_ps(sphere.tipCosine),
                .tipSine = _mm256_set1_ps(sphere.tipSine),
                .acrossStart = _mm256_set1_ps(sphere.atOrigin.x + sphere.fromPixel.x * firstPixelX),
                .acrossPerPoint = _mm256_set1_ps(sphere.fromPixel.x * static_cast<float>(step))
            };
        }

        // Off the sphere the depth is 0, and the points stand where it would put them.
        [[nodiscard]] SphereLanes sphereLanesAt(const SphereRow& row, const std::size_t firstPoint)
        {
            const __m256 point = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(firstPoint)),
                laneOffsets());
            const __m256 across = _mm256_fmadd_ps(point, row.acrossPerPoint, row.acrossStart);
            const __m256 depthSquared = _mm256_sub_ps(_mm256_set1_ps(1.0f),
                _mm256_fmadd_ps(across, across, row.upSquared));
            const __m256 depth = _mm256_sqrt_ps(_mm256_max_ps(depthSquared, _mm256_setzero_ps()));
            const __m256 x = _mm256_fmadd_ps(across, row.tiltCosine, row.upTiltSine);
            const __m256 y = _mm256_fnmadd_ps(across, row.tiltSine, row.upTiltCosine);
            const __m256 untippedY = _mm256_fmsub_ps(y, row.tipCosine,
                _mm256_mul_ps(depth, row.tipSine));
            const __m256 untippedZ = _mm256_fmadd_ps(y, row.tipSine,
                _mm256_mul_ps(depth, row.tipCosine));
            return {
                .across = across,
                .depth = depth,
                .x = x,
                .y = untippedY,
                .z = untippedZ,
                .aroundAxis = _mm256_sqrt_ps(
                    _mm256_fmadd_ps(x, x, _mm256_mul_ps(untippedZ, untippedZ)))
            };
        }

        // The pixels between the points the land is worked out at, 1, 2 or 4, by the globe's
        // radius in pixels.
        [[nodiscard]] int pointStepFor(const float radiusInPixels)
        {
            if (radiusInPixels >= k_coarseRadius)
                return 4;
            if (radiusInPixels >= k_secondPixelRadius)
                return 2;
            return 1;
        }

        // The share of the sphere's radius the ground about a point stands for, across and up, at a
        // step: k_spanWidening times as wide as the points stand apart.
        [[nodiscard]] FloatPoint pointSpanOf(const SphereView& sphere, const int step)
        {
            const float pointSpan = k_spanWidening * static_cast<float>(step);
            return {
                std::abs(sphere.fromPixel.x) * pointSpan,
                std::abs(sphere.fromPixel.y) * pointSpan
            };
        }

        // **The screen's two directions are turned as the points are**, and laid along east and
        // north at each. A span adds what the two reach, so it bounds the ground about the point,
        // `perPoint` of the radius across and up - see pointSpanOf. Near the rim the ground a point
        // stands for runs on over the horizon, and its depth is held at k_leastSpanDepth. The
        // reciprocals are good to 12 bits, ample for choosing a level of detail.
        [[nodiscard]] PointSpans spansAt(const SphereRow& row, const SphereLanes& lanes,
            const __m256 aroundAxis, const FloatPoint perPoint)
        {
            const __m256 nearness = _mm256_rcp_ps(
                _mm256_max_ps(lanes.depth, _mm256_set1_ps(k_leastSpanDepth)));
            const __m256 aroundInverse = _mm256_rcp_ps(aroundAxis);
            // How fast the depth falls, across the screen and up it.
            const __m256 fallAcross = _mm256_mul_ps(lanes.across, nearness);
            const __m256 fallUp = _mm256_mul_ps(row.up, nearness);
            const __m256 signBit = _mm256_set1_ps(-0.0f);
            const auto size = [&](__m256 value) {
                return _mm256_andnot_ps(signBit, value);
            };
            // How far a direction, tilted to `x` and `y` with the depth falling by `fall`, reaches
            // east, in radians of arc, and north.
            const auto reach = [&](__m256 x, __m256 y, __m256 fall) {
                const __m256 alongAxis = _mm256_fmadd_ps(fall, row.tipSine,
                    _mm256_mul_ps(y, row.tipCosine));
                const __m256 towardViewer = _mm256_fnmadd_ps(fall, row.tipCosine,
                    _mm256_mul_ps(y, row.tipSine));
                const __m256 east = _mm256_mul_ps(_mm256_fmsub_ps(x, lanes.z,
                    _mm256_mul_ps(towardViewer, lanes.x)), aroundInverse);
                const __m256 north = _mm256_fmsub_ps(aroundAxis, alongAxis,
                    _mm256_mul_ps(_mm256_mul_ps(lanes.y, _mm256_fmadd_ps(x, lanes.x,
                        _mm256_mul_ps(towardViewer, lanes.z))), aroundInverse));
                return std::array<__m256, 2>{ size(east), size(north) };
            };
            const std::array<__m256, 2> acrossReach = reach(row.tiltCosine,
                _mm256_xor_ps(row.tiltSine, signBit), fallAcross);
            const std::array<__m256, 2> upReach = reach(row.tiltSine, row.tiltCosine, fallUp);
            const __m256 perPointAcross = _mm256_set1_ps(perPoint.x);
            const __m256 perPointUp = _mm256_set1_ps(perPoint.y);
            return {
                .longitudes = _mm256_mul_ps(_mm256_fmadd_ps(acrossReach[0], perPointAcross,
                    _mm256_mul_ps(upReach[0], perPointUp)), aroundInverse),
                .latitudes = _mm256_fmadd_ps(acrossReach[1], perPointAcross,
                    _mm256_mul_ps(upReach[1], perPointUp))
            };
        }

        // Where the points of a row fall on the layer's map, how far the ground about each spans
        // and how clear of the haze each is, from the pixel center `firstPixelX` on, `step` pixels
        // apart. See PathArt#earthscene
        void projectRow(const SphereView& sphere, const float pixelY, const float firstPixelX,
            const int step, MapPoints& onMap, std::span<float> clearness)
        {
            const SphereRow row = sphereRowAt(sphere, pixelY, firstPixelX, step);
            const __m256 spin = _mm256_set1_ps(sphere.spin);
            const FloatPoint perPoint = pointSpanOf(sphere, step);
            // At the pole east and north turn about the axis, and any pair of them will do.
            const __m256 leastAroundAxis = _mm256_set1_ps(1.0e-4f);
            for (std::size_t i = 0; i != clearness.size(); i += 8)
            {
                // Off the sphere the depth is 0, which leaves the point wholly in the haze.
                const SphereLanes lanes = sphereLanesAt(row, i);
                _mm256_storeu_ps(onMap.longitudes.data() + i,
                    _mm256_sub_ps(quickAngle(lanes.x, lanes.z), spin));
                _mm256_storeu_ps(onMap.latitudes.data() + i, quickAngle(lanes.y, lanes.aroundAxis));
                const PointSpans spans = spansAt(row, lanes,
                    _mm256_max_ps(lanes.aroundAxis, leastAroundAxis), perPoint);
                _mm256_storeu_ps(onMap.longitudeSpans.data() + i, spans.longitudes);
                _mm256_storeu_ps(onMap.latitudeSpans.data() + i, spans.latitudes);
                _mm256_storeu_ps(clearness.data() + i, smoothStep8(0.0f, k_cloudHazeDepth,
                    lanes.depth));
            }
        }

        // **How much of the sky a map's cover hides when stylized**: two flat tones, each edge
        // softened by k_edgeSoftening of what the cover changes over the pixel, and no less than
        // k_cloudToneFade.
        [[nodiscard]] __m256 stylizedCoverShown(const SpreadLanes& cover)
        {
            const SoftEdge soft = softEdgeOf(_mm256_max_ps(_mm256_mul_ps(cover.change,
                _mm256_set1_ps(k_edgeSoftening)), _mm256_set1_ps(0.5f * k_cloudToneFade)));
            const __m256 lighter = smoothStepAround(
                _mm256_set1_ps(k_stylizedLeastCover + 0.5f * k_cloudToneFade), soft, cover.values);
            const __m256 denser = smoothStepAround(_mm256_set1_ps(k_stylizedDenseCover), soft,
                cover.values);
            return _mm256_fmadd_ps(_mm256_set1_ps(k_lighterCloudShare), lighter,
                _mm256_mul_ps(_mm256_set1_ps(1.0f - k_lighterCloudShare), denser));
        }

        // The night's colour from where it starts to fall, clear, to where it has fallen whole.
        [[nodiscard]] std::array<GradientStop, 3> nightStops()
        {
            return { {
                { .position = 0.0f, .color = EarthSceneColors::Night.withOpacity(0.0f) },
                { .position = 0.35f, .color = EarthSceneColors::NightBlue.withOpacity(0.55f) },
                { .position = 1.0f, .color = EarthSceneColors::Night.withOpacity(0.9f) }
            } };
        }

        // The colour between the stops at a position, each stop's colour first passed through.
        template<typename Shading>
        [[nodiscard]] Color colorAlong(const std::array<GradientStop, 3>& stops,
            const float position, const Shading& shaded)
        {
            std::size_t next = 1;
            while (next + 1 < stops.size() && stops[next].position < position)
                ++next;
            const GradientStop& from = stops[next - 1];
            const GradientStop& to = stops[next];
            const float share = std::clamp(
                (position - from.position) / (to.position - from.position), 0.0f, 1.0f);
            return colorBetween(shaded(from), shaded(to), share);
        }

        // **The clouds' colour from day to deep night**: the night's colour, drawn toward a
        // moonlit blue-grey as deep as it falls, mixed into the clouds' at the night's opacity.
        [[nodiscard]] std::vector<Color> cloudShades()
        {
            const std::array<GradientStop, 3> stops = nightStops();
            const auto shaded = [](const GradientStop& stop) {
                const float nightShare = static_cast<float>(stop.color.alpha) / 255.0f;
                const Color moonlit = colorBetween(stop.color, EarthSceneColors::Moonlight,
                    nightShare);
                return colorBetween(EarthSceneColors::Cloud, moonlit, nightShare);
            };
            std::vector<Color> shades;
            shades.reserve(k_cloudShadeCount);
            for (int i = 0; i != k_cloudShadeCount; ++i)
            {
                const float position = static_cast<float>(i) / (k_cloudShadeCount - 1);
                shades.push_back(colorAlong(stops, position, shaded));
            }
            return shades;
        }

        // **The night's colours over the land and the sea, from day to deep night**, premultiplied
        // as a band's pixels are: each stop's colour drawn toward the moonlight, at its own
        // opacity. Deep in the night the ground keeps a tenth of its colour, and the moonlight is
        // an eighth.
        [[nodiscard]] std::vector<Color> nightShades()
        {
            const std::array<GradientStop, 3> stops = nightStops();
            const auto moonlit = [](const GradientStop& stop) {
                const Color moonlitColor = colorBetween(stop.color,
                    EarthSceneColors::Moonlight, k_moonlitShareOfNight);
                return moonlitColor.withOpacity(static_cast<float>(stop.color.alpha) / 255.0f);
            };
            std::vector<Color> shades;
            shades.reserve(k_nightShadeCount);
            for (int i = 0; i != k_nightShadeCount; ++i)
            {
                const float position = static_cast<float>(i) / (k_nightShadeCount - 1);
                const Color color = colorAlong(stops, position, moonlit);
                const float alpha = static_cast<float>(color.alpha) / 255.0f;
                const auto times = [&](std::uint8_t channel) {
                    return static_cast<std::uint8_t>(std::lround(channel * alpha));
                };
                shades.push_back(Color{ times(color.red), times(color.green), times(color.blue),
                    color.alpha });
            }
            return shades;
        }

        // Each layer's cover and clearness at the points of a row, `step` pixels apart.
        void samplePoints(const CloudPicture& picture, const float pixelY, const float firstPixelX,
            MapPoints& onMap, CloudPoints& points)
        {
            for (std::size_t i = 0; i != picture.layers.size(); ++i)
            {
                const CloudLayerView& layer = picture.layers[i];
                projectRow(layer.sphere, pixelY, firstPixelX, picture.step, onMap,
                    points[i].clearness);
                layer.map->readCover({
                    .longitudes = onMap.longitudes,
                    .latitudes = onMap.latitudes,
                    .longitudeSpans = onMap.longitudeSpans,
                    .latitudeSpans = onMap.latitudeSpans
                }, points[i].covers);
            }
        }

        // The spread of 8 pixels from a point onward, at a step of 1, 2 or 4.
        [[nodiscard]] PointSpread pointSpreadAt(const int step)
        {
            alignas(32) std::array<int, 8> before{};
            alignas(32) std::array<int, 8> after{};
            alignas(32) std::array<float, 8> shares{};
            for (int lane = 0; lane != 8; ++lane)
            {
                const std::size_t index = static_cast<std::size_t>(lane);
                before[index] = lane / step;
                after[index] = step == 1 ? before[index] : before[index] + 1;
                shares[index] = static_cast<float>(lane % step) / static_cast<float>(step);
            }
            return {
                _mm256_load_si256(reinterpret_cast<const __m256i*>(before.data())),
                _mm256_load_si256(reinterpret_cast<const __m256i*>(after.data())),
                _mm256_load_ps(shares.data())
            };
        }

        // The values of 8 pixels, `downShare` of the way from 8 points of the upper row to the
        // lower and spread across between them, the first pixel standing on the first point.
        [[nodiscard]] __m256 pixelsBetween(const __m256 upperPoints, const __m256 lowerPoints,
            const __m256 downShare, const PointSpread& spread)
        {
            const __m256 points = _mm256_fmadd_ps(_mm256_sub_ps(lowerPoints, upperPoints),
                downShare, upperPoints);
            const __m256 before = _mm256_permutevar8x32_ps(points, spread.before);
            const __m256 after = _mm256_permutevar8x32_ps(points, spread.after);
            return _mm256_fmadd_ps(_mm256_sub_ps(after, before), spread.shares, before);
        }

        [[nodiscard]] PointCorners cornersAround(const std::vector<float>& upper,
            const std::vector<float>& lower, const std::size_t point, const PointSpread& spread)
        {
            const auto at = [&](const std::vector<float>& values, std::size_t first) {
                return _mm256_permutevar8x32_ps(_mm256_loadu_ps(values.data() + first),
                    spread.before);
            };
            return {
                .upperLeft = at(upper, point),
                .upperRight = at(upper, point + 1),
                .lowerLeft = at(lower, point),
                .lowerRight = at(lower, point + 1)
            };
        }

        // **A value spread over 8 pixels from the points about them**, and how much it changes
        // over each: across and down added, as much as the pixel's square spans whichever way
        // the value runs. Where a point is not a number, the change is not either.
        [[nodiscard]] SpreadLanes spreadWithChange(const PointCorners& corners,
            const __m256 downShare, const PointSpread& spread, const __m256 perPixel)
        {
            const __m256 left = _mm256_fmadd_ps(_mm256_sub_ps(corners.lowerLeft,
                corners.upperLeft), downShare, corners.upperLeft);
            const __m256 right = _mm256_fmadd_ps(_mm256_sub_ps(corners.lowerRight,
                corners.upperRight), downShare, corners.upperRight);
            const __m256 top = _mm256_fmadd_ps(_mm256_sub_ps(corners.upperRight,
                corners.upperLeft), spread.shares, corners.upperLeft);
            const __m256 bottom = _mm256_fmadd_ps(_mm256_sub_ps(corners.lowerRight,
                corners.lowerLeft), spread.shares, corners.lowerLeft);
            const __m256 signBit = _mm256_set1_ps(-0.0f);
            const __m256 across = _mm256_andnot_ps(signBit, _mm256_sub_ps(right, left));
            const __m256 downward = _mm256_andnot_ps(signBit, _mm256_sub_ps(bottom, top));
            return {
                .values = _mm256_fmadd_ps(_mm256_sub_ps(right, left), spread.shares, left),
                .change = _mm256_mul_ps(_mm256_add_ps(across, downward), perPixel)
            };
        }

        // 8 pixels of a row from column `x` on, the last group stopping at the row's end.
        void storeEight(Color* pixels, const int x, const int width, const __m256i color)
        {
            int* out = reinterpret_cast<int*>(pixels + x);
            if (width - x >= 8)
            {
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), color);
                return;
            }
            const __m256i inRow = _mm256_cmpgt_epi32(_mm256_set1_epi32(width - x),
                _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
            _mm256_maskstore_epi32(out, inRow, color);
        }

        // **A row of cloud pixels, premultiplied, in one pass**: each layer's cover and haze are
        // spread from the points about each pixel, the look cuts the cover, and the night's shade
        // is laid at the opacity the layers leave. A pixel's cover lies between those of its
        // points, and a stylized edge softens no further than they differ, so 8 pixels whose
        // points all fall short of what a layer's look shows skip that layer, and 8 that no layer
        // covers are written clear. The last group of 8 stops at the row's end.
        void writeCloudRow(const CloudPicture& picture, const CloudPoints& upper,
            const CloudPoints& lower, const float downShare, const float pixelY,
            const float firstPixelX, Color* pixels, const int width)
        {
            const int step = picture.step;
            const PointSpread spread = pointSpreadAt(step);
            const __m256 down = _mm256_set1_ps(downShare);
            const bool realistic = picture.look == SceneLook::Realistic;
            const __m256 perPixel = _mm256_set1_ps(1.0f / static_cast<float>(step));
            const __m256 lighterEdge = _mm256_set1_ps(k_stylizedLeastCover
                + 0.5f * k_cloudToneFade);
            const __m256 denserEdge = _mm256_set1_ps(k_stylizedDenseCover);
            const __m256 leastSoftness = _mm256_set1_ps(0.5f * k_cloudToneFade);
            const __m256 softnessPerDifference = _mm256_mul_ps(perPixel,
                _mm256_set1_ps(2.0f * k_edgeSoftening));
            // The lanes of points the 8 pixels are spread from: their own points and, but at a
            // step of 1, the next; when stylized each lane also stands for the next point after it.
            const int pointLanes = realistic
                ? (1 << (7 / step + (step == 1 ? 1 : 2))) - 1
                : (1 << (7 / step + 1)) - 1;
            const auto anyLane = [&](__m256 lanes) {
                return (_mm256_movemask_ps(lanes) & pointLanes) != 0;
            };
            const auto everyLane = [&](__m256 lanes) {
                return (_mm256_movemask_ps(lanes) & pointLanes) == pointLanes;
            };
            std::array<__m256, k_cloudLayers.size()> opacities{};
            for (std::size_t i = 0; i != opacities.size(); ++i)
                opacities[i] = _mm256_set1_ps(picture.layers[i].opacity);
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 half = _mm256_set1_ps(0.5f);
            const __m256 lastShade = _mm256_set1_ps(static_cast<float>(picture.shades.size() - 1));
            const __m256 rowStart = _mm256_add_ps(_mm256_set1_ps(firstPixelX), laneOffsets());
            const __m256 byteMax = _mm256_set1_ps(255.0f);
            const __m256i byteMask = _mm256_set1_epi32(0xFF);
            const int* shades = reinterpret_cast<const int*>(picture.shades.data());
            const auto premultiplied = [&](__m256i shade, int shift, __m256 alpha) {
                const __m256 channel = _mm256_cvtepi32_ps(
                    _mm256_and_si256(_mm256_srli_epi32(shade, shift), byteMask));
                return _mm256_slli_epi32(
                    _mm256_cvttps_epi32(_mm256_fmadd_ps(channel, alpha, half)), shift);
            };
            for (int x = 0; x < width; x += 8)
            {
                const std::size_t point = static_cast<std::size_t>(x / step);
                // **What a layer's look shows at the 8 pixels**, or nothing where none of them
                // shows it. Each lane of points and the next point after it bound the cover of the
                // pixels between them, and a stylized edge softens no further than twice what
                // they differ by, so where the bounds lie inside one tone it is laid whole.
                const auto shownBy = [&](const LayerPoints& upperLayer,
                    const LayerPoints& lowerLayer) -> std::optional<__m256> {
                    const __m256 upperCovers = _mm256_loadu_ps(upperLayer.covers.data() + point);
                    const __m256 lowerCovers = _mm256_loadu_ps(lowerLayer.covers.data() + point);
                    if (realistic)
                    {
                        if (!anyLane(_mm256_cmp_ps(_mm256_max_ps(upperCovers, lowerCovers),
                            _mm256_setzero_ps(), _CMP_GT_OQ)))
                        {
                            return std::nullopt;
                        }
                        return pixelsBetween(upperCovers, lowerCovers, down, spread);
                    }
                    const __m256 nextUpper = _mm256_loadu_ps(upperLayer.covers.data() + point + 1);
                    const __m256 nextLower = _mm256_loadu_ps(lowerLayer.covers.data() + point + 1);
                    const __m256 top = _mm256_max_ps(_mm256_max_ps(upperCovers, lowerCovers),
                        _mm256_max_ps(nextUpper, nextLower));
                    const __m256 bottom = _mm256_min_ps(_mm256_min_ps(upperCovers, lowerCovers),
                        _mm256_min_ps(nextUpper, nextLower));
                    const __m256 softest = _mm256_max_ps(_mm256_mul_ps(
                        _mm256_sub_ps(top, bottom), softnessPerDifference), leastSoftness);
                    const __m256 highest = _mm256_add_ps(top, softest);
                    const __m256 lowest = _mm256_sub_ps(bottom, softest);
                    if (!anyLane(_mm256_cmp_ps(highest, lighterEdge, _CMP_GT_OQ)))
                        return std::nullopt;
                    if (everyLane(_mm256_cmp_ps(lowest, denserEdge, _CMP_GE_OQ)))
                        return _mm256_set1_ps(1.0f);
                    if (everyLane(_mm256_and_ps(_mm256_cmp_ps(lowest, lighterEdge, _CMP_GE_OQ),
                        _mm256_cmp_ps(highest, denserEdge, _CMP_LE_OQ))))
                    {
                        return _mm256_set1_ps(k_lighterCloudShare);
                    }
                    return stylizedCoverShown(spreadWithChange(cornersAround(upperLayer.covers,
                        lowerLayer.covers, point, spread), down, spread, perPixel));
                };
                __m256 transparency = one;
                bool covered = false;
                for (std::size_t i = 0; i != upper.size(); ++i)
                {
                    const std::optional<__m256> shown = shownBy(upper[i], lower[i]);
                    if (!shown)
                        continue;
                    covered = true;
                    const __m256 clearness = pixelsBetween(
                        _mm256_loadu_ps(upper[i].clearness.data() + point),
                        _mm256_loadu_ps(lower[i].clearness.data() + point), down, spread);
                    const __m256 hidden = _mm256_mul_ps(_mm256_mul_ps(opacities[i], *shown),
                        clearness);
                    transparency = _mm256_fnmadd_ps(hidden, transparency, transparency);
                }
                if (!covered)
                {
                    storeEight(pixels, x, width, _mm256_setzero_si256());
                    continue;
                }

                const __m256 pixelX = _mm256_add_ps(rowStart,
                    _mm256_set1_ps(static_cast<float>(x)));
                const __m256 alpha = _mm256_sub_ps(one, transparency);
                const __m256 night = nightDepths(picture.night, pixelX, pixelY);
                const __m256 shadeIndex = _mm256_min_ps(_mm256_max_ps(
                    _mm256_fmadd_ps(night, lastShade, half), _mm256_setzero_ps()), lastShade);
                const __m256i shade = _mm256_i32gather_epi32(shades,
                    _mm256_cvttps_epi32(shadeIndex), 4);
                const __m256i alphaByte = _mm256_slli_epi32(
                    _mm256_cvttps_epi32(_mm256_fmadd_ps(alpha, byteMax, half)), 24);
                const __m256i color = _mm256_or_si256(
                    _mm256_or_si256(premultiplied(shade, 0, alpha), premultiplied(shade, 8, alpha)),
                    _mm256_or_si256(premultiplied(shade, 16, alpha), alphaByte));
                storeEight(pixels, x, width, color);
            }
        }

        // **The band's bitmap spans the columns the highest layer covers in its widest row**; a
        // band that misses the sphere is left empty. The points stand on the canvas's columns and
        // rows a step apart, so neighbouring bands blend from the same points, and the cover is
        // blended before the look cuts it, so the cut's edge stays sharp. Answers where the
        // bitmap stands, in pixels.
        [[nodiscard]] FloatPoint rasterizeCloudBand(const CloudPicture& picture, const int top,
            const int rows, Bitmap& pixels)
        {
            const float nearestRow = std::clamp(picture.sphereCenter.y,
                static_cast<float>(top) + 0.5f, static_cast<float>(top + rows) - 0.5f);
            const float rise = (nearestRow - picture.sphereCenter.y) / picture.sphereRadii.y;
            const float halfWidth = rise * rise < 1.0f
                ? picture.sphereRadii.x * std::sqrt(1.0f - rise * rise)
                : 0.0f;
            const int step = picture.step;
            const int sphereLeft = static_cast<int>(std::floor(picture.sphereCenter.x - halfWidth));
            const int left = std::max(picture.left, sphereLeft) / step * step;
            const int right = std::min(picture.right,
                static_cast<int>(std::ceil(picture.sphereCenter.x + halfWidth)));
            if (left >= right)
            {
                pixels.resize({});
                return {};
            }

            const int width = right - left;
            const float firstPixelX = static_cast<float>(left) + 0.5f;
            pixels.resize({ width, rows });
            const auto pixelYAt = [&](int bandRow) {
                return static_cast<float>(top + bandRow) + 0.5f;
            };
            // The last group of 8 pixels reads 8 points from the one under its first pixel.
            const std::size_t pointLength =
                groupedLength(static_cast<int>(groupedLength(width)) / step + 8);
            MapPoints onMap = {
                .longitudes = std::vector<float>(pointLength),
                .latitudes = std::vector<float>(pointLength),
                .longitudeSpans = std::vector<float>(pointLength),
                .latitudeSpans = std::vector<float>(pointLength)
            };
            const auto pointsOfRow = [&]() {
                CloudPoints points;
                for (LayerPoints& layer : points)
                {
                    layer.covers.resize(pointLength);
                    layer.clearness.resize(pointLength);
                }
                return points;
            };

            // At a step of 1 every row is a row of points, and the row below gives the change
            // down each pixel.
            CloudPoints upper = pointsOfRow();
            CloudPoints lower = pointsOfRow();
            samplePoints(picture, pixelYAt(0), firstPixelX, onMap, upper);
            samplePoints(picture, pixelYAt(step), firstPixelX, onMap, lower);
            for (int y = 0; y != rows; ++y)
            {
                const int pastPoints = y % step;
                if (y != 0 && pastPoints == 0)
                {
                    std::swap(upper, lower);
                    samplePoints(picture, pixelYAt(y + step), firstPixelX, onMap, lower);
                }
                const float downShare = static_cast<float>(pastPoints) / static_cast<float>(step);
                writeCloudRow(picture, upper, lower, downShare, pixelYAt(y), firstPixelX,
                    pixels.pixel(0, y), width);
            }
            return { static_cast<float>(left), static_cast<float>(top) };
        }

        // What every band of the night is worked out from, this frame.
        struct NightPicture
        {
            NightView night;
            std::span<const Color> shades; // premultiplied, from day to deep night
            float edgeSoftness; // the pixels over which the limb's edge goes from clear to whole
            int left; // the first column in view
            int right; // the column past the last in view
        };

        // **A band of the night, premultiplied**: 8 pixels the night has not reached are written
        // clear, and those by the limb fade out across it, as the bands are laid in past any
        // clip's edge. The bitmap spans the sphere's widest row in the band; answers where it
        // stands.
        [[nodiscard]] FloatPoint rasterizeNightBand(const NightPicture& picture, const int top,
            const int rows, Bitmap& pixels)
        {
            const NightView& night = picture.night;
            const FloatPoint radii = { 1.0f / night.inverseRadii.x, 1.0f / night.inverseRadii.y };
            const float nearestRow = std::clamp(night.center.y, static_cast<float>(top) + 0.5f,
                static_cast<float>(top + rows) - 0.5f);
            const float rise = (nearestRow - night.center.y) / radii.y;
            const float halfWidth = rise * rise < 1.0f ? radii.x * std::sqrt(1.0f - rise * rise)
                : 0.0f;
            const int left = std::max(picture.left,
                static_cast<int>(std::floor(night.center.x - halfWidth)));
            const int right = std::min(picture.right,
                static_cast<int>(std::ceil(night.center.x + halfWidth)));
            if (left >= right)
            {
                pixels.resize({});
                return {};
            }
            const int width = right - left;
            pixels.resize({ width, rows });
            const int* shades = reinterpret_cast<const int*>(picture.shades.data());
            const __m256 lastShade = _mm256_set1_ps(static_cast<float>(picture.shades.size() - 1));
            const __m256 half = _mm256_set1_ps(0.5f);
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256i byteMask = _mm256_set1_epi32(0xFF);
            const __m256 rowStart = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(left) + 0.5f),
                laneOffsets());
            const float leastRadius = std::min(radii.x, radii.y);
            const __m256 edgeScale = _mm256_set1_ps(leastRadius / picture.edgeSoftness);
            const auto faded = [&](__m256i shade, int shift, __m256 share) {
                const __m256 channel = _mm256_cvtepi32_ps(
                    _mm256_and_si256(_mm256_srli_epi32(shade, shift), byteMask));
                return _mm256_slli_epi32(
                    _mm256_cvttps_epi32(_mm256_fmadd_ps(channel, share, half)), shift);
            };
            for (int y = 0; y != rows; ++y)
            {
                const float pixelY = static_cast<float>(top + y) + 0.5f;
                const float up = (night.center.y - pixelY) * night.inverseRadii.y;
                Color* row = pixels.pixel(0, y);
                for (int x = 0; x < width; x += 8)
                {
                    const __m256 pixelX = _mm256_add_ps(rowStart,
                        _mm256_set1_ps(static_cast<float>(x)));
                    const __m256 depth = nightDepths(night, pixelX, pixelY);
                    if (_mm256_movemask_ps(_mm256_cmp_ps(depth, _mm256_setzero_ps(),
                        _CMP_GT_OQ)) == 0)
                    {
                        storeEight(row, x, width, _mm256_setzero_si256());
                        continue;
                    }
                    const __m256i shade = _mm256_i32gather_epi32(shades, _mm256_cvttps_epi32(
                        _mm256_fmadd_ps(depth, lastShade, half)), 4);
                    const __m256 across = _mm256_mul_ps(_mm256_sub_ps(pixelX,
                        _mm256_set1_ps(night.center.x)), _mm256_set1_ps(night.inverseRadii.x));
                    const __m256 reach = _mm256_sqrt_ps(_mm256_fmadd_ps(across, across,
                        _mm256_set1_ps(up * up)));
                    const __m256 inside = _mm256_min_ps(_mm256_max_ps(_mm256_fmadd_ps(
                        _mm256_sub_ps(one, reach), edgeScale, half), _mm256_setzero_ps()), one);
                    if (_mm256_movemask_ps(_mm256_cmp_ps(inside, one, _CMP_LT_OQ)) == 0)
                    {
                        storeEight(row, x, width, shade);
                        continue;
                    }
                    storeEight(row, x, width, _mm256_or_si256(
                        _mm256_or_si256(faded(shade, 0, inside), faded(shade, 8, inside)),
                        _mm256_or_si256(faded(shade, 16, inside), faded(shade, 24, inside))));
                }
            }
            return { static_cast<float>(left), static_cast<float>(top) };
        }

        // A straight piece of a coastline, in the canvas's pixels.
        struct CoastEdge
        {
            FloatPoint from;
            FloatPoint to;
        };

        // A coast's edges in the bands of rows they reach into, an edge in each band it reaches.
        struct BandedCoast
        {
            std::vector<CoastEdge> edges; // band by band
            std::vector<std::size_t> starts; // where each band's edges start, then the end
        };

        // The first and the last band of rows an edge reaches into.
        struct BandRange
        {
            int first;
            int last;
        };

        // The bands an edge reaches into, of `bandCount` bands from row `top` on, each taken
        // `rowsBeyond` rows taller above and below. None when `last` comes before `first`.
        [[nodiscard]] BandRange bandsReachedBy(const CoastEdge& edge, const int top,
            const int bandCount, const int rowsBeyond)
        {
            const float bandHeight = static_cast<float>(k_bandRows);
            const float upper = std::min(edge.from.y, edge.to.y) - static_cast<float>(top)
                - static_cast<float>(rowsBeyond);
            const float lower = std::max(edge.from.y, edge.to.y) - static_cast<float>(top)
                + static_cast<float>(rowsBeyond);
            return {
                std::max(0, static_cast<int>(std::floor(upper / bandHeight))),
                std::min(bandCount - 1, static_cast<int>(std::floor(lower / bandHeight)))
            };
        }

        // Counted first, so each band's edges land together in one vector. A band takes the edges
        // of `rowsBeyond` rows above and below it too.
        [[nodiscard]] BandedCoast bandedCoast(const std::span<const CoastEdge> coast,
            const int top, const int bandCount, const int rowsBeyond)
        {
            BandedCoast banded{ .edges = {}, .starts = {} };
            banded.starts.assign(static_cast<std::size_t>(bandCount) + 1, 0);
            for (const CoastEdge& edge : coast)
            {
                const BandRange bands = bandsReachedBy(edge, top, bandCount, rowsBeyond);
                for (int band = bands.first; band <= bands.last; ++band)
                    ++banded.starts[static_cast<std::size_t>(band) + 1];
            }
            for (std::size_t band = 1; band != banded.starts.size(); ++band)
                banded.starts[band] += banded.starts[band - 1];
            banded.edges.resize(banded.starts.back());
            std::vector<std::size_t> nextSlots(banded.starts.begin(), banded.starts.end() - 1);
            for (const CoastEdge& edge : coast)
            {
                const BandRange bands = bandsReachedBy(edge, top, bandCount, rowsBeyond);
                for (int band = bands.first; band <= bands.last; ++band)
                    banded.edges[nextSlots[static_cast<std::size_t>(band)]++] = edge;
            }
            return banded;
        }

        // A direction with the Earth's axis upright: x across, y up the axis, z toward the viewer.
        struct AxisDirection
        {
            float x;
            float y;
            float z;
        };

        // The ground's points along one row, `step` pixels apart, and the land at them.
        struct GroundPoints
        {
            std::vector<float> longitudes;
            std::vector<float> latitudes;
            std::vector<float> longitudeSpans; // the longitudes the ground about a point spans
            std::vector<float> latitudeSpans; // the latitudes it spans
            std::vector<float> sunlight; // how high the Sun stands: the cosine of its angle down
            std::vector<float> sunEast; // how far the Sun lies east along the ground
            std::vector<float> sunNorth; // how far it lies north
            std::vector<float> depths; // how far the ground faces the viewer
            std::vector<float> heights;
            std::vector<float> slopesEast;
            std::vector<float> slopesNorth;
            std::vector<float> variation;
            std::vector<float> dryness;
            std::vector<float> forest;
            std::vector<float> tundra;
            std::vector<float> red; // the colour at the point, or the stylized zone's
            std::vector<float> green;
            std::vector<float> blue;
            std::vector<float> snowEdges; // where the stylized snow begins
            std::vector<float> zones; // which stylized zones a point lies in whole - see zoneKey
            std::vector<float> light; // how the slope is lit - see reliefLight
            std::vector<float> haze; // how far the air veils the point - see airHazeAt
        };

        // What every band of the land is worked out from, this frame.
        struct LandPicture
        {
            const EarthMap* map;
            SphereView ground;
            SceneLook look;
            int step; // the pixels between the points the land is worked out at, 1, 2 or 4
            AxisDirection sun;
            FloatPoint sphereCenter; // in pixels
            FloatPoint sphereRadii; // across and up, in pixels
            int left; // the first column in view
            int right; // the column past the last in view
            float coastBlur; // the spread of the coast's Gaussian blur in pixels; 0 for none
        };

        // How far 8 points or pixels lie past the edge of each stylized zone, 0 to 1.
        struct ZoneCuts
        {
            __m256 steppe;
            __m256 sand;
            __m256 forest;
            __m256 tundra;
        };

        // The land at 8 points.
        struct GroundLanes
        {
            __m256 heights;
            __m256 slopesEast;
            __m256 slopesNorth;
            __m256 variation;
            __m256 dryness;
            __m256 forest;
            __m256 tundra;
            __m256 latitudes;
            __m256 sunlight;
            __m256 sunEast;
            __m256 sunNorth;
            __m256 depths;
        };

        // The red, green and blue of 8 pixels, 0 to 1.
        struct ColorLanes
        {
            __m256 red;
            __m256 green;
            __m256 blue;
        };

        // The land's colours, each ready for 8 pixels.
        struct LandPalette
        {
            ColorLanes grass;
            ColorLanes grassLight;
            ColorLanes steppe;
            ColorLanes sand;
            ColorLanes sandLight;
            ColorLanes taiga;
            ColorLanes taigaDeep;
            ColorLanes tundra;
            ColorLanes tundraMoss;
            ColorLanes rock;
            ColorLanes rockHigh;
            ColorLanes snow;
            ColorLanes slopeShadow;
            ColorLanes slopeLight;
            ColorLanes flatGrass;
            ColorLanes flatSteppe;
            ColorLanes flatSand;
            ColorLanes flatTaiga;
            ColorLanes flatTundra;
            ColorLanes flatRock;
            ColorLanes flatRockHigh;
        };

        // **An edge's piece within one cell's width**, adding `winding` in all to the cells right
        // of it: the cells it crosses take the share of each that lies right of it, and the cell
        // past the last takes the rest. `fromX` and `toX` lie within the row's cells.
        void sweepPiece(float* cells, const float fromX, const float toX, const float winding)
        {
            const float low = std::min(fromX, toX);
            const float high = std::max(fromX, toX);
            const float lowFloor = std::floor(low);
            const int lowCell = static_cast<int>(lowFloor);
            const float highCeiling = std::ceil(high);
            const int highCell = static_cast<int>(highCeiling);
            if (highCell <= lowCell + 1)
            {
                const float middle = 0.5f * (fromX + toX) - lowFloor;
                cells[lowCell] += winding * (1.0f - middle);
                cells[lowCell + 1] += winding * middle;
                return;
            }
            const float perAcross = 1.0f / (high - low);
            const float lowShare = low - lowFloor;
            const float first = 0.5f * perAcross * (1.0f - lowShare) * (1.0f - lowShare);
            const float highShare = high - highCeiling + 1.0f;
            const float last = 0.5f * perAcross * highShare * highShare;
            cells[lowCell] += winding * first;
            if (highCell == lowCell + 2)
            {
                cells[lowCell + 1] += winding * (1.0f - first - last);
            }
            else
            {
                const float second = perAcross * (1.5f - lowShare);
                cells[lowCell + 1] += winding * (second - first);
                for (int cell = lowCell + 2; cell < highCell - 1; ++cell)
                    cells[cell] += winding * perAcross;
                const float beforeLast = second
                    + static_cast<float>(highCell - lowCell - 3) * perAcross;
                cells[highCell - 1] += winding * (1.0f - beforeLast - last);
            }
            cells[highCell] += winding * last;
        }

        // **An edge's piece across one row**, from `fromX` at its top to `toX` at its bottom. The
        // part left of the first cell counts whole there, as the edge stands left of every cell,
        // and the part right of `width` counts nowhere.
        void sweepRow(float* cells, float fromX, float toX, float winding, const int width)
        {
            const float right = static_cast<float>(width);
            if (std::min(fromX, toX) >= right)
                return;
            if (std::max(fromX, toX) <= 0.0f)
            {
                cells[0] += winding;
                return;
            }
            // The edge runs straight down the row, so its height on either side of a column is
            // the share of its width on that side.
            if (std::min(fromX, toX) < 0.0f)
            {
                const float leftShare = -std::min(fromX, toX) / std::abs(toX - fromX);
                cells[0] += winding * leftShare;
                winding *= 1.0f - leftShare;
                if (fromX < toX)
                    fromX = 0.0f;
                else
                    toX = 0.0f;
            }
            if (std::max(fromX, toX) > right)
            {
                const float rightShare = (std::max(fromX, toX) - right) / std::abs(toX - fromX);
                winding *= 1.0f - rightShare;
                if (fromX > toX)
                    fromX = right;
                else
                    toX = right;
            }
            sweepPiece(cells, fromX, toX, winding);
        }

        // What a group of 8 pixels in a row of coverage holds.
        enum class GroupCover : std::uint8_t
        {
            Clear,
            Whole,
            Partly
        };

        // What 8 pixels of coverage hold, a pixel short of whole by the adding's drift being whole.
        [[nodiscard]] GroupCover groupCoverOf(const __m256 cells)
        {
            const __m256 clear = _mm256_cmp_ps(cells, _mm256_setzero_ps(), _CMP_EQ_OQ);
            if (_mm256_movemask_ps(clear) == 0xFF)
                return GroupCover::Clear;
            const __m256 whole = _mm256_cmp_ps(cells, _mm256_set1_ps(1.0f - 1.0f / 512.0f),
                _CMP_GE_OQ);
            if (_mm256_movemask_ps(whole) == 0xFF)
                return GroupCover::Whole;
            return GroupCover::Partly;
        }

        // The covers of 32 groups from `covers` on, a byte each.
        [[nodiscard]] __m256i coversAt(const GroupCover* covers)
        {
            return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(covers));
        }

        // **The groups a blur has to work out**, of the 32 from `covers` on, as a byte mask: those
        // partly covered, and those whose cover differs from that of any of the `count` groups
        // `step` apart after them. The rest are clear or whole, as is all they are blurred with.
        [[nodiscard]] __m256i unevenGroups(const GroupCover* covers, const std::ptrdiff_t step,
            const int count)
        {
            const __m256i first = coversAt(covers);
            __m256i uneven = _mm256_cmpeq_epi8(first,
                _mm256_set1_epi8(static_cast<char>(GroupCover::Partly)));
            for (int next = 1; next <= count; ++next)
            {
                const __m256i same = _mm256_cmpeq_epi8(first, coversAt(covers + next * step));
                uneven = _mm256_or_si256(uneven, _mm256_xor_si256(same, _mm256_set1_epi8(-1)));
            }
            return uneven;
        }

        // **How much of each pixel of a band the coastlines enclose**: each edge adds the area it
        // sweeps in each cell of every row it crosses, signed by the way it runs, and summing along
        // the row gives the share enclosed. The shapes stand apart, and a sea inside one runs the
        // other way round, so the sum's size is the coverage whichever way a shape runs.
        // `coverage` holds the band's rows `stride` apart, each with 2 floats to spare past
        // `width`; `groupCovers` holds what each 8 pixels of a row hold.
        void coverBand(const std::span<const CoastEdge> coast, const int top, const int rows,
            const int left, const int width, const int stride, const std::span<float> coverage,
            std::vector<GroupCover>& groupCovers)
        {
            std::ranges::fill(coverage, 0.0f);
            const float bandTop = static_cast<float>(top);
            const float bandBottom = static_cast<float>(top + rows);
            for (const CoastEdge& edge : coast)
            {
                const bool downward = edge.from.y < edge.to.y;
                const FloatPoint upper = downward ? edge.from : edge.to;
                const FloatPoint lower = downward ? edge.to : edge.from;
                if (upper.y >= bandBottom || lower.y <= bandTop || upper.y == lower.y)
                    continue;
                const float winding = downward ? 1.0f : -1.0f;
                const float acrossPerRow = (lower.x - upper.x) / (lower.y - upper.y);
                const int firstRow = std::max(top, static_cast<int>(std::floor(upper.y)));
                const int endRow = std::min(top + rows, static_cast<int>(std::ceil(lower.y)));
                for (int row = firstRow; row < endRow; ++row)
                {
                    const float rowTop = std::max(static_cast<float>(row), upper.y);
                    const float rowBottom = std::min(static_cast<float>(row + 1), lower.y);
                    if (rowBottom <= rowTop)
                        continue;
                    const float fromX = upper.x + (rowTop - upper.y) * acrossPerRow
                        - static_cast<float>(left);
                    const float toX = upper.x + (rowBottom - upper.y) * acrossPerRow
                        - static_cast<float>(left);
                    sweepRow(coverage.data() + static_cast<std::ptrdiff_t>(row - top) * stride,
                        fromX, toX, winding * (rowBottom - rowTop), width);
                }
            }

            // Summed 8 cells at a time: within each half, then the lower half's last into the
            // upper half, then what the cells before have come to. A sum short of a 512th is the
            // drift of the adding, not land.
            const int groupsPerRow = stride / 8;
            groupCovers.assign(static_cast<std::size_t>(groupsPerRow) * rows, GroupCover::Clear);
            const __m256 signBit = _mm256_set1_ps(-0.0f);
            const __m256 whole = _mm256_set1_ps(1.0f);
            const __m256 least = _mm256_set1_ps(1.0f / 512.0f);
            for (int row = 0; row != rows; ++row)
            {
                float* cells = coverage.data() + static_cast<std::ptrdiff_t>(row) * stride;
                __m256 carried = _mm256_setzero_ps();
                for (int group = 0; group != groupsPerRow; ++group)
                {
                    __m256 sums = _mm256_loadu_ps(cells + group * 8);
                    sums = _mm256_add_ps(sums, _mm256_castsi256_ps(
                        _mm256_slli_si256(_mm256_castps_si256(sums), 4)));
                    sums = _mm256_add_ps(sums, _mm256_castsi256_ps(
                        _mm256_slli_si256(_mm256_castps_si256(sums), 8)));
                    const __m256 halfLasts = _mm256_permute_ps(sums, 0xFF);
                    sums = _mm256_add_ps(sums, _mm256_permute2f128_ps(halfLasts, halfLasts, 0x08));
                    sums = _mm256_add_ps(sums, carried);
                    carried = _mm256_permute2f128_ps(_mm256_permute_ps(sums, 0xFF),
                        _mm256_permute_ps(sums, 0xFF), 0x11);
                    const __m256 covered = _mm256_min_ps(_mm256_andnot_ps(signBit, sums), whole);
                    const __m256 counted = _mm256_and_ps(covered,
                        _mm256_cmp_ps(covered, least, _CMP_GE_OQ));
                    _mm256_storeu_ps(cells + group * 8, counted);
                    groupCovers[static_cast<std::size_t>(row) * groupsPerRow
                        + static_cast<std::size_t>(group)] = groupCoverOf(counted);
                }
            }
        }

        // The pixels a Gaussian blur of a spread reaches to either side.
        [[nodiscard]] int blurReachOf(const float spread)
        {
            return static_cast<int>(std::ceil(3.0f * spread));
        }

        // A Gaussian's weights from `-reach` to `reach` pixels, summing to 1.
        [[nodiscard]] std::vector<float> gaussianWeights(const float spread, const int reach)
        {
            std::vector<float> weights;
            weights.reserve(static_cast<std::size_t>(2 * reach + 1));
            float sum = 0.0f;
            for (int offset = -reach; offset <= reach; ++offset)
            {
                const float distance = static_cast<float>(offset) / spread;
                weights.push_back(std::exp(-0.5f * distance * distance));
                sum += weights.back();
            }
            for (float& weight : weights)
                weight /= sum;
            return weights;
        }

        // **The coast is blurred as much as the land is**: the coverage of the band and of the
        // blur's reach beyond it on every side, blurred across and then down by a Gaussian of
        // `picture.coastBlur`. Only the groups unevenGroups finds are worked out, so the blur
        // costs by the length of the coast. The loops along the rim are closed past the sphere, so
        // the blur leaves the limb whole, and the limb is cut here by the sphere's own coverage,
        // as the sea's is. Fills `coverage` and `groupCovers` as coverBand does.
        void coverBlurredBand(const LandPicture& picture, const std::span<const CoastEdge> coast,
            const int top, const int rows, const int left, const int width, const int stride,
            const std::span<float> coverage, std::vector<GroupCover>& groupCovers)
        {
            const int reach = blurReachOf(picture.coastBlur);
            const std::vector<float> weights = gaussianWeights(picture.coastBlur, reach);
            const int wideRows = rows + 2 * reach;
            const int wideWidth = width + 2 * reach;
            const int wideStride = static_cast<int>(groupedLength(wideWidth + 2));
            const std::size_t wideLength = static_cast<std::size_t>(wideStride)
                * static_cast<std::size_t>(wideRows);
            // The blur across reads up to 8 floats past the last row.
            NoAllocFloatVector wide(wideLength + 8);
            std::vector<GroupCover> wideCovers;
            coverBand(coast, top - reach, wideRows, left - reach, wideWidth, wideStride,
                std::span<float>{ wide.data(), wideLength }, wideCovers);
            // The masks read 32 groups and the blur's reach past the last row.
            wideCovers.resize(wideCovers.size() + 40, GroupCover::Clear);
            const int wideGroupsPerRow = wideStride / 8;
            const int groupCount = (width + 7) / 8;
            const int groupsPerRow = stride / 8;
            const __m256i partlyBytes = _mm256_set1_epi8(static_cast<char>(GroupCover::Partly));
            const __m256i wholeBytes = _mm256_set1_epi8(static_cast<char>(GroupCover::Whole));

            // **Across**, for the band's columns: an even group takes the cover its blur reads,
            // and an uneven one holds its blurred values.
            const int groupsReadPast = (7 + 2 * reach) / 8;
            const std::size_t blurredStride = static_cast<std::size_t>(stride);
            NoAllocFloatVector blurredAcross(blurredStride * static_cast<std::size_t>(wideRows));
            std::vector<GroupCover> blurredCovers(
                static_cast<std::size_t>(groupsPerRow) * static_cast<std::size_t>(wideRows) + 40);
            for (int y = 0; y != wideRows; ++y)
            {
                const float* wideRow = wide.data() + static_cast<std::ptrdiff_t>(y) * wideStride;
                const GroupCover* wideRowCovers = wideCovers.data()
                    + static_cast<std::ptrdiff_t>(y) * wideGroupsPerRow;
                float* blurredRow = blurredAcross.data()
                    + static_cast<std::ptrdiff_t>(y) * stride;
                GroupCover* blurredRowCovers = blurredCovers.data()
                    + static_cast<std::ptrdiff_t>(y) * groupsPerRow;
                for (int first = 0; first < groupCount; first += 32)
                {
                    const __m256i uneven = unevenGroups(wideRowCovers + first, 1, groupsReadPast);
                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(blurredRowCovers + first),
                        _mm256_blendv_epi8(coversAt(wideRowCovers + first), partlyBytes, uneven));
                    auto unevenBits = static_cast<std::uint32_t>(_mm256_movemask_epi8(uneven));
                    for (; unevenBits != 0; unevenBits &= unevenBits - 1)
                    {
                        const int x = 8 * (first + std::countr_zero(unevenBits));
                        if (x >= width)
                            break;
                        __m256 sum = _mm256_setzero_ps();
                        for (std::size_t tap = 0; tap != weights.size(); ++tap)
                        {
                            sum = _mm256_fmadd_ps(_mm256_set1_ps(weights[tap]),
                                _mm256_loadu_ps(wideRow + x + tap), sum);
                        }
                        _mm256_storeu_ps(blurredRow + x, sum);
                    }
                }
            }

            // **Down**, for the band's rows: a whole group adds its weight and a clear one nothing,
            // and the sphere cuts what lies outside the pixels it covers whole.
            std::ranges::fill(coverage, 0.0f);
            groupCovers.assign(static_cast<std::size_t>(groupsPerRow)
                * static_cast<std::size_t>(rows), GroupCover::Clear);
            const FloatPoint radii = picture.sphereRadii;
            const float edgeScale = std::min(radii.x, radii.y);
            // How far from the sphere's center a pixel's center may lie for it to be covered whole.
            const float wholeReach = 1.0f - 0.5f / edgeScale;
            const __m256 zero = _mm256_setzero_ps();
            const __m256 half = _mm256_set1_ps(0.5f);
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 least = _mm256_set1_ps(1.0f / 512.0f);
            const __m256 rowStart = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(left) + 0.5f),
                laneOffsets());
            for (int y = 0; y != rows; ++y)
            {
                float* cells = coverage.data() + static_cast<std::ptrdiff_t>(y) * stride;
                const float up = (picture.sphereCenter.y - (static_cast<float>(top + y) + 0.5f))
                    / radii.y;
                const float wholeSquare = wholeReach * wholeReach - up * up;
                const float wholeHalfWidth = wholeSquare > 0.0f
                    ? radii.x * std::sqrt(wholeSquare)
                    : -1.0f;
                const float wholeLeft = picture.sphereCenter.x - wholeHalfWidth;
                const float wholeRight = picture.sphereCenter.x + wholeHalfWidth;
                const GroupCover* rowCovers = blurredCovers.data()
                    + static_cast<std::ptrdiff_t>(y) * groupsPerRow;
                for (int first = 0; first < groupCount; first += 32)
                {
                    const __m256i uneven = unevenGroups(rowCovers + first, groupsPerRow,
                        2 * reach);
                    const __m256i evenWhole = _mm256_andnot_si256(uneven,
                        _mm256_cmpeq_epi8(coversAt(rowCovers + first), wholeBytes));
                    const auto unevenBits = static_cast<std::uint32_t>(
                        _mm256_movemask_epi8(uneven));
                    auto coveredBits = static_cast<std::uint32_t>(
                        _mm256_movemask_epi8(_mm256_or_si256(uneven, evenWhole)));
                    for (; coveredBits != 0; coveredBits &= coveredBits - 1)
                    {
                        const int bit = std::countr_zero(coveredBits);
                        const int x = 8 * (first + bit);
                        if (x >= width)
                            break;
                        __m256 sum = one;
                        if (((unevenBits >> bit) & 1) != 0)
                        {
                            sum = _mm256_setzero_ps();
                            for (std::size_t tap = 0; tap != weights.size(); ++tap)
                            {
                                const std::size_t row = static_cast<std::size_t>(y) + tap;
                                const std::size_t group = row
                                    * static_cast<std::size_t>(groupsPerRow)
                                    + static_cast<std::size_t>(first + bit);
                                if (blurredCovers[group] == GroupCover::Whole)
                                {
                                    sum = _mm256_add_ps(sum, _mm256_set1_ps(weights[tap]));
                                }
                                else if (blurredCovers[group] == GroupCover::Partly)
                                {
                                    sum = _mm256_fmadd_ps(_mm256_set1_ps(weights[tap]),
                                        _mm256_loadu_ps(blurredAcross.data()
                                            + row * blurredStride
                                            + static_cast<std::size_t>(x)), sum);
                                }
                            }
                        }
                        const float firstCenter = static_cast<float>(left + x) + 0.5f;
                        if (firstCenter < wholeLeft || firstCenter + 7.0f > wholeRight)
                        {
                            const __m256 across = _mm256_mul_ps(_mm256_sub_ps(_mm256_add_ps(
                                rowStart, _mm256_set1_ps(static_cast<float>(x))),
                                _mm256_set1_ps(picture.sphereCenter.x)),
                                _mm256_set1_ps(1.0f / radii.x));
                            const __m256 reachOnSphere = _mm256_sqrt_ps(_mm256_fmadd_ps(across,
                                across, _mm256_set1_ps(up * up)));
                            const __m256 onSphere = _mm256_min_ps(_mm256_max_ps(_mm256_fmadd_ps(
                                _mm256_sub_ps(one, reachOnSphere), _mm256_set1_ps(edgeScale),
                                half), zero), one);
                            sum = _mm256_mul_ps(sum, onSphere);
                        }
                        const __m256 covered = _mm256_min_ps(sum, one);
                        const __m256 counted = _mm256_and_ps(covered,
                            _mm256_cmp_ps(covered, least, _CMP_GE_OQ));
                        _mm256_storeu_ps(cells + x, counted);
                        groupCovers[static_cast<std::size_t>(y * groupsPerRow + first + bit)]
                            = groupCoverOf(counted);
                    }
                }
            }
        }

        // Where the points `first` to `end` of a row fall on the ground and how far the ground
        // about each spans, how the Sun stands over each and how far each faces the viewer, the
        // row's points standing from the pixel center `firstPixelX` on.
        void projectGroundRow(const LandPicture& picture, const float pixelY,
            const float firstPixelX, const std::size_t first, const std::size_t end,
            GroundPoints& points)
        {
            const SphereRow row = sphereRowAt(picture.ground, pixelY, firstPixelX, picture.step);
            const __m256 spin = _mm256_set1_ps(picture.ground.spin);
            const __m256 sunX = _mm256_set1_ps(picture.sun.x);
            const __m256 sunY = _mm256_set1_ps(picture.sun.y);
            const __m256 sunZ = _mm256_set1_ps(picture.sun.z);
            const FloatPoint perPoint = pointSpanOf(picture.ground, picture.step);
            // At the pole east and north turn about the axis, and any pair of them will do.
            const __m256 leastAroundAxis = _mm256_set1_ps(1.0e-4f);
            for (std::size_t i = first; i != end; i += 8)
            {
                const SphereLanes lanes = sphereLanesAt(row, i);
                _mm256_storeu_ps(points.longitudes.data() + i,
                    _mm256_sub_ps(quickAngle(lanes.x, lanes.z), spin));
                _mm256_storeu_ps(points.latitudes.data() + i,
                    quickAngle(lanes.y, lanes.aroundAxis));
                const __m256 aroundAxis = _mm256_max_ps(lanes.aroundAxis, leastAroundAxis);
                const PointSpans spans = spansAt(row, lanes, aroundAxis, perPoint);
                _mm256_storeu_ps(points.longitudeSpans.data() + i, spans.longitudes);
                _mm256_storeu_ps(points.latitudeSpans.data() + i, spans.latitudes);
                const __m256 sunAcross = _mm256_fmadd_ps(lanes.x, sunX,
                    _mm256_mul_ps(lanes.z, sunZ));
                _mm256_storeu_ps(points.sunlight.data() + i,
                    _mm256_fmadd_ps(lanes.y, sunY, sunAcross));
                _mm256_storeu_ps(points.sunEast.data() + i, _mm256_div_ps(
                    _mm256_fmsub_ps(lanes.z, sunX, _mm256_mul_ps(lanes.x, sunZ)), aroundAxis));
                _mm256_storeu_ps(points.sunNorth.data() + i, _mm256_fmsub_ps(aroundAxis, sunY,
                    _mm256_div_ps(_mm256_mul_ps(lanes.y, sunAcross), aroundAxis)));
                _mm256_storeu_ps(points.depths.data() + i, lanes.depth);
            }
        }

        [[nodiscard]] ColorLanes colorLanesOf(const Color color)
        {
            constexpr float k_perByte{ 1.0f / 255.0f };
            return {
                _mm256_set1_ps(static_cast<float>(color.red) * k_perByte),
                _mm256_set1_ps(static_cast<float>(color.green) * k_perByte),
                _mm256_set1_ps(static_cast<float>(color.blue) * k_perByte)
            };
        }

        [[nodiscard]] LandPalette landPalette()
        {
            using namespace EarthSceneColors;
            return {
                .grass = colorLanesOf(Grass),
                .grassLight = colorLanesOf(GrassLight),
                .steppe = colorLanesOf(Steppe),
                .sand = colorLanesOf(Sand),
                .sandLight = colorLanesOf(SandLight),
                .taiga = colorLanesOf(Taiga),
                .taigaDeep = colorLanesOf(TaigaDeep),
                .tundra = colorLanesOf(Tundra),
                .tundraMoss = colorLanesOf(TundraMoss),
                .rock = colorLanesOf(Rock),
                .rockHigh = colorLanesOf(RockHigh),
                .snow = colorLanesOf(Snow),
                .slopeShadow = colorLanesOf(SlopeShadow),
                .slopeLight = colorLanesOf(SlopeLight),
                .flatGrass = colorLanesOf(FlatGrass),
                .flatSteppe = colorLanesOf(FlatSteppe),
                .flatSand = colorLanesOf(FlatSand),
                .flatTaiga = colorLanesOf(FlatTaiga),
                .flatTundra = colorLanesOf(FlatTundra),
                .flatRock = colorLanesOf(FlatRock),
                .flatRockHigh = colorLanesOf(FlatRockHigh)
            };
        }

        // `share` of the way from `from` to `to`, in each lane.
        [[nodiscard]] ColorLanes mixed(const ColorLanes& from, const ColorLanes& to,
            const __m256 share)
        {
            return {
                _mm256_fmadd_ps(_mm256_sub_ps(to.red, from.red), share, from.red),
                _mm256_fmadd_ps(_mm256_sub_ps(to.green, from.green), share, from.green),
                _mm256_fmadd_ps(_mm256_sub_ps(to.blue, from.blue), share, from.blue)
            };
        }

        // The height snow lies above, falling toward the poles and to the ground past 66 south.
        [[nodiscard]] __m256 snowlineAt(const __m256 latitudes)
        {
            const __m256 fromEquator = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), latitudes);
            const __m256 south = _mm256_sub_ps(_mm256_setzero_ps(), latitudes);
            const __m256 temperate = _mm256_fnmadd_ps(_mm256_set1_ps(0.42f),
                smoothStep8(28.0f * k_degree, 68.0f * k_degree, fromEquator),
                _mm256_set1_ps(0.58f));
            return _mm256_fnmadd_ps(_mm256_set1_ps(0.3f),
                smoothStep8(60.0f * k_degree, 66.0f * k_degree, south), temperate);
        }

        // **How bright the ground is for its slope**, against flat ground under the same Sun: above
        // 1 where it faces the Sun. Across the terminator the slopes fade to flat.
        [[nodiscard]] __m256 reliefLight(const GroundLanes& ground)
        {
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 rise = _mm256_set1_ps(k_reliefHeight);
            const __m256 east = _mm256_mul_ps(ground.slopesEast, rise);
            const __m256 north = _mm256_mul_ps(ground.slopesNorth, rise);
            const __m256 facing = _mm256_div_ps(
                _mm256_fmadd_ps(east, ground.sunEast, _mm256_mul_ps(north, ground.sunNorth)),
                _mm256_max_ps(ground.sunlight, _mm256_set1_ps(k_lowestSunForRelief)));
            const __m256 lean = _mm256_sqrt_ps(
                _mm256_fmadd_ps(east, east, _mm256_fmadd_ps(north, north, one)));
            const __m256 light = _mm256_div_ps(_mm256_sub_ps(one, facing), lean);
            return _mm256_fmadd_ps(_mm256_sub_ps(light, one),
                smoothStep8(-0.05f, 0.05f, ground.sunlight), one);
        }

        // **Realistic**: green, steppe and sand by the dryness, taiga and tundra by the cold, each
        // patch leaning by the variation; rock above the hills, snow above the snowline, and the
        // slopes lit and shaded by the Sun.
        [[nodiscard]] ColorLanes realisticGround(const GroundLanes& ground,
            const LandPalette& palette)
        {
            const __m256 variation = ground.variation;
            ColorLanes color = mixed(palette.grass, palette.grassLight,
                smoothStep8(-0.4f, 0.4f, variation));
            color = mixed(color, palette.steppe, smoothStep8(0.22f, 0.48f, ground.dryness));
            color = mixed(color, mixed(palette.sand, palette.sandLight,
                smoothStep8(-0.3f, 0.5f, variation)), smoothStep8(0.5f, 0.78f, ground.dryness));
            color = mixed(color, mixed(palette.taiga, palette.taigaDeep,
                smoothStep8(-0.3f, 0.3f, variation)), ground.forest);
            color = mixed(color, mixed(palette.tundra, palette.tundraMoss,
                smoothStep8(-0.3f, 0.3f, variation)), ground.tundra);
            color = mixed(color, mixed(palette.rock, palette.rockHigh,
                smoothStep8(0.3f, 0.7f, ground.heights)),
                _mm256_mul_ps(_mm256_set1_ps(0.85f), smoothStep8(0.1f, 0.42f, ground.heights)));
            const __m256 snowEdge = _mm256_add_ps(snowlineAt(ground.latitudes),
                _mm256_set1_ps(0.04f));
            color = mixed(color, palette.snow, _mm256_mul_ps(_mm256_set1_ps(0.95f),
                smoothStepAround(snowEdge, softEdgeOf(_mm256_set1_ps(0.04f)), ground.heights)));

            const __m256 light = reliefLight(ground);
            const __m256 zero = _mm256_setzero_ps();
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 shaded = _mm256_min_ps(_mm256_max_ps(_mm256_sub_ps(one, light), zero),
                one);
            color = mixed(color, palette.slopeShadow, _mm256_mul_ps(_mm256_set1_ps(0.55f), shaded));
            const __m256 lit = _mm256_min_ps(_mm256_max_ps(
                _mm256_mul_ps(_mm256_set1_ps(0.45f), _mm256_sub_ps(light, one)), zero),
                _mm256_set1_ps(0.3f));
            return mixed(color, palette.slopeLight, lit);
        }

        // **Stylized zones**: grass, steppe and sand cut from the dryness, taiga and tundra from
        // the cold. An edge is softened by k_edgeSoftening of what its field changes over the
        // pixel, and no less than k_zoneFade.
        [[nodiscard]] ZoneCuts zoneCutsOf(const SpreadLanes& dryness, const SpreadLanes& forest,
            const SpreadLanes& tundra)
        {
            const __m256 softening = _mm256_set1_ps(k_edgeSoftening);
            const __m256 leastSoftness = _mm256_set1_ps(0.5f * k_zoneFade);
            const auto softEdgeFor = [&](const SpreadLanes& field) {
                return softEdgeOf(_mm256_max_ps(_mm256_mul_ps(field.change, softening),
                    leastSoftness));
            };
            const SoftEdge dry = softEdgeFor(dryness);
            return {
                .steppe = smoothStepAround(_mm256_set1_ps(0.33f), dry, dryness.values),
                .sand = smoothStepAround(_mm256_set1_ps(0.6f), dry, dryness.values),
                .forest = smoothStepAround(_mm256_set1_ps(0.5f), softEdgeFor(forest),
                    forest.values),
                .tundra = smoothStepAround(_mm256_set1_ps(0.5f), softEdgeFor(tundra),
                    tundra.values)
            };
        }

        // The flat colour of the zones, the taiga left out of the steppe.
        [[nodiscard]] ColorLanes zoneColor(const ZoneCuts& cuts, const LandPalette& palette)
        {
            ColorLanes color = mixed(palette.flatGrass, palette.flatSteppe, cuts.steppe);
            color = mixed(color, palette.flatSand, cuts.sand);
            color = mixed(color, palette.flatTaiga, _mm256_mul_ps(cuts.forest,
                _mm256_sub_ps(_mm256_set1_ps(1.0f), cuts.steppe)));
            return mixed(color, palette.flatTundra, cuts.tundra);
        }

        // **Which zones each point lies in whole**: one number for each set of them, 0 to 15,
        // and -1 for a point on an edge, where a cut is neither 0 nor 1.
        [[nodiscard]] __m256 zoneKey(const ZoneCuts& cuts)
        {
            const __m256 zero = _mm256_setzero_ps();
            const __m256 one = _mm256_set1_ps(1.0f);
            const auto whole = [&](__m256 cut) {
                return _mm256_or_ps(_mm256_cmp_ps(cut, zero, _CMP_EQ_OQ),
                    _mm256_cmp_ps(cut, one, _CMP_EQ_OQ));
            };
            const __m256 inside = _mm256_and_ps(_mm256_and_ps(whole(cuts.steppe), whole(cuts.sand)),
                _mm256_and_ps(whole(cuts.forest), whole(cuts.tundra)));
            const __m256 key = _mm256_fmadd_ps(cuts.tundra, _mm256_set1_ps(8.0f),
                _mm256_fmadd_ps(cuts.forest, _mm256_set1_ps(4.0f),
                    _mm256_fmadd_ps(cuts.sand, _mm256_set1_ps(2.0f), cuts.steppe)));
            return _mm256_blendv_ps(_mm256_set1_ps(-1.0f), key, inside);
        }

        // **Stylized bands**: flat bands cut from the broad relief over the zones, and two flat
        // tones of slope, each cut at the pixel. An edge is softened by k_edgeSoftening of what
        // the height or the light changes over the pixel, so it stays sharp where they change
        // slowly and does not step from pixel to pixel where they change fast. A band or a tone
        // none of the 8 pixels reaches is passed over, so the plains cost a compare.
        [[nodiscard]] ColorLanes stylizedRelief(ColorLanes color, const SpreadLanes& heights,
            const __m256 snowEdges, const SpreadLanes& light, const LandPalette& palette)
        {
            const __m256 softening = _mm256_set1_ps(k_edgeSoftening);
            const __m256 bandSoftness = _mm256_max_ps(_mm256_mul_ps(heights.change, softening),
                _mm256_set1_ps(1.0e-4f));
            const __m256 reach = _mm256_add_ps(heights.values, bandSoftness);
            const auto reaches = [&](__m256 edge) {
                return _mm256_movemask_ps(_mm256_cmp_ps(reach, edge, _CMP_GT_OQ)) != 0;
            };
            // The bands lie above the lowest, so 8 pixels below it reach none but the snow, which
            // lies on all the land in the far south.
            const __m256 lowestBand = _mm256_set1_ps(k_lowestBand);
            if (reaches(lowestBand) || reaches(snowEdges))
            {
                const SoftEdge bandEdge = softEdgeOf(bandSoftness);
                const auto lay = [&](const ColorLanes& tone, float share, __m256 edge) {
                    if (!reaches(edge))
                        return;
                    color = mixed(color, tone, _mm256_mul_ps(_mm256_set1_ps(share),
                        smoothStepAround(edge, bandEdge, heights.values)));
                };
                lay(palette.slopeShadow, 0.13f, lowestBand);
                lay(palette.flatRock, 0.95f, _mm256_set1_ps(0.2f));
                lay(palette.flatRockHigh, 0.9f, _mm256_set1_ps(0.42f));
                lay(palette.snow, 0.97f, snowEdges);
            }

            const __m256 toneSoftness = _mm256_max_ps(_mm256_mul_ps(light.change, softening),
                _mm256_set1_ps(0.5f * k_toneFade));
            const __m256 shadedEdge = _mm256_set1_ps(k_shadedBelow - 0.5f * k_toneFade);
            const __m256 litEdge = _mm256_set1_ps(k_litAbove + 0.5f * k_toneFade);
            const bool shaded = _mm256_movemask_ps(_mm256_cmp_ps(
                _mm256_sub_ps(light.values, toneSoftness), shadedEdge, _CMP_LT_OQ)) != 0;
            const bool lit = _mm256_movemask_ps(_mm256_cmp_ps(
                _mm256_add_ps(light.values, toneSoftness), litEdge, _CMP_GT_OQ)) != 0;
            if (!shaded && !lit)
                return color;
            const SoftEdge toneEdge = softEdgeOf(toneSoftness);
            if (shaded)
            {
                color = mixed(color, palette.slopeShadow, _mm256_mul_ps(_mm256_set1_ps(0.34f),
                    _mm256_sub_ps(_mm256_set1_ps(1.0f),
                        smoothStepAround(shadedEdge, toneEdge, light.values))));
            }
            if (lit)
            {
                color = mixed(color, palette.slopeLight, _mm256_mul_ps(_mm256_set1_ps(0.14f),
                    smoothStepAround(litEdge, toneEdge, light.values)));
            }
            return color;
        }

        // **The colour at each point**, whole in the realistic look; in the stylized one the
        // zone's, with what its bands are cut by at the pixels.
        void colorPoints(const LandPicture& picture, const LandPalette& palette,
            const std::size_t first, const std::size_t end, GroundPoints& points)
        {
            const bool realistic = picture.look == SceneLook::Realistic;
            const auto atPoints = [](__m256 values) {
                return SpreadLanes{ .values = values, .change = _mm256_setzero_ps() };
            };
            for (std::size_t i = first; i != end; i += 8)
            {
                const auto at = [&](const std::vector<float>& values) {
                    return _mm256_loadu_ps(values.data() + i);
                };
                const GroundLanes ground = {
                    .heights = at(points.heights),
                    .slopesEast = at(points.slopesEast),
                    .slopesNorth = at(points.slopesNorth),
                    .variation = at(points.variation),
                    .dryness = at(points.dryness),
                    .forest = at(points.forest),
                    .tundra = at(points.tundra),
                    .latitudes = at(points.latitudes),
                    .sunlight = at(points.sunlight),
                    .sunEast = at(points.sunEast),
                    .sunNorth = at(points.sunNorth),
                    .depths = at(points.depths)
                };
                if (realistic)
                {
                    const ColorLanes color = realisticGround(ground, palette);
                    _mm256_storeu_ps(points.red.data() + i, color.red);
                    _mm256_storeu_ps(points.green.data() + i, color.green);
                    _mm256_storeu_ps(points.blue.data() + i, color.blue);
                    continue;
                }
                const ZoneCuts cuts = zoneCutsOf(atPoints(ground.dryness), atPoints(ground.forest),
                    atPoints(ground.tundra));
                const ColorLanes color = zoneColor(cuts, palette);
                _mm256_storeu_ps(points.red.data() + i, color.red);
                _mm256_storeu_ps(points.green.data() + i, color.green);
                _mm256_storeu_ps(points.blue.data() + i, color.blue);
                _mm256_storeu_ps(points.zones.data() + i, zoneKey(cuts));
                _mm256_storeu_ps(points.snowEdges.data() + i,
                    _mm256_add_ps(snowlineAt(ground.latitudes), _mm256_set1_ps(0.03f)));
                _mm256_storeu_ps(points.light.data() + i, reliefLight(ground));
            }
        }

        // **How far the air veils the ground at 8 points, 0 to 1**, by the air the view looks
        // through to it past the air straight down, which grows as the ground turns away and thins
        // with its height. `facing` is how far the ground faces the viewer; the sea's height is 0.
        [[nodiscard]] __m256 airHazeAt(const __m256 facing, const __m256 heights)
        {
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 thinning = _mm256_mul_ps(_mm256_max_ps(heights, _mm256_setzero_ps()),
                _mm256_set1_ps(k_airThinning));
            // The share of the air left above the height: e to the minus `thinning`, near enough.
            const __m256 airLeft = _mm256_div_ps(one, _mm256_fmadd_ps(thinning, _mm256_fmadd_ps(
                thinning, _mm256_fmadd_ps(thinning, _mm256_set1_ps(1.0f / 6.0f),
                    _mm256_set1_ps(0.5f)), one), one));
            const __m256 extraAir = _mm256_mul_ps(_mm256_sub_ps(_mm256_div_ps(one,
                _mm256_max_ps(facing, _mm256_set1_ps(0.01f))), one), airLeft);
            return _mm256_div_ps(extraAir, _mm256_add_ps(extraAir,
                _mm256_set1_ps(k_halfHazeAir)));
        }

        // The ground's points along a row, the land at them in the relief the look draws, and
        // what each pixel is spread from, in the groups of 8 points `needed` marks.
        void sampleGround(const LandPicture& picture, const LandPalette& palette,
            const float pixelY, const float firstPixelX, const std::vector<bool>& needed,
            GroundPoints& points)
        {
            const Relief relief = picture.look == SceneLook::Realistic
                ? Relief::Detailed
                : Relief::Broad;
            std::size_t group = 0;
            while (group != needed.size())
            {
                if (!needed[group])
                {
                    ++group;
                    continue;
                }
                const std::size_t firstGroup = group;
                while (group != needed.size() && needed[group])
                    ++group;
                const std::size_t first = firstGroup * 8;
                const std::size_t end = group * 8;
                const auto part = [&](std::vector<float>& values) {
                    return std::span<float>{ values }.subspan(first, end - first);
                };
                projectGroundRow(picture, pixelY, firstPixelX, first, end, points);
                const MapQuery query = {
                    .longitudes = part(points.longitudes),
                    .latitudes = part(points.latitudes),
                    .longitudeSpans = part(points.longitudeSpans),
                    .latitudeSpans = part(points.latitudeSpans)
                };
                picture.map->readLand(query, relief, {
                    .heights = part(points.heights),
                    .slopesEast = part(points.slopesEast),
                    .slopesNorth = part(points.slopesNorth),
                    .variation = part(points.variation),
                    .dryness = part(points.dryness),
                    .forest = part(points.forest),
                    .tundra = part(points.tundra)
                });
                colorPoints(picture, palette, first, end, points);
                for (std::size_t i = first; i != end; i += 8)
                {
                    _mm256_storeu_ps(points.haze.data() + i, airHazeAt(
                        _mm256_loadu_ps(points.depths.data() + i),
                        _mm256_loadu_ps(points.heights.data() + i)));
                }
            }
        }

        // Whether the points about all 8 pixels lie whole in the same zones - see zoneKey.
        [[nodiscard]] bool isInsideOneZone(const PointCorners& zones)
        {
            const __m256 first = _mm256_permutevar8x32_ps(zones.upperLeft,
                _mm256_setzero_si256());
            const __m256 upperSame = _mm256_and_ps(
                _mm256_cmp_ps(zones.upperLeft, first, _CMP_EQ_OQ),
                _mm256_cmp_ps(zones.upperRight, first, _CMP_EQ_OQ));
            const __m256 lowerSame = _mm256_and_ps(
                _mm256_cmp_ps(zones.lowerLeft, first, _CMP_EQ_OQ),
                _mm256_cmp_ps(zones.lowerRight, first, _CMP_EQ_OQ));
            const __m256 whole = _mm256_cmp_ps(first, _mm256_setzero_ps(), _CMP_GE_OQ);
            return _mm256_movemask_ps(_mm256_and_ps(_mm256_and_ps(upperSame, lowerSame), whole))
                == 0xFF;
        }

        // **Which groups of 8 points of a row any covered pixel reads**: a point is read by the
        // pixels from a step before it to short of a step after, across and down, in the band.
        // `groupCovers` holds what the band's groups of 8 pixels hold, `groupsPerRow` to a row.
        void markNeededGroups(const int step, const std::span<const GroupCover> groupCovers,
            const int groupsPerRow, const int rows, const int pointRow,
            std::vector<std::uint8_t>& anyCovered, std::vector<bool>& needed)
        {
            anyCovered.assign(static_cast<std::size_t>(groupsPerRow), 0);
            const int firstRow = std::max(0, pointRow - step);
            const int lastRow = std::min(rows - 1, pointRow + step - 1);
            for (int row = firstRow; row <= lastRow; ++row)
            {
                const GroupCover* rowCovers = groupCovers.data()
                    + static_cast<std::ptrdiff_t>(row) * groupsPerRow;
                for (std::size_t group = 0; group != anyCovered.size(); ++group)
                    anyCovered[group] |= rowCovers[group] != GroupCover::Clear;
            }
            for (std::size_t group = 0; group != needed.size(); ++group)
            {
                const int firstPoint = static_cast<int>(group) * 8;
                const int from = std::max(0, (firstPoint - 1) * step) / 8;
                const int to = std::min(groupsPerRow - 1, ((firstPoint + 8) * step - 1) / 8);
                needed[group] = from <= to && std::any_of(anyCovered.begin() + from,
                    anyCovered.begin() + to + 1, [](std::uint8_t covered) {
                        return covered != 0;
                    });
            }
        }

        // **A row of land pixels, premultiplied by the coastlines' coverage**, in one pass: each
        // pixel's colour is spread from the four points about it, the stylized bands are cut there,
        // and the air's haze is laid over it. Where the points about 8 pixels lie whole in the same
        // stylized zones, the pixels take the zones' colour from them; elsewhere the zones are cut
        // at each pixel, so their edges do not step from point to point. 8 pixels the coastlines do
        // not reach are written clear. The last group of 8 stops at the row's end.
        void writeLandRow(const LandPicture& picture, const LandPalette& palette,
            const GroundPoints& upper, const GroundPoints& lower, const float downShare,
            const std::span<const float> coverage, Color* pixels, const int width)
        {
            const int step = picture.step;
            const PointSpread spread = pointSpreadAt(step);
            const __m256 down = _mm256_set1_ps(downShare);
            const __m256 zero = _mm256_setzero_ps();
            const __m256 half = _mm256_set1_ps(0.5f);
            const __m256 byteMax = _mm256_set1_ps(255.0f);
            const __m256 perPixel = _mm256_set1_ps(1.0f / static_cast<float>(step));
            const bool realistic = picture.look == SceneLook::Realistic;
            const ColorLanes hazeColor = colorLanesOf(EarthSceneColors::Haze);
            for (int x = 0; x < width; x += 8)
            {
                const __m256 covered = _mm256_loadu_ps(coverage.data() + x);
                if (_mm256_movemask_ps(_mm256_cmp_ps(covered, zero, _CMP_GT_OQ)) == 0)
                {
                    storeEight(pixels, x, width, _mm256_setzero_si256());
                    continue;
                }
                const std::size_t point = static_cast<std::size_t>(x / step);
                const auto spreadOf = [&](const std::vector<float>& upperValues,
                    const std::vector<float>& lowerValues) {
                    return pixelsBetween(_mm256_loadu_ps(upperValues.data() + point),
                        _mm256_loadu_ps(lowerValues.data() + point), down, spread);
                };
                ColorLanes color = {
                    spreadOf(upper.red, lower.red),
                    spreadOf(upper.green, lower.green),
                    spreadOf(upper.blue, lower.blue)
                };
                if (!realistic)
                {
                    const auto around = [&](const std::vector<float>& upperValues,
                        const std::vector<float>& lowerValues) {
                        return cornersAround(upperValues, lowerValues, point, spread);
                    };
                    const auto spreadOver = [&](const std::vector<float>& upperValues,
                        const std::vector<float>& lowerValues) {
                        return spreadWithChange(around(upperValues, lowerValues), down, spread,
                            perPixel);
                    };
                    if (!isInsideOneZone(around(upper.zones, lower.zones)))
                    {
                        color = zoneColor(zoneCutsOf(spreadOver(upper.dryness, lower.dryness),
                            spreadOver(upper.forest, lower.forest),
                            spreadOver(upper.tundra, lower.tundra)), palette);
                    }
                    color = stylizedRelief(color, spreadOver(upper.heights, lower.heights),
                        spreadOf(upper.snowEdges, lower.snowEdges),
                        spreadOver(upper.light, lower.light), palette);
                }
                color = mixed(color, hazeColor, spreadOf(upper.haze, lower.haze));
                // Off the sphere the land is not a number; the maximum takes 0 there.
                const auto byteAt = [&](__m256 channel, int shift) {
                    const __m256 scaled = _mm256_fmadd_ps(_mm256_mul_ps(channel, covered),
                        byteMax, half);
                    return _mm256_slli_epi32(_mm256_cvttps_epi32(_mm256_max_ps(scaled, zero)),
                        shift);
                };
                const __m256i alpha = _mm256_slli_epi32(
                    _mm256_cvttps_epi32(_mm256_fmadd_ps(covered, byteMax, half)), 24);
                storeEight(pixels, x, width, _mm256_or_si256(
                    _mm256_or_si256(byteAt(color.blue, 0), byteAt(color.green, 8)),
                    _mm256_or_si256(byteAt(color.red, 16), alpha)));
            }
        }

        // **The band's bitmap spans the columns the ground covers in its widest row**, inside the
        // view; a band that misses the sphere is left empty. The points stand on the canvas's
        // columns and rows a step apart, so neighbouring bands blend from the same points. `coast`
        // holds the edges of the near side's coastlines that reach into the band. Answers where the
        // bitmap stands, in pixels.
        [[nodiscard]] FloatPoint rasterizeLandBand(const LandPicture& picture,
            const LandPalette& palette, const std::span<const CoastEdge> coast, const int top,
            const int rows, Bitmap& pixels)
        {
            const float nearestRow = std::clamp(picture.sphereCenter.y,
                static_cast<float>(top) + 0.5f, static_cast<float>(top + rows) - 0.5f);
            const float rise = (nearestRow - picture.sphereCenter.y) / picture.sphereRadii.y;
            const float halfWidth = rise * rise < 1.0f
                ? picture.sphereRadii.x * std::sqrt(1.0f - rise * rise)
                : 0.0f;
            const int step = picture.step;
            const int sphereLeft = static_cast<int>(std::floor(picture.sphereCenter.x - halfWidth));
            const int left = std::max(picture.left, sphereLeft) / step * step;
            const int right = std::min(picture.right,
                static_cast<int>(std::ceil(picture.sphereCenter.x + halfWidth)));
            if (left >= right)
            {
                pixels.resize({});
                return {};
            }

            const int width = right - left;
            const float firstPixelX = static_cast<float>(left) + 0.5f;
            pixels.resize({ width, rows });
            const auto pixelYAt = [&](int bandRow) {
                return static_cast<float>(top + bandRow) + 0.5f;
            };
            // The last group of 8 pixels reads 8 points from the one under its first pixel.
            const std::size_t pointLength =
                groupedLength(static_cast<int>(groupedLength(width)) / step + 8);
            const auto pointsOfRow = [&]() {
                GroundPoints points;
                for (std::vector<float>* values : {
                    &points.longitudes, &points.latitudes, &points.longitudeSpans,
                    &points.latitudeSpans, &points.sunlight, &points.sunEast,
                    &points.sunNorth, &points.depths, &points.heights, &points.slopesEast,
                    &points.slopesNorth, &points.variation, &points.dryness, &points.forest,
                    &points.tundra, &points.red, &points.green, &points.blue,
                    &points.snowEdges, &points.zones, &points.light, &points.haze })
                {
                    values->resize(pointLength);
                }
                return points;
            };

            // The band's coverage first, so only the points a covered pixel reads are worked out.
            const int stride = static_cast<int>(groupedLength(width + 2));
            std::vector<float> coverage(static_cast<std::size_t>(stride) * rows);
            std::vector<GroupCover> groupCovers;
            if (picture.coastBlur > 0.0f)
            {
                coverBlurredBand(picture, coast, top, rows, left, width, stride, coverage,
                    groupCovers);
            }
            else
            {
                coverBand(coast, top, rows, left, width, stride, coverage, groupCovers);
            }
            std::vector<std::uint8_t> anyCovered;
            std::vector<bool> needed(pointLength / 8);
            const auto sampleRow = [&](int pointRow, GroundPoints& points) {
                markNeededGroups(step, groupCovers, stride / 8, rows, pointRow, anyCovered,
                    needed);
                sampleGround(picture, palette, pixelYAt(pointRow), firstPixelX, needed, points);
            };

            // At a step of 1 every row is a row of points, and the row below gives the change
            // down each pixel.
            GroundPoints upper = pointsOfRow();
            GroundPoints lower = pointsOfRow();
            sampleRow(0, upper);
            sampleRow(step, lower);
            for (int y = 0; y != rows; ++y)
            {
                const int pastPoints = y % step;
                if (y != 0 && pastPoints == 0)
                {
                    std::swap(upper, lower);
                    sampleRow(y + step, lower);
                }
                const float downShare = static_cast<float>(pastPoints) / static_cast<float>(step);
                writeLandRow(picture, palette, upper, lower, downShare,
                    std::span<const float>{ coverage }.subspan(static_cast<std::size_t>(y) * stride,
                        static_cast<std::size_t>(stride)), pixels.pixel(0, y), width);
            }
            return { static_cast<float>(left), static_cast<float>(top) };
        }

        // What every band of the sea is worked out from, this frame.
        struct SeaPicture
        {
            const EarthMap* map;
            SphereView ground;
            SceneLook look;
            std::span<const Color> shades; // the realistic look's, from the coast to the trenches
            int step; // the pixels between the points the depths are read at
            FloatPoint sphereCenter; // in pixels
            FloatPoint sphereRadii; // across and up, in pixels
            int left; // the first column in view
            int right; // the column past the last in view
        };

        // The realistic sea's colour by its depth from 0 to 1, shading evenly between the levels'.
        [[nodiscard]] std::vector<Color> seaShades()
        {
            struct Stop
            {
                float depth;
                Color color;
            };
            const std::array<Stop, 6> stops = { {
                { 0.0f, EarthSceneColors::SeaShelf },
                { 0.17f, EarthSceneColors::SeaSlope },
                { 0.33f, EarthSceneColors::SeaRise },
                { 0.5f, EarthSceneColors::SeaRidge },
                { 0.67f, EarthSceneColors::OceanDeep },
                { 1.0f, EarthSceneColors::SeaTrench }
            } };
            std::vector<Color> shades;
            shades.reserve(k_seaShadeCount);
            for (int i = 0; i != k_seaShadeCount; ++i)
            {
                const float depth = static_cast<float>(i) / (k_seaShadeCount - 1);
                std::size_t next = 1;
                while (next + 1 < stops.size() && stops[next].depth < depth)
                    ++next;
                const Stop& from = stops[next - 1];
                const Stop& to = stops[next];
                shades.push_back(colorBetween(from.color, to.color, std::clamp(
                    (depth - from.depth) / (to.depth - from.depth), 0.0f, 1.0f)));
            }
            return shades;
        }

        // **The stylized sea in five flat tones by its depth**: an edge is softened by
        // k_edgeSoftening of what the depth changes over the pixel, and no less than half of
        // k_seaToneFade.
        [[nodiscard]] ColorLanes flatSeaColor(const SpreadLanes& depths)
        {
            using namespace EarthSceneColors;
            const SoftEdge edge = softEdgeOf(_mm256_max_ps(_mm256_mul_ps(depths.change,
                _mm256_set1_ps(k_edgeSoftening)), _mm256_set1_ps(0.5f * k_seaToneFade)));
            const auto past = [&](float depth) {
                return smoothStepAround(_mm256_set1_ps(depth), edge, depths.values);
            };
            ColorLanes color = mixed(colorLanesOf(FlatSeaShelf), colorLanesOf(FlatSeaSlope),
                past(0.08f));
            color = mixed(color, colorLanesOf(FlatSeaRidge), past(0.42f));
            color = mixed(color, colorLanesOf(OceanDeep), past(0.6f));
            return mixed(color, colorLanesOf(FlatSeaTrench), past(0.9f));
        }

        // **A band of the sea, premultiplied and opaque on the sphere**: the depths are read at
        // points `step` apart and spread over the pixels between, each pixel takes its colour by
        // the look, and the air's haze veils it as it veils the land; the pixels on the limb fade
        // out across it. The bitmap spans the sphere's widest row in the band; answers where it
        // stands.
        [[nodiscard]] FloatPoint rasterizeSeaBand(const SeaPicture& picture, const int top,
            const int rows, Bitmap& pixels)
        {
            const FloatPoint radii = picture.sphereRadii;
            const float nearestRow = std::clamp(picture.sphereCenter.y,
                static_cast<float>(top) + 0.5f, static_cast<float>(top + rows) - 0.5f);
            const float rise = (nearestRow - picture.sphereCenter.y) / radii.y;
            const float halfWidth = rise * rise < 1.0f ? radii.x * std::sqrt(1.0f - rise * rise)
                : 0.0f;
            const int step = picture.step;
            const int left = std::max(picture.left,
                static_cast<int>(std::floor(picture.sphereCenter.x - halfWidth))) / step * step;
            const int right = std::min(picture.right,
                static_cast<int>(std::ceil(picture.sphereCenter.x + halfWidth)));
            if (left >= right)
            {
                pixels.resize({});
                return {};
            }
            const int width = right - left;
            const float firstPixelX = static_cast<float>(left) + 0.5f;
            pixels.resize({ width, rows });
            // The last group of 8 pixels reads 8 points from the one under its first pixel.
            const std::size_t pointLength =
                groupedLength(static_cast<int>(groupedLength(width)) / step + 8);
            MapPoints onMap = {
                .longitudes = std::vector<float>(pointLength),
                .latitudes = std::vector<float>(pointLength),
                .longitudeSpans = std::vector<float>(pointLength),
                .latitudeSpans = std::vector<float>(pointLength)
            };
            std::vector<float> clearness(pointLength);
            std::vector<float> upper(pointLength);
            std::vector<float> lower(pointLength);
            const auto depthsAt = [&](int bandRow, std::vector<float>& depths) {
                projectRow(picture.ground, static_cast<float>(top + bandRow) + 0.5f, firstPixelX,
                    step, onMap, clearness);
                picture.map->readDepths({
                    .longitudes = onMap.longitudes,
                    .latitudes = onMap.latitudes,
                    .longitudeSpans = onMap.longitudeSpans,
                    .latitudeSpans = onMap.latitudeSpans
                }, depths);
            };

            const PointSpread spread = pointSpreadAt(step);
            const bool realistic = picture.look == SceneLook::Realistic;
            const int* shades = reinterpret_cast<const int*>(picture.shades.data());
            const __m256 lastShade = _mm256_set1_ps(static_cast<float>(picture.shades.size() - 1));
            const __m256 perPixel = _mm256_set1_ps(1.0f / static_cast<float>(step));
            const __m256 zero = _mm256_setzero_ps();
            const __m256 half = _mm256_set1_ps(0.5f);
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 byteMax = _mm256_set1_ps(255.0f);
            const __m256 perByte = _mm256_set1_ps(1.0f / 255.0f);
            const __m256i byteMask = _mm256_set1_epi32(0xFF);
            const __m256 rowStart = _mm256_add_ps(_mm256_set1_ps(firstPixelX), laneOffsets());
            const __m256 edgeScale = _mm256_set1_ps(std::min(radii.x, radii.y));
            const ColorLanes hazeColor = colorLanesOf(EarthSceneColors::Haze);
            const auto shadeAt = [&](__m256 depth) {
                const __m256i shade = _mm256_i32gather_epi32(shades, _mm256_cvttps_epi32(
                    _mm256_min_ps(_mm256_max_ps(_mm256_fmadd_ps(depth, lastShade, half), zero),
                        lastShade)), 4);
                const auto channelOf = [&](int shift) {
                    return _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_and_si256(
                        _mm256_srli_epi32(shade, shift), byteMask)), perByte);
                };
                return ColorLanes{ channelOf(16), channelOf(8), channelOf(0) };
            };
            depthsAt(0, upper);
            depthsAt(step, lower);
            for (int y = 0; y != rows; ++y)
            {
                const int pastPoints = y % step;
                if (y != 0 && pastPoints == 0)
                {
                    std::swap(upper, lower);
                    depthsAt(y + step, lower);
                }
                const __m256 down = _mm256_set1_ps(
                    static_cast<float>(pastPoints) / static_cast<float>(step));
                const float pixelY = static_cast<float>(top + y) + 0.5f;
                const float up = (picture.sphereCenter.y - pixelY) / radii.y;
                Color* row = pixels.pixel(0, y);
                for (int x = 0; x < width; x += 8)
                {
                    const std::size_t point = static_cast<std::size_t>(x / step);
                    const ColorLanes color = realistic
                        ? shadeAt(pixelsBetween(_mm256_loadu_ps(upper.data() + point),
                            _mm256_loadu_ps(lower.data() + point), down, spread))
                        : flatSeaColor(spreadWithChange(cornersAround(upper, lower, point, spread),
                            down, spread, perPixel));
                    const __m256 across = _mm256_mul_ps(_mm256_sub_ps(_mm256_add_ps(rowStart,
                        _mm256_set1_ps(static_cast<float>(x))),
                        _mm256_set1_ps(picture.sphereCenter.x)), _mm256_set1_ps(1.0f / radii.x));
                    const __m256 reach = _mm256_sqrt_ps(_mm256_fmadd_ps(across, across,
                        _mm256_set1_ps(up * up)));
                    const __m256 facing = _mm256_sqrt_ps(_mm256_max_ps(
                        _mm256_fnmadd_ps(reach, reach, one), zero));
                    const __m256 haze = airHazeAt(facing, zero); // the sea's surface at height 0
                    const __m256 inside = _mm256_min_ps(_mm256_max_ps(_mm256_fmadd_ps(
                        _mm256_sub_ps(one, reach), edgeScale, half), zero), one);
                    const ColorLanes hazed = mixed(color, hazeColor, haze);
                    const __m256 opacity = _mm256_mul_ps(inside, byteMax);
                    const auto byteAt = [&](__m256 channel, int shift) {
                        return _mm256_slli_epi32(_mm256_cvttps_epi32(
                            _mm256_fmadd_ps(channel, opacity, half)), shift);
                    };
                    storeEight(row, x, width, _mm256_or_si256(
                        _mm256_or_si256(byteAt(hazed.blue, 0), byteAt(hazed.green, 8)),
                        _mm256_or_si256(byteAt(hazed.red, 16), byteAt(one, 24))));
                }
            }
            return { static_cast<float>(left), static_cast<float>(top) };
        }
    }

    // The coasts take some milliseconds to build, so they are built off the paint from the start,
    // and the first frame takes them.
    EarthScene::EarthScene()
        :
        m_coastsBuilt{ std::async(std::launch::async, buildCoasts) }
    {
    }

    void EarthScene::prepare(const SceneParams& params)
    {
        m_space.prepare(params);
        m_ocean.prepare(params);
    }

    void EarthScene::draw(Canvas& canvas, const FloatRect& bounds, const SceneTime time)
    {
        if (m_coastsBuilt.valid())
        {
            BuiltCoasts built = m_coastsBuilt.get();
            m_coastlines = std::move(built.coastlines);
            m_cities = std::move(built.towns);
        }
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const float altitude = m_view.altitude;
        const GlobeDisc globeDisc = globeDiscAt(width, height, altitude);
        const float radius = globeDisc.radius;
        // The orbit runs on through the cycles, so it never comes round, and the globe's motion
        // runs behind the scene's time by as long as it has stood still.
        const double cycles = static_cast<double>(time.cyclesPassed) + time.cycleTime;
        const double cycleSeconds = std::chrono::duration<double>{ k_cycle }.count();
        const double sceneSeconds = cycles * cycleSeconds;
        const double frameSeconds = secondsSinceLastFrame(sceneSeconds);
        const double globeCycles = cycles - stillSecondsAfter(frameSeconds) / cycleSeconds;
        moveShownSun(frameSeconds);
        // **A hanging satellite flies no orbit**: its orbits stand still while the Earth turns on.
        const double timeInOrbits = globeCycles * std::chrono::duration<double>{ k_cycle }
            / k_orbit;
        const double orbits = m_view.hanging
            ? m_view.orbitsAhead
            : m_view.orbitsAhead + timeInOrbits;
        m_shownOrbits = orbits;
        m_shownTimeInOrbits = timeInOrbits;
        const double earthTurns = earthTurnsAt(timeInOrbits);
        const Globe globe = Globe::FromCamera(globeDisc.center, radius,
            cameraAt(orbits, earthTurns), earthTurns, m_shownSunTurns);

        // As the view climbs the satellite draws smaller and moves to the top right, out of the
        // way of the whole globe.
        const float satelliteShare = std::lerp(1.0f, 0.45f, altitude);
        const float hover = 5.0f * unit * satelliteShare
            * std::sin(k_2Pi * 2.0f * k_lively * time.cycleTime);
        const FloatPoint satellite = {
            width * std::lerp(0.66f, 0.85f, altitude),
            height * std::lerp(0.27f, 0.16f, altitude) + hover
        };
        // **The satellite is in the Earth's shadow when the ground under it is past the
        // terminator**: low down, the ground the satellite is drawn over; with the whole globe in
        // the frame, the ground straight below it.
        const FloatPoint offCenter = {
            (satellite.x - globe.center.x) / radius,
            (globe.center.y - satellite.y) / radius
        };
        const FloatPoint drawnOver = rotated(offCenter, -globe.tilt);
        const float behind = viewBehindAt(altitude);
        const FloatPoint below = rotated({ 0.0f, std::sin(behind) }, -globe.tilt);
        const Direction under = {
            std::lerp(drawnOver.x, below.x, altitude),
            std::lerp(drawnOver.y, below.y, altitude),
            std::lerp(0.35f, std::cos(behind), altitude)
        };
        const float underLength = std::hypot(under.x, under.y, under.z);
        const float sunlit = smoothStep(-0.14f, 0.06f, globe.sunlightAt({
            under.x / underLength,
            under.y / underLength,
            under.z / underLength
        }));
        // What is drawn at a fixed size on the globe shrinks with it.
        const float globeShare = radius / globeDiscAt(width, height, 0.0f).radius;

        if (altitude == 0.0f)
            m_space.draw(frame);
        else
            fillSpace(frame, globeDisc);
        // The stars dim a little by day and shine fully in the Earth's shadow.
        drawStars(frame, time.cycleTime, StarField{
            .count = 200,
            .top = 0.0f,
            .bottom = height,
            .seed = 90.0f,
            .color = EarthSceneColors::Star,
            .opacity = 0.65f + 0.35f * (1.0f - sunlit),
            .twinklesPerCycle = k_lively,
            .drift = { fractionOf(orbits * k_skyWidthsPerOrbit), 0.0f }
        });
        drawGlow(frame, { width, 0.0f }, 340.0f * unit,
            EarthSceneColors::SunGlare.withOpacity(0.16f * sunlit));
        if (m_earthMap)
            drawSea(frame, globe);
        else if (altitude == 0.0f)
            m_ocean.draw(frame);
        else
            fillOcean(frame, globeDisc);

        PixelPath disc;
        disc.drawCircle(globe.center, radius);
        frame.canvas.pushClip(disc, &frame.toCanvas);
        drawLand(frame, globe);
        drawNight(frame, globe);
        if (globe.sun.z > 0.0f)
        {
            drawGlow(frame, globe.onScreen(globe.sun), 150.0f * unit * globeShare,
                EarthSceneColors::SeaGlint.withOpacity(0.3f * globe.sun.z), 0.8f);
        }
        drawCityLights(frame, globe, time.cycleTime);
        frame.canvas.popClip();
        drawClouds(frame, globe, sceneSeconds);
        drawAtmosphere(frame, globe);

        const float size = unit * std::min(1.0f, (width / unit) / 900.0f) * satelliteShare;
        drawSatellite(frame, SatellitePose{
            .center = satellite,
            .size = size,
            .sunlit = sunlit,
            .orbitTime = fractionOf(orbits),
            .blinkTime = time.cycleTime * k_lively
        });
    }

    void EarthScene::drawIcon(Canvas& canvas, const FloatRect& bounds) const
    {
        constexpr float k_figureSpan{ 574.0f };
        const SceneFrame frame = SceneFrame::Spanning(canvas, bounds, k_figureSpan);
        const float unit = frame.unit;
        const FloatPoint center = { frame.width * 0.5f, frame.height * 0.5f };
        // The planet's glow is cut at the icon's edges.
        canvas.pushClip(bounds);
        drawPlanet(frame, PlanetPose{
            .planet = Planet::Earth,
            .center = center + FloatPoint{ -27.0f, 27.0f } * unit,
            .radius = 200.0f * unit,
            .color = EarthSceneColors::IconOcean,
            .light = { -0.6f, -0.5f, 0.6f },
            .orbitTilt = 0.25f,
            .cycleTime = 0.3f
        });
        drawSatellite(frame, SatellitePose{
            .center = center + FloatPoint{ 183.0f, -203.0f } * unit,
            .size = 0.42f * unit,
            .sunlit = 1.0f,
            .orbitTime = 0.1f,
            .blinkTime = 0.0f
        });
        canvas.popClip();
    }

    SceneReadoutColors EarthScene::readoutColors() const
    {
        return {
            .field = EarthSceneColors::SpaceTop,
            .series = EarthSceneColors::Day
        };
    }

    bool EarthScene::dragMovesViewFrom(const FloatPoint start, const FloatRect& bounds) const
    {
        const GlobeDisc disc = globeDiscAt(bounds.width(), bounds.height(), m_view.altitude);
        const FloatPoint fromCenter = {
            start.x - bounds.left - disc.center.x,
            start.y - bounds.top - disc.center.y
        };
        return std::hypot(fromCenter.x, fromCenter.y) <= disc.radius;
    }

    // **A drag turns the globe the way the pointer goes, as a hand turns a ball**: the ground
    // under the middle of the view moves to where the drag takes it, about the axis square to
    // both, wherever the poles stand. On the orbit the turn is the orbit's among the stars, so
    // the satellite flies on along the turned orbit; hanging, it is the view's over the Earth.
    // See PathArt#earthscene
    void EarthScene::dragView(const FloatPoint by, const FloatRect& bounds)
    {
        const Direction from = heldPointAt({ 0.0f, 0.0f }, bounds);
        const Direction to = heldPointAt(by, bounds);
        const Direction square = {
            from.y * to.z - from.z * to.y,
            from.z * to.x - from.x * to.z,
            from.x * to.y - from.y * to.x
        };
        const float squareLength = std::hypot(square.x, square.y, square.z);
        if (squareLength < 1.0e-6f)
            return;
        const float angle = std::atan2(squareLength, from.x * to.x + from.y * to.y + from.z * to.z);
        const Direction axisInView = {
            square.x / squareLength,
            square.y / squareLength,
            square.z / squareLength
        };
        if (m_view.hanging)
        {
            const Rotation lean = Rotation::About({ 1.0f, 0.0f, 0.0f },
                viewBehindAt(m_view.altitude));
            m_view.hangingCamera = m_view.hangingCamera
                .after(Rotation::About(lean.applied(axisInView), -angle)).orthonormal();
            return;
        }
        const Direction axis = Globe::orbitAxesAt(m_shownOrbits, m_view.altitude,
            m_view.trackAngle).applied(axisInView);
        m_view.orbitTurn = m_view.orbitTurn.after(Rotation::About(axis, -angle)).orthonormal();
    }

    // A notch away from the screen comes down, as the wheel zooms in.
    void EarthScene::scrollView(const float notches)
    {
        m_view.altitude = std::clamp(m_view.altitude - notches * k_altitudePerNotch, 0.0f, 1.0f);
    }

    // A press holds the globe still until it is let go.
    void EarthScene::holdView(const bool held)
    {
        m_viewHeld = held;
    }

    // **The let-go decides how the satellite flies on**, and the globe moves on at once. Let go
    // still, it hangs over the ground in the view, and only the day and the night move on. Let go
    // moving, it takes the orbit that carries the ground on the way the pointer went, from where
    // the view stands, at the orbit's own pace. See PathArt#earthscene
    void EarthScene::letGoView(const FloatPoint speed, const FloatRect& bounds)
    {
        m_viewHeld = false;
        if (speed.x == 0.0f && speed.y == 0.0f)
        {
            hangSatellite();
            return;
        }
        flySatelliteOn(trackAngleFor(speed, bounds));
    }

    // **A click stops the satellite or sets it flying again**: a flying one hangs as a still let-go
    // hangs it, and a hanging one flies on along the track it flew before it hung.
    void EarthScene::clickView(const FloatRect&)
    {
        m_viewHeld = false;
        if (!m_view.hanging)
        {
            hangSatellite();
            return;
        }
        flySatelliteOn(m_view.trackAngle);
    }

    // **A double click moves the day on by a quarter**: the Sun goes back round the Earth's axis as
    // far as a quarter turn of the Earth carries the ground on, so the light moves over the ground
    // as the hours move it. See PathArt#earthscene
    void EarthScene::doubleClickView(const FloatRect&)
    {
        m_sunTurns -= k_dayPerDoubleClick;
    }

    std::span<const SceneGesture> EarthScene::gestures() const
    {
        return k_gestures;
    }

    // The orbits and the Earth's turns are those of the last frame, which the viewer last saw.
    void EarthScene::storeView(Dom::Section& view) const
    {
        (view / ViewNodes::Altitude).set(m_view.altitude);
        (view / ViewNodes::Orbits).set(static_cast<float>(m_shownOrbits));
        (view / ViewNodes::EarthTurns).set(static_cast<float>(earthTurnsAt(m_shownTimeInOrbits)));
        (view / ViewNodes::SunTurns).set(fractionOf(m_sunTurns));
        storeRotation(view.childSection(ViewNodes::OrbitTurn), m_view.orbitTurn);
        (view / ViewNodes::TrackAngle).set(m_view.trackAngle / k_degree);
        (view / ViewNodes::Hanging).set(m_view.hanging);
        storeRotation(view.childSection(ViewNodes::HangingCamera), m_view.hangingCamera);
    }

    // **The stored orbits and turns take the place of the last frame's**, and the globe flies on
    // from them as the scene's time goes on.
    void EarthScene::restoreView(const Dom::Section& view)
    {
        m_view.altitude = std::clamp((view / ViewNodes::Altitude).get<float>(), 0.0f, 1.0f);
        m_view.orbitTurn = restoredRotation(view.childSection(ViewNodes::OrbitTurn),
            m_view.orbitTurn);
        m_view.trackAngle = (view / ViewNodes::TrackAngle).get<float>() * k_degree;
        m_view.hanging = (view / ViewNodes::Hanging).get<bool>();
        m_view.hangingCamera = restoredRotation(view.childSection(ViewNodes::HangingCamera),
            m_view.hangingCamera);
        const double storedOrbits = (view / ViewNodes::Orbits).get<float>();
        m_view.orbitsAhead = m_view.hanging
            ? storedOrbits
            : storedOrbits - m_shownTimeInOrbits;
        m_shownOrbits = storedOrbits;
        const double storedEarthTurns = (view / ViewNodes::EarthTurns).get<float>();
        m_earthTurnsAhead += storedEarthTurns - earthTurnsAt(m_shownTimeInOrbits);
        m_sunTurns = (view / ViewNodes::SunTurns).get<float>();
        m_shownSunTurns = m_sunTurns;
    }

    // A view as the scene starts is what a config with none restores.
    Dom::Dt::Section EarthScene::viewSchema()
    {
        const View startingView{};
        return Dom::Dt::Section{
            k_name,
            Dom::Dt::Value{ ViewNodes::Altitude, startingView.altitude },
            Dom::Dt::Value{ ViewNodes::Orbits, 0.0f },
            Dom::Dt::Value{ ViewNodes::EarthTurns, 0.0f },
            Dom::Dt::Value{ ViewNodes::SunTurns, 0.0f },
            rotationSchema(ViewNodes::OrbitTurn, startingView.orbitTurn),
            Dom::Dt::Value{ ViewNodes::TrackAngle, startingView.trackAngle },
            Dom::Dt::Value{ ViewNodes::Hanging, startingView.hanging },
            rotationSchema(ViewNodes::HangingCamera, startingView.hangingCamera)
        };
    }

    // The Earth spins about its axis, and then tips its north pole toward the viewer.
    EarthScene::Direction EarthScene::EarthTurn::applied(const Direction onEarth) const
    {
        const float x = onEarth.x * spinCosine + onEarth.z * spinSine;
        const float z = onEarth.z * spinCosine - onEarth.x * spinSine;
        return { x, onEarth.y * tipCosine + z * tipSine, z * tipCosine - onEarth.y * tipSine };
    }

    EarthScene::Rotation EarthScene::Rotation::About(const Direction axis,
        const float angle)
    {
        const float cosine = std::cos(angle);
        const float sine = std::sin(angle);
        const float rest = 1.0f - cosine;
        return {
            .x = {
                rest * axis.x * axis.x + cosine,
                rest * axis.x * axis.y + sine * axis.z,
                rest * axis.x * axis.z - sine * axis.y
            },
            .y = {
                rest * axis.x * axis.y - sine * axis.z,
                rest * axis.y * axis.y + cosine,
                rest * axis.y * axis.z + sine * axis.x
            },
            .z = {
                rest * axis.x * axis.z + sine * axis.y,
                rest * axis.y * axis.z - sine * axis.x,
                rest * axis.z * axis.z + cosine
            }
        };
    }

    EarthScene::Direction EarthScene::Rotation::applied(const Direction direction) const
    {
        return {
            x.x * direction.x + y.x * direction.y + z.x * direction.z,
            x.y * direction.x + y.y * direction.y + z.y * direction.z,
            x.z * direction.x + y.z * direction.y + z.z * direction.z
        };
    }

    EarthScene::Rotation EarthScene::Rotation::after(const Rotation& first) const
    {
        return { applied(first.x), applied(first.y), applied(first.z) };
    }

    // x keeps its way, y is set square to it, and z is square to both.
    EarthScene::Rotation EarthScene::Rotation::orthonormal() const
    {
        const auto unit = [](const Direction direction) {
            const float length = std::hypot(direction.x, direction.y, direction.z);
            return Direction{ direction.x / length, direction.y / length, direction.z / length };
        };
        const Direction first = unit(x);
        const float along = first.x * y.x + first.y * y.y + first.z * y.z;
        const Direction second = unit({
            y.x - along * first.x,
            y.y - along * first.y,
            y.z - along * first.z
        });
        return {
            .x = first,
            .y = second,
            .z = {
                first.y * second.z - first.z * second.y,
                first.z * second.x - first.x * second.z,
                first.x * second.y - first.y * second.x
            }
        };
    }

    // **Read through the rotation's quaternion**, worked out from its largest part so that no
    // division comes near 0. A rotation by nothing answers the y axis and no angle.
    EarthScene::AxisTurn EarthScene::Rotation::axisTurn() const
    {
        struct Quaternion
        {
            float w;
            float x;
            float y;
            float z;
        };
        const float trace = x.x + y.y + z.z;
        Quaternion quaternion = { 1.0f, 0.0f, 0.0f, 0.0f };
        if (trace > 0.0f)
        {
            const float scale = 2.0f * std::sqrt(1.0f + trace);
            quaternion = {
                0.25f * scale,
                (y.z - z.y) / scale,
                (z.x - x.z) / scale,
                (x.y - y.x) / scale
            };
        }
        else if (x.x >= y.y && x.x >= z.z)
        {
            const float scale = 2.0f * std::sqrt(1.0f + x.x - y.y - z.z);
            quaternion = {
                (y.z - z.y) / scale,
                0.25f * scale,
                (y.x + x.y) / scale,
                (z.x + x.z) / scale
            };
        }
        else if (y.y >= z.z)
        {
            const float scale = 2.0f * std::sqrt(1.0f + y.y - x.x - z.z);
            quaternion = {
                (z.x - x.z) / scale,
                (y.x + x.y) / scale,
                0.25f * scale,
                (z.y + y.z) / scale
            };
        }
        else
        {
            const float scale = 2.0f * std::sqrt(1.0f + z.z - x.x - y.y);
            quaternion = {
                (x.y - y.x) / scale,
                (z.x + x.z) / scale,
                (z.y + y.z) / scale,
                0.25f * scale
            };
        }
        const float halfSine = std::hypot(quaternion.x, quaternion.y, quaternion.z);
        if (halfSine < 1.0e-7f)
            return { .axis = { 0.0f, 1.0f, 0.0f }, .angle = 0.0f };
        return {
            .axis = { quaternion.x / halfSine, quaternion.y / halfSine, quaternion.z / halfSine },
            .angle = 2.0f * std::atan2(halfSine, quaternion.w)
        };
    }

    // A rotation is undone by its transpose.
    EarthScene::Rotation EarthScene::Rotation::inverse() const
    {
        return {
            .x = { x.x, y.x, z.x },
            .y = { x.y, y.y, z.y },
            .z = { x.z, y.z, z.z }
        };
    }

    // **Where the north pole stands in the view gives the tilt and the tip**; where longitude 0 on
    // the equator stands, turned back by them, gives the spin. Looking straight down the Earth's
    // axis any tilt would do, and the spin makes up for the one taken.
    EarthScene::ViewAngles EarthScene::ViewAngles::Of(const Rotation& fromEarth)
    {
        const Direction north = fromEarth.y;
        const Direction meridian = fromEarth.z;
        const float tip = -std::asin(std::clamp(north.z, -1.0f, 1.0f));
        const float tilt = std::atan2(-north.x, north.y);
        const FloatPoint untilted = rotated({ meridian.x, meridian.y }, -tilt);
        return {
            .tilt = tilt,
            .tip = tip,
            .spin = std::atan2(untilted.x,
                untilted.y * std::sin(tip) + meridian.z * std::cos(tip))
        };
    }

    // **The Earth turns among the stars about its own axis, and the view's axes see it**; the
    // angles the pixels are worked out by are read from where the Earth's axes then stand in the
    // view. The Sun turns about the same axis. See PathArt#earthscene
    EarthScene::Globe EarthScene::Globe::FromCamera(const FloatPoint center, const float radius,
        const Rotation& camera, const double earthTurns, const double sunTurns)
    {
        const auto inView = [&](const Direction& direction) {
            const auto along = [&](const Direction& axis) {
                return axis.x * direction.x + axis.y * direction.y + axis.z * direction.z;
            };
            return Direction{ along(camera.x), along(camera.y), along(camera.z) };
        };
        const Rotation earthAxes = earthAxesAt(earthTurns);
        const Rotation fromEarth = {
            .x = inView(earthAxes.x),
            .y = inView(earthAxes.y),
            .z = inView(earthAxes.z)
        };
        const ViewAngles angles = ViewAngles::Of(fromEarth);
        const float sunAround = k_sunAround + k_2Pi * fractionOf(sunTurns);
        const Direction sun = inView({
            std::cos(k_sunNorth) * std::sin(sunAround),
            std::sin(k_sunNorth),
            std::cos(k_sunNorth) * std::cos(sunAround)
        });
        // The Sun across and up the screen, turned back by the tilt as a direction is held.
        const FloatPoint sunOnScreen = rotated({ sun.x, sun.y }, -angles.tilt);
        return {
            .center = center,
            .radius = radius,
            .tilt = angles.tilt,
            .tip = angles.tip,
            .spin = angles.spin,
            .sun = { sunOnScreen.x, sunOnScreen.y, sun.z },
            .fromEarth = fromEarth
        };
    }

    // **The orbit runs in the plane of y and z among the stars, about x**, and the view looks back
    // from the satellite away from the horizon, less the higher it stands. At a track angle of 0
    // the track runs across the view; the angle turns the view about the vertical under it, and
    // the horizon stays at the top.
    EarthScene::Rotation EarthScene::Globe::orbitAxesAt(const double orbits,
        const float altitude, const float trackAngle)
    {
        const float around = k_2Pi * fractionOf(orbits) + k_orbitStart;
        const float aroundCosine = std::cos(around);
        const float aroundSine = std::sin(around);
        const float behind = viewBehindAt(altitude);
        const float behindCosine = std::cos(behind);
        const float behindSine = std::sin(behind);
        const Rotation acrossTrack = {
            .x = { 0.0f, -aroundCosine, aroundSine },
            .y = { behindCosine, behindSine * aroundSine, behindSine * aroundCosine },
            .z = { -behindSine, behindCosine * aroundSine, behindCosine * aroundCosine }
        };
        // The vertical as the leaning view stands.
        const Direction vertical = { 0.0f, behindSine, behindCosine };
        return acrossTrack.after(Rotation::About(vertical, trackAngle));
    }

    EarthScene::Rotation EarthScene::Globe::earthAxesAt(const double earthTurns)
    {
        const float earthTurn = k_2Pi * fractionOf(earthTurns) + k_earthTurnAtStart;
        const float turnCosine = std::cos(earthTurn);
        const float turnSine = std::sin(earthTurn);
        return {
            .x = { turnCosine, 0.0f, -turnSine },
            .y = { 0.0f, 1.0f, 0.0f },
            .z = { turnSine, 0.0f, turnCosine }
        };
    }

    // Unturned, longitude 0 faces the viewer and the north pole points up.
    EarthScene::Direction EarthScene::Globe::onEarthAt(const FloatPoint lonLat)
    {
        const float latitude = lonLat.y * k_degree;
        const float longitude = lonLat.x * k_degree;
        return {
            std::cos(latitude) * std::sin(longitude),
            std::sin(latitude),
            std::cos(latitude) * std::cos(longitude)
        };
    }

    EarthScene::EarthTurn EarthScene::Globe::turn() const
    {
        return {
            .spinCosine = std::cos(spin),
            .spinSine = std::sin(spin),
            .tipCosine = std::cos(tip),
            .tipSine = std::sin(tip)
        };
    }

    FloatPoint EarthScene::Globe::onScreen(const Direction direction) const
    {
        return toScreen().transform({ direction.x, direction.y });
    }

    // Up the view is up the screen, turned by the tilt, and the screen's y runs down.
    Matrix3x2 EarthScene::Globe::toScreen() const
    {
        const float tiltCosine = std::cos(tilt);
        const float tiltSine = std::sin(tilt);
        return {
            radius * tiltCosine,
            -radius * tiltSine,
            -radius * tiltSine,
            -radius * tiltCosine,
            center.x,
            center.y
        };
    }

    // The outline is cut where it goes behind the Earth into stretches on the near side. Each
    // stretch is joined along the rim to the next one that comes round, the way the outline turns,
    // so the shape keeps its inside on the same hand along the rim as along its own edge.
    void EarthScene::Globe::nearSideOf(const std::span<const Direction> outline,
        const bool counterClockwise, const float rimRadius, std::vector<FloatPoint>& points,
        std::vector<std::size_t>& loopEnds) const
    {
        points.clear();
        loopEnds.clear();
        const std::size_t count = outline.size();
        const auto isNear = [](const Direction& direction) {
            return direction.z > 0.0f;
        };
        const auto rimAt = [rimRadius](float angle) {
            return FloatPoint{ rimRadius * std::cos(angle), rimRadius * std::sin(angle) };
        };
        // The point after point `i` round the closed outline.
        const auto nextOf = [count](std::size_t i) {
            return i + 1 == count ? 0 : i + 1;
        };
        // The outline is followed from a point behind the Earth whose next point is in view.
        std::size_t start = count;
        for (std::size_t i = 0; i != count && start == count; ++i)
        {
            if (!isNear(outline[i]) && isNear(outline[nextOf(i)]))
                start = i;
        }
        if (start == count)
        {
            // All of the outline is in view, or none of it.
            if (count == 0 || !isNear(outline.front()))
                return;
            for (const Direction& direction : outline)
                points.push_back({ direction.x, direction.y });
            loopEnds.push_back(points.size());
            return;
        }

        // A stretch of the outline in view, from where it comes round to where it goes behind.
        struct Stretch
        {
            std::size_t first;
            std::size_t end;
            float enterAngle;
            float leaveAngle;
        };
        // The angle round the rim at which the edge from one point to the next crosses it.
        const auto crossingAngle = [](const Direction& from, const Direction& to) {
            const float share = from.z / (from.z - to.z);
            return std::atan2(std::lerp(from.y, to.y, share), std::lerp(from.x, to.x, share));
        };
        std::vector<FloatPoint> walked;
        std::vector<Stretch> stretches;
        std::size_t at = start;
        for (std::size_t edge = 0; edge != count; ++edge)
        {
            const Direction& from = outline[at];
            at = nextOf(at);
            const Direction& to = outline[at];
            const bool fromNear = isNear(from);
            const bool toNear = isNear(to);
            if (!fromNear && toNear)
            {
                const float angle = crossingAngle(from, to);
                stretches.push_back({
                    .first = walked.size(),
                    .end = walked.size(),
                    .enterAngle = angle,
                    .leaveAngle = angle
                });
                walked.push_back(rimAt(angle));
            }
            if (toNear)
                walked.push_back({ to.x, to.y });
            if (fromNear && !toNear)
            {
                const float angle = crossingAngle(from, to);
                walked.push_back(rimAt(angle));
                stretches.back().end = walked.size();
                stretches.back().leaveAngle = angle;
            }
        }

        // How far round the rim, the way the outline turns, from one angle to another.
        const auto rimTravel = [&](float from, float to) {
            const float turn = fractionalPart((counterClockwise ? to - from : from - to) / k_2Pi);
            return k_2Pi * turn;
        };
        std::vector<bool> joined(stretches.size(), false);
        for (std::size_t first = 0; first != stretches.size(); ++first)
        {
            if (joined[first])
                continue;
            std::size_t current = first;
            while (!joined[current])
            {
                joined[current] = true;
                const Stretch& stretch = stretches[current];
                for (std::size_t i = stretch.first; i != stretch.end; ++i)
                    points.push_back(walked[i]);
                std::size_t next = current;
                float nearest = std::numeric_limits<float>::max();
                for (std::size_t j = 0; j != stretches.size(); ++j)
                {
                    const float travel = rimTravel(stretch.leaveAngle, stretches[j].enterAngle);
                    if (travel < nearest)
                    {
                        nearest = travel;
                        next = j;
                    }
                }
                const float sweep = counterClockwise ? nearest : -nearest;
                const int steps = static_cast<int>(std::ceil(nearest / k_rimStep));
                for (int step = 1; step < steps; ++step)
                {
                    const float share = static_cast<float>(step) / static_cast<float>(steps);
                    points.push_back(rimAt(stretch.leaveAngle + sweep * share));
                }
                current = next;
            }
            loopEnds.push_back(points.size());
        }
    }

    bool EarthScene::Globe::addNearSide(const std::span<const Direction> outline,
        const bool counterClockwise, const FloatRect& view, PixelPath& path) const
    {
        std::vector<FloatPoint> points;
        std::vector<std::size_t> loopEnds;
        nearSideOf(outline, counterClockwise, 1.0f, points, loopEnds);
        if (points.empty())
            return false;
        const Matrix3x2 screen = toScreen();
        constexpr float k_far{ std::numeric_limits<float>::max() };
        FloatRect bounds = { k_far, k_far, -k_far, -k_far };
        for (FloatPoint& point : points)
        {
            point = screen.transform(point);
            bounds = {
                std::min(bounds.left, point.x),
                std::min(bounds.top, point.y),
                std::max(bounds.right, point.x),
                std::max(bounds.bottom, point.y)
            };
        }
        const bool outside = bounds.right < view.left || bounds.left > view.right
            || bounds.bottom < view.top || bounds.top > view.bottom;
        if (outside)
            return false;
        std::size_t loopStart = 0;
        for (const std::size_t loopEnd : loopEnds)
        {
            path.moveTo(points[loopStart]);
            for (std::size_t i = loopStart + 1; i != loopEnd; ++i)
                path.lineTo(points[i]);
            path.close();
            loopStart = loopEnd;
        }
        return true;
    }

    float EarthScene::Globe::sunlightAt(const Direction direction) const
    {
        return direction.x * sun.x + direction.y * sun.y + direction.z * sun.z;
    }

    // Until the land's map is built the towns stand anywhere on the land, then they move to where
    // the map places them. See PathArt#earthscene
    EarthScene::BuiltCoasts EarthScene::buildCoasts()
    {
        std::vector<Coastline> coastlines;
        for (LandOutline& outline : landOutlines())
        {
            // The shoelace sum, above 0 when the points run counterclockwise.
            float twiceArea = 0.0f;
            for (std::size_t i = 0; i != outline.size(); ++i)
            {
                const FloatPoint from = outline[i];
                const FloatPoint to = outline[(i + 1) % outline.size()];
                twiceArea += from.x * to.y - to.x * from.y;
            }
            std::vector<Direction> onEarth;
            onEarth.reserve(outline.size());
            for (const FloatPoint lonLat : outline)
                onEarth.push_back(Globe::onEarthAt(lonLat));
            const bool iceSheet = isIceSheet(outline);
            coastlines.push_back({
                .lonLat = std::move(outline),
                .onEarth = std::move(onEarth),
                .counterClockwise = twiceArea > 0.0f,
                .iceSheet = iceSheet
            });
        }
        std::vector<City> towns = evenTowns(coastlines);
        return { .coastlines = std::move(coastlines), .towns = std::move(towns) };
    }

    // The towns go anywhere on the land up to 62 degrees north, the ice sheet left out.
    std::vector<EarthScene::City> EarthScene::evenTowns(
        const std::span<const Coastline> coastlines)
    {
        std::vector<std::span<const FloatPoint>> inhabited;
        for (const Coastline& coastline : coastlines)
        {
            if (!coastline.iceSheet)
                inhabited.push_back(coastline.lonLat);
        }
        const std::vector<std::uint8_t> land = landCellsOf(inhabited);
        std::vector<City> towns;
        for (int seed = 5001; std::ssize(towns) != k_landTownCount; ++seed)
        {
            const FloatPoint site = townSiteAt(seed);
            const std::optional<std::size_t> cell = townCellAt(site);
            if (site.y > 62.0f || !cell || land[*cell] == 0)
                continue;
            towns.push_back({
                .onEarth = Globe::onEarthAt(site),
                .brightness = pseudoRandom(seed, 73.0f)
            });
        }
        return towns;
    }

    // **Each site is kept by a weight the land's map gives it**: highest on low ground, and
    // thinning with height, in the deep desert, in the tundra and a little in the northern forest.
    // The sites over the land weigh more the nearer they are to a coast; those on the coastline
    // weigh the most. The ice sheet has none, though its coast counts. A site within
    // k_leastTownSpacing of a town already placed is tried again, and a town that would glow within
    // k_leastGlowSpacing of a glowing one is a plain light. The sites are read from the map 8 at a
    // time, as it reads 8 points at once. See PathArt#earthscene
    std::vector<EarthScene::City> EarthScene::townsBy(const EarthMap& map,
        const std::span<const Coastline> coastlines)
    {
        std::vector<std::span<const FloatPoint>> outlines;
        std::vector<std::span<const FloatPoint>> inhabited;
        std::size_t inhabitedPointCount = 0;
        for (const Coastline& coastline : coastlines)
        {
            outlines.push_back(coastline.lonLat);
            if (coastline.iceSheet)
                continue;
            inhabited.push_back(coastline.lonLat);
            inhabitedPointCount += coastline.lonLat.size();
        }
        const std::vector<std::uint8_t> land = landCellsOf(inhabited);
        const std::vector<float> coastDistances = coastDistancesOf(outlines);
        // A site tried for a town, how much its nearness to a coast weighs, and its seed.
        struct Site
        {
            FloatPoint lonLat;
            float coastWeight;
            int seed;
        };
        std::array<Site, 8> sites{};
        std::array<float, 8> longitudes{};
        std::array<float, 8> latitudes{};
        // Each site is read for the ground of half a degree about it.
        std::array<float, 8> spans{};
        spans.fill(0.5f * k_degree);
        std::array<float, 8> heights{};
        std::array<float, 8> slopesEast{};
        std::array<float, 8> slopesNorth{};
        std::array<float, 8> variation{};
        std::array<float, 8> dryness{};
        std::array<float, 8> forest{};
        std::array<float, 8> tundra{};
        std::vector<City> towns;
        // The towns placed so far by the cell of the towns' grid each stands in.
        std::unordered_map<std::size_t, std::vector<Direction>> townsInCell;
        std::vector<Direction> glowingTowns;
        const float townSpacingCosine = std::cos(k_leastTownSpacing * k_degree);
        const float glowSpacingCosine = std::cos(k_leastGlowSpacing * k_degree);
        const auto cosineBetween = [](const Direction& first, const Direction& second) {
            return first.x * second.x + first.y * second.y + first.z * second.z;
        };
        const auto standsNearTown = [&](const FloatPoint lonLat, const Direction& onEarth) {
            for (const std::size_t cell : townCellsNear(lonLat, k_leastTownSpacing))
            {
                const auto placed = townsInCell.find(cell);
                if (placed == townsInCell.end())
                    continue;
                for (const Direction& town : placed->second)
                {
                    if (cosineBetween(town, onEarth) > townSpacingCosine)
                        return true;
                }
            }
            return false;
        };
        const auto standsNearGlow = [&](const Direction& onEarth) {
            return std::ranges::any_of(glowingTowns, [&](const Direction& glowingTown) {
                return cosineBetween(glowingTown, onEarth) > glowSpacingCosine;
            });
        };
        // Tries the sites `siteAt` gives from seed `firstSeed` on, until `count` more towns stand.
        const auto placeTowns = [&](int count, int firstSeed, const auto& siteAt) {
            const std::size_t end = towns.size() + static_cast<std::size_t>(count);
            int seed = firstSeed;
            while (towns.size() != end)
            {
                for (std::size_t gathered = 0; gathered != sites.size(); ++seed)
                {
                    const std::optional<Site> site = siteAt(seed);
                    if (!site)
                        continue;
                    sites[gathered] = *site;
                    longitudes[gathered] = site->lonLat.x * k_degree;
                    latitudes[gathered] = site->lonLat.y * k_degree;
                    ++gathered;
                }
                map.readLand(MapQuery{
                    .longitudes = longitudes,
                    .latitudes = latitudes,
                    .longitudeSpans = spans,
                    .latitudeSpans = spans
                }, Relief::Broad, LandPoints{
                    .heights = heights,
                    .slopesEast = slopesEast,
                    .slopesNorth = slopesNorth,
                    .variation = variation,
                    .dryness = dryness,
                    .forest = forest,
                    .tundra = tundra
                });
                for (std::size_t i = 0; i != sites.size() && towns.size() != end; ++i)
                {
                    const float highGround = smoothStep(0.06f, 0.3f, heights[i]);
                    const float weight = (1.0f - highGround)
                        * (1.0f - 0.92f * smoothStep(0.4f, 0.8f, dryness[i]))
                        * (1.0f - smoothStep(0.2f, 0.8f, tundra[i]))
                        * (1.0f - 0.55f * forest[i])
                        * sites[i].coastWeight;
                    if (pseudoRandom(sites[i].seed, 74.0f) > weight)
                        continue;
                    const Direction onEarth = Globe::onEarthAt(sites[i].lonLat);
                    if (standsNearTown(sites[i].lonLat, onEarth))
                        continue;
                    float brightness = pseudoRandom(sites[i].seed, 73.0f);
                    if (brightness > k_glowingBrightness)
                    {
                        if (standsNearGlow(onEarth))
                            brightness = k_glowingBrightness;
                        else
                            glowingTowns.push_back(onEarth);
                    }
                    townsInCell[townCellAt(sites[i].lonLat).value()].push_back(onEarth);
                    towns.push_back({ .onEarth = onEarth, .brightness = brightness });
                }
            }
        };
        placeTowns(k_landTownCount, 5001, [&](int seed) -> std::optional<Site> {
            const FloatPoint lonLat = townSiteAt(seed);
            const std::optional<std::size_t> cell = townCellAt(lonLat);
            if (!cell || land[*cell] == 0)
                return std::nullopt;
            const float nearCoast = std::exp(-coastDistances[*cell] / 2.5f);
            return Site{ .lonLat = lonLat, .coastWeight = 0.22f + 0.78f * nearCoast, .seed = seed };
        });
        placeTowns(k_coastlineTownCount, 90001, [&](int seed) -> std::optional<Site> {
            const FloatPoint lonLat = coastlineSiteAt(inhabited, inhabitedPointCount, seed);
            if (lonLat.y < k_townSouth || lonLat.y > k_townNorth)
                return std::nullopt;
            return Site{ .lonLat = lonLat, .coastWeight = 1.0f, .seed = seed };
        });
        return towns;
    }

    // The space and the ocean are kept for the lowest altitude, where the view spends its time.
    void EarthScene::drawSpace(const SceneFrame& frame)
    {
        fillSpace(frame, globeDiscAt(frame.width, frame.height, 0.0f));
    }

    void EarthScene::drawOcean(const SceneFrame& frame)
    {
        fillOcean(frame, globeDiscAt(frame.width, frame.height, 0.0f));
    }

    // The land and the ice sheet make one path each, so a sea inside the land is cut out of it.
    void EarthScene::drawFlatLand(const SceneFrame& frame, const Globe& globe) const
    {
        const FloatRect view = { 0.0f, 0.0f, frame.width, frame.height };
        const EarthTurn turn = globe.turn();
        std::vector<Direction> outline;
        for (const bool iceSheet : { false, true })
        {
            PixelPath path;
            bool inView = false;
            for (const Coastline& coastline : m_coastlines)
            {
                if (coastline.iceSheet != iceSheet)
                    continue;
                outline.clear();
                for (const Direction onEarth : coastline.onEarth)
                    outline.push_back(turn.applied(onEarth));
                if (globe.addNearSide(outline, coastline.counterClockwise, view, path))
                    inView = true;
            }
            if (!inView)
                continue;
            const Color color = iceSheet ? EarthSceneColors::Snow : EarthSceneColors::Grass;
            fillShape(frame, path, SolidColor{ color });
        }
    }

    // **The land is worked out in bands on all the cores**, each pixel taking the share of it the
    // coastlines enclose as its opacity, so the coasts are drawn alike on either backend. The
    // points stand every 2nd pixel and row on a globe over 1300 pixels in radius, and every 4th
    // over 3200, where the land's map is coarser than that still.
    void EarthScene::drawLand(const SceneFrame& frame, const Globe& globe)
    {
        if (!m_earthMap)
        {
            if (!m_landBuilt.valid())
            {
                m_landBuilt = std::async(std::launch::async, [coastlines = m_coastlines]() {
                    std::unique_ptr<EarthMap> map = std::make_unique<EarthMap>();
                    std::vector<City> towns = townsBy(*map, coastlines);
                    return BuiltLand{ .map = std::move(map), .towns = std::move(towns) };
                });
            }
            if (m_landBuilt.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready)
            {
                drawFlatLand(frame, globe);
                return;
            }
            BuiltLand built = m_landBuilt.get();
            m_earthMap = std::move(built.map);
            m_cities = std::move(built.towns);
        }

        const Matrix3x2 toPixels = frame.canvas.transform() * frame.toCanvas;
        const float radiusInPixels = globe.radius * toPixels.a;
        const int step = pointStepFor(radiusInPixels);
        // **The realistic coast is blurred as much as the land is**, and its loops along the rim
        // are closed past the sphere, far enough for the blur to leave the limb whole.
        const float coastBlur = look() == SceneLook::Realistic
            ? k_coastBlurPerStep * static_cast<float>(step)
            : 0.0f;
        const int blurReach = blurReachOf(coastBlur);
        const float rimRadius = coastBlur > 0.0f
            ? 1.0f + static_cast<float>(blurReach + 1) / radiusInPixels
            : 1.0f;
        std::vector<CoastEdge> coast;
        constexpr float k_far{ std::numeric_limits<float>::max() };
        FloatRect landBox = { k_far, k_far, -k_far, -k_far };
        std::vector<Direction> outline;
        std::vector<FloatPoint> points;
        std::vector<std::size_t> loopEnds;
        const EarthTurn turn = globe.turn();
        const Matrix3x2 viewToPixels = toPixels * globe.toScreen();
        for (const Coastline& coastline : m_coastlines)
        {
            outline.clear();
            for (const Direction onEarth : coastline.onEarth)
                outline.push_back(turn.applied(onEarth));
            globe.nearSideOf(outline, coastline.counterClockwise, rimRadius, points, loopEnds);
            std::size_t loopStart = 0;
            for (const std::size_t loopEnd : loopEnds)
            {
                for (std::size_t i = loopStart; i != loopEnd; ++i)
                {
                    const std::size_t next = i + 1 == loopEnd ? loopStart : i + 1;
                    const FloatPoint from = viewToPixels.transform(points[i]);
                    coast.push_back({ from, viewToPixels.transform(points[next]) });
                    landBox = {
                        std::min(landBox.left, from.x),
                        std::min(landBox.top, from.y),
                        std::max(landBox.right, from.x),
                        std::max(landBox.bottom, from.y)
                    };
                }
                loopStart = loopEnd;
            }
        }
        if (coast.empty())
            return;
        // Inside a path clip the canvas's clip box stands in the clip's own coordinates, so the
        // land is cut to the canvas and the backend cuts it to the clip.
        const FloatRect view = { 0.0f, 0.0f, frame.width, frame.height };
        const FloatRect canvasPixels = {
            0.0f,
            0.0f,
            static_cast<float>(frame.canvas.width()),
            static_cast<float>(frame.canvas.height())
        };
        const IntRect inView = FloatRect::intersection(FloatRect::intersection(
            toPixels.mapRect(view), canvasPixels), landBox).roundedOut();
        if (inView.empty())
            return;
        const float tip = globe.tip;
        const LandPicture picture = {
            .map = m_earthMap.get(),
            .ground = sphereViewOf(toPixels, globe.center, globe.radius, globe.tilt, tip,
                globe.spin),
            .look = look(),
            .step = step,
            .sun = {
                globe.sun.x,
                globe.sun.y * std::cos(tip) - globe.sun.z * std::sin(tip),
                globe.sun.y * std::sin(tip) + globe.sun.z * std::cos(tip)
            },
            .sphereCenter = toPixels.transform(globe.center),
            .sphereRadii = { radiusInPixels, globe.radius * toPixels.d },
            .left = inView.left,
            .right = inView.right,
            .coastBlur = coastBlur
        };
        const float sphereTop = picture.sphereCenter.y - picture.sphereRadii.y;
        const int top = std::max(inView.top, static_cast<int>(std::floor(sphereTop)))
            / picture.step * picture.step;
        if (top >= inView.bottom)
            return;
        const LandPalette palette = landPalette();
        const int bandCount = (inView.bottom - top + k_bandRows - 1) / k_bandRows;
        const BandedCoast banded = bandedCoast(coast, top, bandCount, blurReach);
        m_landBands.resize(static_cast<std::size_t>(bandCount));
        spreadOverCores(bandCount, [&](int band) {
            const int bandTop = top + band * k_bandRows;
            const int rows = std::min(k_bandRows, inView.bottom - bandTop);
            const std::size_t start = banded.starts[static_cast<std::size_t>(band)];
            const std::size_t end = banded.starts[static_cast<std::size_t>(band) + 1];
            const std::span<const CoastEdge> bandCoast{ banded.edges.data() + start, end - start };
            PixelBand& drawn = m_landBands[static_cast<std::size_t>(band)];
            drawn.at = rasterizeLandBand(picture, palette, bandCoast, bandTop, rows, drawn.pixels);
        });
        layBands(frame, m_landBands, canvasPixels);
    }

    // **The sea is coloured by its depth**, read from the land's map at points twice as far apart
    // as the land's, in bands of rows spread over the cores. See PathArt#earthscene
    void EarthScene::drawSea(const SceneFrame& frame, const Globe& globe)
    {
        const Matrix3x2 toPixels = frame.canvas.transform() * frame.toCanvas;
        const FloatRect canvasPixels = {
            0.0f,
            0.0f,
            static_cast<float>(frame.canvas.width()),
            static_cast<float>(frame.canvas.height())
        };
        const FloatRect sphere = toPixels.mapRect({
            globe.center.x - globe.radius,
            globe.center.y - globe.radius,
            globe.center.x + globe.radius,
            globe.center.y + globe.radius
        });
        const IntRect inView = FloatRect::intersection(FloatRect::intersection(
            toPixels.mapRect({ 0.0f, 0.0f, frame.width, frame.height }), canvasPixels),
            sphere).roundedOut();
        if (inView.empty())
            return;
        static const std::vector<Color> shades = seaShades();
        const float radiusInPixels = globe.radius * toPixels.a;
        const SeaPicture picture = {
            .map = m_earthMap.get(),
            .ground = sphereViewOf(toPixels, globe.center, globe.radius, globe.tilt, globe.tip,
                globe.spin),
            .look = look(),
            .shades = shades,
            .step = 2 * pointStepFor(radiusInPixels),
            .sphereCenter = toPixels.transform(globe.center),
            .sphereRadii = { radiusInPixels, globe.radius * toPixels.d },
            .left = inView.left,
            .right = inView.right
        };
        const int top = inView.top / picture.step * picture.step;
        const int bandCount = (inView.bottom - top + k_bandRows - 1) / k_bandRows;
        m_seaBands.resize(static_cast<std::size_t>(bandCount));
        spreadOverCores(bandCount, [&](int band) {
            const int bandTop = top + band * k_bandRows;
            const int rows = std::min(k_bandRows, inView.bottom - bandTop);
            PixelBand& drawn = m_seaBands[static_cast<std::size_t>(band)];
            drawn.at = rasterizeSeaBand(picture, bandTop, rows, drawn.pixels);
        });
        layBands(frame, m_seaBands, canvasPixels);
    }

    // **The night falls over the globe by the sunlight on it**, worked out pixel by pixel in bands
    // spread over the cores, so the terminator curves as it does at any altitude.
    void EarthScene::drawNight(const SceneFrame& frame, const Globe& globe)
    {
        const Matrix3x2 toPixels = frame.canvas.transform() * frame.toCanvas;
        const FloatRect canvasPixels = {
            0.0f,
            0.0f,
            static_cast<float>(frame.canvas.width()),
            static_cast<float>(frame.canvas.height())
        };
        const FloatRect sphere = toPixels.mapRect({
            globe.center.x - globe.radius,
            globe.center.y - globe.radius,
            globe.center.x + globe.radius,
            globe.center.y + globe.radius
        });
        const IntRect inView = FloatRect::intersection(FloatRect::intersection(
            toPixels.mapRect({ 0.0f, 0.0f, frame.width, frame.height }), canvasPixels),
            sphere).roundedOut();
        if (inView.empty())
            return;
        static const std::vector<Color> shades = nightShades();
        const NightPicture picture = {
            .night = nightViewOf(toPixels, globe.center, globe.radius, globe.tilt, globe.sun.x,
                globe.sun.y, globe.sun.z),
            .shades = shades,
            .edgeSoftness = 1.0f,
            .left = inView.left,
            .right = inView.right
        };
        const int bandCount = (inView.bottom - inView.top + k_bandRows - 1) / k_bandRows;
        m_nightBands.resize(static_cast<std::size_t>(bandCount));
        spreadOverCores(bandCount, [&](int band) {
            const int bandTop = inView.top + band * k_bandRows;
            const int rows = std::min(k_bandRows, inView.bottom - bandTop);
            PixelBand& drawn = m_nightBands[static_cast<std::size_t>(band)];
            drawn.at = rasterizeNightBand(picture, bandTop, rows, drawn.pixels);
        });
        layBands(frame, m_nightBands, canvasPixels);
    }

    // The lights come on a little way past the terminator and fade toward the edge of the globe.
    void EarthScene::drawCityLights(const SceneFrame& frame, const Globe& globe,
        const float cycleTime) const
    {
        // The lights shrink as the globe does, a little less fast.
        const float unit = frame.unit * std::sqrt(globe.radius
            / globeDiscAt(frame.width, frame.height, 0.0f).radius);
        const EarthTurn turn = globe.turn();
        const Matrix3x2 toScreen = globe.toScreen();
        // The glows go down city by city, and the lights over all of them afterwards.
        std::vector<LitCity> lit;
        for (const City& city : m_cities)
        {
            const Direction direction = turn.applied(city.onEarth);
            if (direction.z < 0.02f)
                continue;
            const FloatPoint point = toScreen.transform({ direction.x, direction.y });
            const float margin = 10.0f * frame.unit;
            const bool outside = point.x < -margin || point.x > frame.width + margin
                || point.y > frame.height + margin;
            if (outside)
                continue;
            const float pastLine = smoothStep(-0.01f, -0.06f, globe.sunlightAt(direction));
            const float strength = pastLine * smoothStep(0.02f, 0.12f, direction.z);
            if (strength <= 0.01f)
                continue;
            const float phase = city.brightness * 40.0f;
            const float wave = std::sin(k_2Pi * 3.0f * k_lively * cycleTime + phase);
            const float shimmer = 0.65f + 0.35f * wave;
            const float opacity = strength * shimmer;
            const int step = std::min(k_cityOpacitySteps - 1,
                static_cast<int>(opacity * k_cityOpacitySteps));
            const float radius = (0.8f + 1.3f * city.brightness) * unit
                * (0.4f + 0.6f * direction.z);
            lit.push_back({ .at = point, .radius = radius, .step = step });
            if (city.brightness > k_glowingBrightness)
                drawGlow(frame, point, 7.0f * unit,
                    EarthSceneColors::CityGlow.withOpacity(0.45f * strength));
        }
        for (int step = 0; step != k_cityOpacitySteps; ++step)
        {
            const float opacity = (static_cast<float>(step) + 0.5f) / k_cityOpacitySteps;
            const Color light = EarthSceneColors::CityLight.withOpacity(opacity);
            ScatteredShapes lights{ frame, SolidColor{ light } };
            for (const LitCity& city : lit)
            {
                if (city.step == step)
                    addEllipse(lights.next(), city.at, city.radius, city.radius);
            }
            lights.draw();
        }
    }

    // **Three layers of cloud float above the ground**, each a map on a sphere of its own a little
    // larger than the Earth's, turning about an axis of its own. See PathArt#earthscene
    void EarthScene::drawClouds(const SceneFrame& frame, const Globe& globe,
        const double sceneSeconds)
    {
        if (!m_cloudMaps)
        {
            if (!m_cloudMapsBuilt.valid())
            {
                m_cloudMapsBuilt = std::async(std::launch::async, []() {
                    const auto mapOf = [](const CloudLayer& layer) {
                        return CloudMap{ layer.kind, layer.seed, layer.cover };
                    };
                    return std::make_unique<CloudMaps>(CloudMaps{
                        mapOf(k_cloudLayers[0]),
                        mapOf(k_cloudLayers[1]),
                        mapOf(k_cloudLayers[2])
                    });
                });
            }
            if (m_cloudMapsBuilt.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready)
                return;
            m_cloudMaps = m_cloudMapsBuilt.get();
        }

        // The scene is drawn under a scale and a move, so a pixel maps back by undoing them.
        const Matrix3x2 toPixels = frame.canvas.transform() * frame.toCanvas;
        const FloatPoint pixelScale = { toPixels.a, toPixels.d };
        const FloatPoint pixelShift = { toPixels.e, toPixels.f };
        const IntRect inView = FloatRect::intersection(
            toPixels.mapRect({ 0.0f, 0.0f, frame.width, frame.height }),
            frame.canvas.clipBox()).roundedOut();
        if (inView.empty())
            return;

        const float cloudOpacity = static_cast<float>(EarthSceneColors::Cloud.alpha) / 255.0f;
        std::array<CloudLayerView, k_cloudLayers.size()> layers{};
        float outerRadius = 0.0f;
        for (std::size_t i = 0; i != k_cloudLayers.size(); ++i)
        {
            const CloudLayer& layer = k_cloudLayers[i];
            const float radius = globe.radius * (1.0f + layer.height);
            const float drift = k_2Pi * fractionOf(sceneSeconds * layer.driftPerSecond / 360.0);
            // The layer's axis leans off the Earth's by a turn fixed to the Earth.
            const ViewAngles angles = ViewAngles::Of(globe.fromEarth
                .after(Rotation::About({ 1.0f, 0.0f, 0.0f }, layer.axisTilt * k_degree))
                .after(Rotation::About({ 0.0f, 1.0f, 0.0f }, drift)));
            layers[i] = {
                .map = &(*m_cloudMaps)[i],
                .sphere = sphereViewOf(toPixels, globe.center, radius, angles.tilt, angles.tip,
                    angles.spin),
                .opacity = layer.opacity * cloudOpacity
            };
            outerRadius = std::max(outerRadius, radius);
        }

        static const std::vector<Color> shades = cloudShades();
        const CloudPicture picture = {
            .layers = layers,
            .look = look(),
            // The cloud maps' texels are twice the land's, and so are the points' steps.
            .step = 2 * pointStepFor(globe.radius * pixelScale.x),
            .sphereCenter = toPixels.transform(globe.center),
            .sphereRadii = { outerRadius * pixelScale.x, outerRadius * pixelScale.y },
            .left = inView.left,
            .right = inView.right,
            .shades = shades,
            .night = nightViewOf(toPixels, globe.center, globe.radius, globe.tilt, globe.sun.x,
                globe.sun.y, globe.sun.z)
        };

        const float sphereTop = picture.sphereCenter.y - picture.sphereRadii.y;
        const int top = std::max(inView.top, static_cast<int>(std::floor(sphereTop)))
            / picture.step * picture.step;
        if (top >= inView.bottom)
            return;
        const int bandCount = (inView.bottom - top + k_bandRows - 1) / k_bandRows;
        m_cloudBands.resize(static_cast<std::size_t>(bandCount));
        spreadOverCores(bandCount, [&](int band) {
            const int bandTop = top + band * k_bandRows;
            const int rows = std::min(k_bandRows, inView.bottom - bandTop);
            PixelBand& drawn = m_cloudBands[static_cast<std::size_t>(band)];
            drawn.at = rasterizeCloudBand(picture, bandTop, rows, drawn.pixels);
        });
        layBands(frame, m_cloudBands, frame.canvas.clipBox());
    }

    // Direct2D places a view through the canvas's transform, and the bands are in pixels.
    void EarthScene::layBands(const SceneFrame& frame, const std::span<const PixelBand> bands,
        const FloatRect& cut)
    {
        std::vector<PixelView> views;
        views.reserve(bands.size());
        for (const PixelBand& band : bands)
        {
            if (!band.pixels.empty())
                views.push_back(band.pixels.pixelView(band.at, cut));
        }
        const Matrix3x2 canvasTransform = frame.canvas.transform();
        frame.canvas.resetTransform();
        frame.canvas.drawPixelViews(views);
        frame.canvas.setTransform(canvasTransform);
    }

    // **The sunlight on the limb rises evenly across it toward the Sun**, as the limb stands square
    // to the view, so one gradient along the Sun's way across the screen colours it at any
    // altitude - blue by day, orange at dusk, dark at night. Each of the three rings is a glow
    // fading from its circle out to both sides, so none of them shows an edge.
    void EarthScene::drawAtmosphere(const SceneFrame& frame, const Globe& globe)
    {
        constexpr int k_stops = 41;
        const FloatPoint sunAcross = rotated({ globe.sun.x, globe.sun.y }, globe.tilt);
        const float sunReach = std::hypot(sunAcross.x, sunAcross.y);
        // The screen's y runs down.
        const FloatPoint sunward = sunReach > 0.0001f
            ? FloatPoint{ sunAcross.x / sunReach, -sunAcross.y / sunReach }
            : FloatPoint{ 1.0f, 0.0f };
        const FloatPoint from = globe.center - sunward * globe.radius;
        const FloatPoint to = globe.center + sunward * globe.radius;
        std::array<Color, k_stops> colors{};
        for (int i = 0; i != k_stops; ++i)
        {
            const float position = static_cast<float>(i) / (k_stops - 1);
            colors[static_cast<std::size_t>(i)] = atmosphereColor(
                sunReach * (2.0f * position - 1.0f));
        }
        const auto drawRing = [&](float ringRadius, float reach, float strength) {
            LinearGradient gradient{
                .startPoint = from,
                .endPoint = to,
                .stops = {}
            };
            for (int i = 0; i != k_stops; ++i)
            {
                const Color color = colors[static_cast<std::size_t>(i)];
                const float opacity = static_cast<float>(color.alpha) / 255.0f * strength;
                gradient.stops.push_back({
                    .position = static_cast<float>(i) / (k_stops - 1),
                    .color = color.withOpacity(std::min(1.0f, opacity))
                });
            }
            PixelPath ring;
            addEllipse(ring, globe.center, ringRadius, ringRadius);
            PathDrawLayer glow = PathDrawLayer::OuterGlow({}, reach);
            glow.brush = std::move(gradient);
            frame.canvas.drawPath(ring, std::span{ &glow, 1 }, &frame.toCanvas);
        };
        drawRing(globe.radius * 1.012f, globe.radius * 0.02f, 0.3f);
        drawRing(globe.radius * 1.001f, std::max(globe.radius * 0.005f, 2.5f * frame.unit), 1.0f);
        drawRing(globe.radius * 0.988f, globe.radius * 0.016f, 0.34f);
    }

    double EarthScene::secondsSinceLastFrame(const double sceneSeconds)
    {
        const double elapsed = m_lastFrameAt
            ? std::max(sceneSeconds - *m_lastFrameAt, 0.0)
            : 0.0;
        m_lastFrameAt = sceneSeconds;
        return elapsed;
    }

    // **The globe holds still while a press holds the view**: a frame drawn while it is held counts
    // the time since the last frame as still, and a let-go moves the globe on at once.
    double EarthScene::stillSecondsAfter(const double frameSeconds)
    {
        if (m_viewHeld)
            m_stillSeconds += frameSeconds;
        return m_stillSeconds;
    }

    // **The Sun glides as the wheel's notches come in**: most of the way at first, slowing as it
    // nears, and drawn where it was moved once it is that near. The time is the scene's.
    void EarthScene::moveShownSun(const double frameSeconds)
    {
        const double toCome = m_sunTurns - m_shownSunTurns;
        const double step = toCome * (1.0 - std::exp(-frameSeconds / k_sunGliding));
        m_shownSunTurns = std::abs(toCome - step) < k_sunGlideEnd
            ? m_sunTurns
            : m_shownSunTurns + step;
    }

    double EarthScene::earthTurnsAt(const double timeInOrbits) const
    {
        const double turnsByTime = timeInOrbits / k_orbitsPerEarthTurn;
        return m_earthTurnsAhead + turnsByTime;
    }

    // Hanging, the view turns with the Earth, and leans back as it would on the orbit.
    EarthScene::Rotation EarthScene::cameraAt(const double orbits, const double earthTurns) const
    {
        if (!m_view.hanging)
        {
            return m_view.orbitTurn.after(Globe::orbitAxesAt(orbits, m_view.altitude,
                m_view.trackAngle));
        }
        const Rotation lean = Rotation::About({ 1.0f, 0.0f, 0.0f }, viewBehindAt(m_view.altitude));
        return Globe::earthAxesAt(earthTurns).after(m_view.hangingCamera).after(lean);
    }

    // The held point is the middle of the view across it, low in the frame at the lowest view and
    // at the globe's middle with the whole globe in it.
    EarthScene::Direction EarthScene::heldPointAt(const FloatPoint offset,
        const FloatRect& bounds) const
    {
        // How far from the middle toward the rim a point is taken, so it never lies on the rim.
        constexpr float k_rimReach{ 0.98f };
        const GlobeDisc disc = globeDiscAt(bounds.width(), bounds.height(), m_view.altitude);
        const FloatPoint point = {
            bounds.width() * 0.5f + offset.x,
            std::lerp(bounds.height() * 0.72f, disc.center.y, m_view.altitude) + offset.y
        };
        FloatPoint onDisc = {
            (point.x - disc.center.x) / disc.radius,
            (disc.center.y - point.y) / disc.radius
        };
        const float reach = std::hypot(onDisc.x, onDisc.y);
        if (reach > k_rimReach)
            onDisc = onDisc * (k_rimReach / reach);
        return {
            onDisc.x,
            onDisc.y,
            std::sqrt(1.0f - onDisc.x * onDisc.x - onDisc.y * onDisc.y)
        };
    }

    // **The ground under the held point moves by the orbit and by the Earth's turn.** The orbit's
    // part turns with the track angle and the Earth's does not, so the angle taken is the one at
    // which the two add up to a motion along `speed`. See PathArt#earthscene
    float EarthScene::trackAngleFor(const FloatPoint speed, const FloatRect& bounds) const
    {
        const auto cross = [](const Direction& first, const Direction& second) {
            return Direction{
                first.y * second.z - first.z * second.y,
                first.z * second.x - first.x * second.z,
                first.x * second.y - first.y * second.x
            };
        };
        const auto across = [](const FloatPoint first, const FloatPoint second) {
            return first.x * second.y - first.y * second.x;
        };
        const Direction held = heldPointAt({ 0.0f, 0.0f }, bounds);
        const float behind = viewBehindAt(m_view.altitude);
        // The orbit's axis as the view sees it at a track angle of 0, and the vertical it turns
        // about at another.
        const Direction orbitAxis = { 0.0f, std::cos(behind), -std::sin(behind) };
        const Direction vertical = { 0.0f, std::sin(behind), std::cos(behind) };
        // How the held point moves across the view for each radian the satellite flies, with the
        // orbit's axis at a track angle of 0 and at a quarter turn.
        const Direction byOrbit = cross(orbitAxis, held);
        const Direction byQuarterTurnedOrbit = cross(cross(vertical, orbitAxis), held);
        const FloatPoint atNoAngle = { byOrbit.x, byOrbit.y };
        const FloatPoint atQuarterTurn = { -byQuarterTurnedOrbit.x, -byQuarterTurnedOrbit.y };
        // And for the Earth's turn over the same while, about its axis as the view sees it.
        const Rotation camera = cameraAt(m_shownOrbits, earthTurnsAt(m_shownTimeInOrbits));
        const Direction earthAxis = { camera.x.y, camera.y.y, camera.z.y };
        const float turnsPerOrbit = static_cast<float>(1.0 / k_orbitsPerEarthTurn);
        const Direction byTurn = cross(earthAxis, held);
        const FloatPoint byEarth = { byTurn.x * turnsPerOrbit, byTurn.y * turnsPerOrbit };
        // The way the pointer went, up the view.
        const FloatPoint way = { speed.x, -speed.y };
        // The angles at which the motion lies along the way solve a cos + b sin + c = 0.
        const float cosineShare = across(atNoAngle, way);
        const float sineShare = across(atQuarterTurn, way);
        const float steadyShare = across(byEarth, way);
        const float middle = std::atan2(sineShare, cosineShare);
        const float spread = std::acos(std::clamp(
            -steadyShare / std::hypot(cosineShare, sineShare), -1.0f, 1.0f));
        const auto alongWayAt = [&](const float trackAngle) {
            const float cosine = std::cos(trackAngle);
            const float sine = std::sin(trackAngle);
            const FloatPoint motion = {
                atNoAngle.x * cosine + atQuarterTurn.x * sine + byEarth.x,
                atNoAngle.y * cosine + atQuarterTurn.y * sine + byEarth.y
            };
            return motion.x * way.x + motion.y * way.y;
        };
        const float first = middle + spread;
        const float second = middle - spread;
        const float taken = alongWayAt(first) >= alongWayAt(second) ? first : second;
        // Kept between a half turn either way, which is how the config shows it.
        return std::remainder(taken, k_2Pi);
    }

    // The ground stands in the view from here on, and only the day and the night move over it.
    void EarthScene::hangSatellite()
    {
        const double earthTurns = earthTurnsAt(m_shownTimeInOrbits);
        const Rotation camera = cameraAt(m_shownOrbits, earthTurns);
        const Rotation lean = Rotation::About({ 1.0f, 0.0f, 0.0f }, viewBehindAt(m_view.altitude));
        m_view.hangingCamera = Globe::earthAxesAt(earthTurns).inverse().after(camera)
            .after(lean.inverse()).orthonormal();
        m_view.hanging = true;
        m_view.orbitsAhead = m_shownOrbits;
    }

    // **The orbit is laid on the picture as it stands**, read before the new angle turns the view,
    // so the next frame is the last one and the satellite flies on from it at the orbit's own pace.
    void EarthScene::flySatelliteOn(const float trackAngle)
    {
        const Rotation camera = cameraAt(m_shownOrbits, earthTurnsAt(m_shownTimeInOrbits));
        m_view.trackAngle = trackAngle;
        m_view.orbitTurn = camera.after(Globe::orbitAxesAt(m_shownOrbits, m_view.altitude,
            trackAngle).inverse()).orthonormal();
        m_view.hanging = false;
        m_view.orbitsAhead = m_shownOrbits - m_shownTimeInOrbits;
    }

    Dom::Dt::Section EarthScene::rotationSchema(const std::wstring_view key,
        const Rotation& start)
    {
        const AxisTurn turn = start.axisTurn();
        return Dom::Dt::Section{
            key,
            Dom::Dt::Value{ ViewNodes::AxisX, turn.axis.x },
            Dom::Dt::Value{ ViewNodes::AxisY, turn.axis.y },
            Dom::Dt::Value{ ViewNodes::AxisZ, turn.axis.z },
            Dom::Dt::Value{ ViewNodes::Degrees, turn.angle / k_degree }
        };
    }

    void EarthScene::storeRotation(Dom::Section& section, const Rotation& rotation)
    {
        const AxisTurn turn = rotation.axisTurn();
        (section / ViewNodes::AxisX).set(turn.axis.x);
        (section / ViewNodes::AxisY).set(turn.axis.y);
        (section / ViewNodes::AxisZ).set(turn.axis.z);
        (section / ViewNodes::Degrees).set(turn.angle / k_degree);
    }

    EarthScene::Rotation EarthScene::restoredRotation(const Dom::Section& section,
        const Rotation& fallback)
    {
        const Direction axis = {
            (section / ViewNodes::AxisX).get<float>(),
            (section / ViewNodes::AxisY).get<float>(),
            (section / ViewNodes::AxisZ).get<float>()
        };
        const float axisLength = std::hypot(axis.x, axis.y, axis.z);
        if (axisLength == 0.0f)
            return fallback;
        const Direction unitAxis = {
            axis.x / axisLength,
            axis.y / axisLength,
            axis.z / axisLength
        };
        return Rotation::About(unitAxis, (section / ViewNodes::Degrees).get<float>() * k_degree);
    }

}
