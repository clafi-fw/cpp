export module ClaFi.App.Application;

export import ClaFi.Dom;

import ClaFi.App.AppMenu;
import ClaFi.App.Themes;
import ClaFi.Application.ThemesManager;

import ClaFi.StdActions.Transfer;
import ClaFi.StdActions;

import ClaFi.Core.System.UiTypes;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.TextEngine.Text;

import ClaFi.Diagnostic.Log;

import ClaFi.StdLib;

namespace ClaFi
{
    export struct AppParams
    {
        Text name;
        std::wstring_view publisher;
        Text description{}; // what the application does, shown on its Information page
    };

    export class ApplicationBase
    {
    public:
        ApplicationBase(Platform& platform, const AppParams&, BackendFactory, Dom::Dt::Section&&);
        virtual ~ApplicationBase() = default;
        ApplicationBase(const ApplicationBase&) = delete;
    public:
        AppContext& context() { return m_context; }
        [[nodiscard]] const Text& name() const { return m_context.appName(); }
        [[nodiscard]] AppTheme& theme() { return m_context.theme(); };
        [[nodiscard]] const AppTheme& theme() const { return m_context.theme(); };
        [[nodiscard]] ThemeColors& themeColors() { return m_context.themeColors(); }
        [[nodiscard]] const ThemeColors& themeColors() const { return m_context.themeColors(); }
        [[nodiscard]] ThemeMetrics& metrics() { return m_context.themeMetrics(); }
        [[nodiscard]] const ThemeMetrics& metrics() const { return m_context.themeMetrics(); }
        //
        [[nodiscard]] std::wstring_view publisher() const { return m_context.publisher(); }
        void ensureConfigFolderExists() { m_context.ensureConfigFolder(); }
        void deleteConfigFolder() { m_context.deleteConfigFolder(); }
        [[nodiscard]] bool configFolderExists() const { return m_context.configFolderExists(); }
        const std::wstring_view configFolder() { return m_configFolder; }
        const std::wstring_view configSysPath() { return m_configSysPath; }
        const std::wstring_view configPublisherPath() { return m_configPublisherPath; }
        // Where the publisher's applications keep what they share. See Application
        const std::wstring_view configSharePath() { return m_configSharePath; }
        const std::wstring_view configSubPath() { return m_configSubPath; }
        const std::wstring& configFileName() const { return m_configFileName; }
        const std::wstring& themesPath() const { return m_themesPath; }
        // The themes this application can wear. One per application, and what appThemes answers.
        [[nodiscard]] ThemesManager& themes() { return m_themesManager; }
        void saveConfig(bool evenIfNotExists);
        Dom::DocumentBase& config() { return m_context.config(); }
        // Builds a dialog window and its root. The root is offered Interactivity::ActiveContainer
        // and may name its own instead - see the definition.
        template<ClassOfFormControl ControlClass, typename... Args>
        std::unique_ptr<Form<ControlClass>> createDialog(Args&&... args);
    protected:
        // initialize and finalize are called from derived Application<Platform, Backend>
        // constructor and destructor, to make sure the Platform is in valid state
        // during that calls
        void initialize();
        // Runs once. An application whose own members the exit save reaches - a BrowserApplication
        // and its settings - calls it from its destructor, while they stand; the call from
        // ~Application then finds it done.
        void finalize();
    private:
        std::filesystem::path& initializeConfigPath(const AppParams&);
    private:
        bool m_finalized{ false };
        // --- Paths ---
        std::wstring m_configSysPath;
        std::wstring m_configPublisherPath;
        std::wstring m_configSharePath;
        std::wstring m_configSubPath;
        std::wstring m_configFolder;
        std::wstring m_configFileName;
        std::filesystem::path m_configFilePath;
        std::wstring m_themesPath{};
        //
        AppContext m_context;
        // Built over the path above, which initializeConfigPath has filled in by the time the
        // context is constructed.
        ThemesManager m_themesManager{ m_themesPath };
    };

    template<typename T>
        concept IsApplication = std::derived_from<T, ApplicationBase>;

