export module ClaFi.Core.Context.AppContext;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Graphics.Cpu.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    export template <class B>
        concept IsPlatform = requires (B & b, const typename B::Params & p) {
        typename B::Params;
        B(p);
    };

    // How the application's colours moved: a set taken outright, or a crossing's frame. See Context
    export enum class ThemeSwitchKind
    {
        Taken,          // worn at once, from inside whatever took it
        CrossingFrame   // one frame of a running crossing, stated by the animation controller
    };

    // The colours every window is painted from have moved. See Context
    export class ThemeSwitchEvent : public Event
    {
    public:
        explicit ThemeSwitchEvent(ThemeSwitchKind);
        [[nodiscard]] ThemeSwitchKind kind() const { return m_kind; }
    private:
        ThemeSwitchKind m_kind;
    };

    // THE APPLICATION HAS CHANGED WHICH BACKEND ITS WINDOWS DRAW THROUGH. A form takes it at the
    // top of its next frame - see FormBase::stateBackend. See Context
    export struct BackendSwitchEvent : public Event
    {
    };

    // The size the application is drawn at has moved, and every window takes it. See Context
    export struct ScaleSwitchEvent : public Event
    {
    };

    export class AppContext
    {
    public:
        AppContext(Platform&, const Text& appName, std::wstring_view publisher,
            const Text& description, const std::filesystem::path& configPath, BackendFactory,
            Dom::Dt::Section&&);
        AppContext(Platform&, const Text& appName, std::wstring_view publisher,
            const Text& description, const std::filesystem::path& configPath, BackendFactory,
            const Dom::Dt::Section&);
        ~AppContext();
        // The name as it is shown. Its plain text is what the platform and the config folder know.
        [[nodiscard]] const Text& appName() const { return m_appName; }
        // Who publishes the application - the folder its settings stand under.
        [[nodiscard]] std::wstring_view publisher() const { return m_publisher; }
        // What the application does, in a sentence or two - empty where it states none.
        [[nodiscard]] const Text& description() const { return m_description; }

        // ONE ANSWER FOR THE PROCESS, and the only place a form can ask for a backend: the kind
        // the application is on at that moment, so no two windows can disagree. See Context
        [[nodiscard]] std::unique_ptr<Graphics::IBackend> createBackend(IPlatformWindow&, IntSize) const;
        // WHETHER THE APPLICATION HAS A GPU BACKEND AT ALL - whether it named one. Where it did
        // not, the CPU backend is the whole of the choice and nothing is offered to the user.
        [[nodiscard]] bool gpuAvailable() const { return m_createGpuBackend != nullptr; }
        // Which of the two every window draws through now.
        [[nodiscard]] bool usingGpu() const { return m_usingGpu; }

        const Platform& platform() const { return m_platform; }
        Platform& platform() { return m_platform; }
        // THE APPLICATION'S ANIMATIONS: every control's, every form's and the theme crossing's,
        // stepped by one timer. A control reaches it through its form - see Control::animator.
        [[nodiscard]] AnimationController& animator() { return m_animator; }
        AppTheme& theme() { return m_theme; }
        const AppTheme& theme() const { return m_theme; }
        ThemeMetrics& themeMetrics() { return m_theme.metrics; }
        const ThemeMetrics& themeMetrics() const { return m_theme.metrics; }
        ThemeColors& themeColors() { return m_theme.colors; }
        const ThemeColors& themeColors() const { return m_theme.colors; }
        // WHAT EVERY WINDOW IS PAINTED FROM. The theme above is what a file holds and an editor
        // edits; this is what that theme becomes, and the paint path reads only this.
        const BakedColors& bakedColors() const { return m_bakedColors; }
        //
        // --- Switching themes ---:
        // THE APPLICATION PUTS THIS THEME ON, crossing to it over AnimationSlots::themeSwitch.
        // Baking is answered above the core - see connectAppThemes - so the set arrives already
        // made, every value of which has a half way, which is what the crossing rests on.
        // See Context
        //
        // COLOURS ONLY: a theme's metrics take no part, and nothing here lays anything out.
        //
        // The set is copied, because what it was taken from is free to be edited from under it: a
        // theme editor previewing its own work in the application states the same set on every
        // slider step, and each one has to show.
        void takeTheme(const AppTheme&, const BakedColors&);
        // Where a form connects to be told that the theme has moved under it. Carried once per
        // frame for the length of a switch.
        [[nodiscard]] EventDispatcher& events() { return m_events; }
        //
        // --- Configuration ---:
        using ThemePathNode = Dom::Value<std::wstring>;
        using ColorModeNode = Dom::Value<ColorModeSetting>;
        using GpuAccelerationNode = Dom::Value<bool>;
        using ScaleNode = Dom::Value<int>;
        Dom::DocumentBase& config() { return m_config; }
        // The theme the application wears, as a path under a root - see AppTheme. The node holds
        // the path and nothing else: what a path names is answered where the themes are, which is
        // above the core - see Application
        [[nodiscard]] ThemePathNode& themePath() const { return m_themePath; }
        // The mode the user chose to wear the theme in, answered where the path is. See Application
        [[nodiscard]] ColorModeNode& colorMode() const { return m_colorMode; }
        // WHERE THE USER'S ANSWER ABOUT THE GPU IS KEPT, and what the Settings check writes: the
        // node's own change is what reaches stateBackend. Null where the application has no GPU
        // backend, the node not being in the schema then - gpuAvailable is that same answer. The
        // node is the config's state rather than this context's, so a const context hands it out.
        [[nodiscard]] GpuAccelerationNode* gpuAcceleration() const { return m_gpuAcceleration; }
        // WHERE THE USER'S ANSWER ABOUT THE SIZE IS KEPT, and what the Settings slider writes:
        // the node's own change is what reaches stateScale. The node is the config's state
        // rather than this context's, so a const context hands it out. See Context
        [[nodiscard]] ScaleNode& scale() const { return m_scale; }
        // The percent every window is drawn at, inside the band below.
        [[nodiscard]] int scalePercent() const { return m_scalePercent; }
        // The band an application may be drawn in, as a percent of the design it is written at.
        // A config naming anything outside it is brought inside - see stateScale.
        static constexpr int k_minScalePercent{ 75 };
        static constexpr int k_maxScalePercent{ 150 };
        static constexpr int k_defaultScalePercent{ 100 };
        // Whether the user has allowed storing - the config folder is that answer. See Context
        [[nodiscard]] bool configFolderExists() const { return m_configFolderExists; }
        void setConfigFolderExists(bool value) { m_configFolderExists = value; }
        // Where this application's settings are kept: the folder the config file stands in.
        [[nodiscard]] std::filesystem::path configFolder() const;
        // Makes that folder, which is the user allowing anything to be stored at all. See Context
        void ensureConfigFolder();
        // Takes that folder and everything in it away. See Context
        void deleteConfigFolder();
        // The placement kept under this form name, given to the form's window. See Context
        void restoreFormPlacement(std::wstring_view formName, IForm&);
        // The form's placement, written under this name into the Forms section. See Context
        void storeFormPlacement(std::wstring_view formName, IForm&);
        // The form whose loop the application stands on, where one stands. See Context
        [[nodiscard]] const IForm* rootLoopForm() const { return m_rootLoopForm; }
        // Takes the root loop where none stands - the answer is what the caller releases by.
        [[nodiscard]] bool claimRootLoop(const IForm&);
        void releaseRootLoop(const IForm&);
        // Where a form's placement is kept: the section every form named in the schema has.
        static constexpr std::wstring_view k_formsSectionName = L"Forms";
        // The name an application's main window is kept under. See Context
        static constexpr std::wstring_view k_mainFormName = L"MainForm";
        //
    private:
        // What the two factors make of the two sets, and every window told to repaint in it.
        void stateBakedColors();
        // Whether either of the two slots is still running.
    public: // Hunt Stagger
        [[nodiscard]] bool crossing() const;
    private:
        // The core's own section of an application's config - see the definition.
        [[nodiscard]] static Dom::Dt::Section coreConfigSchema(bool withGpuAcceleration);
        // Takes the wish the config holds and stands on the node for what follows. See Context
        void connectGpuAcceleration();
        // The application is on this kind from now on, and every window is told. See Context
        void stateBackend(bool useGpu);
        // Takes the size the config holds and stands on the node for what follows. See Context
        void connectScale();
        // The application is drawn at this percent from now on, and every window is told.
        // See Context
        void stateScale(int percent);
    private:
        static constexpr std::wstring_view k_themeNodeName = L"Theme";
        static constexpr std::wstring_view k_colorModeNodeName = L"ColorMode";
        static constexpr std::wstring_view k_gpuAccelerationNodeName = L"GpuAcceleration";
        static constexpr std::wstring_view k_scaleNodeName = L"Scale";
        Platform& m_platform;
        Text m_appName;
        std::wstring_view m_publisher;
        Text m_description;
        // The GPU backend the application named, if it named one - see gpuAvailable. The CPU
        // backend needs no factory here: the core owns that type.
        BackendFactory m_createGpuBackend;
        bool m_configFolderExists;
        // The form whose loop ends the application when it returns - see claimRootLoop.
        const IForm* m_rootLoopForm{ nullptr };
        AnimationController m_animator{};
        // The application wears the default theme until it is told otherwise. The baked set
        // beside it is filled before any window exists - see connectAppThemes.
        AppTheme m_theme{ defaultTheme() };
        // WHAT EVERY WINDOW IS PAINTED FROM: part way between the two below while a crossing
        // runs, and the incoming set once it has landed.
        BakedColors m_bakedColors{};
        // What the crossing started from - what stood on the screen when it did, itself part
        // way between two sets where a crossing was already running - and what it heads to.
        BakedColors m_outgoingColors{};
        BakedColors m_incomingColors{};
        // How far the crossing has come, and how far its lightness has - two of them because
        // lightness runs on a slot of its own. Both rest at 1, where what is worn is the
        // incoming set.
        float m_crossingFactor{ 1.0f };
        float m_lightnessFactor{ 1.0f };
        // Either factor moved in this tick; the repaint waits for the tick so it is made once.
        bool m_bakedColorsPending{ false };
        // WHAT IS WORN, as an address and a lightness, which is what tells an edit from a
        // crossing. Seeded with what m_theme is, so a first theme equal to it is not crossed to
        // from itself. See Context
        const AppTheme* m_taken{ &defaultTheme() };
        Lightness m_takenLightness{ k_darkLightness };
        Dom::Document<Dom::FileFormat::ClaFi> m_config;
        ThemePathNode& m_themePath{
            *(static_cast<ThemePathNode*>(m_config.child(k_themeNodeName))) };
        ColorModeNode& m_colorMode{
            *(static_cast<ColorModeNode*>(m_config.child(k_colorModeNodeName))) };
        // Null where the schema carries no such node - see coreConfigSchema, which is the one
        // place the node's presence is decided.
        GpuAccelerationNode* m_gpuAcceleration{
            static_cast<GpuAccelerationNode*>(m_config.child(k_gpuAccelerationNodeName)) };
        // WHICH BACKEND EVERY WINDOW DRAWS THROUGH NOW. Taken from the node above while the
        // context is built, so a stored answer is what the first window stands on rather than
        // something switched to once it is up.
        bool m_usingGpu{ false };
        ScopedEventConnection m_gpuConnection{};
        ScaleNode& m_scale{
            *(static_cast<ScaleNode*>(m_config.child(k_scaleNodeName))) };
        // THE PERCENT EVERY WINDOW IS DRAWN AT. Taken from the node above while the context is
        // built, so a stored size is what the first window opens at rather than one it is moved
        // to once it is up.
        int m_scalePercent{ k_defaultScalePercent };
        ScopedEventConnection m_scaleConnection{};
        EventDispatcher m_events{};
    };

    // ThemeSwitchEvent

    ThemeSwitchEvent::ThemeSwitchEvent(ThemeSwitchKind kind)
        :
        m_kind{ kind }
    {
    }

    // AppContext

    AppContext::AppContext(Platform& platform, const Text& appName, const std::wstring_view publisher,
        const Text& description, const std::filesystem::path& configPath,
        BackendFactory createGpuBackend, Dom::Dt::Section&& schema)
        :
        m_platform{ platform },
        m_appName{ appName },
        m_publisher{ publisher },
        m_description{ description },
        m_createGpuBackend{ createGpuBackend },
        m_configFolderExists{ std::filesystem::exists(configPath.parent_path()) },
        m_config{ configPath, Dom::AutoSave::No,
            Dom::Dt::Section{
                std::move(schema),
                coreConfigSchema(createGpuBackend != nullptr)
            },
            Dom::WriteDefaults::No
        }
    {
        connectGpuAcceleration();
        connectScale();
    }

    AppContext::AppContext(Platform& platform, const Text& appName, const std::wstring_view publisher,
        const Text& description, const std::filesystem::path& configPath,
        BackendFactory createGpuBackend, const Dom::Dt::Section& schema)
        :
        m_platform{ platform },
        m_appName{ appName },
        m_publisher{ publisher },
        m_description{ description },
        m_createGpuBackend{ createGpuBackend },
        m_configFolderExists{ std::filesystem::exists(configPath.parent_path()) },
        m_config{ configPath, Dom::AutoSave::No,
            Dom::Dt::Section{
                schema,
                coreConfigSchema(createGpuBackend != nullptr)
            },
            Dom::WriteDefaults::No
        }
    {
        connectGpuAcceleration();
        connectScale();
    }

    AppContext::~AppContext()
    {
        // A crossing outlives nothing: its callback reaches back into this context.
        m_animator.stop(this);
        m_animator.clearTickEndListener(this);
    }

    // NO HANDLER ON THE THEME NODES. A path is resolved against the themes on the disk as well as
    // against the one compiled in, and reading a theme file is not the core's work - see
    // connectAppThemes, which is what answers a change of either.
    //
    // THE GPU NODE STANDS ONLY WHERE A GPU BACKEND DOES, so an application that has none keeps no
    // answer to a question it cannot ask, and a file it wrote holds nothing about a backend.
    Dom::Dt::Section AppContext::coreConfigSchema(const bool withGpuAcceleration)
    {
        Dom::Dt::Section section = {
            Dom::Dt::Value{ k_themeNodeName, std::wstring{ k_defaultThemePath } },
            Dom::Dt::Value{ k_colorModeNodeName, ColorModeSetting::Auto },
            Dom::Dt::Value{ k_scaleNodeName, k_defaultScalePercent }
        };
        if (withGpuAcceleration)
            section.append(Dom::Dt::Section{ Dom::Dt::Value{ k_gpuAccelerationNodeName, true } });
        return section;
    }

    // THE NODE IS WHAT THE ANSWER TRAVELS ON, the same as the theme's: a Settings check writes it
    // and the change arrives here. The connection is a member so that it goes before the config
    // the node stands in.
    void AppContext::connectGpuAcceleration()
    {
        if (!m_gpuAcceleration)
            return;
        m_usingGpu = m_gpuAcceleration->get();
        m_gpuConnection = ScopedEventConnection{ m_gpuAcceleration->connectEvent(
            [this](Dom::ChangeEvent&) {
                stateBackend(m_gpuAcceleration->get());
            }) };
    }

    // A WINDOW IS BUILT ON WHAT THE APPLICATION IS WEARING AT THAT MOMENT, so one opened after a
    // switch matches the windows that swapped. m_usingGpu stands only where a factory does - the
    // node it is read from is in the schema on that condition alone.
    std::unique_ptr<Graphics::IBackend> AppContext::createBackend(IPlatformWindow& window,
        IntSize size) const
    {
        if (m_usingGpu)
            return m_createGpuBackend(window, size);
        return makeBackend<Graphics::Cpu::CpuBackend>(window, size);
    }

    // THE ANSWER MOVES ONCE AND EVERY WINDOW HEARS IT. A form takes it at the top of its next
    // frame rather than here - see FormBase::stateBackend - so a switch asked for from inside a
    // click cannot land between the passes of a frame that is being painted.
    void AppContext::stateBackend(const bool useGpu)
    {
        const bool wanted = useGpu and gpuAvailable();
        if (wanted == m_usingGpu)
            return;
        m_usingGpu = wanted;
        m_events.emit<BackendSwitchEvent>();
    }

    // THE NODE IS WHAT THE ANSWER TRAVELS ON, the same as the backend's: the Settings slider
    // writes it and the change arrives here. The connection is a member so that it goes before
    // the config the node stands in.
    void AppContext::connectScale()
    {
        m_scalePercent = std::clamp(m_scale.get(), k_minScalePercent, k_maxScalePercent);
        m_scaleConnection = ScopedEventConnection{ m_scale.connectEvent(
            [this](Dom::ChangeEvent&) {
                stateScale(m_scale.get());
            }) };
    }

    // THE ANSWER MOVES ONCE AND EVERY WINDOW HEARS IT. The percent is brought inside the band
    // first: the config is text the user is free to edit, and a window drawn at a factor outside
    // it is one nothing on it can be reached in.
    void AppContext::stateScale(const int percent)
    {
        const int wanted = std::clamp(percent, k_minScalePercent, k_maxScalePercent);
        if (wanted == m_scalePercent)
            return;
        m_scalePercent = wanted;
        m_events.emit<ScaleSwitchEvent>();
    }

    std::filesystem::path AppContext::configFolder() const
    {
        return m_config.path().parent_path();
    }

    // THE FOLDER IS THE PERMISSION, and this is the only place one is made. An application with
    // nowhere to put it - no appDataPath - has no folder to offer and asks for nothing.
    void AppContext::ensureConfigFolder()
    {
        if (m_configFolderExists)
            return;
        const std::filesystem::path folder = configFolder();
        if (folder.empty())
            return;
        std::filesystem::create_directories(folder);
        m_configFolderExists = true;
    }

    // EVERYTHING UNDER THE FOLDER GOES, because the folder is what the user was asked about: a
    // permission withdrawn is not withdrawn from one file. The publisher's folder above it goes
    // with it once it is empty: an application that has stopped storing should leave nothing
    // behind, and a publisher folder still holding another application's settings or the shared
    // themes directory is not empty and stays.
    //
    // THE APPLICATION DATA ROOT IS THE FLOOR. Where a publisher is unnamed the folder above this
    // one IS that root, shared with every other publisher, and its being empty says nothing about
    // who may still want it.
    void AppContext::deleteConfigFolder()
    {
        if (!m_configFolderExists)
            return;
        const std::filesystem::path folder = configFolder();
        std::error_code error{};
        std::filesystem::remove_all(folder, error);
        m_configFolderExists = false;

        const std::filesystem::path publisher = folder.parent_path();
        if (publisher.empty())
            return;
        if (std::filesystem::equivalent(publisher, Platform::appDataPath(), error) || error)
            return;
        if (!std::filesystem::is_empty(publisher, error) || error)
            return;
        std::filesystem::remove(publisher, error);
    }

    // NOTHING IS ASKED FOR WITHOUT THE FOLDER. A placement is stored in the config, and on a
    // platform where the display server keeps it, against an id the config holds - so before the
    // user has allowed storing there is nothing to restore from, and no id may be created.
    void AppContext::restoreFormPlacement(std::wstring_view formName, IForm& form)
    {
        if (!m_configFolderExists)
            return;
        Platform::restoreFormPlacement(m_config.childSection(k_formsSectionName), formName, form);
    }

    // Written whether or not the folder exists: the config reaches the disk only through an
    // application that saves it, and that is where the folder decides.
    //
    // THE SESSION IS ASKED FOR HERE WHEN IT WAS NOT ASKED FOR AT THE RESTORE. The folder can be
    // made while the application runs - Keep settings on this PC - and a window standing since
    // before it was never named to a session. It is named now, while it is still up, so that the
    // compositor has its place to give back on the next launch.
    void AppContext::storeFormPlacement(std::wstring_view formName, IForm& form)
    {
        if (m_configFolderExists)
            Platform::addFormToSession(formName, form);
        Platform::storeFormPlacement(m_config.childSection(k_formsSectionName), formName, form);
    }

    // THE FIRST LOOP ENTERED IS THE ONE THE APPLICATION STANDS ON, so a nested one - a dialog, a
    // menu, an in-place editor - never takes it, and the form holding it ends the run by closing.
    bool AppContext::claimRootLoop(const IForm& form)
    {
        if (m_rootLoopForm)
            return false;
        m_rootLoopForm = &form;
        return true;
    }

    void AppContext::releaseRootLoop(const IForm& form)
    {
        if (m_rootLoopForm != &form)
            return;
        m_rootLoopForm = nullptr;
    }

    // ONE REPAINT TO A TICK. Both slots reach this while a mode is crossing, and a set blended
    // and painted from the first of them is overwritten by the second before anyone sees it -
    // half the painting of the first 330 ms of every light-dark crossing. Outside a tick there
    // is nothing to wait for and the answer is stated at once.
    void AppContext::stateBakedColors()
    {
        if (m_animator.ticking())
        {
            m_bakedColorsPending = true;
            return;
        }
        m_bakedColorsPending = false;
        m_bakedColors = blend(m_outgoingColors, m_incomingColors, m_crossingFactor,
            m_lightnessFactor);
        m_events.emit<ThemeSwitchEvent>(ThemeSwitchKind::CrossingFrame);
    }

    bool AppContext::crossing() const
    {
        return m_animator.isActive(AnimationSlots::themeSwitch)
            or m_animator.isActive(AnimationSlots::themeLightness);
    }

    void AppContext::takeTheme(const AppTheme& theme, const BakedColors& colors)
    {
        // THE SAME THEME AT THE SAME LIGHTNESS STATED AGAIN IS AN EDIT. The lightness is not an
        // edit: it inverts everything a set states, so the same theme in the other mode is a
        // crossing. See Context
        const bool edited = &theme == m_taken and colors.lightness == m_takenLightness;
        m_taken = &theme;
        m_takenLightness = colors.lightness;
        m_theme.colors = theme.colors;

        // AN EDIT RESTATES WHAT IS WORN, and while a crossing runs what is worn is not the set it
        // is heading to - so an edit arriving then is a restatement of that destination and the
        // crossing carries on to it. Landing it instead would put the destination on the screen
        // until the next tick took it back off, which is what a preview committed through the
        // config path does: the item was tried on, and the commit states the same theme again.
        if (edited and crossing())
        {
            m_incomingColors = colors;
            return;
        }

        // Nothing is crossing, so an edit lands at once - which is what keeps a slider drag glued
        // to the pointer. A crossing nobody can see is not made either: no window exists until
        // the first form is built, and that is the state an application loads its theme in.
        if (edited or !m_events.hasListeners<ThemeSwitchEvent>())
        {
            m_bakedColors = colors;
            m_events.emit<ThemeSwitchEvent>(ThemeSwitchKind::Taken);
            return;
        }

        // The crossing starts from what is on the screen, so a theme arriving while one is
        // running leaves nothing to reconcile - there is no half way rule to choose and no
        // reversal case, because the set being left is a value and not a name. Stopping first
        // is what starts the factor over: a request naming the end value a running animation
        // already heads to is that animation, and would carry on from where it had reached.
        m_outgoingColors = m_bakedColors;
        m_incomingColors = colors;
        // AN AXIS TWO SETS AGREE ON HAS NOTHING TO CROSS, so its slot is left alone and its
        // factor states the answer outright. Two themes worn in one mode agree on the lightness;
        // one theme worn in the two modes agrees on every colour - and a slot run over an axis
        // that does not move paints every form for a picture that has already stopped.
        const bool crossesColors = !sameColors(m_outgoingColors, m_incomingColors);
        const bool crossesLightness = m_outgoingColors.lightness != m_incomingColors.lightness;
        m_crossingFactor = crossesColors ? 0.0f : 1.0f;
        m_lightnessFactor = crossesLightness ? 0.0f : 1.0f;

        m_animator.stop(this, AnimationSlots::themeSwitch);
        m_animator.stop(this, AnimationSlots::themeLightness);

        // Neither axis moves, so there is no crossing to make and the set is worn at once. Two
        // themes stating the same colours at the same lightness are the same picture, whatever
        // else tells them apart.
        if (!crossesColors and !crossesLightness)
        {
            m_bakedColors = colors;
            m_events.emit<ThemeSwitchEvent>(ThemeSwitchKind::Taken);
            return;
        }

        m_animator.setTickEndListener(this,
            [this]() {
                if (m_bakedColorsPending)
                    stateBakedColors();
            });
        if (crossesColors)
        {
            m_animator.start(this, AnimationSlots::themeSwitch, 0.0f, 1.0f,
                [this](AnimateParams& params) {
                    m_crossingFactor = params.value;
                    stateBakedColors();
                });
        }
        if (crossesLightness)
        {
            m_animator.start(this, AnimationSlots::themeLightness, 0.0f, 1.0f,
                [this](AnimateParams& params) {
                    m_lightnessFactor = params.value;
                    stateBakedColors();
                });
        }
    }


}
