export module ClaFi.Platform.Linux.ConsoleApplication;

import ClaFi.App.ConsoleApplication;

import ClaFi.StdLib;

namespace ClaFi
{
    // What a console application is built against on Linux: the platform needs no window system.
    export struct LinuxConsoleApplication : ConsoleApplication
    {
        using ConsoleApplication::ConsoleApplication;
    };
}
