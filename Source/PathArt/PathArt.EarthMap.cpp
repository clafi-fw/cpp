module;
#include <immintrin.h>
module ClaFi.PathArt.EarthMap;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    namespace
    {
        constexpr float k_pi{ std::numbers::pi_v<float> };
        constexpr float k_degree{ k_pi / 180.0f };
        // The detailed relief's texels round each circle of latitude.
        constexpr int k_detailedWidth{ 4096 };
        // The detailed relief's texels from the pole to the equator.
        constexpr int k_detailedHeight{ 1024 };
        // The heights a byte holds, 0 to this.
        constexpr float k_heightRange{ 1.2f };
        // The steepest slope a byte holds, in heights per radian, kept by its square root.
        constexpr float k_steepest{ 200.0f };
        // The land cover's texels round each circle of latitude, at its finest.
        constexpr int k_coverWidth{ 2048 };
        // The land cover's texels from the pole to the equator, at its finest.
        constexpr int k_coverHeight{ 512 };
        // The levels of detail in a pyramid, as many as its offsets hold.
        constexpr int k_levels{ 8 };
        // The finest level the broad relief is read at, between the third and the fourth.
        constexpr float k_broadLevel{ 2.75f };
        // How much finer each octave of the noise is than the one before.
        constexpr float k_lacunarity{ 2.03f };
        // How much wider a range's foothills reach than its core.
        constexpr float k_footReach{ 2.6f };
        // How far apart a coast's points stand once it is densified, in degrees of arc.
        constexpr float k_coastPointSpacing{ 0.1f };
        // How far the broad warp bends a coast at the noise's full swing, in degrees of arc.
        constexpr float k_coastWarpReach{ 0.35f };
        // How far the fine grain roughens a coast at the noise's full swing, in degrees of arc.
        constexpr float k_coastGrainReach{ 0.22f };

        // A range of mountains along a line. See PathArt#earthmap
        struct Range
        {
            std::span<const float> spine; // longitude, latitude pairs in degrees
            float height; // its crest, 1 at the Himalaya's
            float width; // how far its core reaches to either side, in degrees of arc
        };

        // High ground inside an outline, its edge rising over 1.3 degrees.
        struct Plateau
        {
            std::span<const float> outline; // longitude, latitude pairs in degrees
            float height;
        };

        // Where the land dries out, an ellipse on the map warped by noise.
        struct DryRegion
        {
            float longitude; // its center, in degrees
            float latitude;
            float radiusAcross; // in degrees of longitude
            float radiusUp; // in degrees of latitude
            float dryness; // at its heart
        };

        // Spines as longitude, latitude pairs, west to east or south to north.
        constexpr float k_himalaya[]{
            72.5f, 35.5f, 75.0f, 34.0f, 78.0f, 32.5f, 80.5f, 30.5f, 83.0f, 29.0f, 86.0f, 28.0f,
            88.5f, 27.8f, 91.0f, 27.9f, 93.5f, 28.3f, 95.5f, 29.0f
        };
        constexpr float k_karakoram[]{ 73.5f, 36.6f, 75.5f, 35.9f, 77.5f, 35.3f, 79.0f, 34.6f };
        constexpr float k_hinduKush[]{ 66.0f, 34.6f, 69.0f, 35.5f, 71.5f, 36.3f, 73.5f, 36.6f };
        constexpr float k_pamir[]{ 71.0f, 38.6f, 73.5f, 38.6f, 75.0f, 38.0f };
        constexpr float k_tianShan[]{
            69.0f, 41.5f, 72.0f, 41.5f, 75.0f, 41.8f, 78.0f, 42.0f, 81.0f, 42.8f, 84.0f, 43.3f,
            87.0f, 43.3f, 90.0f, 43.3f, 94.0f, 43.0f
        };
        constexpr float k_kunlun[]{
            75.0f, 36.5f, 79.0f, 36.0f, 83.0f, 36.0f, 87.0f, 36.2f, 91.0f, 36.0f, 95.0f, 35.6f,
            99.0f, 35.0f
        };
        constexpr float k_qilian[]{ 94.0f, 39.0f, 97.0f, 38.5f, 100.0f, 37.8f, 103.0f, 37.0f };
        constexpr float k_altai[]{
            83.0f, 50.5f, 86.5f, 50.0f, 89.0f, 49.0f, 92.0f, 47.5f, 95.0f, 46.3f, 99.0f, 45.0f
        };
        constexpr float k_sayan[]{ 89.0f, 53.0f, 94.0f, 52.5f, 99.0f, 52.0f, 102.0f, 51.5f };
        constexpr float k_hengduan[]{ 97.0f, 31.0f, 99.0f, 28.0f, 100.0f, 25.5f };
        constexpr float k_qinling[]{ 104.0f, 34.0f, 108.0f, 33.8f, 111.0f, 33.5f };
        constexpr float k_sulaiman[]{ 70.0f, 32.0f, 69.5f, 29.5f, 68.0f, 27.0f };
        constexpr float k_zagros[]{
            44.5f, 37.2f, 46.5f, 35.0f, 48.5f, 33.0f, 51.0f, 31.0f, 53.5f, 29.5f, 56.0f, 28.0f,
            58.0f, 27.0f
        };
        constexpr float k_alborz[]{
            48.5f, 37.5f, 51.0f, 36.2f, 53.5f, 36.5f, 56.0f, 37.3f, 59.0f, 37.5f
        };
        constexpr float k_caucasus[]{
            37.5f, 44.8f, 40.5f, 43.6f, 43.0f, 42.9f, 45.5f, 42.3f, 48.5f, 41.2f
        };
        constexpr float k_taurus[]{
            29.0f, 37.0f, 32.0f, 36.8f, 35.0f, 37.3f, 38.0f, 38.0f, 41.0f, 38.5f, 44.0f, 39.5f
        };
        constexpr float k_pontic[]{ 31.0f, 41.0f, 36.0f, 40.8f, 40.0f, 40.6f, 42.0f, 41.0f };
        constexpr float k_urals[]{
            58.5f, 51.0f, 59.0f, 54.0f, 59.0f, 57.0f, 59.5f, 60.0f, 59.8f, 62.5f, 60.5f, 65.0f,
            63.0f, 67.0f, 65.5f, 68.5f
        };
        constexpr float k_verkhoyansk[]{
            126.0f, 61.0f, 128.0f, 64.0f, 130.0f, 66.5f, 133.0f, 68.0f, 136.0f, 68.5f
        };
        constexpr float k_chersky[]{
            138.0f, 62.0f, 142.0f, 64.5f, 146.0f, 66.0f, 150.0f, 67.5f, 154.0f, 68.0f
        };
        constexpr float k_stanovoy[]{ 120.0f, 56.5f, 126.0f, 56.0f, 132.0f, 56.0f, 135.0f, 57.0f };
        constexpr float k_kamchatka[]{ 156.5f, 51.5f, 158.5f, 54.0f, 160.0f, 56.5f, 161.0f, 58.5f };
        constexpr float k_japaneseAlps[]{
            136.0f, 35.5f, 137.5f, 36.3f, 138.5f, 36.6f, 140.0f, 38.5f, 140.8f, 40.5f
        };
        constexpr float k_hejaz[]{
            35.5f, 28.0f, 38.0f, 24.0f, 40.0f, 21.0f, 42.0f, 18.0f, 43.5f, 15.0f
        };
        constexpr float k_hajar[]{ 56.0f, 24.5f, 57.5f, 23.3f, 59.0f, 22.5f };
        constexpr float k_alps[]{
            5.8f, 44.0f, 6.6f, 45.3f, 7.0f, 46.0f, 8.5f, 46.5f, 10.5f, 46.6f, 12.5f, 47.0f,
            14.5f, 47.3f, 16.0f, 47.6f
        };
        constexpr float k_pyrenees[]{ -1.8f, 43.1f, 0.5f, 42.7f, 2.8f, 42.5f };
        constexpr float k_carpathians[]{
            17.5f, 48.6f, 19.5f, 49.3f, 22.0f, 49.0f, 24.5f, 47.8f, 25.8f, 46.5f, 25.5f, 45.6f,
            23.0f, 45.4f, 21.5f, 45.0f
        };
        constexpr float k_dinarides[]{
            14.5f, 45.6f, 16.5f, 44.0f, 18.5f, 42.8f, 20.5f, 41.5f, 21.5f, 40.0f, 22.0f, 38.5f
        };
        constexpr float k_balkans[]{ 22.5f, 43.3f, 25.0f, 42.8f, 27.0f, 42.7f };
        constexpr float k_apennines[]{
            8.5f, 44.3f, 11.0f, 44.0f, 13.0f, 42.5f, 15.0f, 41.0f, 16.0f, 39.5f
        };
        constexpr float k_scandes[]{
            6.5f, 58.8f, 7.5f, 60.5f, 9.0f, 62.0f, 12.0f, 63.8f, 14.0f, 65.8f, 16.5f, 67.5f,
            19.0f, 68.8f, 22.0f, 69.6f
        };
        constexpr float k_scottishHighlands[]{ -5.5f, 56.5f, -4.0f, 57.3f, -3.5f, 57.5f };
        constexpr float k_centralSystem[]{ -8.0f, 40.5f, -5.0f, 40.5f, -3.0f, 40.8f };
        constexpr float k_baetic[]{ -5.0f, 37.3f, -3.0f, 37.1f };
        constexpr float k_atlas[]{
            -9.5f, 30.8f, -7.5f, 31.3f, -5.0f, 32.3f, -3.0f, 33.2f, 0.0f, 34.2f, 3.0f, 35.5f,
            6.0f, 35.8f, 9.0f, 36.0f
        };
        constexpr float k_hoggar[]{ 5.0f, 23.0f, 6.0f, 23.6f };
        constexpr float k_tibesti[]{ 17.5f, 21.0f, 19.0f, 20.6f };
        constexpr float k_redSeaHills[]{ 34.0f, 26.0f, 36.0f, 22.0f };
        constexpr float k_ethiopianHighlands[]{ 37.0f, 14.0f, 38.5f, 11.0f, 39.0f, 8.0f };
        constexpr float k_rockies[]{
            -124.0f, 57.0f, -121.0f, 55.0f, -118.0f, 52.5f, -116.0f, 50.5f, -114.0f, 48.5f,
            -112.0f, 46.5f, -110.5f, 44.5f, -108.0f, 42.8f, -106.5f, 40.5f, -106.0f, 38.0f,
            -105.8f, 36.0f, -105.5f, 33.5f
        };
        constexpr float k_coastMountains[]{
            -136.0f, 59.0f, -132.0f, 56.5f, -128.0f, 53.5f, -125.0f, 51.0f, -122.0f, 49.0f,
            -121.6f, 46.5f, -121.8f, 44.0f, -122.0f, 41.5f
        };
        constexpr float k_sierraNevada[]{
            -121.0f, 40.0f, -120.0f, 38.5f, -118.8f, 37.0f, -118.2f, 35.8f
        };
        constexpr float k_alaskaRange[]{
            -155.0f, 60.0f, -152.0f, 62.5f, -149.0f, 63.3f, -145.0f, 63.0f, -141.0f, 61.2f,
            -137.0f, 60.0f
        };
        constexpr float k_brooksRange[]{
            -162.0f, 68.3f, -155.0f, 68.3f, -148.0f, 68.5f, -142.0f, 68.8f
        };
        constexpr float k_mackenzie[]{
            -140.0f, 65.0f, -133.0f, 64.0f, -128.0f, 62.5f, -125.0f, 61.0f
        };
        constexpr float k_sierraMadreOccidental[]{
            -109.5f, 31.0f, -107.5f, 28.0f, -105.5f, 25.0f, -104.0f, 22.5f
        };
        constexpr float k_sierraMadreOriental[]{
            -102.0f, 27.0f, -100.0f, 25.0f, -99.0f, 22.0f, -98.0f, 20.0f
        };
        constexpr float k_appalachians[]{
            -87.0f, 33.5f, -84.0f, 35.0f, -81.5f, 36.5f, -79.5f, 38.0f, -77.5f, 40.0f,
            -75.5f, 41.5f, -73.5f, 43.0f, -71.5f, 44.5f, -69.0f, 46.0f, -66.0f, 47.5f
        };
        constexpr float k_northernAndes[]{
            -75.0f, 10.0f, -77.0f, 4.0f, -78.0f, 0.0f, -79.0f, -4.0f
        };

        // The ranges the northern half shows, by the height of their crests and their reach.
        constexpr std::array<Range, 50> k_ranges{
            Range{ k_himalaya, 1.0f, 1.7f },
            Range{ k_karakoram, 0.95f, 1.3f },
            Range{ k_hinduKush, 0.7f, 1.3f },
            Range{ k_pamir, 0.75f, 1.6f },
            Range{ k_tianShan, 0.62f, 1.6f },
            Range{ k_kunlun, 0.75f, 1.5f },
            Range{ k_qilian, 0.55f, 1.2f },
            Range{ k_altai, 0.45f, 1.7f },
            Range{ k_sayan, 0.35f, 1.4f },
            Range{ k_hengduan, 0.6f, 1.6f },
            Range{ k_qinling, 0.3f, 0.9f },
            Range{ k_sulaiman, 0.3f, 0.9f },
            Range{ k_zagros, 0.45f, 1.5f },
            Range{ k_alborz, 0.45f, 0.8f },
            Range{ k_caucasus, 0.5f, 1.0f },
            Range{ k_taurus, 0.35f, 1.0f },
            Range{ k_pontic, 0.3f, 0.8f },
            Range{ k_urals, 0.18f, 1.1f },
            Range{ k_verkhoyansk, 0.3f, 1.6f },
            Range{ k_chersky, 0.35f, 1.6f },
            Range{ k_stanovoy, 0.25f, 1.4f },
            Range{ k_kamchatka, 0.35f, 0.9f },
            Range{ k_japaneseAlps, 0.35f, 0.7f },
            Range{ k_hejaz, 0.25f, 0.8f },
            Range{ k_hajar, 0.25f, 0.6f },
            Range{ k_alps, 0.56f, 0.9f },
            Range{ k_pyrenees, 0.35f, 0.6f },
            Range{ k_carpathians, 0.25f, 0.9f },
            Range{ k_dinarides, 0.25f, 1.0f },
            Range{ k_balkans, 0.2f, 0.6f },
            Range{ k_apennines, 0.25f, 0.7f },
            Range{ k_scandes, 0.25f, 1.2f },
            Range{ k_scottishHighlands, 0.12f, 0.8f },
            Range{ k_centralSystem, 0.2f, 0.8f },
            Range{ k_baetic, 0.3f, 0.6f },
            Range{ k_atlas, 0.4f, 1.0f },
            Range{ k_hoggar, 0.3f, 1.6f },
            Range{ k_tibesti, 0.35f, 1.4f },
            Range{ k_redSeaHills, 0.18f, 0.7f },
            Range{ k_ethiopianHighlands, 0.4f, 2.0f },
            Range{ k_rockies, 0.45f, 1.8f },
            Range{ k_coastMountains, 0.35f, 1.0f },
            Range{ k_sierraNevada, 0.4f, 0.7f },
            Range{ k_alaskaRange, 0.55f, 1.0f },
            Range{ k_brooksRange, 0.25f, 1.0f },
            Range{ k_mackenzie, 0.25f, 1.2f },
            Range{ k_sierraMadreOccidental, 0.3f, 1.2f },
            Range{ k_sierraMadreOriental, 0.25f, 0.9f },
            Range{ k_appalachians, 0.15f, 1.4f },
            Range{ k_northernAndes, 0.8f, 1.6f }
        };

        // Outlines as longitude, latitude pairs.
        constexpr float k_tibet[]{
            77.5f, 34.5f, 79.5f, 31.2f, 83.0f, 29.2f, 87.0f, 28.4f, 91.0f, 28.6f, 95.0f, 29.4f,
            98.5f, 31.0f, 100.0f, 33.0f, 99.0f, 35.5f, 93.0f, 36.5f, 86.0f, 36.2f, 80.0f, 35.8f
        };
        constexpr float k_mongolia[]{
            88.0f, 49.0f, 100.0f, 50.0f, 115.0f, 49.0f, 118.0f, 45.0f, 110.0f, 42.0f, 100.0f, 42.0f,
            92.0f, 44.0f
        };
        constexpr float k_iran[]{
            47.0f, 35.0f, 52.0f, 33.0f, 58.0f, 31.0f, 61.0f, 33.0f, 60.0f, 36.0f, 54.0f, 36.0f,
            49.0f, 37.0f
        };
        constexpr float k_anatolia[]{
            30.0f, 38.0f, 36.0f, 38.5f, 41.0f, 39.5f, 42.0f, 40.5f, 34.0f, 40.0f, 30.0f, 39.5f
        };
        constexpr float k_greatBasin[]{
            -120.0f, 42.0f, -112.0f, 42.0f, -111.0f, 37.0f, -115.0f, 35.0f, -119.0f, 37.0f
        };
        constexpr float k_coloradoPlateau[]{
            -112.0f, 38.0f, -108.0f, 39.0f, -106.5f, 36.0f, -109.0f, 34.0f, -112.0f, 35.0f
        };
        constexpr float k_mexicanPlateau[]{
            -106.0f, 29.0f, -102.0f, 28.0f, -100.0f, 24.0f, -101.0f, 20.0f, -104.0f, 21.0f,
            -105.0f, 25.0f
        };
        constexpr float k_greenlandIce[]{
            -55.0f, 80.0f, -30.0f, 81.0f, -24.0f, 74.0f, -38.0f, 66.0f, -45.0f, 63.0f,
            -50.0f, 66.0f, -54.0f, 72.0f
        };

        constexpr std::array<Plateau, 8> k_plateaus{
            Plateau{ k_tibet, 0.55f },
            Plateau{ k_mongolia, 0.18f },
            Plateau{ k_iran, 0.2f },
            Plateau{ k_anatolia, 0.15f },
            Plateau{ k_greatBasin, 0.18f },
            Plateau{ k_coloradoPlateau, 0.2f },
            Plateau{ k_mexicanPlateau, 0.2f },
            Plateau{ k_greenlandIce, 0.32f }
        };

        constexpr std::array<DryRegion, 16> k_dryRegions{
            DryRegion{ 5.0f, 24.0f, 22.0f, 8.0f, 1.0f }, // the western Sahara
            DryRegion{ 22.0f, 25.0f, 12.0f, 7.0f, 1.0f }, // the Libyan desert
            DryRegion{ -8.0f, 23.0f, 10.0f, 6.0f, 1.0f }, // Mauritania
            DryRegion{ 30.0f, 24.0f, 6.0f, 6.0f, 0.95f }, // Egypt
            DryRegion{ 46.0f, 23.0f, 11.0f, 7.0f, 0.95f }, // Arabia
            DryRegion{ 60.0f, 29.0f, 9.0f, 5.0f, 0.75f }, // Iran's deserts
            DryRegion{ 71.0f, 27.0f, 5.0f, 4.0f, 0.7f }, // the Thar
            DryRegion{ 83.0f, 39.0f, 9.0f, 2.8f, 0.9f }, // the Taklamakan
            DryRegion{ 105.0f, 43.0f, 13.0f, 4.0f, 0.8f }, // the Gobi
            DryRegion{ 92.0f, 37.0f, 6.0f, 3.0f, 0.6f }, // the Qaidam
            DryRegion{ 65.0f, 46.0f, 16.0f, 4.0f, 0.55f }, // the Kazakh steppe
            DryRegion{ 55.0f, 41.0f, 6.0f, 4.0f, 0.6f }, // the Karakum
            DryRegion{ -116.0f, 39.0f, 5.0f, 5.0f, 0.6f }, // the Great Basin
            DryRegion{ -112.0f, 32.0f, 6.0f, 4.0f, 0.85f }, // the Sonoran desert
            DryRegion{ -104.0f, 29.0f, 4.0f, 4.0f, 0.7f }, // the Chihuahuan desert
            DryRegion{ -103.0f, 45.0f, 6.0f, 6.0f, 0.35f } // the Great Plains
        };

        // A point in space: on the unit sphere where the noise follows the sphere.
        struct SpacePoint
        {
            float x;
            float y;
            float z;
        };

        // Four values for each texel of a level, row by row from the pole, before they are packed.
        struct TexelValues
        {
            int width;
            int height;
            std::vector<std::array<float, 4>> values;
        };

        // The texels about 8 points, and how far between them each point stands.
        struct TexelCorners
        {
            __m256i upperLeft;
            __m256i upperRight;
            __m256i lowerLeft;
            __m256i lowerRight;
            __m256 acrossShare;
            __m256 downShare;
        };

        // The texels about 8 points at two neighbouring levels, and how far toward the coarser
        // each point reads.
        struct TexelBlend
        {
            TexelCorners finer;
            TexelCorners coarser;
            __m256 coarserShare;
        };

        // How a byte of a texel is kept.
        enum class Byte
        {
            Unsigned,   // 0 to 255
            Signed,     // -127 to 127
            SlopeRoot   // a slope's signed square root, -127 to 127
        };

        // The land at 8 points.
        struct LandLanes
        {
            __m256 heights;
            __m256 slopesEast;
            __m256 slopesNorth;
            __m256 variation;
            __m256 dryness;
            __m256 forest;
            __m256 tundra;
        };

        // The point `share` of the way from `from` to `to`, without std::lerp's end checks.
        [[nodiscard]] constexpr float blend(const float from, const float to, const float share)
        {
            return from + (to - from) * share;
        }
        // Gradient noise, about -1 to 1: smooth ramps between random slopes at the lattice.
        [[nodiscard]] float gradientNoise(SpacePoint, std::uint32_t seed);
        // Octaves of gradient noise, each finer and half as strong, as a share of their sum.
        [[nodiscard]] float fractalNoise(SpacePoint, float frequency, int octaves,
            std::uint32_t seed);
        // Octaves of noise folded into sharp crests, 0 to 1, each finer octave on the crests.
        [[nodiscard]] float ridgedNoise(SpacePoint, float frequency, int octaves,
            std::uint32_t seed);
        // The point on the unit sphere at a longitude and latitude, in degrees.
        [[nodiscard]] SpacePoint onSphereAt(float longitude, float latitude);
        // A difference of longitudes, in degrees, turned into -180 to 180.
        [[nodiscard]] float wrappedLongitude(float difference);
        // How far a point is from a line through `points`, in degrees of arc on a small patch.
        [[nodiscard]] float distanceToLine(std::span<const float> points, bool closed,
            float longitude, float latitude);
        [[nodiscard]] bool isInside(std::span<const float> outline, float longitude,
            float latitude);
        // The heights of the land at every texel of the detailed relief, row by row from the pole.
        [[nodiscard]] std::vector<float> landHeights();
        // A range's height over the texels its foothills reach, kept where it rises highest.
        void raiseRange(const Range&, std::uint32_t seed, std::vector<float>& heights);
        void raisePlateau(const Plateau&, std::uint32_t seed, std::vector<float>& heights);
        // Each texel's height, its slopes east and north, and the variation, at the finest level.
        [[nodiscard]] TexelValues reliefValues(const std::vector<float>& heights);
        // Each texel's dryness, forest and tundra, at the finest level.
        [[nodiscard]] TexelValues coverValues();
        // The slopes of heights kept `width` by `height`, east and north per radian of arc.
        [[nodiscard]] std::array<float, 2> slopesAt(const std::vector<float>& heights, int width,
            int height, int column, int row);
        // A pyramid from its finest level's values, each texel packed by `pack`.
        [[nodiscard]] TexelPyramid pyramidOf(TexelValues finest,
            std::uint32_t (*pack)(const std::array<float, 4>&));
        // An empty pyramid `width` by `height` at its finest, its levels to be filled.
        [[nodiscard]] TexelPyramid emptyPyramid(int width, int height);
        // The next level's values, half as fine each way.
        [[nodiscard]] TexelValues halved(const TexelValues&);
        // A level's texels packed from its values, each row's first copied past its last.
        void packLevel(const TexelValues&, std::uint32_t (*pack)(const std::array<float, 4>&),
            int level, TexelPyramid&);
        // A height, two slopes and a variation packed in a texel's four bytes.
        [[nodiscard]] std::uint32_t reliefTexel(const std::array<float, 4>& values);
        // A dryness, forest and tundra packed in a texel's first three bytes.
        [[nodiscard]] std::uint32_t coverTexel(const std::array<float, 4>& values);
        [[nodiscard]] std::uint32_t byteOf(float share);
        [[nodiscard]] std::uint32_t signedByteOf(float share);
        // About the base-2 logarithm of each positive value, within a hundredth.
        [[nodiscard]] __m256 quickLog2(__m256 values);
        // The level whose texels span what each point's ground does, below 0 where it is finer.
        [[nodiscard]] __m256 levelFor(const TexelPyramid&, __m256 longitudeSpans,
            __m256 latitudeSpans);
        // The texels about each point at the two levels nearest `level`, from `leastLevel` on.
        [[nodiscard]] TexelBlend blendAt(const TexelPyramid&, __m256 level, float leastLevel,
            __m256 longitudes, __m256 latitudes);
        // The texels about each point at its level, every point's level in `level`.
        [[nodiscard]] TexelCorners cornersAt(const TexelPyramid&, __m256i level,
            __m256 longitudes, __m256 latitudes);
        // Byte `index` of each texel, as it is kept.
        [[nodiscard]] __m256 decoded(__m256i texels, int index, Byte);
        // Byte `index` decoded at the corners about each point, blended between them.
        [[nodiscard]] __m256 bilinear(const TexelCorners&, int index, Byte);
        // Byte `index` read at both levels about each point, blended between them.
        [[nodiscard]] __m256 blended(const TexelBlend&, int index, Byte);
        [[nodiscard]] LandLanes landLanesAt(const TexelPyramid& relief, const TexelPyramid& cover,
            float leastReliefLevel, __m256 longitudes, __m256 latitudes, __m256 longitudeSpans,
            __m256 latitudeSpans);
        // The outline with a point at least every `spacing` degrees of arc along each edge.
        [[nodiscard]] LandOutline densified(std::span<const FloatPoint> outline, float spacing);
        // Each point moved by the coast's broad warp and its fine grain.
        void roughen(LandOutline&);
    }

    // **The relief is laid in from the ranges' spines and the plateaus' outlines**, over low hills
    // everywhere. The land cover follows the dry regions and the latitude, its edges wandering
    // with noise. Each coarser level averages the values of the one before, which are packed
    // only then. See PathArt#earthmap
    EarthMap::EarthMap()
    {
        m_relief = pyramidOf(reliefValues(landHeights()), reliefTexel);
        m_cover = pyramidOf(coverValues(), coverTexel);
    }

    // **Eight points at a time**, each read between the two levels nearest the ground it stands
    // for: four texels at each level from the relief and from the cover. A last group short of
    // eight is read from copies padded with its last point.
    void EarthMap::readLand(const MapQuery& query, const Relief relief,
        const LandPoints& points) const
    {
        const float leastReliefLevel = relief == Relief::Detailed ? 0.0f : k_broadLevel;
        const std::size_t count = query.longitudes.size();
        const auto store = [&](const LandLanes& lanes, std::size_t first, std::size_t lanesUsed) {
            alignas(32) std::array<float, 8> values{};
            const auto storeOne = [&](__m256 lane, std::span<float> into) {
                if (lanesUsed == 8)
                {
                    _mm256_storeu_ps(into.data() + first, lane);
                    return;
                }
                _mm256_store_ps(values.data(), lane);
                std::copy_n(values.begin(), lanesUsed, into.begin() + first);
            };
            storeOne(lanes.heights, points.heights);
            storeOne(lanes.slopesEast, points.slopesEast);
            storeOne(lanes.slopesNorth, points.slopesNorth);
            storeOne(lanes.variation, points.variation);
            storeOne(lanes.dryness, points.dryness);
            storeOne(lanes.forest, points.forest);
            storeOne(lanes.tundra, points.tundra);
        };
        const auto read = [&](const std::array<const float*, 4>& from) {
            return landLanesAt(m_relief, m_cover, leastReliefLevel, _mm256_loadu_ps(from[0]),
                _mm256_loadu_ps(from[1]), _mm256_loadu_ps(from[2]), _mm256_loadu_ps(from[3]));
        };
        const std::array<std::span<const float>, 4> queried = {
            query.longitudes, query.latitudes, query.longitudeSpans, query.latitudeSpans
        };
        for (std::size_t first = 0; first < count; first += 8)
        {
            const std::size_t lanesUsed = std::min<std::size_t>(8, count - first);
            if (lanesUsed == 8)
            {
                store(read({
                    queried[0].data() + first, queried[1].data() + first,
                    queried[2].data() + first, queried[3].data() + first
                }), first, 8);
                continue;
            }
            std::array<std::array<float, 8>, 4> padded{};
            for (std::size_t value = 0; value != 4; ++value)
            {
                for (std::size_t lane = 0; lane != 8; ++lane)
                    padded[value][lane] = queried[value][first + std::min(lane, lanesUsed - 1)];
            }
            store(read({ padded[0].data(), padded[1].data(), padded[2].data(), padded[3].data() }),
                first, lanesUsed);
        }
    }

    // **Each outline gets a point every 0.1 degree of arc**, and each point moves by noise fixed
    // to where it stands on the sphere: a broad warp that bends the coast, and a fine grain that
    // roughens it. Both stay small enough that no coast folds over itself.
    std::vector<LandOutline> landOutlines()
    {
        const NaturalEarthLand land = naturalEarthLand();
        std::vector<LandOutline> outlines;
        outlines.reserve(land.pointCounts.size());
        std::vector<FloatPoint> corners;
        std::size_t next = 0;
        for (const std::uint16_t pointCount : land.pointCounts)
        {
            corners.clear();
            for (std::uint16_t point = 0; point != pointCount; ++point)
            {
                corners.push_back({
                    0.01f * static_cast<float>(land.points[next]),
                    0.01f * static_cast<float>(land.points[next + 1])
                });
                next += 2;
            }
            LandOutline outline = densified(corners, k_coastPointSpacing);
            roughen(outline);
            outlines.push_back(std::move(outline));
        }
        return outlines;
    }

    namespace
    {
        // The hash spreads the corner's bits with the multiplies and shifts of a 32-bit mixer.
        float gradientNoise(const SpacePoint point, const std::uint32_t seed)
        {
            const float floorX = std::floor(point.x);
            const float floorY = std::floor(point.y);
            const float floorZ = std::floor(point.z);
            const int x = static_cast<int>(floorX);
            const int y = static_cast<int>(floorY);
            const int z = static_cast<int>(floorZ);
            const float offsetX = point.x - floorX;
            const float offsetY = point.y - floorY;
            const float offsetZ = point.z - floorZ;
            const auto eased = [](float share) {
                return share * share * share * (share * (share * 6.0f - 15.0f) + 10.0f);
            };
            // One of twelve slopes along the edges of a cube, picked by the corner's hash.
            const auto slopeAt = [&](int cornerX, int cornerY, int cornerZ) {
                std::uint32_t hash = seed * 0x9E3779B1u
                    ^ static_cast<std::uint32_t>(x + cornerX) * 0x85EBCA77u
                    ^ static_cast<std::uint32_t>(y + cornerY) * 0xC2B2AE3Du
                    ^ static_cast<std::uint32_t>(z + cornerZ) * 0x27D4EB2Fu;
                hash ^= hash >> 15;
                hash *= 0x2C1B3C6Du;
                hash ^= hash >> 12;
                hash *= 0x297A2D39u;
                hash ^= hash >> 15;
                const float dx = offsetX - static_cast<float>(cornerX);
                const float dy = offsetY - static_cast<float>(cornerY);
                const float dz = offsetZ - static_cast<float>(cornerZ);
                const float u = (hash & 8u) != 0u ? dy : dx;
                const float v = (hash & 12u) == 0u ? dy : ((hash & 12u) == 12u ? dx : dz);
                return ((hash & 1u) != 0u ? -u : u) + ((hash & 2u) != 0u ? -v : v);
            };
            const float shareX = eased(offsetX);
            const float shareY = eased(offsetY);
            const float shareZ = eased(offsetZ);
            const auto alongX = [&](int cornerY, int cornerZ) {
                return blend(slopeAt(0, cornerY, cornerZ), slopeAt(1, cornerY, cornerZ), shareX);
            };
            const float near = blend(alongX(0, 0), alongX(1, 0), shareY);
            const float far = blend(alongX(0, 1), alongX(1, 1), shareY);
            return blend(near, far, shareZ);
        }

        float fractalNoise(const SpacePoint point, float frequency, const int octaves,
            const std::uint32_t seed)
        {
            float total = 0.0f;
            float amplitude = 1.0f;
            float amplitudeSum = 0.0f;
            for (int octave = 0; octave != octaves; ++octave)
            {
                const SpacePoint scaled = {
                    point.x * frequency,
                    point.y * frequency,
                    point.z * frequency
                };
                total += amplitude
                    * gradientNoise(scaled, seed + static_cast<std::uint32_t>(octave));
                amplitudeSum += amplitude;
                amplitude *= 0.5f;
                frequency *= k_lacunarity;
            }
            return total / amplitudeSum;
        }

        // Each octave counts as far as the one before stands on a crest, so the fine ridges run
        // along the coarse ones and the valleys stay smooth.
        float ridgedNoise(const SpacePoint point, float frequency, const int octaves,
            const std::uint32_t seed)
        {
            float total = 0.0f;
            float amplitude = 1.0f;
            float amplitudeSum = 0.0f;
            float weight = 1.0f;
            for (int octave = 0; octave != octaves; ++octave)
            {
                const SpacePoint scaled = {
                    point.x * frequency,
                    point.y * frequency,
                    point.z * frequency
                };
                const float noise = gradientNoise(scaled,
                    seed + static_cast<std::uint32_t>(octave));
                float crest = std::clamp(1.0f - std::abs(noise * 1.4f), 0.0f, 1.0f);
                crest *= crest;
                total += amplitude * crest * weight;
                amplitudeSum += amplitude;
                weight = std::clamp(crest * 1.6f, 0.0f, 1.0f);
                amplitude *= 0.5f;
                frequency *= k_lacunarity;
            }
            return total / amplitudeSum;
        }

        SpacePoint onSphereAt(const float longitude, const float latitude)
        {
            const float lon = longitude * k_degree;
            const float lat = latitude * k_degree;
            return { std::cos(lat) * std::sin(lon), std::sin(lat), std::cos(lat) * std::cos(lon) };
        }

        float wrappedLongitude(const float difference)
        {
            return difference - 360.0f * std::round(difference / 360.0f);
        }

        // The longitudes are shortened by the cosine of the point's latitude, which holds well
        // enough over the few degrees a range or a plateau's edge reaches.
        float distanceToLine(const std::span<const float> points, const bool closed,
            const float longitude, const float latitude)
        {
            const float across = std::cos(latitude * k_degree);
            const std::size_t count = points.size() / 2;
            const std::size_t segments = closed ? count : count - 1;
            float nearest = std::numeric_limits<float>::max();
            for (std::size_t i = 0; i != segments; ++i)
            {
                const std::size_t next = (i + 1) % count;
                const float fromX = wrappedLongitude(points[2 * i] - longitude) * across;
                const float fromY = points[2 * i + 1] - latitude;
                const float toX = wrappedLongitude(points[2 * next] - longitude) * across;
                const float toY = points[2 * next + 1] - latitude;
                const float alongX = toX - fromX;
                const float alongY = toY - fromY;
                const float lengthSquared = alongX * alongX + alongY * alongY;
                const float share = lengthSquared > 0.0f
                    ? std::clamp(-(fromX * alongX + fromY * alongY) / lengthSquared, 0.0f, 1.0f)
                    : 0.0f;
                const float offX = fromX + alongX * share;
                const float offY = fromY + alongY * share;
                nearest = std::min(nearest, std::sqrt(offX * offX + offY * offY));
            }
            return nearest;
        }

        bool isInside(const std::span<const float> outline, const float longitude,
            const float latitude)
        {
            bool inside = false;
            const std::size_t count = outline.size() / 2;
            std::size_t previous = count - 1;
            for (std::size_t i = 0; i != count; ++i)
            {
                const float ax = outline[2 * i];
                const float ay = outline[2 * i + 1];
                const float bx = outline[2 * previous];
                const float by = outline[2 * previous + 1];
                const bool straddles = (ay > latitude) != (by > latitude);
                if (straddles && longitude < (bx - ax) * (latitude - ay) / (by - ay) + ax)
                    inside = !inside;
                previous = i;
            }
            return inside;
        }

        // Low hills everywhere first, the plateaus over them, then the ranges, each raising what
        // it reaches to the greater of the two.
        std::vector<float> landHeights()
        {
            std::vector<float> heights(static_cast<std::size_t>(k_detailedWidth)
                * k_detailedHeight);
            spreadOverCores(k_detailedHeight, [&](int row) {
                const float latitude = 90.0f
                    - (static_cast<float>(row) + 0.5f) * 90.0f / k_detailedHeight;
                float* rowHeights = heights.data()
                    + static_cast<std::size_t>(row) * k_detailedWidth;
                for (int column = 0; column != k_detailedWidth; ++column)
                {
                    const float longitude = (static_cast<float>(column) + 0.5f) * 360.0f
                        / k_detailedWidth;
                    const SpacePoint onSphere = onSphereAt(longitude, latitude);
                    const float swell = fractalNoise(onSphere, 9.0f, 5, 7u);
                    const float grain = ridgedNoise(onSphere, 80.0f, 4, 9u);
                    rowHeights[column] = 0.05f * smoothStep(-0.1f, 0.6f, swell)
                        + 0.03f * grain * smoothStep(-0.2f, 0.4f, swell);
                }
            });
            std::uint32_t seed = 100u;
            for (const Plateau& plateau : k_plateaus)
                raisePlateau(plateau, ++seed, heights);
            for (const Range& range : k_ranges)
                raiseRange(range, ++seed, heights);
            return heights;
        }

        // **A range is a core and a skirt of foothills** 2.6 times as wide, its edges wandering
        // and its crest rising and falling along it, cut into ridges by the ridged noise.
        void raiseRange(const Range& range, const std::uint32_t seed, std::vector<float>& heights)
        {
            float west = 180.0f;
            float east = -180.0f;
            float south = 90.0f;
            float north = -90.0f;
            for (std::size_t i = 0; i + 1 < range.spine.size(); i += 2)
            {
                west = std::min(west, range.spine[i]);
                east = std::max(east, range.spine[i]);
                south = std::min(south, range.spine[i + 1]);
                north = std::max(north, range.spine[i + 1]);
            }
            // The wandering widens the reach by up to 35 percent, and a little more for safety.
            const float reach = range.width * k_footReach * 1.4f;
            const float highest = std::min(80.0f,
                std::max(std::abs(south), std::abs(north)) + reach);
            const float reachAcross = reach / std::cos(highest * k_degree);
            constexpr float k_texelsPerDegree{ k_detailedWidth / 360.0f };
            const int firstColumn = static_cast<int>(std::floor((west - reachAcross)
                * k_texelsPerDegree));
            const int lastColumn = static_cast<int>(std::ceil((east + reachAcross)
                * k_texelsPerDegree));
            const int firstRow = std::max(0,
                static_cast<int>(std::floor((90.0f - (north + reach)) * k_texelsPerDegree)));
            const int lastRow = std::min(k_detailedHeight - 1,
                static_cast<int>(std::ceil((90.0f - (south - reach)) * k_texelsPerDegree)));
            if (firstRow > lastRow)
                return;
            spreadOverCores(lastRow - firstRow + 1, [&](int rowIndex) {
                const int row = firstRow + rowIndex;
                const float latitude = 90.0f
                    - (static_cast<float>(row) + 0.5f) * 90.0f / k_detailedHeight;
                float* rowHeights = heights.data()
                    + static_cast<std::size_t>(row) * k_detailedWidth;
                for (int unwrapped = firstColumn; unwrapped <= lastColumn; ++unwrapped)
                {
                    const int column = (unwrapped % k_detailedWidth + k_detailedWidth)
                        % k_detailedWidth;
                    const float longitude = (static_cast<float>(column) + 0.5f) * 360.0f
                        / k_detailedWidth;
                    const float distance = distanceToLine(range.spine, false, longitude, latitude);
                    if (distance >= reach)
                        continue;
                    const SpacePoint onSphere = onSphereAt(longitude, latitude);
                    const float width = range.width
                        * (1.0f + 0.35f * fractalNoise(onSphere, 30.0f, 3, seed + 50u));
                    const float coreShare = distance / width;
                    const float footShare = distance / (width * k_footReach);
                    if (footShare >= 1.0f)
                        continue;
                    const float core = coreShare < 1.0f
                        ? (1.0f - coreShare * coreShare) * (1.0f - coreShare * coreShare)
                        : 0.0f;
                    const float foot = (1.0f - footShare * footShare)
                        * (1.0f - footShare * footShare);
                    const float along = 0.78f + 0.3f * fractalNoise(onSphere, 12.0f, 2, seed + 70u);
                    const float crest = ridgedNoise(onSphere, 95.0f, 6, seed);
                    const float raised = range.height * (0.72f * core + 0.28f * foot) * along
                        * (0.3f + 0.75f * crest);
                    float& height = rowHeights[column];
                    height = std::max(height, raised + 0.3f * height);
                }
            });
        }

        void raisePlateau(const Plateau& plateau, const std::uint32_t seed,
            std::vector<float>& heights)
        {
            float west = 180.0f;
            float east = -180.0f;
            float south = 90.0f;
            float north = -90.0f;
            for (std::size_t i = 0; i + 1 < plateau.outline.size(); i += 2)
            {
                west = std::min(west, plateau.outline[i]);
                east = std::max(east, plateau.outline[i]);
                south = std::min(south, plateau.outline[i + 1]);
                north = std::max(north, plateau.outline[i + 1]);
            }
            constexpr float k_texelsPerDegree{ k_detailedWidth / 360.0f };
            const int firstColumn = static_cast<int>(std::floor(west * k_texelsPerDegree));
            const int lastColumn = static_cast<int>(std::ceil(east * k_texelsPerDegree));
            const int firstRow = std::max(0,
                static_cast<int>(std::floor((90.0f - north) * k_texelsPerDegree)));
            const int lastRow = std::min(k_detailedHeight - 1,
                static_cast<int>(std::ceil((90.0f - south) * k_texelsPerDegree)));
            spreadOverCores(lastRow - firstRow + 1, [&](int rowIndex) {
                const int row = firstRow + rowIndex;
                const float latitude = 90.0f
                    - (static_cast<float>(row) + 0.5f) * 90.0f / k_detailedHeight;
                float* rowHeights = heights.data()
                    + static_cast<std::size_t>(row) * k_detailedWidth;
                for (int unwrapped = firstColumn; unwrapped <= lastColumn; ++unwrapped)
                {
                    const int column = (unwrapped % k_detailedWidth + k_detailedWidth)
                        % k_detailedWidth;
                    const float longitude = wrappedLongitude((static_cast<float>(column) + 0.5f)
                        * 360.0f / k_detailedWidth);
                    if (!isInside(plateau.outline, longitude, latitude))
                        continue;
                    const float edge = distanceToLine(plateau.outline, true, longitude, latitude);
                    const SpacePoint onSphere = onSphereAt(longitude, latitude);
                    const float rough = ridgedNoise(onSphere, 70.0f, 5, seed);
                    const float swell = fractalNoise(onSphere, 25.0f, 3, seed + 30u);
                    const float raised = plateau.height * smoothStep(0.0f, 1.3f, edge)
                        * (0.72f + 0.35f * rough + 0.12f * swell);
                    float& height = rowHeights[column];
                    height = std::max(height, raised);
                }
            });
        }

        TexelValues reliefValues(const std::vector<float>& heights)
        {
            TexelValues relief{
                .width = k_detailedWidth,
                .height = k_detailedHeight,
                .values = std::vector<std::array<float, 4>>(heights.size())
            };
            spreadOverCores(k_detailedHeight, [&](int row) {
                const float latitude = 90.0f
                    - (static_cast<float>(row) + 0.5f) * 90.0f / k_detailedHeight;
                const std::size_t rowStart = static_cast<std::size_t>(row) * k_detailedWidth;
                for (int column = 0; column != k_detailedWidth; ++column)
                {
                    const float longitude = (static_cast<float>(column) + 0.5f) * 360.0f
                        / k_detailedWidth;
                    const std::size_t at = rowStart + static_cast<std::size_t>(column);
                    const std::array<float, 2> slopes = slopesAt(heights, k_detailedWidth,
                        k_detailedHeight, column, row);
                    relief.values[at] = {
                        heights[at],
                        slopes[0],
                        slopes[1],
                        fractalNoise(onSphereAt(longitude, latitude), 40.0f, 4, 503u)
                    };
                }
            });
            return relief;
        }

        TexelValues coverValues()
        {
            TexelValues cover{
                .width = k_coverWidth,
                .height = k_coverHeight,
                .values = std::vector<std::array<float, 4>>(
                    static_cast<std::size_t>(k_coverWidth) * k_coverHeight)
            };
            spreadOverCores(k_coverHeight, [&](int row) {
                const float latitude = 90.0f
                    - (static_cast<float>(row) + 0.5f) * 90.0f / k_coverHeight;
                const std::size_t rowStart = static_cast<std::size_t>(row) * k_coverWidth;
                for (int column = 0; column != k_coverWidth; ++column)
                {
                    const float longitude = (static_cast<float>(column) + 0.5f) * 360.0f
                        / k_coverWidth;
                    const SpacePoint onSphere = onSphereAt(longitude, latitude);
                    const float warpAcross = 5.0f * fractalNoise(onSphere, 7.0f, 3, 511u);
                    const float warpUp = 3.5f * fractalNoise(onSphere, 7.0f, 3, 512u);
                    float dryness = 0.0f;
                    for (const DryRegion& region : k_dryRegions)
                    {
                        const float across = (wrappedLongitude(longitude - region.longitude)
                            + warpAcross) / region.radiusAcross;
                        const float up = (latitude + warpUp - region.latitude) / region.radiusUp;
                        const float distance = std::sqrt(across * across + up * up);
                        dryness = std::max(dryness,
                            region.dryness * (1.0f - smoothStep(0.35f, 1.35f, distance)));
                    }
                    dryness = std::clamp(dryness
                        + 0.22f * fractalNoise(onSphere, 14.0f, 5, 501u), 0.0f, 1.0f);
                    // The forest and the tundra end along wandering lines, not circles of
                    // latitude.
                    const float coldness = latitude + 3.5f * fractalNoise(onSphere, 10.0f, 3, 502u);
                    const float forest = smoothStep(47.0f, 54.0f, coldness)
                        * (1.0f - smoothStep(0.3f, 0.5f, dryness));
                    const float tundra = smoothStep(63.0f, 69.0f, coldness);
                    cover.values[rowStart + static_cast<std::size_t>(column)] = {
                        dryness, forest, tundra, 0.0f
                    };
                }
            });
            return cover;
        }

        // Central differences, one-sided at the pole and the equator; near the pole a degree of
        // longitude is held at no less than a fiftieth of its length at the equator.
        std::array<float, 2> slopesAt(const std::vector<float>& heights, const int width,
            const int height, const int column, const int row)
        {
            const auto at = [&](int atColumn, int atRow) {
                return heights[static_cast<std::size_t>(atRow) * static_cast<std::size_t>(width)
                    + static_cast<std::size_t>((atColumn + width) % width)];
            };
            const float radiansAcross = 2.0f * k_pi / static_cast<float>(width);
            const float radiansUp = 0.5f * k_pi / static_cast<float>(height);
            const float latitude = 0.5f * k_pi - (static_cast<float>(row) + 0.5f) * radiansUp;
            const float circle = std::max(std::cos(latitude), 0.02f);
            const float east = (at(column + 1, row) - at(column - 1, row))
                / (2.0f * radiansAcross * circle);
            const int northRow = std::max(row - 1, 0);
            const int southRow = std::min(row + 1, height - 1);
            const float north = (at(column, northRow) - at(column, southRow))
                / (static_cast<float>(southRow - northRow) * radiansUp);
            return { east, north };
        }

        TexelPyramid pyramidOf(TexelValues finest,
            std::uint32_t (* const pack)(const std::array<float, 4>&))
        {
            TexelPyramid pyramid = emptyPyramid(finest.width, finest.height);
            TexelValues values = std::move(finest);
            for (int level = 0; level != k_levels; ++level)
            {
                if (level != 0)
                    values = halved(values);
                packLevel(values, pack, level, pyramid);
            }
            return pyramid;
        }

        // The levels follow one another in one run of texels, each row one texel longer than the
        // level is wide, so a 4-byte read never runs past the last.
        TexelPyramid emptyPyramid(const int width, const int height)
        {
            decltype(TexelPyramid::offsets) offsets{};
            std::size_t texels = 0;
            for (int level = 0; level != k_levels; ++level)
            {
                offsets[static_cast<std::size_t>(level)] = static_cast<int>(texels);
                texels += static_cast<std::size_t>((width >> level) + 1)
                    * static_cast<std::size_t>(height >> level);
            }
            return {
                .width = width,
                .height = height,
                .texels = std::vector<std::uint32_t>(texels),
                .offsets = offsets
            };
        }

        // **Each texel is the average of the 2 by 2 under it**, which keeps the mean of every
        // value, the slopes' among them.
        TexelValues halved(const TexelValues& finer)
        {
            TexelValues coarser{
                .width = finer.width / 2,
                .height = finer.height / 2,
                .values = std::vector<std::array<float, 4>>(finer.values.size() / 4)
            };
            const std::size_t finerWidth = static_cast<std::size_t>(finer.width);
            spreadOverCores(coarser.height, [&](int row) {
                const std::array<float, 4>* upper = finer.values.data()
                    + 2 * static_cast<std::size_t>(row) * finerWidth;
                const std::array<float, 4>* lower = upper + finerWidth;
                std::array<float, 4>* to = coarser.values.data()
                    + static_cast<std::size_t>(row) * static_cast<std::size_t>(coarser.width);
                for (std::size_t column = 0; column != static_cast<std::size_t>(coarser.width);
                    ++column)
                {
                    for (std::size_t value = 0; value != 4; ++value)
                    {
                        to[column][value] = 0.25f * (upper[2 * column][value]
                            + upper[2 * column + 1][value] + lower[2 * column][value]
                            + lower[2 * column + 1][value]);
                    }
                }
            });
            return coarser;
        }

        void packLevel(const TexelValues& values,
            std::uint32_t (* const pack)(const std::array<float, 4>&), const int level,
            TexelPyramid& pyramid)
        {
            const std::size_t width = static_cast<std::size_t>(values.width);
            spreadOverCores(values.height, [&](int row) {
                std::uint32_t* texels = pyramid.texels.data()
                    + pyramid.offsets[static_cast<std::size_t>(level)]
                    + static_cast<std::size_t>(row) * (width + 1);
                const std::array<float, 4>* from = values.values.data()
                    + static_cast<std::size_t>(row) * width;
                for (std::size_t column = 0; column != width; ++column)
                    texels[column] = pack(from[column]);
                texels[width] = texels[0];
            });
        }

        // A slope is kept as the square root of its share of the steepest, so a gentle slope
        // keeps its grain where a byte of the plain share would round it away.
        std::uint32_t reliefTexel(const std::array<float, 4>& values)
        {
            const auto slopeByte = [](float slope) {
                const float share = std::sqrt(std::min(std::abs(slope) / k_steepest, 1.0f));
                return signedByteOf(slope < 0.0f ? -share : share);
            };
            return byteOf(values[0] / k_heightRange) | slopeByte(values[1]) << 8
                | slopeByte(values[2]) << 16 | signedByteOf(values[3]) << 24;
        }

        std::uint32_t coverTexel(const std::array<float, 4>& values)
        {
            return byteOf(values[0]) | byteOf(values[1]) << 8 | byteOf(values[2]) << 16;
        }

        std::uint32_t byteOf(const float share)
        {
            return static_cast<std::uint32_t>(std::lround(std::clamp(share, 0.0f, 1.0f) * 255.0f));
        }

        std::uint32_t signedByteOf(const float share)
        {
            const long value = std::lround(std::clamp(share, -1.0f, 1.0f) * 127.0f);
            return static_cast<std::uint32_t>(static_cast<std::uint8_t>(static_cast<std::int8_t>(
                value)));
        }

        // The exponent's bits give the whole part, and a parabola through 1, 2 and the middle
        // the rest from the mantissa. Zero comes out at -127, and every value as a number.
        __m256 quickLog2(const __m256 values)
        {
            const __m256i bits = _mm256_castps_si256(values);
            const __m256 exponent = _mm256_cvtepi32_ps(_mm256_sub_epi32(_mm256_srli_epi32(bits, 23),
                _mm256_set1_epi32(127)));
            const __m256 mantissa = _mm256_sub_ps(_mm256_castsi256_ps(_mm256_or_si256(
                _mm256_and_si256(bits, _mm256_set1_epi32(0x7FFFFF)),
                _mm256_set1_epi32(0x3F800000))), _mm256_set1_ps(1.0f));
            return _mm256_fmadd_ps(mantissa, _mm256_fnmadd_ps(mantissa,
                _mm256_set1_ps(0.3465557f), _mm256_set1_ps(1.3465557f)), exponent);
        }

        // A point's ground spans as many texels as the greater of its two spans does.
        __m256 levelFor(const TexelPyramid& pyramid, const __m256 longitudeSpans,
            const __m256 latitudeSpans)
        {
            const float columnsPerRadian = static_cast<float>(pyramid.width) / (2.0f * k_pi);
            const float rowsPerRadian = static_cast<float>(pyramid.height) / (0.5f * k_pi);
            return quickLog2(_mm256_max_ps(
                _mm256_mul_ps(longitudeSpans, _mm256_set1_ps(columnsPerRadian)),
                _mm256_mul_ps(latitudeSpans, _mm256_set1_ps(rowsPerRadian))));
        }

        // **The level is held from `leastLevel` to the coarsest**, and where no point leans
        // toward the coarser of its two the coarser is not gathered.
        TexelBlend blendAt(const TexelPyramid& pyramid, const __m256 level, const float leastLevel,
            const __m256 longitudes, const __m256 latitudes)
        {
            const __m256 held = _mm256_min_ps(_mm256_max_ps(level, _mm256_set1_ps(leastLevel)),
                _mm256_set1_ps(static_cast<float>(k_levels - 1)));
            const __m256 finerLevel = _mm256_min_ps(_mm256_floor_ps(held),
                _mm256_set1_ps(static_cast<float>(k_levels - 2)));
            const __m256i finer = _mm256_cvttps_epi32(finerLevel);
            const __m256 coarserShare = _mm256_sub_ps(held, finerLevel);
            const TexelCorners finerCorners = cornersAt(pyramid, finer, longitudes, latitudes);
            if (_mm256_movemask_ps(_mm256_cmp_ps(coarserShare, _mm256_setzero_ps(),
                _CMP_GT_OQ)) == 0)
            {
                return { finerCorners, finerCorners, coarserShare };
            }
            return {
                finerCorners,
                cornersAt(pyramid, _mm256_add_epi32(finer, _mm256_set1_epi32(1)), longitudes,
                    latitudes),
                coarserShare
            };
        }

        // **Each level wraps round the longitudes** any number of turns either way, and holds its
        // first row north of its first texel centers and its last south of its last, the
        // equator's included. The maximum takes its second operand where the first is not a
        // number, so every read stays inside the level.
        TexelCorners cornersAt(const TexelPyramid& pyramid, const __m256i level,
            const __m256 longitudes, const __m256 latitudes)
        {
            const __m256i one = _mm256_set1_epi32(1);
            const __m256i width = _mm256_srlv_epi32(_mm256_set1_epi32(pyramid.width), level);
            const __m256i lastRow = _mm256_sub_epi32(
                _mm256_srlv_epi32(_mm256_set1_epi32(pyramid.height), level), one);
            // Two to the minus level, its exponent laid in the bits.
            const __m256 coarseness = _mm256_castsi256_ps(_mm256_slli_epi32(
                _mm256_sub_epi32(_mm256_set1_epi32(127), level), 23));
            const __m256 columnsPerRadian = _mm256_mul_ps(coarseness,
                _mm256_set1_ps(static_cast<float>(pyramid.width) / (2.0f * k_pi)));
            const __m256 rowsPerRadian = _mm256_mul_ps(coarseness,
                _mm256_set1_ps(static_cast<float>(pyramid.height) / (0.5f * k_pi)));
            const __m256 half = _mm256_set1_ps(0.5f);
            const __m256 across = _mm256_fmsub_ps(longitudes, columnsPerRadian, half);
            const __m256 down = _mm256_min_ps(_mm256_max_ps(_mm256_fmsub_ps(
                _mm256_sub_ps(_mm256_set1_ps(0.5f * k_pi), latitudes), rowsPerRadian, half),
                _mm256_setzero_ps()), _mm256_cvtepi32_ps(lastRow));
            const __m256 leftColumn = _mm256_floor_ps(across);
            const __m256 topRow = _mm256_floor_ps(down);
            const __m256i left = _mm256_and_si256(_mm256_cvttps_epi32(leftColumn),
                _mm256_sub_epi32(width, one));
            const __m256i top = _mm256_cvttps_epi32(topRow);
            const __m256i bottom = _mm256_min_epi32(_mm256_add_epi32(top, one), lastRow);
            const __m256i rowLength = _mm256_add_epi32(width, one);
            const __m256i levelStart = _mm256_permutevar8x32_epi32(_mm256_loadu_si256(
                reinterpret_cast<const __m256i*>(pyramid.offsets.data())), level);
            const __m256i upper = _mm256_add_epi32(levelStart,
                _mm256_add_epi32(_mm256_mullo_epi32(top, rowLength), left));
            const __m256i lower = _mm256_add_epi32(levelStart,
                _mm256_add_epi32(_mm256_mullo_epi32(bottom, rowLength), left));
            const int* texels = reinterpret_cast<const int*>(pyramid.texels.data());
            return {
                .upperLeft = _mm256_i32gather_epi32(texels, upper, 4),
                .upperRight = _mm256_i32gather_epi32(texels, _mm256_add_epi32(upper, one), 4),
                .lowerLeft = _mm256_i32gather_epi32(texels, lower, 4),
                .lowerRight = _mm256_i32gather_epi32(texels, _mm256_add_epi32(lower, one), 4),
                .acrossShare = _mm256_sub_ps(across, leftColumn),
                .downShare = _mm256_sub_ps(down, topRow)
            };
        }

        // A slope's byte holds its root, so the slope is the root times its own size.
        __m256 decoded(const __m256i texels, const int index, const Byte kind)
        {
            if (kind == Byte::Unsigned)
            {
                return _mm256_cvtepi32_ps(_mm256_and_si256(
                    _mm256_srl_epi32(texels, _mm_cvtsi32_si128(8 * index)),
                    _mm256_set1_epi32(0xFF)));
            }
            const __m256 value = _mm256_cvtepi32_ps(_mm256_sra_epi32(
                _mm256_sll_epi32(texels, _mm_cvtsi32_si128(24 - 8 * index)),
                _mm_cvtsi32_si128(24)));
            if (kind == Byte::Signed)
                return value;
            return _mm256_mul_ps(value, _mm256_andnot_ps(_mm256_set1_ps(-0.0f), value));
        }

        __m256 bilinear(const TexelCorners& corners, const int index, const Byte kind)
        {
            const __m256 upperLeft = decoded(corners.upperLeft, index, kind);
            const __m256 lowerLeft = decoded(corners.lowerLeft, index, kind);
            const __m256 upper = _mm256_fmadd_ps(_mm256_sub_ps(
                decoded(corners.upperRight, index, kind), upperLeft), corners.acrossShare,
                upperLeft);
            const __m256 lower = _mm256_fmadd_ps(_mm256_sub_ps(
                decoded(corners.lowerRight, index, kind), lowerLeft), corners.acrossShare,
                lowerLeft);
            return _mm256_fmadd_ps(_mm256_sub_ps(lower, upper), corners.downShare, upper);
        }

        __m256 blended(const TexelBlend& texels, const int index, const Byte kind)
        {
            const __m256 finer = bilinear(texels.finer, index, kind);
            return _mm256_fmadd_ps(_mm256_sub_ps(bilinear(texels.coarser, index, kind), finer),
                texels.coarserShare, finer);
        }

        LandLanes landLanesAt(const TexelPyramid& relief, const TexelPyramid& cover,
            const float leastReliefLevel, const __m256 longitudes, const __m256 latitudes,
            const __m256 longitudeSpans, const __m256 latitudeSpans)
        {
            const TexelBlend reliefTexels = blendAt(relief,
                levelFor(relief, longitudeSpans, latitudeSpans), leastReliefLevel, longitudes,
                latitudes);
            const TexelBlend coverTexels = blendAt(cover,
                levelFor(cover, longitudeSpans, latitudeSpans), 0.0f, longitudes, latitudes);
            const __m256 perByte = _mm256_set1_ps(1.0f / 255.0f);
            const __m256 perSlopeRoot = _mm256_set1_ps(k_steepest / (127.0f * 127.0f));
            return {
                .heights = _mm256_mul_ps(blended(reliefTexels, 0, Byte::Unsigned),
                    _mm256_set1_ps(k_heightRange / 255.0f)),
                .slopesEast = _mm256_mul_ps(blended(reliefTexels, 1, Byte::SlopeRoot),
                    perSlopeRoot),
                .slopesNorth = _mm256_mul_ps(blended(reliefTexels, 2, Byte::SlopeRoot),
                    perSlopeRoot),
                .variation = _mm256_mul_ps(blended(reliefTexels, 3, Byte::Signed),
                    _mm256_set1_ps(1.0f / 127.0f)),
                .dryness = _mm256_mul_ps(blended(coverTexels, 0, Byte::Unsigned), perByte),
                .forest = _mm256_mul_ps(blended(coverTexels, 1, Byte::Unsigned), perByte),
                .tundra = _mm256_mul_ps(blended(coverTexels, 2, Byte::Unsigned), perByte)
            };
        }

        // An edge's length shortens its degrees of longitude by the cosine of its middle latitude.
        LandOutline densified(const std::span<const FloatPoint> outline, const float spacing)
        {
            LandOutline dense;
            for (std::size_t i = 0; i != outline.size(); ++i)
            {
                const FloatPoint from = outline[i];
                const FloatPoint to = outline[(i + 1) % outline.size()];
                const float across = (to.x - from.x) * std::cos(0.5f * (from.y + to.y) * k_degree);
                const float length = std::hypot(across, to.y - from.y);
                const int steps = std::max(1, static_cast<int>(std::ceil(length / spacing)));
                for (int step = 0; step != steps; ++step)
                {
                    const float share = static_cast<float>(step) / static_cast<float>(steps);
                    dense.push_back(pointBetween(from, to, share));
                }
            }
            return dense;
        }

        // The noise is read where the point stands on the sphere, so a point that two outlines
        // share at the 180th meridian moves alike in both. The move eastward is in degrees of arc,
        // which take more degrees of longitude toward the pole.
        void roughen(LandOutline& outline)
        {
            for (FloatPoint& point : outline)
            {
                const SpacePoint onSphere = onSphereAt(point.x, point.y);
                const float east = k_coastWarpReach * fractalNoise(onSphere, 5.0f, 3, 11u)
                    + k_coastGrainReach * fractalNoise(onSphere, 45.0f, 3, 13u);
                const float north = k_coastWarpReach * fractalNoise(onSphere, 5.0f, 3, 12u)
                    + k_coastGrainReach * fractalNoise(onSphere, 45.0f, 3, 14u);
                point.x += east / std::cos(point.y * k_degree);
                point.y += north;
            }
        }
    }
}
