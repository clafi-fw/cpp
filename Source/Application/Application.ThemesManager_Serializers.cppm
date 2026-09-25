export module ClaFi.Application.ThemesManager_Serializers;

import ClaFi.Application.ThemesManager_Elements;
import ClaFi.App.ThemeIcon;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.Serialization;
import ClaFi.StdLib;
import ClaFi.Core.System.UiTypes;

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
    // and an element names either a ColorRule or a whole ControlColorRules - so the fields cannot
    // be produced by one transform over the table. Each index answers with a tuple of its own and
    // tuple_cat joins them.
    template <std::size_t I>
    constexpr auto elementField()
    {
        if constexpr (k_uiElements[I].rule != nullptr)
            return std::make_tuple(SerializedField{ k_uiElements[I].token, k_uiElements[I].rule });
        else
            return std::make_tuple(SerializedField{ k_uiElements[I].token, k_uiElements[I].rules });
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

    // The palette is stated here and the elements come from k_uiElements, in that table's order,
    // which is ThemeColors declaration order. Field order in a document is a reading matter -
    // a field is found by name - so the table's order is free to be the one generated C++ needs.
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
