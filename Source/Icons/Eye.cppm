export module ClaFi.Icons.Eye;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::Eye
{
    using namespace ::ClaFi::Graphics;

    // An eye - two lids meeting at the corners, an iris and a pupil.
    //
    // It was the last thing in here still staging into a PixelView, and it staged because it read
    // the pixel underneath itself. That sample was only ever used twice: to mix the iris grey, and
    // to paint the corners back out where the two lids, swept as separately clipped circles,
    // crossed and overshot each other. A grey is asked for by grade, and the lids are one
    // closed path that meets at the corners by construction, so neither reason is left - and with
    // the sample goes the scratch buffer, the CPU rasterization and the texture upload a GPU
    // backend was paying for every frame the icon was on screen.
    export void paint(PaintIconEvent&);


    //----------------------------------------------------------------------------


    namespace
    {
        // In half-widths. The lid is the arc the pixel painter swept: a circle centred this far the
        // other side of the eye's own centre, which is what gives the lid its curve.
        constexpr float k_lidOffset = 0.72f;
        constexpr float k_irisRatio = 0.5f;
        constexpr float k_pupilRatio = 0.44f;
        // Of the full width, as before.
        constexpr float k_strokeRatio = 1.0f / 16.0f;
    }

    void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const FloatPoint center = iconRect.center();
        const float halfWidth = iconRect.width() * 0.5f;
        const float strokeWidth = std::max(1.0f, iconRect.width() * k_strokeRatio);

        // The lid's circle has to pass through the corner, one half-width out, so its radius
        // follows from the offset rather than being named beside it. That is where the pixel
        // painter's pair disagreed: 1.25 against the 1.2322 the offset asks for, which left the
        // two lids crossing short of the corner and is what had to be painted back out.
        //
        // A quadratic reaches half way to its control point, so the control sits at twice the apex
        // and the curve lands within a fiftieth of a half-width of the arc it replaces.
        const float apex = std::sqrt(1.0f + k_lidOffset * k_lidOffset) - k_lidOffset;
        const float control = halfWidth * apex * 2.0f;

        Canvas& canvas = event.canvas();
        const Color ink = event.textRgb(InkGrade::Strongest);

        // The iris was the backdrop mixed two thirds of the way toward the ink, which is a grey
        // arrived at the long way round. The subtle tone lands where that mix did.
        canvas.fillCircle(center, halfWidth * k_irisRatio, event.textRgb(InkGrade::Subtle));
        canvas.fillCircle(center, halfWidth * k_irisRatio * k_pupilRatio, ink);

        PixelPath path;
        path.moveTo({ center.x - halfWidth, center.y });
        path.quadTo({ center.x, center.y - control }, { center.x + halfWidth, center.y });
        path.quadTo({ center.x, center.y + control }, { center.x - halfWidth, center.y });
        path.close();
        canvas.drawPath(path, ink, strokeWidth);
    }

}
