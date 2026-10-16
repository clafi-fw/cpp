export module SeeDocs_App.OpenPage;

import SeeDocs_App.Workspace;

import ClaFi.App.Settings;

namespace SeeDocs_App
{
    // Fills the application menu's Open page: Browse, and the projects opened before, each
    // opening a project in a new tab. Built afresh as the menu opens.
    export void buildOpenPage(ClaFi::AppMenuPage&, Workspace&);
}
