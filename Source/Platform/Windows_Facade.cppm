module;
#include "Windows/Windows.Headers.h"
#include <shellapi.h> // shellExecute

#include <ShlObj.h> // appDataPath
#include <KnownFolders.h> // appDataPath

export module ClaFi.Platform.Windows;

import ClaFi.Platform.Windows.Window;
import ClaFi.Platform.Windows.FormWindow;
import ClaFi.Platform.Windows.Timer;
import ClaFi.Platform.Windows.D2D;
import ClaFi.Platform.Windows.Diagnostic;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.System.UiTypes;
import ClaFi.App.Application;

export using HINSTANCE = ::HINSTANCE;
export using Direct2DBackend = ::ClaFi::PlatformImplementation::Windows::Direct2DBackend;

import ClaFi.StdLib;

namespace ClaFi
{
    // What an application is built against on Win32. See Platform
    export struct Win32Platform
        :
        public Platform,
        public PlatformImplementation::Windows::IMessageWindowFactory
    {
    public:
        struct Params
        {
            HINSTANCE appInstance;
        };
        explicit Win32Platform(Params);
        //void initialize(AppContext&);
        [[nodiscard]] std::unique_ptr<IPlatformWindow> createWindow(IForm&, WindowRole,
            IForm* parentForm) override;
        [[nodiscard]] std::unique_ptr<PlatformImplementation::Windows::MessageWindow>
            createMessageWindow() override;
    private:
        Params m_params;
        // IN THIS ORDER. The manager registers the window classes and opens the OLE apartment,
        // and closes both on the way out, so everything made from it is built after it and goes
        // before it; the clipboard last, its flush wanting the apartment open.
        PlatformImplementation::Windows::WindowsManager m_manager{ m_params.appInstance };
        PlatformImplementation::Windows::TimerManager m_timers{ m_manager };
        Transfer::Clipboard m_clipboard{ *this };
    };

    // Direct2D is the GPU backend offered here; the CPU backend stands beside it, and which of
    // them the windows draw through is the user's answer - see AppContext::gpuAcceleration.
    export using Win32Application = Application<Win32Platform, Direct2DBackend>;

    //-------------------------------------------------------------------------


    static bool isWindows11_orGreater()
    {
        enum class Result {
            NotInitialized,
            False,
            True
        };
        static Result result = Result::NotInitialized;
        if (result == Result::NotInitialized)
            if (HMODULE ntdll = ::GetModuleHandleA("ntdll"))
            {
                using NTSTATUS = std::uint32_t;
                using RtlGetVersionFunc = NTSTATUS(__stdcall*)(OSVERSIONINFOEXW&);
                using FarProc = std::intptr_t(__stdcall*)();
                RtlGetVersionFunc RtlGetVersion;
                result = Result::False;
                *reinterpret_cast<FarProc*>(&RtlGetVersion) = ::GetProcAddress(ntdll, "RtlGetVersion");
                if (RtlGetVersion)
                {
                    ::OSVERSIONINFOEXW osInfo{};
                    osInfo.dwOSVersionInfoSize = sizeof ::OSVERSIONINFOEXW;
                    RtlGetVersion(osInfo);
                    if (osInfo.dwMajorVersion > 10 || (osInfo.dwMajorVersion == 10 && osInfo.dwBuildNumber >= 21996))
                        result = Result::True;
                }
            }
        return result == Result::True;
    }

    //void Win32Platform::initialize(AppContext& appContext)
    //{
    //    appContext.themeMetrics().primaryWindow.radius = isWindows11_orGreater() ? 9.0f : 0.0f;
    //    if (!isWindows11_orGreater())
    //        appContext.themeMetrics().primaryWindow.border = appContext.themeMetrics().border;
    //}

    // Win32Platform

    // The base is handed the clipboard's address before the clipboard is built, which a reference
    // may be: nothing reads through it until the application asks for the clipboard, and the
    // platform stands whole by then.
    Win32Platform::Win32Platform(const Params params)
        :
        Platform{ m_clipboard },
        m_params{ params }
    {
    }

    std::unique_ptr<IPlatformWindow> Win32Platform::createWindow(IForm& form, WindowRole role,
        IForm* parentForm)
    {
        return std::make_unique<PlatformImplementation::Windows::FormWindow>(
            m_manager, form, role, parentForm);
    }

