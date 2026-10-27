module SeeDocs_App.Ui.SidebarRows;

import ClaFi.Controls.Tree;

import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App::Ui
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // SidebarDividerRow

    void SidebarDividerRow::paintOwnText(PaintEvent& event)
    {
        const FloatRect bounds = textBounds(event);
        const float y = std::round(bounds.center().y);
        event.canvas().drawLine({ bounds.left, y }, { bounds.right, y },
            event.textRgb(InkGrade::Faint), event.scaledStrokeWidth(Thickness::Thin));
    }
}
