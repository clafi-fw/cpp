export module ClaFi.Icons.ExpanderMark;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Icons::ExpanderMark
{
    using namespace Graphics;

    // No press argument: the control that owns the mark is scaled as a whole by
    // PaintEvent::pressScale(), and a second shrink here would compound with it.
    //
    // The expanded factor cannot come off the event, so it is an argument. Everything else does.
    export void paint(PaintIconEvent&, float expandedFactor);


    //----------------------------------------------------------------------------


    void paint(PaintIconEvent& event, float expandedFactor)
    {
        constexpr float k_etalonIconSize = 18.0f;
        constexpr float k_strokeWidth = 1.6f;
        constexpr float k_markSize = k_etalonIconSize - k_strokeWidth;
        constexpr float k_markHalfSize = k_markSize / 2.0f;

        // The mark is built at its etalon size and scaled to the icon rect, which carries the
        // stroke width along with it. The canvas transform is composed by the backend on top of
        // this one, so the mark follows its control's animation.
        float scaleFactor = event.iconWidth() / k_etalonIconSize;

        // The same as Chevron
        float animatedHalf = k_markHalfSize * (1.0f - expandedFactor);
        PixelPath path{};
        path.moveTo(-animatedHalf, 0.0f);
        path.lineTo(animatedHalf, 0.0f);
        path.moveTo(0.0f, -k_markHalfSize);
        path.lineTo(0.0f, k_markHalfSize);

        Matrix3x2 transform =
            Matrix3x2::translation(event.iconCenter())
            * Matrix3x2::scale(scaleFactor)
            * Matrix3x2::rotation(expandedFactor * 0.25f * k_2Pi);

        event.canvas().drawPath(
            path,
            { PathDrawLayer::stroke(event.textRgb(InkGrade::Strongest), k_strokeWidth) },
            &transform);
    }

}
