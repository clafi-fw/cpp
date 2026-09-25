export module ClaFi.Core.Syntax.Lines;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    // The state every line of a text starts in, kept in step with the text through its edits.
    // See Syntax
    export class LineStates
    {
    public:
        // Lexes the whole text from its start.
        void reset(const Language&, std::wstring_view text);
        // Re-lexes from the first line the edit reached, and on until a line starts in the state
        // it already had. The edit is stated in the coordinates of the text BEFORE it, the text
        // given is the one after, and the two have to agree on the length that leaves.
        void applyEdit(std::wstring_view textAfter, const TextRange& replaced,
            std::size_t insertedLength);
        [[nodiscard]] std::size_t lineCount() const { return m_starts.size(); }
        [[nodiscard]] const Language& language() const { return m_language; }
        // The tokens of one line, lexed now from the state it starts in. The line's text is the
        // caller's, since it is what the caller is about to draw.
        void tokensOf(std::size_t line, std::wstring_view lineText, Tokens&);
    private:
        using Starts = std::vector<std::size_t>;
        using States = std::vector<LineState>;
    private:
        // Lexes from `line` on, writing the state each following line starts in. Stops at the
        // first line from `converged` on that already starts in the state the line before it
        // left, since everything past it stands as it was.
        void relexFrom(std::wstring_view text, std::size_t line, std::size_t converged);
        [[nodiscard]] std::wstring_view lineText(std::wstring_view text, std::size_t line) const;
        // The line a position stands on. A position right after a newline is on the line that
        // newline opened.
        [[nodiscard]] std::size_t lineAt(std::size_t pos) const;
    private:
        Language m_language{};
        // Where each line begins. A line ends at the next start less its newline, the last at
        // the text's end.
        Starts m_starts;
        States m_states;
        StateStrings m_strings;
        std::size_t m_textLength{ 0 };
    };
}
