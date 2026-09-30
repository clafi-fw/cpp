// =========================================================================
// ClaFi.Application.ThemesManager_Elements
//
// WHAT A THEME IS MADE OF, STATED ONCE. Every element a theme colours, the
// three spellings it goes by, what it is painted on, and for the elements
// an ink names, the ThemeColors member holding that effect.
//
// Four readers share it and none keeps a list of its own: the serializers
// build their field tuples from this table, the Themes app names its pages
// from it, the C++ code generator names members and elements from it, and bake below
// turns a theme into the set the paint path reads. An element added here
// reaches all four; one added anywhere else reaches none.
// =========================================================================
export module ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // What one element is: its spellings, what it stands on, and its effect. See Application
    export struct UiElementDescriptor
    {
        std::wstring_view name{};
        std::wstring_view codeName{};
        std::wstring_view token{};
        ColorEffect ThemeColors::* effect{ nullptr }; // the effect an ink names, where there is one
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
        // AppTheme's WindowShadow. A root stands on the bare colour of the mode, which is what
        // PaintEvent seeds it with - see BakedColors::rootSurface.
        bool isWindowRoot{ false };
    };

    // Indexed by UiElement, so it and this table must stay in step.
    export constexpr std::array<UiElementDescriptor, static_cast<std::size_t>(UiElement::Count)> k_uiElements{
        // A WINDOW ROOT. Menu and Hint are the other two, and the three carry the same pair -
        // the surface the window is filled with and the ink everything in it starts from - and
        // the window's frame: the stroke around it and the shadow it casts. Nothing is under a
        // root, so this one has no base - it stands on the bare colour of the mode.
        UiElementDescriptor{
            .name = L"Dialog",
            .codeName = L"dialog",
            .token = L"Dialog",
            .isWindowRoot = true
        },
        UiElementDescriptor{
            .name = L"Page",
            .codeName = L"page",
            .token = L"Page",
            .base = UiElement::Dialog
        },
        // The open tab wears its page's surface, and its line runs the length of the page.
        UiElementDescriptor{
            .name = L"Tab",
            .codeName = L"tab",
            .token = L"Tab",
            .base = UiElement::Page
        },
        UiElementDescriptor{
            .name = L"Section",
            .codeName = L"section",
            .token = L"Section",
            .base = UiElement::Page
        },
        // The strip an expander shows its title on; a grid draws a section's line from its stroke.
        UiElementDescriptor{
            .name = L"Section Header",
            .codeName = L"sectionHeader",
            .token = L"SectionHeader",
            .base = UiElement::Section
        },
        // A tool bar is painted on the window's content, the same thing a section stands on.
        UiElementDescriptor{
            .name = L"Tool Bar",
            .codeName = L"toolBar",
            .token = L"ToolBar",
            .base = UiElement::Page
        },
        UiElementDescriptor{
            .name = L"Dialog Title",
            .codeName = L"dialogTitle",
            .token = L"DialogTitle",
            .base = UiElement::Dialog
        },
        UiElementDescriptor{
            .name = L"Menu",
            .codeName = L"menu",
            .token = L"Menu",
            .isWindowRoot = true
        },
        UiElementDescriptor{
            .name = L"Hint",
            .codeName = L"hint",
            .token = L"Hint",
            .isWindowRoot = true
        },
        // Its stroke is the line a divider row of a grid carries.
        UiElementDescriptor{
            .name = L"Divider",
            .codeName = L"divider",
            .token = L"Divider",
            .base = UiElement::Section
        },
        // Its stroke is the grid's outer border; each row draws the lines inside with its own.
        UiElementDescriptor{
            .name = L"Grid",
            .codeName = L"grid",
            .token = L"Grid",
            .base = UiElement::Section
        },
        // The row of column names across the top of a grid; its stroke is the lines between them.
        UiElementDescriptor{
            .name = L"Grid Header",
            .codeName = L"gridHeader",
            .token = L"GridHeader",
            .base = UiElement::Grid
        },
        // Every row of a grid, groups and sections included; its stroke is the lines between cells.
        UiElementDescriptor{
            .name = L"Grid Row",
            .codeName = L"gridRow",
            .token = L"GridRow",
            .base = UiElement::Grid
        },
        UiElementDescriptor{
            .name = L"Button",
            .codeName = L"button",
            .token = L"Button",
            .base = UiElement::Section
        },
        // A button whose surface arrives with the pointer: a toolbar's, a menu line, a list item.
        UiElementDescriptor{
            .name = L"Tool Button",
            .codeName = L"toolButton",
            .token = L"ToolButton",
            .base = UiElement::Section
        },
        // The band behind selected text, and the ink drawn over it.
        UiElementDescriptor{
            .name = L"Selected Text",
            .codeName = L"selectedText",
            .token = L"SelectedText",
            .base = UiElement::Section
        },
        // The band behind what a search has found, started from its seed pigment's hue.
        UiElementDescriptor{
            .name = L"Found Text",
            .codeName = L"foundText",
            .token = L"FoundText",
            .base = UiElement::Section
        },
        // The box of a check and the ring of a radio button, with the colour each takes when on.
        UiElementDescriptor{
            .name = L"Selection Indicator",
            .codeName = L"selectionIndicator",
            .token = L"SelectionIndicator",
            .base = UiElement::Section
        },
        // The mark of a button whose IndicatorVisibility is Hover, with nothing at rest.
        UiElementDescriptor{
            .name = L"Hover Indicator",
            .codeName = L"hoverIndicator",
            .token = L"HoverIndicator",
            .base = UiElement::Section
        },
        // THREE THINGS IN ONE EFFECT: the theme's own emphasis, the ring on the control the user is
        // on, and the indicator under an open tab. They are one colour on purpose - the interface
        // says "this one" in a single ink - so they are edited as one row.
        UiElementDescriptor{
            .name = L"Accent",
            .codeName = L"accent",
            .token = L"Accent",
            .effect = &ThemeColors::accent,
            .base = UiElement::Section
        },
        UiElementDescriptor{
            .name = L"Spot",
            .codeName = L"spot",
            .token = L"Spot",
            .effect = &ThemeColors::spot,
            .base = UiElement::Section
        },
        UiElementDescriptor{
            .name = L"Scroll Button",
            .codeName = L"scrollButton",
            .token = L"ScrollButton",
            .base = UiElement::Section
        },
        UiElementDescriptor{
            .name = L"Scroll Thumb",
            .codeName = L"scrollThumb",
            .token = L"ScrollThumb",
            .base = UiElement::Section
        },
        // THE TWO TEST SUBJECTS. Only the rules a theme states for them reach them: with none,
        // each paints no surface and draws its text in the ink it inherits.
        UiElementDescriptor{
            .name = L"Testee",
            .codeName = L"testee",
            .token = L"Testee",
            .base = UiElement::Section
        },
        UiElementDescriptor{
            .name = L"Bestee",
            .codeName = L"bestee",
            .token = L"Bestee",
            .base = UiElement::Section
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
        result.darkModeFloor = themeColors.darkModeFloor;

        for (std::size_t i = 0; i < k_uiElements.size(); ++i)
        {
            const UiElementDescriptor& descriptor = k_uiElements[i];
            if (descriptor.effect != nullptr)
                result.effects[i] = bake(themeColors.*descriptor.effect, themeColors);
        }

        // The harmony answers here and nowhere after: what a pigment stands for is the theme's
        // to say, and a hue is a value two themes have a half way between.
        for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
        {
            result.pigmentHues[pigment] = themeColors.harmony()
                .pigmentColor(static_cast<Pigment>(pigment)).hsl().hue;
        }
        result.rules = bake(themeColors.rules, themeColors);
        return result;
    }
}
