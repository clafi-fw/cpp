export module SeeDocs_App.Edits;

import SeeDocs_App.Notes;
import SeeDocs_App.Surface;

import ClaFi.StdLib;

// WORDS WRITTEN BACK WHERE THEY ARE READ FROM: a hint into the comment it was read off, a footnote
// into its note's section. Every write reads its file again and refuses where the file no longer
// holds what was read; a file keeps its byte order mark and each line its own line end; and what
// the surface and the notes hold afterwards is what the file now reads. See the README beside the
// project.
namespace SeeDocs_App
{
    // The widest a comment line or a footnote line is written.
    export constexpr std::size_t k_lineColumns = 100;

    // Why words were not written, in a sentence the user reads; empty where they were.
    export using Refusal = std::wstring;

    // Where a footnote not written yet goes, and what reaches it.
    export struct NewFootnote
    {
        Reference reference;   // the note and the anchor the comment's See names
        std::wstring after;    // the anchor of the section it follows; empty for the note's end
        std::wstring heading;
    };

    // Writes the hint into the comment the site names, the See it ends with kept: on the
    // declaration's line while the whole line fits, above it otherwise, and always above a type
    // that opens a scope. An empty hint with no See takes the comment out. The tree is the folder
    // the surface's files are named from.
    export [[nodiscard]] Refusal writeHint(Surface&, const std::filesystem::path& tree,
        const CommentSite&, std::wstring_view hint);

    // A section's lines as an editor shows them: each paragraph and each bullet on one line,
    // headings and code as written.
    export [[nodiscard]] std::wstring flowedText(const Lines&);
    // How many comments' See reaches the section.
    export [[nodiscard]] std::size_t referencesTo(const Surface&, const Reference&);
    // Writes flowed words as the section's body. A paragraph or a bullet that reads as it did
    // keeps its lines, and any other is broken at the line width.
    export [[nodiscard]] Refusal writeFootnote(Notes&, const Reference&, std::wstring_view flowed);

    // Where a footnote for the declaration goes in the note: after the section named, or at the
    // note's end where none is, headed by the declaration's name - or by the owner's and the
    // declaration's where the note already reaches the name. Nothing where the note cannot take
    // one, and the refusal says why.
    export [[nodiscard]] std::optional<NewFootnote> placeFootnote(Notes&, const CommentSite&,
        std::wstring_view note, std::wstring_view after, std::wstring_view owner, Refusal&);
    // Writes the new section, then the See that reaches it at the end of the declaration's
    // comment. Nothing is written where the comment could not take the See.
    export [[nodiscard]] Refusal addFootnote(Surface&, Notes&, const std::filesystem::path& tree,
        const CommentSite&, const NewFootnote&, std::wstring_view flowed);
}
