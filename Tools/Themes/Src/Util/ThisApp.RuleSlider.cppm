export module ThisApp.RuleSlider;

import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Slider;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    /// Which field of a colour a rule value drives. Saturation and elevation are the two a
    /// ColorRuleValue is ever pointed at, and they differ in more than the field: elevation is
    /// signed by the theme's mode, saturation is not.
    export enum class RuleChannel
    {
        Saturation,
        Elevation
    };

    /// What a ramp is drawn from: the colour the rule is about to change, and the sign the theme
    /// gives elevation.
    ///
    /// Both belong to the theme being edited, which is not the theme this control is painted in.
    /// The editor keeps its own appearance while the theme under the cursor is another one
    /// entirely, so nothing here may be read off the paint event.
    export struct RuleBase
    {
        Hsl color{};
        ColorMode colorMode{ ColorMode::Dark };
        /// The theme's dark mode floor, which the rule's axis is lifted by.
        float luminosityFloor{};
    };

    /// Asked at paint time rather than bound as a value, because every other rule in the theme
    /// can move it - its colour mode included - and the slider is not told when they do.
    export using OnGetRuleBase = std::function<RuleBase()>;

    /// A grid slider whose slot shows what its value will do: for each position along the slot, the
    /// rule applied at that value to the colour the element sits on.
    ///
    /// The ramp is a legend for the action, not a rendering of the result. A colour close to the
    /// real one that always shows the action beats the exact one, which over much of a theme's
    /// range carries no visible change at all - a saturation sweep over white is white the whole
    /// way, and a luminosity sweep at low saturation is a grey ramp that says nothing about the
    /// hue it will be applied to. So the colour the ramp is drawn from is pulled into the band
    /// where the action reads, and only there: the position of every stop, and the shape the
    /// operation gives the ramp, stay exactly what the rule will do.
    ///
    /// Sampled into a gradient rather than a bitmap. A page of these is on screen at once, and a
    /// bitmap each, rebuilt on every edit, is what the theme editor cannot afford - but every ramp
    /// here carries a single hue, the value driving saturation or luminosity alone, so a handful of
    /// stops follow the curve as closely as pixels would and the fill is one call.
    export class RuleSlider : public Slider
    {
    public:
        template <typename... Args>
        RuleSlider(const CreateParams&, Args&&...);
        /// The value this slider edits, the field it drives, and where its base colour comes from.
        /// Bound per row: a cell is built from a blueprint that names a column, and only the row
        /// says which rule that column stands for.
        void bind(const ColorRuleValue&, RuleChannel, OnGetRuleBase);
    protected:
        void paintSlot(PaintEvent&, const FloatRect&, SlotSpan) override;
        Color thumbColor(PaintEvent&, FloatPoint&) override;
    private:
        [[nodiscard]] bool hasRamp() const;
        [[nodiscard]] Hsl rampColor(const Hsl& base) const;
        [[nodiscard]] Color sampleAt(float position, const Hsl& base, ColorMode,
            float luminosityFloor) const;
    private:
        // Enough to carry the bend in an HSL to RGB curve. The ramp has one hue in it, so what a
        // stop has to follow is a single channel's response, and nine points leave no step a reader
        // can find at the widths a grid column runs to.
        inline static constexpr std::size_t k_stopCount{ 9ull };
        // The band a saturation sweep is legible in. Everything is white above it and black
        // below, whatever the saturation, so a ramp drawn outside it shows one flat colour.
        inline static constexpr float k_minShownLuminosity{ 0.25f };
        inline static constexpr float k_maxShownLuminosity{ 0.75f };
        // The saturation an elevation sweep is drawn at, so that the ramp carries the hue the
        // rule has chosen rather than running grey.
        inline static constexpr float k_minShownElevationSaturation{ 0.1f };
        const ColorRuleValue* m_ruleValue{};
        RuleChannel m_channel{};
        OnGetRuleBase m_ruleBase{};
    };


    //----------------------------------------------------------------------------


    template <typename... Args>
    RuleSlider::RuleSlider(const CreateParams& params, Args&&... args)
        :
        Slider{ params, std::forward<Args>(args)... }
    {
    }

    void RuleSlider::bind(const ColorRuleValue& ruleValue, RuleChannel channel, OnGetRuleBase ruleBase)
    {
        m_ruleValue = &ruleValue;
        m_channel = channel;
        m_ruleBase = std::move(ruleBase);
        invalidate();
    }

    void RuleSlider::paintSlot(PaintEvent& event, const FloatRect& slotRect, SlotSpan span)
    {
        if (!hasRamp())
        {
            Slider::paintSlot(event, slotRect, span);
            return;
        }

        const RuleBase ruleBase = m_ruleBase();
        const Hsl base = rampColor(ruleBase.color);

        Graphics::LinearGradient ramp{
            .startPoint = { slotRect.left, slotRect.centerY() },
            .endPoint = { slotRect.right, slotRect.centerY() },
        };
        ramp.stops.reserve(k_stopCount);
        for (std::size_t i = 0ull; i != k_stopCount; ++i)
        {
            const float position = static_cast<float>(i) / static_cast<float>(k_stopCount - 1ull);
            ramp.stops.push_back({
                position,
                event.applyDisabledFactor(sampleAt(span.at(position), base, ruleBase.colorMode,
                    ruleBase.luminosityFloor))
            });
        }

        const float radius = slotRect.height() / 2.0f;
        event.canvas().fillRoundedRectangle(slotRect, radius, radius, Graphics::Brush{ ramp });
        paintSlotBorder(event, slotRect, radius);
    }

    // The thumb carries the colour the ramp has under it, so that the hole in it reads as the
    // slot showing through rather than as a mark of its own. Sampled at the thumb's position,
    // which is the rule's own value: what shows in the hole is what the rule does.
    Color RuleSlider::thumbColor(PaintEvent& event, FloatPoint& point)
    {
        if (!hasRamp())
            return Slider::thumbColor(event, point);
        const RuleBase ruleBase = m_ruleBase();
        // Not faded here: paintThumb puts what this returns through applyDisabledFactor.
        return sampleAt(relativePosition(), rampColor(ruleBase.color), ruleBase.colorMode,
            ruleBase.luminosityFloor);
    }

    // Whether there is a sweep to show. The ramp states what the rule does across the slot,
    // so a rule that does nothing has nothing to draw: NoChange leaves the field alone at
    // every position, and sampling it gives the element's own colour at every stop - a flat
    // band in whatever colour that row happens to carry, which is not a rule preview at all.
    // The plain slot underneath says the same thing in the one form every row shares.
    //
    // A vertical slider is not what the grid builds, and the ramp runs along the slot, so the
    // one axis this answers for is the one it is used on.
    bool RuleSlider::hasRamp() const
    {
        return m_ruleValue
            and m_ruleBase
            and m_ruleValue->operation() != ColorRuleOp::NoChange
            and axis() == ScrollAxis::Horizontal;
    }

    Hsl RuleSlider::rampColor(const Hsl& base) const
    {
        Hsl result = base;
        if (m_channel == RuleChannel::Saturation)
        {
            result.luminosity = std::clamp(result.luminosity, k_minShownLuminosity, k_maxShownLuminosity);
            return result;
        }

        // It's Elevation channel
        result.saturation = (std::max)(result.saturation, k_minShownElevationSaturation);
        return result;
    }

    Color RuleSlider::sampleAt(float position, const Hsl& base, ColorMode colorMode,
        float luminosityFloor) const
    {
        // A probe rather than the rule itself: the ramp asks what every position would do, and the
        // rule holds only the one the user has settled on. The operation comes with the copy, so a
        // ramp under Offset reads as no change at its middle and under Scale as unchanged at half.
        ColorRuleValue probe = *m_ruleValue;
        probe.setNormalizedValue(position);
        Hsl result = base;
        if (m_channel == RuleChannel::Saturation)
            probe.applyTo(result.saturation, 1.0f, ColorMode::Dark, k_noFloor);
        else
            probe.applyTo(result.luminosity, 1.0f, colorMode, luminosityFloor);
        return result.toColor();
    }

}
