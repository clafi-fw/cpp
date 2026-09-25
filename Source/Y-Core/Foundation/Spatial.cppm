export module ClaFi.Core.Foundation :Spatial;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // One axis of a rectangle, in the space a directional search reads it in.
    export struct AxisSpan
    {
        float start{};
        float end{};
        [[nodiscard]] float center() const { return (start + end) / 2.0f; }
        [[nodiscard]] float extent() const { return end - start; }
        [[nodiscard]] bool covers(AxisSpan other) const { return other.start >= start && other.end <= end; }
        [[nodiscard]] bool contains(float value) const { return value >= start && value <= end; }
        // Zero while the value lies within the span, and the distance to the nearer end outside it.
        [[nodiscard]] float distanceTo(float value) const;
        // The value brought inside the span, and just inside: rectangles that tile share an edge,
        // so a value left sitting on one answers for both neighbours and the tie falls to
        // enumeration order.
        [[nodiscard]] float clampInto(float value) const;
        void offset(float value);
        bool operator==(const AxisSpan& other) const { return start == other.start && end == other.end; }
    };

    // A rectangle turned so travel runs up the primary axis. See Control-Foundation
    export struct OrientedRect
    {
        AxisSpan primary{};
        AxisSpan secondary{};
        [[nodiscard]] static OrientedRect orient(const FloatRect&, KeyCode);
        // A single point, as the empty rectangle standing on it. The line a move keeps is one
        // coordinate, and it has to be mirrored with everything it is compared against.
        [[nodiscard]] static OrientedRect orient(FloatPoint, KeyCode);
        void implode();
        bool operator==(const OrientedRect& other) const { return primary == other.primary && secondary == other.secondary; }
    };

    // How one candidate is ranked against another. See Control-Foundation
    export struct Score
    {
        bool inLane{};
        float primary{ k_maxFloat };
        float secondary{ k_maxFloat };
        [[nodiscard]] bool isBetterThan(const Score&) const;
    };

}
