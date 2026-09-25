export module ClaFi.Icons.Chevron;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Graphics;

    export class ChevronPainter
    {
    public:
        // No press argument: the control that owns the mark is scaled as a whole by
        // PaintEvent::pressScale(), and a second shrink here would compound with it. Drawn through
        // the canvas rather than into a pixel view, so that whole-control scale is applied by the
        // backend - and so a GPU backend fills an outline instead of taking a CPU rasterization and
        // a texture upload for every mark on screen.
        static void paint(Graphics::Canvas&, FloatPoint chevronPos, Color color, float turn,
            ScaleFactor scaleFactor);
    };


    //----------------------------------------------------------------------------


    void ChevronPainter::paint(Graphics::Canvas& canvas, FloatPoint chevronPos, Color color,
        const float turn, ScaleFactor scaleFactor)
    {
        static constexpr float k_markSize = 8.0f;
        static constexpr float k_markHalfSize = k_markSize / 2.0f;
        static constexpr float k_markQuarterSize = k_markSize / 4.0f;
        // The same as ExpanderMark
        // Chevron
        PixelPath path;
        path.moveTo(-k_markHalfSize, -k_markQuarterSize);
        path.lineTo(0.0f, k_markQuarterSize);
        path.lineTo(k_markHalfSize, -k_markQuarterSize);
        path.close();
        Matrix3x2 transform = Matrix3x2::translation(chevronPos)
            * Matrix3x2::scale(scaleFactor)
            * Matrix3x2::rotation(turn * k_2Pi);

        // The placement matrix goes to the canvas rather than into the points, so the backend can
        // compose it with whatever transform is already in effect.
        canvas.fillPath(path, color, &transform);
    }

}
