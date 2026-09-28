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

    // Where the block is meant to be pasted. It picks how a member is stated: a declaration
    // carrying its initializer inside ThemeColors, an assignment inside one of its methods, or
    // an assignment to a named object from outside the class.
    export enum class CodeScope
    {
        ClassDeclarations,
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
            L"ClassDeclarations",
            L"ClassMethod",
            L"OutsideClass"
        };

    export constexpr auto enumNames(CodeContent) { return k_codeContentKeys; }
    export constexpr auto enumNames(CodeScope) { return k_codeScopeKeys; }

    // The block that states a theme, as plain C++ source. Its channel markers line up in a
    // monospaced style only.
    export [[nodiscard]] std::wstring themeToCppCode(const ThemeColors&, CodeContent, CodeScope);

    // WHAT STANDS ABOVE A MEMBER IN THE SOURCE, without the marker or the indent, lines separated
    // by a newline. THE BLOCK IS PASTED OVER THE DECLARATIONS, so a comment the block does not
    // carry is a comment the next regeneration deletes: this is where a member of ThemeColors
    // keeps its prose, and the declaration there keeps a copy. A member named by no entry here
    // carries none.
    //
    // WHAT IS WRITTEN HERE HAS TO OUTLIVE THE NUMBERS BESIDE IT. The block is generated from
    // whatever theme is open, so it restates every value and none of the reasoning: a comment
    // that compares one member's numbers with another's - carries the button's set, states the
    // same set as section - is true of the defaults it was written against and false the first
    // time someone regenerates from a theme they have edited. What a member is, what reads it,
    // and how it resolves survive that; what it currently equals does not.
    //
    // Which members there are and what each one is called in generated code are k_uiElements'
    // answer, not this table's.
    struct MemberComment
    {
        UiElement element{};
        std::wstring_view text{};
    };

    constexpr std::array<MemberComment, 2ull> k_memberComments{
        MemberComment{ .element = UiElement::Accent, .text =
            L"THE THEME'S OWN EMPHASIS, AND WHAT SAYS A THING IS ON - one effect for both, because the\n"
            L"two are one colour. It is the ink anything asking for emphasis is drawn in, over the\n"
            L"palette's accent hue - the same one a button under the pointer moves toward, so an icon\n"
            L"drawn in it belongs to the family of the controls around it - and it is equally the\n"
            L"active state of every mark: a text caret, a hot link, the band a StackView draws behind a\n"
            L"selected item, the focus ring while the user is on the control, and the indicator under\n"
            L"an open tab. A check and a radio dot state their own on colour - see SelectionIndicator." },
        MemberComment{ .element = UiElement::Spot, .text =
            L"The second accent, over the palette's third hue: what stands apart from the interface\n"
            L"rather than answers to it - a brand mark, a run of emphasised text, and the tint a\n"
            L"tooltip carries. Stated apart from the accent so that a theme can spend one sparingly\n"
            L"while the other runs through every control." }
    };

    // The prose that member carries, or nothing where it carries none.
    [[nodiscard]] std::wstring_view memberCommentOf(UiElement);

    // The palette's two members and the dark mode floor are not stated through k_uiElements - a
    // float and an array of floats are not effects - so their prose is kept here, under the same
    // rule: what the block does not carry, the next regeneration deletes.
    constexpr std::wstring_view k_anchorHueComment{
        L"The hue a harmony turns around, and the only thing about the anchor anyone chooses.\n"
        L"A palette is a set of hues and nothing else, so what a swatch or a slider ramp is\n"
        L"drawn in comes from the palette's display pair rather than from here." };

    constexpr std::wstring_view k_paletteHuesComment{
        L"The three hues a rule can name. Only a hue is the palette's own: a control keeps the\n"
        L"saturation and luminosity it already carries, and the palette swatch borrows the\n"
        L"anchor's." };

    constexpr std::wstring_view k_darkModeFloorComment{
        L"The luminosity elevation 0 is lifted to at the dark end. See AppTheme" };

    // Whether two values would come out as the same code. The rule types carry no comparison of
    // their own, and a theme is measured field by field against the framework's defaults to
    // decide what CodeContent::Differences leaves out.
    [[nodiscard]] bool isSame(const ColorRuleValue&, const ColorRuleValue&);
    [[nodiscard]] bool isSame(const ColorRuleHue&, const ColorRuleHue&);
    [[nodiscard]] bool isSame(const ColorEffect&, const ColorEffect&);

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
        CppCodeGenerator& colorEffect(const ColorEffect&, std::size_t level);
        //
        // The member a statement writes to, carrying the object name an outside block goes
        // through.
        CppCodeGenerator& memberName(std::wstring_view name);
        // The head of one member statement, up to where its braced value begins.
        CppCodeGenerator& openMember(std::wstring_view typeName, std::wstring_view name);
        // The prose above whatever comes next, one line at a time, each carrying the marker and
        // the indent. Nothing is written for an empty text.
        CppCodeGenerator& memberComment(std::wstring_view text, std::size_t level);
        // Whether a member is stated: because it differs from the framework's defaults, or
        // because the block states every member regardless.
        [[nodiscard]] bool statesMember(bool differs) const;
        void colorMember(UiElement);
        void floatMember(std::wstring_view name, float value, float defaultValue,
            std::wstring_view comment);
        void paletteMembers();
        void paletteHuesMember();
        // The object an outside block writes through. The other scopes write through the class.
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
        static constexpr std::size_t k_indentWidth = 4ull;
        static constexpr std::wstring_view k_spaces{
            L"                                                                " };
        const ThemeColors& m_colors;
        // What the framework hands out, which is what a difference is measured against.
        const ThemeColors m_defaults{};
        CodeContent m_content;
        CodeScope m_scope;
        std::wstring m_text{};
        std::size_t m_column{ 0ull };
        bool m_statedAnyMember{ false };
    };

}
