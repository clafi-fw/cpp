export module ClaFi.Icons.DotMark;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::DotMark
{
    using namespace ::ClaFi::Graphics;

    // A share of the icon's size. Large enough to hold the slot at sixteen design units, small
    // enough that it does not read as a state the item is in.
    constexpr float k_radiusShare{ 0.145f };

    // What an item holds its icon slot with when it has nothing of its own to show. It claims no
    // meaning, which is the point: the captions beside it stay in line with the ones that do.
    export void paint(PaintIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float size = std::min(iconRect.width(), iconRect.height());
        event.canvas().fillCircle(event.iconCenter(), size * k_radiusShare, event.accentRgb(InkGrade::Strongest));
    }
}
