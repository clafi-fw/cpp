module;
#include <cstdio>
#include <fstream>

export module MillScene_App.Render;

import MillScene_App.Options;

import ClaFi.PathArt.MillScene;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.Scenes;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Cpu.Canvas;
import ClaFi.Core.Graphics.Png;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace MillScene_App
{
    using namespace ::ClaFi;

    // Draws the frames the options name, each into its own PNG; answers the process's exit code.
    export [[nodiscard]] int renderLaunchFrames(const RenderOptions&);

    namespace
    {
        // The file a frame goes to: the path itself where there is one frame, and the frame's
        // number before its extension where there are more.
        std::filesystem::path framePath(const std::wstring& render, const int frame,
            const int frames)
        {
            if (frames == 1)
                return render;
            const std::filesystem::path given{ render };
            const std::wstring numbered = given.stem().wstring() + L"-" + std::to_wstring(frame)
                + given.extension().wstring();
            return given.parent_path() / numbered;
        }

        // What went wrong, where whoever reads the files will see it: beside them, on the error
        // stream, rather than in a window that a render never opens.
        void report(const std::wstring& text)
        {
            std::fprintf(stderr, "%ls\n", text.c_str());
        }

        // **A render that fails says so beside the files it did not write.** A windowed process is
        // given no console of its own, so the error stream is not where whoever asked for the
        // frames is looking.
        void reportRenderFailure(const RenderOptions& options, const std::wstring& text)
        {
            report(text);
            // A path, and not the wide string the file system will not take from every library.
            const std::filesystem::path failures{
                std::filesystem::path{ options.render }.wstring() + L".error.txt" };
            std::ofstream file{ failures };
            file << toUtf8(text) << '\n';
        }
    }

    // Draws the frames themselves. The exported renderLaunchFrames guards it, so that what a scene
    // throws lands beside the files rather than ending the process without a word.
    int renderFrames(const RenderOptions& options)
    {
        PathArt::MillScene mill{};
        PathArt::SceneList others{ PathArt::createScenes() };
        // A render that names no scene is the mill, the scene the showcase opens on.
        PathArt::Scene* scene = options.scene.empty()
            ? &mill
            : sceneNamed(options.scene, mill, others);
        if (!scene)
        {
            reportRenderFailure(options, L"no scene is offered under " + options.scene);
            return 1;
        }
        if (scene->offersThemes())
        {
            std::size_t themeIndex = 0;
            if (!options.theme.empty())
            {
                const std::optional<std::size_t> theme = themeNamed(options.theme);
                if (!theme)
                {
                    reportRenderFailure(options, L"no theme is offered under " + options.theme);
                    return 1;
                }
                themeIndex = *theme;
            }
            // A render has no timer to land a crossfade with, so the theme is worn at once.
            scene->wearTheme(themeIndex);
        }
        else if (!options.theme.empty())
        {
            reportRenderFailure(options, std::wstring{ scene->name() } + L" offers no themes");
            return 1;
        }

        Graphics::Canvas canvas{ std::make_unique<Graphics::Cpu::CpuBackend>(options.size) };
        const FloatRect bounds = {
            0.0f,
            0.0f,
            static_cast<float>(options.size.x),
            static_cast<float>(options.size.y)
        };
        const PathArt::SceneParams params = {
            .bounds = bounds,
            .transform = canvas.transform(),
            .backend = Graphics::BackendType::Cpu
        };
        scene->prepare(params);

        const std::chrono::seconds cycleDuration = scene->cycleDuration();
        const IntRect whole = { 0, 0, options.size.x, options.size.y };
        for (int frame = 0; frame != options.frames; ++frame)
        {
            const float seconds = options.at + static_cast<float>(frame) * options.every;
            const PathArt::SceneTime time = sceneTimeAt(seconds, cycleDuration);

            Graphics::Bitmap* drawn = nullptr;
            canvas.beginPaint(nullptr, whole);
            scene->draw(canvas, bounds, time);
            canvas.endPaint(nullptr, whole, drawn);
            if (!drawn)
            {
                reportRenderFailure(options, L"the scene drew no pixels");
                return 1;
            }

            const std::optional<std::string> png = Graphics::encodePng(*drawn);
            if (!png)
            {
                reportRenderFailure(options, L"the pixels could not be written as a PNG");
                return 1;
            }

            const std::filesystem::path path = framePath(options.render, frame, options.frames);
            std::ofstream file{ path, std::ios::binary };
            file.write(png->data(), static_cast<std::streamsize>(png->size()));
            if (!file)
            {
                reportRenderFailure(options, L"cannot write " + path.wstring());
                return 1;
            }
        }
        return 0;
    }

    int renderLaunchFrames(const RenderOptions& options)
    {
        // **What is thrown lands beside the files**, where whoever asked for them is looking: a
        // render that ended the process without a word would be read as having written nothing.
        try
        {
            return renderFrames(options);
        }
        catch (const std::exception& error)
        {
            reportRenderFailure(options, L"the render could not go on: " + fromUtf8(error.what()));
            return 1;
        }
    }
}
