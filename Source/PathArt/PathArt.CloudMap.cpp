module;
#include <immintrin.h>
module ClaFi.PathArt.CloudMap;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    namespace
    {
        constexpr float k_pi{ std::numbers::pi_v<float> };
        // How much finer each octave of the noise is than the one before.
        constexpr float k_lacunarity{ 2.03f };

        // A point in space: on the unit sphere where the noise follows the sphere.
        struct SpacePoint
        {
            float x;
            float y;
            float z;
        };

        // The point `share` of the way from `from` to `to`; std::lerp's end checks cost the noise
        // half its time.
        [[nodiscard]] constexpr float blend(const float from, const float to, const float share)
        {
            return from + (to - from) * share;
        }
        // The noise's value at a corner of its lattice, 0 to 1, the same for one corner and seed.
        [[nodiscard]] float latticeValue(int x, int y, int z, std::uint32_t seed);
        // Value noise: the lattice's values blended smoothly between its corners.
        [[nodiscard]] float valueNoise(SpacePoint, std::uint32_t seed);
        // Octaves of value noise, each finer and fainter by `gain`, as a share of their sum.
        [[nodiscard]] float fractalNoise(SpacePoint, std::uint32_t seed, int octaves,
            float gain = 0.5f);
        // The noise a kind's shapes are cut from, at a texel. See PathArt#cloudkind
        [[nodiscard]] float shapeNoise(CloudKind, SpacePoint onSphere, float longitude,
            float latitude, std::uint32_t seed);
        // How far above the cut the noise rises before a kind's cloud is whole.
        [[nodiscard]] float edgeSoftness(CloudKind);
        // How a kind's cloud is shaded inside its shape, 1 where it is not.
        [[nodiscard]] float innerShade(CloudKind, SpacePoint onSphere, std::uint32_t seed);
        // The point on the unit sphere at a longitude and latitude, in radians.
        [[nodiscard]] SpacePoint onSphereAt(float longitude, float latitude);
        // About the base-2 logarithm of each positive value, within a hundredth.
        [[nodiscard]] __m256 quickLog2(__m256 values);
    }

    // **The share covered is held exactly**: the cut is the noise's quantile for it, so a kind
    // whose noise runs high is not cloudier for that. Each coarser level averages the 2 by 2
    // texels under each of its own.
    CloudMap::CloudMap(const CloudKind kind, const std::uint32_t seed, const float cover)
    {
        const std::size_t texelCount = static_cast<std::size_t>(k_width) * k_height;
        const auto longitudeAt = [](int column) {
            return (static_cast<float>(column) + 0.5f) / k_width * 2.0f * k_pi;
        };
        const auto latitudeAt = [](int row) {
            return k_pi / 2.0f - (static_cast<float>(row) + 0.5f) / k_height * k_pi;
        };

        std::vector<float> noise(texelCount);
        spreadOverCores(k_height, [&](int row) {
            const float latitude = latitudeAt(row);
            float* rowNoise = noise.data() + static_cast<std::size_t>(row) * k_width;
            for (int column = 0; column != k_width; ++column)
            {
                const float longitude = longitudeAt(column);
                rowNoise[column] = shapeNoise(kind, onSphereAt(longitude, latitude), longitude,
                    latitude, seed);
            }
        });

        std::vector<float> sorted = noise;
        const auto cutIndex = static_cast<std::ptrdiff_t>(
            std::round((1.0f - cover) * static_cast<float>(texelCount - 1)));
        std::ranges::nth_element(sorted, sorted.begin() + cutIndex);
        const float cut = sorted[static_cast<std::size_t>(cutIndex)];
        const float softness = edgeSoftness(kind);

        std::size_t texels = 0;
        for (std::size_t level = 0; level != m_levelStarts.size(); ++level)
        {
            m_levelStarts[level] = static_cast<int>(texels);
            texels += static_cast<std::size_t>((k_width >> level) + 1)
                * static_cast<std::size_t>(k_height >> level);
        }
        m_cover.resize(texels + k_readPadding);
        constexpr std::size_t k_finestRow{ k_width + 1 };
        spreadOverCores(k_height, [&](int row) {
            const float latitude = latitudeAt(row);
            const float* rowNoise = noise.data() + static_cast<std::size_t>(row) * k_width;
            std::uint8_t* rowCover = m_cover.data() + static_cast<std::size_t>(row) * k_finestRow;
            for (int column = 0; column != k_width; ++column)
            {
                const float shape = smoothStep(cut, cut + softness, rowNoise[column]);
                if (shape <= 0.0f)
                {
                    rowCover[column] = 0;
                    continue;
                }
                const SpacePoint onSphere = onSphereAt(longitudeAt(column), latitude);
                const float shaded = std::clamp(shape * innerShade(kind, onSphere, seed),
                    0.0f, 1.0f);
                rowCover[column] = static_cast<std::uint8_t>(std::lround(shaded * 255.0f));
            }
            rowCover[k_width] = rowCover[0];
        });

        for (std::size_t level = 1; level != m_levelStarts.size(); ++level)
        {
            const std::size_t width = static_cast<std::size_t>(k_width >> level);
            const std::uint8_t* finer = m_cover.data() + m_levelStarts[level - 1];
            std::uint8_t* coarser = m_cover.data() + m_levelStarts[level];
            spreadOverCores(k_height >> level, [&](int row) {
                const std::uint8_t* upper = finer
                    + 2 * static_cast<std::size_t>(row) * (2 * width + 1);
                const std::uint8_t* lower = upper + 2 * width + 1;
                std::uint8_t* rowCover = coarser + static_cast<std::size_t>(row) * (width + 1);
                for (std::size_t column = 0; column != width; ++column)
                {
                    const int sum = upper[2 * column] + upper[2 * column + 1] + lower[2 * column]
                        + lower[2 * column + 1];
                    rowCover[column] = static_cast<std::uint8_t>((sum + 2) / 4);
                }
                rowCover[width] = rowCover[0];
            });
        }
    }

    // **Eight points at a time**, each read between the two levels nearest the ground it stands
    // for. Each level wraps round the longitudes and stops at the poles, and between texel
    // centers the cover is blended from the four nearest: one 4-byte read per row of texels holds
    // the texel and its right neighbour side by side, the last column's included. A last group
    // short of eight is read from copies padded with its last point. See PathArt#cloudmap
    void CloudMap::readCover(const MapQuery& query, const std::span<float> covers) const
    {
        constexpr float k_columnsPerRadian{ k_width / (2.0f * k_pi) };
        constexpr float k_rowsPerRadian{ k_height / k_pi };
        const __m256 half = _mm256_set1_ps(0.5f);
        const __m256i one = _mm256_set1_epi32(1);
        const __m256i byteMask = _mm256_set1_epi32(0xFF);
        const __m256i levelStarts = _mm256_loadu_si256(
            reinterpret_cast<const __m256i*>(m_levelStarts.data()));
        const int* readBase = reinterpret_cast<const int*>(m_cover.data());
        // The pair at the left column of a row: the left texel in the low byte, the right above.
        const auto blendPair = [&](__m256i pair, __m256 acrossShare) {
            const __m256 leftTexel = _mm256_cvtepi32_ps(_mm256_and_si256(pair, byteMask));
            const __m256 rightTexel = _mm256_cvtepi32_ps(
                _mm256_and_si256(_mm256_srli_epi32(pair, 8), byteMask));
            return _mm256_fmadd_ps(_mm256_sub_ps(rightTexel, leftTexel), acrossShare, leftTexel);
        };
        // The maximum takes its second operand where the first is not a number, so every read
        // stays inside its level.
        const auto coverAt = [&](__m256i level, __m256 longitudes, __m256 latitudes) {
            const __m256i width = _mm256_srlv_epi32(_mm256_set1_epi32(k_width), level);
            const __m256i lastRow = _mm256_sub_epi32(
                _mm256_srlv_epi32(_mm256_set1_epi32(k_height), level), one);
            // Two to the minus level, its exponent laid in the bits.
            const __m256 coarseness = _mm256_castsi256_ps(_mm256_slli_epi32(
                _mm256_sub_epi32(_mm256_set1_epi32(127), level), 23));
            const __m256 across = _mm256_fmsub_ps(longitudes,
                _mm256_mul_ps(coarseness, _mm256_set1_ps(k_columnsPerRadian)), half);
            const __m256 down = _mm256_min_ps(_mm256_max_ps(_mm256_fmsub_ps(
                _mm256_sub_ps(_mm256_set1_ps(k_pi / 2.0f), latitudes),
                _mm256_mul_ps(coarseness, _mm256_set1_ps(k_rowsPerRadian)), half),
                _mm256_setzero_ps()), _mm256_cvtepi32_ps(lastRow));
            const __m256 leftColumn = _mm256_floor_ps(across);
            const __m256 topRow = _mm256_floor_ps(down);
            const __m256i left = _mm256_and_si256(_mm256_cvttps_epi32(leftColumn),
                _mm256_sub_epi32(width, one));
            const __m256i top = _mm256_cvttps_epi32(topRow);
            const __m256i bottom = _mm256_min_epi32(_mm256_add_epi32(top, one), lastRow);
            const __m256i rowLength = _mm256_add_epi32(width, one);
            const __m256i levelStart = _mm256_permutevar8x32_epi32(levelStarts, level);
            const __m256i upperPair = _mm256_i32gather_epi32(readBase, _mm256_add_epi32(levelStart,
                _mm256_add_epi32(_mm256_mullo_epi32(top, rowLength), left)), 1);
            const __m256i lowerPair = _mm256_i32gather_epi32(readBase, _mm256_add_epi32(levelStart,
                _mm256_add_epi32(_mm256_mullo_epi32(bottom, rowLength), left)), 1);
            const __m256 acrossShare = _mm256_sub_ps(across, leftColumn);
            const __m256 upper = blendPair(upperPair, acrossShare);
            const __m256 lower = blendPair(lowerPair, acrossShare);
            return _mm256_fmadd_ps(_mm256_sub_ps(lower, upper), _mm256_sub_ps(down, topRow), upper);
        };
        // A point's ground spans as many texels as the greater of its two spans does.
        const auto read = [&](const std::array<const float*, 4>& from) {
            const __m256 longitudes = _mm256_loadu_ps(from[0]);
            const __m256 latitudes = _mm256_loadu_ps(from[1]);
            const __m256 level = quickLog2(_mm256_max_ps(
                _mm256_mul_ps(_mm256_loadu_ps(from[2]), _mm256_set1_ps(k_columnsPerRadian)),
                _mm256_mul_ps(_mm256_loadu_ps(from[3]), _mm256_set1_ps(k_rowsPerRadian))));
            const __m256 held = _mm256_min_ps(_mm256_max_ps(level, _mm256_setzero_ps()),
                _mm256_set1_ps(static_cast<float>(k_levels - 1)));
            const __m256 finerLevel = _mm256_min_ps(_mm256_floor_ps(held),
                _mm256_set1_ps(static_cast<float>(k_levels - 2)));
            const __m256i finer = _mm256_cvttps_epi32(finerLevel);
            const __m256 coarserShare = _mm256_sub_ps(held, finerLevel);
            const __m256 finerCover = coverAt(finer, longitudes, latitudes);
            if (_mm256_movemask_ps(_mm256_cmp_ps(coarserShare, _mm256_setzero_ps(),
                _CMP_GT_OQ)) == 0)
            {
                return _mm256_mul_ps(finerCover, _mm256_set1_ps(1.0f / 255.0f));
            }
            const __m256 coarserCover = coverAt(_mm256_add_epi32(finer, one), longitudes,
                latitudes);
            return _mm256_mul_ps(_mm256_fmadd_ps(_mm256_sub_ps(coarserCover, finerCover),
                coarserShare, finerCover), _mm256_set1_ps(1.0f / 255.0f));
        };
        const std::array<std::span<const float>, 4> queried = {
            query.longitudes, query.latitudes, query.longitudeSpans, query.latitudeSpans
        };
        const std::size_t count = covers.size();
        for (std::size_t first = 0; first < count; first += 8)
        {
            const std::size_t lanesUsed = std::min<std::size_t>(8, count - first);
            if (lanesUsed == 8)
            {
                _mm256_storeu_ps(covers.data() + first, read({
                    queried[0].data() + first, queried[1].data() + first,
                    queried[2].data() + first, queried[3].data() + first
                }));
                continue;
            }
            std::array<std::array<float, 8>, 4> padded{};
            for (std::size_t value = 0; value != 4; ++value)
            {
                for (std::size_t lane = 0; lane != 8; ++lane)
                    padded[value][lane] = queried[value][first + std::min(lane, lanesUsed - 1)];
            }
            alignas(32) std::array<float, 8> paddedCovers{};
            _mm256_store_ps(paddedCovers.data(), read({
                padded[0].data(), padded[1].data(), padded[2].data(), padded[3].data()
            }));
            std::copy_n(paddedCovers.begin(), lanesUsed,
                covers.begin() + static_cast<std::ptrdiff_t>(first));
        }
    }

    namespace
    {
        // The hash spreads the corner's bits with the multiply and shifts of MurmurHash2.
        float latticeValue(const int x, const int y, const int z, const std::uint32_t seed)
        {
            std::uint32_t hash = static_cast<std::uint32_t>(x) * 73856093u
                ^ static_cast<std::uint32_t>(y) * 19349663u
                ^ static_cast<std::uint32_t>(z) * 83492791u
                ^ seed * 2654435761u;
            hash ^= hash >> 13;
            hash *= 0x5BD1E995u;
            hash ^= hash >> 15;
            return static_cast<float>(hash) * (1.0f / 4294967295.0f);
        }

        float valueNoise(const SpacePoint point, const std::uint32_t seed)
        {
            const float floorX = std::floor(point.x);
            const float floorY = std::floor(point.y);
            const float floorZ = std::floor(point.z);
            const int x = static_cast<int>(floorX);
            const int y = static_cast<int>(floorY);
            const int z = static_cast<int>(floorZ);
            const auto eased = [](float share) {
                return share * share * (3.0f - 2.0f * share);
            };
            const float shareX = eased(point.x - floorX);
            const float shareY = eased(point.y - floorY);
            const float shareZ = eased(point.z - floorZ);
            const auto alongX = [&](int cornerY, int cornerZ) {
                return blend(latticeValue(x, cornerY, cornerZ, seed),
                    latticeValue(x + 1, cornerY, cornerZ, seed), shareX);
            };
            const float near = blend(alongX(y, z), alongX(y + 1, z), shareY);
            const float far = blend(alongX(y, z + 1), alongX(y + 1, z + 1), shareY);
            return blend(near, far, shareZ);
        }

        float fractalNoise(const SpacePoint point, const std::uint32_t seed, const int octaves,
            const float gain)
        {
            float total = 0.0f;
            float amplitude = 1.0f;
            float amplitudeSum = 0.0f;
            float frequency = 1.0f;
            for (int octave = 0; octave != octaves; ++octave)
            {
                const SpacePoint scaled = {
                    point.x * frequency,
                    point.y * frequency,
                    point.z * frequency
                };
                const std::uint32_t octaveSeed = seed + static_cast<std::uint32_t>(octave) * 17u;
                total += amplitude * valueNoise(scaled, octaveSeed);
                amplitudeSum += amplitude;
                amplitude *= gain;
                frequency *= k_lacunarity;
            }
            return total / amplitudeSum;
        }

        // Each kind warps its noise by more noise, so the shapes curl rather than sit on a grid.
        float shapeNoise(const CloudKind kind, const SpacePoint onSphere, const float longitude,
            const float latitude, const std::uint32_t seed)
        {
            const auto [x, y, z] = onSphere;
            switch (kind)
            {
                case CloudKind::Puffy:
                {
                    constexpr float k_scale{ 7.0f };
                    const float warpX = fractalNoise({ 2.0f * x + 3.1f, 2.0f * y, 2.0f * z },
                        seed + 100u, 3);
                    const float warpY = fractalNoise({ 2.0f * x, 2.0f * y + 5.7f, 2.0f * z },
                        seed + 200u, 3);
                    const float heaps = fractalNoise({
                        k_scale * x + 2.5f * warpX,
                        k_scale * y + 2.5f * warpY,
                        k_scale * z
                    }, seed, 6, 0.55f);
                    // The heaps bunch into fields with clear sky between them.
                    const float fields = fractalNoise({ 1.6f * x, 1.6f * y, 1.6f * z },
                        seed + 300u, 3);
                    return heaps * (0.55f + 0.9f * fields);
                }
                case CloudKind::Patchy:
                {
                    constexpr float k_scale{ 2.6f };
                    const float warpX = fractalNoise({ 2.2f * x + 1.3f, 2.2f * y, 2.2f * z },
                        seed + 100u, 4);
                    const float warpY = fractalNoise({ 2.2f * x, 2.2f * y + 4.2f, 2.2f * z },
                        seed + 200u, 4);
                    const float warpZ = fractalNoise({ 2.2f * x, 2.2f * y, 2.2f * z + 2.9f },
                        seed + 250u, 4);
                    return fractalNoise({
                        k_scale * x + 1.8f * warpX,
                        k_scale * y + 1.8f * warpY,
                        k_scale * z + 1.8f * warpZ
                    }, seed, 7, 0.55f);
                }
                case CloudKind::Streaky:
                {
                    // Noise on a cylinder round the axis, four times finer along it than round it.
                    const float roundX = 3.0f * std::cos(longitude);
                    const float roundY = 3.0f * std::sin(longitude);
                    const float along = 12.0f * latitude;
                    const float warp = fractalNoise({ 1.5f * roundX, 1.5f * roundY, 0.4f * along },
                        seed + 100u, 3);
                    const float streaks = fractalNoise({
                        roundX + 1.2f * warp,
                        roundY,
                        along + 3.0f * warp
                    }, seed, 6, 0.6f);
                    // Broad bands where the streaks gather.
                    const float bands = fractalNoise({ 1.2f * x, 1.2f * y, 1.2f * z },
                        seed + 300u, 3);
                    return 0.7f * streaks + 0.45f * bands;
                }
            }
            return 0.0f;
        }

        float edgeSoftness(const CloudKind kind)
        {
            switch (kind)
            {
                case CloudKind::Puffy:
                    return 0.14f;
                case CloudKind::Patchy:
                    return 0.22f;
                case CloudKind::Streaky:
                    return 0.12f;
            }
            return 0.14f;
        }

        // A heap is lit unevenly across its top, in noise much finer than its shape.
        float innerShade(const CloudKind kind, const SpacePoint onSphere,
            const std::uint32_t seed)
        {
            if (kind != CloudKind::Puffy)
                return 1.0f;
            constexpr float k_scale{ 7.0f * 3.5f };
            const float detail = fractalNoise({
                k_scale * onSphere.x,
                k_scale * onSphere.y,
                k_scale * onSphere.z
            }, seed + 400u, 4, 0.6f);
            return 0.62f + 0.5f * detail;
        }

        SpacePoint onSphereAt(const float longitude, const float latitude)
        {
            return {
                std::cos(latitude) * std::sin(longitude),
                std::sin(latitude),
                std::cos(latitude) * std::cos(longitude)
            };
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
    }
}
