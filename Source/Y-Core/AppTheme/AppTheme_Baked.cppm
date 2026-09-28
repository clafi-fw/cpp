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

    // AN EFFECT AS THE PAINT PATH READS IT - three channels, each operation held at a weight.
    export struct BakedEffect
    {
    public:
        [[nodiscard]] bool changesNothing() const;
        void clear();
        void setExactHsl(Hsl, Lightness);
        float applyTo(Hsl&, float factor, Lightness) const;
        [[nodiscard]] bool operator==(const BakedEffect&) const = default;
    public:
        BakedHue hue{};
        BakedValue saturation{};
        BakedValue elevation{};
    };

    // A RULE AS THE PAINT PATH READS IT.
    export struct BakedColorRule
    {
    public:
        [[nodiscard]] bool atRest() const { return inputs.empty() and andInputs.empty(); }
        // How far the rule applies at these levels: its two sets joined as "and".
        [[nodiscard]] float levelIn(const RuleInputLevels&) const;
        [[nodiscard]] bool operator==(const BakedColorRule&) const = default;
    public:
        RuleInputs inputs{};
        RuleInputs andInputs{};
        PaintChannel output{ PaintChannel::Surface };
        BakedEffect effect{};
    };

    export using BakedColorRules = std::vector<BakedColorRule>;

    // THE RULES AS THE PAINT PATH READS THEM - each element's own, the window's and the shared.
    export struct BakedRules
    {
    public:
        [[nodiscard]] const BakedColorRules& of(UiElement element) const
        {
            return elements[static_cast<std::size_t>(element)];
        }
        [[nodiscard]] bool operator==(const BakedRules&) const = default;
    public:
        BakedColorRules shared{};
        BakedColorRules anyWindow{};
        BakedColorRules focusRing{};
        std::array<BakedColorRules, k_uiElementCount> elements{}; // indexed by UiElement
    };

    export using BakedEffects = std::array<BakedEffect, k_uiElementCount>;
    export using PigmentHues = std::array<float, k_pigmentsCount>;

    // A THEME AS THE PAINT PATH READS IT: every element's rules, and the effects an ink names.
    export struct BakedColors
    {
    public:
        [[nodiscard]] const BakedEffect& effect(UiElement) const;
        // Which of this theme's effects an ink names.
        [[nodiscard]] const BakedEffect& effectOf(InkColor) const;
        // A pigment's hue, at InkWell's tone for the side of the theme the painter stands on.
        [[nodiscard]] Hsl pigmentHsl(Pigment, Lightness) const;
        [[nodiscard]] Hsl pigmentHsl(Pigment, InkTone) const;
        // Where a form root starts: the bare colour and ink of the lightness, at the anchor hue.
        [[nodiscard]] Hsl rootSurface() const;
        [[nodiscard]] Hsl rootText() const;
        // Black at the anchor hue, where the shadow channel starts at a form root.
        [[nodiscard]] Hsl bareShadow() const;
        // The colour a window root casts its shadow in, before any opacity. See AppTheme
        [[nodiscard]] Hsl windowShadow(OptionalUiElement root, float windowFocusedFactor) const;
    public:
        Lightness lightness{ k_darkLightness };
        // The hue a harmony turns around, for a drawing that has to answer where a rule states
        // no hue of its own.
        float anchorHue{};
        BakedEffects effects{}; // an element's bare effect, where UiElementDescriptor names one
        // Every pigment's hue, taken off the harmony as the theme is baked, so nothing
        // downstream has a harmony to derive or a kind to read.
        PigmentHues pigmentHues{};
        BakedRules rules{};
    };

    // WHETHER TWO SETS STATE THE SAME COLOURS, lightness aside - which is the whole of what the
    // colour factor has to cross. One theme worn in the two modes is one design read at the two
    // lightnesses, so a switch of mode moves nothing at all on this axis.
    export [[nodiscard]] bool sameColors(const BakedColors&, const BakedColors&);

    export [[nodiscard]] BakedValue bake(const ColorRuleValue&, float luminosityFloor);
    export [[nodiscard]] BakedHue bake(const ColorRuleHue&, const ThemeColors&);
    export [[nodiscard]] BakedEffect bake(const ColorEffect&, const ThemeColors&);
    // A window root's shadow rule, its elevation lifted by no floor. See AppTheme#windowshadow
    export [[nodiscard]] BakedEffect bakeShadow(const ColorEffect&, const ThemeColors&);
    // A shadow rule's effect is baked the way bakeShadow bakes, every other one the way bake does.
    export [[nodiscard]] BakedColorRule bake(const ColorRule&, const ThemeColors&);
    export [[nodiscard]] BakedColorRules bake(const ColorRules&, const ThemeColors&);
    export [[nodiscard]] BakedRules bake(const ThemeRules&, const ThemeColors&);

    // CROSSING A SET WITH ITSELF ANSWERS THAT SET AT EVERY FACTOR, which is what the weighted
    // targets below are for. See AppTheme
    export [[nodiscard]] BakedValue blend(const BakedValue& from, const BakedValue& to,
        float factor);
    export [[nodiscard]] BakedHue blend(const BakedHue& from, const BakedHue& to, float factor);
    export [[nodiscard]] BakedEffect blend(const BakedEffect& from, const BakedEffect& to,
        float factor);
    // Rule by rule where both lists state the same rules in one order, the destination otherwise.
    export [[nodiscard]] BakedColorRules blend(const BakedColorRules& from,
        const BakedColorRules& to, float factor);
    export [[nodiscard]] BakedRules blend(const BakedRules& from, const BakedRules& to,
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

    // BakedEffect

    bool BakedEffect::changesNothing() const
    {
        return hue.changesNothing()
            and saturation.changesNothing()
            and elevation.changesNothing();
    }

    void BakedEffect::clear()
    {
        hue.clear();
        saturation.clear();
        elevation.clear();
    }

    void BakedEffect::setExactHsl(Hsl value, Lightness lightness)
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

    float BakedEffect::applyTo(Hsl& hsl, float factor, Lightness lightness) const
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

    // BakedColorRule

    float BakedColorRule::levelIn(const RuleInputLevels& levels) const
    {
        return inputs.anyLevelIn(levels) * andInputs.allLevelIn(levels);
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

    BakedEffect bake(const ColorEffect& rule, const ThemeColors& themeColors)
    {
        return {
            bake(rule.hue, themeColors),
            bake(rule.saturation, k_noFloor),
            bake(rule.elevation, themeColors.darkModeFloor)
        };
    }

    // The floor lifts the theme's own surfaces, and a shadow falls on what is behind the window.
    BakedEffect bakeShadow(const ColorEffect& rule, const ThemeColors& themeColors)
    {
        return {
            bake(rule.hue, themeColors),
            bake(rule.saturation, k_noFloor),
            bake(rule.elevation, k_noFloor)
        };
    }

    BakedColorRule bake(const ColorRule& rule, const ThemeColors& themeColors)
    {
        return {
            .inputs = rule.inputs,
            .andInputs = rule.andInputs,
            .output = rule.output,
            .effect = rule.output == PaintChannel::Shadow
                ? bakeShadow(rule.effect, themeColors)
                : bake(rule.effect, themeColors)
        };
    }

    BakedColorRules bake(const ColorRules& rules, const ThemeColors& themeColors)
    {
        BakedColorRules result{};
        result.reserve(rules.size());
        for (const ColorRule& rule : rules)
            result.push_back(bake(rule, themeColors));
        return result;
    }

    BakedRules bake(const ThemeRules& rules, const ThemeColors& themeColors)
    {
        BakedRules result{};
        result.shared = bake(rules.shared, themeColors);
        result.anyWindow = bake(rules.anyWindow, themeColors);
        result.focusRing = bake(rules.focusRing, themeColors);
        for (std::size_t i = 0; i < result.elements.size(); ++i)
            result.elements[i] = bake(rules.elements[i], themeColors);
        return result;
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
            and a.effects == b.effects
            and a.pigmentHues == b.pigmentHues
            and a.rules == b.rules;
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

    BakedEffect blend(const BakedEffect& from, const BakedEffect& to, float factor)
    {
        return {
            blend(from.hue, to.hue, factor),
            blend(from.saturation, to.saturation, factor),
            blend(from.elevation, to.elevation, factor)
        };
    }

    // Two lists that differ in what their rules read or write have no half way between them.
    BakedColorRules blend(const BakedColorRules& from, const BakedColorRules& to, float factor)
    {
        const bool sameRules = std::ranges::equal(from, to,
            [](const BakedColorRule& fromRule, const BakedColorRule& toRule) {
                return fromRule.inputs == toRule.inputs
                    and fromRule.andInputs == toRule.andInputs
                    and fromRule.output == toRule.output;
            });
        if (!sameRules)
            return to;
        BakedColorRules result = to;
        for (std::size_t i = 0; i < result.size(); ++i)
            result[i].effect = blend(from[i].effect, to[i].effect, factor);
        return result;
    }

    // Element by element, so a change to one list leaves the others crossing smoothly.
    BakedRules blend(const BakedRules& from, const BakedRules& to, float factor)
    {
        BakedRules result{};
        result.shared = blend(from.shared, to.shared, factor);
        result.anyWindow = blend(from.anyWindow, to.anyWindow, factor);
        result.focusRing = blend(from.focusRing, to.focusRing, factor);
        for (std::size_t i = 0; i < result.elements.size(); ++i)
            result.elements[i] = blend(from.elements[i], to.elements[i], factor);
        return result;
    }

    // BakedColors

    const BakedEffect& BakedColors::effect(UiElement value) const
    {
        return effects[static_cast<std::size_t>(value)];
    }

    const BakedEffect& BakedColors::effectOf(InkColor value) const
    {
        switch (value)
        {
            case InkColor::Accent:
                return effect(UiElement::Accent);
            case InkColor::Spot:
                return effect(UiElement::Spot);
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

    Hsl BakedColors::rootSurface() const
    {
        return { anchorHue, 0.0f, luminosityOf(0.0f, lightness) };
    }

    // The bare ink of the lightness. PaintEvent gives it the hue of the surface it is drawn on.
    Hsl BakedColors::rootText() const
    {
        return { rootSurface().hue, 0.0f, luminosityOf(1.0f, lightness) };
    }

    Hsl BakedColors::bareShadow() const
    {
        return { rootSurface().hue, 0.0f, 0.0f };
    }

    // Read at the dark end whatever the lightness - a shadow is the absence of light on both sides.
    // The root's own rules, the window's and the shared ones, in the order PaintEvent takes - a
    // root wearing no element takes the window's alone. A window has no state but its focus, so a
    // rule reading only other inputs does not reach it. The shadow starts in the hue of the
    // window's stroke, itself resolved from the root's surface, so a shadow rule naming no hue
    // follows the stroke.
    Hsl BakedColors::windowShadow(OptionalUiElement root, float windowFocusedFactor) const
    {
        const BakedColorRules* own = root ? &rules.of(*root) : nullptr;
        const BakedColorRules* shared = root ? &rules.shared : nullptr;
        RuleInputLevels levels = {};
        levels[static_cast<std::size_t>(RuleInput::WindowFocused)] = windowFocusedFactor;
        auto applyRules = [&](const PaintChannel channel, Hsl& color){
            const Lightness readIn = channel == PaintChannel::Shadow ? k_darkLightness : lightness;
            for (const BakedColorRules* list : { own, &rules.anyWindow, shared })
            {
                if (!list)
                    continue;
                for (const BakedColorRule& rule : *list)
                {
                    if (rule.output == channel)
                        rule.effect.applyTo(color, rule.levelIn(levels), readIn);
                }
            }
        };

        Hsl stroke = rootSurface();
        applyRules(PaintChannel::Surface, stroke);
        applyRules(PaintChannel::Stroke, stroke);

        Hsl result = { stroke.hue, 0.0f, 0.0f };
        applyRules(PaintChannel::Shadow, result);
        return result;
    }

    BakedColors blend(const BakedColors& from, const BakedColors& to, float factor,
        float lightnessFactor)
    {
        BakedColors result{};
        result.lightness = std::lerp(from.lightness, to.lightness, lightnessFactor);
        result.anchorHue = blendedHue(from.anchorHue, 1.0f - factor, to.anchorHue, factor,
            factor);

        for (std::size_t i = 0; i < result.effects.size(); ++i)
            result.effects[i] = blend(from.effects[i], to.effects[i], factor);

        for (std::size_t i = 0; i < result.pigmentHues.size(); ++i)
            result.pigmentHues[i] = blendedHue(from.pigmentHues[i], 1.0f - factor,
                to.pigmentHues[i], factor, factor);

        result.rules = blend(from.rules, to.rules, factor);

        return result;
    }

}
