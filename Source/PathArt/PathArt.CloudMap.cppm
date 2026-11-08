export module ClaFi.PathArt.CloudMap;

import ClaFi.PathArt.SceneHelpers;

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
        // The cover at each point, 0 to 1, as fine as its ground. See PathArt#cloudmap
        void readCover(const MapQuery&, std::span<float> covers) const;
    private:
        // The finest level's texels round each circle of latitude.
        static constexpr int k_width{ 2048 };
        // The finest level's texels from the north pole to the south pole.
        static constexpr int k_height{ 1024 };
        // The levels of detail, each half as fine each way as the one before.
        static constexpr int k_levels{ 8 };
        // The bytes past the last row, for a 4-byte read at its last texel.
        static constexpr int k_readPadding{ 3 };
        // The cover, 0 to 255, level by level, each row's first texel again past its last.
        std::vector<std::uint8_t> m_cover{};
        // Where each level starts in the cover.
        std::array<int, k_levels> m_levelStarts{};
    };
}
