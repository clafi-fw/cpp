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
        void fill(const FloatRect& rect, const Brush& brush, const Matrix3x2* brushTransform = nullptr);
        void stroke(const FloatRect& rect, const Brush& brush, float borderWidth, const Matrix3x2* brushTransform = nullptr);
    private:
        // The brush over `area`, each pixel as much as `rect` covers it, through any path clip.
        void fillCovered(const IntRect& area, const FloatRect& rect, const Brush&,
            const Matrix3x2* brushTransform);
    };
}