    // THE PLATFORM STANDS FIRST AND FALLS LAST. Held in a base ahead of ApplicationBase rather
    // than as a member behind it, so the context, the themes manager and every window are built
    // on a standing platform and go down on one. See Application
    template <IsPlatform PlatformType>
    class PlatformHolder
    {
    protected:
        explicit PlatformHolder(const typename PlatformType::Params& params);
    protected:
        PlatformType m_platform;
    };

    // THE GPU BACKEND IS THE APPLICATION'S TO NAME, and naming none is an answer. The CPU backend
    // is the core's own and every application has it, so Application<Win32Platform> draws on the
    // CPU and offers nothing, while Application<Win32Platform, Direct2DBackend> carries both and
    // starts on whichever the config holds. Nothing in the core depends on which. See Context
    export template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend = void>
    class Application : private PlatformHolder<PlatformType>, public ApplicationBase
    {
    public:
        Application(PlatformType::Params&& platformParams, const AppParams&, Dom::Dt::Section&&);
        ~Application() override;
    };

//-----------------------------------------------------------------------------

    using namespace Dom::Dt;

    // What stands between the folder Platform::appDataPath answers and the names the application
    // adds under it. The file system reads the result back, so the separator is its own.
    constexpr wchar_t k_pathSeparator{ std::filesystem::path::preferred_separator };

    // ApplicationBase

    ApplicationBase::ApplicationBase(Platform& platform, const AppParams& appParams, BackendFactory createBackend, Section&& schema)
        :
        m_context{
            platform,
            appParams.name,
            appParams.publisher,
            appParams.description,
            initializeConfigPath(appParams),
            createBackend,
            Section{
                Section{
                    AppContext::k_formsSectionName,
                    Platform::createFormsConfigSchema({
                        AppContext::k_mainFormName,
                        k_diagnosticFormName
                    })
                },
                createAppMenuConfigSchema(),
                createDiagnosticLogConfigSchema(),
                std::forward<Section>(schema)
            }
        }
    {
    }

    void ApplicationBase::saveConfig(bool evenIfNotExists)
    {
        if (!(configFolderExists() || evenIfNotExists))
            return;
        ensureConfigFolderExists();
        // The diagnostic window is up for as long as the application, so what it stands at is
        // read here, on the way to the file, rather than on a hide that never comes.
        storeDiagnosticLogState();
        config().save();
    }

    void ApplicationBase::initialize()
    {
        // FIRST, before any window exists: the identity a toplevel is made under.
        Platform::setApplicationId(publisher(), name().plainText());
        // The standard commands, in the application's shortcut scope. Every application has
        // them from here; one that wants a key for itself clears that action's shortcut.
        StdActions::registerAll();
        // The framework's own clipboard formats, in the Transfer tables. Registered by a call
        // rather than at static initialisation: a static library drops a translation unit nothing
        // references, and the tables would be filled in some builds and empty in others.
        StdActions::registerTransferFormats(m_context.platform().clipboard());
        // The application menu under every AppButton.
        connectAppMenu();
        // Built before the config is read, so a line logged while reading it has somewhere to
        // go; shown after, because whether it is shown is in the config.
        initializeDiagnosticLog(m_context);
        for (const std::wstring& line : m_context.platform().diagnosticLines())
            diagnosticLog(line);
        // BEFORE THE CONFIG IS READ, so that the theme the file names is carried in by the load
        // itself rather than applied a second time after it. No window exists yet, so what the
        // node states is what the first one opens in - see AppContext::takeTheme.
        connectAppThemes(m_context, m_themesManager);
        // Every write of the config is on record, whoever asked for it.
        config().connectEvent([](Dom::SaveEvent& event) {
            diagnosticLog(std::format(L"config written {}", event.sender().path().wstring()));
        });
        if (configFolderExists())
        {
            const bool read = config().load();
            diagnosticLog(std::format(
                L"config {} {}", read ? L"read" : L"absent", config().path().wstring()));
        }
        restoreDiagnosticLogState();
    }

