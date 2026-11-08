export module ClaFi.PathArt.EarthMap;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // Which of the map's two reliefs a read gives.
    export enum class Relief
    {
        Detailed,   // every ridge the ground a point stands for leaves room for
        Broad       // smoothed to the ranges' broad shapes
    };

    // The land at a row of points, one value per point in each. See PathArt#landpoints
    export struct LandPoints
    {
        std::span<float> heights; // 1 at the highest peaks
        std::span<float> slopesEast; // the rise eastward, in heights per radian of arc
        std::span<float> slopesNorth; // the rise northward, in heights per radian of arc
        std::span<float> variation; // -1 to 1, which way the colour of the patch leans
        std::span<float> dryness; // 0 where it is green, 1 in the deepest desert
        std::span<float> forest; // the share of northern forest
        std::span<float> tundra; // the share of tundra
    };

    // Texels of four bytes each, by longitude, and by latitude from the north pole to the south,
    // in 8 levels of detail, each half as fine each way as the one before.
    struct TexelPyramid
    {
        int width; // the finest level's texels round each circle of latitude
        int height; // the finest level's texels from pole to pole
        // Level by level, rows from the north pole, each row's first texel again past its last.
        std::vector<std::uint32_t> texels;
        std::array<int, 8> offsets; // where each level starts in the texels
    };

    // Natural Earth's outlines of the land, one after another. See PathArt#landoutlines
    struct NaturalEarthLand
    {
        std::span<const std::uint16_t> pointCounts; // one for each outline
        // Each point's longitude, then its latitude, in hundredths of a degree.
        std::span<const std::int16_t> points;
    };

    // The outlines, kept in PathArt.EarthMap.NaturalEarth.cpp.
    [[nodiscard]] NaturalEarthLand naturalEarthLand();

    // The Earth's land: how high it stands and what covers it. See PathArt
    export class EarthMap
    {
    public:
        // Spreads its work over the processor's cores.
        EarthMap();
    public:
        // The land at each point, as fine as the ground it stands for. See PathArt#earthmap
        void readLand(const MapQuery&, Relief, const LandPoints&) const;
    private:
        // Height, the two slopes and the variation.
        TexelPyramid m_relief{};
        // Dryness, forest and tundra, then a byte unused.
        TexelPyramid m_cover{};
    };

    // A closed coast, x the longitude and y the latitude in degrees. See PathArt#landoutline
    export using LandOutline = std::vector<FloatPoint>;

    // The coasts of the land, with a grain finer than their data's. See PathArt#landoutlines
    export [[nodiscard]] std::vector<LandOutline> landOutlines();
}
