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
    // Which members there are, what each one is called in generated code and which of its states
    // the block may state are k_uiElements' answer, not this table's.
    struct MemberComment
    {
        UiElement element{};
        std::wstring_view text{};
    };

    constexpr std::array<MemberComment, 12ull> k_memberComments{
        MemberComment{ .element = UiElement::Form, .text =
            L"THE WINDOW EVERYTHING ELSE IS PAINTED ON. A form is the root a control tree stands in,\n"
            L"and the pair this set states is what that root establishes: surface is applied to the\n"
            L"bare colour of the mode - black at the dark end, white at the light one - and text is\n"
            L"applied to the bare ink, white at the dark end and black at the light one, carrying the\n"
            L"surface's hue so that a rule raising saturation alone tints toward the family the theme\n"
            L"is already in. Every control carries the ink it inherits and applies its own text rule\n"
            L"to that, so a rule stated here reaches everything in the form.\n"
            L"\n"
            L"Menu and tooltip are the other two window roots, each stating the same pair for the\n"
            L"window it opens." },
        MemberComment{ .element = UiElement::TabLine, .text =
            L"Every tab's outline and the line it stands on, applied to the tab's own surface." },
        MemberComment{ .element = UiElement::Header, .text =
            L"The strip an expander shows its title on, and the row of column names across the top\n"
            L"of a grid. An expander takes the whole set for its header; a grid header takes surface\n"
            L"and text and leaves its other states as they stand, so those two are the whole of what\n"
            L"a column name is drawn in." },
        MemberComment{ .element = UiElement::Bar, .text =
            L"A STRIP OF COMMANDS ACROSS AN EDGE OF A WINDOW: a toolbar along the top, the row of\n"
            L"answers along the bottom of a message box, and anything else that is a band of the\n"
            L"window rather than a card standing in it. A bar is what the controls on it are painted\n"
            L"over, so what it states is a place for them to stand and never an emphasis of its own -\n"
            L"it names no state, because nothing makes a bar hovered, pressed or the one in effect." },
        MemberComment{ .element = UiElement::Grid, .text =
            L"THE GRID AS A WHOLE, and its stroke is the grid's OUTER border - the line around the\n"
            L"lattice and not one of the lines in it, which is gridLine's. GridBase wears this set,\n"
            L"so the surface is what every row and cell of the grid stands on." },
        MemberComment{ .element = UiElement::GridRow, .text =
            L"Every row of a grid, groups and sections included. See Grids" },
        MemberComment{ .element = UiElement::GridLine, .text =
            L"Every line of a grid's lattice. One rule for the whole lattice, so a cell's own\n"
            L"surface cannot move the line beside it." },
        MemberComment{ .element = UiElement::SelectedText, .text =
            L"The band behind selected text, and the ink drawn on it. The two are resolved in\n"
            L"different places: the band once, against the surface the text sits on, and the ink\n"
            L"where each run is drawn, against whatever colour that run already carries - so a grey\n"
            L"run stays grey under the band and an accent run stays accent.\n"
            L"\n"
            L"surface is the band while the focus is elsewhere, and active is what the focus adds to\n"
            L"it, applied by the owning control's focused factor so the two crossfade as the focus\n"
            L"moves. Both are on screen together whenever a second box still holds a selection,\n"
            L"which is what they exist to tell apart." },
        MemberComment{ .element = UiElement::InactiveIndicator, .text =
            L"THE MARK WHILE IT IS OFF: the empty box of a check, the ring of a radio button, the\n"
            L"well a caret or a selection band is raised out of. What the mark becomes when it comes\n"
            L"on is the accent, stated once there, so this set names no active state at all - see\n"
            L"accent. The states it does name answer the pointer, so a mark inside a button can move\n"
            L"with the button around it, and text is the ink over the mark once something is drawn\n"
            L"on it." },
        MemberComment{ .element = UiElement::Accent, .text =
            L"THE THEME'S OWN EMPHASIS, AND WHAT SAYS A THING IS ON - one rule for both, because the\n"
            L"two are one colour. It is the ink anything asking for emphasis is drawn in, over the\n"
            L"palette's accent hue - the same one a button under the pointer moves toward, so an icon\n"
            L"drawn in it belongs to the family of the controls around it - and it is equally the\n"
            L"active state of every mark: a check, a radio dot, a text caret, a hot link, the band a\n"
            L"StackView draws behind a selected item, the focus ring while the user is on the control,\n"
            L"and the indicator under an open tab. That is why inactiveIndicator states no active rule\n"
            L"of its own: the on state is here, once, and the whole interface says it in one colour." },
        MemberComment{ .element = UiElement::Spot, .text =
            L"The second accent, over the palette's third hue: what stands apart from the interface\n"
            L"rather than answers to it - a brand mark, a run of emphasised text, and the tint a\n"
            L"tooltip carries. Stated apart from the accent so that a theme can spend one sparingly\n"
            L"while the other runs through every control." },
        MemberComment{ .element = UiElement::ScrollButton, .text =
            L"The button at each end of a scroll bar. ScrollBar hands this set to those two and\n"
            L"scrollThumb to the thumb, so the halves of a bar are themed apart: a thumb has to\n"
            L"read against the trough it runs in, an end button against the bar." }
    };

    // The prose that member carries, or nothing where it carries none.
    [[nodiscard]] std::wstring_view memberCommentOf(UiElement);

    // The palette's two members and the dark mode floor are not stated through k_uiElements - a
    // float and an array of floats are neither a rule nor a set of them - so their prose is kept
    // here, under the same rule: what the block does not carry, the next regeneration deletes.
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

    // A comment standing inside one member's braces, above one of its state rules. It is named by
    // the pair because a state means something different in each member that carries it, so this
    // is a table of its own rather than a field on the state's own descriptor.
    struct StateComment
    {
        std::wstring_view memberName{};
        std::wstring_view stateName{};
        std::wstring_view text{};
    };

    constexpr std::array<StateComment, 2ull> k_stateComments{
        StateComment{ .memberName = L"gridRow", .stateName = L"active", .text =
            L"A SELECTED ROW. A row wears this set and paints no surface of its own, so the rule\n"
            L"reaches the screen through the cells the row fills. It lands where a focused text\n"
            L"selection lands - selectedText's surface and active come to the same place - so the\n"
            L"two selections an interface can show read as one colour." },
        StateComment{ .memberName = L"inactiveIndicator", .stateName = L"text", .text =
            L"Set states an absolute target and reads it against the colour mode, so 0 comes out\n"
            L"at the floor in dark mode and white in light mode. That is how a mark gets ink that\n"
            L"contrasts with it under either mode, without the element having to state a flip." }
    };

    // The comment that state carries in that member, or nothing where it carries none.
    [[nodiscard]] std::wstring_view stateCommentOf(std::wstring_view memberName,
        std::wstring_view stateName);

    // Whether two values would come out as the same code. The rule types carry no comparison of
    // their own, and a theme is measured field by field against the framework's defaults to
    // decide what CodeContent::Differences leaves out.
    [[nodiscard]] bool isSame(const ColorRuleValue&, const ColorRuleValue&);
    [[nodiscard]] bool isSame(const ColorRuleHue&, const ColorRuleHue&);
    [[nodiscard]] bool isSame(const ColorRule&, const ColorRule&);
    // Only the states the element paints with are compared. A rule the element never applies
    // cannot make a block differ from the defaults, because the block would not state it.
    [[nodiscard]] bool isSame(const ControlColorRules&, const ControlColorRules&, UiElementStates);

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
        // The braces of one rule, opening to closing, over as many lines as it has channels.
        // level is the indent its closing brace sits at. Saturation and elevation are positional
        // arguments, so both are stated whenever the rule states anything; hue is stated only
        // when it moves.
        CppCodeGenerator& colorRule(const ColorRule&, std::size_t level);
        // The same for a set of state rules, as designated initializers. Only the states the
        // owner paints with are considered, and of those only the ones it moves off a bare
        // ControlColorRules are named. The owner is also what a state's own comment is looked up
        // by - a state means something different in each member that carries it.
        CppCodeGenerator& controlColorRules(const ControlColorRules&, std::size_t level,
            const UiElementDescriptor& owner);
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
