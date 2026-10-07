export module SeeDocs_App.Studio.Main;

import ClaFi.App.Application;

import ClaFi.StdLib;

// NOTHING HERE NAMES A PLATFORM OR A BACKEND. What an entry point keeps is what its operating
// system asks for and the two types the application is built out of; what the studio is called,
// where it finds the tree and what it builds into its window stand here and run over any
// platform the framework has.
namespace SeeDocs_App
{
    using namespace ClaFi;

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams studioParams();

    // The tree the studio reads: the folder stated, else the nearest folder holding Source at or
    // above the working directory, else at or above the executable. Empty where there is none.
    export [[nodiscard]] std::filesystem::path findTree(const std::filesystem::path& stated);

    // Builds the studio into a form of the application over the tree's database and notes, runs
    // it, and answers what the form answered.
    export int runStudio(ApplicationBase&, const std::filesystem::path& tree);
}
