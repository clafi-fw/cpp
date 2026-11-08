export module ClaFi.PathArt.CloudMap;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // What a cloud map's clouds look like. See PathArt#cloudkind
    export enum class CloudKind
    {
        Puffy,      // heaps bunched into fields, with shading inside
        Patchy,     // broad sheets with soft edges
        Streaky     // thin streaks along the circles of latitude
    };

    // How much cloud lies over each point of a sphere, kept by longitude and latitude. See PathArt
    export class CloudMap
    {
    public:
        // Spreads its work over the processor's cores; `cover` is the share of the sphere covered.
        CloudMap(CloudKind, std::uint32_t seed, float cover);
    public:
        // The cover, 0 to 1, at each longitude and latitude in radians. See PathArt#cloudmap
        void readCover(std::span<const float> longitudes, std::span<const float> latitudes,
            std::span<float> covers) const;
    private:
        // The texels round each circle of latitude.
        static constexpr int k_width{ 2048 };
        // The texels from the north pole to the south pole.
        static constexpr int k_height{ 1024 };
        // A row as kept: its texels, then its first again to stand right of its last.
        static constexpr int k_rowLength{ k_width + 1 };
        // The bytes past the last row, for a 4-byte read at its last texel.
        static constexpr int k_readPadding{ 3 };
        // The cover at each texel, 0 to 255, row by row from the north pole.
        std::vector<std::uint8_t> m_cover{};
    };
}