    std::unique_ptr<PlatformImplementation::Windows::MessageWindow>
        Win32Platform::createMessageWindow()
    {
        return std::make_unique<PlatformImplementation::Windows::MessageWindow>(m_manager);
    }

    // A process is known to the desktop by its executable; there is nothing to state here.
    void Platform::setApplicationId(std::wstring_view, std::wstring_view)
    {
    }

    // THE CLIENT KEEPS THE RECTANGLE. A window's normal geometry in screen pixels and whether it
    // stands maximized - what WINDOWPLACEMENT answers, less the ring the window wears past it -
    // and whether it is held above the others, which is a style here and the client's to keep.
    Dom::Dt::Section Platform::createFormsConfigSchema(FormNames formNames)
    {
        Dom::Dt::Section forms{};
        for (const std::wstring_view formName : formNames)
        {
            forms.children.push_back(std::make_unique<Dom::Dt::Section>(
                formName,
                Dom::Dt::Value{ L"Bounds", IntRect{} },
                Dom::Dt::Value{ L"Maximized", false },
                Dom::Dt::Value{ L"AlwaysOnTop", false }));
        }
        return forms;
    }

    void Platform::restoreFormPlacement(Dom::Section& forms, std::wstring_view name, IForm& form)
    {
        using PlatformImplementation::Windows::FormWindow;

        const Dom::Section& storage = forms.childSection(name);
        const IntRect geometry = (storage / L"Bounds").get<IntRect>();
        if (geometry.empty())
            return;

        auto& window = static_cast<FormWindow&>(form.wnd_window());
        window.setNormalPlacement({
            geometry,
            (storage / L"Maximized").get<bool>(),
            (storage / L"AlwaysOnTop").get<bool>() });
    }

    void Platform::storeFormPlacement(Dom::Section& forms, std::wstring_view name, const IForm& form)
    {
        using PlatformImplementation::Windows::FormWindow;
        using PlatformImplementation::Windows::NormalPlacement;

        Dom::Section& storage = forms.childSection(name);
        const auto& window = static_cast<const FormWindow&>(form.wnd_window());
        const NormalPlacement placement = window.normalPlacement();
        (storage / L"Bounds").set(placement.geometry);
        (storage / L"Maximized").set(placement.maximized);
        (storage / L"AlwaysOnTop").set(placement.alwaysOnTop);
    }

    // The client keeps the rectangle here and the window manager is told nothing, so a window
    // that stood before storing was allowed has nothing to be named to.
    void Platform::addFormToSession(std::wstring_view, IForm&)
    {
    }

    std::wstring Platform::keyName(const KeyCode key)
    {
        // GetKeyNameText reads the scan code out of bits 16-23 of a keystroke message's
        // lParam, and bit 24 says the key came from the block beside the numeric keypad
        // rather than from the pad itself. Both halves have to be rebuilt here, because
        // there is no message: the shortcut was never pressed, it is only being spelled.
        const UINT scanCode = MapVirtualKeyW(key, MAPVK_VK_TO_VSC);
        if (!scanCode)
            return {};

        bool extended = false;
        switch (key)
        {
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_INSERT:
        case VK_DELETE:
        case VK_DIVIDE:
        case VK_NUMLOCK:
            extended = true;
            break;
        }

        LONG lParam = static_cast<LONG>(scanCode << 16);
        if (extended)
            lParam |= 1 << 24;

        std::array<wchar_t, 64> buffer{};
        const int length = GetKeyNameTextW(lParam, buffer.data(), static_cast<int>(buffer.size()));
        return { buffer.data(), static_cast<std::size_t>(length) };
    }

    KeyModifiers Platform::keyModifiers()
    {
        return{
            .shift = ::GetKeyState(VK_SHIFT) < 0,
            .ctrl = ::GetKeyState(VK_CONTROL) < 0,
            .alt = ::GetKeyState(VK_MENU) < 0,
        };
    }

    void Platform::beep(Frequency frequency, MilliSeconds duration)
    {
        ::Beep(frequency.value, duration.value);
    }

    ScaleFactor Platform::globalScaleFactor()
    {
        const HDC dc = ::GetDC(nullptr);
        const int dpi = ::GetDeviceCaps(dc, LOGPIXELSY);
        ::ReleaseDC(nullptr, dc);
        return dpi / 96.0f;
    }

