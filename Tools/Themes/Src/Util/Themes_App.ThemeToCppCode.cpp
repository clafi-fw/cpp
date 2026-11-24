module Themes_App.ThemeToCppCode;

import ClaFi.Application.ThemesManager_Serializers;
import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ClaFi;

    namespace
    {
        [[nodiscard]] std::wstring wideLiteral(const std::wstring_view value)
        {
            std::wstring literal{ L"L\"" };
            for (const wchar_t character : value)
            {
                switch (character)
                {
                    case L'\\':
                        literal.append(L"\\\\");
                        break;
                    case L'"':
                        literal.append(L"\\\"");
                        break;
                    case L'\n':
                        literal.append(L"\\n");
                        break;
                    case L'\r':
                        literal.append(L"\\r");
                        break;
                    case L'\t':
                        literal.append(L"\\t");
                        break;
                    default:
                        literal.push_back(character);
                        break;
                }
            }
            literal.push_back(L'"');
            return literal;
        }
    }

    std::wstring themeToModule(const AppTheme& theme, const std::wstring_view name)
    {
        std::wstring moduleName{ L"AppThemes.Theme" };
        bool wordStart = true;
        for (const wchar_t character : name)
        {
            if ((character >= L'a' && character <= L'z')
                || (character >= L'A' && character <= L'Z')
                || (character >= L'0' && character <= L'9'))
            {
                if (wordStart)
                {
                    moduleName.push_back(L'_');
                }
                moduleName.push_back(character);
                wordStart = false;
            }
            else
            {
                wordStart = true;
            }
        }

        std::wstring result{ L"export module " };
        result.append(moduleName).append(L";\n\n"
            L"import ClaFi.Core.AppTheme_Theme;\n"
            L"import ClaFi.Core.AppTheme_Colors;\n"
            L"import ClaFi.Core.AppTheme_Metrics;\n"
            L"import ClaFi.Core.AppTheme_Palette;\n"
            L"import ClaFi.Core.AppTheme_Styles;\n"
            L"import ClaFi.Core.System.UiTypes;\n"
            L"\n"
            L"namespace AppThemes\n"
            L"{\n"
            L"    export [[nodiscard]] ClaFi::AppTheme ");
        result.append(moduleName.substr(std::wstring_view{ L"AppThemes." }.size())).append(L"()\n"
            L"    {\n"
            L"        using namespace ClaFi;\n"
            L"        AppTheme theme{};\n"
            L"\n");
        result.append(CppCodeGenerator{ theme }.text());
        return result.append(L"\n        return theme;\n    }\n}\n");
    }

    // literalValue

    float roundedTo(float value, int digits)
    {
        std::array<char, 64ull> buffer{};
        const auto [last, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
            value, std::chars_format::general, digits);
        if (error != std::errc{})
            return value;
        float result = value;
        std::from_chars(buffer.data(), last, result);
        return result;
    }

    float literalValue(const ColorRuleValue& value)
    {
        static constexpr int k_maxDigits = 9;
        for (int digits = 1; digits != k_maxDigits; ++digits)
        {
            const float rounded = roundedTo(value.value(), digits);
            // The conversion the literal will go through is the one that decides, so the
            // candidate is put through it rather than compared against the value.
            if (ColorRuleValue{ value.operation(), rounded }.normalizedValue()
                == value.normalizedValue())
                return rounded;
        }
        return value.value();
    }

    // CppCodeGenerator

    CppCodeGenerator::CppCodeGenerator(const AppTheme& theme)
        :
        m_theme{ theme }
    {
        build();
    }

    CppCodeGenerator& CppCodeGenerator::write(std::wstring_view value)
    {
        m_text += value;
        m_column += value.size();
        return *this;
    }

    CppCodeGenerator& CppCodeGenerator::newLine()
    {
        m_text += L'\n';
        m_column = 0ull;
        return *this;
    }

    CppCodeGenerator& CppCodeGenerator::indent(std::size_t level)
    {
        return write(k_spaces.substr(0ull, level * k_indentWidth));
    }

    CppCodeGenerator& CppCodeGenerator::padTo(std::size_t column)
    {
        if (m_column >= column)
            return write(L" ");
        return write(k_spaces.substr(0ull, column - m_column));
    }

    CppCodeGenerator& CppCodeGenerator::enumValue(std::wstring_view enumName, std::wstring_view value)
    {
        return write(enumName).write(L"::").write(value);
    }

    CppCodeGenerator& CppCodeGenerator::number(float value)
    {
        std::wstring text = floatToStr(value);
        // A float literal needs a decimal point or an exponent before its suffix, and the
        // shortest round trip form of a whole number carries neither.
        if (text.find(L'.') == std::wstring::npos and text.find(L'e') == std::wstring::npos)
            text += L".0";
        text += L'f';
        return write(text);
    }

    CppCodeGenerator& CppCodeGenerator::closeLine()
    {
        return write(L";").newLine();
    }

    CppCodeGenerator& CppCodeGenerator::colorRuleOp(ColorRuleOp value)
    {
        return enumValue(L"ColorRuleOp", enumNames(value)[static_cast<std::size_t>(value)]);
    }

    CppCodeGenerator& CppCodeGenerator::colorRuleHueOp(ColorRuleHueOp value)
    {
        return enumValue(L"ColorRuleHueOp", enumNames(value)[static_cast<std::size_t>(value)]);
    }

    CppCodeGenerator& CppCodeGenerator::colorRuleValue(const ColorRuleValue& value)
    {
        if (value == ColorRuleValue{})
            return write(L"{}");
        write(L"{ ").colorRuleOp(value.operation());
        write(L", ").number(literalValue(value));
        return write(L" }");
    }

    CppCodeGenerator& CppCodeGenerator::colorRuleHue(const ColorRuleHue& value)
    {
        if (value == ColorRuleHue{})
            return write(L"{}");
        write(L"{ ").colorRuleHueOp(value.operation());
        // The exact value is read only by ExactValue, and carrying it under any other operation
        // keeps the generated rule identical to the one on screen.
        if (value.exactValue() != 0.0f)
            write(L", ").number(value.exactValue());
        return write(L" }");
    }

    CppCodeGenerator& CppCodeGenerator::colorEffect(const ColorEffect& value, std::size_t level,
        std::size_t markerColumn)
    {
        if (value == ColorEffect{})
            return write(L"{}");
        write(L"{").newLine();
        if (value.hue != ColorRuleHue{})
        {
            indent(level + 1ull).colorRuleHue(value.hue).write(L",");
            padTo(markerColumn).write(L"// H").newLine();
        }
        indent(level + 1ull).colorRuleValue(value.saturation).write(L",");
        padTo(markerColumn).write(L"// S").newLine();
        indent(level + 1ull).colorRuleValue(value.elevation);
        padTo(markerColumn).write(L"// E").newLine();
        return indent(level).write(L"}");
    }

    CppCodeGenerator& CppCodeGenerator::ruleInputs(std::wstring_view designator,
        const RuleInputs& inputs)
    {
        write(L".").write(designator).write(L"{ ");
        bool first = true;
        for (std::size_t i = 0ull; i != k_ruleInputKeys.size(); ++i)
        {
            if (!inputs.has(static_cast<RuleInput>(i)))
                continue;
            if (!first)
                write(L", ");
            enumValue(L"RuleInput", k_ruleInputKeys[i]);
            first = false;
        }
        return write(L" }");
    }

    CppCodeGenerator& CppCodeGenerator::fieldEnd(std::size_t fieldsLeft)
    {
        if (fieldsLeft != 0ull)
            write(L",");
        return newLine();
    }

    CppCodeGenerator& CppCodeGenerator::colorRule(const ColorRule& rule, std::size_t level,
        std::size_t markerColumn)
    {
        const bool statesInputs = !rule.inputs.empty();
        const bool statesAndInputs = !rule.andInputs.empty();
        const bool statesOutput = rule.output != ColorRule{}.output;
        const bool statesEffect = rule.effect != ColorEffect{};
        std::size_t fieldsLeft = static_cast<std::size_t>(std::ranges::count(
            std::array{ statesInputs, statesAndInputs, statesOutput, statesEffect }, true));
        write(L"ColorRule{");
        if (fieldsLeft == 0ull)
            return write(L"}");
        newLine();
        if (statesInputs)
        {
            --fieldsLeft;
            indent(level + 1ull).ruleInputs(L"inputs", rule.inputs).fieldEnd(fieldsLeft);
        }
        if (statesAndInputs)
        {
            --fieldsLeft;
            indent(level + 1ull).ruleInputs(L"andInputs", rule.andInputs).fieldEnd(fieldsLeft);
        }
        if (statesOutput)
        {
            --fieldsLeft;
            const std::size_t output = static_cast<std::size_t>(rule.output);
            indent(level + 1ull).write(L".output = ");
            enumValue(L"PaintChannel", enumNames(rule.output)[output]).fieldEnd(fieldsLeft);
        }
        if (statesEffect)
        {
            --fieldsLeft;
            indent(level + 1ull).write(L".effect");
            colorEffect(rule.effect, level + 1ull, markerColumn).fieldEnd(fieldsLeft);
        }
        return indent(level).write(L"}");
    }

    CppCodeGenerator& CppCodeGenerator::colorRules(const ColorRules& rules, std::size_t level,
        std::size_t markerColumn)
    {
        if (rules.empty())
            return write(L"{}");
        write(L"{").newLine();
        for (std::size_t i = 0ull; i != rules.size(); ++i)
        {
            indent(level + 1ull).colorRule(rules[i], level + 1ull, markerColumn);
            if (i + 1ull != rules.size())
                write(L",");
            newLine();
        }
        return indent(level).write(L"}");
    }

    CppCodeGenerator& CppCodeGenerator::colorsMemberName(std::wstring_view name)
    {
        return write(L"theme.colors.").write(name);
    }

    void CppCodeGenerator::colorMember(const UiElement element)
    {
        const UiElementDescriptor& entry = uiElementOf(element);
        if (!entry.effect)
        {
            return;
        }
        colorsMemberName(entry.codeName).write(L" = ColorEffect");
        colorEffect(m_theme.colors.*entry.effect, 0ull, k_channelColumn);
        closeLine().newLine().newLine();
    }

    void CppCodeGenerator::floatMember(const std::wstring_view name, const float value)
    {
        colorsMemberName(name).write(L" = ").number(value);
        closeLine().newLine().newLine();
    }

    void CppCodeGenerator::paletteMembers()
    {
        const ThemeColors& colors = m_theme.colors;
        floatMember(L"anchorHue", colors.anchorHue);
        const std::wstring_view harmonyName =
            enumNames(colors.harmonyKind)[static_cast<std::size_t>(colors.harmonyKind)];
        colorsMemberName(L"harmonyKind").write(L" = ");
        enumValue(L"ColorHarmonyKind", harmonyName);
        closeLine().newLine().newLine();
        paletteHuesMember();
    }

    void CppCodeGenerator::paletteHuesMember()
    {
        const ThemeColors& colors = m_theme.colors;
        colorsMemberName(L"paletteHues").write(L" = {").newLine();
        for (std::size_t i = 0ull; i != colors.paletteHues.size(); ++i)
        {
            indent(1ull).number(colors.paletteHues[i]);
            if (i + 1ull != colors.paletteHues.size())
            {
                write(L",");
            }
            newLine();
        }
        write(L"}").closeLine().newLine().newLine();
    }

    CppCodeGenerator& CppCodeGenerator::ruleSelector(const RuleSelector& selector)
    {
        write(L"RuleSelector{ ");
        if (!selector.inputs.empty())
            ruleInputs(L"inputs", selector.inputs).write(L", ");
        if (!selector.andInputs.empty())
            ruleInputs(L"andInputs", selector.andInputs).write(L", ");
        const std::size_t output = static_cast<std::size_t>(selector.output);
        write(L".output = ");
        enumValue(L"PaintChannel", enumNames(selector.output)[output]);
        return write(L" }");
    }

    CppCodeGenerator& CppCodeGenerator::ruleSelectors(const RuleSelectors& selectors,
        std::size_t level)
    {
        write(L"{").newLine();
        for (std::size_t i = 0ull; i != selectors.size(); ++i)
        {
            indent(level + 1ull).ruleSelector(selectors[i]);
            if (i + 1ull != selectors.size())
                write(L",");
            newLine();
        }
        return indent(level).write(L"}");
    }

    CppCodeGenerator& CppCodeGenerator::statedSize(const std::wstring_view designator,
        const std::wstring_view typeName, const FloatPoint& value)
    {
        write(L".").write(designator).write(L" = ").write(typeName).write(L"{ ");
        number(value.x).write(L", ").number(value.y);
        return write(L" }");
    }

    CppCodeGenerator& CppCodeGenerator::statedSize(const std::wstring_view designator,
        const float value)
    {
        return write(L".").write(designator).write(L" = ").number(value);
    }

    // The designators stand in StatedMetrics declaration order, which is what a designated
    // initializer list has to keep.
    CppCodeGenerator& CppCodeGenerator::statedMetrics(const StatedMetrics& metrics,
        const std::size_t level)
    {
        write(L"{");
        std::size_t stated = 0ull;
        auto openSize = [this, level, &stated]() {
            if (stated != 0ull)
                write(L",");
            ++stated;
            newLine().indent(level + 1ull);
        };
        if (metrics.border)
        {
            openSize();
            const std::size_t border = static_cast<std::size_t>(*metrics.border);
            write(L".border = ").enumValue(L"Thickness", enumNames(Thickness{})[border]);
        }
        if (metrics.band)
        {
            openSize();
            statedSize(L"band", *metrics.band);
        }
        if (metrics.radius)
        {
            openSize();
            statedSize(L"radius", *metrics.radius);
        }
        if (metrics.padding)
        {
            openSize();
            statedSize(L"padding", L"Padding", *metrics.padding);
        }
        if (metrics.spacing)
        {
            openSize();
            statedSize(L"spacing", L"Spacing", *metrics.spacing);
        }
        if (metrics.minSize)
        {
            openSize();
            statedSize(L"minSize", L"MinSize", *metrics.minSize);
        }
        if (metrics.preferredSize)
        {
            openSize();
            statedSize(L"preferredSize", L"PreferredSize", *metrics.preferredSize);
        }
        if (metrics.depth)
        {
            openSize();
            statedSize(L"depth", *metrics.depth);
        }
        if (metrics.growIn)
        {
            openSize();
            statedSize(L"growIn", *metrics.growIn);
        }
        if (metrics.shadowBlur)
        {
            openSize();
            statedSize(L"shadowBlur", *metrics.shadowBlur);
        }
        if (metrics.shadowOffset)
        {
            openSize();
            statedSize(L"shadowOffset", L"FloatPoint", *metrics.shadowOffset);
        }
        if (metrics.shadowSpread)
        {
            openSize();
            statedSize(L"shadowSpread", *metrics.shadowSpread);
        }
        if (metrics.shadowOpacity)
        {
            openSize();
            statedSize(L"shadowOpacity", *metrics.shadowOpacity);
        }
        newLine();
        return indent(level).write(L"}");
    }

    CppCodeGenerator& CppCodeGenerator::controlStyle(const ControlStyle& style,
        std::size_t level)
    {
        const bool statesBase = !style.base.empty();
        const bool statesStandsOn = !style.standsOn.empty();
        const bool statesSeed = style.seed.has_value();
        const bool statesRules = !style.rules.empty();
        const bool statesDropped = !style.dropped.empty();
        const bool statesMetrics = !style.metrics.empty();
        std::size_t fieldsLeft = 1ull + static_cast<std::size_t>(std::ranges::count(
            std::array{ statesBase, statesStandsOn, statesSeed, statesRules, statesDropped,
                statesMetrics },
            true));
        write(L"ControlStyle{").newLine();
        --fieldsLeft;
        indent(level + 1ull).write(L".name = ").write(wideLiteral(style.name));
        fieldEnd(fieldsLeft);
        if (statesBase)
        {
            --fieldsLeft;
            indent(level + 1ull).write(L".base = ").write(wideLiteral(style.base));
            fieldEnd(fieldsLeft);
        }
        if (statesStandsOn)
        {
            --fieldsLeft;
            indent(level + 1ull).write(L".standsOn = ").write(wideLiteral(style.standsOn));
            fieldEnd(fieldsLeft);
        }
        if (statesSeed)
        {
            --fieldsLeft;
            const std::size_t seed = static_cast<std::size_t>(*style.seed);
            indent(level + 1ull).write(L".seed = ").enumValue(L"Pigment", k_pigmentKeys[seed]);
            fieldEnd(fieldsLeft);
        }
        if (statesRules)
        {
            --fieldsLeft;
            indent(level + 1ull).write(L".rules = ");
            colorRules(style.rules, level + 1ull, ruleMarkerColumn(level + 2ull));
            fieldEnd(fieldsLeft);
        }
        if (statesDropped)
        {
            --fieldsLeft;
            indent(level + 1ull).write(L".dropped = ");
            ruleSelectors(style.dropped, level + 1ull);
            fieldEnd(fieldsLeft);
        }
        if (statesMetrics)
        {
            --fieldsLeft;
            indent(level + 1ull).write(L".metrics = ");
            statedMetrics(style.metrics, level + 1ull);
            fieldEnd(fieldsLeft);
        }
        return indent(level).write(L"}");
    }

    void CppCodeGenerator::themeStyles()
    {
        const ControlStyleList& list = m_theme.styles.list;
        if (list.empty())
        {
            write(L"theme.styles.list = {}").closeLine();
            return;
        }
        write(L"theme.styles.list = {").newLine();
        for (std::size_t i = 0ull; i != list.size(); ++i)
        {
            indent(1ull).controlStyle(list[i], 1ull);
            if (i + 1ull != list.size())
                write(L",");
            newLine();
        }
        write(L"}").closeLine();
    }

    void CppCodeGenerator::build()
    {
        paletteMembers();
        floatMember(L"darkModeFloor", m_theme.colors.darkModeFloor);
        for (std::size_t i = 0ull; i != k_uiElements.size(); ++i)
        {
            colorMember(static_cast<UiElement>(i));
        }
        themeStyles();
        constexpr std::wstring_view indent{ L"        " };
        std::wstring indented;
        for (const auto line : m_text | std::views::split(L'\n'))
        {
            if (!line.empty())
            {
                indented.append(indent);
                indented.append(line.begin(), line.end());
            }
            indented.push_back(L'\n');
        }
        m_text = std::move(indented);
    }

}
