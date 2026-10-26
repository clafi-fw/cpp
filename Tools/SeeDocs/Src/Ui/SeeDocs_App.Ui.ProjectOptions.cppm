export module SeeDocs_App.Ui.ProjectOptions;

import SeeDocs_App.Ui.Browser;
import SeeDocs_App.Ui.Workspace;

import ClaFi.App.Settings;

namespace SeeDocs_App::Ui
{
    // Fills a project's options page: its name to rename it by, its folder, when it was scanned
    // with the button that scans it again and the one that closes it; the folders of sources it
    // reads; how it reads them; and whether a hint edited on a page is written back.
    export void buildProjectOptions(ClaFi::AppMenuPage&, Workspace&, OpenProject&);
}
