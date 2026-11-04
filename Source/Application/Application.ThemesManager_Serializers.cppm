export module ClaFi.Application.ThemesManager_Serializers;

export import ClaFi.Core.Dom_UiSerializers;

import ClaFi.Application.ThemesManager_Elements;
import ClaFi.App.ThemeIcon;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.Serialization;
import ClaFi.StdLib;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Dom
{
    // A rule's inputs are one scalar in a document: their names, a space apart, and nothing at
    // rest. A name no input goes by is passed over.
    template <>
    struct ScalarSerializer<RuleInputs>
    {
        [[nodiscard]] static std::wstring toWString(const RuleInputs& inputs);
        static void fromWString(std::wstring_view str, RuleInputs& inputs);
    };

    // A seed is its pigment's name, and nothing where a set states none. A name no pigment goes
    // by reads as none.
    template <>
    struct ScalarSerializer<OptionalPigment>
    {
        [[nodiscard]] static std::wstring toWString(const OptionalPigment& seed);
        static void fromWString(std::wstring_view str, OptionalPigment& seed);
    };
}

namespace ClaFi // AppTheme serializers
{
    export constexpr auto enumNames(ColorRuleOp)
    {
        return std::array{
            L"NoChange",
            L"Offset",
            L"Scale",
            L"Set",
        };
    }

    export constexpr auto serializedFields(const ColorRuleValue&) {
        return std::make_tuple(
            SerializedField{ L"Operation", &ColorRuleValue::operation, &ColorRuleValue::setOperation },
            SerializedField{ L"NormalizedValue", &ColorRuleValue::normalizedValue, &ColorRuleValue::setNormalizedValue }
        );
    }

    export constexpr auto enumNames(ColorRuleHueOp)
    {
        return std::array{
            L"PaletteColor1",
            L"PaletteColor2",
            L"PaletteColor3",
            L"NoChange",
            L"ExactValue",
        };
    }

    export constexpr auto serializedFields(const ColorRuleHue&) {
        return std::make_tuple(
            SerializedField{ L"Operation", &ColorRuleHue::operation, &ColorRuleHue::setOperation },
            SerializedField{ L"ExactValue", &ColorRuleHue::exactValue, &ColorRuleHue::setExactValue }
        );
    }

    export constexpr auto serializedFields(const ColorEffect&) {
        return std::make_tuple(
            SerializedField{ L"Hue", &ColorEffect::hue },
            SerializedField{ L"Saturation", &ColorEffect::saturation },
            SerializedField{ L"Elevation", &ColorEffect::elevation }
        );
    }

    // **One field at a time, because only some elements name an effect.** Each index answers with a
    // tuple of its own, empty where the element names none, and tuple_cat joins them.
    template <std::size_t I>
    constexpr auto elementField()
    {
        if constexpr (k_uiElements[I].effect != nullptr)
        {
            return std::make_tuple(
                SerializedField{ k_uiElements[I].token, k_uiElements[I].effect });
        }
        else
            return std::tuple<>{};
    }

    template <std::size_t... I>
    constexpr auto elementFields(std::index_sequence<I...>)
    {
        return std::tuple_cat(elementField<I>()...);
    }

    export constexpr auto enumNames(ColorHarmonyKind) { return k_harmonyKeys; }

    export constexpr auto serializedFields(const Hsl&) {
        return std::make_tuple(
            SerializedField{ L"Hue", &Hsl::hue },
            SerializedField{ L"Saturation", &Hsl::saturation },
            SerializedField{ L"Luminosity", &Hsl::luminosity }
        );
    }

    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(RuleInput::Count)>
        k_ruleInputKeys{
            L"Hot",
            L"Pressed",
            L"Focused",
            L"Selected",
            L"Disabled",
            L"Current",
            L"Edit",
            L"WindowFocused"
        };

    export constexpr auto enumNames(RuleInput) { return k_ruleInputKeys; }

    export constexpr auto enumNames(PaintChannel)
    {
        return std::array{
            L"Surface",
            L"Stroke",
            L"Band",
            L"Text",
            L"Shadow",
        };
    }

