export module ClaFi.PathArt.WaterReflection;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // How the water breaks a reflection: each row shifted sideways, more toward the viewer.
    export struct Ripples
    {
        float top; // the water's highest row in the frame, where the shifts are least
        float farReach; // how far the ripple moves a row at `top`, in units of the frame
        float nearReach; // how far it moves the frame's bottom row
        float cycleTime; // the ripples run a whole number of times a cycle
        float opacity; // how much of the water's colour the reflection covers
    };

    // What a scene shows mirrored in its water, laid into the water in rippling rows. See PathArt
    export class WaterReflection
    {
    public:
        // Draws parts of the scene as the water shows them, into the frame.
        using Drawing = std::function<void(const SceneFrame&)>;
    public:
        WaterReflection();
        WaterReflection(const WaterReflection&) = delete;
        WaterReflection& operator=(const WaterReflection&) = delete;
        ~WaterReflection();
    public:
        // Drops the kept pixels, for a scene whose still parts or water have changed.
        void forget() { m_kept = false; }
        // `still` is drawn once and kept, `moving` every frame. See PathArt
        void draw(const SceneFrame&, const PixelPath& water, const Drawing& still,
            const Drawing& moving, const Ripples&);
    private:
        // The first and the last column the water covers in a row; first past last where it
        // covers none.
        struct ColumnSpan
        {
            int first;
            int last;
        };
    private:
        void keep(const SceneFrame&, const PixelPath& water, const Drawing& still, float top);
    private:
        std::unique_ptr<Canvas> m_mirrorCanvas{}; // where each frame's reflection is drawn
        Bitmap m_still{}; // what `still` drew, from the water's top row down
        Bitmap m_rippled{}; // the frame's reflection as it is laid into the water
        std::vector<ColumnSpan> m_waterSpans{}; // one for each row
        std::vector<std::uint8_t> m_waterCoverage{}; // the water's share of each pixel, 0 to 255
        IntSize m_keptSize{};
        float m_keptTop{};
        bool m_kept{ false };
    };
}
