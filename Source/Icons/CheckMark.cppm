export module ClaFi.Icons.CheckMark;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::CheckMark
{
    using namespace ::ClaFi::Graphics;

    // Everything the mark needs is on the event: how far it is drawn from the pressed and selected
    // factors, its weight and padding from the metrics, its size and place from the icon rect. That
    // is what lets it be a plain PaintIconFunc rather than something a control has to feed by hand.
    //
    // The caller decides what the icon rect is. ButtonBase hands over its indicator's bounds inset
    // by a third of the margin, which is a framing decision and stays with the control.
    export void paint(PaintIconEvent&, const StateFactors&);


    //-------------------------------------------------------------------------


    // The mark as an outline, for the canvas to fill. Drawn through the canvas it becomes native
    // geometry on a GPU backend, where staging it as pixels cost a buffer clear, a CPU
    // rasterization and a texture upload for every instance on screen, every frame.
    //
    // Both boundaries are piecewise linear, so the outline could be an exact polygon of a handful
    // of points rather than the sampled one below. Doing that means finding where the two clamps
    // in calculateY engage, which is a change to the mark's weight if it is got wrong - left as
    // it is until there is a reason to touch it.
    class MarkShape
    {
    public:
        static void buildPath(PixelPath&, FloatRect, float padding, float lw, float lineFactor);
    private:
        // The two boundaries of the mark at a given x, lower and upper.
        struct FuncResult
        {
            float y1{ 0.0f };
            float y2{ 0.0f };
        };
        void calculateY(float x, FuncResult&);
        // Returns the drawn width.
        float setupGeometry(FloatRect, float padding, float lw);
    private:
        FloatRect m_markRect{};
        float m_markY1{ 0.0f };
        float m_markY2{ 0.0f };
        float m_markY3{ 0.0f };
        float m_dY{ 0.0f };
        float m_halfDy{ 0.0f };
        float m_quartDy{ 0.0f };
        float m_eighthDy{ 0.0f };
        float m_vTurnX{ 0.0f };
    };


    void MarkShape::buildPath(PixelPath& path, FloatRect drawRect, float padding, float markLineW, float lineFactor)
    {
        if (!lineFactor)
            return;

        MarkShape shape{};
        float w = shape.setupGeometry(drawRect, padding, markLineW);

        float startX = shape.m_markRect.left;
        float endX = startX + w * lineFactor;
        if (!(endX > startX))
            return;

        // The mapping the scanline painter used for xRightYUp, kept exactly. Local x runs right
        // from the rect's left edge; local y runs up, and the fill took its two edges from
        // references one pixel apart - one for the upper boundary, one for the lower - which is how
        // it covered whole pixels on both sides. Reproducing that here rather than the bare dY band
        // is what keeps the mark the weight it has always been.
        const float originX = static_cast<float>(static_cast<int>(drawRect.left));
        const float upperOriginY = static_cast<float>(static_cast<int>(drawRect.top) - 1);
        const float lowerOriginY = static_cast<float>(static_cast<int>(drawRect.top));

        // Sampled from calculateY rather than derived again, so the outline cannot drift from the
        // shape the scanline painter produced. Both boundaries are piecewise linear, so sampling is
        // exact away from their breakpoints - and vTurnX, where the two arms meet, is visited
        // explicitly because it is the one breakpoint that carries the shape.
        constexpr float k_step = 0.5f;
        std::vector<float> xs;
        xs.reserve(static_cast<std::size_t>((endX - startX) / k_step) + 4);
        for (float x = startX; x < endX; x += k_step)
            xs.push_back(x);
        if (shape.m_vTurnX > startX && shape.m_vTurnX < endX)
            xs.push_back(shape.m_vTurnX);
        xs.push_back(endX);
        std::sort(xs.begin(), xs.end());

        FuncResult result{};
        shape.calculateY(xs.front(), result);
        path.moveTo(originX + xs.front(), lowerOriginY - result.y1);
        for (std::size_t i = 1; i != xs.size(); ++i)
        {
            shape.calculateY(xs[i], result);
            path.lineTo(originX + xs[i], lowerOriginY - result.y1);
        }
        for (std::size_t i = xs.size(); i-- != 0; )
        {
            shape.calculateY(xs[i], result);
            path.lineTo(originX + xs[i], upperOriginY - result.y2);
        }
        path.close();
    }

    void MarkShape::calculateY(float x, FuncResult& result)
    {
        float y1;
        if (x <= m_vTurnX)
            y1 = m_markRect.left - x - m_markY1 - m_dY;
        else
            y1 = x - m_vTurnX - m_markY2;

        float y2 = y1 + m_dY;

        if (x <= m_vTurnX)
            y2 = std::min(y2, -m_markY1 - m_halfDy);
        else
        {
            y2 = std::min(y2, -m_markY3 - m_eighthDy);
            y1 += m_dY * (x - m_vTurnX) / (m_markRect.right - m_vTurnX) * 0.83f;
            y1 = std::min(y1, y2);
        }
        result = {y1, y2};
    }

    float MarkShape::setupGeometry(FloatRect drawRect, float padding, float markLineW)
    {
        m_dY = markLineW * 1.66f;
        m_halfDy = m_dY / 2.0f;
        m_quartDy = m_halfDy / 2.0f;
        m_eighthDy = m_quartDy / 2.0f;
        //
        m_markRect = drawRect;
        m_markRect.offset(m_quartDy - drawRect.left, -drawRect.top);
        m_markRect = m_markRect.fitRect(1.0f, 1.0f);
        m_markRect.inflate(-padding, -padding);
        //
        float w = m_markRect.bottom - m_markRect.top;
        float tmp1 = w * 0.666f;
        float tmp2 = -w * 0.1f;
        m_markY1 = -m_halfDy + tmp1 - tmp2;
        m_markY2 = m_markRect.bottom - m_halfDy - tmp2;
        m_vTurnX = m_markRect.right - tmp1;
        m_markY3 = m_markY2 - (m_markRect.right - m_vTurnX) - m_halfDy;
        w -= m_halfDy;
        return w;
    }

    void paint(PaintIconEvent& event, const StateFactors& stateFactors)
    {
        const float lineFactor = StateFactors::compose(stateFactors.pressed(), stateFactors.selected());
        if (!lineFactor)
        {
            return;
        }

        const float markPadding = event.scaleF(event.themeMetrics().checkMark.padding.x);
        const float markLineW = event.scaleF(1.5f);
        PixelPath markPath;
        MarkShape::buildPath(markPath, event.iconRect(), markPadding, markLineW, lineFactor);
        event.canvas().fillPath(markPath, event.textRgb(InkGrade::Strongest));
    }
}
