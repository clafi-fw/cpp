module ClaFi.PathArt.SatelliteScene;

import ClaFi.PathArt.SatelliteSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.PathArt.StarsEffect;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace SatelliteSceneColors
    {
        constexpr Color SpaceTop{ 0xFF02030A };
        constexpr Color SpaceBottom{ 0xFF0A1230 };
        constexpr Color Star{ 0xFFF0F3FF };
        constexpr Color SunGlare{ 0xFFFFF1D6 };
        constexpr Color OceanLimb{ 0xFF3F8EE0 };
        constexpr Color OceanDeep{ 0xFF0C3D84 };
        constexpr Color Cloud{ 0x5CFFFFFF };
        constexpr Color Night{ 0xFF020614 };
        constexpr Color NightBlue{ 0xFF030818 };
        constexpr Color SeaGlint{ 0xFFCFE8FF };
        constexpr Color CityGlow{ 0xFFFFB84D };
        constexpr Color CityLight{ 0xFFFFD27A };
        constexpr Color Day{ 0xFF6CB8FF };
        constexpr Color Dusk{ 0xFFFF8A3D };
        constexpr Color DuskDeep{ 0xFFFF7040 };
        constexpr Color NightAir{ 0xFF1B2A5C };
    }

    namespace
    {
        constexpr float k_degree{ std::numbers::pi_v<float> / 180.0f };
        // The latitude the view looks straight down on, so the cap in view holds mid-latitudes.
        constexpr float k_viewLatitude{ -20.0f * k_degree };
        // The pace things keep at a 40-second cycle, in this scene's 240-second one.
        constexpr float k_lively{ 6.0f };
        // How many opacities the city lights are drawn in, one fill each on Direct2D.
        constexpr int k_cityOpacitySteps{ 6 };
        // How far apart the points are where a shape's edge follows the rim.
        constexpr float k_rimStep{ 2.0f * k_degree };
        // How far a cloud's edge fades out, in scene units.
        constexpr float k_cloudSoftness{ 20.0f };
        // The points round a lump of cloud, enough for the finest wave along its edge.
        constexpr int k_cloudPoints{ 32 };

        // A city's light where it shows this frame, and the opacity step it is drawn at.
        struct LitCity
        {
            FloatPoint at;
            float radius;
            int step;
        };

        // Coarse coastlines as longitude, latitude pairs.
        constexpr float k_northAmerica[]{
            -168, 66, -160, 70, -140, 70, -125, 70, -110, 73, -95, 72, -85, 70, -80, 64, -90, 58,
            -80, 52, -65, 60, -60, 55, -56, 52, -66, 45, -70, 42, -75, 38, -76, 35, -81, 31,
            -80, 26, -82, 25, -84, 30, -90, 29, -97, 27, -97, 22, -92, 18, -88, 15, -84, 11, -79, 9,
            -83, 8, -86, 12, -92, 15, -96, 16, -105, 20, -106, 23, -110, 24, -112, 29, -117, 32,
            -121, 35, -124, 40, -124, 46, -125, 50, -130, 55, -135, 58, -142, 60, -150, 60,
            -155, 58, -160, 57, -165, 60
        };
        constexpr float k_southAmerica[]{
            -79, 9, -77, 8, -72, 12, -62, 10, -52, 5, -50, 0, -44, -2, -35, -6, -35, -9, -39, -15,
            -41, -22, -48, -26, -53, -34, -58, -38, -62, -40, -65, -45, -68, -50, -70, -54,
            -74, -52, -74, -45, -73, -37, -71, -30, -70, -18, -76, -14, -80, -6, -81, -2, -80, 2,
            -78, 6
        };
        constexpr float k_africa[]{
            -17, 21, -16, 28, -10, 32, -6, 36, 0, 36, 10, 37, 11, 33, 20, 32, 25, 32, 32, 31,
            35, 28, 39, 20, 43, 12, 51, 12, 47, 6, 41, -2, 40, -10, 40, -16, 35, -24, 32, -29,
            27, -34, 20, -35, 18, -31, 14, -23, 12, -17, 13, -10, 9, -1, 9, 4, 5, 5, -4, 5, -8, 4,
            -13, 8, -17, 14
        };
        constexpr float k_eurasia[]{
            -10, 36, -9, 43, -2, 44, -5, 48, 0, 49, 5, 53, 9, 54, 10, 57, 5, 58, 5, 62, 12, 66,
            18, 70, 25, 71, 30, 70, 40, 68, 45, 68, 55, 70, 68, 72, 80, 73, 100, 77, 115, 74,
            130, 72, 140, 72, 160, 70, 170, 69, 180, 66, 178, 62, 165, 60, 160, 55, 156, 51,
            150, 59, 140, 55, 138, 47, 133, 43, 129, 40, 127, 35, 121, 31, 122, 25, 115, 22,
            108, 21, 106, 17, 109, 12, 105, 9, 100, 13, 99, 8, 103, 2, 100, 4, 98, 10, 93, 17,
            90, 22, 85, 20, 80, 15, 77, 8, 73, 17, 70, 22, 65, 25, 57, 25, 56, 27, 51, 25, 50, 30,
            48, 29, 43, 13, 39, 20, 35, 29, 34, 32, 36, 36, 30, 36, 27, 37, 26, 40, 23, 40, 20, 40,
            19, 42, 15, 45, 13, 45, 16, 41, 18, 40, 16, 38, 12, 44, 8, 44, 3, 43, -1, 37, -6, 36
        };
        constexpr float k_australia[]{
            114, -22, 114, -34, 118, -35, 124, -34, 131, -31, 137, -35, 140, -38, 147, -39,
            150, -37, 153, -31, 153, -25, 146, -19, 142, -11, 141, -17, 136, -12, 130, -12,
            126, -14, 122, -18
        };
        constexpr float k_greenland[]{
            -73, 78, -60, 82, -35, 83, -20, 81, -18, 75, -22, 70, -40, 64, -44, 60, -50, 62,
            -55, 68, -58, 74, -68, 77
        };
        constexpr float k_sumatra[]{ 95, 5, 98, 4, 104, -2, 106, -6, 102, -4, 98, 0 };
        constexpr float k_borneo[]{ 109, 1, 112, 3, 117, 7, 119, 5, 118, 0, 116, -4, 110, -3 };
        constexpr float k_newGuinea[]{ 131, -1, 138, -2, 147, -6, 150, -10, 141, -9, 137, -5 };
        constexpr float k_japan[]{
            130, 31, 135, 34, 140, 36, 142, 40, 145, 44, 141, 45, 140, 41, 137, 37, 132, 34
        };
        constexpr float k_britain[]{ -5, 50, 1, 51, 1, 53, -3, 56, -2, 58, -6, 58, -5, 55, -3, 53 };
        constexpr float k_madagascar[]{ 44, -25, 47, -25, 50, -15, 49, -12, 44, -16 };

        struct CoastlineSource
        {
            std::span<const float> lonLat;
            Color color;
            bool inhabited; // whether its towns light up at night
        };

        constexpr Color k_green{ 0xFF5C8F45 };
        constexpr Color k_deepGreen{ 0xFF4F8A3A };
        constexpr std::array<CoastlineSource, 12> k_coastlineSources{
            CoastlineSource{ k_northAmerica, k_green, true },
            CoastlineSource{ k_southAmerica, k_deepGreen, true },
            CoastlineSource{ k_africa, Color{ 0xFFA8935A }, true },
            CoastlineSource{ k_eurasia, Color{ 0xFF6A9446 }, true },
            CoastlineSource{ k_australia, Color{ 0xFFB29A5C }, true },
            CoastlineSource{ k_greenland, Color{ 0xFFE6EEF5 }, false },
            CoastlineSource{ k_sumatra, k_deepGreen, true },
            CoastlineSource{ k_borneo, k_deepGreen, true },
            CoastlineSource{ k_newGuinea, k_deepGreen, true },
            CoastlineSource{ k_japan, k_green, true },
            CoastlineSource{ k_britain, k_green, true },
            CoastlineSource{ k_madagascar, k_green, true }
        };

        [[nodiscard]] bool isInside(std::span<const FloatPoint> polygon, const FloatPoint point)
        {
            bool inside = false;
            std::size_t previous = polygon.size() - 1;
            for (std::size_t i = 0; i != polygon.size(); ++i)
            {
                const FloatPoint a = polygon[i];
                const FloatPoint b = polygon[previous];
                const bool straddles = (a.y > point.y) != (b.y > point.y);
                if (straddles && point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x)
                    inside = !inside;
                previous = i;
            }
            return inside;
        }

        // Where the globe stands in a frame.
        struct GlobeDisc
        {
            FloatPoint center;
            float radius;
        };

        // The globe so large that only a cap shows, its top a little above the middle.
        [[nodiscard]] GlobeDisc globeDiscIn(const SceneFrame& frame)
        {
            const float radius = 1.15f * std::max(frame.width, frame.height);
            return { { frame.width * 0.4f, frame.height * 0.44f + radius }, radius };
        }

        // The atmosphere's colour at a point of the limb, from the sunlight there.
        [[nodiscard]] Color atmosphereColor(const float sunlight)
        {
            if (sunlight > 0.18f)
                return SatelliteSceneColors::Day.withOpacity(0.9f);
            if (sunlight > 0.0f)
            {
                const Color tint = colorBetween(SatelliteSceneColors::Dusk,
                    SatelliteSceneColors::Day, sunlight / 0.18f);
                return tint.withOpacity(0.92f);
            }
            if (sunlight > -0.16f)
            {
                const float share = 1.0f + sunlight / 0.16f;
                const Color tint = colorBetween(SatelliteSceneColors::NightAir,
                    SatelliteSceneColors::DuskDeep, share);
                return tint.withOpacity(std::lerp(0.3f, 0.92f, share));
            }
            return SatelliteSceneColors::NightAir.withOpacity(0.3f);
        }
    }

    // The coastlines get a point every 4 degrees so a long edge bends with the globe; the towns are
    // scattered inside them, a fixed set for every frame.
    SatelliteScene::SatelliteScene()
    {
        int seed = 0;
        for (const CoastlineSource& source : k_coastlineSources)
        {
            std::vector<FloatPoint> corners;
            for (std::size_t i = 0; i + 1 < source.lonLat.size(); i += 2)
                corners.push_back({ source.lonLat[i], source.lonLat[i + 1] });

            Coastline coastline{ .color = source.color, .lonLat = {}, .counterClockwise = false };
            // The shoelace sum, above 0 when the corners run counterclockwise.
            float twiceArea = 0.0f;
            for (std::size_t i = 0; i != corners.size(); ++i)
            {
                const FloatPoint from = corners[i];
                const FloatPoint to = corners[(i + 1) % corners.size()];
                twiceArea += from.x * to.y - to.x * from.y;
                const float span = std::hypot(to.x - from.x, to.y - from.y);
                const int steps = std::max(1, static_cast<int>(std::ceil(span / 4.0f)));
                for (int step = 0; step != steps; ++step)
                {
                    const float share = static_cast<float>(step) / static_cast<float>(steps);
                    coastline.lonLat.push_back(pointBetween(from, to, share));
                }
            }
            coastline.counterClockwise = twiceArea > 0.0f;
            m_coastlines.push_back(std::move(coastline));

            if (!source.inhabited)
                continue;
            FloatPoint low = corners.front();
            FloatPoint high = corners.front();
            for (const FloatPoint corner : corners)
            {
                low = { std::min(low.x, corner.x), std::min(low.y, corner.y) };
                high = { std::max(high.x, corner.x), std::max(high.y, corner.y) };
            }
            // About one town to every 26 square degrees of the land's bounding box.
            const float area = (high.x - low.x) * (high.y - low.y);
            const int wanted = static_cast<int>(std::round(area / 26.0f));
            int placed = 0;
            for (int attempt = 0; attempt < wanted * 6 && placed < wanted; ++attempt)
            {
                ++seed;
                const FloatPoint town = {
                    std::lerp(low.x, high.x, pseudoRandom(seed, 71.0f)),
                    std::lerp(low.y, high.y, pseudoRandom(seed, 72.0f))
                };
                if (town.y > 62.0f || !isInside(corners, town))
                    continue;
                m_cities.push_back({ town, pseudoRandom(seed, 73.0f) });
                ++placed;
            }
        }
        // A cluster is a lump, and some carry one or two smaller lumps overlapping it.
        for (int i = 0; i != 90; ++i)
        {
            const FloatPoint lonLat = {
                pseudoRandom(i, 81.0f) * 360.0f - 180.0f,
                pseudoRandom(i, 82.0f) * 120.0f - 50.0f
            };
            const float radius = 1.2f + 2.6f * pseudoRandom(i, 83.0f);
            const float stretch = 1.2f + 1.2f * pseudoRandom(i, 84.0f);
            const FloatPoint halfSize = { 0.8f * radius * (1.0f + 0.7f * stretch), 0.6f * radius };
            const auto lumpAt = [&](int seed, FloatPoint at, FloatPoint size) {
                return Cloud{
                    .lonLat = at,
                    .halfSize = size,
                    .tilt = (pseudoRandom(seed, 95.0f) - 0.5f) * 0.5f,
                    .phases = {
                        k_2Pi * pseudoRandom(seed, 91.0f),
                        k_2Pi * pseudoRandom(seed, 92.0f),
                        k_2Pi * pseudoRandom(seed, 93.0f),
                        k_2Pi * pseudoRandom(seed, 94.0f)
                    },
                    .driftPhase = static_cast<float>(i)
                };
            };
            m_clouds.push_back(lumpAt(i * 3, lonLat, halfSize));
            const float extra = pseudoRandom(i, 85.0f);
            const int companions = extra < 0.15f ? 2 : (extra < 0.45f ? 1 : 0);
            for (int companion = 1; companion <= companions; ++companion)
            {
                const int lumpSeed = i * 3 + companion;
                const float direction = k_2Pi * pseudoRandom(lumpSeed, 86.0f);
                const float distance = 0.35f + 0.35f * pseudoRandom(lumpSeed, 87.0f);
                const FloatPoint offset = {
                    std::cos(direction) * halfSize.x,
                    std::sin(direction) * halfSize.y
                };
                const float scale = 0.45f + 0.35f * pseudoRandom(lumpSeed, 88.0f);
                m_clouds.push_back(lumpAt(lumpSeed, lonLat + offset * distance, halfSize * scale));
            }
        }
    }

    // The Earth turns twice a cycle and the Sun goes round once, so each continent passes once by
    // day and once by night. See PathArt#satellitescene
    void SatelliteScene::prepare(const SceneParams& params)
    {
        m_space.prepare(params);
        m_ocean.prepare(params);
    }

    void SatelliteScene::draw(Canvas& canvas, const FloatRect& bounds, const float cycleTime)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const GlobeDisc globeDisc = globeDiscIn(frame);
        const float radius = globeDisc.radius;
        const float sunTurn = k_2Pi * cycleTime + 0.9f;
        const float sunLength = std::hypot(std::sin(sunTurn), 0.22f, std::cos(sunTurn));
        const Globe globe = {
            .center = globeDisc.center,
            .radius = radius,
            .tilt = 0.22f,
            .spin = k_2Pi * 2.0f * cycleTime + 1.2f,
            .sun = {
                std::sin(sunTurn) / sunLength,
                0.22f / sunLength,
                std::cos(sunTurn) / sunLength
            }
        };

        // The satellite is in the Earth's shadow when the ground under it is past the terminator.
        const float hover = 5.0f * unit * std::sin(k_2Pi * 2.0f * k_lively * cycleTime);
        const FloatPoint satellite = { width * 0.66f, height * 0.27f + hover };
        const FloatPoint offCenter = {
            (satellite.x - globe.center.x) / radius,
            (globe.center.y - satellite.y) / radius
        };
        const FloatPoint fromCenter = rotated(offCenter, -globe.tilt);
        const float underLength = std::hypot(fromCenter.x, fromCenter.y, 0.35f);
        const Direction under = {
            fromCenter.x / underLength,
            fromCenter.y / underLength,
            0.35f / underLength
        };
        const float sunlit = smoothStep(-0.14f, 0.06f, globe.sunlightAt(under));

        m_space.draw(frame);
        // The stars dim a little by day and shine fully in the Earth's shadow.
        drawStars(frame, cycleTime, StarField{
            .count = 160,
            .top = 0.0f,
            .bottom = height * 0.8f,
            .seed = 90.0f,
            .color = SatelliteSceneColors::Star,
            .opacity = 0.65f + 0.35f * (1.0f - sunlit),
            .twinklesPerCycle = k_lively
        });
        drawGlow(frame, { width, 0.0f }, 340.0f * unit,
            SatelliteSceneColors::SunGlare.withOpacity(0.16f * sunlit));
        m_ocean.draw(frame);

        PixelPath disc;
        disc.drawCircle(globe.center, radius);
        frame.canvas.pushClip(disc, &frame.toCanvas);
        drawSurface(frame, globe, cycleTime);
        const Terminator terminator = terminatorOf(frame, globe);
        drawNight(frame, globe, terminator);
        if (globe.sun.z > 0.0f)
        {
            drawGlow(frame, globe.onScreen(globe.sun), 150.0f * unit,
                SatelliteSceneColors::SeaGlint.withOpacity(0.3f * globe.sun.z), 0.8f);
        }
        drawCityLights(frame, globe, terminator, cycleTime);
        frame.canvas.popClip();
        drawAtmosphere(frame, globe);

        const float size = unit * std::min(1.0f, (width / unit) / 900.0f);
        drawSatellite(frame, SatellitePose{
            .center = satellite,
            .size = size,
            .sunlit = sunlit,
            .orbitTime = cycleTime,
            .blinkTime = cycleTime * k_lively
        });
    }

    SatelliteScene::Direction SatelliteScene::Globe::surfaceAt(const FloatPoint lonLat) const
    {
        const float latitude = lonLat.y * k_degree;
        const float longitude = lonLat.x * k_degree + spin;
        const float x = std::cos(latitude) * std::sin(longitude);
        const float y = std::sin(latitude);
        const float z = std::cos(latitude) * std::cos(longitude);
        const float tipCosine = std::cos(-k_viewLatitude);
        const float tipSine = std::sin(-k_viewLatitude);
        return { x, y * tipCosine + z * tipSine, -y * tipSine + z * tipCosine };
    }

    FloatPoint SatelliteScene::Globe::onScreen(const Direction direction) const
    {
        const float tiltCosine = std::cos(tilt);
        const float tiltSine = std::sin(tilt);
        return {
            center.x + radius * (direction.x * tiltCosine - direction.y * tiltSine),
            center.y - radius * (direction.x * tiltSine + direction.y * tiltCosine)
        };
    }

    FloatPoint SatelliteScene::Globe::onRim(const float angle) const
    {
        return onScreen({ std::cos(angle), std::sin(angle), 0.0f });
    }

    // The outline is cut where it goes behind the Earth into stretches on the near side. Each
    // stretch is joined along the rim to the next one that comes round, the way the outline turns,
    // so the shape keeps its inside on the same hand along the rim as along its own edge.
    void SatelliteScene::Globe::nearSideOf(const std::span<const Direction> outline,
        const bool counterClockwise, std::vector<FloatPoint>& points,
        std::vector<std::size_t>& loopEnds) const
    {
        points.clear();
        loopEnds.clear();
        const std::size_t count = outline.size();
        const auto isNear = [](const Direction& direction) {
            return direction.z > 0.0f;
        };
        // The outline is followed from a point behind the Earth whose next point is in view.
        std::size_t start = count;
        for (std::size_t i = 0; i != count && start == count; ++i)
        {
            if (!isNear(outline[i]) && isNear(outline[(i + 1) % count]))
                start = i;
        }
        if (start == count)
        {
            // All of the outline is in view, or none of it.
            if (count == 0 || !isNear(outline.front()))
                return;
            for (const Direction& direction : outline)
                points.push_back(onScreen(direction));
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
        for (std::size_t i = 0; i != count; ++i)
        {
            const Direction& from = outline[(start + i) % count];
            const Direction& to = outline[(start + i + 1) % count];
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
                walked.push_back(onRim(angle));
            }
            if (toNear)
                walked.push_back(onScreen(to));
            if (fromNear && !toNear)
            {
                const float angle = crossingAngle(from, to);
                walked.push_back(onRim(angle));
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
                    points.push_back(onRim(stretch.leaveAngle + sweep * share));
                }
                current = next;
            }
            loopEnds.push_back(points.size());
        }
    }

    float SatelliteScene::Globe::sunlightAt(const Direction direction) const
    {
        return direction.x * sun.x + direction.y * sun.y + direction.z * sun.z;
    }

    float SatelliteScene::Terminator::distanceTo(const FloatPoint point) const
    {
        return (point.x - through.x) * dayward.x + (point.y - through.y) * dayward.y;
    }

    // Over the small cap in view the terminator is near enough to a straight line, so it is one:
    // the line through the terminator's points on two rows of the frame.
    // Below the lowest point of the limb in the frame the ocean covers everything, so space
    // stops a little past it.
    void SatelliteScene::drawSpace(const SceneFrame& frame)
    {
        const GlobeDisc disc = globeDiscIn(frame);
        const float farthest = std::max(disc.center.x, frame.width - disc.center.x);
        const float limbLowest = disc.center.y
            - std::sqrt(disc.radius * disc.radius - farthest * farthest);
        const float bottom = std::min(frame.height, limbLowest + 2.0f * frame.unit);
        fillBox(frame, { 0.0f, 0.0f, frame.width, bottom },
            LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, frame.height },
                SatelliteSceneColors::SpaceTop, SatelliteSceneColors::SpaceBottom));
    }

    void SatelliteScene::drawOcean(const SceneFrame& frame)
    {
        const GlobeDisc disc = globeDiscIn(frame);
        PixelPath outline;
        outline.drawCircle(disc.center, disc.radius);
        frame.canvas.pushClip(outline, &frame.toCanvas);
        const float top = disc.center.y - disc.radius;
        fillBox(frame, { 0.0f, 0.0f, frame.width, frame.height },
            LinearGradient::Simple({ 0.0f, top }, { 0.0f, top + disc.radius * 0.45f },
            SatelliteSceneColors::OceanLimb, SatelliteSceneColors::OceanDeep));
        frame.canvas.popClip();
    }

    SatelliteScene::Terminator SatelliteScene::terminatorOf(const SceneFrame& frame,
        const Globe& globe)
    {
        // Turned so the Sun is on +x, the terminator is a half ellipse sun.z wide.
        const float sunTurn = std::atan2(-globe.sun.y, globe.sun.x);
        const auto toTurned = [&](FloatPoint point) {
            return rotated((point - globe.center) * (1.0f / globe.radius), globe.tilt - sunTurn);
        };
        const auto fromTurned = [&](FloatPoint point) {
            return globe.center + rotated(point, sunTurn - globe.tilt) * globe.radius;
        };
        const auto terminatorAt = [&](float row) {
            return FloatPoint{ -globe.sun.z * std::sqrt(std::max(0.0f, 1.0f - row * row)), row };
        };
        const float middleX = frame.width * 0.5f;
        const float upperRow = toTurned({ middleX, frame.height * 0.5f }).y;
        const float lowerRow = toTurned({ middleX, frame.height * 0.9f }).y;
        const FloatPoint upper = fromTurned(terminatorAt(upperRow));
        const FloatPoint lower = fromTurned(terminatorAt(lowerRow));
        FloatPoint along = lower - upper;
        const float length = std::hypot(along.x, along.y);
        along = length > 0.0f ? along * (1.0f / length) : FloatPoint{ 0.0f, 1.0f };
        const FloatPoint sunward = rotated({ 1.0f, 0.0f }, sunTurn - globe.tilt);
        FloatPoint dayward = { -along.y, along.x };
        if (dayward.x * sunward.x + dayward.y * sunward.y < 0.0f)
            dayward = -dayward;
        return { .through = upper, .along = along, .dayward = dayward };
    }

    void SatelliteScene::drawSurface(const SceneFrame& frame, const Globe& globe,
        const float cycleTime) const
    {
        const FloatRect view = { 0.0f, 0.0f, frame.width, frame.height };

        // A shape is drawn when any of it is on the near side and its outline reaches the frame.
        std::vector<Direction> outline;
        std::vector<FloatPoint> points;
        std::vector<std::size_t> loopEnds;
        const auto nearSideShape = [&](bool counterClockwise) -> std::optional<PixelPath> {
            globe.nearSideOf(outline, counterClockwise, points, loopEnds);
            if (points.empty())
                return std::nullopt;
            constexpr float k_far{ std::numeric_limits<float>::max() };
            FloatRect reach = { k_far, k_far, -k_far, -k_far };
            for (const FloatPoint point : points)
            {
                reach = {
                    std::min(reach.left, point.x),
                    std::min(reach.top, point.y),
                    std::max(reach.right, point.x),
                    std::max(reach.bottom, point.y)
                };
            }
            const bool outside = reach.right < view.left || reach.left > view.right
                || reach.bottom < view.top || reach.top > view.bottom;
            if (outside)
                return std::nullopt;
            PixelPath shape;
            std::size_t loopStart = 0;
            for (const std::size_t loopEnd : loopEnds)
            {
                shape.moveTo(points[loopStart]);
                for (std::size_t i = loopStart + 1; i != loopEnd; ++i)
                    shape.lineTo(points[i]);
                shape.close();
                loopStart = loopEnd;
            }
            return shape;
        };

        for (const Coastline& coastline : m_coastlines)
        {
            outline.clear();
            for (const FloatPoint lonLat : coastline.lonLat)
                outline.push_back(globe.surfaceAt(lonLat));
            if (const std::optional<PixelPath> shape = nearSideShape(coastline.counterClockwise))
                fillShape(frame, *shape, SolidColor{ coastline.color });
        }

        // **Each lump is filled on its own with a soft edge**, so lumps that overlap show brighter
        // where they meet rather than cut holes in each other. The edge is four waves round an
        // ellipse, set apart for each lump by where each wave starts.
        const PathDrawLayer soft = PathDrawLayer::Shadow(SatelliteSceneColors::Cloud,
            k_cloudSoftness * frame.unit);
        for (const Cloud& cloud : m_clouds)
        {
            const float drift = 6.0f * std::sin(k_2Pi * cycleTime + cloud.driftPhase);
            const FloatPoint center = cloud.lonLat + FloatPoint{ drift, 0.0f };
            outline.clear();
            for (int j = 0; j != k_cloudPoints; ++j)
            {
                const float angle = k_2Pi * static_cast<float>(j) / k_cloudPoints;
                const std::array<float, 4>& phases = cloud.phases;
                const float lump = 1.0f + 0.16f * std::sin(2.0f * angle + phases[0])
                    + 0.11f * std::sin(3.0f * angle + phases[1])
                    + 0.08f * std::sin(5.0f * angle + phases[2])
                    + 0.06f * std::sin(9.0f * angle + phases[3]);
                const FloatPoint around = rotated({
                    std::cos(angle) * cloud.halfSize.x,
                    std::sin(angle) * cloud.halfSize.y
                }, cloud.tilt);
                outline.push_back(globe.surfaceAt(center + around * lump));
            }
            // The lump's points run counterclockwise.
            if (const std::optional<PixelPath> shape = nearSideShape(true))
                frame.canvas.drawPath(*shape, std::span{ &soft, 1 }, &frame.toCanvas);
        }
    }

    // One gradient over the night side, rising from nothing just on the day side of the line.
    // Before its start it holds its first stop, which is clear, so it covers the whole frame and
    // leaves the day side as it is.
    void SatelliteScene::drawNight(const SceneFrame& frame, const Globe& globe,
        const Terminator& terminator)
    {
        const float soft = 0.03f * globe.radius;
        const float depth = 0.13f * globe.radius;
        const FloatPoint edge = terminator.through + terminator.dayward * soft;
        fillBox(frame, { 0.0f, 0.0f, frame.width, frame.height }, LinearGradient{
            .startPoint = edge,
            .endPoint = terminator.through - terminator.dayward * depth,
            .stops = {
                { .position = 0.0f, .color = SatelliteSceneColors::Night.withOpacity(0.0f) },
                { .position = 0.35f, .color = SatelliteSceneColors::NightBlue.withOpacity(0.55f) },
                { .position = 1.0f, .color = SatelliteSceneColors::Night.withOpacity(0.9f) }
            }
        });
    }

    // The lights come on a little way past the terminator and fade toward the edge of the globe.
    void SatelliteScene::drawCityLights(const SceneFrame& frame, const Globe& globe,
        const Terminator& terminator, const float cycleTime) const
    {
        const float unit = frame.unit;
        // The glows go down city by city, and the lights over all of them afterwards.
        std::vector<LitCity> lit;
        for (const City& city : m_cities)
        {
            const Direction direction = globe.surfaceAt(city.lonLat);
            if (direction.z < 0.02f)
                continue;
            const FloatPoint point = globe.onScreen(direction);
            const float margin = 10.0f * unit;
            const bool outside = point.x < -margin || point.x > frame.width + margin
                || point.y > frame.height + margin;
            if (outside)
                continue;
            const float pastLine = smoothStep(-0.01f * globe.radius, -0.07f * globe.radius,
                terminator.distanceTo(point));
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
            if (city.brightness > 0.82f)
                drawGlow(frame, point, 7.0f * unit,
                    SatelliteSceneColors::CityGlow.withOpacity(0.45f * strength));
        }
        for (int step = 0; step != k_cityOpacitySteps; ++step)
        {
            const float opacity = (static_cast<float>(step) + 0.5f) / k_cityOpacitySteps;
            const Color light = SatelliteSceneColors::CityLight.withOpacity(opacity);
            ScatteredShapes lights{ frame, SolidColor{ light } };
            for (const LitCity& city : lit)
            {
                if (city.step == step)
                    addEllipse(lights.next(), city.at, city.radius, city.radius);
            }
            lights.draw();
        }
    }

    // The limb in view runs across the frame, so one gradient across the frame colours it by the
    // sunlight above each x - blue by day, orange at dusk, dark at night.
    void SatelliteScene::drawAtmosphere(const SceneFrame& frame, const Globe& globe)
    {
        constexpr int k_stops = 15;
        const float from = -0.05f * frame.width;
        const float to = 1.05f * frame.width;
        std::array<Color, k_stops> colors{};
        for (int i = 0; i != k_stops; ++i)
        {
            const float x = std::lerp(from, to, static_cast<float>(i) / (k_stops - 1));
            const float across = std::clamp((x - globe.center.x) / globe.radius, -1.0f, 1.0f);
            const float limbAngle = -globe.tilt + std::acos(across);
            const float sunlight = std::cos(limbAngle) * globe.sun.x
                + std::sin(limbAngle) * globe.sun.y;
            colors[static_cast<std::size_t>(i)] = atmosphereColor(sunlight);
        }
        const auto layer = [&](float ringRadius, float width, float strength) {
            LinearGradient gradient{
                .startPoint = { from, 0.0f },
                .endPoint = { to, 0.0f },
                .stops = {}
            };
            for (int i = 0; i != k_stops; ++i)
            {
                const Color color = colors[static_cast<std::size_t>(i)];
                gradient.stops.push_back({
                    .position = static_cast<float>(i) / (k_stops - 1),
                    .color = color.withOpacity(static_cast<float>(color.alpha) / 255.0f * strength)
                });
            }
            PixelPath ring;
            addEllipse(ring, globe.center, ringRadius, ringRadius);
            strokeShape(frame, ring, gradient, width, StrokeCap::Butt);
        };
        layer(globe.radius * 1.012f, globe.radius * 0.03f, 0.22f);
        layer(globe.radius * 1.003f, std::max(globe.radius * 0.006f, 3.0f * frame.unit), 1.0f);
        layer(globe.radius * 0.988f, globe.radius * 0.024f, 0.25f);
    }

}
