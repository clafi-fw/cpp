export module ThisApp.Main;

import ThisApp.ThemesBrowser;

import ClaFi.Browser.Application;

import ClaFi.App.Application;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

// NOTHING HERE NAMES A PLATFORM OR A BACKEND. What an entry point keeps is what its operating
// system asks for and the two types the application is built out of; everything else - what the
// application is called, what its config holds, the main form and its size - stands here and runs
// over any platform the framework has.
namespace ThisApp
{
    using namespace ClaFi;

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams themesParams();

    // The theme browser over the platform an entry point names, and the GPU backend it names if
    // that platform has one. The name and both config schemas are supplied here, so an entry
    // point states nothing but the platform's own parameters.
    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend = void>
    class ThemesApplication : public Browser::BrowserApplication<PlatformType, GpuBackend, ThemesBrowser>
    {
    public:
        using Base = Browser::BrowserApplication<PlatformType, GpuBackend, ThemesBrowser>;
    public:
        explicit ThemesApplication(PlatformType::Params&&);
        // Builds the main form on the themes directory, runs it, writes the config, and answers
        // what the form answered.
        [[nodiscard]] int run();
    };

    // The choices every page reads. One answer serves the whole application, so they are named in
    // the root config section.
    [[nodiscard]] Dom::Dt::Section rootConfigBlueprint();
    // What a tab's entry keeps beyond what the browser puts there: what its icon is drawn from.
    [[nodiscard]] Dom::Dt::Section tabEntryBlueprint();
    // What a tab's page keeps: whether its preview is shown, and the theme it is editing.
    [[nodiscard]] Dom::Dt::Section tabConfigBlueprint();


//-----------------------------------------------------------------------------


    constexpr MinSize k_mainFormMinSize{ 900.0f, 600.0f };

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    ThemesApplication<PlatformType, GpuBackend>::ThemesApplication(typename PlatformType::Params&& platformParams)
        :
        Base{
            std::move(platformParams),
            themesParams(),
            rootConfigBlueprint(),
            tabEntryBlueprint(),
            tabConfigBlueprint()
        }
    {
    }

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    int ThemesApplication<PlatformType, GpuBackend>::run()
    {
        const std::unique_ptr<Form<ThemesBrowser>> form = this->createMainForm(
            this->metrics().primaryWindow,
            this->metrics().primaryWindowShadow,
            UiElement::Form,
            k_mainFormMinSize
        );
        return form->execute();
    }
}
