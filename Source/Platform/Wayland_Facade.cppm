export module ClaFi.Platform.Wayland;

import ClaFi.Platform.Wayland.FormWindow;
import ClaFi.Platform.Wayland.Display;
import ClaFi.Platform.Linux.Timer;
import ClaFi.Platform.Linux.DirWatch;

import ClaFi.App.Application;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // What an application is built against on Wayland. See Platform
    export struct WaylandPlatform
        :
        public Platform,
        public PlatformImplementation::Wayland::IDisplayAccess
    {
    public:
        struct Params
        {
            // WATCHING THE CLIPBOARD IS NOT THE SAME PRIVILEGE AS PASTING FROM IT. A compositor
            // announces a selection to the client holding the keyboard focus and to nobody else,
            // so a window that shows what is on the clipboard is told nothing while it is not the
            // one being typed into. An application that states this here is given the clipboard
            // manager's channel instead, where the compositor advertises one - Mutter does not,
            // and there the focus rule stands whatever is asked for.
            bool watchesClipboard{ false };
        };
        // Connects to the compositor, or throws - see DisplayManager.
        explicit WaylandPlatform(Params);
        [[nodiscard]] std::unique_ptr<IPlatformWindow> createWindow(IForm&, WindowRole,
            IForm* parentForm) override;
        // The optional protocols the compositor offers and the ones it does not.
        [[nodiscard]] std::vector<std::wstring> diagnosticLines() const override;
        [[nodiscard]] PlatformImplementation::Wayland::DisplayManager& display() override;
    private:
        // IN THIS ORDER. The display is the connection and the loop, so everything else is built
        // on it and goes before it; the two managers put their descriptor in its loop for as
        // long as they stand; the clipboard last, handing its selection and its change handler
        // back to the display on the way out.
        PlatformImplementation::Wayland::DisplayManager m_display{};
        PlatformImplementation::Linux::TimerManager m_timers{};
        PlatformImplementation::Linux::DirWatchManager m_dirWatches{};
        PlatformImplementation::Wayland::PollSource m_timerSource{
            m_display,
            m_timers.eventFd(),
            [this]() {
                m_timers.dispatchPending();
            }
        };
        PlatformImplementation::Wayland::PollSource m_dirWatchSource{
            m_display,
            m_dirWatches.notifyFd(),
            [this]() {
                m_dirWatches.dispatchPending();
            }
        };
        Transfer::Clipboard m_clipboard{ *this };
    };

    // NO GPU BACKEND ON THIS PLATFORM, so an application here draws on the CPU and is offered
    // nothing to turn on. A wl_shm buffer is memory the compositor can read, which is exactly
    // what a CPU backend already produces.
    export using WaylandApplication = Application<WaylandPlatform>;
}


//-----------------------------------------------------------------------------


namespace ClaFi
{
    // The base is handed the clipboard's address before the clipboard is built, which a reference
    // may be: nothing reads through it until the application asks for the clipboard, and the
    // platform stands whole by then.
    WaylandPlatform::WaylandPlatform(const Params params)
        :
        Platform{ m_clipboard }
    {
        if (!params.watchesClipboard)
            return;

        m_display.createDataControlDevice();
    }

    std::unique_ptr<IPlatformWindow> WaylandPlatform::createWindow(IForm& form, WindowRole role,
        IForm* parentForm)
    {
        return std::make_unique<PlatformImplementation::Wayland::FormWindow>(
            m_display, form, role, parentForm);
    }

    std::vector<std::wstring> WaylandPlatform::diagnosticLines() const
    {
        using PlatformImplementation::Wayland::DisplayManager;

        std::wstring present{};
        std::wstring missing{};
        for (const DisplayManager::OptionalGlobal& global : m_display.optionalGlobals())
        {
            std::wstring& list = global.offered ? present : missing;
            if (!list.empty())
                list += L", ";
            // Interface names are ASCII, so widening is a copy.
            list.append(global.name.begin(), global.name.end());
        }
        if (present.empty())
            present = L"none";
        if (missing.empty())
            missing = L"none";
        return {
            L"Wayland optional protocols present: " + present,
            L"Wayland optional protocols missing: " + missing
        };
    }

    PlatformImplementation::Wayland::DisplayManager& WaylandPlatform::display()
    {
        return m_display;
    }
}
