export module SeeDocs_App.ProjectOptions;

import SeeDocs_App.Browser;
import SeeDocs_App.Workspace;

import ClaFi.App.Settings;

namespace SeeDocs_App
{
    // Fills a project's options page: its name to rename it by, its folder, when it was scanned
    // with the button that scans it again and the one that closes it; the folders of sources it
    // reads; how it reads them; and whether a hint edited on a page is written back.
    export void buildProjectOptions(ClaFi::AppMenuPage&, Workspace&, OpenProject&);
}
