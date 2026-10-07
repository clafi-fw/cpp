export module SeeDocs_App.Studio.Main;

import ClaFi.App.Application;

import ClaFi.Core.DomEngine_Dt;

import ClaFi.StdLib;

// NOTHING HERE NAMES A PLATFORM OR A BACKEND. What an entry point keeps is what its operating
// system asks for and the two types the application is built out of; what the studio is called,
// what it keeps and what it builds into its window stand here and run over any platform the
// framework has.
namespace SeeDocs_App
{
    using namespace ClaFi;

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams studioParams();

    // What the studio keeps of its own, handed to the application beside the framework's schema:
    // the project open and the projects opened before.
    export [[nodiscard]] Dom::Dt::Section createStudioConfigSchema();

    // Builds the studio into a form of the application, opens the project it starts on - the
    // folder stated, else the one it was last left on - runs the form, and answers what the form
    // answered.
    export int runStudio(ApplicationBase&, const std::filesystem::path& stated);
}
