module;
#include <immintrin.h>
module ClaFi.PathArt.NightLights;

import ClaFi.PathArt.EarthMap;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    namespace
    {
        constexpr float k_pi{ std::numbers::pi_v<float> };
        constexpr float k_degree{ k_pi / 180.0f };
        // The finest level's texels round each circle of latitude.
        constexpr int k_columns{ 8192 };
        // The finest level's texels from k_north down to 61 degrees south.
        constexpr int k_rows{ 3328 };
        constexpr float k_north{ 85.0f };
        // The side of the finest level's texels, in degrees.
        constexpr float k_texelDegrees{ 360.0f / k_columns };
        constexpr int k_tileSide{ 16 };
        constexpr int k_tileTexels{ k_tileSide * k_tileSide };
        // The light a texel's byte holds at 255, in light a pixel. A pixel is lit whole past about
        // 5, and the glow gathers the brighter cores' light from the coarser levels.
        constexpr float k_brightestLight{ 64.0f };
        // The finest cells the tiny lights stand in, a 16th of a texel each way, in degrees.
        constexpr float k_finestCell{ k_texelDegrees / 16.0f };
        // The levels of cells finer than the texels.
        constexpr int k_cellLevelsPerTexel{ 4 };
        // The pixels across the cells a pixel's lights are taken from.
        constexpr float k_cellPixels{ 2.0f };
        // Where across an octave of cell sizes the coarser cells' lights start to take over, and
        // over how much of it.
        constexpr float k_coarserFrom{ 0.3f };
        constexpr float k_coarserSpan{ 0.4f };
        // The pixels across the ground the glow gathers its light from.
        constexpr float k_glowPixels{ 6.0f };
        // The glow's share of the light it gathers.
        constexpr float k_glowShare{ 0.12f };
        // A tiny light's brightness is k_dimmestLight and k_lightRise times a draw to the 6th.
        constexpr float k_dimmestLight{ 0.15f };
        constexpr float k_lightRise{ 2.8f };
        constexpr float k_meanLight{ k_dimmestLight + k_lightRise / 7.0f };
        // What a tent one pixel in radius lays, in pixels.
        constexpr float k_tentVolume{ k_pi / 3.0f };
        // The darkness over which a tiny light switches on.
        constexpr float k_switchingDarkness{ 0.12f };
        // How much earlier the densest lights switch on, as a share of the night's falling.
        constexpr float k_denseHeadStart{ 0.65f };

        // The light at a town's middle, by its size past a 20th.
        constexpr float k_coreLight{ 250.0f };
        // The light along a town's rays, its spokes and the corridors, by its size past a floor.
        constexpr float k_rayLight{ 1.5f };
        constexpr float k_spokeLight{ 1.25f };
        constexpr float k_corridorLight{ 1.0f };
        // The light at a village's middle.
        constexpr float k_villageLight{ 20.0f };
        // The chance of a village on a texel of the best land beside a town of size 1.
        constexpr float k_villageChance{ 0.006f };
        // The farthest a town reaches for the two nearest it to send corridors to, in degrees.
        constexpr float k_corridorReach{ 1.6f };
        // The least size of a town that sends out spokes.
        constexpr float k_leastSpokeSize{ 0.1f };
        // How far a coast stands at most from a town that sends a spoke along it, in degrees.
        constexpr float k_coastReach{ 0.3f };
        // The side of the cells the coast's points are found by, in degrees.
        constexpr float k_coastCell{ 0.25f };
        constexpr int k_coastColumns{ 1440 };
        // The side of the cells the towns' nearness is kept in, in degrees.
        constexpr float k_nearnessCell{ 0.25f };
        constexpr int k_nearnessColumns{ 1440 };
        constexpr int k_nearnessRows{ 580 };
        // The latitudes the villages are tried between.
        constexpr float k_villageNorth{ 72.0f };
        constexpr float k_villageSouth{ -56.0f };

        // A stream of numbers from an integer hash, the same with every compiler.
        class HashRandom
        {
        public:
            explicit HashRandom(std::uint32_t seed);
        public:
            // 0 to 1.
            [[nodiscard]] float next();
            // About normally spread about 0, with a spread of 1.
            [[nodiscard]] float wobble();
            // 1 or -1, as likely either way.
            [[nodiscard]] float side();
        private:
            std::uint32_t m_seed{};
            std::uint32_t m_drawn{};
        };

        // A bit for each texel of the finest level, set where its middle lies on the land.
        class LandGrid
        {
        public:
            explicit LandGrid(std::span<const std::span<const FloatPoint>> inhabited);
        public:
            [[nodiscard]] bool holds(int row, int column) const;
            [[nodiscard]] bool holds(FloatPoint lonLat) const;
            // Whether the land holds every point between two, a 200th of a degree apart.
            [[nodiscard]] bool holdsBetween(FloatPoint from, FloatPoint to) const;
        private:
            std::vector<std::uint64_t> m_bits{};
        };

        // The light laid down while the lights are built, in tiles made where light first lands.
        class LightField
        {
        public:
            LightField(int columns, int rows);
        public:
            [[nodiscard]] int columns() const { return m_columns; }
            [[nodiscard]] int rows() const { return m_rows; }
            [[nodiscard]] int tilesAcross() const { return m_tilesAcross; }
            [[nodiscard]] int tilesDown() const { return m_tilesDown; }
            // Adds light to a texel; the column wraps round, and a row past either end takes none.
            void add(int row, int column, float light);
            // Adds light about a point, shared among the 4 texels nearest by how near each is.
            void addAt(FloatPoint lonLat, float light);
            // The light a tile holds, texel by texel, or none where no light landed in it.
            [[nodiscard]] std::span<float> tileAt(int tileRow, int tileColumn);
            [[nodiscard]] std::span<const float> tileAt(int tileRow, int tileColumn) const;
        private:
            int m_columns{};
            int m_rows{};
            int m_tilesAcross{};
            int m_tilesDown{};
            // Where each tile's light is kept in m_tiles, or -1.
            std::vector<std::int32_t> m_tileIndexes{};
            std::vector<float> m_tiles{};
        };

        // What every part of the build lays its light from, and into.
        struct LightBuild
        {
            std::span<const LightTown> towns;
            std::span<const std::span<const FloatPoint>> inhabited;
            const EarthMap& map;
            const LandGrid& land;
            LightField& field;
        };

        [[nodiscard]] std::uint32_t mixedBits(std::uint32_t value);
        [[nodiscard]] float unitOf(std::uint32_t bits);
        // The move from one point to another in degrees of arc, east and north.
        [[nodiscard]] FloatPoint arcBetween(FloatPoint from, FloatPoint to);
        [[nodiscard]] FloatPoint movedOn(FloatPoint from, FloatPoint arc);
        [[nodiscard]] float arcLength(FloatPoint arc);
        // Light along a path, `lightAt` the light per degree at each distance along it.
        void lightAlong(LightField&, std::span<const FloatPoint> path,
            const std::function<float(float)>& lightAt);
        // A blot of light about a point, falling off as 1 / (1 + (r / radius)^2)^1.5.
        void lightBlot(LightField&, FloatPoint center, float light, float radius);
        // A town or a village: a blot at its middle and rays wandering out over the land.
        void lightStar(const LightBuild&, FloatPoint center, float size, float coreLight,
            HashRandom&);
        // Each town as a star, with villages about it.
        void lightTowns(const LightBuild&);
        // The spokes the larger towns send along the coast and into the land.
        void lightSpokes(const LightBuild&);
        // A corridor from each town to the two nearest it, where the land joins them.
        void lightCorridors(const LightBuild&);
        // Villages over the land, where the map makes towns likely and more near the towns.
        void lightVillages(const LightBuild&);
        // The light clumped by noise, and cut where its texel lies over the water.
        void clumpAndCut(LightField&, const LandGrid&);
        // Value noise in the plane, 0 to 1.
        [[nodiscard]] float planeNoise(float x, float y, std::uint32_t seed);
        // The kept light, as the reads see it.
        struct KeptLight
        {
            const std::uint8_t* texels;
            const std::int32_t* tileStarts;
            std::span<const LightLevel> levels;
        };

        // 8 points of a query, in degrees.
        struct PointLanes
        {
            __m256 longitudes; // -180 to 180
            __m256 latitudes;
            __m256 footprints;
            __m256 darkness;
            __m256 dark; // all bits set where the darkness is above 0
        };

        // The light at texels of a level; rows past either end hold none.
        [[nodiscard]] __m256 lightAt(const KeptLight&, int level, __m256i rows, __m256i columns);
        // Where points stand among a level's texels, across and down.
        [[nodiscard]] std::array<__m256, 2> texelPlacesAt(int level, __m256 longitudes,
            __m256 latitudes);
        // The light at points, blended from the 4 texels of a level about each.
        [[nodiscard]] __m256 blendedLightAt(const KeptLight&, int level, __m256 longitudes,
            __m256 latitudes);
        // The light of the texel of a level each point lies in.
        [[nodiscard]] __m256 nearestLightAt(const KeptLight&, int level, __m256 longitudes,
            __m256 latitudes);
        // The tiny lights of one level of cells that reach 8 pixels.
        [[nodiscard]] __m256 lightsOfLevel(const KeptLight&, int cellLevel, const PointLanes&,
            __m256 inverseFootprints);
        // The finest whole level any of 8 points in the dark asks for, and no less than 0.
        [[nodiscard]] float finestLevelOf(__m256 levels, __m256 dark);
        [[nodiscard]] PointLanes pointLanesOf(const std::array<const float*, 4>& from);
        // `read` over a query 8 points at a time; points past the end take the last point's place
        // and no darkness.
        void readByEight(const LightQuery&, std::span<float> values,
            const std::function<__m256(const std::array<const float*, 4>&)>& read);
        // About the base-2 logarithm of each positive value, within a hundredth.
        [[nodiscard]] __m256 quickLog2(__m256 values);
        // The cosine of each angle between -pi / 2 and pi / 2, within three millionths.
        [[nodiscard]] __m256 quickCosine(__m256 angles);
        [[nodiscard]] __m256i mixedLanes(__m256i values);
        [[nodiscard]] __m256 unitLanes(__m256i bits);
        // 0 to 1 by the upper 16 bits of each lane, and by the lower 16.
        [[nodiscard]] std::array<__m256, 2> unitPairLanes(__m256i bits);
        // 0 to 1 by the lowest 8 bits of each lane.
        [[nodiscard]] __m256 byteUnitLanes(__m256i bits);
        // 0 below `edge0`, 1 above `edge0` plus `width`, and an S-curve between, in each lane.
        [[nodiscard]] __m256 smoothLanes(__m256 edge0, float width, __m256 values);
    }

    // **The light is laid down as a field of floats, then kept as bytes**: the cube root of the
    // light as a share of k_brightestLight, so the faint light keeps its steps fine. Each coarser
    // level averages the 2 by 2 texels under each of its own. A tile no light reached is the
    // first tile, all 0.
    NightLights::NightLights(const std::span<const LightTown> towns,
        const std::span<const std::span<const FloatPoint>> inhabited, const EarthMap& map)
    {
        const LandGrid land{ inhabited };
        LightField field{ k_columns, k_rows };
        const LightBuild build{
            .towns = towns,
            .inhabited = inhabited,
            .map = map,
            .land = land,
            .field = field
        };
        lightTowns(build);
        lightSpokes(build);
        lightCorridors(build);
        lightVillages(build);
        clumpAndCut(field, land);

        m_texels.assign(static_cast<std::size_t>(k_tileTexels), 0);
        LightField finer = std::move(field);
        for (int level = 0; level != k_lightLevels; ++level)
        {
            LightLevel& kept = m_levels[static_cast<std::size_t>(level)];
            kept = {
                .tilesAcross = finer.tilesAcross(),
                .tilesDown = finer.tilesDown(),
                .firstTile = static_cast<int>(m_tileStarts.size())
            };
            for (int tileRow = 0; tileRow != kept.tilesDown; ++tileRow)
            {
                for (int tileColumn = 0; tileColumn != kept.tilesAcross; ++tileColumn)
                {
                    const std::span<const float> tile = std::as_const(finer).tileAt(tileRow,
                        tileColumn);
                    std::array<std::uint8_t, k_tileTexels> bytes{};
                    bool lit = false;
                    for (std::size_t texel = 0; texel != tile.size(); ++texel)
                    {
                        const float share = std::min(tile[texel], k_brightestLight)
                            / k_brightestLight;
                        bytes[texel] = static_cast<std::uint8_t>(
                            std::lround(255.0f * std::cbrt(std::max(share, 0.0f))));
                        lit = lit || bytes[texel] != 0;
                    }
                    if (!lit)
                    {
                        m_tileStarts.push_back(0);
                        continue;
                    }
                    m_tileStarts.push_back(static_cast<std::int32_t>(m_texels.size()));
                    m_texels.insert(m_texels.end(), bytes.begin(), bytes.end());
                }
            }
            if (level + 1 == k_lightLevels)
                break;
            LightField coarser{ finer.columns() / 2, (finer.rows() + 1) / 2 };
            for (int tileRow = 0; tileRow != finer.tilesDown(); ++tileRow)
            {
                for (int tileColumn = 0; tileColumn != finer.tilesAcross(); ++tileColumn)
                {
                    const std::span<const float> tile = std::as_const(finer).tileAt(tileRow,
                        tileColumn);
                    for (std::size_t texel = 0; texel != tile.size(); ++texel)
                    {
                        if (tile[texel] == 0.0f)
                            continue;
                        const int row = tileRow * k_tileSide + static_cast<int>(texel) / k_tileSide;
                        const int column = tileColumn * k_tileSide
                            + static_cast<int>(texel) % k_tileSide;
                        coarser.add(row / 2, column / 2, 0.25f * tile[texel]);
                    }
                }
            }
            finer = std::move(coarser);
        }
        m_texels.resize(m_texels.size() + k_readPadding, 0);
    }

    // **Each pixel takes the tiny lights of the 4 cells nearest it**, at the two levels of cells
    // whose cells stand about k_cellPixels across, the coarser taking over across the middle of
    // the octave between them. A cell holds one light at a spot of its own, lit with the chance
    // the light there gives it, brighter past a chance of 1, so a pixel gathers the kept light on
    // average at every level. A light lays a tent a pixel in radius, and switches on at a
    // darkness of its own, the densest lights first. 8 pixels are read at a time, at the finest
    // pair of levels any of them needs, and 8 with no light in the texels about them are passed
    // over. See PathArt#nightlights-readlights
    void NightLights::readLights(const LightQuery& query, const std::span<float> lights) const
    {
        const KeptLight kept = { m_texels.data(), m_tileStarts.data(), m_levels };
        const __m256 zero = _mm256_setzero_ps();
        const __m256 one = _mm256_set1_ps(1.0f);
        readByEight(query, lights, [&](const std::array<const float*, 4>& from) {
            const PointLanes points = pointLanesOf(from);
            if (_mm256_movemask_ps(points.dark) == 0)
                return zero;
            const __m256 cellLevels = quickLog2(_mm256_mul_ps(points.footprints,
                _mm256_set1_ps(k_cellPixels / k_finestCell)));
            const int finerCells = static_cast<int>(finestLevelOf(cellLevels, points.dark));
            // The coarser cells' texels about each pixel, which reach past every cell it reads.
            const int nearbyLevel = std::clamp(finerCells + 1 - k_cellLevelsPerTexel, 0,
                k_lightLevels - 1);
            const __m256 nearby = blendedLightAt(kept, nearbyLevel, points.longitudes,
                points.latitudes);
            if (_mm256_movemask_ps(_mm256_and_ps(_mm256_cmp_ps(nearby, zero, _CMP_GT_OQ),
                points.dark)) == 0)
            {
                return zero;
            }
            const __m256 inverseFootprints = _mm256_div_ps(one, points.footprints);
            const __m256 coarserShare = _mm256_min_ps(one, _mm256_max_ps(zero, _mm256_mul_ps(
                _mm256_sub_ps(cellLevels, _mm256_set1_ps(static_cast<float>(finerCells)
                    + k_coarserFrom)), _mm256_set1_ps(1.0f / k_coarserSpan))));
            const bool finerShown = _mm256_movemask_ps(_mm256_and_ps(_mm256_cmp_ps(coarserShare,
                one, _CMP_LT_OQ), points.dark)) != 0;
            const bool coarserShown = _mm256_movemask_ps(_mm256_and_ps(_mm256_cmp_ps(coarserShare,
                zero, _CMP_GT_OQ), points.dark)) != 0;
            const __m256 finerLights = finerShown
                ? lightsOfLevel(kept, finerCells, points, inverseFootprints)
                : zero;
            const __m256 coarserLights = coarserShown
                ? lightsOfLevel(kept, finerCells + 1, points, inverseFootprints)
                : zero;
            return _mm256_and_ps(_mm256_fmadd_ps(_mm256_sub_ps(coarserLights, finerLights),
                coarserShare, finerLights), points.dark);
        });
    }

    // **The glow is the kept light gathered over k_glowPixels of ground**, read between the two
    // levels nearest that, blended by how near each is, as dark as the darkness. See
    // PathArt#nightlights-readglow
    void NightLights::readGlow(const LightQuery& query, const std::span<float> glows) const
    {
        const KeptLight kept = { m_texels.data(), m_tileStarts.data(), m_levels };
        const __m256 zero = _mm256_setzero_ps();
        const __m256 one = _mm256_set1_ps(1.0f);
        readByEight(query, glows, [&](const std::array<const float*, 4>& from) {
            const PointLanes points = pointLanesOf(from);
            if (_mm256_movemask_ps(points.dark) == 0)
                return zero;
            const __m256 levels = quickLog2(_mm256_mul_ps(points.footprints,
                _mm256_set1_ps(k_glowPixels / k_texelDegrees)));
            const float finerLevel = std::min(finestLevelOf(levels, points.dark),
                static_cast<float>(k_lightLevels - 2));
            const int finer = static_cast<int>(finerLevel);
            const __m256 coarserShare = _mm256_min_ps(one, _mm256_max_ps(zero,
                _mm256_sub_ps(levels, _mm256_set1_ps(finerLevel))));
            const __m256 finerLight = blendedLightAt(kept, finer, points.longitudes,
                points.latitudes);
            const __m256 gathered = _mm256_fmadd_ps(_mm256_sub_ps(blendedLightAt(kept, finer + 1,
                points.longitudes, points.latitudes), finerLight), coarserShare, finerLight);
            return _mm256_and_ps(_mm256_mul_ps(gathered, _mm256_mul_ps(points.darkness,
                _mm256_set1_ps(k_glowShare))), points.dark);
        });
    }

    namespace
    {
        HashRandom::HashRandom(const std::uint32_t seed)
            :
            m_seed{ mixedBits(seed ^ 0x2545F491u) }
        {
        }

        float HashRandom::next()
        {
            ++m_drawn;
            return unitOf(mixedBits(m_seed ^ mixedBits(m_drawn * 0x9E3779B1u)));
        }

        // The sum of four draws, scaled to a spread of 1.
        float HashRandom::wobble()
        {
            const float sum = next() + next() + next() + next();
            return (sum - 2.0f) * std::numbers::sqrt3_v<float>;
        }

        float HashRandom::side()
        {
            return next() < 0.5f ? 1.0f : -1.0f;
        }

        // **Each edge marks the rows whose middle latitude it spans**, half open so a point two
        // edges share counts once, and a row's crossings, sorted, bound the land between each
        // pair. A sea inside the land, its outline the other way round, stays sea.
        LandGrid::LandGrid(const std::span<const std::span<const FloatPoint>> inhabited)
        {
            std::vector<std::vector<float>> crossings(static_cast<std::size_t>(k_rows));
            for (const std::span<const FloatPoint> outline : inhabited)
            {
                for (std::size_t i = 0; i != outline.size(); ++i)
                {
                    const FloatPoint from = outline[i];
                    const FloatPoint to = outline[(i + 1) % outline.size()];
                    const float lower = std::min(from.y, to.y);
                    const float upper = std::max(from.y, to.y);
                    const int firstRow = std::max(0, static_cast<int>(
                        std::floor((k_north - upper) / k_texelDegrees - 0.5f)) + 1);
                    const int lastRow = std::min(k_rows - 1, static_cast<int>(
                        std::floor((k_north - lower) / k_texelDegrees - 0.5f)));
                    for (int row = firstRow; row <= lastRow; ++row)
                    {
                        const float latitude = k_north
                            - (static_cast<float>(row) + 0.5f) * k_texelDegrees;
                        const float share = (latitude - from.y) / (to.y - from.y);
                        crossings[static_cast<std::size_t>(row)].push_back(
                            from.x + (to.x - from.x) * share);
                    }
                }
            }
            m_bits.assign(static_cast<std::size_t>(k_rows) * (k_columns / 64), 0);
            for (int row = 0; row != k_rows; ++row)
            {
                std::vector<float>& rowCrossings = crossings[static_cast<std::size_t>(row)];
                std::ranges::sort(rowCrossings);
                for (std::size_t i = 0; i + 1 < rowCrossings.size(); i += 2)
                {
                    const int firstColumn = static_cast<int>(
                        std::ceil((rowCrossings[i] + 180.0f) / k_texelDegrees - 0.5f));
                    const int lastColumn = static_cast<int>(
                        std::floor((rowCrossings[i + 1] + 180.0f) / k_texelDegrees - 0.5f));
                    for (int column = firstColumn; column <= lastColumn; ++column)
                    {
                        const std::size_t wrapped = static_cast<std::size_t>(
                            (column % k_columns + k_columns) % k_columns);
                        const std::size_t bit = static_cast<std::size_t>(row) * k_columns + wrapped;
                        m_bits[bit / 64] |= std::uint64_t{ 1 } << (bit % 64);
                    }
                }
            }
        }

        bool LandGrid::holds(const int row, const int column) const
        {
            if (row < 0 || row >= k_rows)
                return false;
            const std::size_t wrapped = static_cast<std::size_t>(
                (column % k_columns + k_columns) % k_columns);
            const std::size_t bit = static_cast<std::size_t>(row) * k_columns + wrapped;
            return (m_bits[bit / 64] >> (bit % 64) & 1) != 0;
        }

        bool LandGrid::holds(const FloatPoint lonLat) const
        {
            return holds(static_cast<int>(std::floor((k_north - lonLat.y) / k_texelDegrees)),
                static_cast<int>(std::floor((lonLat.x + 180.0f) / k_texelDegrees)));
        }

        bool LandGrid::holdsBetween(const FloatPoint from, const FloatPoint to) const
        {
            const FloatPoint arc = arcBetween(from, to);
            const int samples = 2 + static_cast<int>(std::ceil(arcLength(arc) / 0.005f));
            for (int sample = 1; sample < samples; ++sample)
            {
                const float share = static_cast<float>(sample) / static_cast<float>(samples);
                if (!holds(movedOn(from, { arc.x * share, arc.y * share })))
                    return false;
            }
            return holds(to);
        }

        LightField::LightField(const int columns, const int rows)
            :
            m_columns{ columns },
            m_rows{ rows },
            m_tilesAcross{ (columns + k_tileSide - 1) / k_tileSide },
            m_tilesDown{ (rows + k_tileSide - 1) / k_tileSide },
            m_tileIndexes(static_cast<std::size_t>(m_tilesAcross)
                * static_cast<std::size_t>(m_tilesDown), -1)
        {
        }

        void LightField::add(const int row, int column, const float light)
        {
            if (row < 0 || row >= m_rows)
                return;
            column = (column % m_columns + m_columns) % m_columns;
            std::int32_t& index = m_tileIndexes[static_cast<std::size_t>(row / k_tileSide
                * m_tilesAcross + column / k_tileSide)];
            if (index < 0)
            {
                index = static_cast<std::int32_t>(m_tiles.size() / k_tileTexels);
                m_tiles.resize(m_tiles.size() + k_tileTexels, 0.0f);
            }
            m_tiles[static_cast<std::size_t>(index) * k_tileTexels
                + static_cast<std::size_t>(row % k_tileSide * k_tileSide + column % k_tileSide)]
                += light;
        }

        void LightField::addAt(const FloatPoint lonLat, const float light)
        {
            const float across = (lonLat.x + 180.0f) / k_texelDegrees - 0.5f;
            const float down = (k_north - lonLat.y) / k_texelDegrees - 0.5f;
            const int column = static_cast<int>(std::floor(across));
            const int row = static_cast<int>(std::floor(down));
            const float acrossShare = across - static_cast<float>(column);
            const float downShare = down - static_cast<float>(row);
            add(row, column, light * (1.0f - acrossShare) * (1.0f - downShare));
            add(row, column + 1, light * acrossShare * (1.0f - downShare));
            add(row + 1, column, light * (1.0f - acrossShare) * downShare);
            add(row + 1, column + 1, light * acrossShare * downShare);
        }

        std::span<float> LightField::tileAt(const int tileRow, const int tileColumn)
        {
            const std::int32_t index = m_tileIndexes[static_cast<std::size_t>(tileRow
                * m_tilesAcross + tileColumn)];
            if (index < 0)
                return {};
            return { m_tiles.data() + static_cast<std::size_t>(index) * k_tileTexels,
                static_cast<std::size_t>(k_tileTexels) };
        }

        std::span<const float> LightField::tileAt(const int tileRow, const int tileColumn) const
        {
            const std::int32_t index = m_tileIndexes[static_cast<std::size_t>(tileRow
                * m_tilesAcross + tileColumn)];
            if (index < 0)
                return {};
            return { m_tiles.data() + static_cast<std::size_t>(index) * k_tileTexels,
                static_cast<std::size_t>(k_tileTexels) };
        }

        std::uint32_t mixedBits(std::uint32_t value)
        {
            value ^= value >> 16;
            value *= 0x7FEB352Du;
            value ^= value >> 15;
            value *= 0x846CA68Bu;
            value ^= value >> 16;
            return value;
        }

        float unitOf(const std::uint32_t bits)
        {
            return static_cast<float>(bits >> 8) * (1.0f / 16777216.0f);
        }

        // A degree of longitude shortens by the latitude's cosine.
        FloatPoint arcBetween(const FloatPoint from, const FloatPoint to)
        {
            const float east = std::remainder(to.x - from.x, 360.0f);
            return { east * std::cos(from.y * k_degree), to.y - from.y };
        }

        FloatPoint movedOn(const FloatPoint from, const FloatPoint arc)
        {
            const float longitude = from.x + arc.x / std::max(std::cos(from.y * k_degree), 0.05f);
            return { std::remainder(longitude, 360.0f), from.y + arc.y };
        }

        float arcLength(const FloatPoint arc)
        {
            return std::hypot(arc.x, arc.y);
        }

        // **A path lays its light every 50th of a degree**, shared among the texels about each
        // step, so a path a texel wide holds `lightAt` in each texel it runs through.
        void lightAlong(LightField& field, const std::span<const FloatPoint> path,
            const std::function<float(float)>& lightAt)
        {
            constexpr float k_stepDegrees{ 0.02f };
            float along = 0.0f;
            for (std::size_t i = 0; i + 1 < path.size(); ++i)
            {
                const FloatPoint arc = arcBetween(path[i], path[i + 1]);
                const float length = arcLength(arc);
                const int steps = std::max(1, static_cast<int>(std::ceil(length / k_stepDegrees)));
                const float step = length / static_cast<float>(steps);
                for (int stepIndex = 0; stepIndex != steps; ++stepIndex)
                {
                    const float share = (static_cast<float>(stepIndex) + 0.5f)
                        / static_cast<float>(steps);
                    field.addAt(movedOn(path[i], { arc.x * share, arc.y * share }),
                        lightAt(along + step * (static_cast<float>(stepIndex) + 0.5f)) * step
                            / k_texelDegrees);
                }
                along += length;
            }
        }

        void lightBlot(LightField& field, const FloatPoint center, const float light,
            const float radius)
        {
            const float reach = radius * 4.0f;
            const float cosLatitude = std::max(std::cos(center.y * k_degree), 0.05f);
            const int firstRow = static_cast<int>(std::floor((k_north - center.y - reach)
                / k_texelDegrees));
            const int lastRow = static_cast<int>(std::floor((k_north - center.y + reach)
                / k_texelDegrees));
            const int firstColumn = static_cast<int>(std::floor(
                (center.x - reach / cosLatitude + 180.0f) / k_texelDegrees));
            const int lastColumn = static_cast<int>(std::floor(
                (center.x + reach / cosLatitude + 180.0f) / k_texelDegrees));
            for (int row = firstRow; row <= lastRow; ++row)
            {
                const float latitude = k_north - (static_cast<float>(row) + 0.5f) * k_texelDegrees;
                for (int column = firstColumn; column <= lastColumn; ++column)
                {
                    const float longitude = -180.0f + (static_cast<float>(column) + 0.5f)
                        * k_texelDegrees;
                    const float east = (longitude - center.x) * cosLatitude;
                    const float north = latitude - center.y;
                    const float squared = (east * east + north * north) / (radius * radius);
                    if (squared > 16.0f)
                        continue;
                    const float falloff = 1.0f / (1.0f + squared);
                    field.add(row, column, light * falloff * std::sqrt(falloff));
                }
            }
        }

        // **A star's rays wander out evenly round it**, each stopping at the water, and fade out
        // along their length. The larger the star, the more and the longer its rays.
        void lightStar(const LightBuild& build, const FloatPoint center, const float size,
            const float coreLight, HashRandom& random)
        {
            constexpr float k_rayStep{ 0.008f };
            lightBlot(build.field, center, coreLight, 0.004f + 0.025f * size);
            const int rays = static_cast<int>(2.0f + 18.0f * size + 3.0f * random.next());
            const float firstAngle = 2.0f * k_pi * random.next();
            std::vector<FloatPoint> path;
            for (int ray = 0; ray != rays; ++ray)
            {
                const float length = (0.02f + 0.5f * std::pow(size, 0.7f))
                    * (0.3f + 0.7f * random.next());
                float heading = firstAngle + 2.0f * k_pi * static_cast<float>(ray)
                    / static_cast<float>(rays) + 0.35f * random.wobble();
                float turn = 0.0f;
                path.assign(1, center);
                for (float along = 0.0f; along < length; along += k_rayStep)
                {
                    turn = std::clamp(0.85f * turn + 0.05f * random.wobble(), -0.1f, 0.1f);
                    heading += turn;
                    const FloatPoint next = movedOn(path.back(),
                        { k_rayStep * std::cos(heading), k_rayStep * std::sin(heading) });
                    if (!build.land.holds(next))
                        break;
                    path.push_back(next);
                }
                if (path.size() < 2)
                    continue;
                lightAlong(build.field, path, [&](float along) {
                    const float share = std::clamp(along / length, 0.0f, 1.0f);
                    return k_rayLight * (0.25f + size) * std::pow(1.0f - share, 1.5f);
                });
            }
        }

        void lightTowns(const LightBuild& build)
        {
            for (std::size_t index = 0; index != build.towns.size(); ++index)
            {
                const LightTown& town = build.towns[index];
                HashRandom random{ static_cast<std::uint32_t>(index) * 104729u + 3u };
                lightStar(build, town.lonLat, town.size, k_coreLight * (0.05f + town.size),
                    random);
                const int villages = static_cast<int>(1.0f + 7.0f * town.size
                    + 2.0f * random.next());
                for (int village = 0; village != villages; ++village)
                {
                    const float angle = 2.0f * k_pi * random.next();
                    const float reach = 0.12f + (0.25f + 0.8f * town.size) * random.next();
                    const FloatPoint at = movedOn(town.lonLat,
                        { reach * std::cos(angle), reach * std::sin(angle) });
                    if (!build.land.holds(at))
                        continue;
                    const float light = k_villageLight * (0.1f + 0.5f * random.next()
                        * random.next());
                    lightBlot(build.field, at, light, 0.008f + 0.012f * random.next());
                }
            }
        }

        // **A town of size k_leastSpokeSize or more sends out 2 spokes or more**, the larger the
        // more and the longer. A town near the coast sends its first along the coast, a little
        // inland, and from size 0.5 a second the other way; larger ones branch inland off them.
        // The rest wander inland, spread about the way away from the coast, turning aside from
        // the water, and the largest towns' spokes fork once. Every spoke stops at the water.
        void lightSpokes(const LightBuild& build)
        {
            const std::span<const std::span<const FloatPoint>> outlines = build.inhabited;
            const LandGrid& land = build.land;
            struct CoastPoint
            {
                std::size_t outline;
                std::size_t point;
            };
            const auto coastCellOf = [](const FloatPoint lonLat) {
                const int row = static_cast<int>(std::floor((90.0f - lonLat.y) / k_coastCell));
                const int column = static_cast<int>(std::floor((lonLat.x + 180.0f) / k_coastCell));
                return static_cast<std::int64_t>(row) * k_coastColumns
                    + (column % k_coastColumns + k_coastColumns) % k_coastColumns;
            };
            std::unordered_map<std::int64_t, std::vector<CoastPoint>> coastCells;
            for (std::size_t outline = 0; outline != outlines.size(); ++outline)
            {
                for (std::size_t point = 0; point != outlines[outline].size(); ++point)
                    coastCells[coastCellOf(outlines[outline][point])].push_back({ outline, point });
            }
            struct Spoke
            {
                FloatPoint from;
                float fromAlong;
                float heading;
                float endAlong;
                bool forks;
            };
            for (std::size_t index = 0; index != build.towns.size(); ++index)
            {
                const LightTown& town = build.towns[index];
                if (town.size < k_leastSpokeSize)
                    continue;
                HashRandom random{ static_cast<std::uint32_t>(index) * 7919u + 17u };
                const int spokeCount = std::max(2,
                    static_cast<int>(town.size * 5.0f + random.next()));
                const float planned = 2.6f * (0.08f + std::pow(town.size, 0.8f));
                const float strength = k_spokeLight * (0.3f + town.size);
                const auto lightSpoke = [&](const std::vector<FloatPoint>& path,
                    const std::vector<float>& alongs) {
                    if (path.size() < 2)
                        return;
                    const float start = alongs.front();
                    const float end = std::max(alongs.back(), 0.05f);
                    lightAlong(build.field, path, [&](float along) {
                        const float share = std::clamp((start + along) / end, 0.0f, 1.0f);
                        return strength * std::pow(1.0f - share, 1.3f);
                    });
                };

                // The nearest point of a coast the land joins the town to.
                std::optional<CoastPoint> nearest;
                float nearestArc = k_coastReach;
                const int centerRow = static_cast<int>(std::floor((90.0f - town.lonLat.y)
                    / k_coastCell));
                const int centerColumn = static_cast<int>(std::floor((town.lonLat.x + 180.0f)
                    / k_coastCell));
                const int columnsAcross = static_cast<int>(std::ceil(k_coastReach
                    / std::max(std::cos((std::abs(town.lonLat.y) + k_coastReach) * k_degree),
                        0.05f) / k_coastCell));
                for (int row = centerRow - 2; row <= centerRow + 2; ++row)
                {
                    for (int column = centerColumn - columnsAcross;
                        column <= centerColumn + columnsAcross; ++column)
                    {
                        const auto found = coastCells.find(static_cast<std::int64_t>(row)
                            * k_coastColumns + (column % k_coastColumns + k_coastColumns)
                                % k_coastColumns);
                        if (found == coastCells.end())
                            continue;
                        for (const CoastPoint& coastPoint : found->second)
                        {
                            const FloatPoint onCoast
                                = outlines[coastPoint.outline][coastPoint.point];
                            const FloatPoint arc = arcBetween(town.lonLat, onCoast);
                            const float distance = arcLength(arc);
                            if (distance >= nearestArc)
                                continue;
                            const float inward = 0.02f / std::max(distance, 1.0e-4f);
                            if (!land.holdsBetween(town.lonLat,
                                movedOn(onCoast, { -arc.x * inward, -arc.y * inward })))
                            {
                                continue;
                            }
                            nearestArc = distance;
                            nearest = coastPoint;
                        }
                    }
                }

                std::vector<Spoke> inlandSpokes;
                const auto walkCoast = [&](const int direction, const float endAlong) {
                    const std::span<const FloatPoint> outline = outlines[nearest->outline];
                    const int pointCount = static_cast<int>(outline.size());
                    const auto pointAt = [&](int point) {
                        return outline[static_cast<std::size_t>(
                            (point % pointCount + pointCount) % pointCount)];
                    };
                    std::vector<FloatPoint> path{ town.lonLat };
                    std::vector<float> alongs{ 0.0f };
                    const float phase = 2.0f * k_pi * random.next();
                    const float inlandBase = 0.035f + 0.03f * random.next();
                    float along = 0.0f;
                    for (int step = 0; step < pointCount; ++step)
                    {
                        const int point = static_cast<int>(nearest->point) + direction * step;
                        FloatPoint run = arcBetween(pointAt(point - 1), pointAt(point + 1));
                        const float runLength = arcLength(run);
                        if (runLength < 1.0e-5f)
                            continue;
                        run = { run.x / runLength, run.y / runLength };
                        // The land lies left of the way an outline runs.
                        const FloatPoint inland = { -run.y, run.x };
                        const float offset = inlandBase + 0.02f
                            * std::sin(phase + 0.5f * static_cast<float>(step));
                        FloatPoint next = movedOn(pointAt(point),
                            { inland.x * offset, inland.y * offset });
                        if (!land.holds(next))
                        {
                            next = movedOn(pointAt(point),
                                { inland.x * 0.015f, inland.y * 0.015f });
                            if (!land.holds(next))
                                break;
                        }
                        const float stepLength = arcLength(arcBetween(path.back(), next));
                        if (stepLength > 0.5f)
                            break;
                        if (stepLength < 0.04f)
                            continue;
                        if (!land.holdsBetween(path.back(), next))
                            break;
                        along += stepLength;
                        if (along > endAlong)
                            break;
                        path.push_back(next);
                        alongs.push_back(along);
                    }
                    if (path.size() < 2)
                        return;
                    const int branches = town.size > 0.25f
                        ? static_cast<int>(town.size * 1.5f * random.next() + 0.4f)
                        : 0;
                    for (int branch = 0; branch < branches && path.size() > 3; ++branch)
                    {
                        const std::size_t at = 1 + static_cast<std::size_t>(random.next()
                            * static_cast<float>(path.size() - 2));
                        FloatPoint run = arcBetween(path[at - 1], path[at + 1]);
                        const float runLength = arcLength(run);
                        if (runLength < 1.0e-5f)
                            continue;
                        run = { run.x / runLength, run.y / runLength };
                        const float inlandSide = static_cast<float>(direction);
                        inlandSpokes.push_back({
                            .from = path[at],
                            .fromAlong = alongs[at],
                            .heading = std::atan2(run.x * inlandSide, -run.y * inlandSide)
                                + 0.35f * random.wobble(),
                            .endAlong = alongs[at] + planned * (0.25f + 0.35f * random.next()),
                            .forks = false
                        });
                    }
                    lightSpoke(path, alongs);
                };
                int coastSpokes = 0;
                if (nearest)
                {
                    const int first = random.next() < 0.5f ? 1 : -1;
                    walkCoast(first, planned * (0.5f + 0.5f * random.next()));
                    ++coastSpokes;
                    if (town.size >= 0.5f)
                    {
                        walkCoast(-first, planned * (0.5f + 0.5f * random.next()));
                        ++coastSpokes;
                    }
                }
                std::optional<float> awayFromCoast;
                if (nearest)
                {
                    const FloatPoint arc = arcBetween(outlines[nearest->outline][nearest->point],
                        town.lonLat);
                    if (arcLength(arc) > 1.0e-4f)
                        awayFromCoast = std::atan2(arc.y, arc.x);
                }
                const int inlandCount = spokeCount - coastSpokes;
                const float base = awayFromCoast ? *awayFromCoast : 2.0f * k_pi * random.next();
                for (int spoke = 0; spoke < inlandCount; ++spoke)
                {
                    const float share = (static_cast<float>(spoke) + 0.5f)
                        / static_cast<float>(inlandCount);
                    const float heading = awayFromCoast
                        ? base + (inlandCount == 1 ? 0.0f : std::lerp(-1.3f, 1.3f, share))
                            + 0.3f * random.wobble()
                        : base + 2.0f * k_pi * static_cast<float>(spoke)
                            / static_cast<float>(inlandCount) + 0.4f * random.wobble();
                    inlandSpokes.push_back({
                        .from = town.lonLat,
                        .fromAlong = 0.0f,
                        .heading = heading,
                        .endAlong = planned * (0.45f + 0.55f * random.next()),
                        .forks = town.size > 0.55f
                    });
                }
                // Each inland spoke steps a 20th of a degree at a time.
                for (std::size_t spokeIndex = 0; spokeIndex != inlandSpokes.size(); ++spokeIndex)
                {
                    constexpr float k_step{ 0.05f };
                    const Spoke spoke = inlandSpokes[spokeIndex];
                    std::vector<FloatPoint> path{ spoke.from };
                    std::vector<float> alongs{ spoke.fromAlong };
                    float heading = spoke.heading;
                    float turn = 0.0f;
                    float along = spoke.fromAlong;
                    const float side = random.side();
                    float forkAt = spoke.forks && spoke.endAlong - spoke.fromAlong > 0.8f
                        ? std::lerp(spoke.fromAlong, spoke.endAlong, 0.3f + 0.35f * random.next())
                        : -1.0f;
                    while (along < spoke.endAlong)
                    {
                        turn = std::clamp(0.92f * turn + 0.025f * random.wobble(), -0.06f, 0.06f);
                        heading += turn;
                        bool moved = false;
                        for (const float aside : { 0.0f, 0.4f, -0.4f })
                        {
                            const float tried = heading + aside * side;
                            const FloatPoint next = movedOn(path.back(),
                                { k_step * std::cos(tried), k_step * std::sin(tried) });
                            if (!land.holdsBetween(path.back(), next))
                                continue;
                            path.push_back(next);
                            heading = tried;
                            moved = true;
                            break;
                        }
                        if (!moved)
                            break;
                        along += k_step;
                        alongs.push_back(along);
                        if (forkAt > 0.0f && along >= forkAt)
                        {
                            forkAt = -1.0f;
                            inlandSpokes.push_back({
                                .from = path.back(),
                                .fromAlong = along,
                                .heading = heading + random.side() * (0.5f + 0.4f * random.next()),
                                .endAlong = along + (spoke.endAlong - along) * 0.7f,
                                .forks = false
                            });
                        }
                    }
                    lightSpoke(path, alongs);
                }
            }
        }

        // **Each town reaches for the two others nearest it**, within k_corridorReach and past a
        // 20th of a degree, and a pair is joined once. A corridor steers toward the other town,
        // turning aside from the water, and lights only once it arrives.
        void lightCorridors(const LightBuild& build)
        {
            const std::span<const LightTown> towns = build.towns;
            const auto bucketOf = [](const FloatPoint lonLat) {
                return static_cast<std::int64_t>(std::floor(lonLat.y)) * 1000
                    + static_cast<std::int64_t>(std::floor(lonLat.x));
            };
            std::unordered_map<std::int64_t, std::vector<std::size_t>> buckets;
            for (std::size_t index = 0; index != towns.size(); ++index)
                buckets[bucketOf(towns[index].lonLat)].push_back(index);
            std::set<std::pair<std::size_t, std::size_t>> pairs;
            const int bucketReach = static_cast<int>(std::ceil(k_corridorReach)) + 1;
            for (std::size_t index = 0; index != towns.size(); ++index)
            {
                const FloatPoint here = towns[index].lonLat;
                std::array<std::pair<float, std::size_t>, 2> nearestTwo{ {
                    { k_corridorReach, towns.size() },
                    { k_corridorReach, towns.size() }
                } };
                const int latitude = static_cast<int>(std::floor(here.y));
                const int longitude = static_cast<int>(std::floor(here.x));
                for (int north = -bucketReach; north <= bucketReach; ++north)
                {
                    for (int east = -bucketReach * 2; east <= bucketReach * 2; ++east)
                    {
                        const auto found = buckets.find(static_cast<std::int64_t>(latitude + north)
                            * 1000 + longitude + east);
                        if (found == buckets.end())
                            continue;
                        for (const std::size_t other : found->second)
                        {
                            if (other == index)
                                continue;
                            const float distance = arcLength(arcBetween(here, towns[other].lonLat));
                            if (distance >= nearestTwo[1].first || distance <= 0.05f)
                                continue;
                            nearestTwo[1] = { distance, other };
                            if (nearestTwo[1].first < nearestTwo[0].first)
                                std::swap(nearestTwo[0], nearestTwo[1]);
                        }
                    }
                }
                for (const auto& [distance, other] : nearestTwo)
                {
                    if (other != towns.size())
                        pairs.insert({ std::min(index, other), std::max(index, other) });
                }
            }
            constexpr float k_step{ 0.025f };
            std::vector<FloatPoint> path;
            for (const auto& [first, second] : pairs)
            {
                HashRandom random{ static_cast<std::uint32_t>(first * 7 + second * 131 + 5) };
                const FloatPoint target = towns[second].lonLat;
                const FloatPoint toTarget = arcBetween(towns[first].lonLat, target);
                float heading = std::atan2(toTarget.y, toTarget.x) + 0.3f * random.wobble();
                const int stepLimit = static_cast<int>(3.0f * arcLength(toTarget) / k_step);
                path.assign(1, towns[first].lonLat);
                bool arrived = false;
                for (int step = 0; step < stepLimit; ++step)
                {
                    const FloatPoint left = arcBetween(path.back(), target);
                    if (arcLength(left) < k_step * 1.5f)
                    {
                        path.push_back(target);
                        arrived = true;
                        break;
                    }
                    const float wanted = std::atan2(left.y, left.x);
                    heading += std::clamp(std::remainder(wanted - heading, 2.0f * k_pi), -0.12f,
                        0.12f) + 0.05f * random.wobble();
                    bool moved = false;
                    for (const float aside : { 0.0f, 0.35f, -0.35f, 0.7f, -0.7f })
                    {
                        const FloatPoint next = movedOn(path.back(), {
                            k_step * std::cos(heading + aside),
                            k_step * std::sin(heading + aside)
                        });
                        if (!build.land.holds(next))
                            continue;
                        path.push_back(next);
                        heading += aside;
                        moved = true;
                        break;
                    }
                    if (!moved)
                        break;
                }
                if (!arrived)
                    continue;
                const float light = k_corridorLight * (0.15f
                    + 2.0f * std::sqrt(towns[first].size * towns[second].size));
                lightAlong(build.field, path, [&](float) {
                    return light;
                });
            }
        }

        // **A texel of the land may hold a village**, by a chance that is the map's weight for a
        // town there times the towns' nearness squared: each town spreads a nearness of 0.15 and
        // its size, fading over 0.6 degrees and 1.2 more by its size. A village is a small star.
        void lightVillages(const LightBuild& build)
        {
            std::vector<float> nearness(static_cast<std::size_t>(k_nearnessRows)
                * k_nearnessColumns, 0.0f);
            for (const LightTown& town : build.towns)
            {
                const float radius = 0.6f + 1.2f * town.size;
                const float amount = 0.15f + town.size;
                const int centerRow = static_cast<int>((k_north - town.lonLat.y) / k_nearnessCell);
                const int centerColumn = static_cast<int>((town.lonLat.x + 180.0f)
                    / k_nearnessCell);
                const int cellsAcross = static_cast<int>(std::ceil(radius * 2.5f / k_nearnessCell));
                const float cosLatitude = std::max(std::cos(town.lonLat.y * k_degree), 0.1f);
                for (int north = -cellsAcross; north <= cellsAcross; ++north)
                {
                    const int row = centerRow + north;
                    if (row < 0 || row >= k_nearnessRows)
                        continue;
                    const int columnsAcross = static_cast<int>(std::ceil(
                        static_cast<float>(cellsAcross) / cosLatitude));
                    for (int east = -columnsAcross; east <= columnsAcross; ++east)
                    {
                        const int column = ((centerColumn + east) % k_nearnessColumns
                            + k_nearnessColumns) % k_nearnessColumns;
                        const float eastArc = static_cast<float>(east) * k_nearnessCell
                            * cosLatitude;
                        const float northArc = static_cast<float>(north) * k_nearnessCell;
                        const float squared = (eastArc * eastArc + northArc * northArc)
                            / (radius * radius);
                        nearness[static_cast<std::size_t>(row) * k_nearnessColumns
                            + static_cast<std::size_t>(column)] += amount * std::exp(-squared);
                    }
                }
            }
            const auto nearnessAt = [&](const float longitude, const float latitude) {
                const float across = (longitude + 180.0f) / k_nearnessCell - 0.5f;
                const float down = (k_north - latitude) / k_nearnessCell - 0.5f;
                const int column = static_cast<int>(std::floor(across));
                const int row = std::clamp(static_cast<int>(std::floor(down)), 0,
                    k_nearnessRows - 2);
                const float acrossShare = across - static_cast<float>(column);
                const float downShare = std::clamp(down - static_cast<float>(row), 0.0f, 1.0f);
                const auto at = [&](int atRow, int atColumn) {
                    atColumn = (atColumn % k_nearnessColumns + k_nearnessColumns)
                        % k_nearnessColumns;
                    return nearness[static_cast<std::size_t>(atRow) * k_nearnessColumns
                        + static_cast<std::size_t>(atColumn)];
                };
                return std::lerp(std::lerp(at(row, column), at(row, column + 1), acrossShare),
                    std::lerp(at(row + 1, column), at(row + 1, column + 1), acrossShare),
                    downShare);
            };

            // Read from the map 8 texels at a time, for the ground of a texel about each.
            std::array<float, 8> longitudes{};
            std::array<float, 8> latitudes{};
            std::array<float, 8> spans{};
            spans.fill(k_texelDegrees * k_degree);
            std::array<float, 8> heights{};
            std::array<float, 8> slopesEast{};
            std::array<float, 8> slopesNorth{};
            std::array<float, 8> variation{};
            std::array<float, 8> dryness{};
            std::array<float, 8> forest{};
            std::array<float, 8> tundra{};
            std::array<std::uint32_t, 8> texelIndexes{};
            std::vector<FloatPoint> villages;
            std::size_t gathered = 0;
            const auto readGathered = [&](const float latitude) {
                for (std::size_t lane = gathered; lane != 8; ++lane)
                {
                    longitudes[lane] = longitudes[0];
                    latitudes[lane] = latitudes[0];
                }
                build.map.readLand(MapQuery{
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
                for (std::size_t lane = 0; lane != gathered; ++lane)
                {
                    const float weight = (1.0f - smoothStep(0.06f, 0.3f, heights[lane]))
                        * (1.0f - 0.95f * smoothStep(0.35f, 0.75f, dryness[lane]))
                        * (1.0f - smoothStep(0.2f, 0.7f, tundra[lane]))
                        * (1.0f - 0.6f * forest[lane]);
                    const float longitude = longitudes[lane] / k_degree;
                    const float near = nearnessAt(longitude, latitude);
                    const float chance = std::min(0.6f, k_villageChance * weight * near * near);
                    const std::uint32_t bits = mixedBits(texelIndexes[lane] * 0x9E3779B1u
                        ^ 0x5BD1E995u);
                    if (unitOf(bits) >= chance)
                        continue;
                    villages.push_back({
                        longitude + (unitOf(mixedBits(bits ^ 1u)) - 0.5f) * k_texelDegrees,
                        latitude + (unitOf(mixedBits(bits ^ 2u)) - 0.5f) * k_texelDegrees
                    });
                }
                gathered = 0;
            };
            const int firstRow = static_cast<int>((k_north - k_villageNorth) / k_texelDegrees);
            const int endRow = static_cast<int>((k_north - k_villageSouth) / k_texelDegrees);
            for (int row = firstRow; row < endRow; ++row)
            {
                const float latitude = k_north - (static_cast<float>(row) + 0.5f) * k_texelDegrees;
                for (int column = 0; column != k_columns; ++column)
                {
                    if (!build.land.holds(row, column))
                        continue;
                    longitudes[gathered] = (-180.0f + (static_cast<float>(column) + 0.5f)
                        * k_texelDegrees) * k_degree;
                    latitudes[gathered] = latitude * k_degree;
                    texelIndexes[gathered] = static_cast<std::uint32_t>(row) * k_columns
                        + static_cast<std::uint32_t>(column);
                    if (++gathered == 8)
                        readGathered(latitude);
                }
                if (gathered != 0)
                    readGathered(latitude);
            }
            HashRandom random{ 77u };
            for (const FloatPoint village : villages)
            {
                const float size = 0.06f * std::pow(random.next(), 3.0f);
                lightStar(build, village, size, k_villageLight * (0.2f + 0.8f * random.next()),
                    random);
            }
        }

        // **The noise has three octaves**, 0.4, 0.15 and 0.06 degrees across, and the light is
        // multiplied by 0.15 and 1.7 times its square, so it gathers into clumps and thins
        // between them.
        void clumpAndCut(LightField& field, const LandGrid& land)
        {
            for (int tileRow = 0; tileRow != field.tilesDown(); ++tileRow)
            {
                for (int tileColumn = 0; tileColumn != field.tilesAcross(); ++tileColumn)
                {
                    const std::span<float> tile = field.tileAt(tileRow, tileColumn);
                    for (std::size_t texel = 0; texel != tile.size(); ++texel)
                    {
                        if (tile[texel] == 0.0f)
                            continue;
                        const int row = tileRow * k_tileSide + static_cast<int>(texel) / k_tileSide;
                        const int column = tileColumn * k_tileSide
                            + static_cast<int>(texel) % k_tileSide;
                        if (!land.holds(row, column))
                        {
                            tile[texel] = 0.0f;
                            continue;
                        }
                        const float latitude = k_north - (static_cast<float>(row) + 0.5f)
                            * k_texelDegrees;
                        const float longitude = -180.0f + (static_cast<float>(column) + 0.5f)
                            * k_texelDegrees;
                        const float across = longitude * std::max(std::cos(latitude * k_degree),
                            0.05f);
                        const float noise = 0.4f * planeNoise(across / 0.4f, latitude / 0.4f, 11u)
                            + 0.35f * planeNoise(across / 0.15f, latitude / 0.15f, 12u)
                            + 0.25f * planeNoise(across / 0.06f, latitude / 0.06f, 13u);
                        tile[texel] *= 0.15f + 1.7f * noise * noise;
                    }
                }
            }
        }

        float planeNoise(const float x, const float y, const std::uint32_t seed)
        {
            const float floorX = std::floor(x);
            const float floorY = std::floor(y);
            const float acrossShare = x - floorX;
            const float downShare = y - floorY;
            const float smoothAcross = acrossShare * acrossShare * (3.0f - 2.0f * acrossShare);
            const float smoothDown = downShare * downShare * (3.0f - 2.0f * downShare);
            const auto corner = [&](int east, int north) {
                const std::uint32_t column = static_cast<std::uint32_t>(
                    static_cast<int>(floorX) + east);
                const std::uint32_t row = static_cast<std::uint32_t>(
                    static_cast<int>(floorY) + north);
                return unitOf(mixedBits(column * 0x9E3779B1u
                    ^ mixedBits(row * 0x85EBCA77u ^ seed)));
            };
            return std::lerp(std::lerp(corner(0, 0), corner(1, 0), smoothAcross),
                std::lerp(corner(0, 1), corner(1, 1), smoothAcross), smoothDown);
        }

        __m256 lightAt(const KeptLight& kept, const int level, const __m256i rows,
            const __m256i columns)
        {
            const LightLevel& texels = kept.levels[static_cast<std::size_t>(level)];
            const int levelRows = k_rows >> level;
            const int levelColumns = k_columns >> level;
            const __m256i inside = _mm256_andnot_si256(_mm256_cmpgt_epi32(_mm256_setzero_si256(),
                rows), _mm256_cmpgt_epi32(_mm256_set1_epi32(levelRows), rows));
            const __m256i heldRows = _mm256_and_si256(rows, inside);
            const __m256i wrappedColumns = _mm256_and_si256(columns,
                _mm256_set1_epi32(levelColumns - 1));
            const __m256i tile = _mm256_add_epi32(_mm256_set1_epi32(texels.firstTile),
                _mm256_add_epi32(_mm256_mullo_epi32(_mm256_srli_epi32(heldRows, 4),
                    _mm256_set1_epi32(texels.tilesAcross)), _mm256_srli_epi32(wrappedColumns, 4)));
            const __m256i tileStart = _mm256_i32gather_epi32(kept.tileStarts, tile, 4);
            const __m256i texel = _mm256_add_epi32(tileStart, _mm256_add_epi32(
                _mm256_slli_epi32(_mm256_and_si256(heldRows, _mm256_set1_epi32(15)), 4),
                _mm256_and_si256(wrappedColumns, _mm256_set1_epi32(15))));
            const __m256i bytes = _mm256_and_si256(_mm256_and_si256(_mm256_i32gather_epi32(
                reinterpret_cast<const int*>(kept.texels), texel, 1), _mm256_set1_epi32(0xFF)),
                inside);
            const __m256 root = _mm256_mul_ps(_mm256_cvtepi32_ps(bytes),
                _mm256_set1_ps(1.0f / 255.0f));
            return _mm256_mul_ps(_mm256_mul_ps(_mm256_mul_ps(root, root), root),
                _mm256_set1_ps(k_brightestLight));
        }

        std::array<__m256, 2> texelPlacesAt(const int level, const __m256 longitudes,
            const __m256 latitudes)
        {
            const __m256 perTexel = _mm256_set1_ps(1.0f / (k_texelDegrees
                * static_cast<float>(1 << level)));
            return {
                _mm256_mul_ps(_mm256_add_ps(longitudes, _mm256_set1_ps(180.0f)), perTexel),
                _mm256_mul_ps(_mm256_sub_ps(_mm256_set1_ps(k_north), latitudes), perTexel)
            };
        }

        __m256 blendedLightAt(const KeptLight& kept, const int level, const __m256 longitudes,
            const __m256 latitudes)
        {
            const std::array<__m256, 2> places = texelPlacesAt(level, longitudes, latitudes);
            const __m256 across = _mm256_sub_ps(places[0], _mm256_set1_ps(0.5f));
            const __m256 down = _mm256_sub_ps(places[1], _mm256_set1_ps(0.5f));
            const __m256 leftColumn = _mm256_floor_ps(across);
            const __m256 topRow = _mm256_floor_ps(down);
            const __m256i left = _mm256_cvttps_epi32(leftColumn);
            const __m256i top = _mm256_cvttps_epi32(topRow);
            const __m256i right = _mm256_add_epi32(left, _mm256_set1_epi32(1));
            const __m256i bottom = _mm256_add_epi32(top, _mm256_set1_epi32(1));
            const __m256 acrossShare = _mm256_sub_ps(across, leftColumn);
            const __m256 upperLeft = lightAt(kept, level, top, left);
            const __m256 upper = _mm256_fmadd_ps(_mm256_sub_ps(lightAt(kept, level, top, right),
                upperLeft), acrossShare, upperLeft);
            const __m256 lowerLeft = lightAt(kept, level, bottom, left);
            const __m256 lower = _mm256_fmadd_ps(_mm256_sub_ps(lightAt(kept, level, bottom, right),
                lowerLeft), acrossShare, lowerLeft);
            return _mm256_fmadd_ps(_mm256_sub_ps(lower, upper), _mm256_sub_ps(down, topRow), upper);
        }

        __m256 nearestLightAt(const KeptLight& kept, const int level, const __m256 longitudes,
            const __m256 latitudes)
        {
            const std::array<__m256, 2> places = texelPlacesAt(level, longitudes, latitudes);
            return lightAt(kept, level, _mm256_cvttps_epi32(_mm256_floor_ps(places[1])),
                _mm256_cvttps_epi32(_mm256_floor_ps(places[0])));
        }

        // **A cell's row and column are counted from the equator and the 0th meridian**, the
        // columns a row's cosine apart, so the cells stand about square; the columns west of it
        // are counted on from the east's, as many as the circle of latitude holds. The two rows
        // and the two columns nearest each pixel are read. A cell's hash places its light and
        // draws whether it stands and how bright, and another draws when it switches on.
        __m256 lightsOfLevel(const KeptLight& kept, const int cellLevel, const PointLanes& points,
            const __m256 inverseFootprints)
        {
            const __m256 zero = _mm256_setzero_ps();
            const __m256 one = _mm256_set1_ps(1.0f);
            const __m256 half = _mm256_set1_ps(0.5f);
            const float cellSize = k_finestCell * static_cast<float>(1 << cellLevel);
            const int lightLevel = std::clamp(cellLevel - k_cellLevelsPerTexel, 0,
                k_lightLevels - 1);
            const __m256 cell = _mm256_set1_ps(cellSize);
            const __m256 perCell = _mm256_set1_ps(1.0f / cellSize);
            const __m256 cellsPerPixel = _mm256_mul_ps(cell, inverseFootprints);
            const __m256 chancePerLight = _mm256_mul_ps(_mm256_mul_ps(cellsPerPixel, cellsPerPixel),
                _mm256_set1_ps(1.0f / (k_tentVolume * k_meanLight)));
            const __m256 rowPlace = _mm256_mul_ps(points.latitudes, perCell);
            const __m256 rowFloor = _mm256_floor_ps(rowPlace);
            const __m256 rowStep = _mm256_blendv_ps(one, _mm256_set1_ps(-1.0f),
                _mm256_cmp_ps(_mm256_sub_ps(rowPlace, rowFloor), half, _CMP_LT_OQ));
            const std::array<__m256, 2> rows = { rowFloor, _mm256_add_ps(rowFloor, rowStep) };
            __m256 sum = zero;
            for (const __m256 row : rows)
            {
                const __m256 rowLatitude = _mm256_min_ps(_mm256_max_ps(_mm256_mul_ps(
                    _mm256_add_ps(row, half), cell), _mm256_set1_ps(-90.0f)),
                    _mm256_set1_ps(90.0f));
                const __m256 rowCosine = _mm256_max_ps(quickCosine(_mm256_mul_ps(rowLatitude,
                    _mm256_set1_ps(k_degree))), _mm256_set1_ps(0.02f));
                const __m256 perRowCosine = _mm256_div_ps(one, rowCosine);
                const __m256 across = _mm256_mul_ps(points.longitudes, rowCosine);
                const __m256 columnPlace = _mm256_mul_ps(across, perCell);
                const __m256 columnFloor = _mm256_floor_ps(columnPlace);
                const __m256 columnStep = _mm256_blendv_ps(one, _mm256_set1_ps(-1.0f),
                    _mm256_cmp_ps(_mm256_sub_ps(columnPlace, columnFloor), half, _CMP_LT_OQ));
                const __m256 cellsRound = _mm256_ceil_ps(_mm256_mul_ps(rowCosine,
                    _mm256_mul_ps(perCell, _mm256_set1_ps(360.0f))));
                const __m256i rowBits = mixedLanes(_mm256_xor_si256(_mm256_mullo_epi32(
                    _mm256_cvttps_epi32(row), _mm256_set1_epi32(static_cast<int>(0x85EBCA77u))),
                    _mm256_set1_epi32(static_cast<int>(static_cast<std::uint32_t>(cellLevel)
                        * 0xC2B2AE3Du))));
                const std::array<__m256, 2> columns = {
                    columnFloor,
                    _mm256_add_ps(columnFloor, columnStep)
                };
                for (const __m256 column : columns)
                {
                    const __m256 wrapped = _mm256_add_ps(column, _mm256_and_ps(cellsRound,
                        _mm256_cmp_ps(column, zero, _CMP_LT_OQ)));
                    const __m256i spot = mixedLanes(_mm256_xor_si256(_mm256_mullo_epi32(
                        _mm256_cvttps_epi32(wrapped),
                        _mm256_set1_epi32(static_cast<int>(0x9E3779B1u))), rowBits));
                    const std::array<__m256, 2> spotPlace = unitPairLanes(spot);
                    const __m256 spread = _mm256_set1_ps(0.8f);
                    const __m256 margin = _mm256_set1_ps(0.1f);
                    const __m256 pointAcross = _mm256_mul_ps(_mm256_add_ps(column,
                        _mm256_fmadd_ps(spotPlace[0], spread, margin)), cell);
                    const __m256 pointLatitude = _mm256_mul_ps(_mm256_add_ps(row,
                        _mm256_fmadd_ps(spotPlace[1], spread, margin)), cell);
                    const __m256 offsetAcross = _mm256_mul_ps(_mm256_sub_ps(across, pointAcross),
                        inverseFootprints);
                    const __m256 offsetDown = _mm256_mul_ps(_mm256_sub_ps(points.latitudes,
                        pointLatitude), inverseFootprints);
                    const __m256 squaredOffset = _mm256_fmadd_ps(offsetAcross, offsetAcross,
                        _mm256_mul_ps(offsetDown, offsetDown));
                    if (_mm256_movemask_ps(_mm256_cmp_ps(squaredOffset, one, _CMP_LT_OQ)) == 0)
                        continue;
                    const __m256 light = nearestLightAt(kept, lightLevel,
                        _mm256_mul_ps(pointAcross, perRowCosine), pointLatitude);
                    const __m256 chance = _mm256_mul_ps(light, chancePerLight);
                    const __m256i drawn = mixedLanes(_mm256_xor_si256(spot,
                        _mm256_set1_epi32(static_cast<int>(0xB5297A4Du))));
                    const __m256 standing = _mm256_cmp_ps(unitLanes(drawn), chance, _CMP_LT_OQ);
                    if (_mm256_movemask_ps(standing) == 0)
                        continue;
                    const __m256i switchBits = mixedLanes(_mm256_xor_si256(drawn,
                        _mm256_set1_epi32(static_cast<int>(0xE6546B64u))));
                    const __m256 switchAt = _mm256_mul_ps(unitLanes(switchBits),
                        _mm256_fnmadd_ps(_mm256_set1_ps(k_denseHeadStart), _mm256_min_ps(one,
                            _mm256_mul_ps(chance, half)), one));
                    const __m256 switchedOn = smoothLanes(switchAt, k_switchingDarkness,
                        points.darkness);
                    const __m256 draw = byteUnitLanes(drawn);
                    const __m256 drawSquared = _mm256_mul_ps(draw, draw);
                    const __m256 brightness = _mm256_fmadd_ps(_mm256_mul_ps(_mm256_mul_ps(
                        drawSquared, drawSquared), drawSquared), _mm256_set1_ps(k_lightRise),
                        _mm256_set1_ps(k_dimmestLight));
                    const __m256 tent = _mm256_max_ps(zero, _mm256_sub_ps(one,
                        _mm256_sqrt_ps(squaredOffset)));
                    const __m256 laid = _mm256_mul_ps(_mm256_mul_ps(brightness,
                        _mm256_max_ps(one, chance)), _mm256_mul_ps(tent, switchedOn));
                    sum = _mm256_add_ps(sum, _mm256_and_ps(laid, standing));
                }
            }
            return sum;
        }

        float finestLevelOf(const __m256 levels, const __m256 dark)
        {
            alignas(32) std::array<float, 8> lanes{};
            _mm256_store_ps(lanes.data(), _mm256_blendv_ps(
                _mm256_set1_ps(std::numeric_limits<float>::max()), levels, dark));
            return std::max(0.0f, std::floor(*std::ranges::min_element(lanes)));
        }

        // The longitudes are brought within half a turn of the 0th meridian.
        PointLanes pointLanesOf(const std::array<const float*, 4>& from)
        {
            const __m256 turns = _mm256_mul_ps(_mm256_loadu_ps(from[0]),
                _mm256_set1_ps(1.0f / (2.0f * k_pi)));
            const __m256 darkness = _mm256_loadu_ps(from[3]);
            return {
                .longitudes = _mm256_mul_ps(_mm256_sub_ps(turns, _mm256_floor_ps(
                    _mm256_add_ps(turns, _mm256_set1_ps(0.5f)))), _mm256_set1_ps(360.0f)),
                .latitudes = _mm256_mul_ps(_mm256_loadu_ps(from[1]),
                    _mm256_set1_ps(1.0f / k_degree)),
                .footprints = _mm256_mul_ps(_mm256_loadu_ps(from[2]),
                    _mm256_set1_ps(1.0f / k_degree)),
                .darkness = darkness,
                .dark = _mm256_cmp_ps(darkness, _mm256_setzero_ps(), _CMP_GT_OQ)
            };
        }

        void readByEight(const LightQuery& query, const std::span<float> values,
            const std::function<__m256(const std::array<const float*, 4>&)>& read)
        {
            const std::array<std::span<const float>, 4> queried = {
                query.longitudes, query.latitudes, query.footprints, query.darkness
            };
            const std::size_t count = values.size();
            for (std::size_t first = 0; first < count; first += 8)
            {
                const std::size_t lanesUsed = std::min<std::size_t>(8, count - first);
                if (lanesUsed == 8)
                {
                    _mm256_storeu_ps(values.data() + first, read({
                        queried[0].data() + first, queried[1].data() + first,
                        queried[2].data() + first, queried[3].data() + first
                    }));
                    continue;
                }
                std::array<std::array<float, 8>, 4> padded{};
                for (std::size_t value = 0; value != 4; ++value)
                {
                    for (std::size_t lane = 0; lane != 8; ++lane)
                    {
                        padded[value][lane] = lane < lanesUsed || value != 3
                            ? queried[value][first + std::min(lane, lanesUsed - 1)]
                            : 0.0f;
                    }
                }
                std::array<float, 8> paddedValues{};
                _mm256_storeu_ps(paddedValues.data(), read({
                    padded[0].data(), padded[1].data(), padded[2].data(), padded[3].data()
                }));
                std::copy_n(paddedValues.begin(), lanesUsed,
                    values.begin() + static_cast<std::ptrdiff_t>(first));
            }
        }

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

        // Taylor's series to the 8th power.
        __m256 quickCosine(const __m256 angles)
        {
            const __m256 square = _mm256_mul_ps(angles, angles);
            __m256 series = _mm256_set1_ps(1.0f / 40320.0f);
            series = _mm256_fmadd_ps(series, square, _mm256_set1_ps(-1.0f / 720.0f));
            series = _mm256_fmadd_ps(series, square, _mm256_set1_ps(1.0f / 24.0f));
            series = _mm256_fmadd_ps(series, square, _mm256_set1_ps(-0.5f));
            return _mm256_fmadd_ps(series, square, _mm256_set1_ps(1.0f));
        }

        // mixedBits in each lane.
        __m256i mixedLanes(__m256i values)
        {
            values = _mm256_xor_si256(values, _mm256_srli_epi32(values, 16));
            values = _mm256_mullo_epi32(values, _mm256_set1_epi32(0x7FEB352D));
            values = _mm256_xor_si256(values, _mm256_srli_epi32(values, 15));
            values = _mm256_mullo_epi32(values,
                _mm256_set1_epi32(static_cast<int>(0x846CA68Bu)));
            return _mm256_xor_si256(values, _mm256_srli_epi32(values, 16));
        }

        // unitOf in each lane.
        __m256 unitLanes(const __m256i bits)
        {
            return _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_srli_epi32(bits, 8)),
                _mm256_set1_ps(1.0f / 16777216.0f));
        }

        std::array<__m256, 2> unitPairLanes(const __m256i bits)
        {
            const __m256 perStep = _mm256_set1_ps(1.0f / 65536.0f);
            return {
                _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_srli_epi32(bits, 16)), perStep),
                _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_and_si256(bits,
                    _mm256_set1_epi32(0xFFFF))), perStep)
            };
        }

        // The middle of each of the 256 steps.
        __m256 byteUnitLanes(const __m256i bits)
        {
            return _mm256_fmadd_ps(_mm256_cvtepi32_ps(_mm256_and_si256(bits,
                _mm256_set1_epi32(0xFF))), _mm256_set1_ps(1.0f / 256.0f),
                _mm256_set1_ps(0.5f / 256.0f));
        }

        __m256 smoothLanes(const __m256 edge0, const float width, const __m256 values)
        {
            const __m256 share = _mm256_min_ps(_mm256_max_ps(_mm256_mul_ps(
                _mm256_sub_ps(values, edge0), _mm256_set1_ps(1.0f / width)), _mm256_setzero_ps()),
                _mm256_set1_ps(1.0f));
            return _mm256_mul_ps(_mm256_mul_ps(share, share),
                _mm256_fnmadd_ps(_mm256_set1_ps(2.0f), share, _mm256_set1_ps(3.0f)));
        }
    }
}
