module ClaFi.Core.Foundation;

import :Spatial;

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

}
