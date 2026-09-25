export module ClaFi.Core.TextEngine.History;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.RingBuffer;

import ClaFi.StdLib;

namespace ClaFi
{
    // Where the caret stood, so an undone step puts the user back. See TextEngine-Types
    export struct EditSelection
    {
        TextRange range{};
        bool caretOnLeft{};
        bool affinityTrailing{};
    };

    // What made an edit, which decides whether it joins the one before it. See TextEngine-Types
    export enum class EditKind
    {
        Replace, // a paste, or anything written over a selection. See TextEngine-Types
        Typing,
        DeletingBack,
        DeletingForward
    };

    // The undo history of one editable text. See TextEngine-Types
    export class TextHistory
    {
    public:
        // Replaces `range` with `inserted` and records the step. `before` is where the caret
        // stood, which is what undoing this step restores. Answers where the caret lands now:
        // collapsed at the end of what went in.
        [[nodiscard]] EditSelection apply(ControlText&, EditKind, const TextRange& range,
            const Text& inserted, const EditSelection& before);
        // Puts the text back the way the newest step in force found it, and answers where the
        // caret stood then. Empty when there is nothing to undo.
        [[nodiscard]] std::optional<EditSelection> undo(ControlText&);
        // Does the newest undone step over again, and answers where its caret lands.
        [[nodiscard]] std::optional<EditSelection> redo(ControlText&);
        [[nodiscard]] bool canUndo() const { return m_next != 0; }
        [[nodiscard]] bool canRedo() const { return m_next != m_records.size(); }
        // Ends the run the next edit could have joined. A run is what the user typed without
        // looking away, so a click, an arrow key or a focus change closes one even though the
        // text is untouched.
        void breakRun() { m_open = false; }
        void clear();
    private:
        // One step. `start` is where the replacement happened, `removed` is what stood there
        // and `inserted` is what took its place - so undo and redo are the same call with those
        // two the other way round.
        struct Record
        {
            EditKind kind{ EditKind::Replace };
            std::size_t start{ 0 };
            Text removed{};
            Text inserted{};
            EditSelection before{};
        };

        // Bounded by its own capacity: once the store holds k_maxRecords, the record that goes
        // in takes the place of the oldest one, which is the whole of how the depth is kept.
        using RecordCollection = RingBuffer<Record>;
    private:
        // Where the caret ends up once a record is in force: collapsed at the end of what went
        // in, keeping the side and the affinity the step started with. Derived rather than
        // stored, because that is all an edit leaves behind.
        [[nodiscard]] static EditSelection landing(const Record&);
        // Whether an edit of this shape continues the run the newest record holds.
        [[nodiscard]] bool joins(EditKind, std::size_t start, const Text& removed,
            const Text& inserted) const;
        static void mergeInto(Record&, std::size_t start, const Text& removed, const Text& inserted);
        // Whether the text is still the one the records were measured against.
        [[nodiscard]] bool inSync(const Text&) const;
        // Makes room for one more record, growing the store towards k_maxRecords. At that depth
        // the store stays full and the next record takes the oldest one's place instead.
        void reserveForRecord();
    private:
        // Deep enough that reaching the end takes a session's typing, and shallow enough that
        // the deltas of a whole session cost less than one copy of a large text.
        static constexpr std::size_t k_maxRecords = 512;
        // What the first edit reserves. The store is grown to that depth rather than taken at
        // it, because a TextHistory stands in every TextBox and most boxes are never typed
        // into: one that takes no edit holds no records and no block to keep them in.
        static constexpr std::size_t k_firstRecords = 16;
    private:
        RecordCollection m_records{ 0 };
        // How many records are in force. It is also the index of the next one to redo, so undo
        // and redo are one number moving along one list: no record is ever carried between two
        // stacks, and a fresh edit drops what was undone by truncating here.
        std::size_t m_next{ 0 };
        // Whether the newest record is still open to being joined by the edit that follows it.
        bool m_open{ false };
        // The plain length this history believes the text has - see the note on the class.
        std::size_t m_textSize{ 0 };
    };

}
