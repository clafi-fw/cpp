export module SeeDocs_App.Studio.OpenPage;

import SeeDocs_App.Studio.Workspace;

import ClaFi.App.Settings;

namespace SeeDocs_App
{
    // Fills the application menu's Open page: the project open, a Browse button for another, and
    // the projects opened before, one row each. Built afresh every time the menu opens.
    export void buildOpenPage(ClaFi::OptionsPage&, Workspace&);
}
