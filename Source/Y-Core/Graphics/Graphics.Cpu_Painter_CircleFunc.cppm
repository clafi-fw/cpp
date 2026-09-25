export module ClaFi.Core.Graphics.Cpu_Painter_CircleFunc;

import ClaFi.Core.Graphics.Cpu_FuncPainter;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{

    export struct CircleParams
    {
        FloatPoint origin;
        Color color;
        float radius;
        float lineWidth;
        bool top{ true };
        bool bottom{ true };
    };

    export class CustomCirclePainter : public FuncPainterBase<CustomCirclePainter>
    {
    public:
        using FuncPainterBase::FuncPainterBase;
    public:
        void paint(const CircleParams&, float xFrom, float xTo);
        void calculateY(float x, FuncResult& result) const;
    private:
        float m_sqrRadius;
        float m_innerRadius;
        float m_innerSqrRadius;
    };


    //----------------------------------------------------------------------------

    // CustomCirclePainter

    void CustomCirclePainter::paint(const CircleParams& params, float xFrom, float xTo)
    {
        m_sqrRadius = params.radius * params.radius;
        m_innerRadius = params.radius - params.lineWidth;
        m_innerSqrRadius = m_innerRadius * m_innerRadius;
        if (params.bottom)
        {
            beginPaint(params.origin, params.color, xRightYDown);
            FuncPainterBase::paint(xFrom, xTo);
        }
        if (params.top)
        {
            beginPaint(params.origin, params.color, xRightYUp);
            FuncPainterBase::paint(xFrom, xTo);
        }
    }

    void CustomCirclePainter::calculateY(float x, FuncResult& result) const
    {
        float xx = x * x;
        if (x <= -m_innerRadius || x >= m_innerRadius)
            result.y1 = 0.0f;
        else
            result.y1 = std::sqrt(m_innerSqrRadius - xx);
        result.y2 = std::sqrt(m_sqrRadius - xx);
    }

}
