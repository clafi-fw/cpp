export module Themes_App.ThemeToCppCode;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ClaFi;

    export enum class CodeContent
    {
        Differences,
        Full
    };

    export constexpr auto enumNames(CodeContent)
    {
        return std::array<std::wstring_view, 2>{ L"Differences", L"Full" };
    }

    export [[nodiscard]] std::wstring themeToModule(const AppTheme&, std::wstring_view name);

    // The value a literal has to carry to rebuild a rule value. A rule holds its value
    // normalized, and an Offset normalizes by half a unit, which is enough to put the shortest
    // form of the value that built it out of reach: 0.1 comes back as 0.100000024. So the
    // shortest form that does rebuild the same normalized value is the one searched for.
    [[nodiscard]] float literalValue(const ColorRuleValue&);
    // value carrying the given count of significant digits, or value itself when it cannot be
    // written that way.
    [[nodiscard]] float roundedTo(float value, int digits);

    class CppCodeGenerator
    {
    public:
        explicit CppCodeGenerator(const AppTheme&);
    public:
        [[nodiscard]] std::wstring text() && { return std::move(m_text); }
    private:
        // build helpers
        CppCodeGenerator& write(std::wstring_view);
        CppCodeGenerator& newLine();
        CppCodeGenerator& indent(std::size_t level);
        // Spaces up to a column, or one space when the line already reaches past it.
        CppCodeGenerator& padTo(std::size_t column);
        CppCodeGenerator& enumValue(std::wstring_view enumName, std::wstring_view value);
        CppCodeGenerator& number(float);
        CppCodeGenerator& closeLine();
        //
        // ColorTheme specific helpers
        CppCodeGenerator& colorRuleOp(ColorRuleOp);
        CppCodeGenerator& colorRuleHueOp(ColorRuleHueOp);
        // The braces of one value, on the line it starts on.
        CppCodeGenerator& colorRuleValue(const ColorRuleValue&);
        CppCodeGenerator& colorRuleHue(const ColorRuleHue&);
        // The braces of one effect, opening to closing, over as many lines as it has channels.
        // level is the indent its closing brace sits at. Saturation and elevation are positional
        // arguments, so both are stated whenever the effect states anything; hue is stated only
        // when it moves.
        CppCodeGenerator& colorEffect(const ColorEffect&, std::size_t level,
            std::size_t markerColumn);
        // One designator of a set of inputs, the inputs named in RuleInput order.
        CppCodeGenerator& ruleInputs(std::wstring_view designator, const RuleInputs&);
        // Ends a designator's line, with a comma while more follow.
        CppCodeGenerator& fieldEnd(std::size_t fieldsLeft);
        // The braces of one rule, stating only what differs from a plain ColorRule{}.
        CppCodeGenerator& colorRule(const ColorRule&, std::size_t level, std::size_t markerColumn);
        CppCodeGenerator& colorRules(const ColorRules&, std::size_t level,
            std::size_t markerColumn);
        // The braces of one dropped selector, on the line it starts on.
        CppCodeGenerator& ruleSelector(const RuleSelector&);
        CppCodeGenerator& ruleSelectors(const RuleSelectors&, std::size_t level);
        // One stated size as a designator, with the type a pair is spelled in.
        CppCodeGenerator& statedSize(std::wstring_view designator, std::wstring_view typeName,
            const FloatPoint&);
        CppCodeGenerator& statedSize(std::wstring_view designator, float);
        // The braces of the sizes a style states, one designator per stated size.
        CppCodeGenerator& statedMetrics(const StatedMetrics&, std::size_t level);
        // The braces of one style, stating only what it has. level is the indent its closing
        // brace sits at, and its rules' channel markers stand further right the deeper it sits.
        CppCodeGenerator& controlStyle(const ControlStyle&, std::size_t level);
        //
        CppCodeGenerator& colorsMemberName(std::wstring_view name);
        void colorMember(UiElement);
        void floatMember(std::wstring_view name, float value);
        void paletteMembers();
        void paletteHuesMember();
        void themeStyles();
        void build();
    private:
        // The column a channel marker starts at. The block is shown in a monospaced style, so
        // one column puts every marker of a rule under the one above it.
        static constexpr std::size_t k_channelColumn = 48ull;
        // The column a rule's channel marker starts at for a rule standing one level in. A rule
        // standing deeper takes a column further right by the same amount.
        static constexpr std::size_t k_ruleChannelColumn{ 52ull };
        [[nodiscard]] static constexpr std::size_t ruleMarkerColumn(std::size_t ruleLevel)
        {
            return k_ruleChannelColumn + (ruleLevel - 1ull) * k_indentWidth;
        }
        static constexpr std::size_t k_indentWidth = 4ull;
        static constexpr std::wstring_view k_spaces{
            L"                                                                " };
        const AppTheme& m_theme;
        std::wstring m_text{};
        std::size_t m_column{ 0ull };
    };

}
