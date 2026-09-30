export module ThisApp.Main;

import ThisApp.ScriptsBrowser;
import ThisApp.Scripts;
import ThisApp.Language;

import ClaFi.Documents.Application;

import ClaFi.App.Application;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine_Dt;

import ClaFi.StdLib;

// NOTHING HERE NAMES A PLATFORM OR A BACKEND. What an entry point keeps is what its operating
// system asks for and the two types the application is built out of; everything else - what the
// application is called, what its config holds, where its scripts stand - stands here and runs
// over any platform the framework has.
namespace ThisApp
{
    using namespace ClaFi;

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams scriptsParams();
    // Where the scripts stand: a folder of the application's name in the user's documents. Empty
    // where the platform names no documents folder.
    export [[nodiscard]] std::filesystem::path scriptsDirectory();

    // The scripts browser over the platform an entry point names, and the GPU backend it names if
    // that platform has one. The name and the config schemas are supplied here, so an entry point
    // states nothing but the platform's own parameters.
    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend = void>
    class ScriptsApplication
        : public Documents::DocumentsApplication<PlatformType, GpuBackend, ScriptsBrowser>
    {
    public:
        using Base = Documents::DocumentsApplication<PlatformType, GpuBackend, ScriptsBrowser>;
    public:
        explicit ScriptsApplication(PlatformType::Params&&);
        // Runs the browser over the scripts folder, and answers what the form answered.
        [[nodiscard]] int run();
    private:
        // Read once the base knows the config folder, and ahead of the folder that views it.
        Language m_language{ std::filesystem::path{ this->configFolder() } };
        // Built after the platform, which its watch stands on, and outliving every window: the
        // browser and its pages hold it by reference.
        ScriptsFolder m_scripts{ scriptsDirectory(), m_language };
    };

    // Nothing the application reads as one answer for every page.
    [[nodiscard]] Dom::Dt::Section rootConfigBlueprint();
    // Nothing a tab's entry keeps beyond what the browser puts there.
    [[nodiscard]] Dom::Dt::Section tabEntryBlueprint();
    // What a tab's page keeps: the script it is editing, as it stands on screen.
    [[nodiscard]] Dom::Dt::Section tabConfigBlueprint();


//-----------------------------------------------------------------------------


    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    ScriptsApplication<PlatformType, GpuBackend>::ScriptsApplication(
        typename PlatformType::Params&& platformParams)
        :
        Base{
            std::move(platformParams),
            scriptsParams(),
            rootConfigBlueprint(),
            tabEntryBlueprint(),
            tabConfigBlueprint()
        }
    {
    }

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    int ScriptsApplication<PlatformType, GpuBackend>::run()
    {
        return Base::run(m_scripts);
    }
}
