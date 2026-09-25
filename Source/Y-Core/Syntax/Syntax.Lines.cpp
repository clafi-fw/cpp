module ClaFi.Core.Syntax.Lines;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    void LineStates::reset(const Language& language, const std::wstring_view text)
    {
        m_language = language;
        m_strings.clear();
        m_starts.clear();
        m_starts.push_back(0);
        for (std::size_t newline = text.find(L'\n'); newline != std::wstring_view::npos;
            newline = text.find(L'\n', newline + 1))
        {
            m_starts.push_back(newline + 1);
        }
        m_states.assign(m_starts.size(), LineState{});
        m_textLength = text.size();
        relexFrom(text, 0, lineCount());
    }

    void LineStates::applyEdit(const std::wstring_view textAfter, const TextRange& replaced,
        const std::size_t insertedLength)
    {
        if (m_textLength - replaced.length + insertedLength != textAfter.size())
            unreachable("An edit applied to the line states describes a text they do not hold.");

        const std::size_t firstLine = lineAt(replaced.start);
        const std::size_t lastLine = lineAt(replaced.end());

        // The lines the edit reached become one line plus one per newline the edit put in: every
        // newline those lines held stood inside the replaced range, and went with it.
        const std::wstring_view inserted = textAfter.substr(replaced.start, insertedLength);
        const std::size_t insertedNewlines
            = static_cast<std::size_t>(std::ranges::count(inserted, L'\n'));

        m_starts.erase(m_starts.begin() + firstLine + 1, m_starts.begin() + lastLine + 1);
        m_states.erase(m_states.begin() + firstLine + 1, m_states.begin() + lastLine + 1);
        m_starts.insert(m_starts.begin() + firstLine + 1, insertedNewlines, 0);
        m_states.insert(m_states.begin() + firstLine + 1, insertedNewlines, LineState{});

        std::size_t line = firstLine + 1;
        for (std::size_t newline = inserted.find(L'\n'); newline != std::wstring_view::npos;
            newline = inserted.find(L'\n', newline + 1))
        {
            m_starts[line] = replaced.start + newline + 1;
            ++line;
        }

        // Everything past the edit moved by what the edit changed the length by, and nothing else.
        const std::ptrdiff_t delta = static_cast<std::ptrdiff_t>(insertedLength)
            - static_cast<std::ptrdiff_t>(replaced.length);
        for (std::size_t i = line; i != m_starts.size(); ++i)
        {
            const std::ptrdiff_t moved = static_cast<std::ptrdiff_t>(m_starts[i]) + delta;
            m_starts[i] = static_cast<std::size_t>(moved);
        }

        m_textLength = textAfter.size();
        relexFrom(textAfter, firstLine, line);
    }

    void LineStates::tokensOf(const std::size_t line, const std::wstring_view lineText,
        Tokens& tokens)
    {
        if (line >= lineCount())
            unreachable("A line asked for its tokens past the lines the states hold.");

        tokens.clear();
        std::ignore = lexLine(m_language, lineText, m_states[line], m_strings, &tokens);
    }

    void LineStates::relexFrom(const std::wstring_view text, const std::size_t line,
        const std::size_t converged)
    {
        LineState state = m_states[line];
        for (std::size_t i = line; i != lineCount(); ++i)
        {
            state = lexLine(m_language, lineText(text, i), state, m_strings, nullptr);
            const std::size_t next = i + 1;
            if (next == lineCount())
                break;
            if (next >= converged && m_states[next] == state)
                break;
            m_states[next] = state;
        }
    }

    std::wstring_view LineStates::lineText(const std::wstring_view text,
        const std::size_t line) const
    {
        const std::size_t start = m_starts[line];
        const std::size_t end = line + 1 != lineCount() ? m_starts[line + 1] - 1 : text.size();
        return text.substr(start, end - start);
    }

    std::size_t LineStates::lineAt(const std::size_t pos) const
    {
        const Starts::const_iterator past = std::ranges::upper_bound(m_starts, pos);
        return static_cast<std::size_t>(past - m_starts.begin()) - 1;
    }
}
