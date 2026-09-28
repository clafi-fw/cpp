export module ThisApp.ThemeToCppCode;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ClaFi;

    // Whether the block states the whole theme, or only the members this theme moved off the
    // framework's defaults.
    export enum class CodeContent
    {
        Differences,
        Full,
        Count
    };

    // Where the block is meant to be pasted: inside a ThemeColors method, or outside the class,
    // where it writes through a named object.
    export enum class CodeScope
    {
        ClassMethod,
        OutsideClass,
        Count
    };

    // The names the file uses, spelled to match the enumerators. What a person reads is the text
    // of the command that states each choice, in ThisApp.CodeOptions.
    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(CodeContent::Count)>
        k_codeContentKeys{
            L"Differences",
            L"Full"
        };

    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(CodeScope::Count)>
        k_codeScopeKeys{
            L"ClassMethod",
            L"OutsideClass"
        };

    export constexpr auto enumNames(CodeContent) { return k_codeContentKeys; }
    export constexpr auto enumNames(CodeScope) { return k_codeScopeKeys; }

    // What writing the built-in colours came to.
    export enum class SourceWrite
    {
        Written,
        Unchanged,  // the file already states this theme and is left as it stands
        Failed
    };

    // The block that states a theme, as plain C++ source. Its channel markers line up in a
    // monospaced style only.
    export [[nodiscard]] std::wstring themeToCppCode(const ThemeColors&, CodeContent, CodeScope);

    // The whole of the built-in colours' file, stating this theme as the framework's own.
    export [[nodiscard]] std::wstring themeToSourceFile(const ThemeColors&);

    // That file in the source tree this application was compiled from, or nothing without one.
    export [[nodiscard]] std::filesystem::path builtInColorsFile();

    // Writes the file in the line ends the module beside it is checked out with.
    export [[nodiscard]] SourceWrite writeBuiltInColors(const std::filesystem::path& file,
        const ThemeColors&);

    // What stands in the built-in colours' file around the constructor's statements.
    constexpr std::wstring_view k_sourceFileHead{
        L"// The framework's own theme, written whole by the Themes app's Write to source.\n"
        L"// An edit made here is lost at the next write.\n"
        L"module ClaFi.Core.AppTheme_Colors;\n"
        L"\n"
        L"import ClaFi.Core.AppTheme_Palette;\n"
        L"\n"
        L"import ClaFi.StdLib;\n"
        L"\n"
        L"namespace ClaFi\n"
        L"{\n"
        L"    ThemeColors::ThemeColors()\n"
        L"    {\n" };

    constexpr std::wstring_view k_sourceFileTail{
        L"    }\n"
        L"}\n" };

    // How deep the constructor's statements stand: the namespace, then the body.
    constexpr std::size_t k_sourceFileIndent{ 8ull };

    // The module the built-in colours belong to, whose file marks a folder as the tree's.
    constexpr std::wstring_view k_colorsModuleFileName{ L"AppTheme_Colors.cppm" };
    constexpr std::wstring_view k_builtInColorsFileName{ L"AppTheme_BuiltInColors.cpp" };

    // The line end a file is checked out with, read off its first line.
    [[nodiscard]] std::string_view lineEndOf(const std::filesystem::path& file);
    // Everything a file holds, or nothing where there is no file to read.
    [[nodiscard]] std::string fileBytes(const std::filesystem::path& file);

    // A list ThemeRules holds by name rather than by element, and the name generated code gives it.
    struct NamedRules
    {
        std::wstring_view codeName{};
        ColorRules ThemeRules::* rules{ nullptr };
    };

    // In ThemeRules declaration order, which is the order the block states them in.
    constexpr std::array<NamedRules, 3ull> k_namedRules{
        NamedRules{ .codeName = L"shared", .rules = &ThemeRules::shared },
        NamedRules{ .codeName = L"anyWindow", .rules = &ThemeRules::anyWindow },
        NamedRules{ .codeName = L"focusRing", .rules = &ThemeRules::focusRing }
    };

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
        CppCodeGenerator(const ThemeColors&, CodeContent, CodeScope);
        // The statements of the built-in theme's constructor, whose object starts with no rules.
        explicit CppCodeGenerator(const ThemeColors&);
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
        //
        // The member a statement writes to, carrying the object name an outside block goes
        // through.
        CppCodeGenerator& memberName(std::wstring_view name);
        // The head of one member statement, up to where its braced value begins.
        CppCodeGenerator& openMember(std::wstring_view typeName, std::wstring_view name);
        // Whether a member is stated: because it differs from the framework's defaults, or
        // because the block states every member regardless.
        [[nodiscard]] bool statesMember(bool differs) const;
        void colorMember(UiElement);
        void floatMember(std::wstring_view name, float value, float defaultValue);
        void paletteMembers();
        void paletteHuesMember();
        // Full states the rules from none, so there an empty list needs no statement.
        [[nodiscard]] bool statesList(const ColorRules& value,
            const ColorRules& defaultValue) const;
        // One list's statement. target is what follows the object: a name or an of() call.
        void rulesList(std::wstring_view target, const ColorRules& value,
            const ColorRules& defaultValue);
        void themeRules();
        // The object an outside block writes through. A method writes through the class.
        void objectDeclaration();
        void headerComment();
        //
        void build();
    private:
        // The object an outside block writes through.
        static constexpr std::wstring_view k_objectName{ L"colors" };
        // The column a channel marker starts at. The block is shown in a monospaced style, so
        // one column puts every marker of a rule under the one above it.
        static constexpr std::size_t k_channelColumn = 48ull;
        // The column a rule's channel marker starts at. Its channels stand deeper than a member's.
        static constexpr std::size_t k_ruleChannelColumn{ 52ull };
        static constexpr std::size_t k_indentWidth = 4ull;
        static constexpr std::wstring_view k_spaces{
            L"                                                                " };
        const ThemeColors& m_colors;
        // What the framework hands out, which is what a difference is measured against.
        const ThemeColors m_defaults{};
        CodeContent m_content;
        CodeScope m_scope;
        bool m_startsEmpty{ false }; // a constructor's statements: no header, no rules to clear
        std::wstring m_text{};
        std::size_t m_column{ 0ull };
        bool m_statedAnyMember{ false };
        bool m_statedAnyList{ false };
    };

}
