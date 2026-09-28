module ThisApp.ThemeToCppCode;

import ClaFi.Application.ThemesManager_Serializers;
import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ClaFi;

    // themeToCppCode

    std::wstring themeToCppCode(const ThemeColors& colors, CodeContent content, CodeScope scope)
    {
        return CppCodeGenerator{ colors, content, scope }.text();
    }

    // themeToSourceFile

    std::wstring themeToSourceFile(const ThemeColors& colors)
    {
        const std::wstring text = CppCodeGenerator{ colors }.text();
        std::wstring_view statements = text;
        std::wstring result{ k_sourceFileHead };
        while (!statements.empty())
        {
            const std::size_t lineEnd = statements.find(L'\n');
            const std::wstring_view line = statements.substr(0ull, lineEnd);
            // A blank line stays empty rather than carrying the indent as trailing whitespace.
            if (!line.empty())
                result.append(k_sourceFileIndent, L' ').append(line);
            result.push_back(L'\n');
            if (lineEnd == std::wstring_view::npos)
                break;
            statements.remove_prefix(lineEnd + 1ull);
        }
        // The last statement's blank line would stand above the closing brace.
        while (result.ends_with(L"\n\n"))
            result.pop_back();
        return result.append(k_sourceFileTail);
    }

    // builtInColorsFile

    std::filesystem::path builtInColorsFile()
    {
        // This file stands in the tree it was compiled from, wherever the executable was put.
        std::error_code error;
        std::filesystem::path folder =
            std::filesystem::path{ std::source_location::current().file_name() }.parent_path();
        while (!folder.empty())
        {
            const std::filesystem::path appThemeFolder =
                folder / L"Source" / L"Y-Core" / L"AppTheme";
            if (std::filesystem::exists(appThemeFolder / k_colorsModuleFileName, error))
                return appThemeFolder / k_builtInColorsFileName;
            std::filesystem::path parent = folder.parent_path();
            // A root is its own parent.
            if (parent == folder)
                break;
            folder = std::move(parent);
        }
        return {};
    }

    // writeBuiltInColors

    SourceWrite writeBuiltInColors(const std::filesystem::path& file, const ThemeColors& colors)
    {
        const std::string_view lineEnd = lineEndOf(file.parent_path() / k_colorsModuleFileName);
        std::string content{};
        for (const char c : toUtf8(themeToSourceFile(colors)))
        {
            if (c == '\n')
            {
                content.append(lineEnd);
            }
            else
            {
                content.push_back(c);
            }
        }
        // An untouched file is not rebuilt.
        if (fileBytes(file) == content)
            return SourceWrite::Unchanged;
        std::ofstream stream{ file, std::ios::binary | std::ios::trunc };
        stream.write(content.data(), static_cast<std::streamsize>(content.size()));
        stream.close();
        return stream ? SourceWrite::Written : SourceWrite::Failed;
    }

    // lineEndOf

    std::string_view lineEndOf(const std::filesystem::path& file)
    {
        std::ifstream stream{ file, std::ios::binary };
        std::string line{};
        std::getline(stream, line);
        if (line.ends_with('\r'))
            return "\r\n";
        return "\n";
    }

    // fileBytes

    std::string fileBytes(const std::filesystem::path& file)
    {
        std::ifstream stream{ file, std::ios::binary };
        return { std::istreambuf_iterator<char>{ stream }, std::istreambuf_iterator<char>{} };
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

    CppCodeGenerator::CppCodeGenerator(const ThemeColors& colors, CodeContent content,
        CodeScope scope)
        :
        m_colors{ colors },
        m_content{ content },
        m_scope{ scope }
    {
        build();
    }

    CppCodeGenerator::CppCodeGenerator(const ThemeColors& colors)
        :
        m_colors{ colors },
        m_content{ CodeContent::Full },
        m_scope{ CodeScope::ClassMethod },
        m_startsEmpty{ true }
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

    CppCodeGenerator& CppCodeGenerator::memberName(std::wstring_view name)
    {
        if (m_scope == CodeScope::OutsideClass)
            write(k_objectName).write(L".");
        return write(name);
    }

    CppCodeGenerator& CppCodeGenerator::openMember(std::wstring_view typeName, std::wstring_view name)
    {
        return memberName(name).write(L" = ").write(typeName);
    }

    bool CppCodeGenerator::statesMember(bool differs) const
    {
        return m_content == CodeContent::Full or differs;
    }

    void CppCodeGenerator::colorMember(UiElement element)
    {
        const UiElementDescriptor& entry = uiElementOf(element);
        // An element whose effect no ink names has no member of ThemeColors to state.
        if (!entry.effect)
            return;
        if (!statesMember(m_colors.*entry.effect != m_defaults.*entry.effect))
            return;
        openMember(L"ColorEffect", entry.codeName);
        colorEffect(m_colors.*entry.effect, 0ull, k_channelColumn);
        closeLine();
        newLine();
        m_statedAnyMember = true;
    }

    void CppCodeGenerator::floatMember(std::wstring_view name, float value, float defaultValue)
    {
        if (!statesMember(value != defaultValue))
            return;
        memberName(name).write(L" = ").number(value);
        closeLine().newLine();
        m_statedAnyMember = true;
    }

    void CppCodeGenerator::paletteMembers()
    {
        floatMember(L"anchorHue", m_colors.anchorHue, m_defaults.anchorHue);
        if (statesMember(m_colors.harmonyKind != m_defaults.harmonyKind))
        {
            const std::wstring_view harmonyName =
                enumNames(m_colors.harmonyKind)[static_cast<std::size_t>(m_colors.harmonyKind)];
            memberName(L"harmonyKind").write(L" = ").enumValue(L"ColorHarmonyKind", harmonyName);
            closeLine().newLine();
            m_statedAnyMember = true;
        }
        paletteHuesMember();
    }

    void CppCodeGenerator::paletteHuesMember()
    {
        if (!statesMember(m_colors.paletteHues != m_defaults.paletteHues))
            return;
        memberName(L"paletteHues").write(L" = {").newLine();
        for (std::size_t i = 0ull; i != m_colors.paletteHues.size(); ++i)
        {
            indent(1ull).number(m_colors.paletteHues[i]);
            if (i + 1ull != m_colors.paletteHues.size())
                write(L",");
            newLine();
        }
        write(L"}").closeLine().newLine();
        m_statedAnyMember = true;
    }

    bool CppCodeGenerator::statesList(const ColorRules& value,
        const ColorRules& defaultValue) const
    {
        if (m_content == CodeContent::Full)
            return !value.empty();
        return value != defaultValue;
    }

    void CppCodeGenerator::rulesList(std::wstring_view target, const ColorRules& value,
        const ColorRules& defaultValue)
    {
        if (!statesList(value, defaultValue))
            return;
        // Statements stand a blank line apart. The first one follows what opened the rules.
        if (m_statedAnyList)
            newLine();
        memberName(L"rules").write(L".").write(target).write(L" = ");
        colorRules(value, 0ull, k_ruleChannelColumn);
        closeLine();
        m_statedAnyList = true;
        m_statedAnyMember = true;
    }

    void CppCodeGenerator::themeRules()
    {
        // An object built with the framework's rules is brought to none before Full states its own.
        if (m_content == CodeContent::Full and !m_startsEmpty)
        {
            memberName(L"rules").write(L" = {}").closeLine();
            m_statedAnyMember = true;
        }
        for (const NamedRules& entry : k_namedRules)
            rulesList(entry.codeName, m_colors.rules.*entry.rules, m_defaults.rules.*entry.rules);
        for (std::size_t i = 0ull; i != k_uiElementCount; ++i)
        {
            // An element's token is spelled as its enumerator, as every name a theme file uses is.
            const std::wstring target = std::format(L"of(UiElement::{})", k_uiElements[i].token);
            rulesList(target, m_colors.rules.elements[i], m_defaults.rules.elements[i]);
        }
    }

    void CppCodeGenerator::objectDeclaration()
    {
        if (m_scope != CodeScope::OutsideClass)
            return;
        write(L"ThemeColors ").write(k_objectName).write(L"{}");
        closeLine().newLine();
    }

    void CppCodeGenerator::headerComment()
    {
        switch (m_scope)
        {
        case CodeScope::ClassMethod:
            write(L"// This code is to be used inside a ThemeColors method").newLine();
            write(L"// to initialize a theme compiled into an application").newLine();
            break;
        case CodeScope::OutsideClass:
            write(L"// This code builds a theme compiled into an application").newLine();
            break;
        case CodeScope::Count:
            break;
        }
        if (m_content == CodeContent::Differences)
        {
            write(L"// Anything left unstated keeps the framework's default").newLine();
        }
        newLine();
    }

    void CppCodeGenerator::build()
    {
        if (!m_startsEmpty)
            headerComment();
        objectDeclaration();
        paletteMembers();
        floatMember(L"darkModeFloor", m_colors.darkModeFloor, m_defaults.darkModeFloor);
        for (std::size_t i = 0ull; i != k_uiElements.size(); ++i)
            colorMember(static_cast<UiElement>(i));
        themeRules();
        if (!m_statedAnyMember)
            write(L"// This theme is the framework's default").newLine();
    }

}
