// =========================================================================
// ClaFi.Application.ThemesManager_Elements
//
// **What a theme is made of, stated once.** Every element a theme colours, the
// three spellings it goes by, what it is painted on, and for the elements
// an ink names, the ThemeColors member holding that effect - and every set of
// sizes a theme states, with the three spellings it goes by.
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
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // What one element is: its spellings, and the effect an ink names where there is one. What
    // it is painted on and what it stands over are the theme's to say - see ElementRules.
    export struct UiElementDescriptor
    {
        std::wstring_view name{};
        std::wstring_view codeName{};
        std::wstring_view token{};
        ColorEffect ThemeColors::* effect{ nullptr }; // the effect an ink names, where there is one
    };

    // Indexed by UiElement, so it and this table must stay in step.
    export constexpr std::array<UiElementDescriptor, static_cast<std::size_t>(UiElement::Count)> k_uiElements{
        // **A window root.** Menu and Tooltip are the other two, and the three carry the same
        // pair - the surface the window is filled with and the ink everything in it starts from -
        // and, through the Any Window set they stand over, the window's frame: the stroke around
        // it and the shadow it casts.
        UiElementDescriptor{
            .name = L"Dialog",
            .codeName = L"dialog",
            .token = L"Dialog"
        },
        UiElementDescriptor{
            .name = L"Page",
            .codeName = L"page",
            .token = L"Page"
        },
        // The open tab wears its page's surface, and its line runs the length of the page.
        UiElementDescriptor{
            .name = L"Tab",
            .codeName = L"tab",
            .token = L"Tab"
        },
        UiElementDescriptor{
            .name = L"Section",
            .codeName = L"section",
            .token = L"Section"
        },
        // The strip an expander shows its title on; a grid draws a section's line from its stroke.
        UiElementDescriptor{
            .name = L"Section Header",
            .codeName = L"sectionHeader",
            .token = L"SectionHeader"
        },
        // A tool bar is painted on the window's content, the same thing a section stands on.
        UiElementDescriptor{
            .name = L"Tool Bar",
            .codeName = L"toolBar",
            .token = L"ToolBar"
        },
        UiElementDescriptor{
            .name = L"Dialog Title",
            .codeName = L"dialogTitle",
            .token = L"DialogTitle"
        },
        UiElementDescriptor{
            .name = L"Menu",
            .codeName = L"menu",
            .token = L"Menu"
        },
        UiElementDescriptor{
            .name = L"Tooltip",
            .codeName = L"tooltip",
            .token = L"Tooltip"
        },
        // Its stroke is the line a divider row of a grid carries.
        UiElementDescriptor{
            .name = L"Divider",
            .codeName = L"divider",
            .token = L"Divider"
        },
        // Its stroke is the grid's outer border; each row draws the lines inside with its own.
        UiElementDescriptor{
            .name = L"Grid",
            .codeName = L"grid",
            .token = L"Grid"
        },
        // The row of column names across the top of a grid; its stroke is the lines between them.
        UiElementDescriptor{
            .name = L"Grid Header",
            .codeName = L"gridHeader",
            .token = L"GridHeader"
        },
        // Every row of a grid, groups and sections included; its stroke is the lines between cells.
        UiElementDescriptor{
            .name = L"Grid Row",
            .codeName = L"gridRow",
            .token = L"GridRow"
        },
        UiElementDescriptor{
            .name = L"Button",
            .codeName = L"button",
            .token = L"Button"
        },
        // A button whose surface arrives with the pointer: a toolbar's, a menu line, a list item.
        UiElementDescriptor{
            .name = L"Tool Button",
            .codeName = L"toolButton",
            .token = L"ToolButton"
        },
        // A field the user types into, and the box a code view shows its lines in.
        UiElementDescriptor{
            .name = L"Text Box",
            .codeName = L"textBox",
            .token = L"TextBox"
        },
        // The band behind selected text, and the ink drawn over it.
        UiElementDescriptor{
            .name = L"Selected Text",
            .codeName = L"selectedText",
            .token = L"SelectedText"
        },
        // The band behind what a search has found, started from its seed pigment's hue.
        UiElementDescriptor{
            .name = L"Found Text",
            .codeName = L"foundText",
            .token = L"FoundText"
        },
        // The box of a check and the ring of a radio button, with the colour each takes when on.
        UiElementDescriptor{
            .name = L"Selection Indicator",
            .codeName = L"selectionIndicator",
            .token = L"SelectionIndicator"
        },
        // The mark of a button whose IndicatorVisibility is Hover, with nothing at rest.
        UiElementDescriptor{
            .name = L"Hover Indicator",
            .codeName = L"hoverIndicator",
            .token = L"HoverIndicator"
        },
        // **Two things in one effect**: the theme's own emphasis and the ring on the control the
        // user is on. They are one colour on purpose - the interface says "this one" in a single
        // ink - so they are edited as one row.
        UiElementDescriptor{
            .name = L"Accent",
            .codeName = L"accent",
            .token = L"Accent",
            .effect = &ThemeColors::accent
        },
        UiElementDescriptor{
            .name = L"Spot",
            .codeName = L"spot",
            .token = L"Spot",
            .effect = &ThemeColors::spot
        },
        UiElementDescriptor{
            .name = L"Scroll Button",
            .codeName = L"scrollButton",
            .token = L"ScrollButton"
        },
        UiElementDescriptor{
            .name = L"Scroll Thumb",
            .codeName = L"scrollThumb",
            .token = L"ScrollThumb"
        },
        // **A note set apart, one element for each kind of message** - a message bar, a notice, a
        // callout in a page of prose - each started from its kind's pigment, as the sides of a Do
        // and Don't block are - see ElementRules seed. Its band is drawn in its stroke colour.
        UiElementDescriptor{
            .name = L"Info Callout",
            .codeName = L"infoCallout",
            .token = L"InfoCallout"
        },
        UiElementDescriptor{
            .name = L"Question Callout",
            .codeName = L"questionCallout",
            .token = L"QuestionCallout"
        },
        UiElementDescriptor{
            .name = L"Warning Callout",
            .codeName = L"warningCallout",
            .token = L"WarningCallout"
        },
        UiElementDescriptor{
            .name = L"Error Callout",
            .codeName = L"errorCallout",
            .token = L"ErrorCallout"
        },
        UiElementDescriptor{
            .name = L"Ok Callout",
            .codeName = L"okCallout",
            .token = L"OkCallout"
        },
        UiElementDescriptor{
            .name = L"Do",
            .codeName = L"doSide",
            .token = L"Do"
        },
        UiElementDescriptor{
            .name = L"Don't",
            .codeName = L"dontSide",
            .token = L"Dont"
        },
        UiElementDescriptor{
            .name = L"Code Block",
            .codeName = L"codeBlock",
            .token = L"CodeBlock"
        }
    };

    export constexpr const UiElementDescriptor& uiElementOf(UiElement element)
    {
        return k_uiElements[static_cast<std::size_t>(element)];
    }

    // The spellings one set of sizes goes by. See Application
    export struct MetricsSetDescriptor
    {
        std::wstring_view name{};
        std::wstring_view codeName{};
        std::wstring_view token{};
    };

    // Indexed by MetricsSet, so it and this table must stay in step.
    export constexpr std::array<MetricsSetDescriptor, k_metricsSetCount> k_metricsSets{
        MetricsSetDescriptor{ .name = L"Button", .codeName = L"button", .token = L"Button" },
        MetricsSetDescriptor{
            .name = L"Tool Button",
            .codeName = L"toolButton",
            .token = L"ToolButton"
        },
        MetricsSetDescriptor{
            .name = L"Dialog Title",
            .codeName = L"dialogTitle",
            .token = L"DialogTitle"
        },
        MetricsSetDescriptor{
            .name = L"Primary Window",
            .codeName = L"primaryWindow",
            .token = L"PrimaryWindow"
        },
        MetricsSetDescriptor{
            .name = L"Secondary Window",
            .codeName = L"secondaryWindow",
            .token = L"SecondaryWindow"
        },
        MetricsSetDescriptor{ .name = L"Page", .codeName = L"page", .token = L"Page" },
        MetricsSetDescriptor{
            .name = L"Check Mark",
            .codeName = L"checkMark",
            .token = L"CheckMark"
        },
        MetricsSetDescriptor{ .name = L"List Item", .codeName = L"listItem", .token = L"ListItem" },
        MetricsSetDescriptor{ .name = L"Grid Cell", .codeName = L"gridCell", .token = L"GridCell" },
        MetricsSetDescriptor{
            .name = L"Scroll Button",
            .codeName = L"scrollButton",
            .token = L"ScrollButton"
        },
        MetricsSetDescriptor{
            .name = L"Scroll Thumb",
            .codeName = L"scrollThumb",
            .token = L"ScrollThumb"
        },
        MetricsSetDescriptor{ .name = L"Tab", .codeName = L"tab", .token = L"Tab" },
        MetricsSetDescriptor{ .name = L"Callout", .codeName = L"callout", .token = L"Callout" },
        MetricsSetDescriptor{
            .name = L"Do and Don't",
            .codeName = L"doDont",
            .token = L"DoDont"
        },
        MetricsSetDescriptor{
            .name = L"Code Block",
            .codeName = L"codeBlock",
            .token = L"CodeBlock"
        },
        MetricsSetDescriptor{ .name = L"Text Box", .codeName = L"textBox", .token = L"TextBox" },
        MetricsSetDescriptor{ .name = L"Slider", .codeName = L"slider", .token = L"Slider" }
    };

    export constexpr const MetricsSetDescriptor& metricsSetOf(MetricsSet set)
    {
        return k_metricsSets[static_cast<std::size_t>(set)];
    }

    // A theme as the paint path reads it, in the mode it is worn in, laid out by the framework's
    // elements and then the application's own. See Application#bake
    export [[nodiscard]] BakedColors bake(const ThemeColors&, ColorMode, AppElementNames);

    // The names every element goes by, in UiElement order: the framework's, then the
    // application's.
    export [[nodiscard]] std::vector<std::wstring_view> elementNamesOf(AppElementNames);

    // A set's surface at rest, walked down from the bare colour of the mode along standsOn.
    export [[nodiscard]] Hsl restingSurface(const ThemeColors&, std::wstring_view set, ColorMode);
    // The ink a set's text starts from, walked down the same nesting through text rules.
    export [[nodiscard]] Hsl restingInk(const ThemeColors&, std::wstring_view set, ColorMode);
    // A set's stroke at rest, which starts at its resting surface.
    export [[nodiscard]] Hsl restingStroke(const ThemeColors&, std::wstring_view set, ColorMode);
    // The same three for an element, by the token the theme names its set with.
    export [[nodiscard]] Hsl restingSurface(const ThemeColors&, OptionalUiElement, ColorMode);
    export [[nodiscard]] Hsl restingInk(const ThemeColors&, OptionalUiElement, ColorMode);
    export [[nodiscard]] Hsl restingStroke(const ThemeColors&, OptionalUiElement, ColorMode);


    //-------------------------------------------------------------------------


    namespace
    {
        // A set's resolved rules at rest on one channel. True where one of them names a hue.
        bool applyRestingRules(Hsl& color, const ThemeColors& themeColors,
            const std::wstring_view set, const PaintChannel channel, const ColorMode mode)
        {
            bool namesHue = false;
            for (const ColorRule& rule : themeColors.rules.resolvedRules(set))
            {
                if (!rule.atRest() or rule.output != channel)
                    continue;
                rule.effect.applyTo(color, 1.0f, themeColors, mode);
                if (rule.effect.hue.operation() != ColorRuleHueOp::NoChange)
                    namesHue = true;
            }
            return namesHue;
        }

        // The ink of the colour mode before any rule: white at the dark end, black at the light
        // one, in the hue of the bare surface - the theme's anchor.
        [[nodiscard]] Hsl bareInk(const ThemeColors& themeColors, const ColorMode mode)
        {
            return {
                themeColors.rootSurface(mode).hue,
                0.0f,
                mode == ColorMode::Dark ? 1.0f : 0.0f
            };
        }

        // Only text rules are applied on the way: what a set is painted on is a separate chain
        // and does not reach the ink. No set is the bare ink of the colour mode, with nothing
        // applied - a window root's own text rule is the first thing on the chain, exactly as its
        // surface rule is the first thing on the other one. The one thing the surface lends the
        // ink is its hue, until a text rule on the way names one - the seed PaintEvent takes.
        [[nodiscard]] Hsl walkInk(const ThemeColors& themeColors, const std::wstring_view set,
            const ColorMode mode, bool& hueNamed)
        {
            const ElementRules* stated = themeColors.rules.of(set);
            if (!stated)
                return bareInk(themeColors, mode);

            bool named = false;
            Hsl result = walkInk(themeColors, stated->standsOn, mode, named);
            if (!named)
                result.hue = restingSurface(themeColors, set, mode).hue;
            if (applyRestingRules(result, themeColors, set, PaintChannel::Text, mode))
                named = true;
            hueNamed = named;
            return result;
        }

        // The effect an element names, where its token names one - the accent and the spot.
        [[nodiscard]] ColorEffect ThemeColors::* effectOfToken(const std::wstring_view token)
        {
            for (const UiElementDescriptor& descriptor : k_uiElements)
                if (descriptor.token == token)
                    return descriptor.effect;
            return nullptr;
        }
    }

    BakedColors bake(const ThemeColors& themeColors, ColorMode mode,
        const AppElementNames appElements)
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
        result.rules = bake(themeColors.rules, themeColors, elementNamesOf(appElements));
        return result;
    }

    std::vector<std::wstring_view> elementNamesOf(const AppElementNames appElements)
    {
        std::vector<std::wstring_view> result{};
        result.reserve(k_uiElements.size() + appElements.size());
        for (const UiElementDescriptor& descriptor : k_uiElements)
            result.push_back(descriptor.token);
        result.insert(result.end(), appElements.begin(), appElements.end());
        return result;
    }

    // Only resting rules are applied: nothing below the set is in a state. A set stated nowhere
    // stands on the bare colour of the mode, as a window root does.
    Hsl restingSurface(const ThemeColors& themeColors, const std::wstring_view set,
        const ColorMode mode)
    {
        const ElementRules* stated = themeColors.rules.of(set);
        if (!stated)
            return themeColors.rootSurface(mode);

        Hsl result = restingSurface(themeColors, stated->standsOn, mode);
        // The hue ControlPaintContext::foundTextRgb starts the band from.
        if (stated->seed)
            result.hue = themeColors.harmony().pigmentColor(*stated->seed).hsl().hue;
        applyRestingRules(result, themeColors, set, PaintChannel::Surface, mode);
        return result;
    }

    Hsl restingInk(const ThemeColors& themeColors, const std::wstring_view set,
        const ColorMode mode)
    {
        bool hueNamed = false;
        return walkInk(themeColors, set, mode, hueNamed);
    }

    Hsl restingStroke(const ThemeColors& themeColors, const std::wstring_view set,
        const ColorMode mode)
    {
        Hsl result = restingSurface(themeColors, set, mode);
        applyRestingRules(result, themeColors, set, PaintChannel::Stroke, mode);
        return result;
    }

    // An element naming an effect rather than a set - the accent and the spot - shows that
    // effect over what it stands on.
    Hsl restingSurface(const ThemeColors& themeColors, const OptionalUiElement element,
        const ColorMode mode)
    {
        if (!element)
            return themeColors.rootSurface(mode);
        const std::wstring_view token = uiElementOf(*element).token;
        if (const ColorEffect ThemeColors::* effect = effectOfToken(token))
        {
            Hsl result = restingSurface(themeColors, uiElementOf(UiElement::Section).token, mode);
            (themeColors.*effect).applyTo(result, 1.0f, themeColors, mode);
            return result;
        }
        return restingSurface(themeColors, token, mode);
    }

    Hsl restingInk(const ThemeColors& themeColors, const OptionalUiElement element,
        const ColorMode mode)
    {
        if (!element)
            return bareInk(themeColors, mode);
        return restingInk(themeColors, uiElementOf(*element).token, mode);
    }

    Hsl restingStroke(const ThemeColors& themeColors, const OptionalUiElement element,
        const ColorMode mode)
    {
        if (!element)
            return themeColors.rootSurface(mode);
        return restingStroke(themeColors, uiElementOf(*element).token, mode);
    }
}
