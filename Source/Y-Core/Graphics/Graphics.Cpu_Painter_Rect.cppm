export module ClaFi.Core.Graphics.Cpu_Painter_Rect;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{

    // Rects, already in buffer coordinates when they reach here. See Graphics-Types
    export class RectPainter : public PainterBase
    {
    public:
        using PainterBase::PainterBase;
    public:
        void paintSolid(const FloatRect& rect, const Brush& brush, float opacity = 1.0f, const Matrix3x2* brushTransform = nullptr);
        void paintBorder(const FloatRect& rect, const Brush& brush, float borderWidth, float opacity = 1.0f, const Matrix3x2* brushTransform = nullptr);
    };
}
