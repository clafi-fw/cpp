export module ClaFi.Application.ThemesManager_Serializers;

import ClaFi.Application.ThemesManager_Elements;
import ClaFi.App.ThemeIcon;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Palette;
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

    export constexpr auto serializedFields(const ColorRule&) {
        return std::make_tuple(
            SerializedField{ L"Hue", &ColorRule::hue },
            SerializedField{ L"Saturation", &ColorRule::saturation },
            SerializedField{ L"Elevation", &ColorRule::elevation }
        );
    }

    template <std::size_t I>
    constexpr auto stateField()
    {
        return SerializedField{ k_uiElementStates[I].token, k_uiElementStates[I].rule };
    }

    template <std::size_t... I>
    constexpr auto stateFields(std::index_sequence<I...>)
    {
        return std::make_tuple(stateField<I>()...);
    }

    // ONE FIELD AT A TIME, BECAUSE THE LIST IS HETEROGENEOUS. A tuple carries a type per element,
    // and an element names a ColorRule, a whole ControlColorRules or neither - so the fields cannot
    // be produced by one transform over the table. Each index answers with a tuple of its own and
    // tuple_cat joins them.
    template <std::size_t I>
    constexpr auto elementField()
    {
        if constexpr (k_uiElements[I].rule != nullptr)
            return std::make_tuple(SerializedField{ k_uiElements[I].token, k_uiElements[I].rule });
        else if constexpr (k_uiElements[I].rules != nullptr)
            return std::make_tuple(SerializedField{ k_uiElements[I].token, k_uiElements[I].rules });
        else
            return std::tuple<>{};
    }

    template <std::size_t... I>
    constexpr auto elementFields(std::index_sequence<I...>)
    {
        return std::tuple_cat(elementField<I>()...);
    }

    // Flip is named here rather than through the state table, which lists rules.
    export constexpr auto serializedFields(const ControlColorRules&) {
        return std::tuple_cat(
            std::make_tuple(
                SerializedField{ L"Flip", &ControlColorRules::flip }
            ),
            stateFields(std::make_index_sequence<k_uiElementStates.size()>{})
        );
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
            L"Hovered",
            L"Pressed",
            L"Focused",
            L"Selected",
            L"Disabled",
            L"TextHovered",
            L"Current",
            L"WindowFocused"
        };

    export constexpr auto enumNames(RuleInput) { return k_ruleInputKeys; }

    export constexpr auto enumNames(PaintChannel)
    {
        return std::array{
            L"Surface",
            L"Stroke",
            L"Text",
            L"Shadow",
        };
    }

    export constexpr auto serializedFields(const ColorRule2&) {
        return std::make_tuple(
            SerializedField{ L"Inputs", &ColorRule2::inputs },
            SerializedField{ L"Output", &ColorRule2::output },
            SerializedField{ L"Effect", &ColorRule2::effect }
        );
    }

    // Each element's list stands under the element's token, the name its old rules stand under.
    template <std::size_t I>
    constexpr auto elementRulesField()
    {
        return SerializedField{ k_uiElements[I].token, &ThemeRules2::element<I>,
            &ThemeRules2::setElement<I> };
    }

    template <std::size_t... I>
    constexpr auto elementRulesFields(std::index_sequence<I...>)
    {
        return std::make_tuple(elementRulesField<I>()...);
    }

    export constexpr auto serializedFields(const ThemeRules2&) {
        return std::tuple_cat(
            std::make_tuple(
                SerializedField{ L"Shared", &ThemeRules2::shared }
            ),
            elementRulesFields(std::make_index_sequence<k_uiElementCount>{})
        );
    }

    // The palette and the new rules are stated here and the elements come from k_uiElements, in
    // that table's order, which is ThemeColors declaration order. Field order in a document is a
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
                SerializedField{ L"Rules2", &ThemeColors::rules2 }
            )
        );
    }

    export constexpr auto serializedFields(const AppTheme&) {
        return std::make_tuple(
            SerializedField{ L"Colors", &AppTheme::colors }
        );
    }

    export constexpr auto serializedFields(const ThemeIconColors&) {
        return std::make_tuple(
            SerializedField{ L"PaletteHues", &ThemeIconColors::paletteHues },
            SerializedField{ L"DarkSurface", &ThemeIconColors::darkSurface },
            SerializedField{ L"LightSurface", &ThemeIconColors::lightSurface }
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
}
