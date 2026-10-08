export module SeeDocs_App.Studio.Main;

import SeeDocs_App.Studio.Browser;

import ClaFi.Browser.Application;

import ClaFi.App.Application;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

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

    // The studio over the platform an entry point names, and the GPU backend it names if that
    // platform has one. The name and the config schemas are supplied here, so an entry point
    // states nothing but the platform's own parameters.
    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend = void>
    class StudioApplication
        : public Browser::BrowserApplication<PlatformType, GpuBackend, SurfaceBrowser>
    {
    public:
        using Base = Browser::BrowserApplication<PlatformType, GpuBackend, SurfaceBrowser>;
    public:
        explicit StudioApplication(PlatformType::Params&&);
        // Builds the studio's window, opens the project it starts on - the folder stated, else
        // the one it was last left on - runs the window, and answers what it answered.
        [[nodiscard]] int run(const std::filesystem::path& stated);
    };

    // What the studio keeps of its own, beside the browser's schema: the project open and the
    // projects opened before.
    [[nodiscard]] Dom::Dt::Section rootConfigBlueprint();
    // Nothing a tab's entry keeps beyond what the browser puts there.
    [[nodiscard]] Dom::Dt::Section tabEntryBlueprint();
    // Nothing a tab's page keeps of its own.
    [[nodiscard]] Dom::Dt::Section tabConfigBlueprint();
    // Runs the studio in the browser's form: the workspace over it, the Open page connected and
    // the project started. Answers what the form answered.
    [[nodiscard]] int runStudio(ApplicationBase&, Form<SurfaceBrowser>&,
        const std::filesystem::path& stated);

    constexpr float k_minWidth = 820.0f;
    constexpr float k_minHeight = 520.0f;
    constexpr float k_preferredWidth = 1360.0f;
    constexpr float k_preferredHeight = 880.0f;


//-----------------------------------------------------------------------------


    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    StudioApplication<PlatformType, GpuBackend>::StudioApplication(
        typename PlatformType::Params&& platformParams)
        :
        Base{
            std::move(platformParams),
            studioParams(),
            rootConfigBlueprint(),
            tabEntryBlueprint(),
            tabConfigBlueprint()
        }
    {
    }

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    int StudioApplication<PlatformType, GpuBackend>::run(const std::filesystem::path& stated)
    {
        const std::unique_ptr<Form<SurfaceBrowser>> form = this->createMainForm(
            FormPlacement::Default,
            this->metrics().primaryWindow,
            this->metrics().primaryWindowShadow,
            UiElement::ToolBar,
            Border{ Thickness::Thin },
            Padding{ 0.0f },
            Spacing{ 0.0f },
            MinSize{ k_minWidth, k_minHeight },
            PreferredSize{ k_preferredWidth, k_preferredHeight }
        );
        return runStudio(*this, *form, stated);
    }
}