    export constexpr auto serializedFields(const ColorRule&) {
        return std::make_tuple(
            SerializedField{ L"Inputs", &ColorRule::inputs },
            SerializedField{ L"AndInputs", &ColorRule::andInputs },
            SerializedField{ L"Output", &ColorRule::output },
            SerializedField{ L"Effect", &ColorRule::effect }
        );
    }

    export constexpr auto serializedFields(const RuleSelector&) {
        return std::make_tuple(
            SerializedField{ L"Inputs", &RuleSelector::inputs },
            SerializedField{ L"AndInputs", &RuleSelector::andInputs },
            SerializedField{ L"Output", &RuleSelector::output }
        );
    }

    export constexpr std::array<std::wstring_view, k_pigmentsCount> k_pigmentKeys{
        L"Yellow",
        L"Green",
        L"Blue",
        L"Red"
    };

    // A set stands whole in a file, its name first. See Application#elementrules-in-a-file
    export constexpr auto serializedFields(const ElementRules&) {
        return std::make_tuple(
            SerializedField{ L"Name", &ElementRules::name },
            SerializedField{ L"Base", &ElementRules::base },
            SerializedField{ L"StandsOn", &ElementRules::standsOn },
            SerializedField{ L"Seed", &ElementRules::seed },
            SerializedField{ L"Rules", &ElementRules::rules },
            SerializedField{ L"Dropped", &ElementRules::dropped }
        );
    }

    // The palette and the rules are stated here and the effects come from k_uiElements, in that
    // table's order, which is ThemeColors declaration order. Field order in a document is a
    // reading matter - a field is found by name - so the table's order is free to be the one
    // generated C++ needs.
    export constexpr auto serializedFields(const ThemeColors&) {
        return std::tuple_cat(
            std::make_tuple(
                SerializedField{ L"Harmony", &ThemeColors::harmonyKind },
                SerializedField{ L"AnchorHue", &ThemeColors::anchorHue },
                SerializedField{ L"PaletteHues", &ThemeColors::paletteHues },
                SerializedField{ L"DarkModeFloor", &ThemeColors::darkModeFloor }
            ),
            elementFields(std::make_index_sequence<k_uiElements.size()>{}),
            std::make_tuple(
                SerializedField{ L"Rules", &ThemeColors::elementRules,
                    &ThemeColors::setElementRules }
            )
        );
    }

    export constexpr auto enumNames(Thickness)
    {
        return std::array{
            L"None",
            L"Hairline",
            L"Thin",
            L"Regular",
            L"Bold",
            L"Heavy",
        };
    }

    // A pair of sizes, across and down.
    export template <std::derived_from<FloatPoint> Pair>
        constexpr auto serializedFields(const Pair&)
    {
        return std::make_tuple(
            SerializedField{ L"X", &Pair::x },
            SerializedField{ L"Y", &Pair::y }
        );
    }

    // The maximum stays the built-in one, which is unbounded in every set a file can name.
    export constexpr auto serializedFields(const ControlMetrics&)
    {
        return std::make_tuple(
            SerializedField{ L"Border", &ControlMetrics::border },
            SerializedField{ L"Band", &ControlMetrics::band },
            SerializedField{ L"Radius", &ControlMetrics::radius },
            SerializedField{ L"Padding", &ControlMetrics::padding },
            SerializedField{ L"Spacing", &ControlMetrics::spacing },
            SerializedField{ L"MinSize", &ControlMetrics::minSize },
            SerializedField{ L"PreferredSize", &ControlMetrics::preferredSize },
            SerializedField{ L"Depth", &ControlMetrics::zDepthFactor },
            SerializedField{ L"GrowIn", &ControlMetrics::surfaceGrowInScale }
        );
    }

    export constexpr auto serializedFields(const WindowShadow&)
    {
        return std::make_tuple(
            SerializedField{ L"Blur", &WindowShadow::blur },
            SerializedField{ L"Offset", &WindowShadow::offset },
            SerializedField{ L"Spread", &WindowShadow::spread },
            SerializedField{ L"Opacity", &WindowShadow::opacity }
        );
    }

