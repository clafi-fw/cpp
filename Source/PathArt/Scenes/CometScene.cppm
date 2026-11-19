export module ClaFi.PathArt.CometScene;

import ClaFi.PathArt.CloudMap;
import ClaFi.PathArt.CometSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SceneLayer;
import ClaFi.PathArt.WaterReflection;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A comet falls across the night sky and lands behind the mountains. See PathArt
    export class CometScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Comet"; }
        void prepare(const SceneParams&) override;
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
        void drawIcon(Graphics::Canvas&, const FloatRect& bounds) const override;
        [[nodiscard]] SceneReadoutColors readoutColors() const override;
    private:
        // One pass of the comet: the curve it falls along and how far into the pass it is.
        struct Pass
        {
            CometCourse course;
            // Where the pass stands, 0 to 1: the fall, then the glow behind the ridge.
            float passTime;
            // 0 until the comet lands, then 0 to 1 as the glow behind the ridge fades.
            float afterglow;
        };
        // A band of the fog's clouds: where it lies, which latitude of the map its clouds are read
        // along, and how much of the fog's colour it takes. See PathArt#cloudkind
        struct FogBand
        {
            float topShare; // where the band starts and ends down the frame
            float bottomShare;
            float latitude; // the map's latitude its clouds are read along
            float longitude; // where along that latitude its strip starts
            float strength; // the fog's opacity where it is densest
        };
        // The fog's two bands: one in the valley behind the near range, one nearest, drawn over the
        // lake and its trees, each reading its own strip of the clouds.
        static constexpr FogBand k_valleyFog{
            .topShare = 0.400f,
            .bottomShare = 0.640f,
            .latitude = 0.0f,
            .longitude = 0.0f,
            .strength = 0.55f
        };
        static constexpr FogBand k_nearFog{
            .topShare = 0.545f,
            .bottomShare = 0.840f,
            .latitude = 0.13f,
            .longitude = 0.8f,
            .strength = 0.45f
        };
    private:
        [[nodiscard]] static float farRidgeY(const SceneFrame&, float x);
        [[nodiscard]] static float nearRidgeY(const SceneFrame&, float x);
        [[nodiscard]] static Pass passAt(const SceneFrame&, float cycleTime);
        // The frame turned over about the lake's edge, as the lake shows what is above it.
        [[nodiscard]] static SceneFrame reflectionOf(const SceneFrame&);
        // Everything above the lake, which the lake shows again upside down.
        void drawAboveLake(const SceneFrame& upright, float cycleTime, bool reflection) const;
        // Starts the fog's cloud map building, and takes it once it is built.
        void takeFogMap();
        // One of the fog's bands, cut from a strip of the Earth's clouds into `pixels`.
        void drawFogBand(const SceneFrame&, float cycleTime, const Pass&, const FogBand&,
            Graphics::Bitmap& pixels) const;
        void drawLake(const SceneFrame&, float cycleTime);
        // The comet's light on the water: the Mill's sun path, over the lake's own sparkles.
        static void drawGlitterPath(const SceneFrame&, const Pass&, const Ripples&, float cycleTime);
        static void drawSky(const SceneFrame&);
        static void drawFarRidge(const SceneFrame&);
        static void drawNearRidge(const SceneFrame&);
        static void drawMirroredFarRidge(const SceneFrame&);
        static void drawMirroredNearRidge(const SceneFrame&);
        static void drawLakeWater(const SceneFrame&);
        static void drawLakeShade(const SceneFrame&);
        static void drawShore(const SceneFrame&);
        static void drawMirroredShore(const SceneFrame&);
    private:
        SceneLayer m_farRidge{ drawFarRidge };
        SceneLayer m_nearRidge{ drawNearRidge };
        SceneLayer m_lakeWater{ drawLakeWater };
        SceneLayer m_mirroredFarRidge{ drawMirroredFarRidge };
        SceneLayer m_mirroredNearRidge{ drawMirroredNearRidge };
        SceneLayer m_lakeShade{ drawLakeShade };
        SceneLayer m_shore{ drawShore };
        SceneLayer m_mirroredShore{ drawMirroredShore };
        WaterReflection m_reflection{}; // what the lake mirrors
        // By pointer, as MSVC's std::future builds what it carries with no arguments first.
        std::future<std::unique_ptr<CloudMap>> m_fogMapBuilt{};
        // Built off the paint from the first frames drawn on, and kept. See PathArt#earthscene
        std::unique_ptr<CloudMap> m_fogMap{};
        // The fog's bands, kept from frame to frame so a frame of the same size allocates nothing.
        mutable Graphics::Bitmap m_valleyFogPixels{};
        mutable Graphics::Bitmap m_nearFogPixels{};
    };
}
