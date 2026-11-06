export module ClaFi.Application.ThemesManager_Serializers;

export import ClaFi.Core.Dom_UiSerializers;

import ClaFi.Application.ThemesManager_Elements;
import ClaFi.App.ThemeIcon;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Dom_StdSerializers;
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

    // A seed is its pigment's name, and nothing where a style states none. A name no pigment goes
    // by reads as none.
    template <>
    struct ScalarSerializer<OptionalPigment>
    {
        [[nodiscard]] static std::wstring toWString(const OptionalPigment& seed);
        static void fromWString(std::wstring_view str, OptionalPigment& seed);
    };

    // A pair of sizes is two numbers a space apart, across then down, and nothing where a style
    // states none. Anything else reads as none.
    template <std::derived_from<FloatPoint> Pair>
    struct ScalarSerializer<std::optional<Pair>>
    {
        [[nodiscard]] static std::wstring toWString(const std::optional<Pair>& pair);
        static void fromWString(std::wstring_view str, std::optional<Pair>& pair);
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
        L"Red",
        L"Purple",
        L"Orange"
    };

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

    // A style writes the sizes it states and nothing else. See Application#controlstyle-in-a-file
    export constexpr auto serializedFields(const StatedMetrics&)
    {
        return std::make_tuple(
            SerializedField{ L"Border", &StatedMetrics::border },
            SerializedField{ L"Band", &StatedMetrics::band },
            SerializedField{ L"Radius", &StatedMetrics::radius },
            SerializedField{ L"Padding", &StatedMetrics::padding },
            SerializedField{ L"Spacing", &StatedMetrics::spacing },
            SerializedField{ L"MinSize", &StatedMetrics::minSize },
            SerializedField{ L"PreferredSize", &StatedMetrics::preferredSize },
            SerializedField{ L"Depth", &StatedMetrics::depth },
            SerializedField{ L"GrowIn", &StatedMetrics::growIn },
            SerializedField{ L"ShadowBlur", &StatedMetrics::shadowBlur },
            SerializedField{ L"ShadowOffset", &StatedMetrics::shadowOffset },
            SerializedField{ L"ShadowSpread", &StatedMetrics::shadowSpread },
            SerializedField{ L"ShadowOpacity", &StatedMetrics::shadowOpacity }
        );
    }

    // A style stands whole in a file, its name first. See Application#controlstyle-in-a-file
    export constexpr auto serializedFields(const ControlStyle&) {
        return std::make_tuple(
            SerializedField{ L"Name", &ControlStyle::name },
            SerializedField{ L"Base", &ControlStyle::base },
            SerializedField{ L"StandsOn", &ControlStyle::standsOn },
            SerializedField{ L"Seed", &ControlStyle::seed },
            SerializedField{ L"Rules", &ControlStyle::rules },
            SerializedField{ L"Dropped", &ControlStyle::dropped },
            SerializedField{ L"Metrics", &ControlStyle::metrics }
        );
    }

    // The palette is stated here and the effects come from k_uiElements, in that table's order,
    // which is ThemeColors declaration order. Field order in a document is a reading matter - a
    // field is found by name - so the table's order is free to be the one generated C++ needs.
    export constexpr auto serializedFields(const ThemeColors&) {
        return std::tuple_cat(
            std::make_tuple(
                SerializedField{ L"Harmony", &ThemeColors::harmonyKind },
                SerializedField{ L"AnchorHue", &ThemeColors::anchorHue },
                SerializedField{ L"PaletteHues", &ThemeColors::paletteHues },
                SerializedField{ L"DarkModeFloor", &ThemeColors::darkModeFloor }
            ),
            elementFields(std::make_index_sequence<k_uiElements.size()>{})
        );
    }

    export constexpr auto serializedFields(const AppTheme&) {
        return std::make_tuple(
            SerializedField{ L"Colors", &AppTheme::colors },
            SerializedField{ L"Styles", &AppTheme::styleList, &AppTheme::setStyleList }
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

    template <std::derived_from<FloatPoint> Pair>
    std::wstring ScalarSerializer<std::optional<Pair>>::toWString(const std::optional<Pair>& pair)
    {
        if (!pair)
            return {};
        return ScalarSerializer<float>::toWString(pair->x) + L' '
            + ScalarSerializer<float>::toWString(pair->y);
    }

    template <std::derived_from<FloatPoint> Pair>
    void ScalarSerializer<std::optional<Pair>>::fromWString(std::wstring_view str,
        std::optional<Pair>& pair)
    {
        pair = std::nullopt;
        const std::size_t space = str.find(L' ');
        if (space == std::wstring_view::npos)
            return;
        Pair value{ 0.0f, 0.0f };
        ScalarSerializer<float>::fromWString(str.substr(0ull, space), value.x);
        ScalarSerializer<float>::fromWString(str.substr(space + 1ull), value.y);
        pair = value;
    }
}
