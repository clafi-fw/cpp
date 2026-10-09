export module SeeDocs_App.Reading;

import ClaFi.StdLib;

// WHAT EVERY READER OF A TREE SHARES, whatever the language: which branch of a conditional it
// reads, and the hint a doc comment gives the declaration under it.
namespace SeeDocs_App
{
    // THE FIRST BRANCH OF A CONDITIONAL IS READ, and one whose condition is a plain 0 gives way
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

    // THE HINT A DOC COMMENT GIVES, its lines' words stripped of their marks: the paragraph a
    // @brief opens, or else the first sentence of the first paragraph of words. A paragraph ends
    // at a blank line and at a command - @param, \sa, @{.
    export [[nodiscard]] std::wstring briefOf(std::span<const std::wstring> lines);
}