    // Each set stands under its token, in k_metricsSets.
    template <std::size_t I>
    constexpr auto metricsSetField()
    {
        return SerializedField{ k_metricsSets[I].token, k_metricsSetMembers[I] };
    }

    template <std::size_t... I>
    constexpr auto metricsSetFields(std::index_sequence<I...>)
    {
        return std::make_tuple(metricsSetField<I>()...);
    }

    export constexpr auto serializedFields(const ThemeMetrics&)
    {
        return std::tuple_cat(
            metricsSetFields(std::make_index_sequence<k_metricsSetCount>{}),
            std::make_tuple(
                SerializedField{ L"PrimaryWindowShadow", &ThemeMetrics::primaryWindowShadow },
                SerializedField{ L"SecondaryWindowShadow", &ThemeMetrics::secondaryWindowShadow }
            )
        );
    }

    export constexpr auto serializedFields(const AppTheme&) {
        return std::make_tuple(
            SerializedField{ L"Metrics", &AppTheme::metrics },
            SerializedField{ L"Colors", &AppTheme::colors }
        );
    }

    export constexpr auto serializedFields(const ThemeSampleColors&) {
        return std::make_tuple(
            SerializedField{ L"RootSurface", &ThemeSampleColors::rootSurface },
            SerializedField{ L"Title", &ThemeSampleColors::title },
            SerializedField{ L"Dialog", &ThemeSampleColors::dialog },
            SerializedField{ L"ToolBar", &ThemeSampleColors::toolBar },
            SerializedField{ L"Page", &ThemeSampleColors::page },
            SerializedField{ L"Section", &ThemeSampleColors::section },
            SerializedField{ L"Border", &ThemeSampleColors::border },
            SerializedField{ L"MutedText", &ThemeSampleColors::mutedText },
            SerializedField{ L"Accent", &ThemeSampleColors::accent },
            SerializedField{ L"Spot", &ThemeSampleColors::spot }
        );
    }

    export constexpr auto serializedFields(const ThemeIconColors&) {
        return std::make_tuple(
            SerializedField{ L"PaletteHues", &ThemeIconColors::paletteHues },
            SerializedField{ L"DarkSample", &ThemeIconColors::darkSample },
            SerializedField{ L"LightSample", &ThemeIconColors::lightSample }
        );
    }

}


//-----------------------------------------------------------------------------


namespace ClaFi::Dom
{
    std::wstring ScalarSerializer<RuleInputs>::toWString(const RuleInputs& inputs)
    {
        std::wstring result{};
        for (std::size_t i = 0ull; i != k_ruleInputKeys.size(); ++i)
        {
            if (!inputs.has(static_cast<RuleInput>(i)))
                continue;
            if (!result.empty())
                result.push_back(L' ');
            result.append(k_ruleInputKeys[i]);
        }
        return result;
    }

    void ScalarSerializer<RuleInputs>::fromWString(std::wstring_view str, RuleInputs& inputs)
    {
        inputs = {};
        for (const auto word : std::views::split(str, L' '))
        {
            const std::wstring_view name{ word.begin(), word.end() };
            for (std::size_t i = 0ull; i != k_ruleInputKeys.size(); ++i)
                if (k_ruleInputKeys[i] == name)
                    inputs.add(static_cast<RuleInput>(i));
        }
    }

    std::wstring ScalarSerializer<OptionalPigment>::toWString(const OptionalPigment& seed)
    {
        if (!seed)
            return {};
        return std::wstring{ k_pigmentKeys[static_cast<std::size_t>(*seed)] };
    }

    void ScalarSerializer<OptionalPigment>::fromWString(std::wstring_view str,
        OptionalPigment& seed)
    {
        seed = std::nullopt;
        for (std::size_t i = 0ull; i != k_pigmentKeys.size(); ++i)
            if (k_pigmentKeys[i] == str)
                seed = static_cast<Pigment>(i);
    }
}
