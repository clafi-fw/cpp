module;
#include "../Y-Core/System/Switches.h"

module ClaFi.Controls.Button;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.TextEngine;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Controls
{
    DrawTextResult LinkButton::drawText(PaintEvent& event, const FloatRect& textBounds,
        const Text& text)
    {
        const ControlPaintContext& paintContext = event.controlContext();
        ControlPaintContext context = paintContext;
        context.disabledAmount = 0.0f;
        context.textHsl = context.inkHsl(k_linkInk);
        context.disabledAmount = paintContext.disabledAmount;
        const TextRange underline = event.hotFactor() > 0.0f
            ? TextRange{ 0, text.plainText().size() }
            : TextRange{};
        return textEngine().drawText(
            context,
            textBounds,
            text,
            textAnchor(),
            editProps(),
#if CLAFI_TEXT_MOVING
            textRenderMode(),
#endif
            wordWrap(),
            textFormat(),
            underline
        );
    }
}
