export module ClaFi.Core.TextEngine.History;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.StepHistory;
import ClaFi.Core.System.UiTypes;

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
        DeletingForward,
        Automatic // made on account of the edit before it. See TextEngine-Types
    };

    // The undo history of one editable text. See TextEngine-Types
    export class TextHistory
    {
    public:
        // Replaces `range` with `inserted` and records the step. `before` is where the caret
        // stood, which is what undoing this step restores. Answers where the caret lands now:
        // on `after` where one is stated - a step stating one joins no run, and none joins it -
        // and otherwise collapsed at the end of what went in. `what` names the step, and a step
        // left unnamed is called after what it did to the text.
        [[nodiscard]] EditSelection apply(ControlText&, EditKind, const TextRange& range,
            const Text& inserted, const Text& what, const EditSelection& before,
            const std::optional<EditSelection>& after = std::nullopt);
        // Puts the text back the way that many steps found it, and answers where the caret stood
        // before the oldest of them. Empty where the history does not reach.
        [[nodiscard]] std::optional<EditSelection> undo(ControlText&, std::size_t steps);
        // Does that many undone steps over again, and answers where the newest one's caret lands.
        [[nodiscard]] std::optional<EditSelection> redo(ControlText&, std::size_t steps);
        [[nodiscard]] std::size_t undoDepth() const { return m_records.undoDepth(); }
        [[nodiscard]] std::size_t redoDepth() const { return m_records.redoDepth(); }
        void writeUndoStep(std::size_t i, Text&) const; // the newest at 0
        void writeRedoStep(std::size_t i, Text&) const; // the nearest at 0
        // Ends the run the next edit could have joined, and the step an automatic edit could.
        // A run is what the user typed without looking away, so a click, an arrow key or a
        // focus change closes one even though the text is untouched.
        void breakRun();
        void clear();
    private:
        // One replacement: at `start`, `removed` went out and `inserted` took its place - so undo
        // and redo are the same call with those two the other way round.
        struct Change
        {
            std::size_t start{ 0 };
            Text removed{};
            Text inserted{};
        };

        using Changes = std::vector<Change>;

        // One step: the edit that made it, and the automatic ones made on its account.
        struct Record
        {
            EditKind kind{ EditKind::Replace };
            Change change{};
            Changes automatic{};   // in the order they were made
            Text what{};           // the step's name, where its caller gave one
            EditSelection before{};
            std::optional<EditSelection> after{};   // where a redo lands, where the step stated it
        };
    private:
        // Deep enough that reaching the end takes a session's typing, and shallow enough that
        // the deltas of a whole session cost less than one copy of a large text.
        static constexpr std::size_t k_maxRecords = 512;
        // What the first edit reserves. The store is grown to that depth rather than taken at
        // it, because a TextHistory stands in every TextBox and most boxes are never typed
        // into: one that takes no edit holds no records and no block to keep them in.
        static constexpr std::size_t k_firstRecords = 16;
        // How much of a step's text its name quotes.
        static constexpr std::size_t k_quotedChars = 24;
    private:
        using Records = StepHistory<Record, k_firstRecords, k_maxRecords>;
    private:
        // Where the caret ends up once a record is in force: where the step stated it, and
        // otherwise collapsed at the end of what went in, keeping the side and the affinity the
        // step started with - derived rather than stored, since that is all an edit leaves.
        [[nodiscard]] static EditSelection landing(const Record&);
        // Whether an edit of this shape continues the run the record holds.
        [[nodiscard]] static bool joins(const Record&, EditKind, std::size_t start,
            const Text& removed, const Text& inserted);
        static void mergeInto(Record&, std::size_t start, const Text& removed,
            const Text& inserted);
        static void revert(ControlText&, const Record&);
        static void reapply(ControlText&, const Record&);
        // The step's name, or what it did to the text. See TextEngine-Types#step-names
        static void writeStep(const Record&, Text&);
        // Whether the text is still the one the records were measured against.
        [[nodiscard]] bool inSync(const Text&) const;
    private:
        Records m_records{};
        // Whether the newest record is the step the last edit made, which an automatic edit joins.
        bool m_takesAutomatic{ false };
        // The plain length this history believes the text has - see the note on the class.
        std::size_t m_textSize{ 0 };
    };
}
