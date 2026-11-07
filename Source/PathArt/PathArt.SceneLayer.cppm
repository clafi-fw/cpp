export module ClaFi.PathArt.SceneLayer;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A part of a scene that stays still, kept as pixels on the CPU backend. See PathArt
    export class SceneLayer
    {
    public:
        // Draws the part into the scene's frame, from nothing but the frame.
        using Drawing = void (*)(const SceneFrame&);
    public:
        explicit SceneLayer(Drawing);
    public:
        // Keeps the part's pixels for the params, unless they are kept for them already.
        void prepare(const SceneParams&);
        // The kept pixels where they fit the frame; the part drawn live where they do not.
        void draw(const SceneFrame&) const;
    private:
        Drawing m_drawing;
        Graphics::Bitmap m_pixels{};
        FloatPoint m_pixelsAt{}; // the pixels' top left on the canvas, in pixels
        Graphics::Matrix3x2 m_sceneToPixels{}; // the mapping the pixels were drawn through
        FloatPoint m_frameSize{}; // the width and height of the frame they were drawn for
        bool m_kept{ false };
    };
}
