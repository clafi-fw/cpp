module ClaFi.Core.Graphics.Cpu_Painter_RoundedRect;

import ClaFi.Core.Graphics.Cpu_Painter_Circle;
import ClaFi.Core.Graphics.Cpu_Painter_Rect;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics::Cpu
{
    // Where each corner's arc meets the two edges it joins, snapped to whole pixels - floor on the
    // near edge, ceil on the far one - so the bodies between the joints meet on exact integer
    // lines while the outer fractional edges stay where the bounds put them. A square corner has
    // no arc, so its joint is the corner point itself.
    struct CornerJoints
    {
        CornerRadii radii{};
        FloatPoint topLeft{};
        FloatPoint topRight{};
        FloatPoint bottomRight{};
        FloatPoint bottomLeft{};
    };

    static float nearJoint(float edge, float radius)
    {
        return radius > 0.0f ? std::floor(edge + radius) : edge;
    }

    static float farJoint(float edge, float radius)
    {
        return radius > 0.0f ? std::ceil(edge - radius) : edge;
    }

    // No corner reaches past the middle of the shorter side, so two corners on one edge never
    // cross.
    static CornerJoints cornerJoints(const RoundedRectangleParts& parts)
    {
        const FloatRect& bounds = parts.bounds;
        const float cap = std::min(bounds.width(), bounds.height()) / 2.0f;
        CornerJoints result;
        for (std::size_t i = 0; i < k_cornersNum; ++i)
            result.radii[i] = std::clamp(parts.radii[i], 0.0f, cap);
        const CornerRadii& radii = result.radii;
        result.topLeft = { nearJoint(bounds.left, radii[0]), nearJoint(bounds.top, radii[0]) };
        result.topRight = { farJoint(bounds.right, radii[1]), nearJoint(bounds.top, radii[1]) };
        result.bottomRight = { farJoint(bounds.right, radii[2]), farJoint(bounds.bottom, radii[2]) };
        result.bottomLeft = { nearJoint(bounds.left, radii[3]), farJoint(bounds.bottom, radii[3]) };
        return result;
    }

    // The arc's radius is the mean of the joint's two distances from the edges, which keeps the
    // outer fractional edges in place.
    static void paintCorner(CirclePainter& painter, Corner corner, FloatPoint joint, float reachX, float reachY, const Matrix3x2* brushTransform)
    {
        painter.setPivot(joint);
        painter.setRadius((reachX + reachY) * 0.5f);
        painter.initBoundary(corner);
        painter.paint(brushTransform);
    }

    void RoundedRectPainter::fillPartial(const RoundedRectangleParts& parts, const Brush& brush, float opacity, const Matrix3x2* brushTransform)
    {
        if (opacity <= 0.0f || parts.bounds.empty())
        {
            return;
        }

        RectPainter rectPainter{ m_pixelView };
        if (isSquare(parts.radii))
        {
            rectPainter.paintSolid(parts.bounds, brush, opacity, brushTransform);
            return;
        }

        const float left = parts.bounds.left;
        const float top = parts.bounds.top;
        const float right = parts.bounds.right;
        const float bottom = parts.bounds.bottom;
        const CornerJoints joints = cornerJoints(parts);

        auto fill = [&](const FloatRect& rect) {
            if (!rect.empty())
                rectPainter.paintSolid(rect, brush, opacity, brushTransform);
        };

        // The bodies: a band across the middle, one between the two corners of the top edge and
        // one of the bottom edge, and under the shorter corner of an edge the strip its neighbour
        // stands taller by.
        const float topBand = std::max(joints.topLeft.y, joints.topRight.y);
        const float bottomBand = std::min(joints.bottomRight.y, joints.bottomLeft.y);
        fill({ left, topBand, right, bottomBand });
        fill({ joints.topLeft.x, top, joints.topRight.x, topBand });
        fill({ joints.bottomLeft.x, bottomBand, joints.bottomRight.x, bottom });
        fill({ left, joints.topLeft.y, joints.topLeft.x, topBand });
        fill({ joints.topRight.x, joints.topRight.y, right, topBand });
        fill({ left, bottomBand, joints.bottomLeft.x, joints.bottomLeft.y });
        fill({ joints.bottomRight.x, bottomBand, right, joints.bottomRight.y });

        CirclePainter circlePainter{ m_pixelView };
        circlePainter.setBackgroundColor(brush);
        circlePainter.setOpacity(opacity);

        if (joints.radii[0] > 0.0f)
            paintCorner(circlePainter, Corner::TopLeft, joints.topLeft, joints.topLeft.x - left, joints.topLeft.y - top, brushTransform);
        if (joints.radii[1] > 0.0f)
            paintCorner(circlePainter, Corner::TopRight, joints.topRight, right - joints.topRight.x, joints.topRight.y - top, brushTransform);
        if (joints.radii[2] > 0.0f)
            paintCorner(circlePainter, Corner::BottomRight, joints.bottomRight, right - joints.bottomRight.x, bottom - joints.bottomRight.y, brushTransform);
        if (joints.radii[3] > 0.0f)
            paintCorner(circlePainter, Corner::BottomLeft, joints.bottomLeft, joints.bottomLeft.x - left, bottom - joints.bottomLeft.y, brushTransform);
    }

    void RoundedRectPainter::drawPartial(const RoundedRectangleParts& parts, const Brush& brush, float borderWidth, float opacity, const Matrix3x2* brushTransform)
    {
        if (borderWidth <= 0.0f || opacity <= 0.0f || parts.bounds.empty())
        {
            return;
        }

        RectPainter rectPainter{ m_pixelView };
        if (isSquare(parts.radii) && parts.sides == k_allRectSidesTrue)
        {
            rectPainter.paintBorder(parts.bounds, brush, borderWidth, opacity, brushTransform);
            return;
        }

        const float left = parts.bounds.left;
        const float top = parts.bounds.top;
        const float right = parts.bounds.right;
        const float bottom = parts.bounds.bottom;
        const CornerJoints joints = cornerJoints(parts);
        const bool roundTopLeft = joints.radii[0] > 0.0f;
        const bool roundTopRight = joints.radii[1] > 0.0f;
        const bool roundBottomRight = joints.radii[2] > 0.0f;
        const bool roundBottomLeft = joints.radii[3] > 0.0f;

        CirclePainter circlePainter{ m_pixelView };
        circlePainter.setBorderColor(brush);
        circlePainter.setBorderWidth(borderWidth);
        circlePainter.setOpacity(opacity);

        // Butt joint layout mapping. A square corner runs its two sides to the outer edge; a round
        // one stops them at the joint so the arc can close the gap. A square corner's joint is the
        // corner point, so a horizontal side runs between the joints either way.
        //
        // At a square corner the horizontal side owns the square the two sides share, so a
        // vertical one steps clear of it - but only where that horizontal side is drawn. Where it
        // is not, the vertical side runs to the outer edge itself: the corner square is then the
        // butt joint against whatever shape shares this edge, and a side stopping short of it
        // leaves a notch one border wide in the line the two shapes form together.
        const float verticalStart = parts.sides[0] ? top + borderWidth : top;
        const float verticalEnd = parts.sides[2] ? bottom - borderWidth : bottom;

        const float rightStart = roundTopRight ? joints.topRight.y : verticalStart;
        const float rightEnd = roundBottomRight ? joints.bottomRight.y : verticalEnd;

        const float leftStart = roundTopLeft ? joints.topLeft.y : verticalStart;
        const float leftEnd = roundBottomLeft ? joints.bottomLeft.y : verticalEnd;

        if (parts.sides[0] && joints.topLeft.x < joints.topRight.x)
        {
            rectPainter.paintSolid({ joints.topLeft.x, top, joints.topRight.x, top + borderWidth }, brush, opacity, brushTransform);
        }

        if (parts.sides[1] && rightStart < rightEnd)
        {
            rectPainter.paintSolid({ right - borderWidth, rightStart, right, rightEnd }, brush, opacity, brushTransform);
        }

        if (parts.sides[2] && joints.bottomLeft.x < joints.bottomRight.x)
        {
            rectPainter.paintSolid({ joints.bottomLeft.x, bottom - borderWidth, joints.bottomRight.x, bottom }, brush, opacity, brushTransform);
        }

        if (parts.sides[3] && leftStart < leftEnd)
        {
            rectPainter.paintSolid({ left, leftStart, left + borderWidth, leftEnd }, brush, opacity, brushTransform);
        }

        // A corner arc is drawn only when at least one of the two sides it joins is present.

        if (roundTopLeft && (parts.sides[0] || parts.sides[3]))
            paintCorner(circlePainter, Corner::TopLeft, joints.topLeft, joints.topLeft.x - left, joints.topLeft.y - top, brushTransform);

        if (roundTopRight && (parts.sides[0] || parts.sides[1]))
            paintCorner(circlePainter, Corner::TopRight, joints.topRight, right - joints.topRight.x, joints.topRight.y - top, brushTransform);

        if (roundBottomRight && (parts.sides[1] || parts.sides[2]))
            paintCorner(circlePainter, Corner::BottomRight, joints.bottomRight, right - joints.bottomRight.x, bottom - joints.bottomRight.y, brushTransform);

        if (roundBottomLeft && (parts.sides[2] || parts.sides[3]))
            paintCorner(circlePainter, Corner::BottomLeft, joints.bottomLeft, joints.bottomLeft.x - left, bottom - joints.bottomLeft.y, brushTransform);
    }
}
