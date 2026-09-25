export module ClaFi.Core.Graphics.Cpu_FuncPainter;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{
    // Whether a span is filled with one colour or one asked for per pixel.
    export enum class FillMode {
        solidColor,
        dynamicColor
    };

    export enum AxisDirection {
        xRightYUp,
        xRightYDown,
        xDownYLeft
    };

    export template<typename Derived, FillMode fillMode = FillMode::solidColor>
        class FuncPainterBase : public PainterBase
    {
    public:
        using PainterBase::PainterBase;
        struct FuncResult { float y1; float y2; };
    protected:
        Color color() { return m_color; }
        void beginPaint(FloatPoint origin, Color c, AxisDirection axis = AxisDirection::xRightYUp);
        void paint(float startX, float endX);
    private:
        inline void setPixInternal(int x, int y, float k, int shiftY)
        {
            int cx, cy;
            switch (m_axis)
            {
            case AxisDirection::xRightYUp:   cx = m_origX + x; cy = shiftY - y; break;
            case AxisDirection::xRightYDown: cx = m_origX + x; cy = shiftY + y; break;
            case AxisDirection::xDownYLeft:  cx = shiftY - y;  cy = m_origY + x; break;
            default: return;
            }
            if (Color* px = m_pixelView.pixelAbs(static_cast<float>(cx), static_cast<float>(cy))) // m_canvas.pixelForPaint(cx, cy, true))
            {
                if constexpr (fillMode == FillMode::solidColor)
                    px->blend(m_color, k);
                else
                    px->blend(static_cast<Derived*>(this)->calculateColor(x, y), k);
            }
        }
        void paintLedge(int x, float pixW, float v0, float v1, int ySign, int& ledgeInner, int shiftY);
        void paintColumn(int xCoord, float pixW, const FuncResult& res1, const FuncResult& next);
    private:
        AxisDirection m_axis;
        int m_origX, m_origY;
        Color m_color;
    };

    template<typename Derived, FillMode fillMode>
    void FuncPainterBase<Derived, fillMode>::beginPaint(FloatPoint origin, Color c, AxisDirection axis)
    {
        m_axis = axis;
        m_color = c;
        m_origX = static_cast<int>(origin.x);
        m_origY = static_cast<int>(origin.y);
        switch (m_axis)
        {
            case AxisDirection::xRightYUp:
                --m_origY;
                break;
            case AxisDirection::xRightYDown:
                break;
            case AxisDirection::xDownYLeft:
                --m_origX;
                break;
        }
    }

    template<typename Derived, FillMode fillMode>
    void FuncPainterBase<Derived, fillMode>::paint(float startX, float endX)
    {
        int xCoord = static_cast<int>(std::floor(startX));
        int endXCoord = static_cast<int>(std::floor(endX));

        float pixW = (xCoord + 1.0f) - startX;
        FuncResult prevResult;
        static_cast<Derived*>(this)->calculateY(startX, prevResult);

        while (xCoord < endXCoord)
        {
            FuncResult nextResult;
            static_cast<Derived*>(this)->calculateY(xCoord + 1.0f, nextResult);
            paintColumn(xCoord, pixW, prevResult, nextResult);
            prevResult = nextResult;
            ++xCoord;
            pixW = 1.0f;
        }

        float finalW = endX - xCoord;
        if (finalW > 0.001f)
        {
            FuncResult nextResult;
            static_cast<Derived*>(this)->calculateY(endX, nextResult);
            paintColumn(xCoord, finalW, prevResult, nextResult);
        }
    }

    template<typename Derived, FillMode fillMode>
    void FuncPainterBase<Derived, fillMode>::paintLedge(int x, float pixW, float v0, float v1,
        int ySign, int& ledgeInner, int shiftY)
    {
        if (v0 > v1)
            std::swap(v0, v1);
        int y1 = static_cast<int>(std::floor(v1));
        ledgeInner = static_cast<int>(std::floor(v0));
        float baseD = v0 - ledgeInner;
        float spreadD = v1 - v0;

        if (spreadD > 0.0001f)
        {
            const float _k = 1.0f / spreadD;
            float topD = v1 - y1;
            float dy = 1.0f - baseD;
            float dx = dy * _k;

            if (ledgeInner == y1)
                setPixInternal(x, ledgeInner * ySign, ((topD + baseD) / 2.0f) * pixW, shiftY);
            else
            {
                setPixInternal(x, ledgeInner * ySign, (1.0f - dx * dy / 2.0f) * pixW, shiftY);
                float x1 = dx;
                for (int y = ledgeInner + 1; y < y1; ++y)
                {
                    float x2 = x1 + _k;
                    setPixInternal(x, y * ySign, (1.0f - (x1 + x2) / 2.0f), shiftY);
                    x1 = x2;
                }
                setPixInternal(x, y1 * ySign, ((topD * _k) * topD / 2.0f) * pixW, shiftY);
            }
        }
        else if (baseD > 0)
            setPixInternal(x, y1 * ySign, baseD * pixW, shiftY);
    }

    template<typename Derived, FillMode fillMode>
    void FuncPainterBase<Derived, fillMode>::paintColumn(int xCoord, float pixW, const FuncResult& res1, const FuncResult& next)
    {
        FloatRect clip = m_pixelView.clippedBounds();
        int solidsLast, solidsBegin;

        int sTop = (m_axis == AxisDirection::xDownYLeft) ? m_origX : m_origY;
        paintLedge(xCoord, pixW, res1.y2, next.y2, 1, solidsLast, sTop);

        int sBottom = (m_axis == AxisDirection::xRightYUp) ? m_origY + 1 :
            (m_axis == AxisDirection::xRightYDown) ? m_origY - 1 :
            (m_axis == AxisDirection::xDownYLeft) ? m_origX + 1 : m_origY;
        paintLedge(xCoord, pixW, -res1.y1, -next.y1, -1, solidsBegin, sBottom);

        int yStart = 1 - solidsBegin;
        int yEnd = solidsLast;
        if (yStart > yEnd)
            return;
        if (m_axis == AxisDirection::xRightYDown)
        {
            yStart *= -1;
            yEnd *= -1;
        }
        if (m_axis == AxisDirection::xDownYLeft)
        {
            int cy = m_origY + xCoord;
            if (cy >= static_cast<int>(std::floor(clip.top)) && cy < static_cast<int>(std::ceil(clip.bottom)))
            {
                int cx1 = sBottom - yEnd;
                int cx2 = sBottom - yStart;
                if (cx1 > cx2)
                    std::swap(cx1, cx2);

                cx1 = std::max(cx1, static_cast<int>(std::floor(clip.left)));
                cx2 = std::min(cx2, static_cast<int>(std::ceil(clip.right)) - 1);
                if (pixW >= 0.999f /* && m_color.a == 255 */)
                {
                    if constexpr (fillMode == FillMode::solidColor)
                        PixelView::fillPixelLine(m_pixelView.pixelAbs(static_cast<float>(cx1), static_cast<float>(cy), false), cx2 - cx1 + 1, m_color);
                    else
                        for (int cx = cx1; cx <= cx2; ++cx)
                            *m_pixelView.pixelAbs(static_cast<float>(cx), static_cast<float>(cy), false) = static_cast<Derived*>(this)->calculateColor(xCoord, cx2 - cx);
                }
                else
                    for (int cx = cx1; cx <= cx2; ++cx)
                        if constexpr (fillMode == FillMode::solidColor)
                            m_pixelView.pixelAbs(static_cast<float>(cx), static_cast<float>(cy), false)->blend(m_color, pixW);
                        else
                            m_pixelView.pixelAbs(static_cast<float>(cx), static_cast<float>(cy), false)->blend(
                                static_cast<Derived*>(this)->calculateColor(xCoord, cx2 - cx),
                                pixW
                            );
            }
        }
        else
        {
            int cx = m_origX + xCoord;
            if (cx >= static_cast<int>(std::floor(clip.left)) && cx < static_cast<int>(std::ceil(clip.right)))
            {
                int cy1 = sBottom - yStart;
                int cy2 = sBottom - yEnd;
                if (cy1 > cy2)
                    std::swap(cy1, cy2);
                cy1 = std::max(cy1, static_cast<int>(std::floor(clip.top)));
                cy2 = std::min(cy2, static_cast<int>(std::ceil(clip.bottom)) - 1);
                for (int cy = cy1; cy <= cy2; ++cy)
                    if constexpr (fillMode == FillMode::solidColor)
                        m_pixelView.pixelAbs(static_cast<float>(cx), static_cast<float>(cy), false)->blend(m_color, pixW);
                    else
                        m_pixelView.pixelAbs(static_cast<float>(cx), static_cast<float>(cy), false)->blend(
                            static_cast<Derived*>(this)->calculateColor(xCoord, cy2 - cy),
                            pixW
                        );
            }
        }
    }

}
