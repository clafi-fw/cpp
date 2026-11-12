export module ClaFi.PathArt.NightLights;

import ClaFi.PathArt.EarthMap;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A town the night's lights gather about.
    export struct LightTown
    {
        FloatPoint lonLat; // in degrees
        float size; // 0 to 1, the largest towns' 1
    };

    // Where a row of pixels looks at the ground, and how dark it is there. See PathArt#lightquery
    export struct LightQuery
    {
        std::span<const float> longitudes; // in radians, any number of turns either way
        std::span<const float> latitudes; // in radians
        std::span<const float> footprints; // the ground a pixel spans, in radians of arc
        std::span<const float> darkness; // 0 by day, 1 deep in the night
    };

    // The levels of detail of the lights' texels, each half as fine each way as the one before.
    constexpr int k_lightLevels{ 9 };

    // Where a level of the lights' texels stands among the tiles' starts.
    struct LightLevel
    {
        int tilesAcross;
        int tilesDown;
        int firstTile;
    };

    // The towns' lights at night, as countless tiny lights gathered into sprawls. See PathArt
    export class NightLights
    {
    public:
        // The lights stand on the land inside `inhabited`. See PathArt#nightlights
        NightLights(std::span<const LightTown>,
            std::span<const std::span<const FloatPoint>> inhabited, const EarthMap&);
    public:
        // The tiny lights at each pixel. See PathArt#nightlights-readlights
        void readLights(const LightQuery&, std::span<float> lights) const;
        // The glow over the ground about each point. See PathArt#nightlights-readglow
        void readGlow(const LightQuery&, std::span<float> glows) const;
    private:
        // The bytes past the last texel, for a 4-byte read at it.
        static constexpr int k_readPadding{ 3 };
        // Tile by tile, a byte a texel; the first tile is all 0, and every empty tile is it.
        std::vector<std::uint8_t> m_texels{};
        // Where each tile of each level starts in the texels.
        std::vector<std::int32_t> m_tileStarts{};
        std::array<LightLevel, k_lightLevels> m_levels{};
    };
}
