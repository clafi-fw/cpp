export module ClaFi.Core.AppTheme_Baked;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // ONE CHANNEL OF A RULE, EVERY OPERATION CARRIED AT A WEIGHT SO TWO OF THEM HAVE A HALF WAY.
    // See AppTheme
    export struct BakedValue
    {
    public:
        [[nodiscard]] bool changesNothing() const;
        void clear();
        float applyTo(float& field, float factor, Lightness) const;
        [[nodiscard]] bool operator==(const BakedValue&) const = default;
    public:
        float offset{};
        float multiplier{ 1.0f };
        float setTarget{};
        float setPull{};
        // The floor this channel is read on - the theme's, k_noFloor for a saturation. See AppTheme
        float luminosityFloor{};
    };

    // THE HUE A RULE PULLS TOWARD AND HOW FAR, a pull of nothing being no hue of its own.
    export struct BakedHue
    {
    public:
        [[nodiscard]] bool changesNothing() const { return setPull <= 0.0f; }
        void clear();
        [[nodiscard]] bool operator==(const BakedHue&) const = default;
    public:
        float hue{};
        float setPull{};
    };

    // A RULE AS THE PAINT PATH READS IT - three channels, each operation held at a weight.
    export struct BakedRule
    {
    public:
        [[nodiscard]] bool changesNothing() const;
        void clear();
        void setExactHsl(Hsl, Lightness);
        float applyTo(Hsl&, float factor, Lightness) const;
        [[nodiscard]] bool operator==(const BakedRule&) const = default;
    public:
        BakedHue hue{};
        BakedValue saturation{};
        BakedValue elevation{};
    };

    // THE RULES ONE ELEMENT PAINTS ITSELF FROM, in the order PaintEvent applies them. See AppTheme
    export struct BakedElement
    {
    public:
        // The lightness this element is read in - the far side of the theme where it flips.
        [[nodiscard]] Lightness lightnessIn(Lightness) const;
        [[nodiscard]] bool operator==(const BakedElement&) const = default;
    public:
        float flip{};
        BakedRule surface{};
        BakedRule stroke{};
        BakedRule active{};
        BakedRule hovered{};
        BakedRule pressed{};
        BakedRule text{};
        BakedRule activeText{};
    };

    export using BakedElements = std::array<BakedElement, k_uiElementCount>;
    export using BakedRules = std::array<BakedRule, k_uiElementCount>;
    export using PigmentHues = std::array<float, k_pigmentsCount>;

    // A THEME AS THE PAINT PATH READS IT, each set standing at the element it belongs to. An
    // element is either a set of state rules or one bare rule, so one of the two arrays carries
    // it and the other stands empty there - see UiElementDescriptor, which states which.
    export struct BakedColors
    {
    public:
        [[nodiscard]] const BakedElement& element(UiElement) const;
        [[nodiscard]] const BakedRule& rule(UiElement) const;
        // Which of this theme's rules an ink names.
        [[nodiscard]] const BakedRule& ruleOf(InkColor) const;
        // A pigment's hue, at InkWell's tone for the side of the theme the painter stands on.
        [[nodiscard]] Hsl pigmentHsl(Pigment, Lightness) const;
        [[nodiscard]] Hsl pigmentHsl(Pigment, InkTone) const;
        // WHAT A FORM ROOT ESTABLISHES: the bare colour of the lightness with the form's own
        // surface rule over it, and the bare ink with its text rule.
        [[nodiscard]] Hsl formSurface() const;
        [[nodiscard]] Hsl formText() const;
        // The colour a window root casts its shadow in, before any opacity. See AppTheme
        [[nodiscard]] Hsl windowShadow(OptionalUiElement root) const;
    public:
        Lightness lightness{ k_darkLightness };
        // The hue a harmony turns around, for a drawing that has to answer where a rule states
        // no hue of its own.
        float anchorHue{};
        BakedElements elements{};
        BakedRules rules{};
        // Each window root's shadow rule, standing at the root and empty at every other element.
        BakedRules shadows{};
        // Every pigment's hue, taken off the harmony as the theme is baked, so nothing
        // downstream has a harmony to derive or a kind to read.
        PigmentHues pigmentHues{};
    };

    // One rule inside a set, named the way a whole set is. It is resolved against the set the
    // pass reading it carries, so a control built once answers for the application's theme and
    // for a theme a subtree was reset to - see AdjustPaintEvent::resetTheme.
    export struct ThemeRule
    {
        OptionalUiElement element{};
        BakedRule BakedElement::* rule{ nullptr };
        [[nodiscard]] const BakedRule* of(const BakedColors&) const;
    };

    // WHETHER TWO SETS STATE THE SAME COLOURS, lightness aside - which is the whole of what the
    // colour factor has to cross. One theme worn in the two modes is one design read at the two
    // lightnesses, so a switch of mode moves nothing at all on this axis.
    export [[nodiscard]] bool sameColors(const BakedColors&, const BakedColors&);

    export [[nodiscard]] BakedValue bake(const ColorRuleValue&, float luminosityFloor);
    export [[nodiscard]] BakedHue bake(const ColorRuleHue&, const ThemeColors&);
    export [[nodiscard]] BakedRule bake(const ColorRule&, const ThemeColors&);
    export [[nodiscard]] BakedElement bake(const ControlColorRules&, const ThemeColors&);
    // A window root's shadow rule, its elevation lifted by no floor. See AppTheme#windowshadow
    export [[nodiscard]] BakedRule bakeShadow(const ColorRule&, const ThemeColors&);

    // CROSSING A SET WITH ITSELF ANSWERS THAT SET AT EVERY FACTOR, which is what the weighted
    // targets below are for. See AppTheme
    export [[nodiscard]] BakedValue blend(const BakedValue& from, const BakedValue& to,
        float factor);
    export [[nodiscard]] BakedHue blend(const BakedHue& from, const BakedHue& to, float factor);
    export [[nodiscard]] BakedRule blend(const BakedRule& from, const BakedRule& to, float factor);
    export [[nodiscard]] BakedElement blend(const BakedElement& from, const BakedElement& to,
        float factor);
    // Two factors, because a theme's lightness crosses on a slot of its own - see
    // AnimationSlots::themeLightness. Everything else answers to the first.
    export [[nodiscard]] BakedColors blend(const BakedColors& from, const BakedColors& to,
        float factor, float lightnessFactor);

    // A mean weighted by the two pulls, so a value whose pull is nothing cannot drag one with any.
    [[nodiscard]] float blendedTarget(float fromValue, float fromWeight, float toValue,
        float toWeight, float factor);
    // The same, taken the short way around the wheel.
    [[nodiscard]] float blendedHue(float fromHue, float fromWeight, float toHue, float toWeight,
        float factor);


    //-------------------------------------------------------------------------


    // BakedValue

    bool BakedValue::changesNothing() const
    {
        return offset == 0.0f
            and multiplier == 1.0f
            and setPull <= 0.0f;
    }

    // The floor is the axis the channel is read on rather than an operation, so it stays.
    void BakedValue::clear()
    {
        offset = 0.0f;
        multiplier = 1.0f;
        setTarget = 0.0f;
        setPull = 0.0f;
    }

    // The field crosses to the elevation axis once, every operation holding a weight runs there in
    // turn, and the result crosses back. A set left at its identity is skipped, so a rule standing
    // at rest - where one operation holds the whole weight - costs the one crossing it always did.
    float BakedValue::applyTo(float& field, float factor, Lightness lightness) const
    {
        if (factor <= 0.0f or changesNothing())
            return 0.0f;

        const float elevation = elevationOf(field, lightness, luminosityFloor);
        float newElevation = elevation;

        if (multiplier != 1.0f)
            newElevation = newElevation * multiplier * factor + newElevation * (1.0f - factor);

        if (offset != 0.0f)
            newElevation = newElevation + offset * factor;

        if (setPull > 0.0f)
            newElevation = newElevation + (setTarget - newElevation) * setPull * factor;

        const float newValue = luminosityOf(std::clamp(newElevation, 0.0f, 1.0f), lightness,
            luminosityFloor);
        if (field == newValue)
            return 0.0f;

        field = newValue;
        return factor;
    }

    // BakedHue

    void BakedHue::clear()
    {
        hue = 0.0f;
        setPull = 0.0f;
    }

    // BakedRule

    bool BakedRule::changesNothing() const
    {
        return hue.changesNothing()
            and saturation.changesNothing()
            and elevation.changesNothing();
    }

    void BakedRule::clear()
    {
        hue.clear();
        saturation.clear();
        elevation.clear();
    }

    void BakedRule::setExactHsl(Hsl value, Lightness lightness)
    {
        hue.hue = value.hue;
        hue.setPull = 1.0f;
        saturation.clear();
        saturation.setTarget = value.saturation;
        saturation.setPull = 1.0f;
        // The rule states an elevation, so the colour's luminosity crosses to that axis.
        elevation.clear();
        elevation.setTarget = elevationOf(value.luminosity, lightness, elevation.luminosityFloor);
        elevation.setPull = 1.0f;
    }

    float BakedRule::applyTo(Hsl& hsl, float factor, Lightness lightness) const
    {
        if (factor <= 0.0f)
            return 0.0f;

        float appliedHueFactor = 0.0f;
        bool doMixHue = false;

        if (hue.setPull > 0.0f and hsl.hue != hue.hue)
        {
            appliedHueFactor = factor * hue.setPull;
            if (hsl.saturation)
                doMixHue = true;
            else
                hsl.hue = hue.hue;
        }

        float appliedSaturationFactor = saturation.applyTo(hsl.saturation, factor, k_darkLightness);
        float appliedElevationFactor = elevation.applyTo(hsl.luminosity, factor, lightness);

        if (doMixHue)
        {
            Hsl target = {
                hue.hue,
                hsl.saturation,
                hsl.luminosity
            };
            hsl = Hsl{ hsl, target, appliedHueFactor };
        }

        return StateFactors::compose({ appliedHueFactor, appliedSaturationFactor,
            appliedElevationFactor });
    }

    // BakedElement

    Lightness BakedElement::lightnessIn(Lightness lightness) const
    {
        return lightness + flip * (1.0f - 2.0f * lightness);
    }

    // bake

    BakedValue bake(const ColorRuleValue& value, float luminosityFloor)
    {
        BakedValue result{};
        result.luminosityFloor = luminosityFloor;
        switch (value.operation())
        {
        case ColorRuleOp::Offset:
            result.offset = value.value();
            break;

        case ColorRuleOp::Scale:
            result.multiplier = value.value();
            break;

        case ColorRuleOp::Set:
            result.setTarget = value.value();
            result.setPull = 1.0f;
            break;

        case ColorRuleOp::NoChange:
            break;
        }
        return result;
    }

    // A pigment names a place in the palette, and the palette is the theme's to read. What comes
    // out is the hue itself, so nothing downstream has a pigment left to resolve.
    BakedHue bake(const ColorRuleHue& hue, const ThemeColors& themeColors)
    {
        if (hue.operation() == ColorRuleHueOp::NoChange)
            return {};

        return {
            hue.actualHue(themeColors, 0.0f),
            1.0f
        };
    }

    BakedRule bake(const ColorRule& rule, const ThemeColors& themeColors)
    {
        return {
            bake(rule.hue, themeColors),
            bake(rule.saturation, k_noFloor),
            bake(rule.elevation, themeColors.darkModeFloor)
        };
    }

    BakedElement bake(const ControlColorRules& rules, const ThemeColors& themeColors)
    {
        return {
            rules.flip ? 1.0f : 0.0f,
            bake(rules.surface, themeColors),
            bake(rules.stroke, themeColors),
            bake(rules.active, themeColors),
            bake(rules.hovered, themeColors),
            bake(rules.pressed, themeColors),
            bake(rules.text, themeColors),
            bake(rules.activeText, themeColors)
        };
    }

    // The floor lifts the theme's own surfaces, and a shadow falls on what is behind the window.
    BakedRule bakeShadow(const ColorRule& rule, const ThemeColors& themeColors)
    {
        return {
            bake(rule.hue, themeColors),
            bake(rule.saturation, k_noFloor),
            bake(rule.elevation, k_noFloor)
        };
    }

    // blend

    float blendedTarget(float fromValue, float fromWeight, float toValue, float toWeight,
        float factor)
    {
        // Two sets naming one target answer it exactly, where the weighting would land a
        // unit in the last place either side of it.
        if (fromValue == toValue)
            return fromValue;

        const float sum = fromWeight + toWeight;
        if (sum <= 0.0f)
            return std::lerp(fromValue, toValue, factor);

        return (fromValue * fromWeight + toValue * toWeight) / sum;
    }

    float blendedHue(float fromHue, float fromWeight, float toHue, float toWeight, float factor)
    {
        if (fromHue == toHue)
            return fromHue;

        const float sum = fromWeight + toWeight;
        if (sum <= 0.0f)
            return std::lerp(fromHue, toHue, factor);

        float difference = toHue - fromHue;
        if (difference > 0.5f)
            difference -= 1.0f;
        if (difference < -0.5f)
            difference += 1.0f;

        float result = fromHue + difference * (toWeight / sum);
        result -= std::floor(result);
        return result;
    }

    bool sameColors(const BakedColors& a, const BakedColors& b)
    {
        return a.anchorHue == b.anchorHue
            and a.elements == b.elements
            and a.rules == b.rules
            and a.shadows == b.shadows
            and a.pigmentHues == b.pigmentHues;
    }

    BakedValue blend(const BakedValue& from, const BakedValue& to, float factor)
    {
        return {
            std::lerp(from.offset, to.offset, factor),
            std::lerp(from.multiplier, to.multiplier, factor),
            blendedTarget(from.setTarget, from.setPull * (1.0f - factor),
                to.setTarget, to.setPull * factor, factor),
            std::lerp(from.setPull, to.setPull, factor),
            std::lerp(from.luminosityFloor, to.luminosityFloor, factor)
        };
    }

    BakedHue blend(const BakedHue& from, const BakedHue& to, float factor)
    {
        return {
            blendedHue(from.hue, from.setPull * (1.0f - factor), to.hue, to.setPull * factor,
                factor),
            std::lerp(from.setPull, to.setPull, factor)
        };
    }

    BakedRule blend(const BakedRule& from, const BakedRule& to, float factor)
    {
        return {
            blend(from.hue, to.hue, factor),
            blend(from.saturation, to.saturation, factor),
            blend(from.elevation, to.elevation, factor)
        };
    }

    BakedElement blend(const BakedElement& from, const BakedElement& to, float factor)
    {
        return {
            std::lerp(from.flip, to.flip, factor),
            blend(from.surface, to.surface, factor),
            blend(from.stroke, to.stroke, factor),
            blend(from.active, to.active, factor),
            blend(from.hovered, to.hovered, factor),
            blend(from.pressed, to.pressed, factor),
            blend(from.text, to.text, factor),
            blend(from.activeText, to.activeText, factor)
        };
    }

    // BakedColors

    const BakedElement& BakedColors::element(UiElement value) const
    {
        return elements[static_cast<std::size_t>(value)];
    }

    const BakedRule& BakedColors::rule(UiElement value) const
    {
        return rules[static_cast<std::size_t>(value)];
    }

    const BakedRule& BakedColors::ruleOf(InkColor value) const
    {
        switch (value)
        {
            case InkColor::Accent:
                return rule(UiElement::Accent);
            case InkColor::Spot:
                return rule(UiElement::Spot);
            case InkColor::Text:
            case InkColor::Yellow:
            case InkColor::Green:
            case InkColor::Blue:
            case InkColor::Red:
            case InkColor::Black:
            case InkColor::White:
            case InkColor::Count:
                break;
        }
        unreachable("an ink names a rule this theme does not hold");
    }

    // The two tones are one colour stated once for each side of the theme rather than one the
    // mode would orient, so a theme standing between the sides is drawn between the tones.
    Hsl BakedColors::pigmentHsl(Pigment pigment, Lightness aLightness) const
    {
        const PigmentTones tones = InkWell::pigmentTones(pigment);
        InkTone tone = {
            std::lerp(tones.dark.saturation, tones.light.saturation, aLightness),
            std::lerp(tones.dark.luminosity, tones.light.luminosity, aLightness)
        };
        return pigmentHsl(pigment, tone);
    }

    Hsl BakedColors::pigmentHsl(Pigment pigment, InkTone tone) const
    {
        return {
            pigmentHues[static_cast<std::size_t>(pigment)],
            std::clamp(tone.saturation, 0.0f, 1.0f),
            std::clamp(tone.luminosity, 0.0f, 1.0f)
        };
    }

    Hsl BakedColors::formSurface() const
    {
        Hsl result = { 0.0f, 0.0f, luminosityOf(0.0f, lightness) };
        element(UiElement::Form).surface.applyTo(result, 1.0f, lightness);
        return result;
    }

    // The bare ink of the lightness, carrying the form surface's hue so that a text rule raising
    // saturation alone tints toward the theme's own family rather than toward red.
    Hsl BakedColors::formText() const
    {
        Hsl result = { formSurface().hue, 0.0f, luminosityOf(1.0f, lightness) };
        element(UiElement::Form).text.applyTo(result, 1.0f, lightness);
        return result;
    }

    // Read at the dark end whatever the lightness - a shadow is the absence of light on both sides.
    Hsl BakedColors::windowShadow(OptionalUiElement root) const
    {
        Hsl result = { formSurface().hue, 0.0f, 0.0f };
        if (!root)
            return result;
        shadows[static_cast<std::size_t>(*root)].applyTo(result, 1.0f, k_darkLightness);
        return result;
    }

    // ThemeRule

    const BakedRule* ThemeRule::of(const BakedColors& colors) const
    {
        if (!element or rule == nullptr)
            return nullptr;

        return &(colors.element(*element).*rule);
    }

    BakedColors blend(const BakedColors& from, const BakedColors& to, float factor,
        float lightnessFactor)
    {
        BakedColors result{};
        result.lightness = std::lerp(from.lightness, to.lightness, lightnessFactor);
        result.anchorHue = blendedHue(from.anchorHue, 1.0f - factor, to.anchorHue, factor,
            factor);

        for (std::size_t i = 0; i < result.elements.size(); ++i)
            result.elements[i] = blend(from.elements[i], to.elements[i], factor);

        for (std::size_t i = 0; i < result.rules.size(); ++i)
            result.rules[i] = blend(from.rules[i], to.rules[i], factor);

        for (std::size_t i = 0; i < result.shadows.size(); ++i)
            result.shadows[i] = blend(from.shadows[i], to.shadows[i], factor);

        for (std::size_t i = 0; i < result.pigmentHues.size(); ++i)
            result.pigmentHues[i] = blendedHue(from.pigmentHues[i], 1.0f - factor,
                to.pigmentHues[i], factor, factor);

        return result;
    }

}
