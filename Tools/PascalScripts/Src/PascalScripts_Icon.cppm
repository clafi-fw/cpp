export module PascalScripts_App.AppIcon;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

namespace PascalScripts_App::AppIcon
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    // The application's mark, in its own colours: it stands on task bars and title bars alike.
    export void draw(Canvas&, const FloatRect& bounds, Opacity);
    // The same mark shaped as a DrawIconFunc, so a control can take it as a property.
    export void drawIcon(DrawIconEvent&);
    // A script's mark: a white sheet with the app's Run arrow, and lines where there is room.
    export void drawScriptIcon(DrawIconEvent&);
}
