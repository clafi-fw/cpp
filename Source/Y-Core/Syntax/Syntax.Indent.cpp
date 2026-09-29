module ClaFi.Core.Syntax.Indent;

import ClaFi.Core.Syntax.Lines;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    namespace
    {
        // One line's indent as an edit leaves it.
        struct IndentChange
        {
            std::size_t line{ 0 };
            std::size_t blanks{ 0 };   // the length of the blanks it replaces
            std::wstring indent{};
        };

        using IndentChanges = std::vector<IndentChange>;

        // A bracket of a line, where it stands.
        struct Bracket
        {
            std::size_t line{ 0 };
            std::size_t index{ 0 };
            wchar_t value{ L'\0' };
        };

        using Brackets = std::vector<Bracket>;

        // The indent changes shorter than this share one step of a detected width.
        constexpr std::size_t k_widestLevel = 8;

        [[nodiscard]] bool isBlankLine(const std::wstring_view line)
        {
            return leadingBlanks(line) == line.size();
        }

        [[nodiscard]] bool opensBracket(const wchar_t value)
        {
            return value == L'(' || value == L'[' || value == L'{';
        }

        [[nodiscard]] bool closesBracket(const wchar_t value)
        {
            return value == L')' || value == L']' || value == L'}';
        }

        // What a block opened by the bracket is closed with. A parenthesis opens no block.
        [[nodiscard]] std::wstring_view blockCloserOf(const wchar_t opener)
        {
            if (opener == L'{')
                return L"}";
            if (opener == L'[')
                return L"]";
            return {};
        }

        // The blanks from one column to another, a tab wherever a whole one fits when the unit
        // writes tabs.
        [[nodiscard]] std::wstring blanksBetween(std::size_t from, const std::size_t to,
            const IndentUnit unit)
        {
            std::wstring result;
            if (unit.tabs)
            {
                while (nextStop(from, k_tabColumns) <= to)
                {
                    result.push_back(L'\t');
                    from = nextStop(from, k_tabColumns);
                }
            }
            if (to > from)
                result.append(to - from, L' ');
            return result;
        }

        // The brackets of a line in its own order - the punctuation's, so none inside a comment
        // or a string.
        void appendBrackets(const SourceLines& lines, const std::size_t line, Brackets& out)
        {
            const std::wstring_view text = lines.text(line);
            for (const Token& token : lines.tokens(line))
            {
                if (token.kind != TokenKind::Punctuation)
                    continue;
                for (std::size_t i = token.range.start; i != token.range.end(); ++i)
                {
                    if (opensBracket(text[i]) || closesBracket(text[i]))
                        out.push_back({ line, i, text[i] });
                }
            }
        }

        // The line holding the bracket the one standing before `index` on `line` would close,
        // read back from there - nothing where none is left open.
        [[nodiscard]] std::optional<std::size_t> openingLine(const SourceLines& lines,
            const std::size_t line, const std::size_t index)
        {
            std::size_t depth = 0;
            Brackets brackets;
            std::size_t current = line + 1;
            while (current != 0)
            {
                --current;
                brackets.clear();
                appendBrackets(lines, current, brackets);
                for (Brackets::const_reverse_iterator it = brackets.rbegin(); it != brackets.rend();
                    ++it)
                {
                    if (current == line && it->index >= index)
                        continue;
                    if (closesBracket(it->value))
                    {
                        ++depth;
                        continue;
                    }
                    if (depth == 0)
                        return current;
                    --depth;
                }
            }
            return std::nullopt;
        }

        // The nearest line above that holds more than blanks.
        [[nodiscard]] std::optional<std::size_t> filledLineAbove(const SourceLines& lines,
            std::size_t line)
        {
            while (line != 0)
            {
                --line;
                if (!isBlankLine(lines.text(line)))
                    return line;
            }
            return std::nullopt;
        }

        // Where a position lands once the changes are in. A position inside the blanks a change
        // replaced keeps its offset as far as the new blanks reach, the start of the line's text
        // stays the start of its text, and the line's own start stays its start.
        [[nodiscard]] std::size_t moved(const IndentLines& lines, const IndentChanges& changes,
            const std::size_t pos)
        {
            std::ptrdiff_t shift = 0;
            for (const IndentChange& change : changes)
            {
                const std::size_t lineStart = lines.start(change.line);
                if (pos < lineStart)
                    break;
                const std::size_t offset = pos - lineStart;
                if (offset <= change.blanks)
                {
                    std::size_t landed = std::min(offset, change.indent.size());
                    if (offset != 0 && offset == change.blanks)
                        landed = change.indent.size();
                    return static_cast<std::size_t>(static_cast<std::ptrdiff_t>(lineStart) + shift)
                        + landed;
                }
                shift += static_cast<std::ptrdiff_t>(change.indent.size())
                    - static_cast<std::ptrdiff_t>(change.blanks);
            }
            return static_cast<std::size_t>(static_cast<std::ptrdiff_t>(pos) + shift);
        }

        // The changes as one replacement, from the first changed line's start to the last one's
        // blanks, with the selection carried over it. The changes stand in line order.
        [[nodiscard]] IndentEdit editOf(const IndentLines& lines, const IndentChanges& changes,
            const TextRange selection, const bool caretOnLeft)
        {
            const std::wstring_view held = lines.heldText();
            const std::size_t from = lines.start(changes.front().line);
            std::wstring inserted;
            std::size_t at = from;
            for (const IndentChange& change : changes)
            {
                const std::size_t lineStart = lines.start(change.line);
                inserted.append(held.substr(at, lineStart - at));
                inserted.append(change.indent);
                at = lineStart + change.blanks;
            }

            const std::size_t selectionStart = moved(lines, changes, selection.start);
            const std::size_t selectionEnd = moved(lines, changes, selection.end());
            return {
                .replaced = { from, at - from },
                .inserted = std::move(inserted),
                .selection = { selectionStart, selectionEnd - selectionStart },
                .caretOnLeft = caretOnLeft,
            };
        }

        // Records the line's indent as the language places it, restating the line so that the
        // lines after it read it there, and answers the column it stands at.
        std::size_t placeByRule(IndentLines& lines, const std::size_t line, const IndentUnit unit,
            IndentChanges& changes)
        {
            const std::wstring text{ lines.text(line) };
            const std::size_t blanks = leadingBlanks(text);
            const LineIndent placed = lineIndent(lines.language(), lines, line, unit.width);
            std::wstring indent = indentText(placed.column, unit);
            if (indent != std::wstring_view{ text }.substr(0, blanks))
            {
                lines.restate(line, indent + text.substr(blanks));
                changes.push_back({ line, blanks, std::move(indent) });
            }
            return placed.column;
        }

        // Whether the block a line at `column` opens is closed below `line` already: the next line
        // holding anything stands inside the block, or at its column and placing itself there.
        [[nodiscard]] bool closedBelow(const IndentLines& lines, const std::size_t line,
            const std::size_t column, const std::size_t width)
        {
            for (std::size_t below = line + 1; below < lines.count(); ++below)
            {
                const std::wstring_view text = lines.text(below);
                if (isBlankLine(text))
                    continue;
                const std::size_t belowColumn = indentColumns(text);
                if (belowColumn != column)
                    return belowColumn > column;
                return lineIndent(lines.language(), lines, below, width).placesItself;
            }
            return false;
        }
    }

    // IndentLines

    IndentLines::IndentLines(LineStates& states, const std::wstring_view text)
        :
        m_states{ states },
        m_text{ text }
    {
    }

    std::size_t IndentLines::count() const
    {
        return m_states.lineCount();
    }

    std::wstring_view IndentLines::text(const std::size_t line) const
    {
        const Restated::const_iterator restated = m_restated.find(line);
        if (restated != m_restated.end())
            return restated->second;
        return m_states.lineText(m_text, line);
    }

    const Tokens& IndentLines::tokens(const std::size_t line) const
    {
        const LineTokens::const_iterator lexed = m_tokens.find(line);
        if (lexed != m_tokens.end())
            return lexed->second;
        Tokens& tokens = m_tokens[line];
        m_states.tokensOf(line, text(line), tokens);
        return tokens;
    }

    bool IndentLines::continues(const std::size_t line) const
    {
        return m_states.stateOf(line).mode != Mode::normal;
    }

    const Language& IndentLines::language() const
    {
        return m_states.language();
    }

    std::size_t IndentLines::start(const std::size_t line) const
    {
        return m_states.lineStart(line);
    }

    std::size_t IndentLines::lineAt(const std::size_t pos) const
    {
        return m_states.lineAt(pos);
    }

    void IndentLines::restate(const std::size_t line, std::wstring lineText)
    {
        m_restated[line] = std::move(lineText);
        m_tokens.erase(line);
    }

    // Columns and stops

    std::size_t leadingBlanks(const std::wstring_view line)
    {
        std::size_t count = 0;
        while (count != line.size() && isBlank(line[count]))
            ++count;
        return count;
    }

    std::size_t columnAt(const std::wstring_view line, const std::size_t index)
    {
        std::size_t column = 0;
        const std::size_t end = std::min(index, line.size());
        for (std::size_t i = 0; i != end; ++i)
        {
            if (line[i] == L'\t')
                column = nextStop(column, k_tabColumns);
            else
                ++column;
        }
        return column;
    }

    std::size_t indentColumns(const std::wstring_view line)
    {
        return columnAt(line, leadingBlanks(line));
    }

    std::wstring indentText(const std::size_t columns, const IndentUnit unit)
    {
        return blanksBetween(0, columns, unit);
    }

    std::size_t nextStop(const std::size_t column, const std::size_t width)
    {
        if (width == 0)
            return column;
        return (column / width + 1) * width;
    }

    std::size_t previousStop(const std::size_t column, const std::size_t width)
    {
        if (width == 0 || column == 0)
            return column;
        return (column - 1) / width * width;
    }

    std::wstring stopInsertion(const std::size_t column, const IndentUnit unit)
    {
        return blanksBetween(column, nextStop(column, unit.width), unit);
    }

    std::optional<IndentUnit> detectIndentUnit(const std::wstring_view text)
    {
        // The width is the step most often taken between the indents of two lines that follow
        // one another, which is what a level is; a step of one is an alignment, not a level.
        std::array<std::size_t, k_widestLevel + 1> steps{};
        std::size_t tabLines = 0;
        std::size_t spaceLines = 0;
        std::optional<std::size_t> previousSpaces;
        std::size_t lineStart = 0;
        while (lineStart <= text.size())
        {
            std::size_t lineEnd = text.find(L'\n', lineStart);
            if (lineEnd == std::wstring_view::npos)
                lineEnd = text.size();
            const std::wstring_view line = text.substr(lineStart, lineEnd - lineStart);
            lineStart = lineEnd + 1;
            if (isBlankLine(line))
                continue;

            if (line.front() == L'\t')
            {
                ++tabLines;
                previousSpaces.reset();
                continue;
            }
            std::size_t spaces = 0;
            while (spaces != line.size() && line[spaces] == L' ')
                ++spaces;
            if (spaces)
                ++spaceLines;
            if (previousSpaces.has_value())
            {
                const std::size_t step = spaces > previousSpaces.value()
                    ? spaces - previousSpaces.value()
                    : previousSpaces.value() - spaces;
                if (step >= 2 && step <= k_widestLevel)
                    ++steps[step];
            }
            previousSpaces = spaces;
        }

        if (tabLines == 0 && spaceLines == 0)
            return std::nullopt;
        if (tabLines > spaceLines)
            return IndentUnit{ .tabs = true, .width = k_tabColumns };

        // The tab's own width wins a tie, then the narrower step.
        std::size_t width = k_tabColumns;
        for (std::size_t step = 2; step <= k_widestLevel; ++step)
        {
            if (steps[step] > steps[width])
                width = step;
        }
        if (steps[width] == 0)
            return std::nullopt;
        return IndentUnit{ .tabs = false, .width = width };
    }

    // Placing a line

    LineIndent lineIndent(const Language& language, const SourceLines& lines,
        const std::size_t line, const std::size_t width)
    {
        // Inside a comment or a string the words are not the language's, and the line is where
        // the writer put it.
        if (lines.continues(line))
        {
            LineIndent kept;
            if (const std::optional<std::size_t> above = filledLineAbove(lines, line))
                kept.column = indentColumns(lines.text(above.value()));
            return kept;
        }
        if (language.indent)
            return language.indent(lines, line, width);
        return bracketIndent(lines, line, width);
    }

    LineIndent bracketIndent(const SourceLines& lines, const std::size_t line,
        const std::size_t width)
    {
        LineIndent result;
        Brackets brackets;
        appendBrackets(lines, line, brackets);
        if (!brackets.empty() && opensBracket(brackets.back().value))
            result.closer = blockCloserOf(brackets.back().value);

        // A line opening with a closing bracket stands where the line that opened it stands.
        const std::size_t blanks = leadingBlanks(lines.text(line));
        if (!brackets.empty() && brackets.front().index == blanks
            && closesBracket(brackets.front().value))
        {
            result.placesItself = true;
            if (const std::optional<std::size_t> opener = openingLine(lines, line, blanks))
                result.column = indentColumns(lines.text(opener.value()));
            return result;
        }

        const std::optional<std::size_t> above = filledLineAbove(lines, line);
        if (!above.has_value())
            return result;
        const std::size_t aboveColumn = indentColumns(lines.text(above.value()));

        // What the line above leaves open steps in; what it closes of the lines before it steps
        // back to where the line that opened it stands.
        brackets.clear();
        appendBrackets(lines, above.value(), brackets);
        std::size_t open = 0;
        std::optional<std::size_t> unmatchedCloser;
        for (const Bracket& bracket : brackets)
        {
            if (opensBracket(bracket.value))
                ++open;
            else if (open)
                --open;
            else
                unmatchedCloser = bracket.index;
        }

        result.column = aboveColumn;
        if (open)
            result.column = aboveColumn + width;
        else if (unmatchedCloser.has_value())
        {
            const std::optional<std::size_t> opener =
                openingLine(lines, above.value(), unmatchedCloser.value());
            if (opener.has_value())
                result.column = indentColumns(lines.text(opener.value()));
        }
        return result;
    }

    bool firstWordEndsAt(const Language& language, const std::wstring_view line,
        const std::size_t end)
    {
        const std::size_t start = leadingBlanks(line);
        if (start >= end || end > line.size())
            return false;
        if (isNameStart(line[start], language))
            return endOfName(line, start, language) == end;
        return start + 1 == end;
    }

    // The edits

    std::optional<IndentEdit> breakEdit(IndentLines& lines, const std::size_t line,
        const IndentUnit unit)
    {
        if (line == 0 || line >= lines.count())
            return std::nullopt;
        const Language& language = lines.language();
        const std::size_t width = unit.width;
        const std::size_t left = line - 1;
        const std::wstring leftText{ lines.text(left) };
        const std::wstring rightText{ lines.text(line) };
        const std::size_t leftStart = lines.start(left);
        const std::size_t rightStart = lines.start(line);

        // The line left loses its trailing blanks, and all of them where it holds nothing else.
        // A first word it ends with has just been finished, and places the line.
        std::size_t leftEnd = leftText.size();
        while (leftEnd != 0 && isBlank(leftText[leftEnd - 1]))
            --leftEnd;
        const std::size_t leftBlanks = leadingBlanks(leftText);
        std::wstring leftKept = leftText.substr(0, leftEnd);
        bool leftRewritten = false;
        if (leftEnd != 0 && firstWordEndsAt(language, leftText, leftText.size()))
        {
            const LineIndent placed = lineIndent(language, lines, left, width);
            const std::wstring indent = indentText(placed.column, unit);
            if (placed.placesItself && indent != leftText.substr(0, leftBlanks))
            {
                leftKept = indent + leftText.substr(leftBlanks, leftEnd - leftBlanks);
                leftRewritten = true;
            }
        }
        lines.restate(left, leftKept);
        const std::size_t leftColumn = indentColumns(leftKept);

        // The line begun: where an empty line after the left one stands, unless what the break
        // carried down places itself. A closer carried down from under its opener keeps an
        // empty line between the two, and an opener left at the end of its line is closed below
        // it when nothing below closes it yet.
        const std::size_t rightBlanks = leadingBlanks(rightText);
        const std::wstring rightContent = rightText.substr(rightBlanks);
        lines.restate(line, L"");
        const std::size_t inner = lineIndent(language, lines, line, width).column;
        const std::wstring innerIndent = indentText(inner, unit);
        std::wstring tail;
        std::size_t caretInTail = 0;
        if (rightContent.empty())
        {
            tail = innerIndent;
            caretInTail = tail.size();
            const LineIndent opened = lineIndent(language, lines, left, width);
            if (!opened.closer.empty() && !closedBelow(lines, line, leftColumn, width))
            {
                tail.push_back(L'\n');
                tail.append(indentText(leftColumn, unit));
                tail.append(opened.closer);
            }
        }
        else
        {
            lines.restate(line, rightContent);
            const LineIndent placed = lineIndent(language, lines, line, width);
            const bool leftOpens = leftEnd != 0 && inner > leftColumn;
            if (leftOpens && placed.placesItself && placed.column <= leftColumn)
            {
                tail = innerIndent;
                caretInTail = tail.size();
                tail.push_back(L'\n');
                tail.append(indentText(placed.column, unit));
            }
            else
            {
                tail = indentText(placed.column, unit);
                caretInTail = tail.size();
            }
        }

        const std::size_t from = leftRewritten ? leftStart : leftStart + leftEnd;
        const std::size_t to = rightStart + rightBlanks;
        std::wstring inserted = leftRewritten ? leftKept : std::wstring{};
        const std::size_t caret = from + inserted.size() + 1 + caretInTail;
        inserted.push_back(L'\n');
        inserted.append(tail);
        if (inserted == lines.heldText().substr(from, to - from))
            return std::nullopt;
        return IndentEdit{
            .replaced = { from, to - from },
            .inserted = std::move(inserted),
            .selection = { caret, 0 },
        };
    }

    std::optional<IndentEdit> realignEdit(IndentLines& lines, const std::size_t line,
        const IndentUnit unit, const std::size_t caret)
    {
        const std::wstring_view text = lines.text(line);
        const std::size_t blanks = leadingBlanks(text);
        if (blanks == text.size())
            return std::nullopt;
        const LineIndent placed = lineIndent(lines.language(), lines, line, unit.width);
        if (!placed.placesItself)
            return std::nullopt;
        std::wstring indent = indentText(placed.column, unit);
        if (indent == text.substr(0, blanks))
            return std::nullopt;

        const IndentChanges changes = { { line, blanks, std::move(indent) } };
        return editOf(lines, changes, { caret, 0 }, false);
    }

    std::optional<IndentEdit> shiftEdit(IndentLines& lines, const std::size_t first,
        const std::size_t last, const bool back, const IndentUnit unit, const TextRange selection,
        const bool caretOnLeft)
    {
        IndentChanges changes;
        for (std::size_t line = first; line <= last && line < lines.count(); ++line)
        {
            const std::wstring_view text = lines.text(line);
            const std::size_t blanks = leadingBlanks(text);
            if (blanks == text.size())
                continue;
            const std::size_t column = columnAt(text, blanks);
            if (back && column == 0)
                continue;
            const std::size_t target = back
                ? previousStop(column, unit.width)
                : nextStop(column, unit.width);
            std::wstring indent = indentText(target, unit);
            if (indent == text.substr(0, blanks))
                continue;
            changes.push_back({ line, blanks, std::move(indent) });
        }
        if (changes.empty())
            return std::nullopt;
        return editOf(lines, changes, selection, caretOnLeft);
    }

    std::optional<IndentEdit> reindentEdit(IndentLines& lines, const std::size_t first,
        const std::size_t last, const IndentUnit unit, const TextRange selection,
        const bool caretOnLeft)
    {
        IndentChanges changes;
        for (std::size_t line = first; line <= last && line < lines.count(); ++line)
        {
            // A comment or a string running on from the line above is laid out by its writer.
            if (lines.continues(line))
                continue;
            const std::wstring text{ lines.text(line) };
            const std::size_t blanks = leadingBlanks(text);
            if (blanks == text.size())
            {
                // A line of blanks alone is emptied, which the lines below read no differently.
                if (blanks)
                {
                    lines.restate(line, L"");
                    changes.push_back({ line, blanks, L"" });
                }
                continue;
            }
            std::ignore = placeByRule(lines, line, unit, changes);
        }
        if (changes.empty())
            return std::nullopt;
        return editOf(lines, changes, selection, caretOnLeft);
    }

    std::optional<IndentEdit> pasteEdit(IndentLines& lines, const TextRange pasted,
        const IndentUnit unit)
    {
        if (!pasted.length)
            return std::nullopt;
        const std::size_t first = lines.lineAt(pasted.start);
        std::size_t last = lines.lineAt(pasted.end());
        // A paste ending with its newline wrote nothing of the line after it.
        if (last > first && lines.start(last) == pasted.end())
            --last;
        if (last == first)
            return std::nullopt;

        const std::wstring_view held = lines.heldText();
        const bool startsInBlanks =
            leadingBlanks(lines.text(first)) >= pasted.start - lines.start(first);
        const auto nextFilled = [&](std::size_t line) -> std::optional<std::size_t> {
            while (line <= last && isBlankLine(lines.text(line)))
                ++line;
            if (line > last)
                return std::nullopt;
            return line;
        };

        // The first line the paste wrote whole is placed by the language, and the rest keep the
        // shape they had against it - against the column the paste itself gave that line, since
        // blanks standing before the paste are the line's and not the paste's. A paste whose own
        // first line came without its indent says nothing of where that line stood, so the line
        // after it is placed by the language too and is the one the rest keep their shape against.
        std::optional<std::size_t> anchor = nextFilled(startsInBlanks ? first : first + 1);
        if (!anchor.has_value())
            return std::nullopt;
        const std::wstring_view pastedText = held.substr(pasted.start, pasted.length);
        const std::size_t shapeColumn = anchor.value() == first
            ? indentColumns(pastedText)
            : indentColumns(lines.text(anchor.value()));
        IndentChanges changes;
        std::size_t placedColumn = placeByRule(lines, anchor.value(), unit, changes);
        std::ptrdiff_t shift = static_cast<std::ptrdiff_t>(placedColumn)
            - static_cast<std::ptrdiff_t>(shapeColumn);
        const bool firstCameBare = anchor.value() == first && !isBlank(pastedText.front());
        if (firstCameBare)
        {
            if (const std::optional<std::size_t> second = nextFilled(first + 1))
            {
                const std::size_t secondColumn = indentColumns(lines.text(second.value()));
                anchor = second;
                placedColumn = placeByRule(lines, second.value(), unit, changes);
                shift = static_cast<std::ptrdiff_t>(placedColumn)
                    - static_cast<std::ptrdiff_t>(secondColumn);
            }
        }

        for (std::size_t line = anchor.value() + 1; line <= last; ++line)
        {
            const std::wstring_view text = lines.text(line);
            const std::size_t blanks = leadingBlanks(text);
            if (blanks == text.size())
                continue;
            const std::ptrdiff_t column = static_cast<std::ptrdiff_t>(columnAt(text, blanks));
            const std::size_t target = static_cast<std::size_t>(std::max<std::ptrdiff_t>(0,
                column + shift));
            std::wstring indent = indentText(target, unit);
            if (indent == text.substr(0, blanks))
                continue;
            changes.push_back({ line, blanks, std::move(indent) });
        }
        if (changes.empty())
            return std::nullopt;
        return editOf(lines, changes, { pasted.end(), 0 }, false);
    }

    std::optional<TextRange> unindentRange(const std::wstring_view line, const std::size_t caret,
        const std::size_t width)
    {
        if (caret == 0 || caret > line.size() || leadingBlanks(line) < caret)
            return std::nullopt;
        if (line[caret - 1] != L' ')
            return std::nullopt;

        const std::size_t target = previousStop(columnAt(line, caret), width);
        std::size_t from = caret;
        while (from != 0 && line[from - 1] == L' ' && columnAt(line, from) > target)
            --from;
        return TextRange{ from, caret - from };
    }

    std::size_t homeIndex(const std::wstring_view line, const std::size_t caret)
    {
        const std::size_t textStart = leadingBlanks(line);
        return caret == textStart ? 0 : textStart;
    }
}
