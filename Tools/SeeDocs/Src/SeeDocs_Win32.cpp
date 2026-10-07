import SeeDocs_App.Main;

import ClaFi.StdLib;

// THE WHOLE OF WHAT THIS PROJECT KNOWS ABOUT WINDOWS: the entry point that takes its arguments
// as the wide strings the framework works in. The commands are in SeeDocs_App.Main, beside the
// Linux entry point that names them the same way.
int wmain(int argc, wchar_t** argv)
{
    SeeDocs_App::Arguments arguments;
    for (int i = 0; i != argc; ++i)
        arguments.emplace_back(argv[i]);

    return SeeDocs_App::run(arguments);
}
