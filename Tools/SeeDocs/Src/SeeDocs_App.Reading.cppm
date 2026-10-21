export module SeeDocs_App.Reading;

import SeeDocs_App.Surface;

import ClaFi.StdLib;

// **What every reader of a tree shares**, whatever the language: which branch of a conditional it
// reads, and the words a comment gives the declaration it stands by.
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

    // One line of a comment cut in three: what stands before its words, the words, what after.
    export struct CommentLine
    {
        std::wstring lead;              // the code a trailing comment ends, the indent, the marks
        std::wstring words;
        std::wstring tail;              // the marks closing a block, and the blanks before them
        bool speaks{ true };            // false for a TODO, a separator, a mark standing alone
        bool opensParagraph{ false };   // the words start a paragraph of their own
    };

    export using CommentLines = std::vector<CommentLine>;

    // One line of a comment's words, and the line of the comment it stands for.
    export struct CommentTextLine
    {
        std::wstring words;
        std::size_t line{ 0 };   // npos for the empty line ending a paragraph
    };

    export using CommentTextLines = std::vector<CommentTextLine>;

    // A comment's lines as the file holds them, cut into what each says. The comment opens at the
    // column given on the first line, and on every other line it is the line's whole. See README
    export [[nodiscard]] CommentLines cutComment(std::span<const std::wstring_view> lines,
        std::size_t start, SourceLanguage, HintRules);
    // The words of the lines that speak, a line apiece, one empty line between two paragraphs.
    export [[nodiscard]] CommentTextLines textLinesOf(const CommentLines&);
    // The words of the lines that speak, a line apiece, a blank line between two paragraphs.
    export [[nodiscard]] std::wstring commentText(const CommentLines&);

    // Whether words opening a line of a comment start a part of it of their own: a doc command
    // such as @param, and by loose rules a doc tag such as <summary>.
    export [[nodiscard]] bool opensPart(std::wstring_view words, HintRules);
    // A comment line's lead without the brief's command or the summary's tag it may end with.
    export [[nodiscard]] std::wstring_view leadWithoutBrief(std::wstring_view lead);
    // How many blanks a text opens with, in characters.
    export [[nodiscard]] std::size_t leadingBlanks(std::wstring_view text);
    // The column a character of a line stands in, a tab reaching the next multiple of four.
    export [[nodiscard]] std::size_t columnOf(std::wstring_view line, std::size_t index);
    // The column the first character of a line that is no blank stands in.
    export [[nodiscard]] std::size_t indentOf(std::wstring_view line);

    // A comment line's words with its separators taken out: a run of three or more of one mark
    // - ---, ===, ***, ~~~, ###, ___, +++ - draws a line and says nothing.
    export [[nodiscard]] std::wstring withoutSeparators(std::wstring_view words);
}
