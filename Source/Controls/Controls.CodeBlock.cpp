module ClaFi.Controls.CodeBlock;

import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // CodeBlock

    // The band runs down the left edge, as a callout's does.
    void CodeBlock::paintFrame(PaintEvent& event)
    {
        PanelBase::paintFrame(event);
        event.paintBand(RectSide::Left, event.bandRgb());
    }
}