    MilliSeconds Platform::caretBlinkTime()
    {
        const UINT result = ::GetCaretBlinkTime();
        // INFINITE is what the system reports for a caret the user has asked not to blink, and 0
        // is what it reports when it cannot answer. Both come back as zero, which the caller reads
        // as a caret that holds still - the safe reading of a rate that is not available.
        if (result == INFINITE || result == 0)
            return { 0 };
        return { result };
    }

    // The current-process handle is a pseudo handle, so there is nothing to open and nothing to
    // close. A FILETIME counts in units of 100 nanoseconds.
    std::chrono::nanoseconds Platform::processCpuTime()
    {
        FILETIME creationTime{};
        FILETIME exitTime{};
        FILETIME kernelTime{};
        FILETIME userTime{};
        if (!::GetProcessTimes(::GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime))
            return {};

        const std::uint64_t kernel = ULARGE_INTEGER{ kernelTime.dwLowDateTime, kernelTime.dwHighDateTime }.QuadPart;
        const std::uint64_t user = ULARGE_INTEGER{ userTime.dwLowDateTime, userTime.dwHighDateTime }.QuadPart;
        return std::chrono::nanoseconds{ (kernel + user) * 100 };
    }

    void Platform::setCursor(CursorShape value)
    {
        const wchar_t* cursorName = nullptr;
        switch (value)
        {
        case CursorShape::IBeam:
            cursorName = IDC_IBEAM;
            break;
        case CursorShape::Hand:
            cursorName = IDC_HAND;
            break;
        case CursorShape::Arrow:
            cursorName = IDC_ARROW;
            break;
        case CursorShape::Wait:
            cursorName = IDC_WAIT;
            break;
        default:
            return;
        }
        ::SetCursor(::LoadCursorW(nullptr, cursorName));
    }

    // What the thread is showing, whoever set it - the sizing arrow of a modal loop included.
    Cursor Platform::cursor()
    {
        return { reinterpret_cast<std::uintptr_t>(::GetCursor()) };
    }

    void Platform::setCursor(Cursor value)
    {
        ::SetCursor(reinterpret_cast<HCURSOR>(value.value));
    }

    CursorInfo Platform::getCursorInfo(CursorShape value)
    {
        CursorInfo result{};

        const wchar_t* cursorName = nullptr;
        switch (value)
        {
        case CursorShape::IBeam:
            cursorName = IDC_IBEAM;
            break;
        case CursorShape::Hand:
            cursorName = IDC_HAND;
            break;
        case CursorShape::Arrow:
            cursorName = IDC_ARROW;
            break;
        default:
            return result;
        }

        HCURSOR hCursor = ::LoadCursorW(nullptr, cursorName);
        ICONINFO info{};
        if (::GetIconInfo(hCursor, &info))
        {
            result.hotSpot.x = static_cast<int>(info.xHotspot);
            result.hotSpot.y = static_cast<int>(info.yHotspot);
            BITMAP bmpinfo{};
            if (::GetObject(info.hbmMask, sizeof BITMAP, &bmpinfo))
            {
                result.size.x = bmpinfo.bmWidth;
                result.size.y = std::abs(bmpinfo.bmHeight);
                if (info.hbmColor == nullptr)
                    result.size.y /= 2;
                // assuming that the cursor is an IDC_ARROW
            }
            if (info.hbmColor)
                ::DeleteObject(info.hbmColor);
            ::DeleteObject(info.hbmMask);
        }
        return result;
    }

    void Platform::shellExecute(const IForm* form, const std::wstring_view file, const std::wstring_view params)
    {
        ::ShellExecuteW(
            form ? static_cast<const PlatformImplementation::Windows::FormWindow&>(form->wnd_window()).handle() : nullptr,
            L"", file.data(), params.data(), nullptr, SW_NORMAL);

    }

    std::wstring Platform::appDataPath()
    {
        wchar_t* path = nullptr;
        if (!SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path)))
        {
            ::CoTaskMemFree(path);
            return {};
        }
        std::wstring resutl = path;
        ::CoTaskMemFree(path);
        return resutl.append(L"\\");

    }

    void Platform::debugOutput(const std::wstring_view line)
    {
        std::wstring text{ line };
        text.push_back(L'\n');
        ::OutputDebugStringW(text.c_str());
    }

    ColorMode Platform::systemColorMode()
    {
        return PlatformImplementation::Windows::WindowsManager::appsColorMode();
    }
}
