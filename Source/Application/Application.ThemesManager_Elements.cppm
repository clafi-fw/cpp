// =========================================================================
// ClaFi.Application.ThemesManager_Elements
//
// WHAT A THEME IS MADE OF, STATED ONCE. ThemeColors declares a colour rule
// per element and a rule per state within an element; this describes those
// declarations - the member each one names, what it is painted on, the three
// spellings it goes by, and which of its rules actually reach the screen.
//
// Four readers share it and none keeps a list of its own: the serializers
// build their field tuples from these tables, the Themes app builds its grid
// from them, the C++ code generator emits designators from them, and bake
// below turns a theme into the set the paint path reads. An element or a
// state added here reaches all four; one added anywhere else reaches none.
// =========================================================================
export module ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // The states an element paints with, in the order they are applied. See Application
    export enum class UiElementState : TagValue
    {
        Surface,
        Stroke,
        Active,
        Hovered,
        Pressed,
        Text,
        ActiveText,
        Shadow,
        Count
    };

    // THE THREE SPELLINGS A RULE GOES BY, and they are not the same word. See Application
    export struct UiElementStateDescriptor
    {
        std::wstring_view name{};
        std::wstring_view codeName{};
        std::wstring_view token{};
        ColorRule ControlColorRules::* rule{ nullptr };
    };

    // Indexed by UiElementState, so it and this table must stay in step.
    export constexpr std::array<UiElementStateDescriptor, static_cast<std::size_t>(UiElementState::Count)> k_uiElementStates{
        UiElementStateDescriptor{
            .name = L"Surface",
            .codeName = L"surface",
            .token = L"Surface",
            .rule = &ControlColorRules::surface
        },
        UiElementStateDescriptor{
            .name = L"Stroke",
            .codeName = L"stroke",
            .token = L"Stroke",
            .rule = &ControlColorRules::stroke
        },
        UiElementStateDescriptor{
            .name = L"Active",
            .codeName = L"active",
            .token = L"Active",
            .rule = &ControlColorRules::active
        },
        UiElementStateDescriptor{
            .name = L"Hovered",
            .codeName = L"hovered",
            .token = L"Hovered",
            .rule = &ControlColorRules::hovered
        },
        UiElementStateDescriptor{
            .name = L"Pressed",
            .codeName = L"pressed",
            .token = L"Pressed",
            .rule = &ControlColorRules::pressed
        },
        UiElementStateDescriptor{
            .name = L"Text",
            .codeName = L"text",
            .token = L"Text",
            .rule = &ControlColorRules::text
        },
        UiElementStateDescriptor{
            .name = L"Active text",
            .codeName = L"activeText",
            .token = L"ActiveText",
            .rule = &ControlColorRules::activeText
        },
        UiElementStateDescriptor{
            .name = L"Shadow",
            .codeName = L"shadow",
            .token = L"Shadow",
            .rule = &ControlColorRules::shadow
        }
    };

    export constexpr const UiElementStateDescriptor& uiElementStateOf(UiElementState state)
    {
        return k_uiElementStates[static_cast<std::size_t>(state)];
    }

    // The states one element paints with, as a set. See Application
    export class UiElementStates
    {
    public:
        constexpr UiElementStates() = default;
        // Not explicit: a descriptor names its set as a braced list of enumerators, and an
        // aggregate initializes its members by copy-initialization, which an explicit
        // constructor would refuse.
        template <std::same_as<UiElementState>... States>
        constexpr UiElementStates(States... states)
        {
            m_bits = (0u | ... | bitOf(states));
        }
        [[nodiscard]] constexpr bool has(UiElementState state) const { return (m_bits & bitOf(state)) != 0u; }
        [[nodiscard]] constexpr std::size_t count() const { return static_cast<std::size_t>(std::popcount(m_bits)); }
    private:
        [[nodiscard]] static constexpr std::uint32_t bitOf(UiElementState state)
        {
            return 1u << static_cast<std::uint32_t>(state);
        }
    private:
        std::uint32_t m_bits{ 0u };
    };

    // What one element is: its spellings, the ThemeColors member, and its rules. See Application
    export struct UiElementDescriptor
    {
        std::wstring_view name{};
        std::wstring_view codeName{};
        std::wstring_view token{};
        // Exactly one of these two. An element is either one bare rule or a set of state rules,
        // never both, and null is what says which.
        ColorRule ThemeColors::* rule{ nullptr };
        ControlColorRules ThemeColors::* rules{ nullptr };
        // What the element is painted on. The framework nests these controls; this states that
        // nesting once, so a rule can be shown against the colour it will actually change.
        // An empty base is the bare colour of the mode, before any rule.
        OptionalUiElement base{};
        // WHETHER THE ELEMENT OPENS A WINDOW OF ITS OWN, and with it whether its Surface rule
        // states an absolute colour. What stands behind a window is not the theme's to know - the
        // desktop, another application, a form of its own - so a root's surface names its hue
        // outright and sets its saturation and elevation rather than moving them. Its text and
        // stroke rules are ordinary: they are applied to an ink and a surface the root itself has
        // just established, and both may derive. Its shadow is read on an axis of its own - see
        // AppTheme's WindowShadow.
        //
        // A root still carries a base, because the ink it inherits is real: PaintEvent seeds a
        // root's surface and ink from formSurface() and formText(), so a menu and a tooltip stand
        // on the form the way every other element does. Only the form itself has nothing under it.
        bool isWindowRoot{ false };
        // Empty for a bare rule. One state is one row in an editor; two or more is a group.
        UiElementStates states{};
    };

    // Indexed by UiElement, so it and this table must stay in step.
    //
    // A STATE LISTED HERE IS ONE THE ELEMENT PAINTS WITH. An omission is a statement, and every
    // kind of omission is checkable. No Active where nothing can make the element the one in
    // effect. No Stroke where the element's ControlMetrics::border is None: the stroke reaches
    // the screen only through scaler().scaledStrokeWidth(metrics.border), which is 0 for None, so
    // the rule would reach nothing. Check the metrics the element is constructed with, not its
    // colour rules. No Shadow on anything but a window root: FormBase::shadowDesign reads the
    // root's, and nothing reads another element's.
    export constexpr std::array<UiElementDescriptor, static_cast<std::size_t>(UiElement::Count)> k_uiElements{
        // A WINDOW ROOT. Menu and Tooltip are the other two, and the three carry the same pair -
        // the surface the window is filled with and the ink everything in it starts from - and
        // the window's frame: the stroke around it and the shadow it casts. Nothing is under a
        // root, so this one has no base - it stands on the bare colour of the mode.
        UiElementDescriptor{
            .name = L"Form",
            .codeName = L"form",
            .token = L"Form",
            .rules = &ThemeColors::form,
            .isWindowRoot = true,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Text,
                UiElementState::Shadow
            }
        },
        UiElementDescriptor{
            .name = L"Page",
            .codeName = L"page",
            .token = L"Page",
            .rules = &ThemeColors::page,
            .base = UiElement::Form,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Text
            }
        },
        // The open tab wears its page's surface, and its line runs the length of the page.
        UiElementDescriptor{
            .name = L"Tab Line",
            .codeName = L"tabLine",
            .token = L"TabLine",
            .rule = &ThemeColors::tabLine,
            .base = UiElement::Page
        },
        UiElementDescriptor{
            .name = L"Section",
            .codeName = L"section",
            .token = L"Section",
            .rules = &ThemeColors::section,
            .base = UiElement::Page,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Text
            }
        },
        // No Stroke: ThemeMetrics gives an expander header and a grid header no border, and
        // GridHeader copies only surface and text out of this set in any case.
        UiElementDescriptor{
            .name = L"Header",
            .codeName = L"header",
            .token = L"Header",
            .rules = &ThemeColors::header,
            .base = UiElement::Section,
            .states{
                UiElementState::Surface,
                UiElementState::Text
            }
        },
        // What a bar is painted on is the window's content, the same thing a section stands on,
        // so it previews against the page. No Active, Hovered or Pressed: nothing makes a bar the
        // one in effect and nothing points at it - the controls standing on it answer all three.
        UiElementDescriptor{
            .name = L"Bar",
            .codeName = L"bar",
            .token = L"Bar",
            .rules = &ThemeColors::bar,
            .base = UiElement::Page,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Text
            }
        },
        UiElementDescriptor{
            .name = L"Form Title",
            .codeName = L"formTitle",
            .token = L"FormTitle",
            .rules = &ThemeColors::formTitle,
            .base = UiElement::Form,
            .states{
                UiElementState::Surface,
                UiElementState::Active,
                UiElementState::Text,
                UiElementState::ActiveText
            }
        },
        UiElementDescriptor{
            .name = L"Menu",
            .codeName = L"menu",
            .token = L"Menu",
            .rules = &ThemeColors::menu,
            .base = UiElement::Form,
            .isWindowRoot = true,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Text,
                UiElementState::Shadow
            }
        },
        UiElementDescriptor{
            .name = L"Tooltip",
            .codeName = L"tooltip",
            .token = L"Tooltip",
            .rules = &ThemeColors::tooltip,
            .base = UiElement::Form,
            .isWindowRoot = true,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Text,
                UiElementState::Shadow
            }
        },
        UiElementDescriptor{
            .name = L"Divider",
            .codeName = L"divider",
            .token = L"Divider",
            .rules = &ThemeColors::divider,
            .base = UiElement::Section,
            .states{
                UiElementState::Surface
            }
        },
        // STROKE HERE IS THE GRID'S OUTER BORDER - the line drawn around the lattice, and not one
        // of the lines in it. Those are GridLine, one rule for the whole lattice, and they stand
        // on the rows. No Active: what the selection lights is a row, which wears GridRow.
        UiElementDescriptor{
            .name = L"Grid",
            .codeName = L"grid",
            .token = L"Grid",
            .rules = &ThemeColors::grid,
            .base = UiElement::Section,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Text
            }
        },
        // No Stroke: a row draws no border of its own - the lines between its cells are GridLine.
        UiElementDescriptor{
            .name = L"Grid Row",
            .codeName = L"gridRow",
            .token = L"GridRow",
            .rules = &ThemeColors::gridRow,
            .base = UiElement::Grid,
            .states{
                UiElementState::Surface,
                UiElementState::Active,
                UiElementState::Hovered,
                UiElementState::Pressed,
                UiElementState::Text,
                UiElementState::ActiveText
            }
        },
        UiElementDescriptor{
            .name = L"Grid Line",
            .codeName = L"gridLine",
            .token = L"GridLine",
            .rule = &ThemeColors::gridLine,
            .base = UiElement::GridRow
        },
        UiElementDescriptor{
            .name = L"Button",
            .codeName = L"button",
            .token = L"Button",
            .rules = &ThemeColors::button,
            .base = UiElement::Section,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Active,
                UiElementState::Hovered,
                UiElementState::Pressed,
                UiElementState::Text,
                UiElementState::ActiveText
            }
        },
        // Surface is the band while the focus is elsewhere and Active is what the focus adds, so
        // the Surface row is what is seen on a box the user is not typing in.
        UiElementDescriptor{
            .name = L"Selected Text",
            .codeName = L"selectedText",
            .token = L"SelectedText",
            .rules = &ThemeColors::selectedText,
            .base = UiElement::Section,
            .states{
                UiElementState::Surface,
                UiElementState::Active,
                UiElementState::Text,
                UiElementState::ActiveText
            }
        },
        // NO ACTIVE AND NO ACTIVE TEXT, and the omission is the whole point of the name: what a
        // mark becomes when it comes on is Accent, stated once there for the mark, the focus ring
        // and the tab indicator alike. ButtonBase::adjustChildPaint puts that rule into the set it
        // hands its indicator, so the on state still arrives through UiElementState::Active - it
        // is simply not this element's to state.
        UiElementDescriptor{
            .name = L"Inactive Indicator",
            .codeName = L"inactiveIndicator",
            .token = L"InactiveIndicator",
            .rules = &ThemeColors::inactiveIndicator,
            .base = UiElement::Section,
            .states{
                UiElementState::Surface,
                UiElementState::Stroke,
                UiElementState::Hovered,
                UiElementState::Pressed,
                UiElementState::Text
            }
        },
        // THREE THINGS IN ONE RULE, which is what the name says: the theme's own emphasis, the
        // ring on the control the user is on, and the on state of every indicator. They are one
        // colour on purpose - the interface says "this one" in a single ink - so they are edited
        // as one row.
        UiElementDescriptor{
            .name = L"Accent, Focus, Indicator",
            .codeName = L"accent",
            .token = L"Accent",
            .rule = &ThemeColors::accent,
            .base = UiElement::Section
        },
        UiElementDescriptor{
            .name = L"Spot",
            .codeName = L"spot",
            .token = L"Spot",
            .rule = &ThemeColors::spot,
            .base = UiElement::Section
        },
        // No Active on either half of a scroll bar: both are copied from button in the
        // ThemeColors constructor and inherit its active rule, but nothing ever makes a scroll
        // button or a thumb the one in effect. Neither is given a border either.
        UiElementDescriptor{
            .name = L"Scroll Button",
            .codeName = L"scrollButton",
            .token = L"ScrollButton",
            .rules = &ThemeColors::scrollButton,
            .base = UiElement::Section,
            .states{
                UiElementState::Surface,
                UiElementState::Hovered,
                UiElementState::Pressed,
                UiElementState::Text
            }
        },
        UiElementDescriptor{
            .name = L"Scroll Thumb",
            .codeName = L"scrollThumb",
            .token = L"ScrollThumb",
            .rules = &ThemeColors::scrollThumb,
            .base = UiElement::Section,
            .states{
                UiElementState::Surface,
                UiElementState::Hovered,
                UiElementState::Pressed,
                UiElementState::Text
            }
        }
    };

    export constexpr const UiElementDescriptor& uiElementOf(UiElement element)
    {
        return k_uiElements[static_cast<std::size_t>(element)];
    }

    // A theme as the paint path reads it, in the mode it is worn in. The walk is this table, so
    // an element added above is baked without anything here being touched.
    export [[nodiscard]] BakedColors bake(const ThemeColors&, ColorMode);


    //-------------------------------------------------------------------------


    BakedColors bake(const ThemeColors& themeColors, ColorMode mode)
    {
        BakedColors result{};
        result.lightness = lightnessOf(mode);
        result.anchorHue = themeColors.anchorHue;

        for (std::size_t i = 0; i < k_uiElements.size(); ++i)
        {
            const UiElementDescriptor& descriptor = k_uiElements[i];
            if (descriptor.rule != nullptr)
            {
                result.rules[i] = bake(themeColors.*descriptor.rule, themeColors);
            }
            else
            {
                const ControlColorRules& rules = themeColors.*descriptor.rules;
                result.elements[i] = bake(rules, themeColors);
                if (descriptor.states.has(UiElementState::Shadow))
                    result.shadows[i] = bakeShadow(rules.shadow, themeColors);
            }
        }

        // The harmony answers here and nowhere after: what a pigment stands for is the theme's
        // to say, and a hue is a value two themes have a half way between.
        for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
        {
            result.pigmentHues[pigment] = themeColors.harmony()
                .pigmentColor(static_cast<Pigment>(pigment)).hsl().hue;
        }
        return result;
    }
}
