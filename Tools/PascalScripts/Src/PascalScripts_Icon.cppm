export module ThisApp.AppIcon;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

namespace ThisApp::AppIcon
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Graphics;

    // The application's mark, in its own colours: it stands on task bars and title bars alike.
    // A placeholder until the mark is designed - see the icon file beside the project.
    export void paint(Canvas&, const FloatRect& bounds, Opacity);
    // The same mark shaped as a PaintIconFunc, so a control can take it as a property.
    export void paintIcon(PaintIconEvent&);
    // A script's mark, in the inks of where it stands: a crumb, a tab, a tile.
    export void paintScriptIcon(PaintIconEvent&);
}
