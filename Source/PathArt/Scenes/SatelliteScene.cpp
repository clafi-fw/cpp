module;
#include <immintrin.h>
module ClaFi.PathArt.SatelliteScene;

import ClaFi.PathArt.CloudMap;
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
        constexpr Color Cloud{ 0xCAFFFFFF }; // where a layer covers the sky whole
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
        // How far a point of cloud faces the viewer when it starts fading into the haze.
        constexpr float k_cloudHazeDepth{ 0.12f };
        // The rows of the clouds' picture worked out together on one core, and laid in as one.
        constexpr int k_cloudBandRows{ 32 };
        // The shades of the clouds kept along the night's gradient.
        constexpr int k_cloudShadeCount{ 1024 };
        // The globe's radius in pixels from which the clouds are worked out at every 4th pixel.
        constexpr float k_cloudCoarseRadius{ 3200.0f };
        // The cover the stylized look's lighter tone starts from, showing nothing below it.
        constexpr float k_stylizedLeastCover{ 0.28f };

        // A layer of cloud: what its map holds, and how it floats. See PathArt#satellitescene
        struct CloudLayer
        {
            CloudKind kind;
            std::uint32_t seed;
            float cover; // the share of the sphere its map covers
            float height; // above the ground, as a share of the Earth's radius
            float axisTilt; // how far its axis leans from the Earth's, in degrees
            float drift; // how far it drifts each way against the ground, in degrees
            float driftPhase; // where in its drift it stands when the cycle starts
            float opacity; // its share of the cloud colour's opacity
        };

        // Low heaps, a middle sheet and high streaks, each drifting further than the one below.
        constexpr std::array<CloudLayer, 3> k_cloudLayers{
            CloudLayer{
                .kind = CloudKind::Puffy,
                .seed = 11,
                .cover = 0.32f,
                .height = 0.006f,
                .axisTilt = 0.0f,
                .drift = 1.7f,
                .driftPhase = 0.0f,
                .opacity = 0.9f
            },
            CloudLayer{
                .kind = CloudKind::Patchy,
                .seed = 21,
                .cover = 0.45f,
                .height = 0.011f,
                .axisTilt = 5.0f,
                .drift = 2.3f,
                .driftPhase = 2.1f,
                .opacity = 0.55f
            },
            CloudLayer{
                .kind = CloudKind::Streaky,
                .seed = 31,
                .cover = 0.30f,
                .height = 0.018f,
                .axisTilt = -6.0f,
                .drift = 5.7f,
                .driftPhase = 4.2f,
                .opacity = 0.45f
            }
        };

        // A layer of cloud as this frame shows it, its sphere placed by the canvas's pixels.
        struct CloudLayerView
        {
            const CloudMap* map;
            FloatPoint fromPixel; // across and up from the sphere's center, in its radii, per pixel
            FloatPoint atOrigin; // the same at the canvas's pixel 0, 0
            float tiltCosine;
            float tiltSine;
            float tipCosine; // of how far its axis tips back from the screen's up
            float tipSine;
            float spin;
            float opacity; // where its map's cover is whole
        };

        // Where the points of a row fall on a layer's map.
        struct MapPoints
        {
            std::vector<float> longitudes;
            std::vector<float> latitudes;
        };

        // What a layer shows at the points of a row.
        struct LayerPoints
        {
            std::vector<float> covers;
            std::vector<float> clearness; // how clear of the haze, 0 off the sphere
        };

        // Each layer's points along one row, in the order of k_cloudLayers.
        using CloudPoints = std::array<LayerPoints, k_cloudLayers.size()>;

        // What every band of the clouds' picture is worked out from, this frame.
        struct CloudPicture
        {
            std::span<const CloudLayerView> layers;
            SceneLook look;
            int step; // the pixels between the points the layers are worked out at, 1 or 4
            FloatPoint sphereCenter; // the highest layer's, in pixels
            FloatPoint sphereRadii; // the highest layer's, across and up, in pixels
            int left; // the first column in view
            int right; // the column past the last in view
            std::span<const Color> shades; // the clouds' colour along the night's gradient
            FloatPoint nightPerPixel; // how far along the night's gradient a pixel moves
            float nightAtOrigin; // how far along it the canvas's pixel 0, 0 stands
        };

        // How 8 pixels in a row stand between its points at a step: the point before each, the
        // point after it, and the share of the way from the one to the other.
        struct PointSpread
        {
            __m256i before;
            __m256i after;
            __m256 shares;
        };

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

        // Where the points of a row fall on the layer's map, and how clear of the haze each is,
        // from the pixel center `firstPixelX` on, `step` pixels apart. See PathArt#satellitescene
        void projectRow(const CloudLayerView& layer, const float pixelY, const float firstPixelX,
            const int step, MapPoints& onMap, std::span<float> clearness)
        {
            const float up = layer.atOrigin.y + layer.fromPixel.y * pixelY;
            const __m256 upSquared = _mm256_set1_ps(up * up);
            const __m256 upTiltCosine = _mm256_set1_ps(up * layer.tiltCosine);
            const __m256 upTiltSine = _mm256_set1_ps(up * layer.tiltSine);
            const __m256 tiltCosine = _mm256_set1_ps(layer.tiltCosine);
            const __m256 tiltSine = _mm256_set1_ps(layer.tiltSine);
            const __m256 tipCosine = _mm256_set1_ps(layer.tipCosine);
            const __m256 tipSine = _mm256_set1_ps(layer.tipSine);
            const __m256 spin = _mm256_set1_ps(layer.spin);
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 acrossStart = _mm256_set1_ps(layer.atOrigin.x
                + layer.fromPixel.x * firstPixelX);
            const __m256 acrossPerPoint = _mm256_set1_ps(layer.fromPixel.x
                * static_cast<float>(step));
            for (std::size_t i = 0; i != clearness.size(); i += 8)
            {
                const __m256 point = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(i)),
                    laneOffsets());
                const __m256 across = _mm256_fmadd_ps(point, acrossPerPoint, acrossStart);
                // Off the sphere the depth is 0, which leaves the point wholly in the haze.
                const __m256 depthSquared = _mm256_sub_ps(one,
                    _mm256_fmadd_ps(across, across, upSquared));
                const __m256 z = _mm256_sqrt_ps(_mm256_max_ps(depthSquared, _mm256_setzero_ps()));
                const __m256 x = _mm256_fmadd_ps(across, tiltCosine, upTiltSine);
                const __m256 y = _mm256_fnmadd_ps(across, tiltSine, upTiltCosine);
                const __m256 untippedY = _mm256_fmsub_ps(y, tipCosine, _mm256_mul_ps(z, tipSine));
                const __m256 untippedZ = _mm256_fmadd_ps(y, tipSine, _mm256_mul_ps(z, tipCosine));
                const __m256 aroundAxis = _mm256_sqrt_ps(
                    _mm256_fmadd_ps(x, x, _mm256_mul_ps(untippedZ, untippedZ)));
                _mm256_storeu_ps(onMap.longitudes.data() + i,
                    _mm256_sub_ps(quickAngle(x, untippedZ), spin));
                _mm256_storeu_ps(onMap.latitudes.data() + i, quickAngle(untippedY, aroundAxis));
                _mm256_storeu_ps(clearness.data() + i, smoothStep8(0.0f, k_cloudHazeDepth, z));
            }
        }

        // How much of the sky a map's cover hides in a look: two flat tones when stylized.
        [[nodiscard]] __m256 coverShown(const __m256 cover, const SceneLook look)
        {
            if (look == SceneLook::Realistic)
                return cover;
            return _mm256_fmadd_ps(_mm256_set1_ps(0.55f),
                smoothStep8(k_stylizedLeastCover, 0.34f, cover),
                _mm256_mul_ps(_mm256_set1_ps(0.45f), smoothStep8(0.62f, 0.68f, cover)));
        }

        // The cover up to which a look shows nothing.
        [[nodiscard]] float leastCoverShown(const SceneLook look)
        {
            return look == SceneLook::Realistic ? 0.0f : k_stylizedLeastCover;
        }

        // The night's colour mixed into the clouds', at its own opacity, along its gradient.
        [[nodiscard]] std::vector<Color> cloudShadesUnder(const LinearGradient& night)
        {
            std::vector<Color> shades;
            shades.reserve(k_cloudShadeCount);
            const auto shaded = [](const GradientStop& stop) {
                const float nightShare = static_cast<float>(stop.color.alpha) / 255.0f;
                return colorBetween(SatelliteSceneColors::Cloud, stop.color, nightShare);
            };
            std::size_t next = 1;
            for (int i = 0; i != k_cloudShadeCount; ++i)
            {
                const float position = static_cast<float>(i) / (k_cloudShadeCount - 1);
                while (next + 1 < night.stops.size() && night.stops[next].position < position)
                    ++next;
                const GradientStop& from = night.stops[next - 1];
                const GradientStop& to = night.stops[next];
                const float share = std::clamp(
                    (position - from.position) / (to.position - from.position), 0.0f, 1.0f);
                shades.push_back(colorBetween(shaded(from), shaded(to), share));
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
                projectRow(layer, pixelY, firstPixelX, picture.step, onMap, points[i].clearness);
                layer.map->readCover(onMap.longitudes, onMap.latitudes, points[i].covers);
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

        // **A row of cloud pixels, premultiplied, in one pass**: each layer's cover and haze are
        // spread from the points the row stands between, the look cuts the cover, and the
        // night's shade is laid at the opacity the layers leave. A pixel's cover lies between
        // those of the points it is blended from, so 8 pixels whose points all fall short of what
        // a layer's look shows skip that layer, and 8 that no layer covers are written clear.
        // The last group of 8 stops at the row's end.
        void writeCloudRow(const CloudPicture& picture, const CloudPoints& upper,
            const CloudPoints& lower, const float downShare, const float rowNight, Color* pixels,
            const int width)
        {
            const int step = picture.step;
            const PointSpread spread = pointSpreadAt(step);
            const __m256 down = _mm256_set1_ps(downShare);
            const __m256 leastCover = _mm256_set1_ps(leastCoverShown(picture.look));
            std::array<__m256, k_cloudLayers.size()> opacities{};
            for (std::size_t i = 0; i != opacities.size(); ++i)
                opacities[i] = _mm256_set1_ps(picture.layers[i].opacity);
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 half = _mm256_set1_ps(0.5f);
            const __m256 lastShade = _mm256_set1_ps(static_cast<float>(picture.shades.size() - 1));
            const __m256 nightPerPixel = _mm256_set1_ps(picture.nightPerPixel.x);
            const __m256 nightStart = _mm256_set1_ps(rowNight);
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
                const auto store = [&](__m256i color) {
                    int* out = reinterpret_cast<int*>(pixels + x);
                    if (width - x >= 8)
                    {
                        _mm256_storeu_si256(reinterpret_cast<__m256i*>(out), color);
                        return;
                    }
                    const __m256i inRow = _mm256_cmpgt_epi32(_mm256_set1_epi32(width - x),
                        _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
                    _mm256_maskstore_epi32(out, inRow, color);
                };
                __m256 transparency = one;
                bool covered = false;
                for (std::size_t i = 0; i != upper.size(); ++i)
                {
                    const __m256 upperCovers = _mm256_loadu_ps(upper[i].covers.data() + point);
                    const __m256 lowerCovers = _mm256_loadu_ps(lower[i].covers.data() + point);
                    const __m256 showing = _mm256_cmp_ps(
                        _mm256_max_ps(upperCovers, lowerCovers), leastCover, _CMP_GT_OQ);
                    if (_mm256_movemask_ps(showing) == 0)
                        continue;
                    covered = true;
                    const __m256 cover = pixelsBetween(upperCovers, lowerCovers, down, spread);
                    const __m256 clearness = pixelsBetween(
                        _mm256_loadu_ps(upper[i].clearness.data() + point),
                        _mm256_loadu_ps(lower[i].clearness.data() + point), down, spread);
                    const __m256 hidden = _mm256_mul_ps(_mm256_mul_ps(opacities[i],
                        coverShown(cover, picture.look)), clearness);
                    transparency = _mm256_fnmadd_ps(hidden, transparency, transparency);
                }
                if (!covered)
                {
                    store(_mm256_setzero_si256());
                    continue;
                }

                const __m256 pixel = _mm256_add_ps(_mm256_set1_ps(static_cast<float>(x)),
                    laneOffsets());
                const __m256 alpha = _mm256_sub_ps(one, transparency);
                const __m256 night = _mm256_fmadd_ps(pixel, nightPerPixel, nightStart);
                const __m256 shadeIndex = _mm256_min_ps(_mm256_max_ps(
                    _mm256_fmadd_ps(night, lastShade, half), _mm256_setzero_ps()), lastShade);
                const __m256i shade = _mm256_i32gather_epi32(shades,
                    _mm256_cvttps_epi32(shadeIndex), 4);
                const __m256i alphaByte = _mm256_slli_epi32(
                    _mm256_cvttps_epi32(_mm256_fmadd_ps(alpha, byteMax, half)), 24);
                const __m256i color = _mm256_or_si256(
                    _mm256_or_si256(premultiplied(shade, 0, alpha), premultiplied(shade, 8, alpha)),
                    _mm256_or_si256(premultiplied(shade, 16, alpha), alphaByte));
                store(color);
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
            const auto nightAt = [&](float pixelY) {
                return picture.nightAtOrigin + picture.nightPerPixel.y * pixelY
                    + picture.nightPerPixel.x * firstPixelX;
            };
            // The last group of 8 pixels reads 8 points from the one under its first pixel.
            const std::size_t pointLength =
                groupedLength(static_cast<int>(groupedLength(width)) / step + 8);
            MapPoints onMap = {
                .longitudes = std::vector<float>(pointLength),
                .latitudes = std::vector<float>(pointLength)
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

            // At a step of 1 every row is a row of points, and the lower row is never read.
            CloudPoints upper = pointsOfRow();
            CloudPoints lower = step == 1 ? CloudPoints{} : pointsOfRow();
            samplePoints(picture, pixelYAt(0), firstPixelX, onMap, upper);
            if (step != 1)
                samplePoints(picture, pixelYAt(step), firstPixelX, onMap, lower);
            for (int y = 0; y != rows; ++y)
            {
                const int pastPoints = y % step;
                if (y != 0 && pastPoints == 0)
                {
                    if (step == 1)
                    {
                        samplePoints(picture, pixelYAt(y), firstPixelX, onMap, upper);
                    }
                    else
                    {
                        std::swap(upper, lower);
                        samplePoints(picture, pixelYAt(y + step), firstPixelX, onMap, lower);
                    }
                }
                const float downShare = static_cast<float>(pastPoints) / static_cast<float>(step);
                writeCloudRow(picture, upper, step == 1 ? upper : lower, downShare,
                    nightAt(pixelYAt(y)), pixels.pixel(0, y), width);
            }
            return { static_cast<float>(left), static_cast<float>(top) };
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
        drawSurface(frame, globe);
        const Terminator terminator = terminatorOf(frame, globe);
        const LinearGradient night = nightOf(globe, terminator);
        fillBox(frame, { 0.0f, 0.0f, width, height }, night);
        if (globe.sun.z > 0.0f)
        {
            drawGlow(frame, globe.onScreen(globe.sun), 150.0f * unit,
                SatelliteSceneColors::SeaGlint.withOpacity(0.3f * globe.sun.z), 0.8f);
        }
        drawCityLights(frame, globe, terminator, cycleTime);
        frame.canvas.popClip();
        drawClouds(frame, globe, night, cycleTime);
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

    std::optional<PixelPath> SatelliteScene::Globe::nearSidePath(
        const std::span<const Direction> outline, const bool counterClockwise,
        const FloatRect& view) const
    {
        std::vector<FloatPoint> points;
        std::vector<std::size_t> loopEnds;
        nearSideOf(outline, counterClockwise, points, loopEnds);
        if (points.empty())
            return std::nullopt;
        constexpr float k_far{ std::numeric_limits<float>::max() };
        FloatRect bounds = { k_far, k_far, -k_far, -k_far };
        for (const FloatPoint point : points)
        {
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
            return std::nullopt;
        PixelPath path;
        std::size_t loopStart = 0;
        for (const std::size_t loopEnd : loopEnds)
        {
            path.moveTo(points[loopStart]);
            for (std::size_t i = loopStart + 1; i != loopEnd; ++i)
                path.lineTo(points[i]);
            path.close();
            loopStart = loopEnd;
        }
        return path;
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

    void SatelliteScene::drawSurface(const SceneFrame& frame, const Globe& globe) const
    {
        const FloatRect view = { 0.0f, 0.0f, frame.width, frame.height };
        std::vector<Direction> outline;
        for (const Coastline& coastline : m_coastlines)
        {
            outline.clear();
            for (const FloatPoint lonLat : coastline.lonLat)
                outline.push_back(globe.surfaceAt(lonLat));
            const std::optional<PixelPath> path =
                globe.nearSidePath(outline, coastline.counterClockwise, view);
            if (path)
                fillShape(frame, *path, SolidColor{ coastline.color });
        }
    }

    // One gradient over the night side, rising from nothing just on the day side of the line.
    // Before its start it holds its first stop, which is clear, so it covers the whole frame and
    // leaves the day side as it is.
    LinearGradient SatelliteScene::nightOf(const Globe& globe, const Terminator& terminator)
    {
        const float soft = 0.03f * globe.radius;
        const float depth = 0.13f * globe.radius;
        return {
            .startPoint = terminator.through + terminator.dayward * soft,
            .endPoint = terminator.through - terminator.dayward * depth,
            .stops = {
                { .position = 0.0f, .color = SatelliteSceneColors::Night.withOpacity(0.0f) },
                { .position = 0.35f, .color = SatelliteSceneColors::NightBlue.withOpacity(0.55f) },
                { .position = 1.0f, .color = SatelliteSceneColors::Night.withOpacity(0.9f) }
            }
        };
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

    // **Three layers of cloud float above the ground**, each a map on a sphere of its own a little
    // larger than the Earth's, turning about an axis of its own. See PathArt#satellitescene
    void SatelliteScene::drawClouds(const SceneFrame& frame, const Globe& globe,
        const LinearGradient& night, const float cycleTime)
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

        const float cloudOpacity = static_cast<float>(SatelliteSceneColors::Cloud.alpha) / 255.0f;
        std::array<CloudLayerView, k_cloudLayers.size()> layers{};
        float outerRadius = 0.0f;
        for (std::size_t i = 0; i != k_cloudLayers.size(); ++i)
        {
            const CloudLayer& layer = k_cloudLayers[i];
            const float radius = globe.radius * (1.0f + layer.height);
            const float tip = -(k_viewLatitude + layer.axisTilt * k_degree);
            const float drift = layer.drift * k_degree
                * std::sin(k_2Pi * cycleTime + layer.driftPhase);
            layers[i] = {
                .map = &(*m_cloudMaps)[i],
                .fromPixel = { 1.0f / (pixelScale.x * radius), -1.0f / (pixelScale.y * radius) },
                .atOrigin = {
                    -(pixelShift.x / pixelScale.x + globe.center.x) / radius,
                    (globe.center.y + pixelShift.y / pixelScale.y) / radius
                },
                .tiltCosine = std::cos(globe.tilt),
                .tiltSine = std::sin(globe.tilt),
                .tipCosine = std::cos(tip),
                .tipSine = std::sin(tip),
                .spin = globe.spin + drift,
                .opacity = layer.opacity * cloudOpacity
            };
            outerRadius = std::max(outerRadius, radius);
        }

        const FloatPoint nightSpan = night.endPoint - night.startPoint;
        const float nightLengthSquared = nightSpan.x * nightSpan.x + nightSpan.y * nightSpan.y;
        const FloatPoint originInScene = {
            -pixelShift.x / pixelScale.x - night.startPoint.x,
            -pixelShift.y / pixelScale.y - night.startPoint.y
        };
        const std::vector<Color> shades = cloudShadesUnder(night);
        const CloudPicture picture = {
            .layers = layers,
            .look = look(),
            .step = globe.radius * pixelScale.x >= k_cloudCoarseRadius ? 4 : 1,
            .sphereCenter = toPixels.transform(globe.center),
            .sphereRadii = { outerRadius * pixelScale.x, outerRadius * pixelScale.y },
            .left = inView.left,
            .right = inView.right,
            .shades = shades,
            .nightPerPixel = {
                nightSpan.x / (pixelScale.x * nightLengthSquared),
                nightSpan.y / (pixelScale.y * nightLengthSquared)
            },
            .nightAtOrigin = (originInScene.x * nightSpan.x + originInScene.y * nightSpan.y)
                / nightLengthSquared
        };

        const float sphereTop = picture.sphereCenter.y - picture.sphereRadii.y;
        const int top = std::max(inView.top, static_cast<int>(std::floor(sphereTop)))
            / picture.step * picture.step;
        if (top >= inView.bottom)
            return;
        const int bandCount = (inView.bottom - top + k_cloudBandRows - 1) / k_cloudBandRows;
        m_cloudBands.resize(static_cast<std::size_t>(bandCount));
        spreadOverCores(bandCount, [&](int band) {
            const int bandTop = top + band * k_cloudBandRows;
            const int rows = std::min(k_cloudBandRows, inView.bottom - bandTop);
            CloudBand& drawn = m_cloudBands[static_cast<std::size_t>(band)];
            drawn.at = rasterizeCloudBand(picture, bandTop, rows, drawn.pixels);
        });

        // Direct2D places a view through the canvas's transform, and the bands are in pixels.
        std::vector<PixelView> views;
        views.reserve(m_cloudBands.size());
        const FloatRect canvasClip = frame.canvas.clipBox();
        for (const CloudBand& band : m_cloudBands)
        {
            if (!band.pixels.empty())
                views.push_back(band.pixels.pixelView(band.at, canvasClip));
        }
        const Matrix3x2 canvasTransform = frame.canvas.transform();
        frame.canvas.resetTransform();
        frame.canvas.drawPixelViews(views);
        frame.canvas.setTransform(canvasTransform);
    }

    // The limb in view runs across the frame, so one gradient across the frame colours it by the
    // sunlight above each x - blue by day, orange at dusk, dark at night. Each of the three rings
    // is a glow fading from its circle out to both sides, so none of them shows an edge.
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
        const auto drawRing = [&](float ringRadius, float reach, float strength) {
            LinearGradient gradient{
                .startPoint = { from, 0.0f },
                .endPoint = { to, 0.0f },
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

}
