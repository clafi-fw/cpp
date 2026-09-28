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

    // memberCommentOf

    std::wstring_view memberCommentOf(UiElement element)
    {
        for (const MemberComment& entry : k_memberComments)
            if (entry.element == element)
                return entry.text;
        return {};
    }

    // isSame

    bool isSame(const ColorRuleValue& first, const ColorRuleValue& second)
    {
        return first.operation() == second.operation()
            and first.normalizedValue() == second.normalizedValue();
    }

    bool isSame(const ColorRuleHue& first, const ColorRuleHue& second)
    {
        return first.operation() == second.operation()
            and first.exactValue() == second.exactValue();
    }

    bool isSame(const ColorEffect& first, const ColorEffect& second)
    {
        return isSame(first.hue, second.hue)
            and isSame(first.saturation, second.saturation)
            and isSame(first.elevation, second.elevation);
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
        if (isSame(value, ColorRuleValue{}))
            return write(L"{}");
        write(L"{ ").colorRuleOp(value.operation());
        write(L", ").number(literalValue(value));
        return write(L" }");
    }

    CppCodeGenerator& CppCodeGenerator::colorRuleHue(const ColorRuleHue& value)
    {
        if (isSame(value, ColorRuleHue{}))
            return write(L"{}");
        write(L"{ ").colorRuleHueOp(value.operation());
        // The exact value is read only by ExactValue, and carrying it under any other operation
        // keeps the generated rule identical to the one on screen.
        if (value.exactValue() != 0.0f)
            write(L", ").number(value.exactValue());
        return write(L" }");
    }

    CppCodeGenerator& CppCodeGenerator::colorEffect(const ColorEffect& value, std::size_t level)
    {
        if (isSame(value, ColorEffect{}))
            return write(L"{}");
        write(L"{").newLine();
        if (!isSame(value.hue, ColorRuleHue{}))
        {
            indent(level + 1ull).colorRuleHue(value.hue).write(L",");
            padTo(k_channelColumn).write(L"// H").newLine();
        }
        indent(level + 1ull).colorRuleValue(value.saturation).write(L",");
        padTo(k_channelColumn).write(L"// S").newLine();
        indent(level + 1ull).colorRuleValue(value.elevation);
        padTo(k_channelColumn).write(L"// E").newLine();
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
        if (m_scope == CodeScope::ClassDeclarations)
            return write(typeName).write(L" ").write(name);
        return memberName(name).write(L" = ").write(typeName);
    }

    CppCodeGenerator& CppCodeGenerator::memberComment(std::wstring_view text, std::size_t level)
    {
        while (!text.empty())
        {
            const std::size_t lineEnd = text.find(L'\n');
            const std::wstring_view line = text.substr(0ull, lineEnd);
            indent(level);
            // A blank line carries the marker alone. A marker and a space would leave trailing
            // whitespace in a block written to be pasted into a source file.
            if (line.empty())
            {
                write(L"//");
            }
            else
            {
                write(L"// ").write(line);
            }
            newLine();
            if (lineEnd == std::wstring_view::npos)
                break;
            text.remove_prefix(lineEnd + 1ull);
        }
        return *this;
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
        // The comment is written after the member is known to be stated, so a block of
        // differences carries prose only for what it does state.
        if (!statesMember(!isSame(m_colors.*entry.effect, m_defaults.*entry.effect)))
            return;
        memberComment(memberCommentOf(element), 0ull);
        openMember(L"ColorEffect", entry.codeName);
        colorEffect(m_colors.*entry.effect, 0ull);
        closeLine();
        newLine();
        m_statedAnyMember = true;
    }

    void CppCodeGenerator::floatMember(std::wstring_view name, float value, float defaultValue,
        std::wstring_view comment)
    {
        if (!statesMember(value != defaultValue))
            return;
        memberComment(comment, 0ull);
        if (m_scope == CodeScope::ClassDeclarations)
        {
            write(L"float ").write(name);
            write(L"{ ").number(value).write(L" }");
        }
        else
        {
            memberName(name).write(L" = ");
            number(value);
        }
        closeLine().newLine();
        m_statedAnyMember = true;
    }

    void CppCodeGenerator::paletteMembers()
    {
        floatMember(L"anchorHue", m_colors.anchorHue, m_defaults.anchorHue, k_anchorHueComment);
        if (statesMember(m_colors.harmonyKind != m_defaults.harmonyKind))
        {
            const std::wstring_view harmonyName =
                enumNames(m_colors.harmonyKind)[static_cast<std::size_t>(m_colors.harmonyKind)];
            // An enum names its value outright, so an assignment has no braces to put it in.
            if (m_scope == CodeScope::ClassDeclarations)
            {
                write(L"ColorHarmonyKind harmonyKind");
                write(L"{ ").enumValue(L"ColorHarmonyKind", harmonyName).write(L" }");
            }
            else
            {
                memberName(L"harmonyKind").write(L" = ");
                enumValue(L"ColorHarmonyKind", harmonyName);
            }
            closeLine().newLine();
            m_statedAnyMember = true;
        }
        paletteHuesMember();
    }

    void CppCodeGenerator::paletteHuesMember()
    {
        bool differs = false;
        for (std::size_t i = 0ull; i != m_colors.paletteHues.size(); ++i)
            if (m_colors.paletteHues[i] != m_defaults.paletteHues[i])
                differs = true;
        if (!statesMember(differs))
            return;
        memberComment(k_paletteHuesComment, 0ull);
        if (m_scope == CodeScope::ClassDeclarations)
        {
            write(L"std::array<float, 3> paletteHues");
        }
        else
        {
            memberName(L"paletteHues").write(L" = ");
        }
        write(L"{").newLine();
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
        case CodeScope::ClassDeclarations:
            write(L"// This code is to be used inside the ThemeColors class").newLine();
            write(L"// to initialize a theme compiled into an application").newLine();
            break;
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
            write(L"// A member left unstated keeps the framework's default").newLine();
        }
        newLine();
    }

    void CppCodeGenerator::build()
    {
        headerComment();
        objectDeclaration();
        paletteMembers();
        floatMember(L"darkModeFloor", m_colors.darkModeFloor,
            m_defaults.darkModeFloor, k_darkModeFloorComment);
        for (std::size_t i = 0ull; i != k_uiElements.size(); ++i)
            colorMember(static_cast<UiElement>(i));
        if (!m_statedAnyMember)
            write(L"// This theme is the framework's default").newLine();
    }

}
