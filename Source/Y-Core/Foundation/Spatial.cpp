module ClaFi.Core.Foundation;

import :Spatial;

import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // AxisSpan

    float AxisSpan::distanceTo(float value) const
    {
        if (value < start)
            return start - value;
        if (value > end)
            return value - end;
        return 0.0f;
    }

    float AxisSpan::clampInto(float value) const
    {
        // Half a device pixel, and never more than a quarter of the span, so a span too thin to
        // hold the inset still answers with a value inside it.
        const float inset = std::min(0.5f, extent() / 4.0f);
        return std::clamp(value, start + inset, end - inset);
    }

    void AxisSpan::offset(float value)
    {
        start += value;
        end += value;
    }

    // OrientedRect

    OrientedRect OrientedRect::orient(const FloatRect& rect, KeyCode key)
    {
        switch (key)
        {
        case Keys::Right:
            // Moving Right: the next lane is down, so Y keeps its sign.
            return { rect.left, rect.right, rect.top, rect.bottom };

        case Keys::Down:
            // Moving Down: the next lane is right, so X keeps its sign.
            return { rect.top, rect.bottom, rect.left, rect.right };

        case Keys::Left:
            // Moving Left: the next lane is up, so Y is mirrored.
            return { -rect.right, -rect.left, -rect.bottom, -rect.top };

        case Keys::Up:
            // Moving Up: the next lane is left, so X is mirrored.
            return { -rect.bottom, -rect.top, -rect.right, -rect.left };

        default:
            return {};
        }
    }

    OrientedRect OrientedRect::orient(FloatPoint point, KeyCode key)
    {
        return orient(FloatRect{ point.x, point.y, point.x, point.y }, key);
    }

    void OrientedRect::implode()
    {
        primary.end = primary.start;
        secondary.end = secondary.start;
    }

    // Score

    bool Score::isBetterThan(const Score& other) const
    {
        if (inLane != other.inLane)
            return inLane;
        if (primary != other.primary)
            return primary < other.primary;
        return secondary < other.secondary;
    }

}
