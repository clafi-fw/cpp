export module ClaFi.Core.Graphics.Cpu_Painter_RoundedRect;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{

    // Rounded rects, bounds and radius alike already in buffer coordinates. See Graphics-Types
    export class RoundedRectPainter : public PainterBase
    {
    public:
        using PainterBase::PainterBase;
    public:
        void fillPartial(const RoundedRectangleParts& parts, const Brush& brush, float opacity = 1.0f, const Matrix3x2* brushTransform = nullptr);
        void drawPartial(const RoundedRectangleParts& parts, const Brush& brush, float borderWidth, float opacity = 1.0f, const Matrix3x2* brushTransform = nullptr);
    };
}
