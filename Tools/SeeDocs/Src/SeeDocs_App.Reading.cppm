export module SeeDocs_App.Reading;

import ClaFi.StdLib;

// **What every reader of a tree shares**, whatever the language: which branch of a conditional it
// reads, and the hint a doc comment gives the declaration under it.
namespace SeeDocs_App
{
    // **The first branch of a conditional is read**, and one whose condition is a plain 0 gives way
    // to the next; the branches after the one read are left out. A library declares the same
    // things in every branch as a rule, and reading one keeps the brackets the branches open
    // balanced.
    export class Branches
    {
    public:
        // A conditional's word - if, ifdef, ifndef, elif, elifdef, elifndef, else, endif - and
        // its condition; any other word is no conditional.
        void take(std::wstring_view word, std::wstring_view condition);
        [[nodiscard]] bool reading() const;
    private:
        struct Frame
        {
            bool outerReading{ true };
            bool reading{ true };
            bool taken{ false };
        };
    private:
        std::vector<Frame> m_frames;
    };

    // **The hint a doc comment gives**, its lines' words stripped of their marks: the paragraph a
    // @brief opens, or else the first sentence of the first paragraph of words. A paragraph ends
    // at a blank line and at a command - @param, \sa, @{.
    export [[nodiscard]] std::wstring briefOf(std::span<const std::wstring> lines);

    // The column a character of a line stands in, a tab reaching the next multiple of four.
    export [[nodiscard]] std::size_t columnOf(std::wstring_view line, std::size_t index);
    // The column the first character of a line that is no blank stands in.
    export [[nodiscard]] std::size_t indentOf(std::wstring_view line);

    // A comment line's words with its separators taken out: a run of three or more of one mark
    // - ---, ===, ***, ~~~, ###, ___, +++ - draws a line and says nothing.
    export [[nodiscard]] std::wstring withoutSeparators(std::wstring_view words);
}
