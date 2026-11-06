export module ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // One of the sizes ControlMetrics holds, named apart from the others. See AppTheme
    export enum class MetricsField
    {
        Border,
        Band,
        Radius,
        Padding,
        Spacing,
        MinSize,
        PreferredSize,
        MaxSize,
        Depth,
        GrowIn,
        ShadowBlur,
        ShadowOffset,
        ShadowSpread,
        ShadowOpacity,
        Count
    };

    // The sizes a control was given outright, over the style it is sized by. See AppTheme
    export class MetricsFields
    {
    public:
        [[nodiscard]] constexpr bool has(MetricsField field) const
        {
            return (m_bits & bitOf(field)) != 0u;
        }
        constexpr void add(MetricsField field) { m_bits |= bitOf(field); }
        // Every size, which is what a block given whole states.
        [[nodiscard]] static constexpr MetricsFields every()
        {
            MetricsFields result{};
            result.m_bits = (1u << static_cast<std::uint32_t>(MetricsField::Count)) - 1u;
            return result;
        }
    private:
        [[nodiscard]] static constexpr std::uint32_t bitOf(MetricsField field)
        {
            return 1u << static_cast<std::uint32_t>(field);
        }
    private:
        std::uint32_t m_bits{ 0u };
    };

    // The shadow a window casts, in logical pixels. See AppTheme
    export struct WindowShadow
    {
        float blur{};
        FloatPoint offset{};
        float spread{};
        float opacity{};
        [[nodiscard]] bool operator==(const WindowShadow&) const = default;
    };

    // The design metrics a control is laid out by.
    export class ControlMetrics
    {
    public:
        ControlMetrics() = default;
    public:
        void setFixedWidth(float value) { minSize.x = maxSize.x = value; }
        void setFixedHeight(float value) { minSize.y = maxSize.y = value; }
        void setFixedSize(FixedSize value) {
            minSize = value;
            maxSize = value;
        }
        // Takes from source every size named in fields, and keeps the rest.
        void copyFields(const ControlMetrics& source, MetricsFields fields);
        [[nodiscard]] bool operator==(const ControlMetrics&) const = default;
    public:
        Thickness border{};
        float radius{};
        // The width of the band the control draws along one edge, none at 0. See AppTheme
        float band{};
        Padding padding{};
        Spacing spacing{};
        MinSize minSize{};
        // **What this control asks for when it is free to choose**, per axis. Zero states no
        // preference on that axis and the content decides it, which is the usual case; a stated
        // one is the whole box, padding included, and stands in place of what the content
        // measured. Held between minSize and maxSize like any other answer.
        //
        // It is the third thing a size can be, and the other two cannot say it: a minimum is
        // never smaller, a maximum is never bigger, and neither of them is "this size, unless
        // something with a better claim decides otherwise" - a dialog that opens at a stated size
        // and is still resizable, a pane that starts at a third of the split.
        PreferredSize preferredSize{};
        MaxSize maxSize{ k_maxFloat, k_maxFloat };
        // How deep into the surface this style of control sits, as a multiple of the depths in
        // ThemeMetrics. It governs the whole of the Z movement and not one end of it: the resting
        // depth a control rises out of under the pointer, and the further depth it sinks to while
        // held, are one push seen at two distances, and this scales both. Flush is the fixed point
        // and stays flush at any value.
        //
        // 1 is the theme's depth. 0 opts the style out. Above 1 overstates, which is worth doing
        // only where the projection has left a control too small for its travel to be seen.
        float zDepthFactor{};
        // The size the surface arrives from as it appears, a fraction of the size it settles at.
        float surfaceGrowInScale{ 1.0f };
        // The shadow the control's window casts, where the control is a window's root. See AppTheme
        WindowShadow shadow{};
    };

    export struct ScaledControlMetrics
    {
        ScaledControlMetrics(const ControlMetrics& designMetrics, const Scaler&);
        ScaledControlMetrics& operator=(const ScaledControlMetrics&) = default;
        ScaledPadding padding;
        ScaledSpacing spacing;
        ScaledSize minSize;
        // Zero on an axis means the content decides it - see ControlMetrics::preferredSize.
        ScaledSize preferredSize; // Stated
        ScaledSize maxSize;
    };

    export using OptionalThickness = std::optional<Thickness>;
    export using OptionalFloat = std::optional<float>;
    export using OptionalPadding = std::optional<Padding>;
    export using OptionalSpacing = std::optional<Spacing>;
    export using OptionalMinSize = std::optional<MinSize>;
    export using OptionalPreferredSize = std::optional<PreferredSize>;
    export using OptionalFloatPoint = std::optional<FloatPoint>;

    // The sizes a style states, each one over its base's. See AppTheme
    export struct StatedMetrics
    {
    public:
        // Puts every stated size into the metrics, and leaves the rest as they are.
        void applyTo(ControlMetrics&) const;
        [[nodiscard]] bool empty() const { return count() == 0ull; }
        [[nodiscard]] std::size_t count() const; // how many sizes are stated
        [[nodiscard]] bool operator==(const StatedMetrics&) const = default;
    public:
        OptionalThickness border{};
        OptionalFloat band{};
        OptionalFloat radius{};
        OptionalPadding padding{};
        OptionalSpacing spacing{};
        OptionalMinSize minSize{};
        OptionalPreferredSize preferredSize{};
        OptionalFloat depth{};  // ControlMetrics zDepthFactor
        OptionalFloat growIn{}; // ControlMetrics surfaceGrowInScale
        OptionalFloat shadowBlur{};
        OptionalFloatPoint shadowOffset{};
        OptionalFloat shadowSpread{};
        OptionalFloat shadowOpacity{};
    };

    // The sizes every style shares. See AppTheme
    export namespace ThemeMetrics
    {
        constexpr Thickness Border = Thickness::Thin;
        // The widest stroke a control can paint: a focused one thickens its border to this.
        // Anything laid over a control's edge has to leave at least this much clear, or it covers
        // the focus ring on the side it reaches.
        constexpr Thickness FocusedBorder = Thickness::Regular;
        // The Z animation stated as depth. A control rests PressRestDepth into the surface, rises
        // flush with it under the pointer or the keyboard's focus and sinks to PressHeldDepth
        // while held - the same distances for every control, so nothing on a surface travels
        // further into it than anything else does.
        //
        // Hover is exactly flush on purpose: nothing ever paints outside its own bounds, so no
        // control needs an inflated invalidate rect or an escape from its parent's clip. It is also
        // why the two depths cannot be scaled apart - flush is where both meet, so anything scaling
        // one of them from there scales the other. ControlMetrics::zDepthFactor is that scale, and
        // Control::allowZAnimation is what decides whether a control has any of this at all.
        //
        // ViewerDistance is where the eye is, in the same points. It is a stylistic choice and not
        // a measured one: a real viewer sits some two thousand points from the screen, and at that
        // distance a two point press projects to under a twentieth of a pixel on anything. This is
        // a far closer, far wider angle than reality, picked so the movement registers at all.
        constexpr float ViewerDistance = 98.0f;
        constexpr float PressRestDepth = 2.0f;
        constexpr float PressHeldDepth = 3.0f;

        // Perspective projection, and the reason a small control moves less than a large one for
        // the same push. The ratio carries no size term - one depth gives one ratio to everything -
        // so what differs between controls is not how far they travel but how much of that travel
        // reaches the screen, which is proportional to how big they are. Deriving the ratio rather
        // than authoring it is what keeps the depths comparable to each other and to a real one.
        constexpr float PressRestScale = ViewerDistance / (ViewerDistance + PressRestDepth);
        constexpr float PressHeldScale = ViewerDistance / (ViewerDistance + PressHeldDepth);
    }


    //-------------------------------------------------------------------------


    // ControlMetrics

    void ControlMetrics::copyFields(const ControlMetrics& source, const MetricsFields fields)
    {
        if (fields.has(MetricsField::Border))
            border = source.border;
        if (fields.has(MetricsField::Band))
            band = source.band;
        if (fields.has(MetricsField::Radius))
            radius = source.radius;
        if (fields.has(MetricsField::Padding))
            padding = source.padding;
        if (fields.has(MetricsField::Spacing))
            spacing = source.spacing;
        if (fields.has(MetricsField::MinSize))
            minSize = source.minSize;
        if (fields.has(MetricsField::PreferredSize))
            preferredSize = source.preferredSize;
        if (fields.has(MetricsField::MaxSize))
            maxSize = source.maxSize;
        if (fields.has(MetricsField::Depth))
            zDepthFactor = source.zDepthFactor;
        if (fields.has(MetricsField::GrowIn))
            surfaceGrowInScale = source.surfaceGrowInScale;
        if (fields.has(MetricsField::ShadowBlur))
            shadow.blur = source.shadow.blur;
        if (fields.has(MetricsField::ShadowOffset))
            shadow.offset = source.shadow.offset;
        if (fields.has(MetricsField::ShadowSpread))
            shadow.spread = source.shadow.spread;
        if (fields.has(MetricsField::ShadowOpacity))
            shadow.opacity = source.shadow.opacity;
    }


    // ScaledControlMetrics

    ScaledControlMetrics::ScaledControlMetrics(const ControlMetrics& designMetrics, const Scaler& scaler)
        :
        padding{        scaler.scale(designMetrics.padding) },
        spacing{        scaler.scale(designMetrics.spacing) },
        minSize{        scaler.scale(designMetrics.minSize) },
        preferredSize{  scaler.scale(designMetrics.preferredSize) },
        maxSize{        scaler.scale(designMetrics.maxSize) }
    {
    }

    // StatedMetrics

    void StatedMetrics::applyTo(ControlMetrics& metrics) const
    {
        if (border)
            metrics.border = *border;
        if (band)
            metrics.band = *band;
        if (radius)
            metrics.radius = *radius;
        if (padding)
            metrics.padding = *padding;
        if (spacing)
            metrics.spacing = *spacing;
        if (minSize)
            metrics.minSize = *minSize;
        if (preferredSize)
            metrics.preferredSize = *preferredSize;
        if (depth)
            metrics.zDepthFactor = *depth;
        if (growIn)
            metrics.surfaceGrowInScale = *growIn;
        if (shadowBlur)
            metrics.shadow.blur = *shadowBlur;
        if (shadowOffset)
            metrics.shadow.offset = *shadowOffset;
        if (shadowSpread)
            metrics.shadow.spread = *shadowSpread;
        if (shadowOpacity)
            metrics.shadow.opacity = *shadowOpacity;
    }

    std::size_t StatedMetrics::count() const
    {
        const std::array stated = {
            border.has_value(),
            band.has_value(),
            radius.has_value(),
            padding.has_value(),
            spacing.has_value(),
            minSize.has_value(),
            preferredSize.has_value(),
            depth.has_value(),
            growIn.has_value(),
            shadowBlur.has_value(),
            shadowOffset.has_value(),
            shadowSpread.has_value(),
            shadowOpacity.has_value(),
        };
        return static_cast<std::size_t>(std::ranges::count(stated, true));
    }
}
