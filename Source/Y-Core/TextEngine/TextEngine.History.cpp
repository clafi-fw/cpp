module ClaFi.Core.TextEngine.History;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.StepHistory;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // Whether a run that has just produced `previous` ends before `next`. A run is one word,
    // and the break is where a whitespace character meets one that is not, taken in the order
    // the two were typed or removed. One statement of the rule, read by typing and by both
    // delete keys - a backspace run reads its characters in the order the key took them out.
    bool endsRun(wchar_t previous, wchar_t next)
    {
        const bool previousIsSpace = std::iswspace(previous) != 0;
        const bool nextIsSpace = std::iswspace(next) != 0;
        return previousIsSpace != nextIsSpace;
    }

    std::wstring countedText(const std::size_t count, const std::wstring_view one,
        const std::wstring_view many)
    {
        if (count == 1)
            return std::wstring{ one };
        return std::to_wstring(count).append(L" ").append(many);
    }

    // Blanks draw nothing, so a name says what they are and how many.
    std::wstring blanksText(const std::wstring_view text)
    {
        const std::size_t breaks = static_cast<std::size_t>(std::ranges::count(text, L'\n'));
        if (breaks != 0)
            return countedText(breaks, L"line break", L"line breaks");
        const std::size_t tabs = static_cast<std::size_t>(std::ranges::count(text, L'\t'));
        if (tabs == text.size())
            return countedText(tabs, L"tab", L"tabs");
        if (tabs == 0)
            return countedText(text.size(), L"space", L"spaces");
        return countedText(text.size(), L"blank", L"blanks");
    }

    // What a name quotes of a step's text: the first line of what it holds, cut at `maxChars`,
    // with an ellipsis where more follows.
    std::wstring quotedText(const std::wstring_view text, const std::size_t maxChars)
    {
        const auto isBlank = [](const wchar_t character) {
            return std::iswspace(character) != 0;
        };
        const auto first = std::ranges::find_if_not(text, isBlank);
        if (first == text.end())
            return blanksText(text);

        const auto last = std::ranges::find_if_not(text | std::views::reverse, isBlank).base();
        const std::wstring_view shown{ first, last };
        std::wstring_view line = shown.substr(0, shown.find(L'\n'));
        while (!line.empty() && isBlank(line.back()))
            line.remove_suffix(1);

        bool cut = line.size() != shown.size();
        if (line.size() > maxChars)
        {
            line = line.substr(0, maxChars);
            cut = true;
        }
        std::wstring result{ line };
        if (cut)
            result += L'\u2026'; // it's ellipsis
        return result;
    }

    // TextHistory

    EditSelection TextHistory::apply(ControlText& text, EditKind kind, const TextRange& range,
        const Text& inserted, const Text& what, const EditSelection& before,
        const std::optional<EditSelection>& after)
    {
        if (!inSync(text))
            clear();

        // Both calls below clamp the range against the text, so this is the position the edit
        // actually happened at rather than the one it was asked for at.
        const std::size_t start = std::min(range.start, text.plainText().size());
        Text removed = text.selectedText(range);
        text.replaceText(range, inserted);
        m_textSize = text.plainText().size();

        // AN AUTOMATIC EDIT IS PART OF THE STEP THAT CAUSED IT, and lands where it says: the
        // indent a line break brings, the block a paste is moved to. The step is closed to its
        // run from here, since undo takes its changes back in the reverse of the order they were
        // made, and a run growing past them would break that order.
        if (kind == EditKind::Automatic && m_takesAutomatic)
        {
            Record& record = *m_records.newestStep();
            const std::size_t end = start + inserted.plainText().size();
            record.automatic.push_back(Change{
                .start = start,
                .removed = std::move(removed),
                .inserted = inserted,
            });
            record.after = after.value_or(EditSelection{
                .range = { end, 0 },
                .caretOnLeft = record.before.caretOnLeft,
                .affinityTrailing = record.before.affinityTrailing,
            });
            m_records.closeRun();
            return record.after.value();
        }

        Record* open = m_records.openStep();
        if (open && !after.has_value() && joins(*open, kind, start, removed, inserted))
        {
            mergeInto(*open, start, removed, inserted);
            return landing(*open);
        }

        // A step that replaced a selection is a boundary the user drew, and one that states where
        // it lands is a step of its own, so nothing joins either.
        const bool opensRun = kind != EditKind::Replace && kind != EditKind::Automatic
            && !after.has_value();
        m_records.push(Record{
            .kind = kind,
            .change = {
                .start = start,
                .removed = std::move(removed),
                .inserted = inserted,
            },
            .what = what,
            .before = before,
            .after = after,
        }, opensRun);
        m_takesAutomatic = true;
        return landing(*m_records.newestStep());
    }

    std::optional<EditSelection> TextHistory::undo(ControlText& text, const std::size_t steps)
    {
        breakRun();
        if (!inSync(text))
            clear();
        if (steps == 0 || steps > m_records.undoDepth())
            return {};

        for (std::size_t i = 0; i != steps; ++i)
            revert(text, m_records.undoStep(i));
        const EditSelection result = m_records.undoStep(steps - 1).before;
        m_records.back(steps);
        m_textSize = text.plainText().size();
        return result;
    }

    std::optional<EditSelection> TextHistory::redo(ControlText& text, const std::size_t steps)
    {
        breakRun();
        if (!inSync(text))
            clear();
        if (steps == 0 || steps > m_records.redoDepth())
            return {};

        for (std::size_t i = 0; i != steps; ++i)
            reapply(text, m_records.redoStep(i));
        const EditSelection result = landing(m_records.redoStep(steps - 1));
        m_records.forward(steps);
        m_textSize = text.plainText().size();
        return result;
    }

    void TextHistory::writeUndoStep(const std::size_t i, Text& text) const
    {
        writeStep(m_records.undoStep(i), text);
    }

    void TextHistory::writeRedoStep(const std::size_t i, Text& text) const
    {
        writeStep(m_records.redoStep(i), text);
    }

    void TextHistory::breakRun()
    {
        m_records.closeRun();
        m_takesAutomatic = false;
    }

    void TextHistory::clear()
    {
        m_records.clear();
        m_takesAutomatic = false;
        m_textSize = 0;
    }

    EditSelection TextHistory::landing(const Record& record)
    {
        if (record.after.has_value())
            return record.after.value();
        const Change& change = record.change;
        const std::size_t caretPos = change.start + change.inserted.plainText().size();
        return {
            .range = { caretPos, 0 },
            .caretOnLeft = record.before.caretOnLeft,
            .affinityTrailing = record.before.affinityTrailing,
        };
    }

    bool TextHistory::joins(const Record& last, const EditKind kind, const std::size_t start,
        const Text& removed, const Text& inserted)
    {
        if (kind != last.kind)
            return false;

        const std::wstring& newRemoved = removed.plainText();
        const std::wstring& newInserted = inserted.plainText();
        const std::wstring& runRemoved = last.change.removed.plainText();
        const std::wstring& runInserted = last.change.inserted.plainText();

        switch (kind)
        {
        case EditKind::Typing:
            // The character went in where the run left off, and took nothing out on the way:
            // a selection written over is a boundary of the user's own.
            if (!newRemoved.empty() || newInserted.empty() || runInserted.empty())
                return false;
            if (start != last.change.start + runInserted.size())
                return false;
            return !endsRun(runInserted.back(), newInserted.front());

        case EditKind::DeletingBack:
            // The key eats towards the start of the text, so the run grows backwards and this
            // step ends exactly where the run so far begins.
            if (!newInserted.empty() || newRemoved.empty() || runRemoved.empty())
                return false;
            if (start + newRemoved.size() != last.change.start)
                return false;
            return !endsRun(runRemoved.front(), newRemoved.back());

        case EditKind::DeletingForward:
            // The key eats what follows the caret, and the caret stays where it is, so every
            // step of the run starts at the same position.
            if (!newInserted.empty() || newRemoved.empty() || runRemoved.empty())
                return false;
            if (start != last.change.start)
                return false;
            return !endsRun(runRemoved.back(), newRemoved.front());

        default:
            return false;
        }
    }

    void TextHistory::mergeInto(Record& record, const std::size_t start, const Text& removed,
        const Text& inserted)
    {
        switch (record.kind)
        {
        case EditKind::Typing:
            record.change.inserted << inserted;
            break;

        case EditKind::DeletingBack:
            {
                // The run grows towards the start of the text: this step took out what sits
                // before everything the run has taken so far, so the change's start moves back
                // with it and what came out goes in front.
                Text merged = removed;
                merged << record.change.removed;
                record.change.removed = std::move(merged);
                record.change.start = start;
                break;
            }

        case EditKind::DeletingForward:
            record.change.removed << removed;
            break;

        default:
            noReach("A step that joins no run was merged into");
        }
    }

    void TextHistory::revert(ControlText& text, const Record& record)
    {
        for (const Change& change : record.automatic | std::views::reverse)
            text.replaceText({ change.start, change.inserted.plainText().size() }, change.removed);
        const Change& change = record.change;
        text.replaceText({ change.start, change.inserted.plainText().size() }, change.removed);
    }

    void TextHistory::reapply(ControlText& text, const Record& record)
    {
        const Change& change = record.change;
        text.replaceText({ change.start, change.removed.plainText().size() }, change.inserted);
        for (const Change& automatic : record.automatic)
            text.replaceText({ automatic.start, automatic.removed.plainText().size() },
                automatic.inserted);
    }

    void TextHistory::writeStep(const Record& record, Text& text)
    {
        if (!record.what.empty())
        {
            text << record.what;
            return;
        }
        const bool typed = !record.change.inserted.empty();
        const Text& shown = typed ? record.change.inserted : record.change.removed;
        text << (typed ? L"Type " : L"Delete ") << InkWell::accentInk()
            << quotedText(shown.plainText(), k_quotedChars) << PopColor{};
    }

    bool TextHistory::inSync(const Text& text) const
    {
        // An empty history holds no position to be wrong about, so it is in step with any text.
        return m_records.empty() || text.plainText().size() == m_textSize;
    }
}
