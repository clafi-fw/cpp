export module SeeDocs_App.Studio.PageText;

import SeeDocs_App.Pages;

import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

// A PAGE AS THE TEXT THE STUDIO SHOWS: the engine's named styles for the blocks, tab stops for
// the tables, and a link carrying the name the page gave it.
namespace SeeDocs_App
{
    using namespace ClaFi;

    export [[nodiscard]] Text textOf(const Page&);
}
