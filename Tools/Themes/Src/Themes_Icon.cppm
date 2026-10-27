export module Themes_App.AppIcon;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

namespace Themes_App::AppIcon
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    // The application's mark, in its own colours: it stands on task bars and title bars alike.
    export void draw(Canvas&, const FloatRect& bounds, Opacity opacity);
    // The same mark shaped as a DrawIconFunc, so a control can take it as a property.
    export void drawIcon(DrawIconEvent&);
}
