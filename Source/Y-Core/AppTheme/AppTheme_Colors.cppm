export module ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // The names a mode is written under. Kept here rather than beside the enum because they are
    // the serializer's vocabulary, the same way ColorHarmonyKind's are.
    export constexpr auto enumNames(ColorMode) {
        return std::array{ L"Dark", L"Light" };
    }

    // The names a setting is written under, in the enum's order - a name's index is its value.
    export constexpr auto enumNames(ColorModeSetting)
    {
        return std::array{ L"Auto", L"Dark", L"Light" };
    }

    // WHERE A THEME IS WORN BETWEEN THE TWO POLES: 0 is Dark, 1 is Light, and anything between
    // is a crossing between the modes part way through. See AppTheme
    export using Lightness = float;

    export constexpr Lightness k_darkLightness{ 0.0f };
    export constexpr Lightness k_lightLightness{ 1.0f };

    // The lightness a mode stands at, and the one place the switch becomes the float.
    export [[nodiscard]] constexpr Lightness lightnessOf(ColorMode mode)
    {
        return ColorMode::Light == mode ? k_lightLightness : k_darkLightness;
    }

    // The mode a lightness reads as: the pole it stands nearer to.
    export [[nodiscard]] constexpr ColorMode colorModeOf(Lightness lightness)
    {
        return lightness < 0.5f ? ColorMode::Dark : ColorMode::Light;
    }

    // What a colour rule does to the value it is given.
    export enum class ColorRuleOp
    {
        NoChange,
        Offset,
        Scale,
        Set,
    };

    export struct ThemeColors;

    export class ColorRuleValue
    {
    public:
        ColorRuleValue() = default;
        constexpr ColorRuleValue(ColorRuleOp, float);
        void clear();
        float value() const { return fromNormalizedValue(m_operation, m_normalizedValue); }
        float value(ColorRuleOp action) const { return fromNormalizedValue(action, m_normalizedValue); }
        float normalizedValue() const { return m_normalizedValue; }
        void setNormalizedValue(float normalizedValue) { m_normalizedValue = normalizedValue; }

        ColorRuleOp operation() const { return m_operation; }
        void setOperation(ColorRuleOp);
        void setOperationAndValue(ColorRuleOp, float value);

        float applyTo(float& field, float factor, ColorMode, float luminosityFloor) const;
        [[nodiscard]] bool operator==(const ColorRuleValue&) const = default;
    private:
        static constexpr float toNormalizedValue(ColorRuleOp, float);
        static constexpr float fromNormalizedValue(ColorRuleOp, float);
    private:
        ColorRuleOp m_operation{ ColorRuleOp::NoChange };
        float m_normalizedValue{ 0.5f };
    };

    // Where a rule takes its hue from: a palette slot, or a value it states.
    export enum class ColorRuleHueOp
    {
        PaletteColor1, // palette Colors first - to be able to address them directly
        PaletteColor2,
        PaletteColor3,
        NoChange,
        EndPalette = NoChange,
        ExactValue,
        Count // required for Config:: serialization
    };

    export class ColorRuleHue
    {
    public:
        constexpr ColorRuleHue();
        constexpr ColorRuleHue(ColorRuleHueOp, float exactValue = 0.0f);
        void clear();
        ColorRuleHueOp operation() const { return m_operation; }
        void setOperation(ColorRuleHueOp);
        float exactValue() const { return m_exactValue; }
        void setExactValue(float value) { m_exactValue = value; }
        float actualHue(const ThemeColors&, float fallbackHue) const;
        [[nodiscard]] bool operator==(const ColorRuleHue&) const = default;
    private:
        ColorRuleHueOp m_operation;
        // exactValue is used only if (action == ExactValue);
        float m_exactValue;
    };

    export struct ColorRule
    {
    public:
        static constexpr ColorRuleHueOp k_defaultHueAction{ ColorRuleHueOp::NoChange };
    public:
        constexpr ColorRule() = default;
        constexpr ColorRule(ColorRuleValue saturation, ColorRuleValue elevation);
        constexpr ColorRule(ColorRuleHue hue, ColorRuleValue saturation, ColorRuleValue luminosity);
        void clear();
        // Elevation moves in the direction the mode states. Every value the rule holds is read in
        // that direction, a Set target included: a rule applied to an element standing on the far
        // side of the theme names its places and its moves from that side.
        // See PaintEvent::contrastSign.
        float applyTo(Hsl&, float factor, const ThemeColors&, ColorMode mode) const;
        // Whether applying this rule to anything can come out as the colour it was handed. A
        // caller that would pay for the application - a per span pass over a text run - asks
        // first, so a rule left at its default costs nothing.
        [[nodiscard]] bool changesNothing() const;
        void setExactHsl(Hsl, ColorMode);
        [[nodiscard]] bool operator==(const ColorRule&) const = default;
    public:
        ColorRuleHue hue{};
        ColorRuleValue saturation{};
        ColorRuleValue elevation{};
    };

    // The colours a control paints itself from, per state.
    export struct ControlColorRules
    {
        // Whether this element stands on the far side of the theme - a dark strip on a light
        // theme, a light card on a dark one. Every rule below is then read from that side, and so
        // is the element's border, every ink resolved on it and every control it contains. The
        // values stay as they are written: what a rule states is a distance or a place measured
        // from the side the element stands on, so the gap between a surface and the ink over it
        // survives the crossing.
        //
        // The ink the element inherits crosses with it, before its own text rule is applied, so
        // an element that states no text rule is legible on the side it has moved to. See
        // PaintEvent::colorMode.
        bool flip{ false };
        // DECLARED IN THE ORDER PAINTEVENT APPLIES THEM: surface, then active, hovered and
        // pressed over it, then text and activeText over the ink - and shadow last, which the
        // form reads instead. Stroke follows surface, being the border drawn around it.
        // Generated C++ names these as designated initializers, which have to appear in
        // declaration order, so the two orders are one - see UiElementState, which every reader
        // of a set walks by.
        ColorRule surface{};
        // The border, applied to the higher of the element's surface and the one it stands on.
        ColorRule stroke{};
        // The element while it is the one in effect: the open tab, the title strip of the form
        // the user is in, a mark that is on, the selection the keys act on. It is applied over
        // surface by whatever factor says the element is in effect, which for a control is its
        // selected visual state - the state names what the user did, this names what is painted.
        ColorRule active{};
        ColorRule hovered{};
        ColorRule pressed{};
        ColorRule text{};
        // What being the element in effect does to the ink text established, applied over it by
        // the same factor active is taken at. Active is the one state that is ambient and lasting
        // rather than pointer-driven - an inactive window stays inactive with the pointer
        // elsewhere and nothing pressed - so its ink has to carry that itself. Hover and press
        // are momentary and the surface says them, which is why the pair stops here and there
        // is no hoveredText or pressedText.
        //
        // A set states text as the muted ink and this as what restores it, not text as the full
        // ink with this muting it. Written the other way, muted would be the default for every
        // element in the framework and each would have to opt back out; this way the member is
        // {} everywhere except where an element has something to say about being in effect.
        ColorRule activeText{};
        // The shadow a window root casts around its window. See AppTheme#windowshadow
        ColorRule shadow{};
    };

    // The harmony a theme's pigments come from, built over the anchor and the harmony kind and
    // kept until either moves. A theme carries one so that a pigment can be resolved while
    // painting, where there is nothing to derive a harmony from.
    //
    // The anchor is this object's own copy and the harmony holds a reference to it, so copying and
    // moving drop the harmony rather than carrying it: one taken from another theme would answer
    // from that theme's anchor, and one taken from a moved-from theme would read storage that has
    // gone. What is dropped is derived, so a theme that starts empty builds its own from its own
    // anchor the first time a pigment is asked for.
    class PigmentPalette
    {
    public:
        PigmentPalette() = default;
        PigmentPalette(const PigmentPalette&) {}
        PigmentPalette(PigmentPalette&&) noexcept {}
        PigmentPalette& operator=(const PigmentPalette&);
        PigmentPalette& operator=(PigmentPalette&&) noexcept;
        ~PigmentPalette() = default;
        [[nodiscard]] const ColorHarmony& harmony(float anchorHue, ColorHarmonyKind) const;
    private:
        void reset() const;
    private:
        // Declared before the harmony, which binds a reference to it.
        mutable float m_anchor{};
        mutable ColorHarmonyKind m_kind{ k_defaultHarmonyKind };
        mutable std::unique_ptr<ColorHarmony> m_harmony{};
    };

    // Every element a theme colours, in ThemeColors declaration order. See Application
    export enum class UiElement : TagValue
    {
        Dialog,
        Page,
        Tab,
        Section,
        SectionHeader,
        Bar,
        DialogTitle,
        Menu,
        Tooltip,
        Divider,
        Grid,
        GridHeader,
        GridRow,
        Button,
        ToolButton,
        SelectedText,
        SelectionIndicator,
        HoverIndicator,
        Accent,
        Spot,
        ScrollButton,
        ScrollThumb,
        Count
    };

    export constexpr std::size_t k_uiElementCount{ static_cast<std::size_t>(UiElement::Count) };
    export using OptionalUiElement = std::optional<UiElement>;

    // What a rule of the new color theme architecture reads.
    export enum class RuleInput
    {
        Hovered,
        Pressed,
        Focused,        // the control's own focus, whether or not its form has it
        Selected,
        Disabled,       // the enabled factor turned over
        TextHovered,
        Current,
        WindowFocused,  // the focus of the form the control stands in
        Count
    };

    // How far each input stands on one control, in RuleInput order.
    export using RuleInputLevels = std::array<float, static_cast<std::size_t>(RuleInput::Count)>;

    // The inputs a rule reads, joined as "or". An empty set stands at 1 and holds nothing back.
    export class RuleInputs
    {
    public:
        constexpr RuleInputs() = default;
        // Not explicit, so a rule names its set as a braced list of enumerators.
        template <std::same_as<RuleInput>... Inputs>
        constexpr RuleInputs(Inputs... inputs)
            :
            m_bits{ (0u | ... | bitOf(inputs)) }
        {
        }
        [[nodiscard]] constexpr bool empty() const { return m_bits == 0u; }
        [[nodiscard]] constexpr bool has(RuleInput input) const
        {
            return (m_bits & bitOf(input)) != 0u;
        }
        void add(RuleInput input) { m_bits |= bitOf(input); }
        void remove(RuleInput input) { m_bits &= ~bitOf(input); }
        // How far the set stands at these levels: its inputs joined as "or", and 1 at rest.
        [[nodiscard]] float levelIn(const RuleInputLevels&) const;
        [[nodiscard]] bool operator==(const RuleInputs&) const = default;
    private:
        [[nodiscard]] static constexpr std::uint32_t bitOf(RuleInput input)
        {
            return 1u << static_cast<std::uint32_t>(input);
        }
    private:
        std::uint32_t m_bits{ 0u };
    };

    // Where a rule writes. A stroke starts at the control's own surface, the rest at the parent's.
    export enum class PaintChannel
    {
        Surface,
        Stroke,
        Text,
        Shadow,
        Count
    };

    // A rule of the new color theme architecture: it reads its inputs and writes to its output.
    export struct ColorRule2
    {
        RuleInputs inputs{};                            // when the rule applies, at rest if empty
        RuleInputs andInputs{};                         // what must hold as well, joined as "and"
        PaintChannel output{ PaintChannel::Surface };   // the channel the rule writes to
        ColorRule effect{};                             // what the rule does to that channel
        [[nodiscard]] bool atRest() const { return inputs.empty() and andInputs.empty(); }
        [[nodiscard]] bool operator==(const ColorRule2&) const = default;
    };

    export using ColorRules2 = std::vector<ColorRule2>;

    // The new rules a theme states: each element's own, then the window's, then the shared ones.
    export struct ThemeRules2
    {
        ColorRules2 shared{};
        ColorRules2 anyWindow{}; // what every form's root control takes, whatever it wears
        std::array<ColorRules2, k_uiElementCount> elements{}; // indexed by UiElement
        [[nodiscard]] ColorRules2& of(UiElement element)
        {
            return elements[indexOf(element)];
        }
        [[nodiscard]] const ColorRules2& of(UiElement element) const
        {
            return elements[indexOf(element)];
        }
        // One element's list as a document field reads and writes it, by its place in the array.
        template<std::size_t I>
        [[nodiscard]] const ColorRules2& element() const { return elements[I]; }
        template<std::size_t I>
        void setElement(const ColorRules2& value) { elements[I] = value; }
        [[nodiscard]] static constexpr std::size_t indexOf(UiElement element)
        {
            return static_cast<std::size_t>(element);
        }
    };

    // The rules the framework's own theme states.
    export [[nodiscard]] ThemeRules2 defaultRules2();

    // A theme's colours. The mode they are worn in is the application's. See AppTheme
    export struct ThemeColors
    {
    public:
        [[nodiscard]] Hsl rootSurface(ColorMode) const;
        [[nodiscard]] Hsl rootText(ColorMode) const;
        [[nodiscard]] const ColorHarmony& harmony() const;
    public:




        // This code is to be used inside the ThemeColors class
        // to initialize a theme compiled into an application

        // The hue a harmony turns around, and the only thing about the anchor anyone chooses.
        // A palette is a set of hues and nothing else, so what a swatch or a slider ramp is
        // drawn in comes from the palette's display pair rather than from here.
        float anchorHue{ 0.666534f };

        ColorHarmonyKind harmonyKind{ ColorHarmonyKind::Complementary };

        // The three hues a rule can name. Only a hue is the palette's own: a control keeps the
        // saturation and luminosity it already carries, and the palette swatch borrows the
        // anchor's.
        std::array<float, 3> paletteHues{
            0.666534f,
            0.666534f,
            0.16653395f
        };

        // The luminosity elevation 0 is lifted to at the dark end. See AppTheme
        float darkModeFloor{ 0.098735f };

        // THE THEME'S OWN EMPHASIS, AND WHAT SAYS A THING IS ON - one rule for both, because the
        // two are one colour. It is the ink anything asking for emphasis is drawn in, over the
        // palette's accent hue - the same one a button under the pointer moves toward, so an icon
        // drawn in it belongs to the family of the controls around it - and it is equally the
        // active state of every mark: a text caret, a hot link, the band a StackView draws behind a
        // selected item, the focus ring while the user is on the control, and the indicator under
        // an open tab. A check and a radio dot state their own on colour - see SelectionIndicator.
        ColorRule accent{
            { ColorRuleHueOp::PaletteColor2 },          // H
            { ColorRuleOp::Set, 1.0f },                 // S
            { ColorRuleOp::Set, 0.544542f }             // E
        };

        // The second accent, over the palette's third hue: what stands apart from the interface
        // rather than answers to it - a brand mark, a run of emphasised text, and the tint a
        // tooltip carries. Stated apart from the accent so that a theme can spend one sparingly
        // while the other runs through every control.
        ColorRule spot{
            { ColorRuleHueOp::PaletteColor3 },          // H
            { ColorRuleOp::Set, 0.926093f },            // S
            { ColorRuleOp::Set, 0.689641f }             // E
        };




        ThemeRules2 rules2{ defaultRules2() }; // the rules of the new color theme architecture

        [[nodiscard]] Hsl pigmentHsl(Pigment, ColorMode) const;
        [[nodiscard]] Hsl pigmentHsl(Pigment, InkTone) const;
        // The half of the resolver that needs the theme: a pigment names a colour, and only the
        // theme knows what the name stands for. A grade needs nothing of the theme, so it does not
        // appear here.
        // Which of this theme's rules an ink names. The ink says which; what it does is the
        // theme's to say, and this is the one place the two are put together.
        [[nodiscard]] const ColorRule& ruleOf(InkColor) const;

    private:
        // Derived from anchorHue and harmonyKind, and stated by them, so it is not a field of
        // the theme and nothing writes it to a file.
        PigmentPalette m_pigmentPalette{};
    };

    // What a channel no floor reaches states, which is every saturation.
    export constexpr float k_noFloor = 0.0f;

    // Where a colour stands on the luminosity axis, oriented so that higher is always further from
    // the page and closer to the eye: 0 is the mode's floor and 1 its ceiling. A coordinate rather
    // than a relation - two elements at 0.6 stand at the same luminosity whatever each is drawn on.
    //
    // The lightness is the element's own rather than the theme's, because an element that has
    // carried the theme across itself rises the other way - see PaintEvent::lightness. A channel
    // with no orientation is read at k_darkLightness.
    //
    // At the dark end a surface's axis starts at its floor rather than at black, so a colour below
    // the floor has no elevation and clamps to the end it lies past. See AppTheme
    export [[nodiscard]] constexpr float elevationOf(float luminosity, Lightness lightness,
        float luminosityFloor)
    {
        const float lift = luminosityFloor * (1.0f - lightness);
        const float unlifted = (luminosity - lift) / (1.0f - lift);
        const float oriented = unlifted + lightness * (1.0f - 2.0f * unlifted);
        return std::clamp(oriented, 0.0f, 1.0f);
    }

    export [[nodiscard]] constexpr float elevationOf(float luminosity, Lightness lightness)
    {
        return elevationOf(luminosity, lightness, k_noFloor);
    }

    export [[nodiscard]] constexpr float elevationOf(float luminosity, ColorMode mode,
        float luminosityFloor)
    {
        return elevationOf(luminosity, lightnessOf(mode), luminosityFloor);
    }

    export [[nodiscard]] constexpr float elevationOf(float luminosity, ColorMode mode)
    {
        return elevationOf(luminosity, lightnessOf(mode));
    }

    // The luminosity an elevation names, and the inverse of elevationOf.
    export [[nodiscard]] constexpr float luminosityOf(float elevation, Lightness lightness,
        float luminosityFloor)
    {
        const float oriented = elevation + lightness * (1.0f - 2.0f * elevation);
        const float lift = luminosityFloor * (1.0f - lightness);
        return lift + oriented * (1.0f - lift);
    }

    export [[nodiscard]] constexpr float luminosityOf(float elevation, Lightness lightness)
    {
        return luminosityOf(elevation, lightness, k_noFloor);
    }

    export [[nodiscard]] constexpr float luminosityOf(float elevation, ColorMode mode,
        float luminosityFloor)
    {
        return luminosityOf(elevation, lightnessOf(mode), luminosityFloor);
    }

    export [[nodiscard]] constexpr float luminosityOf(float elevation, ColorMode mode)
    {
        return luminosityOf(elevation, lightnessOf(mode));
    }


    //-------------------------------------------------------------------------


    // ColorRuleValue

    constexpr ColorRuleValue::ColorRuleValue(ColorRuleOp operation, float value)
        :
        m_operation{ operation },
        m_normalizedValue{ toNormalizedValue(operation, value) }
    {
    }

    void ColorRuleValue::clear()
    {
        m_operation = ColorRuleOp::NoChange;
        m_normalizedValue = 0.0f;
    }

    void ColorRuleValue::setOperation(ColorRuleOp operation)
    {
        m_operation = operation;
    }

    void ColorRuleValue::setOperationAndValue(ColorRuleOp operation, float value)
    {
        m_operation = operation;
        m_normalizedValue = toNormalizedValue(m_operation, value);
    }

    // Every operation states an elevation or a move in elevations, so the field crosses to that
    // axis, the operation happens there, and the result crosses back. The floor is the theme's,
    // and k_noFloor for a saturation.
    float ColorRuleValue::applyTo(float& field, float factor, ColorMode mode,
        float luminosityFloor) const
    {
        const float elevation = elevationOf(field, mode, luminosityFloor);
        float newElevation = elevation;

        switch (m_operation)
        {
        case ColorRuleOp::Offset:
        {
            const float addedValue = value() * factor;
            if (addedValue == 0.0f)
                return 0.0f;

            newElevation = elevation + addedValue;
            break;
        }

        case ColorRuleOp::Set:
        {
            // Set states an elevation outright, so the crossing back runs whatever the
            // field reads as here. An elevation read from a colour below the floor is the
            // end it lies past rather than a place it sits, so taking a clamped 0 that meets
            // the target for a colour already at it would leave that colour at the pole.
            const float targetValue = value();
            newElevation = elevation + (targetValue - elevation) * factor;
            break;
        }

        case ColorRuleOp::Scale:
        {
            const float multiplier = value();

            if (factor <= 0.0f || multiplier == 1.0f)
                return 0.0f;

            // Linear interpolation: elevation * multiplier when factor is 1.0f
            newElevation = elevation * multiplier * factor + elevation * (1.0f - factor);
            break;
        }

        case ColorRuleOp::NoChange:
            return 0.0f;
        }

        const float newValue = luminosityOf(std::clamp(newElevation, 0.0f, 1.0f), mode,
            luminosityFloor);
        if (field == newValue)
            return 0.0f; // No change due to boundary clamping or value matching

        field = newValue;
        return factor;
    }

    constexpr float ColorRuleValue::toNormalizedValue(ColorRuleOp action, float value)
    {
        switch (action)
        {
        case ColorRuleOp::Offset:
            return value + 0.5f;
        case ColorRuleOp::Scale:
            return value / 2.0f;
        case ColorRuleOp::NoChange:
        case ColorRuleOp::Set:
            break;
        }
        return value;
    }

    constexpr float ColorRuleValue::fromNormalizedValue(ColorRuleOp action, float normalizedValue)
    {
        switch (action)
        {
        case ColorRuleOp::Offset:
            return normalizedValue - 0.5f;
        case ColorRuleOp::Scale:
            return normalizedValue * 2.0f;
        case ColorRuleOp::NoChange:
        case ColorRuleOp::Set:
            break;
        }
        return normalizedValue;
    }

    // ColorRuleHue

    constexpr ColorRuleHue::ColorRuleHue()
        :
        ColorRuleHue{ ColorRuleHueOp::NoChange }
    {
    }

    constexpr ColorRuleHue::ColorRuleHue(ColorRuleHueOp operation, float exactValue)
        :
        m_operation{ operation },
        m_exactValue{ exactValue }
    {
    }

    void ColorRuleHue::clear()
    {
        m_operation = ColorRuleHueOp::NoChange;
        m_exactValue = 0.0f;
    }

    void ColorRuleHue::setOperation(ColorRuleHueOp operation)
    {
        m_operation = operation;
    }

    float ColorRuleHue::actualHue(const ThemeColors& themeColors, float fallbackHue) const
    {
        switch (m_operation)
        {
        case ColorRuleHueOp::PaletteColor1:
        case ColorRuleHueOp::PaletteColor2:
        case ColorRuleHueOp::PaletteColor3:
            return themeColors.paletteHues[static_cast<std::size_t>(m_operation)];
        case ColorRuleHueOp::ExactValue:
            return m_exactValue;
        case ColorRuleHueOp::NoChange:
        case ColorRuleHueOp::Count:
            break;
        }
        return fallbackHue;
    }

    // ColorRule

    constexpr ColorRule::ColorRule(ColorRuleValue saturation, ColorRuleValue elevation)
        :
        saturation{ saturation },
        elevation{ elevation }
    {
    }

    constexpr ColorRule::ColorRule(ColorRuleHue hue, ColorRuleValue saturation, ColorRuleValue elevation)
        :
        hue{ hue },
        saturation{ saturation },
        elevation{ elevation }
    {
    }

    void ColorRule::clear()
    {
        hue.clear();
        saturation.clear();
        elevation.clear();
    }

    float ColorRule::applyTo(Hsl& hsl, float factor, const ThemeColors& themeColors,
        ColorMode mode) const
    {
        if (factor <= 0.0f)
        {
            return 0.0f;
        }

        float appliedHueFactor = 0.0f;
        bool doMixHue = false;
        float newHue = hsl.hue;

        const bool exactHue = hue.operation() == ColorRuleHueOp::ExactValue;

        if (const bool paletteHue = hue.operation() < ColorRuleHueOp::EndPalette; paletteHue || exactHue)
        {
            newHue = hue.actualHue(themeColors, hsl.hue);

            if (hsl.hue != newHue)
            {
                appliedHueFactor = factor;
                if (hsl.saturation)
                    doMixHue = true;
                else
                    hsl.hue = newHue;
            }
        }

        float appliedSatFactor = saturation.applyTo(hsl.saturation, factor, ColorMode::Dark,
            k_noFloor);
        float appliedLumFactor = elevation.applyTo(hsl.luminosity, factor, mode,
            themeColors.darkModeFloor);

        if (doMixHue)
        {
            Hsl tmpHsl{
                newHue,
                hsl.saturation,
                hsl.luminosity
            };
            hsl = Hsl{ hsl, tmpHsl, factor };
        }

        return StateFactors::compose({ appliedHueFactor, appliedSatFactor, appliedLumFactor });
    }

    bool ColorRule::changesNothing() const
    {
        return hue.operation() == ColorRuleHueOp::NoChange
            and saturation.operation() == ColorRuleOp::NoChange
            and elevation.operation() == ColorRuleOp::NoChange;
    }

    void ColorRule::setExactHsl(Hsl value, ColorMode mode)
    {
        hue.setOperation(ColorRuleHueOp::ExactValue);
        hue.setExactValue(value.hue);
        saturation.setOperationAndValue(ColorRuleOp::Set, value.saturation);
        // The rule states an elevation, so the colour's luminosity crosses to that axis.
        const float exactElevation = elevationOf(value.luminosity, mode);
        elevation.setOperationAndValue(ColorRuleOp::Set, exactElevation);
    }

    // PigmentPalette

    PigmentPalette& PigmentPalette::operator=(const PigmentPalette&)
    {
        reset();
        return *this;
    }

    PigmentPalette& PigmentPalette::operator=(PigmentPalette&&) noexcept
    {
        reset();
        return *this;
    }

    const ColorHarmony& PigmentPalette::harmony(float anchorHue, ColorHarmonyKind kind) const
    {
        // The kind is a constructor argument, so a harmony of another kind is replaced rather
        // than told. An anchor that has moved under the same kind is told, which is the case a
        // slider drag makes on every frame.
        if (m_harmony && m_kind == kind)
        {
            if (m_anchor != anchorHue)
            {
                m_anchor = anchorHue;
                m_harmony->anchorChanged();
            }
            return *m_harmony;
        }

        m_anchor = anchorHue;
        m_kind = kind;
        m_harmony = std::make_unique<ColorHarmony>(m_anchor, kind);
        return *m_harmony;
    }

    void PigmentPalette::reset() const
    {
        m_harmony.reset();
    }

    // RuleInputs

    float RuleInputs::levelIn(const RuleInputLevels& levels) const
    {
        if (empty())
            return 1.0f;

        float result = 0.0f;
        for (std::size_t i = 0; i != levels.size(); ++i)
        {
            if (has(static_cast<RuleInput>(i)))
                result = StateFactors::compose(result, levels[i]);
        }
        return result;
    }

    // ThemeRules2

    ThemeRules2 defaultRules2()
    {
        ThemeRules2 result{};
        // Dialog - a window root: its surface named outright, then its stroke and window shadow.
        result.of(UiElement::Dialog) = {
            ColorRule2{
                .effect{
                    { ColorRuleHueOp::PaletteColor1, 0.252055f }, // H
                    { ColorRuleOp::Set, 0.0684084f },       // S
                    { ColorRuleOp::Set, 0.0018784736f }     // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.120151f }      // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Shadow,
                .effect{
                    { ColorRuleHueOp::PaletteColor1 },      // H
                    { ColorRuleOp::Set, 0.6926495f },       // S
                    { ColorRuleOp::Set, 0.23802227f }       // E
                }
            }
        };

        // Page - its surface at rest, then its stroke.
        result.of(UiElement::Page) = {
            ColorRule2{
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.0f }              // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.044491f }      // E
                }
            }
        };

        // Tab - its stroke while hovered or open, the open tab's outline and the line it stands on.
        result.of(UiElement::Tab) = {
            ColorRule2{
                .inputs{ RuleInput::Hovered, RuleInput::Selected },
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.928767f }, // H
                    { ColorRuleOp::NoChange, 0.490111f },   // S
                    { ColorRuleOp::Offset, 0.25f }          // E
                }
            }
        };

        // Section - its surface at rest.
        result.of(UiElement::Section) = {
            ColorRule2{
                .effect{
                    { ColorRuleOp::NoChange, 0.195241f },   // S
                    { ColorRuleOp::Offset, 0.044533f }      // E
                }
            }
        };

        // Section header - its surface at rest and its text, then its stroke.
        result.of(UiElement::SectionHeader) = {
            ColorRule2{
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.111111f }         // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Text,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.897275f }         // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.860274f }, // H
                    { ColorRuleOp::NoChange, 0.765086f },   // S
                    { ColorRuleOp::Offset, 0.080088f }      // E
                }
            }
        };

        // Bar - its surface at rest.
        result.of(UiElement::Bar) = {
            ColorRule2{
                .effect{
                    { ColorRuleHueOp::NoChange, 0.512329f }, // H
                    { ColorRuleOp::NoChange, 0.137752f },   // S
                    { ColorRuleOp::Set, 0.102818f }         // E
                }
            }
        };

        // Dialog title - its surface at rest and selected, then its text at rest and selected.
        result.of(UiElement::DialogTitle) = {
            ColorRule2{
                .effect{
                    { ColorRuleOp::NoChange, 0.18f },       // S
                    { ColorRuleOp::Set, 0.13 }              // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleOp::NoChange, 0.501121f },   // S
                    { ColorRuleOp::Set, 0.08f }             // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Text,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.131507f }, // H
                    { ColorRuleOp::NoChange, 1.0f },        // S
                    { ColorRuleOp::Offset, -0.3 }           // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Selected },
                .output = PaintChannel::Text,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.3 }            // E
                }
            }
        };

        // Menu - a window root: its surface named outright, then its stroke and window shadow.
        result.of(UiElement::Menu) = {
            ColorRule2{
                .effect{
                    { ColorRuleHueOp::PaletteColor1 },      // H
                    { ColorRuleOp::Set, 0.072888434f },     // S
                    { ColorRuleOp::Set, 0.0011279281f }     // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.5f }              // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Shadow,
                .effect{
                    { ColorRuleOp::Set, 0.68912506f },      // S
                    { ColorRuleOp::Set, 0.23544617f }       // E
                }
            }
        };

        // Tooltip - a window root: its surface named outright, then its stroke and window shadow.
        result.of(UiElement::Tooltip) = {
            ColorRule2{
                .effect{
                    { ColorRuleHueOp::PaletteColor3 },      // H
                    { ColorRuleOp::Set, 0.12352588f },      // S
                    { ColorRuleOp::Set, 0.053887f }         // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.278328f }         // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Shadow,
                .effect{
                    { ColorRuleHueOp::PaletteColor3 },      // H
                    { ColorRuleOp::Set, 0.69047594f },      // S
                    { ColorRuleOp::Set, 0.23544617f }       // E
                }
            }
        };

        // Divider - its surface at rest, then its stroke, the line a grid's divider row carries.
        result.of(UiElement::Divider) = {
            ColorRule2{
                .effect{
                    { ColorRuleOp::NoChange, 0.191474f },   // S
                    { ColorRuleOp::Offset, 0.05f }          // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.860274f }, // H
                    { ColorRuleOp::NoChange, 0.765086f },   // S
                    { ColorRuleOp::Offset, 0.080088f }      // E
                }
            }
        };

        // Grid - its stroke, the outer border of the lattice.
        result.of(UiElement::Grid) = {
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.120654f }      // E
                }
            }
        };

        // Grid header - its surface at rest and its text, then its stroke between the columns.
        result.of(UiElement::GridHeader) = {
            ColorRule2{
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.111111f }         // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Text,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Set, 0.897275f }         // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.860274f }, // H
                    { ColorRuleOp::NoChange, 0.765086f },   // S
                    { ColorRuleOp::Offset, 0.080088f }      // E
                }
            }
        };

        // Grid row - its surface selected and hovered, then its stroke, the lines between cells.
        result.of(UiElement::GridRow) = {
            ColorRule2{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.190021f },     // S
                    { ColorRuleOp::Offset, 0.08306903f }    // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Hovered },
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.080827f }      // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    { ColorRuleHueOp::NoChange, 0.860274f }, // H
                    { ColorRuleOp::NoChange, 0.765086f },   // S
                    { ColorRuleOp::Offset, 0.080088f }      // E
                }
            }
        };

        // Button - its surface at rest and in each state, then its stroke.
        result.of(UiElement::Button) = {
            ColorRule2{
                .effect{
                    { ColorRuleOp::NoChange, 0.0f },        // S
                    { ColorRuleOp::Offset, 0.054944f }      // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.094499f },     // S
                    { ColorRuleOp::Offset, 0.099311f }      // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleOp::Offset, 0.085609f },     // S
                    { ColorRuleOp::Offset, 0.126926f }      // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            },
            ColorRule2{
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.037839f }      // E
                }
            }
        };

        // Tool button - its surface selected, hovered and pressed, then its stroke as that surface
        // arrives. Nothing at rest.
        result.of(UiElement::ToolButton) = {
            ColorRule2{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.094499f },     // S
                    { ColorRuleOp::Offset, 0.099311f }      // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleOp::Offset, 0.085609f },     // S
                    { ColorRuleOp::Offset, 0.126926f }      // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Hovered, RuleInput::Selected },
                .output = PaintChannel::Stroke,
                .effect{
                    {},                                     // S
                    { ColorRuleOp::Offset, 0.037839f }      // E
                }
            }
        };

        // Selected text - the band at rest, then what the focus adds to it.
        result.of(UiElement::SelectedText) = {
            ColorRule2{
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, -0.042085f },    // S
                    { ColorRuleOp::Offset, 0.092057f }      // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Focused },
                .andInputs{ RuleInput::WindowFocused },
                .effect{
                    { ColorRuleOp::Offset, 0.382042f },     // S
                    { ColorRuleOp::Offset, 0.094934f }      // E
                }
            }
        };

        // Selection indicator - its surface at rest, selected, hovered and pressed, then its text.
        result.of(UiElement::SelectionIndicator) = {
            ColorRule2{
                .effect{
                    { ColorRuleOp::NoChange, 0.0f },        // S
                    { ColorRuleOp::Offset, 0.155899f }      // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Set, 1.0f },             // S
                    { ColorRuleOp::Set, 0.544542f }         // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.125f },        // S
                    { ColorRuleOp::Offset, 0.06f }          // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            },
            // Set 0 lands at the floor in dark mode and at white in light: legible either way.
            ColorRule2{
                .output = PaintChannel::Text,
                .effect{
                    { ColorRuleOp::Set, 0.0f },             // S
                    { ColorRuleOp::Set, 0.0f }              // E
                }
            }
        };

        // Hover indicator - its surface selected, hovered and pressed, then its text.
        result.of(UiElement::HoverIndicator) = {
            ColorRule2{
                .inputs{ RuleInput::Selected },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Set, 1.0f },             // S
                    { ColorRuleOp::Set, 0.544542f }         // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleHueOp::PaletteColor2 },      // H
                    { ColorRuleOp::Offset, 0.125f },        // S
                    { ColorRuleOp::Offset, 0.06f }          // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            },
            // Set 0 lands at the floor in dark mode and at white in light: legible either way.
            ColorRule2{
                .output = PaintChannel::Text,
                .effect{
                    { ColorRuleOp::Set, 0.0f },             // S
                    { ColorRuleOp::Set, 0.0f }              // E
                }
            }
        };

        // Scroll button - its surface at rest, hovered and pressed.
        result.of(UiElement::ScrollButton) = {
            ColorRule2{
                .effect{
                    { ColorRuleOp::NoChange, 0.0f },        // S
                    { ColorRuleOp::Offset, 0.03f }          // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleOp::Offset, 0.0f },          // S
                    { ColorRuleOp::Offset, 0.3f }           // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            }
        };

        // Scroll thumb - its surface at rest, hovered and pressed.
        result.of(UiElement::ScrollThumb) = {
            ColorRule2{
                .effect{
                    { ColorRuleOp::NoChange, 0.0f },        // S
                    { ColorRuleOp::Offset, 0.4f }           // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Hovered },
                .effect{
                    { ColorRuleOp::Offset, 0.0f },          // S
                    { ColorRuleOp::Offset, 0.06f }          // E
                }
            },
            ColorRule2{
                .inputs{ RuleInput::Pressed },
                .effect{
                    { ColorRuleOp::Scale, 0.8f },           // S
                    { ColorRuleOp::Scale, 0.9f }            // E
                }
            }
        };
        return result;
    }

    // ThemeColors

    Hsl ThemeColors::rootSurface(ColorMode mode) const
    {
        return mode == ColorMode::Dark
            ? Hsl{ anchorHue, 0.0f, darkModeFloor }
            : Hsl{ anchorHue, 0.0f, 1.0f };
    }

    // The bare ink of the colour mode.
    Hsl ThemeColors::rootText(ColorMode mode) const
    {
        return mode == ColorMode::Dark
            ? Hsl{ anchorHue, 0.0f, 1.0f }
            : Hsl{ anchorHue, 0.0f, 0.0f };
    }

    const ColorHarmony& ThemeColors::harmony() const
    {
        return m_pigmentPalette.harmony(anchorHue, harmonyKind);
    }

    Hsl ThemeColors::pigmentHsl(Pigment pigment, ColorMode controlColorMode) const
    {
        const PigmentTones tones = InkWell::pigmentTones(pigment);
        const InkTone& tone = controlColorMode == ColorMode::Light ? tones.light : tones.dark;
        return pigmentHsl(pigment, tone);
    }

    Hsl ThemeColors::pigmentHsl(Pigment pigment, InkTone tone) const
    {
        return {
            harmony().pigmentColor(pigment).hsl().hue,
            std::clamp(tone.saturation, 0.0f, 1.0f),
            std::clamp(tone.luminosity, 0.0f, 1.0f)
        };
    }

    const ColorRule& ThemeColors::ruleOf(InkColor color) const
    {
        switch (color)
        {
            case InkColor::Accent:
                return accent;
            case InkColor::Spot:
                return spot;
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

}