    // WHAT THE USER ALLOWED TO BE KEPT IS KEPT, whether or not the application saves for itself.
    // Keep settings on this PC is the framework's offer, so honouring it is the framework's work;
    // an application that wants its config written earlier still saves it itself. Nothing is
    // written where no folder was allowed - that is what saveConfig(false) answers.
    //
    // Ahead of the diagnostic window going, because saveConfig reads the state that window is
    // standing in.
    void ApplicationBase::finalize()
    {
        if (m_finalized)
            return;
        m_finalized = true;
        saveConfig(false);
        finalizeDiagnosticLog();
    }

    std::filesystem::path& ApplicationBase::initializeConfigPath(const AppParams& appParams)
    {
        std::wstring sysPath = Platform::appDataPath();
        if (!sysPath.empty())
        {
            std::wstring subPath{ appParams.publisher };
            if (!subPath.empty())
                subPath.push_back(k_pathSeparator);
            subPath.append(appParams.name.plainText());

            m_configSysPath = sysPath;
            m_configSubPath = subPath;
            m_configPublisherPath.reserve(sysPath.size() + appParams.publisher.size() + 1ull);
            m_configPublisherPath = sysPath;
            if (!appParams.publisher.empty())
            {
                m_configPublisherPath.append(appParams.publisher);
                m_configPublisherPath.push_back(k_pathSeparator);
            }
            m_configFolder.reserve(m_configSysPath.size() + m_configSubPath.size());
            m_configFolder = m_configSysPath + m_configSubPath;

            constexpr std::wstring_view k_fileName = L"Settings";
            m_configFileName.reserve(configFolder().size() + 1ull + k_fileName.size());
            m_configFileName = configFolder();
            m_configFileName.push_back(k_pathSeparator);
            m_configFileName.append(k_fileName);
        }
        m_configFilePath = m_configFileName + L".cfg";

        // WHAT THE PUBLISHER'S APPLICATIONS SHARE STANDS UNDER Share, apart from their own
        // folders, so an application named after one of them takes no directory it shares.
        constexpr std::wstring_view k_shareFolderName = L"Share";
        constexpr std::wstring_view k_themesFolderName = L"Themes";
        m_configSharePath = m_configPublisherPath;
        m_configSharePath.append(k_shareFolderName);
        m_configSharePath.push_back(k_pathSeparator);
        m_themesPath = m_configSharePath;
        m_themesPath.append(k_themesFolderName);
        return m_configFilePath;
    }

    // ApplicationBase

    template <ClassOfFormControl ControlClass, typename ... Args>
    std::unique_ptr<Form<ControlClass>> ApplicationBase::createDialog(Args&&... args)
    {
        // WHAT A DIALOG'S ROOT IS FOR, offered rather than imposed. It leads the pack, and Props
        // takes the last match, so a root naming its own Interactivity overrides this while one
        // that says nothing gets what a dialog root wants: Home, End and the page keys, which
        // address the nearest ActiveContainer in scope, and a current item the focus returns to
        // when the window is shown again.
        //
        // The window is a dialog either way - that is FormBase's answer and this does not touch
        // it. Reaching the root through the pack is what makes the value overridable, and it
        // asks that the root take props at all: a root built from a fixed argument list names
        // its Interactivity in its own constructor instead.
        return std::make_unique<Form<ControlClass>>(
            m_context, WindowRole::Dialog, nullptr, std::forward<Args>(args)...);
    }

    // PlatformHolder

    template <IsPlatform PlatformType>
    PlatformHolder<PlatformType>::PlatformHolder(const typename PlatformType::Params& params)
        :
        m_platform{ params }
    {
    }

    // Application

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    Application<PlatformType, GpuBackend>::Application(
        typename PlatformType::Params&& platform,
        const AppParams& appParams,
        Dom::Dt::Section&& configSchema)
        :
        PlatformHolder<PlatformType>{ platform },
        ApplicationBase{
            this->m_platform,
            appParams,
            gpuBackendFactory<GpuBackend>(),
            std::forward<Dom::Dt::Section>(configSchema)
        }
    {
        //m_platform.initialize(context());
        initialize();
    }

    template <IsPlatform PlatformType, IsOptionalGpuBackend GpuBackend>
    Application<PlatformType, GpuBackend>::~Application()
    {
        finalize();
    }

}
