export module SeeDocs_App.Ui.Main;

import SeeDocs_App.Ui.Browser;
import SeeDocs_App.Ui.Workspace;

import ClaFi.Browser.Application;

import ClaFi.App.ApplicationProperties;
import ClaFi.App.WindowApplication;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace SeeDocs_App::Ui
{
    using namespace ::ClaFi;

    export [[nodiscard]] ApplicationProperties seeDocsParams();
    [[nodiscard]] Dom::Dt::Section rootConfigBlueprint();
    [[nodiscard]] Dom::Dt::Section tabEntryBlueprint();
    [[nodiscard]] Dom::Dt::Section tabConfigBlueprint();

    class MainForm : public SurfaceBrowser
    {
    public:
        explicit MainForm(const CreateParams& params);
    public:
        static void connectMenus();
        void start(WindowApplicationBase& application, const std::filesystem::path& stated);
    private:
        std::optional<Workspace> m_workspace{};
    };

    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend = void>
    class SeeDocsApplication
        : public Browser::BrowserApplication<PlatformType, GpuBackend, MainForm>
    {
    public:
        using Base = Browser::BrowserApplication<PlatformType, GpuBackend, MainForm>;
    public:
        explicit SeeDocsApplication(PlatformType::Params&& platformParams);
    public:
        [[nodiscard]] int run(const std::filesystem::path& stated);
    };

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    SeeDocsApplication<PlatformType, GpuBackend>::SeeDocsApplication(
        typename PlatformType::Params&& platformParams)
        :
        Base{
            std::move(platformParams),
            seeDocsParams(),
            rootConfigBlueprint(),
            tabEntryBlueprint(),
            tabConfigBlueprint()
        }
    {
        MainForm::connectMenus();
    }

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    int SeeDocsApplication<PlatformType, GpuBackend>::run(const std::filesystem::path& stated)
    {
        const std::unique_ptr<Form<MainForm>> form = this->createMainForm();
        form->start(*this, stated);
        return form->execute();
    }
}
