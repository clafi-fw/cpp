module;
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>

// AN IMPLEMENTATION UNIT OF THE MODULE THAT DECLARED Platform, not of the Wayland layer. A class
// member belongs to the module its class was declared in and must be defined there, so the bodies
// of Platform:: live here whatever platform supplies them.
module ClaFi.Core.Context.FormContext;

import ClaFi.Platform.Wayland.FormWindow;
import ClaFi.Platform.Wayland.Window;
import ClaFi.Platform.Wayland.Display;
import ClaFi.Platform.Linux.Appearance;
import ClaFi.Platform.Linux.Diagnostic;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        using PlatformImplementation::Wayland::DisplayManager;

        // What the desktop layer calls when the mode it reads has moved.
        void announceSystemColorMode()
        {
            Platform::events().emit<SystemColorModeEvent>();
        }

        // THE DISPLAY THE PLATFORM HOLDS, for the statics here, which have no object in hand. A
        // static asked while none stands is a broken order - see DisplayManager::standing.
        [[nodiscard]] DisplayManager& standingDisplay()
        {
            DisplayManager* display = DisplayManager::standing();
            if (display == nullptr)
                unreachable("Platform: no display stands - asked outside the platform's life");

            return *display;
        }
    }

    // THE DESKTOP-FILE SHAPE: the two names joined by a dot, spaces dropped, so that a
    // .desktop entry can be named the same and match.
    void Platform::setApplicationId(std::wstring_view publisher, std::wstring_view name)
    {
        std::string appId = toUtf8(publisher) + "." + toUtf8(name);
        std::erase(appId, ' ');
        standingDisplay().setAppId(std::move(appId));
    }

    // A WINDOW'S PLACE IS NOT THE CLIENT'S TO STORE. There is no global coordinate space to write
    // down and none to restore into: a compositor that remembers where a window was keeps the
    // rectangle itself, against an opaque session id the client persists and hands back - the
    // one value here that belongs to the application rather than to a form. What a form's own
    // section keeps is what a client can restore by itself: its ordinary size in surface pixels,
    // scale-independent, and whether it stood maximized.
    Dom::Dt::Section Platform::createFormsConfigSchema(FormNames formNames)
    {
        Dom::Dt::Section forms{ Dom::Dt::Value{ L"Session", L"" } };
        for (const std::wstring_view formName : formNames)
        {
            forms.children.push_back(std::make_unique<Dom::Dt::Section>(
                formName,
                Dom::Dt::Value{ L"Width", 0 },
                Dom::Dt::Value{ L"Height", 0 },
                Dom::Dt::Value{ L"Maximized", false }));
        }
        return forms;
    }

    // The session is asked for here, on the first form restored, with whatever id the config
    // holds; the window is named to it by its first placement.
    void Platform::restoreFormPlacement(Dom::Section& forms, std::wstring_view name, IForm& form)
    {
        using PlatformImplementation::Wayland::FormWindow;

        const std::wstring sessionId = (forms / L"Session").get<std::wstring>();
        standingDisplay().openSession(toUtf8(sessionId));

        const Dom::Section& storage = forms.childSection(name);
        auto& window = static_cast<FormWindow&>(form.wnd_window());
        window.setSessionName(toUtf8(name));
        const IntSize size = { (storage / L"Width").get<int>(), (storage / L"Height").get<int>() };
        if (size.x > 0 && size.y > 0)
            window.setRememberedPlacement({ size, (storage / L"Maximized").get<bool>() });
    }

    void Platform::storeFormPlacement(Dom::Section& forms, std::wstring_view name, const IForm& form)
    {
        using PlatformImplementation::Wayland::FormWindow;
        using PlatformImplementation::Wayland::RememberedPlacement;

        (forms / L"Session").set(fromUtf8(standingDisplay().sessionId()));

        Dom::Section& storage = forms.childSection(name);
        const auto& window = static_cast<const FormWindow&>(form.wnd_window());
        const RememberedPlacement placement = window.rememberedPlacement();
        (storage / L"Width").set(placement.size.x);
        (storage / L"Height").set(placement.size.y);
        (storage / L"Maximized").set(placement.maximized);
    }

    // A WINDOW THAT WAS ALREADY UP WHEN STORING WAS ALLOWED. Nothing asked the compositor for a
    // session while there was nowhere to write its id, so the session is asked for here instead,
    // and the window is added to it rather than restored from it: restore_toplevel is refused
    // once the surface has been committed, add_toplevel is not, and the compositor keeps the
    // state of what it has been given from the moment it is given it. The id is waited for
    // because the store that follows writes it.
    void Platform::addFormToSession(std::wstring_view name, IForm& form)
    {
        using PlatformImplementation::Wayland::FormWindow;

        DisplayManager& manager = standingDisplay();
        if (!manager.session())
        {
            manager.openSession({});
            manager.awaitSessionId();
        }
        static_cast<FormWindow&>(form.wnd_window()).addToSession(toUtf8(name));
    }

    // THE NAME IS THE KEY'S, NOT THE LAYOUT'S. A key code here is a Win32 virtual key, and a
    // letter's is the Latin letter on the key whatever layout is active - see Keyboard - so the
    // shortcut Ctrl+C is spelled with a C under a Cyrillic layout too. Win32 spells it with the
    // layout's letter, which is the one thing here that reads differently between the two.
    std::wstring Platform::keyName(const KeyCode key)
    {
        namespace VirtualKey = PlatformImplementation::Wayland::VirtualKey;

        if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9'))
            return { static_cast<wchar_t>(key) };

        constexpr KeyCode k_functionKeys = 24;
        constexpr KeyCode k_numpadDigits = 10;
        if (key >= VirtualKey::f1 && key < VirtualKey::f1 + k_functionKeys)
            return L"F" + std::to_wstring(key - VirtualKey::f1 + 1);
        if (key >= VirtualKey::numpad0 && key < VirtualKey::numpad0 + k_numpadDigits)
            return L"Num " + std::to_wstring(key - VirtualKey::numpad0);

        switch (key)
        {
        case Keys::BackSpace:
            return L"Backspace";
        case Keys::Tab:
            return L"Tab";
        case Keys::Return:
            return L"Enter";
        case Keys::Shift:
            return L"Shift";
        case Keys::Ctrl:
            return L"Ctrl";
        case Keys::Alt:
            return L"Alt";
        case Keys::Escape:
            return L"Esc";
        case Keys::Space:
            return L"Space";
        case Keys::Prior:
            return L"Page Up";
        case Keys::Next:
            return L"Page Down";
        case Keys::End:
            return L"End";
        case Keys::Home:
            return L"Home";
        case Keys::Left:
            return L"Left";
        case Keys::Up:
            return L"Up";
        case Keys::Right:
            return L"Right";
        case Keys::Down:
            return L"Down";
        case Keys::Delete:
            return L"Delete";
        case VirtualKey::clear:
            return L"Clear";
        case VirtualKey::pause:
            return L"Pause";
        case VirtualKey::capsLock:
            return L"Caps Lock";
        case VirtualKey::print:
            return L"Print Screen";
        case VirtualKey::insert:
            return L"Insert";
        case VirtualKey::help:
            return L"Help";
        case VirtualKey::applications:
            return L"Menu";
        case VirtualKey::multiply:
            return L"Num *";
        case VirtualKey::add:
            return L"Num +";
        case VirtualKey::separator:
            return L"Separator";
        case VirtualKey::subtract:
            return L"Num -";
        case VirtualKey::decimal:
            return L"Num .";
        case VirtualKey::divide:
            return L"Num /";
        case VirtualKey::numLock:
            return L"Num Lock";
        case VirtualKey::scrollLock:
            return L"Scroll Lock";
        case VirtualKey::oem1:
            return L";";
        case VirtualKey::oemPlus:
            return L"=";
        case VirtualKey::oemComma:
            return L",";
        case VirtualKey::oemMinus:
            return L"-";
        case VirtualKey::oemPeriod:
            return L".";
        case VirtualKey::oem2:
            return L"/";
        case VirtualKey::oem3:
            return L"`";
        case VirtualKey::oem4:
            return L"[";
        case VirtualKey::oem5:
            return L"\\";
        case VirtualKey::oem6:
            return L"]";
        case VirtualKey::oem7:
            return L"'";
        default:
            return {};
        }
    }

    // As of the keyboard's last word. A client is told which modifiers are held as part of being
    // given the keyboard and on every change while it has it, and nothing while it does not - so
    // this is exact while a window of this client is being typed into and empty otherwise.
    KeyModifiers Platform::keyModifiers()
    {
        return standingDisplay().keyModifiers();
    }

    // No protocol carries this. A desktop bell belongs to the sound daemon, which is reached
    // through the session bus rather than through the display server.
    void Platform::beep(Frequency, MilliSeconds)
    {
    }

    // ONE, ALWAYS. Scale is per-surface here and arrives on the surface that landed on a monitor,
    // so there is no global answer - and a form that has a window reads its own.
    ScaleFactor Platform::globalScaleFactor()
    {
        return 1.0f;
    }

    // The rate every toolkit settles on, there being no protocol that publishes one.
    MilliSeconds Platform::caretBlinkTime()
    {
        return { 530 };
    }

    // CLOCK_PROCESS_CPUTIME_ID counts every thread of this process, kernel time included, which is
    // the same quantity Windows sums two FILETIMEs for.
    std::chrono::nanoseconds Platform::processCpuTime()
    {
        timespec cpuTime{};
        if (::clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpuTime) != 0)
            return {};

        return std::chrono::seconds{ cpuTime.tv_sec } + std::chrono::nanoseconds{ cpuTime.tv_nsec };
    }

    // The shape is the seat's to show, the pointer being the seat's - see DisplayManager::setCursor
    // for what it does when there is no pointer over a window of this client.
    void Platform::setCursor(CursorShape value)
    {
        standingDisplay().setCursor(value);
    }

    // The shape the seat is shown, a resize edge's included, as the protocol numbers it.
    Cursor Platform::cursor()
    {
        return { standingDisplay().cursorShape() };
    }

    void Platform::setCursor(Cursor value)
    {
        standingDisplay().setCursorShape(static_cast<std::uint32_t>(value.value));
    }

    // A cursor is drawn by the compositor from a name, and its size and hotspot are never sent to
    // the client. These are the sizes a theme conventionally uses, which is what a caller laying
    // out around the pointer needs and all it can be given.
    CursorInfo Platform::getCursorInfo(CursorShape value)
    {
        switch (value)
        {
        case CursorShape::IBeam:
            return { .size = { 24, 24 }, .hotSpot = { 12, 12 } };
        case CursorShape::Hand:
            return { .size = { 24, 24 }, .hotSpot = { 10, 5 } };
        case CursorShape::Wait:
            return { .size = { 24, 24 }, .hotSpot = { 12, 12 } };
        default:
            return { .size = { 24, 24 }, .hotSpot = { 5, 5 } };
        }
    }

    // TWICE FORKED, so the child is reparented to init and there is no zombie for this process to
    // reap. A UI process that waits on a document viewer is a UI process that has stopped.
    void Platform::shellExecute(const IForm*, const std::wstring_view file,
        const std::wstring_view params)
    {
        const std::string target = toUtf8(file);
        const std::string arguments = toUtf8(params);

        const pid_t outer = ::fork();
        if (!PlatformImplementation::Linux::check(outer >= 0))
            return;

        if (outer == 0)
        {
            if (::fork() == 0)
            {
                if (arguments.empty())
                    ::execlp("xdg-open", "xdg-open", target.c_str(), nullptr);
                else
                    ::execlp("xdg-open", "xdg-open", target.c_str(), arguments.c_str(), nullptr);
                ::_exit(127);
            }
            ::_exit(0);
        }

        int status = 0;
        ::waitpid(outer, &status, 0);
    }

    std::wstring Platform::appDataPath()
    {
        if (const char* configHome = std::getenv("XDG_CONFIG_HOME"))
            if (*configHome)
                return fromUtf8(configHome) + L"/";

        if (const char* home = std::getenv("HOME"))
            return fromUtf8(home) + L"/.config/";

        return L"./";
    }

    void Platform::debugOutput(const std::wstring_view line)
    {
        std::string text = toUtf8(line);
        text.push_back('\n');
        std::fputs(text.c_str(), stderr);
    }

    // THE PORTAL'S ANSWER, read over the session bus. The handler is named ahead of the first
    // read, which is what subscribes to the changes.
    ColorMode Platform::systemColorMode()
    {
        PlatformImplementation::Linux::setAppearanceHandler(announceSystemColorMode);
        return PlatformImplementation::Linux::desktopColorMode();
    }
}
