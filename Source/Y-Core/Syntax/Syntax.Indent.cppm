export module ClaFi.Core.Syntax.Indent;

import ClaFi.Core.Syntax.Lines;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    // The columns a tab reaches to: the layout's own tab stop.
    export constexpr std::size_t k_tabColumns = static_cast<std::size_t>(k_tabStopSpaces);

    // How one level of indent is written. See Syntax#indent
    export struct IndentUnit
    {
        bool tabs{ false };       // a tab wherever a whole one fits, spaces for the rest
        std::size_t width{ 4 };   // the columns one level steps by
        bool operator==(const IndentUnit&) const = default;
    };

    // One replacement an indent key makes, and the selection it leaves. See Syntax#indent
    export struct IndentEdit
    {
        TextRange replaced{};        // in the text before the edit
        std::wstring inserted{};
        TextRange selection{};       // in the text after the edit
        bool caretOnLeft{ false };   // the caret at the selection's start rather than its end
    };

    // A text's lines over its line states, some restated ahead of an edit. See Syntax#indent
    export class IndentLines : public SourceLines
    {
    public:
        // The states have to be in step with the text, and both have to outlive the lines.
        IndentLines(LineStates&, std::wstring_view text);
    public:
        [[nodiscard]] std::size_t count() const override;
        [[nodiscard]] std::wstring_view text(std::size_t line) const override;
        [[nodiscard]] const Tokens& tokens(std::size_t line) const override;
        [[nodiscard]] bool continues(std::size_t line) const override;
        [[nodiscard]] const Language& language() const;
        // The text as it stands, the restated lines aside.
        [[nodiscard]] std::wstring_view heldText() const { return m_text; }
        // Where the line starts in the text, which a restated line keeps.
        [[nodiscard]] std::size_t start(std::size_t line) const;
        [[nodiscard]] std::size_t lineAt(std::size_t pos) const;
        // Reads the line as that text from here on, ahead of the edit that will put it there.
        void restate(std::size_t line, std::wstring lineText);
    private:
        using Restated = std::unordered_map<std::size_t, std::wstring>;
        using LineTokens = std::unordered_map<std::size_t, Tokens>;
    private:
        // Not owned, and written through a const reading: the tokens are lexed on demand into
        // the states' own buffers, which is structure, not the lines' state.
        LineStates& m_states;
        std::wstring_view m_text;
        Restated m_restated{};
        mutable LineTokens m_tokens{};
    };

    // How many blanks the line opens with.
    export [[nodiscard]] std::size_t leadingBlanks(std::wstring_view line);
    // The column a position of the line stands at, a tab reaching the next tab stop.
    export [[nodiscard]] std::size_t columnAt(std::wstring_view line, std::size_t index);
    // The column the line's first character past its blanks stands at.
    export [[nodiscard]] std::size_t indentColumns(std::wstring_view line);
    // The blanks that reach that column from the line's start, written the unit's way.
    export [[nodiscard]] std::wstring indentText(std::size_t columns, IndentUnit);
    // The first stop `width` columns apart past the column.
    export [[nodiscard]] std::size_t nextStop(std::size_t column, std::size_t width);
    // The last stop `width` columns apart before the column, and zero from the first stop in.
    export [[nodiscard]] std::size_t previousStop(std::size_t column, std::size_t width);
    // What Tab puts in at a column: the blanks that reach the next stop, the unit's way.
    export [[nodiscard]] std::wstring stopInsertion(std::size_t column, IndentUnit);
    // What a text's own lines say a level is written as - nothing where they say nothing.
    export [[nodiscard]] std::optional<IndentUnit> detectIndentUnit(std::wstring_view text);

    // Where a language places the line: its own rule, or its brackets where it states none. A
    // line inside a comment or a string the line before it left open keeps that line's column.
    export [[nodiscard]] LineIndent lineIndent(const Language&, const SourceLines&,
        std::size_t line, std::size_t width);
    // The rule of a language that states none: a level per bracket left open. See Syntax#indent
    export [[nodiscard]] LineIndent bracketIndent(const SourceLines&, std::size_t line,
        std::size_t width);
    // Whether the line's first word ends at `end`, or its first character is a mark standing
    // just before it - a word just finished, a closing bracket just typed.
    export [[nodiscard]] bool firstWordEndsAt(const Language&, std::wstring_view line,
        std::size_t end);

    // What a line break typed at the start of `line` goes on to do: the line left trimmed and
    // realigned, the line begun placed, a block split or closed. See Syntax#indent
    export [[nodiscard]] std::optional<IndentEdit> breakEdit(IndentLines&, std::size_t line,
        IndentUnit);
    // The line put at the column its first word decides, where that differs from its own.
    export [[nodiscard]] std::optional<IndentEdit> realignEdit(IndentLines&, std::size_t line,
        IndentUnit, std::size_t caret);
    // Every line from first to last holding more than blanks, moved to the next stop or back to
    // the previous one. See Syntax#indent
    export [[nodiscard]] std::optional<IndentEdit> shiftEdit(IndentLines&, std::size_t first,
        std::size_t last, bool back, IndentUnit, TextRange selection, bool caretOnLeft);
    // Every line from first to last placed where the language places it, top down.
    export [[nodiscard]] std::optional<IndentEdit> reindentEdit(IndentLines&, std::size_t first,
        std::size_t last, IndentUnit, TextRange selection, bool caretOnLeft);
    // The lines of a paste moved as one block to where the language places it. See Syntax#indent
    export [[nodiscard]] std::optional<IndentEdit> pasteEdit(IndentLines&, TextRange pasted,
        IndentUnit);
    // The blanks Backspace takes at a caret in a line's indent: back to the previous stop.
    export [[nodiscard]] std::optional<TextRange> unindentRange(std::wstring_view line,
        std::size_t caret, std::size_t width);
    // Where Home takes a caret on the line: past the blanks, and to the line's start from there.
    export [[nodiscard]] std::size_t homeIndex(std::wstring_view line, std::size_t caret);
}
