export module SeeDocs_App.OpenPage;

import SeeDocs_App.Workspace;

import ClaFi.App.Settings;

namespace SeeDocs_App
{
    // Fills the application menu's Open page: the project open, renamed and scanned again there, a
    // Browse button for another, and the projects opened before. Built afresh as the menu opens.
    export void buildOpenPage(ClaFi::OptionsPage&, Workspace&);
}
