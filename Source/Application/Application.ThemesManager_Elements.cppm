// =========================================================================
// ClaFi.Application.ThemesManager_Elements
//
// **What a theme is made of, stated once.** Every element a theme styles, the
// three spellings it goes by, and for the elements an ink names, the
// ThemeColors member holding that effect.
//
// Four readers share it and none keeps a list of its own: the serializers
// build their field tuples from this table, the Themes app names its pages
// from it, the C++ code generator names members and elements from it, and bake below
// turns a theme into the colours the paint path reads. An element added here
// reaches all four; one added anywhere else reaches none.
// =========================================================================
export module ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // What one element is: its spellings, and the effect an ink names where there is one. What
    // it is painted on and what it stands over are the theme's to say - see ControlStyle.
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
        // A section that opens and shuts under its header, framed as a page is.
        UiElementDescriptor{
            .name = L"Expander",
            .codeName = L"expander",
            .token = L"Expander"
        },
        // A section holding one answer - a box, a list - at a button's corner and inset.
        UiElementDescriptor{
            .name = L"Field",
            .codeName = L"field",
            .token = L"Field"
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
        // The list a combo box or a drop list opens under itself: a window, surfaced as a section.
        UiElementDescriptor{
            .name = L"Dropdown",
            .codeName = L"dropdown",
            .token = L"Dropdown"
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
        // The sizes a grid lays its cells out by; the rows paint them.
        UiElementDescriptor{
            .name = L"Grid Cell",
            .codeName = L"gridCell",
            .token = L"GridCell"
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
        // The row of a check box or a radio button: the mark and its caption, as a list lays them.
        UiElementDescriptor{
            .name = L"List Item",
            .codeName = L"listItem",
            .token = L"ListItem"
        },
        // A field the user types into, and the box a code view shows its lines in.
        UiElementDescriptor{
            .name = L"Text Box",
            .codeName = L"textBox",
            .token = L"TextBox"
        },
        // The track a value is dragged along; its band shows while the slider is in edit mode.
        UiElementDescriptor{
            .name = L"Slider",
            .codeName = L"slider",
            .token = L"Slider"
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
        // The bar a scroll box is moved by, and the strip the box stands its bars in.
        UiElementDescriptor{
            .name = L"Scroll Bar",
            .codeName = L"scrollBar",
            .token = L"ScrollBar"
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
        // and Don't block are - see ControlStyle seed. Its band is drawn in its stroke colour.
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

    // A theme as the paint path reads it, in the mode it is worn in, laid out by the framework's
    // elements and then the application's own. See Application#bake
    export [[nodiscard]] BakedColors bake(const AppTheme&, ColorMode, AppElementNames);
    // Every style's sizes as the layout reads them, laid out the same way. See Application
    export [[nodiscard]] StyleMetrics resolveStyleMetrics(const AppTheme&, AppElementNames);

    // The names every element goes by, in UiElement order: the framework's, then the
    // application's.
    export [[nodiscard]] std::vector<std::wstring_view> elementNamesOf(AppElementNames);

    // A style's surface at rest, walked down from the bare colour of the mode along standsOn.
    export [[nodiscard]] Hsl restingSurface(const AppTheme&, std::wstring_view style, ColorMode);
    // The ink a style's text starts from, walked down the same nesting through text rules.
    export [[nodiscard]] Hsl restingInk(const AppTheme&, std::wstring_view style, ColorMode);
    // A style's stroke at rest, which starts at its resting surface.
    export [[nodiscard]] Hsl restingStroke(const AppTheme&, std::wstring_view style, ColorMode);
    // The same three for an element, by the token the theme names its style with.
    export [[nodiscard]] Hsl restingSurface(const AppTheme&, OptionalUiElement, ColorMode);
    export [[nodiscard]] Hsl restingInk(const AppTheme&, OptionalUiElement, ColorMode);
    export [[nodiscard]] Hsl restingStroke(const AppTheme&, OptionalUiElement, ColorMode);


    //-------------------------------------------------------------------------


    namespace
    {
        // A style's resolved rules at rest on one channel. True where one of them names a hue.
        bool applyRestingRules(Hsl& color, const AppTheme& theme, const std::wstring_view style,
            const PaintChannel channel, const ColorMode mode)
        {
            bool namesHue = false;
            for (const ColorRule& rule : theme.styles.resolvedRules(style))
            {
                if (!rule.atRest() or rule.output != channel)
                    continue;
                rule.effect.applyTo(color, 1.0f, theme.colors, mode);
                if (rule.effect.hue.operation() != ColorRuleHueOp::NoChange)
                    namesHue = true;
            }
            return namesHue;
        }

        // The ink of the colour mode before any rule: white at the dark end, black at the light
        // one, in the hue of the bare surface - the theme's anchor.
        [[nodiscard]] Hsl bareInk(const AppTheme& theme, const ColorMode mode)
        {
            return {
                theme.colors.rootSurface(mode).hue,
                0.0f,
                mode == ColorMode::Dark ? 1.0f : 0.0f
            };
        }

        // Only text rules are applied on the way: what a style is painted on is a separate chain
        // and does not reach the ink. No style is the bare ink of the colour mode, with nothing
        // applied - a window root's own text rule is the first thing on the chain, exactly as its
        // surface rule is the first thing on the other one. The one thing the surface lends the
        // ink is its hue, until a text rule on the way names one - the seed PaintEvent takes.
        [[nodiscard]] Hsl walkInk(const AppTheme& theme, const std::wstring_view style,
            const ColorMode mode, bool& hueNamed)
        {
            const ControlStyle* stated = theme.styles.of(style);
            if (!stated)
                return bareInk(theme, mode);

            bool named = false;
            Hsl result = walkInk(theme, stated->standsOn, mode, named);
            if (!named)
                result.hue = restingSurface(theme, style, mode).hue;
            if (applyRestingRules(result, theme, style, PaintChannel::Text, mode))
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

        // **These sizes follow from others**, whatever a style states. See Application#settle
        void settle(const UiElement element, ControlMetrics& metrics)
        {
            switch (element)
            {
                case UiElement::Dialog:
                {
                    const float windowBorder = strokeWidth(metrics.border);
                    metrics.padding = { windowBorder, windowBorder };
                    break;
                }
                case UiElement::SelectionIndicator:
                case UiElement::HoverIndicator:
                    metrics.minSize.y = metrics.minSize.x;
                    metrics.maxSize = MaxSize{ metrics.minSize };
                    break;
                case UiElement::ScrollButton:
                    metrics.minSize.y = metrics.minSize.x;
                    break;
                default:
                    break;
            }
        }
    }

    BakedColors bake(const AppTheme& theme, ColorMode mode, const AppElementNames appElements)
    {
        BakedColors result{};
        result.lightness = lightnessOf(mode);
        result.anchorHue = theme.colors.anchorHue;
        result.darkModeFloor = theme.colors.darkModeFloor;

        for (std::size_t i = 0; i < k_uiElements.size(); ++i)
        {
            const UiElementDescriptor& descriptor = k_uiElements[i];
            if (descriptor.effect != nullptr)
                result.effects[i] = bake(theme.colors.*descriptor.effect, theme.colors);
        }

        // The harmony answers here and nowhere after: what a pigment stands for is the theme's
        // to say, and a hue is a value two themes have a half way between.
        for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
        {
            result.pigmentHues[pigment] = theme.colors.harmony()
                .pigmentColor(static_cast<Pigment>(pigment)).hsl().hue;
        }
        result.rules = bake(theme.styles, theme.colors, elementNamesOf(appElements));
        return result;
    }

    StyleMetrics resolveStyleMetrics(const AppTheme& theme, const AppElementNames appElements)
    {
        const std::vector<std::wstring_view> names = elementNamesOf(appElements);
        StyleMetrics result{};
        result.elements.reserve(names.size());
        for (std::size_t i = 0ull; i != names.size(); ++i)
        {
            ControlMetrics& metrics = result.elements.emplace_back(
                theme.styles.resolvedMetrics(names[i]));
            settle(static_cast<UiElement>(i), metrics);
        }
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

    // Only resting rules are applied: nothing below the style is in a state. A style stated
    // nowhere stands on the bare colour of the mode, as a window root does.
    Hsl restingSurface(const AppTheme& theme, const std::wstring_view style, const ColorMode mode)
    {
        const ControlStyle* stated = theme.styles.of(style);
        if (!stated)
            return theme.colors.rootSurface(mode);

        Hsl result = restingSurface(theme, stated->standsOn, mode);
        // The hue ControlPaintContext::foundTextRgb starts the band from.
        if (stated->seed)
            result.hue = theme.colors.harmony().pigmentColor(*stated->seed).hsl().hue;
        applyRestingRules(result, theme, style, PaintChannel::Surface, mode);
        return result;
    }

    Hsl restingInk(const AppTheme& theme, const std::wstring_view style, const ColorMode mode)
    {
        bool hueNamed = false;
        return walkInk(theme, style, mode, hueNamed);
    }

    Hsl restingStroke(const AppTheme& theme, const std::wstring_view style, const ColorMode mode)
    {
        Hsl result = restingSurface(theme, style, mode);
        applyRestingRules(result, theme, style, PaintChannel::Stroke, mode);
        return result;
    }

    // An element naming an effect rather than a style - the accent and the spot - shows that
    // effect over what it stands on.
    Hsl restingSurface(const AppTheme& theme, const OptionalUiElement element, const ColorMode mode)
    {
        if (!element)
            return theme.colors.rootSurface(mode);
        const std::wstring_view token = uiElementOf(*element).token;
        if (const ColorEffect ThemeColors::* effect = effectOfToken(token))
        {
            Hsl result = restingSurface(theme, uiElementOf(UiElement::Section).token, mode);
            (theme.colors.*effect).applyTo(result, 1.0f, theme.colors, mode);
            return result;
        }
        return restingSurface(theme, token, mode);
    }

    Hsl restingInk(const AppTheme& theme, const OptionalUiElement element, const ColorMode mode)
    {
        if (!element)
            return bareInk(theme, mode);
        return restingInk(theme, uiElementOf(*element).token, mode);
    }

    Hsl restingStroke(const AppTheme& theme, const OptionalUiElement element, const ColorMode mode)
    {
        if (!element)
            return theme.colors.rootSurface(mode);
        return restingStroke(theme, uiElementOf(*element).token, mode);
    }
}
