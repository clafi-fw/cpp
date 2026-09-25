export module ClaFi.Core.Graphics.Cpu_Painter_Circle;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{

    // Circles, with pivots and radii already in buffer coordinates. See Graphics-Types
    export class CirclePainter : public PainterBase
    {
    public:
        using PainterBase::PainterBase;
    public:
        [[nodiscard]] const FloatRect& boundingBox() const { return m_boundingBox; }
        void setBoundingBox(const FloatRect& value) { m_boundingBox = value; }
        [[nodiscard]] FloatPoint pivot() const { return m_pivot; }
        void setPivot(FloatPoint value) { m_pivot = value; }
        [[nodiscard]] float radius() const { return m_radius; }
        void setRadius(float value) { m_radius = value; }
        [[nodiscard]] float borderWidth() const { return m_borderWidth; }
        void setBorderWidth(float value) { m_borderWidth = value; }

        [[nodiscard]] Brush borderColor() const { return m_borderColor; }
        void setBorderColor(const Brush& value) { m_borderColor = value; }
        void setBorderColor(Color value) { m_borderColor = SolidColor{ value }; }

        [[nodiscard]] Brush backgroundColor() const { return m_backGroundColor; }
        void setBackgroundColor(const Brush& value) { m_backGroundColor = value; }
        void setBackgroundColor(Color value) { m_backGroundColor = SolidColor{ value }; }

        [[nodiscard]] float opacity() const { return m_opacity; }
        void setOpacity(float value) { m_opacity = value; }

        void initBoundary();
        void initBoundary(Corner);
        void paint(const Matrix3x2* brushTransform = nullptr);

        // Brush Overloads
        void paintRing(FloatPoint pivot, const Brush& borderBrush, float radius, float borderWidth, const Matrix3x2* brushTransform = nullptr);
        void paintSolid(FloatPoint pivot, const Brush& bgColor, float radius, const Matrix3x2* brushTransform = nullptr);

        // Raw Color Compatibility Overloads
        void paintRing(FloatPoint pivot, Color borderColor, float radius, float borderWidth, const Matrix3x2* brushTransform = nullptr)
        {
            paintRing(pivot, SolidColor{ borderColor }, radius, borderWidth, brushTransform);
        }
        void paintSolid(FloatPoint pivot, Color bgColor, float radius, const Matrix3x2* brushTransform = nullptr)
        {
            paintSolid(pivot, SolidColor{ bgColor }, radius, brushTransform);
        }

    private:
        void paintDirectCircle(const FloatRect& boundary, float opacity);
        void rasterizeCircleMask(bool isRing, const FloatRect& bounds, float* mask, int stride);
        static void compositeBrush(const PixelView& target, const Brush& brush, float opacity, const Matrix3x2* brushTransform);

    private:
        FloatRect m_boundingBox;
        FloatPoint m_pivot;
        float m_radius{ 0.0f };
        float m_borderWidth{};
        Brush m_borderColor{ SolidColor{Color{}} };
        Brush m_backGroundColor{ SolidColor{Color{}} };
        float m_opacity{ 1.0f };
        Corner m_corner{ Corner::None };
    };
}
