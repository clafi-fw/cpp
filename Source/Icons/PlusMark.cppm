export module ClaFi.Icons.PlusMark;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Icons
{
    export class PlusMark
    {
    public:
        void paintPlusOrMinus(bool plus);
        static PaintIconFunc paintInText;
        static void paint(PaintIconEvent&);
    public:
        // The same fields as in the XMark
        Graphics::Canvas& canvas;
        FloatPoint center;
        float size;
        float lineWidth;
        Color color;
    };


    //-------------------------------------------------------------------------


    void PlusMark::paintPlusOrMinus(bool plus)
    {
        float sz = (size - lineWidth) * 0.5f;
        canvas.drawLine({ center.x - sz, center.y }, { center.x + sz, center.y }, color, lineWidth);
        if (plus)
            canvas.drawLine({ center.x, center.y - sz }, { center.x, center.y + sz }, color, lineWidth);
    }

    void PlusMark::paint(PaintIconEvent& event)
    {
        PlusMark mark{
            .canvas = event.canvas(),
            .center = event.iconCenter(),
            .size = event.iconWidth(),
            .lineWidth = event.scaledStrokeWidth(Thickness::Thin),
            .color = event.textRgb(InkGrade::Strongest)
        };
        mark.paintPlusOrMinus(true);
    }

}
