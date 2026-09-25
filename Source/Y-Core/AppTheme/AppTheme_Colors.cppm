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

    // The harmony a theme's icon hues come from, built over the anchor and the harmony kind and
    // kept until either moves. A theme carries one so that a slot can be resolved while painting,
    // where there is nothing to derive a harmony from.
    //
    // The anchor is this object's own copy and the harmony holds a reference to it, so copying and
    // moving drop the harmony rather than carrying it: one taken from another theme would answer
    // from that theme's anchor, and one taken from a moved-from theme would read storage that has
    // gone. What is dropped is derived, so a theme that starts empty builds its own from its own
    // anchor the first time a slot is asked for.
    class IconPalette
    {
    public:
        IconPalette() = default;
        IconPalette(const IconPalette&) {}
        IconPalette(IconPalette&&) noexcept {}
        IconPalette& operator=(const IconPalette&);
        IconPalette& operator=(IconPalette&&) noexcept;
        ~IconPalette() = default;
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
        Form,
        Page,
        TabLine,
        Section,
        Header,
        Bar,
        FormTitle,
        Menu,
        Tooltip,
        Divider,
        Grid,
        GridRow,
        GridLine,
        Button,
        SelectedText,
        InactiveIndicator,
        Accent,
        Spot,
        ScrollButton,
        ScrollThumb,
        Count
    };

    export constexpr std::size_t k_uiElementCount{ static_cast<std::size_t>(UiElement::Count) };
    export using OptionalUiElement = std::optional<UiElement>;

    // A theme's colours. The mode they are worn in is the application's. See AppTheme
    export struct ThemeColors
    {
    public:
        [[nodiscard]] Hsl formSurface(ColorMode) const;
        [[nodiscard]] Hsl formText(ColorMode) const;
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

        // THE WINDOW EVERYTHING ELSE IS PAINTED ON. A form is the root a control tree stands in,
        // and the pair this set states is what that root establishes: surface is applied to the
        // bare colour of the mode - black at the dark end, white at the light one - and text is
        // applied to the bare ink, white at the dark end and black at the light one, carrying the
        // surface's hue so that a rule raising saturation alone tints toward the family the theme
        // is already in. Every control carries the ink it inherits and applies its own text rule
        // to that, so a rule stated here reaches everything in the form.
        //
        // Menu and tooltip are the other two window roots, each stating the same pair for the
        // window it opens.
        ControlColorRules form{
            .surface{
                { ColorRuleHueOp::PaletteColor1, 0.252055f }, // H
                { ColorRuleOp::Set, 0.0684084f },       // S
                { ColorRuleOp::Set, 0.0018784736f }     // E
            },
            .stroke{
                {},                                     // S
                { ColorRuleOp::Offset, 0.120151f }      // E
            },
            .shadow{
                { ColorRuleHueOp::PaletteColor1 },      // H
                { ColorRuleOp::Set, 0.6926495f },       // S
                { ColorRuleOp::Set, 0.23802227f }       // E
            }
        };

        ControlColorRules page{
            .surface{
                {},                                     // S
                { ColorRuleOp::Set, 0.0f }              // E
            },
            .stroke{
                {},                                     // S
                { ColorRuleOp::Offset, 0.044491f }      // E
            }
        };

        // Every tab's outline and the line it stands on, applied to the tab's own surface.
        ColorRule tabLine{
            { ColorRuleHueOp::NoChange, 0.928767f },    // H
            { ColorRuleOp::NoChange, 0.490111f },       // S
            { ColorRuleOp::Offset, 0.25f }              // E
        };

        ControlColorRules section{
            .surface{
                { ColorRuleOp::NoChange, 0.195241f },   // S
                { ColorRuleOp::Offset, 0.044533f }      // E
            },
            .stroke{
                {},                                     // S
                { ColorRuleOp::NoChange, 0.541019f }    // E
            }
        };

        // The strip an expander shows its title on, and the row of column names across the top
        // of a grid. An expander takes the whole set for its header; a grid header takes surface
        // and text and leaves its other states as they stand, so those two are the whole of what
        // a column name is drawn in.
        ControlColorRules header{
            .surface{
                {},                                     // S
                { ColorRuleOp::Set, 0.111111f }         // E
            },
            .text{
                {},                                     // S
                { ColorRuleOp::Set, 0.897275f }         // E
            }
        };

        // A STRIP OF COMMANDS ACROSS AN EDGE OF A WINDOW: a toolbar along the top, the row of
        // answers along the bottom of a message box, and anything else that is a band of the
        // window rather than a card standing in it. A bar is what the controls on it are painted
        // over, so what it states is a place for them to stand and never an emphasis of its own -
        // it names no state, because nothing makes a bar hovered, pressed or the one in effect.
        ControlColorRules bar{
            .surface{
                { ColorRuleHueOp::NoChange, 0.512329f }, // H
                { ColorRuleOp::NoChange, 0.137752f },   // S
                { ColorRuleOp::Set, 0.102818f }         // E
            },
            .stroke{
                {},                                     // S
                { ColorRuleOp::NoChange, 0.271235f }    // E
            }
        };

        ControlColorRules formTitle{
            .surface{
                { ColorRuleOp::NoChange, 0.18f },       // S
                { ColorRuleOp::Set, 0.13 }         // E
            },
            .active{
                { ColorRuleOp::NoChange, 0.501121f },   // S
                { ColorRuleOp::Set, 0.08f }              // E
            },
            .text{
                { ColorRuleHueOp::NoChange, 0.131507f }, // H
                { ColorRuleOp::NoChange, 1.0f },        // S
                { ColorRuleOp::Offset, -0.3 }     // E
            },
            .activeText{
                {},                                     // S
                { ColorRuleOp::Offset, 0.3 }      // E
            }
        };

        ControlColorRules menu{
            .surface{
                { ColorRuleHueOp::PaletteColor1 },      // H
                { ColorRuleOp::Set, 0.072888434f },     // S
                { ColorRuleOp::Set, 0.0011279281f }     // E
            },
            .stroke{
                {},                                     // S
                { ColorRuleOp::Set, 0.5f }              // E
            },
            .shadow{
                { ColorRuleOp::Set, 0.68912506f },      // S
                { ColorRuleOp::Set, 0.23544617f }       // E
            }
        };

        ControlColorRules tooltip{
            //.flip = true,
            .surface{
                { ColorRuleHueOp::PaletteColor3 },      // H
                { ColorRuleOp::Set, 0.12352588f },      // S
                { ColorRuleOp::Set, 0.053887f }         // E
            },
            .stroke{
                {},                                     // S
                { ColorRuleOp::Set, 0.278328f }         // E
            },
            .text{
                { ColorRuleOp::NoChange, 0.68665785f }, // S
                { ColorRuleOp::NoChange, 0.236442f }    // E
            },
            .shadow{
                { ColorRuleHueOp::PaletteColor3 },      // H
                { ColorRuleOp::Set, 0.69047594f },      // S
                { ColorRuleOp::Set, 0.23544617f }       // E
            }
        };

        ControlColorRules divider{
            .surface{
                { ColorRuleOp::NoChange, 0.191474f },   // S
                { ColorRuleOp::Offset, 0.05f }      // E
            }
        };

        // THE GRID AS A WHOLE, and its stroke is the grid's OUTER border - the line around the
        // lattice and not one of the lines in it, which is gridLine's. GridBase wears this set,
        // so the surface is what every row and cell of the grid stands on.
        ControlColorRules grid{
            .stroke{
                {},                                     // S
                { ColorRuleOp::Offset, 0.120654f }      // E
            }
        };

        // Every row of a grid, groups and sections included. See Grids
        ControlColorRules gridRow{
            // A SELECTED ROW. A row wears this set and paints no surface of its own, so the rule
            // reaches the screen through the cells the row fills. It lands where a focused text
            // selection lands - selectedText's surface and active come to the same place - so the
            // two selections an interface can show read as one colour.
            .active{
                { ColorRuleHueOp::PaletteColor2 },      // H
                { ColorRuleOp::Offset, 0.190021f },     // S
                { ColorRuleOp::Offset, 0.08306903f }    // E
            },
            .hovered{
                {},                                     // S
                { ColorRuleOp::Offset, 0.080827f }      // E
            }
        };

        // Every line of a grid's lattice. One rule for the whole lattice, so a cell's own
        // surface cannot move the line beside it.
        ColorRule gridLine{
            { ColorRuleHueOp::NoChange, 0.860274f },    // H
            { ColorRuleOp::NoChange, 0.765086f },       // S
            { ColorRuleOp::Offset, 0.080088f }          // E
        };

        ControlColorRules button{
            .surface{
                { ColorRuleOp::NoChange, 0.0f },        // S
                { ColorRuleOp::Offset, 0.054944f }      // E
            },
            .stroke{
                {},                                     // S
                { ColorRuleOp::Offset, 0.037839f }      // E
            },
            .active{
                { ColorRuleHueOp::PaletteColor2 },      // H
                { ColorRuleOp::Offset, 0.094499f },     // S
                { ColorRuleOp::Offset, 0.099311f }      // E
            },
            .hovered{
                { ColorRuleOp::Offset, 0.085609f },     // S
                { ColorRuleOp::Offset, 0.126926f }      // E
            },
            .pressed{
                { ColorRuleOp::Scale, 0.8f },           // S
                { ColorRuleOp::Scale, 0.9f }            // E
            }
        };

        // The band behind selected text, and the ink drawn on it. The two are resolved in
        // different places: the band once, against the surface the text sits on, and the ink
        // where each run is drawn, against whatever colour that run already carries - so a grey
        // run stays grey under the band and an accent run stays accent.
        //
        // surface is the band while the focus is elsewhere, and active is what the focus adds to
        // it, applied by the owning control's focused factor so the two crossfade as the focus
        // moves. Both are on screen together whenever a second box still holds a selection,
        // which is what they exist to tell apart.
        ControlColorRules selectedText{
            .surface{
                { ColorRuleHueOp::PaletteColor2 },      // H
                { ColorRuleOp::Offset, -0.042085f },    // S
                { ColorRuleOp::Offset, 0.092057f }      // E
            },
            .active{
                { ColorRuleOp::Offset, 0.382042f },     // S
                { ColorRuleOp::Offset, 0.094934f }      // E
            },
            .text{
                {},                                     // S
                { ColorRuleOp::NoChange, 0.071926f }    // E
            },
            .activeText{
                {},                                     // S
                { ColorRuleOp::NoChange, 0.706543f }    // E
            }
        };

        // THE MARK WHILE IT IS OFF: the empty box of a check, the ring of a radio button, the
        // well a caret or a selection band is raised out of. What the mark becomes when it comes
        // on is the accent, stated once there, so this set names no active state at all - see
        // accent. The states it does name answer the pointer, so a mark inside a button can move
        // with the button around it, and text is the ink over the mark once something is drawn
        // on it.
        ControlColorRules inactiveIndicator{
            .surface{
                { ColorRuleOp::NoChange, 0.0f },        // S
                { ColorRuleOp::Offset, 0.155899f }      // E
            },
            .stroke{
                {},                                     // S
                { ColorRuleOp::NoChange, 0.646367f }    // E
            },
            .hovered{
                { ColorRuleHueOp::PaletteColor2 },      // H
                { ColorRuleOp::Offset, 0.125f },        // S
                { ColorRuleOp::Offset, 0.06f }          // E
            },
            .pressed{
                { ColorRuleOp::Scale, 0.8f },           // S
                { ColorRuleOp::Scale, 0.9f }            // E
            },
            // Set states an absolute target and reads it against the colour mode, so 0 comes out
            // at the floor in dark mode and white in light mode. That is how a mark gets ink that
            // contrasts with it under either mode, without the element having to state a flip.
            .text{
                { ColorRuleOp::Set, 0.0f },             // S
                { ColorRuleOp::Set, 0.0f }              // E
            }
        };

        // THE THEME'S OWN EMPHASIS, AND WHAT SAYS A THING IS ON - one rule for both, because the
        // two are one colour. It is the ink anything asking for emphasis is drawn in, over the
        // palette's accent hue - the same one a button under the pointer moves toward, so an icon
        // drawn in it belongs to the family of the controls around it - and it is equally the
        // active state of every mark: a check, a radio dot, a text caret, a hot link, the band a
        // StackView draws behind a selected item, the focus ring while the user is on the control,
        // and the indicator under an open tab. That is why inactiveIndicator states no active rule
        // of its own: the on state is here, once, and the whole interface says it in one colour.
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

        // The button at each end of a scroll bar. ScrollBar hands this set to those two and
        // scrollThumb to the thumb, so the halves of a bar are themed apart: a thumb has to
        // read against the trough it runs in, an end button against the bar.
        ControlColorRules scrollButton{
            .surface{
                { ColorRuleOp::NoChange, 0.0f },        // S
                { ColorRuleOp::Offset, 0.03f }          // E
            },
            .hovered{
                { ColorRuleOp::Offset, 0.0f },          // S
                { ColorRuleOp::Offset, 0.3f }           // E
            },
            .pressed{
                { ColorRuleOp::Scale, 0.8f },           // S
                { ColorRuleOp::Scale, 0.9f }            // E
            }
        };

        ControlColorRules scrollThumb{
            .surface{
                { ColorRuleOp::NoChange, 0.0f },        // S
                { ColorRuleOp::Offset, 0.4f }           // E
            },
            .hovered{
                { ColorRuleOp::Offset, 0.0f },          // S
                { ColorRuleOp::Offset, 0.06f }          // E
            },
            .pressed{
                { ColorRuleOp::Scale, 0.8f },           // S
                { ColorRuleOp::Scale, 0.9f }            // E
            }
        };




        [[nodiscard]] Hsl slotHsl(ColorSlot, ColorMode) const;
        [[nodiscard]] Hsl slotHsl(ColorSlot, InkTone) const;
        // The half of the resolver that needs the theme: a slot names a colour, and only the theme
        // knows what the name stands for. A grade needs nothing of the theme, so it does not
        // appear here.
        // Which of this theme's rules an ink names. The ink says which; what it does is the
        // theme's to say, and this is the one place the two are put together.
        [[nodiscard]] const ColorRule& ruleOf(InkColor) const;

    private:
        // Derived from anchorHue and harmonyKind, and stated by them, so it is not a field of
        // the theme and nothing writes it to a file.
        IconPalette m_iconPalette{};
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

    // IconPalette

    IconPalette& IconPalette::operator=(const IconPalette&)
    {
        reset();
        return *this;
    }

    IconPalette& IconPalette::operator=(IconPalette&&) noexcept
    {
        reset();
        return *this;
    }

    const ColorHarmony& IconPalette::harmony(float anchorHue, ColorHarmonyKind kind) const
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

    void IconPalette::reset() const
    {
        m_harmony.reset();
    }

    // ThemeColors

    Hsl ThemeColors::formSurface(ColorMode mode) const
    {
        Hsl result = mode == ColorMode::Dark
            ? Hsl{ 0.0f, 0.0f, 0.0f }
            : Hsl{ 0.0f, 0.0f, 1.0f };
        form.surface.applyTo(result, 1.0f, *this, mode);
        return result;
    }

    // The bare ink of the colour mode, carrying the form surface's hue so that a text rule
    // raising saturation alone tints toward the theme's own family rather than toward red.
    Hsl ThemeColors::formText(ColorMode mode) const
    {
        Hsl result = mode == ColorMode::Dark
            ? Hsl{ formSurface(mode).hue, 0.0f, 1.0f }
            : Hsl{ formSurface(mode).hue, 0.0f, 0.0f };
        form.text.applyTo(result, 1.0f, *this, mode);
        return result;
    }

    const ColorHarmony& ThemeColors::harmony() const
    {
        return m_iconPalette.harmony(anchorHue, harmonyKind);
    }

    Hsl ThemeColors::slotHsl(ColorSlot slot, ColorMode controlColorMode) const
    {
        const SlotTones tones = InkWell::slotTones(slot);
        const InkTone& tone = controlColorMode == ColorMode::Light ? tones.light : tones.dark;
        return slotHsl(slot, tone);
    }

    Hsl ThemeColors::slotHsl(ColorSlot slot, InkTone tone) const
    {
        return {
            harmony().iconColor(slot).hsl().hue,
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
