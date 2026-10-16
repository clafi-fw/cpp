export module SeeDocs_App.Cli;

import ClaFi.StdLib;

// **Nothing here names a platform.** What an entry point keeps is the signature its operating
// system asks for and the spelling of its arguments; the commands, their arguments and what they
// print stand here and run over any platform the framework has.
namespace SeeDocs_App
{
    export using Arguments = std::vector<std::wstring>;

    // Runs the command the arguments name and answers the process's exit code.
    //
    //     SeeDocsCli scan [folder] [--out file] [--time]  writes the database
    //     SeeDocsCli check [folder]                       reports deviations, exit code 1 on errors
    export [[nodiscard]] int run(const Arguments&);
}
