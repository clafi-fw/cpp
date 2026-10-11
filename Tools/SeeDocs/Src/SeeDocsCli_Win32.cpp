import SeeDocs_App.Cli;

import ClaFi.StdLib;

// **The whole of what this project knows about Windows**: the entry point that takes its arguments
// as the wide strings the framework works in. The commands are in SeeDocs_App.Cli, beside the
// Linux entry point that names them the same way.
int wmain(int argc, wchar_t** argv)
{
    SeeDocs_App::Arguments arguments;
    for (int i = 0; i != argc; ++i)
        arguments.emplace_back(argv[i]);

    return SeeDocs_App::run(arguments);
}
