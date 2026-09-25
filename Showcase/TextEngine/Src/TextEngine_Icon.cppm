export module ClaFi.Showcase.TextEngine.Icon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Showcase::TextEngine::AppIcon
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    // The application's mark, in its own colours: it stands on task bars and title bars alike.
    export void paint(Canvas&, const FloatRect& bounds, Opacity opacity);
    // The same mark shaped as a PaintIconFunc, so a control can take it as a property.
    export void paintIcon(PaintIconEvent&);
}
