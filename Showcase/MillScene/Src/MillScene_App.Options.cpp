module MillScene_App.Options;

import ClaFi.PathArt.MillScene;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.Scenes;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;

import ClaFi.App.ApplicationProperties;
import ClaFi.App.CommandLine;

import ClaFi.Core.Context.UpdateCheck;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace MillScene_App
{
    namespace
    {
        // The whole number `text` holds, or nothing where it holds anything else.
        std::optional<int> integerOf(const std::wstring_view text)
        {
            const std::wstring copy{ text };
            wchar_t* end = nullptr;
            const long value = std::wcstol(copy.c_str(), &end, 10);
            if (end == copy.c_str() || *end != L'\0' || value < 1 || value > 100000)
                return std::nullopt;
            return static_cast<int>(value);
        }

        // The seconds `text` holds, or nothing where it holds anything else.
        std::optional<float> secondsOf(const std::wstring_view text)
        {
            const std::wstring copy{ text };
            wchar_t* end = nullptr;
            const float value = std::wcstof(copy.c_str(), &end);
            if (end == copy.c_str() || *end != L'\0' || !std::isfinite(value) || value < 0.0f)
                return std::nullopt;
            return value;
        }

        // A size written as the pixels across and down: 1280x720.
        std::optional<IntSize> sizeOf(const std::wstring_view text)
        {
            const std::size_t across = text.find(L'x');
            if (across == std::wstring_view::npos)
                return std::nullopt;
            const std::optional<int> width = integerOf(text.substr(0, across));
            const std::optional<int> height = integerOf(text.substr(across + 1));
            if (!width || !height)
                return std::nullopt;
            return IntSize{ *width, *height };
        }
    }

    ApplicationProperties millSceneProperties(const std::wstring_view name)
    {
        return {
            .name = Text{ name },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Shows the ClaFi path painter at work in a choice of scenes and themes"
            },
            .version = CLAFI_APP_VERSION,
            .updates = UpdateSource{
                .repository = L"clafi-fw/cpp",
                .tagPrefix = L"MillScene-v"
            }
        };
    }

    PathArt::Scene* sceneNamed(const std::wstring& name, PathArt::MillScene& mill,
        PathArt::SceneList& others)
    {
        if (mill.name() == name)
            return &mill;
        for (const std::unique_ptr<PathArt::Scene>& scene : others)
        {
            if (scene->name() == name)
                return scene.get();
        }
        return nullptr;
    }

    std::optional<std::size_t> themeNamed(const std::wstring& name)
    {
        std::size_t index = 0;
        for (const PathArt::SceneTheme& theme : PathArt::Themes::allThemes)
        {
            if (theme.name == name)
                return index;
            ++index;
        }
        return std::nullopt;
    }

    std::optional<LaunchOptions> launchOptionsOf(const CommandLine& commandLine,
        std::wstring& error)
    {
        LaunchOptions options{};
        if (const std::optional<std::wstring_view> scene = commandLine.value(L"--scene"))
            options.scene = *scene;
        if (const std::optional<std::wstring_view> theme = commandLine.value(L"--theme"))
            options.theme = *theme;

        if (const std::optional<std::wstring_view> at = commandLine.value(L"--at"))
        {
            const std::optional<float> seconds = secondsOf(*at);
            if (!seconds || *seconds >= static_cast<float>(std::numeric_limits<std::int64_t>::max()))
            {
                error = L"--at needs a non-negative number of seconds in range";
                return std::nullopt;
            }
            options.at = *seconds;
        }

        if (!options.scene.empty() || !options.theme.empty())
        {
            PathArt::MillScene mill{};
            PathArt::SceneList others{ PathArt::createScenes() };
            if (!options.scene.empty() && !sceneNamed(options.scene, mill, others))
            {
                error = L"no scene is offered under " + options.scene;
                return std::nullopt;
            }
        }
        if (!options.theme.empty() && !themeNamed(options.theme))
        {
            error = L"no theme is offered under " + options.theme;
            return std::nullopt;
        }
        return options;
    }

    std::optional<RenderOptions> renderOptionsOf(const CommandLine& commandLine,
        std::wstring& error)
    {
        RenderOptions options{};
        if (const std::optional<std::wstring_view> render = commandLine.value(L"--render"))
            options.render = *render;
        if (options.render.empty())
        {
            error = L"no file to render - name one with --render";
            return std::nullopt;
        }
        if (const std::optional<std::wstring_view> scene = commandLine.value(L"--scene"))
            options.scene = *scene;
        if (const std::optional<std::wstring_view> theme = commandLine.value(L"--theme"))
            options.theme = *theme;

        if (const std::optional<std::wstring_view> at = commandLine.value(L"--at"))
        {
            const std::optional<float> seconds = secondsOf(*at);
            if (!seconds)
            {
                error = L"--at needs a number of seconds";
                return std::nullopt;
            }
            options.at = *seconds;
        }
        if (const std::optional<std::wstring_view> every = commandLine.value(L"--every"))
        {
            const std::optional<float> seconds = secondsOf(*every);
            if (!seconds)
            {
                error = L"--every needs a number of seconds";
                return std::nullopt;
            }
            options.every = *seconds;
        }
        if (const std::optional<std::wstring_view> frames = commandLine.value(L"--frames"))
        {
            const std::optional<int> count = integerOf(*frames);
            if (!count)
            {
                error = L"--frames needs a count";
                return std::nullopt;
            }
            options.frames = *count;
        }
        if (const std::optional<std::wstring_view> size = commandLine.value(L"--size"))
        {
            const std::optional<IntSize> pixels = sizeOf(*size);
            if (!pixels)
            {
                error = L"--size needs the pixels as <width>x<height>";
                return std::nullopt;
            }
            options.size = *pixels;
        }
        return options;
    }

    PathArt::SceneTime sceneTimeAt(const float seconds, const std::chrono::seconds cycleDuration)
    {
        const float totalCycles = seconds / std::chrono::duration<float>(cycleDuration).count();
        const float wholeCycles = std::floor(totalCycles);
        return {
            .cycleTime = totalCycles - wholeCycles,
            .cyclesPassed = static_cast<std::int64_t>(wholeCycles),
        };
    }
}
