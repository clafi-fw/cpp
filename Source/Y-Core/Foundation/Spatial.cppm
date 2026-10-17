export module ClaFi.Core.Foundation :Spatial;

namespace ClaFi
{
    // One axis of a rectangle: where it starts and where it ends along it.
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

}
