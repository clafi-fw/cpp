module;
#include <immintrin.h>
module ClaFi.PathArt.CloudMap;

import ClaFi.PathArt.SceneHelpers;

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
    }

    // **The share covered is held exactly**: the cut is the noise's quantile for it, so a kind
    // whose noise runs high is not cloudier for that.
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

        m_cover.resize(static_cast<std::size_t>(k_rowLength) * k_height + k_readPadding);
        spreadOverCores(k_height, [&](int row) {
            const float latitude = latitudeAt(row);
            const float* rowNoise = noise.data() + static_cast<std::size_t>(row) * k_width;
            std::uint8_t* rowCover = m_cover.data() + static_cast<std::size_t>(row) * k_rowLength;
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
    }

    // **The map wraps round the longitudes** and stops at the poles, and between texel centers
    // the cover is blended from the four nearest. Eight points at a time take one 4-byte read per
    // row of texels: the texel and its right neighbour stand side by side in it, the last
    // column's included. See PathArt#cloudmap
    void CloudMap::readCover(const std::span<const float> longitudes,
        const std::span<const float> latitudes, const std::span<float> covers) const
    {
        constexpr float k_columnsPerRadian{ k_width / (2.0f * k_pi) };
        constexpr float k_rowsPerRadian{ k_height / k_pi };
        const std::uint8_t* cover = m_cover.data();
        const std::size_t count = covers.size();
        const std::size_t groupedCount = count - count % 8;

        const __m256 columnsPerRadian = _mm256_set1_ps(k_columnsPerRadian);
        const __m256 rowsPerRadian = _mm256_set1_ps(k_rowsPerRadian);
        const __m256 half = _mm256_set1_ps(0.5f);
        const __m256 northPole = _mm256_set1_ps(k_pi / 2.0f);
        const __m256 lastRow = _mm256_set1_ps(static_cast<float>(k_height - 1));
        const __m256 perByte = _mm256_set1_ps(1.0f / 255.0f);
        const __m256i columnMask = _mm256_set1_epi32(k_width - 1);
        const __m256i rowLength = _mm256_set1_epi32(k_rowLength);
        const __m256i lastRowIndex = _mm256_set1_epi32(k_height - 1);
        const __m256i oneRow = _mm256_set1_epi32(1);
        const __m256i byteMask = _mm256_set1_epi32(0xFF);
        const int* readBase = reinterpret_cast<const int*>(cover);
        // The pair at the left column of a row: the left texel in the low byte, the right above.
        const auto blendPair = [&](__m256i pair, __m256 acrossShare) {
            const __m256 leftTexel = _mm256_cvtepi32_ps(_mm256_and_si256(pair, byteMask));
            const __m256 rightTexel = _mm256_cvtepi32_ps(
                _mm256_and_si256(_mm256_srli_epi32(pair, 8), byteMask));
            return _mm256_fmadd_ps(_mm256_sub_ps(rightTexel, leftTexel), acrossShare, leftTexel);
        };
        for (std::size_t i = 0; i != groupedCount; i += 8)
        {
            const __m256 across = _mm256_fmsub_ps(_mm256_loadu_ps(longitudes.data() + i),
                columnsPerRadian, half);
            // The maximum takes its second operand where the first is not a number, so every
            // read stays inside the map.
            const __m256 down = _mm256_min_ps(_mm256_max_ps(_mm256_fmsub_ps(
                _mm256_sub_ps(northPole, _mm256_loadu_ps(latitudes.data() + i)),
                rowsPerRadian, half), _mm256_setzero_ps()), lastRow);
            const __m256 leftColumn = _mm256_floor_ps(across);
            const __m256 topRow = _mm256_floor_ps(down);
            const __m256 acrossShare = _mm256_sub_ps(across, leftColumn);
            const __m256 downShare = _mm256_sub_ps(down, topRow);
            const __m256i left = _mm256_and_si256(_mm256_cvttps_epi32(leftColumn), columnMask);
            const __m256i top = _mm256_cvttps_epi32(topRow);
            const __m256i bottom = _mm256_min_epi32(_mm256_add_epi32(top, oneRow), lastRowIndex);
            const __m256i upperPair = _mm256_i32gather_epi32(readBase,
                _mm256_add_epi32(_mm256_mullo_epi32(top, rowLength), left), 1);
            const __m256i lowerPair = _mm256_i32gather_epi32(readBase,
                _mm256_add_epi32(_mm256_mullo_epi32(bottom, rowLength), left), 1);
            const __m256 upper = blendPair(upperPair, acrossShare);
            const __m256 lower = blendPair(lowerPair, acrossShare);
            const __m256 blended = _mm256_fmadd_ps(_mm256_sub_ps(lower, upper), downShare, upper);
            _mm256_storeu_ps(covers.data() + i, _mm256_mul_ps(blended, perByte));
        }
        for (std::size_t i = groupedCount; i != count; ++i)
        {
            const float across = longitudes[i] * k_columnsPerRadian - 0.5f;
            const float down = std::clamp((k_pi / 2.0f - latitudes[i]) * k_rowsPerRadian - 0.5f,
                0.0f, static_cast<float>(k_height - 1));
            const float leftColumn = std::floor(across);
            const float topRow = std::floor(down);
            const int left = static_cast<int>(leftColumn) & (k_width - 1);
            const int top = static_cast<int>(topRow);
            const int bottom = std::min(top + 1, k_height - 1);
            const std::uint8_t* upperTexels = cover
                + static_cast<std::ptrdiff_t>(top) * k_rowLength;
            const std::uint8_t* lowerTexels = cover
                + static_cast<std::ptrdiff_t>(bottom) * k_rowLength;
            const float acrossShare = across - leftColumn;
            const float upper = std::lerp(static_cast<float>(upperTexels[left]),
                static_cast<float>(upperTexels[left + 1]), acrossShare);
            const float lower = std::lerp(static_cast<float>(lowerTexels[left]),
                static_cast<float>(lowerTexels[left + 1]), acrossShare);
            covers[i] = std::lerp(upper, lower, down - topRow) * (1.0f / 255.0f);
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
    }
}
