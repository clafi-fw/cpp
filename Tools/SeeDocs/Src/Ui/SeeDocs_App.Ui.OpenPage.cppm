export module SeeDocs_App.Ui.OpenPage;

import SeeDocs_App.Ui.Workspace;

import ClaFi.App.Settings;

namespace SeeDocs_App::Ui
{
    // Fills the application menu's Open page: Browse, and the projects opened before, each
    // opening a project in a new tab. Built afresh as the menu opens.
    export void buildOpenPage(ClaFi::AppMenuPage&, Workspace&);
}
