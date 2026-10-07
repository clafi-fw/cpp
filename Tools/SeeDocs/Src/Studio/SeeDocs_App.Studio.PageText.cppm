export module SeeDocs_App.Studio.PageText;

import SeeDocs_App.Pages;

import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

// A PAGE'S WORDS AS THE TEXT THE STUDIO SHOWS: the engine's named styles for the runs and the
// prose, and a link carrying the name the page gave it.
namespace SeeDocs_App
{
    using namespace ClaFi;

    // Writes the runs as they are styled, a linked run inside a link.
    export void writeRuns(Text&, const Runs&);
    export [[nodiscard]] Text textOf(const Runs&);
    // The prose of a note, a paragraph per line.
    export [[nodiscard]] Text textOf(const Blocks&);
    // A footnote: the mark and the name it belongs to, the two standing on the anchor a link
    // reaches the footnote by, then the source and the prose.
    export void writeFootnote(Text&, std::wstring_view anchor, std::wstring_view mark,
        const Runs& name, const Excerpt&);
}
