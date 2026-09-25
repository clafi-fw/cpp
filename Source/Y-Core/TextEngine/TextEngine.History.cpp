module ClaFi.Core.TextEngine.History;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.RingBuffer;

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

    // TextHistory

    EditSelection TextHistory::apply(ControlText& text, EditKind kind, const TextRange& range,
        const Text& inserted, const EditSelection& before)
    {
        if (!inSync(text))
            clear();

        // Both calls below clamp the range against the text, so this is the position the edit
        // actually happened at rather than the one it was asked for at.
        const std::size_t start = std::min(range.start, text.plainText().size());
        Text removed = text.selectedText(range);
        text.replaceText(range, inserted);
        m_textSize = text.plainText().size();

        if (m_open && joins(kind, start, removed, inserted))
        {
            Record& record = m_records[m_next - 1];
            mergeInto(record, start, removed, inserted);
            return landing(record);
        }

        // What was undone is dropped. Those steps were measured against a text that took a
        // different turn from here, and no position in them names anything in this one.
        m_records.resize(m_next);

        reserveForRecord();
        m_records.push_back(Record{
            .kind = kind,
            .start = start,
            .removed = std::move(removed),
            .inserted = inserted,
            .before = before,
        });
        m_next = m_records.size();
        // A step that replaced a selection is a boundary the user drew, so nothing joins it.
        m_open = kind != EditKind::Replace;
        return landing(m_records.back());
    }

    std::optional<EditSelection> TextHistory::undo(ControlText& text)
    {
        m_open = false;
        if (!inSync(text))
            clear();
        if (!canUndo())
            return {};

        const Record& record = m_records[m_next - 1];
        const std::size_t insertedSize = record.inserted.plainText().size();
        text.replaceText({ record.start, insertedSize }, record.removed);
        m_textSize = text.plainText().size();
        --m_next;
        return record.before;
    }

    std::optional<EditSelection> TextHistory::redo(ControlText& text)
    {
        m_open = false;
        if (!inSync(text))
            clear();
        if (!canRedo())
            return {};

        const Record& record = m_records[m_next];
        const std::size_t removedSize = record.removed.plainText().size();
        text.replaceText({ record.start, removedSize }, record.inserted);
        m_textSize = text.plainText().size();
        ++m_next;
        return landing(record);
    }

    void TextHistory::clear()
    {
        // The records go, the block they stood in stays. A history is cleared because the text
        // it was measured against moved, which says nothing about whether the box is still
        // being edited - and it usually is.
        m_records.clear();
        m_next = 0;
        m_open = false;
        m_textSize = 0;
    }

    EditSelection TextHistory::landing(const Record& record)
    {
        const std::size_t caretPos = record.start + record.inserted.plainText().size();
        return {
            .range = { caretPos, 0 },
            .caretOnLeft = record.before.caretOnLeft,
            .affinityTrailing = record.before.affinityTrailing,
        };
    }

    bool TextHistory::joins(EditKind kind, std::size_t start, const Text& removed,
        const Text& inserted) const
    {
        const Record& last = m_records[m_next - 1];
        if (kind != last.kind)
            return false;

        const std::wstring& newRemoved = removed.plainText();
        const std::wstring& newInserted = inserted.plainText();
        const std::wstring& runRemoved = last.removed.plainText();
        const std::wstring& runInserted = last.inserted.plainText();

        switch (kind)
        {
        case EditKind::Typing:
            // The character went in where the run left off, and took nothing out on the way:
            // a selection written over is a boundary of the user's own.
            if (!newRemoved.empty() || newInserted.empty() || runInserted.empty())
                return false;
            if (start != last.start + runInserted.size())
                return false;
            return !endsRun(runInserted.back(), newInserted.front());

        case EditKind::DeletingBack:
            // The key eats towards the start of the text, so the run grows backwards and this
            // step ends exactly where the run so far begins.
            if (!newInserted.empty() || newRemoved.empty() || runRemoved.empty())
                return false;
            if (start + newRemoved.size() != last.start)
                return false;
            return !endsRun(runRemoved.front(), newRemoved.back());

        case EditKind::DeletingForward:
            // The key eats what follows the caret, and the caret stays where it is, so every
            // step of the run starts at the same position.
            if (!newInserted.empty() || newRemoved.empty() || runRemoved.empty())
                return false;
            if (start != last.start)
                return false;
            return !endsRun(runRemoved.back(), newRemoved.front());

        default:
            return false;
        }
    }

    void TextHistory::mergeInto(Record& record, std::size_t start, const Text& removed,
        const Text& inserted)
    {
        switch (record.kind)
        {
        case EditKind::Typing:
            record.inserted << inserted;
            break;

        case EditKind::DeletingBack:
            {
                // The run grows towards the start of the text: this step took out what sits
                // before everything the run has taken so far, so the record's start moves back
                // with it and what came out goes in front.
                Text merged = removed;
                merged << record.removed;
                record.removed = std::move(merged);
                record.start = start;
                break;
            }

        case EditKind::DeletingForward:
            record.removed << removed;
            break;

        default:
            unreachable("A replacing step was merged into, and joins() refuses to join one");
        }
    }

    bool TextHistory::inSync(const Text& text) const
    {
        // An empty history holds no position to be wrong about, so it is in step with any text.
        return m_records.empty() || text.plainText().size() == m_textSize;
    }

    void TextHistory::reserveForRecord()
    {
        if (m_records.size() != m_records.capacity())
            return;
        if (m_records.capacity() == k_maxRecords)
            return;

        m_records.set_capacity(std::min(k_maxRecords,
            std::max(k_firstRecords, m_records.capacity() * 2)));
    }

}
