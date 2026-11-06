module ClaFi.Controls.ToolBar;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    RoundedRectangleParts ToolBar::silhouette(const PaintEvent& hostEvent,
        const FloatRect& barRect) const
    {
        const float radius = hostEvent.scaleF(designMetrics().radius);
        return { .bounds = barRect, .radii = { radius, radius, radius, radius } };
    }
}
