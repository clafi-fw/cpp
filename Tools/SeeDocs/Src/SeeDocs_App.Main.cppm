export module SeeDocs_App.Main;

import ClaFi.StdLib;

// NOTHING HERE NAMES A PLATFORM. What an entry point keeps is the signature its operating system
// asks for and the spelling of its arguments; the commands, their arguments and what they print
// stand here and run over any platform the framework has.
namespace SeeDocs_App
{
    export using Arguments = std::vector<std::wstring>;

    // Runs the command the arguments name and answers the process's exit code.
    //
    //     seedocs scan [tree] [--out file]    writes the database
    //     seedocs check [tree]                reports deviations, exit code 1 if any are errors
    export [[nodiscard]] int run(const Arguments&);
}
