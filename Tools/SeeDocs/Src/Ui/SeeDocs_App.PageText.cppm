export module SeeDocs_App.PageText;

import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

// **A page's words as the text SeeDocs shows**: the engine's named styles for the runs and the
// prose, and a link carrying the name the page gave it.
namespace SeeDocs_App
{
    using namespace ClaFi;

    // Writes the runs as they are styled, a linked run inside a link.
    export void writeRuns(Text&, const Runs&);
    export [[nodiscard]] Text textOf(const Runs&);
    // The words of a text as a note line writes them: bold between its marks, code between its.
    export [[nodiscard]] std::wstring markdownOf(const Text&);
    // The prose of a note, a paragraph per line, the whole of it set in from the margin as asked.
    export void writeBlocks(Text&, const Blocks&, float indent = 0.0f);
    export [[nodiscard]] Text textOf(const Blocks&);
    // A footnote: the mark and the name it belongs to, the two standing on the anchor a link
    // reaches the footnote by, then the prose.
    export void writeFootnote(Text&, std::wstring_view anchor, std::wstring_view mark,
        const Runs& name, const Excerpt&);
    // A footnote whose section the page shows in full elsewhere: the mark and the name, where it
    // has one, then where the section stands - a link to it where a link is given.
    export void writeShownAt(Text&, std::wstring_view anchor, std::wstring_view mark,
        const Runs& name, const Runs& holder, std::wstring_view holderLink);
    // A declaration as code in its lexer's inks, a linked run a link where links are asked for.
    export void writeSignature(Text&, const Runs&, bool links, SourceLanguage);
}
