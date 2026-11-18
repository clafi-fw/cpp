export module MillScene_App.Options;

export import ClaFi.App.CommandLine;
export import ClaFi.App.ApplicationProperties;

import ClaFi.PathArt.MillScene;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.Scenes;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

// **Nothing here names a window or a console.** Both applications read the same option tables and
// the same scene and theme names; what each does with them is its own.
namespace MillScene_App
{
    using namespace ::ClaFi;

    // What the window application opens on.
    export struct LaunchOptions
    {
        std::wstring scene; // the scene to open on, by the name it is offered under
        std::wstring theme; // the theme to open on, by its name in allThemes
    };

    // What the console application renders.
    export struct RenderOptions
    {
        std::wstring scene; // the scene to draw, the mill where none is named
        std::wstring theme; // the theme to draw it in, by its name in allThemes
        std::wstring render; // the PNG to write, in place of opening a window
        float at{ 0.0f }; // the story's second the first frame falls on
        int frames{ 1 }; // how many frames are written
        float every{ 0.1f }; // the story's seconds between them
        IntSize size{ 1280, 720 }; // the pixels each frame is written at
    };

    // The options the window application declares.
    export inline constexpr std::array<CommandLine::Option, 2> k_windowOptions{
        CommandLine::Option{
            .name = L"--scene",
            .valueName = L"<name>",
            .help = L"the scene to open on, by the name it is offered under"
        },
        CommandLine::Option{
            .name = L"--theme",
            .valueName = L"<name>",
            .help = L"the theme to open on, by the name it is offered under"
        }
    };

    // The options the console application declares.
    export inline constexpr std::array<CommandLine::Option, 7> k_renderOptions{
        CommandLine::Option{
            .name = L"--render",
            .valueName = L"<file.png>",
            .help = L"writes the frames as PNG files instead of opening a window"
        },
        CommandLine::Option{
            .name = L"--scene",
            .valueName = L"<name>",
            .help = L"the scene to draw, the mill where none is named"
        },
        CommandLine::Option{
            .name = L"--theme",
            .valueName = L"<name>",
            .help = L"the theme to draw it in, by the name it is offered under"
        },
        CommandLine::Option{
            .name = L"--at",
            .valueName = L"<seconds>",
            .help = L"the story's second the first frame falls on, 0 on its own"
        },
        CommandLine::Option{
            .name = L"--frames",
            .valueName = L"<count>",
            .help = L"how many frames are written, 1 on its own"
        },
        CommandLine::Option{
            .name = L"--every",
            .valueName = L"<seconds>",
            .help = L"the story's seconds between them, 0.1 on its own"
        },
        CommandLine::Option{
            .name = L"--size",
            .valueName = L"<width>x<height>",
            .help = L"the pixels each frame is written at, 1280x720 on its own"
        }
    };

    // What the application is called and what it states about itself, under either name.
    export [[nodiscard]] ApplicationProperties millSceneProperties(std::wstring_view name);
    // Reads the window application's launch pick, and names what it did not find.
    export [[nodiscard]] std::optional<LaunchOptions> launchOptionsOf(const CommandLine&,
        std::wstring& error);
    // Reads what the console application renders, and names what it did not find.
    export [[nodiscard]] std::optional<RenderOptions> renderOptionsOf(const CommandLine&,
        std::wstring& error);
    // The scene `name` is offered under, or nothing where no scene goes by it.
    export [[nodiscard]] PathArt::Scene* sceneNamed(const std::wstring& name,
        PathArt::MillScene& mill, PathArt::SceneList& others);
    // The index in allThemes that `name` is offered under, or nothing where none goes by it.
    export [[nodiscard]] std::optional<std::size_t> themeNamed(const std::wstring& name);
}
