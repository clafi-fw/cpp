export module ClaFi.Icons.PropertiesIcon;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::PropertiesIcon
{
    using namespace ::ClaFi::Graphics;

    // Three named fields, the Properties command's own mark. See Icons#propertiesicon
    export void draw(DrawIconEvent&);


    //-------------------------------------------------------------------------


    namespace
    {
        // A field is a key that names the property and the value written against it. The key is
        // the part that says what the command is about, so it takes the accent.
        constexpr float k_keyLeftShare = 0.06f;
        constexpr float k_keyRightShare = 0.32f;
        constexpr float k_valueLeftShare = 0.44f;
        constexpr float k_valueRightShare = 0.94f;

        // Three rows take the whole of the icon's square, and the gap between two of them is wider
        // than a row is tall, which is what keeps them apart at the toolbar's eighteen design
        // units.
        constexpr float k_rowHeightShare = 0.14f;
        constexpr float k_rowHalfHeightShare = k_rowHeightShare * 0.5f;
        constexpr float k_rowShares[]{ 0.20f, 0.50f, 0.80f };
    }


    //-------------------------------------------------------------------------


    void draw(DrawIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        // The stroked icons beside this one stand a stroke inside their rect, and the rows keep
        // the same margin so that the set reads as one optical size.
        const float size = std::min(iconRect.width(), iconRect.height()) - strokeWidth;
        const FloatRect box = iconRect.centerRect(size);
        // Half a row's height, which leaves both ends of every row round.
        const float radius = size * k_rowHalfHeightShare;
        Canvas& canvas = event.canvas();

        const Color bodyColor = event.textRgb(InkGrade::Strong);
        const Color keyColor = event.accentRgb(InkGrade::Strongest);

        for (const float row : k_rowShares)
        {
            const float top = row - k_rowHalfHeightShare;
            const float bottom = row + k_rowHalfHeightShare;
            canvas.fillRoundedRectangle(
                box.relativeRect({ k_keyLeftShare, top }, { k_keyRightShare, bottom }),
                radius,
                radius,
                keyColor
            );
            canvas.fillRoundedRectangle(
                box.relativeRect({ k_valueLeftShare, top }, { k_valueRightShare, bottom }),
                radius,
                radius,
                bodyColor
            );
        }
    }
}
