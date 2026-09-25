module;
#include "Windows.Headers.h"
// OleInitialize and its pair, for the clipboard. WIN32_LEAN_AND_MEAN leaves OLE out of
// Windows.h, so it is named here.
#include <ole2.h>
// RegGetValueW, for the mode the desktop asks applications to be drawn in.
#pragma comment(lib, "advapi32.lib")
#ifndef WS_EX_NOREDIRECTIONBITMAP
#define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif
export module ClaFi.Platform.Windows.Window;

import ClaFi.Platform.Windows.Diagnostic;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.Context.FormContext;
import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Windows
{
    export class Window;
    using WindowsMap = std::unordered_map<HWND, Window*>;

    export class WindowsManager
    {
    public:
        explicit WindowsManager(HINSTANCE);
        ~WindowsManager();
        HINSTANCE appHandle() const { return m_appHandle; }
        // The class a shadow window is made from - no icon, no procedure of its own.
        [[nodiscard]] static const wchar_t* shadowClassName();
        // The mode the desktop asks applications to be drawn in, read afresh. See Platform
        [[nodiscard]] static ColorMode appsColorMode();
    private:
        static LRESULT __stdcall staticWndProc(HWND, UINT message, WPARAM, LPARAM);
        // Announces the desktop's mode where a change of its settings has moved it. See Platform
        static void settingChanged(LPARAM area);
    private:
        // The desktop's mode as last read, which is what a broadcast is compared against.
        inline static std::optional<ColorMode> s_appsColorMode{};
    private:
        const HINSTANCE m_appHandle;
        const HICON m_appIcon;
    };

    export class WindowBase
    {
    public:
        virtual ~WindowBase() = default;
        WindowBase();
        explicit WindowBase(const HWND);
        HWND handle() const { return m_handle; }
        void setHandle(const HWND);
        DWORD processId();
        bool checkStopped();
        std::wstring text() const;
        void setText(std::wstring_view value) const;
        IntRect bounds() const;
        IntRect clientRect() const;
    protected:
        virtual void handleChanged();
    private:
        HWND m_handle;
        DWORD m_processId;
    };

    export struct WinApiMsg
    {
        IntPoint mousePos() const { return { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; }
        //
        UINT msg;
        WPARAM wParam;
        LPARAM lParam;
        bool handled;
        LRESULT result;
    };

    // What the manager's window procedure delivers to - every window it made, by the pointer in
    // the window's user data - and what a message window hands its messages on to. See Platform
    export class IMessageSink
    {
    public:
        virtual ~IMessageSink() = default;
        virtual void wndProc(WinApiMsg&) = 0;
    };


    //-------------------------------------------------------------------------


    // HOW FAR INTO THE SHADOW A WINDOW CAN BE GRABBED TO BE SIZED, at 100%. On this platform it
    // is also how far the window itself reaches past its geometry: a pixel owns the pointer by
    // being inside the window rect, and this band of the shadow is drawn by the window for that.
    export constexpr int k_resizeGrab = 8;

    // A window's ordinary geometry in screen pixels, whether it stands maximized and whether it is
    // held above the others. See Platform
    export struct NormalPlacement
    {
        IntRect geometry{};
        bool maximized{};
        bool alwaysOnTop{};
    };

    export class Window : public WindowBase, public IPlatformWindow, public IMessageSink
    {
        friend WindowsManager;
    public:
        Window(WindowsManager& owner, WindowRole, HWND hWndParent);
        // Takes the window down with the object. A handle outliving the object it is registered
        // to is a live window whose user data names freed memory, and the next message the window
        // manager sends it - the z-order pass another popup owned by the same window sets off is
        // one - calls into that.
        ~Window() override;
    public:
        //
        WindowRole role() const { return m_Role; }
        // EVERY WINDOW HERE IS COMPOSED: it has no redirection surface, the platform presents the
        // form's frame through DirectComposition, the window wears the frame the form designs and
        // casts its shadow from a window of its own - see FormWindow. So every frame carries alpha.
        [[nodiscard]] bool wantsAlphaChannel() const override { return true; }
        //
        // IFormWindow ---->
        void doLoop() override;
        void show() override;
        void hide() override;
        void close() override;
        void minimize() override;
        void maximize() override;
        void restore() override;
        [[nodiscard]] bool isFocused() override { return s_focusedWnd == handle(); }
        [[nodiscard]] bool isMaximized() override;
        [[nodiscard]] bool isMinimized() override { return ::IsIconic(handle()); }
        void setFocus(InputStamp) override { ::SetForegroundWindow(handle()); }
        FloatRect clientRect();
        void invalidateRect(const IntRect*) override;
        void setTitle(const std::wstring_view value) override { setText(value); }
        void update() override { ::UpdateWindow(handle()); }
        void setSizeRange(ScaledDimensions minSize, ScaledDimensions maxSize) override;
        void setFrame(const WindowFrame& value) override { m_frameDesign = value; }
        [[nodiscard]] const WindowFrame& frameDesign() const { return m_frameDesign; }
        // THE FRAME THIS WINDOW APPLIES, in real pixels: the design, less the corners when
        // snapped, less everything when maximized.
        [[nodiscard]] WindowFrame appliedFrame() const;
        // HOW FAR THE WINDOW RECT REACHES PAST THE GEOMETRY on each side: k_resizeGrab scaled,
        // held within the margins, only on a window the user may size, and none while snapped.
        [[nodiscard]] FrameMargins ring() const;
        // WHERE THE WINDOW RECT'S TOP LEFT STANDS IN THE FORM'S SURFACE. The surface is the
        // geometry and its margins; the window is the geometry and its ring; the two share the
        // geometry, so this is margins less ring - and zero where there are neither.
        [[nodiscard]] IntPoint surfaceOrigin() const;
        // The form's surface for a client area of this size.
        [[nodiscard]] IntSize surfaceSize(IntSize clientSize) const;
        PlacedWindow place(const WindowPlacement&) override;
        ColorByte alpha() override;
        void setAlpha(ColorByte) override;
        void initiateWindowDrag(IntPoint pt, InputStamp) override;
        void showWindowMenu(PointInForm, InputStamp) override;
        [[nodiscard]] bool canSetAlwaysOnTop() const override { return true; }
        [[nodiscard]] bool isAlwaysOnTop() const override;
        void setAlwaysOnTop(bool value) override;
        // <---- IFormWindow
        // THE PLACEMENT A CONFIG REMEMBERED, taken instead of the default by every placement
        // until the window is shown, and shown maximized and held on top where it says so.
        void setNormalPlacement(const NormalPlacement& value) { m_normalPlacement = value; }
        // What the window would come back to now, as WINDOWPLACEMENT answers it.
        [[nodiscard]] NormalPlacement normalPlacement() const;
        // Removed from Platform API --->
        void setBounds(const IntRect& value); // not in interface anymore
        // <---
    protected:
        // setStoredBounds is overriden in FormWindow
        virtual void setStoredBounds(const IntRect&) {}
        void wndProc(WinApiMsg& msg) override;
        virtual void focusChanged() {}
        [[nodiscard]] float scaleFactor() const { return m_scaleFactor; }
        void setScaleFactor(float value) { m_scaleFactor = value; }
        [[nodiscard]] bool isArranged() const;
    private:
        // The ring an ordinary window wears - what a normal rect reaches past its geometry by.
        [[nodiscard]] FrameMargins normalRing() const;
        [[nodiscard]] FrameMargins ringWithin(const FrameMargins& margins) const;
    private:
        static HWND getFocusedWnd() { return s_focusedWnd; }
        static void setFocusedWnd(const HWND);
        static HWND createHandle(const WindowsManager& owner, WindowRole, HWND hWndParent);
    private:
        static HWND s_focusedWnd;
        WindowsManager* m_owner;
        WindowRole m_Role;
        // base handle() becomes 0 when window destroyed
        // so we keeping initial value in m_createdHandle to be able to find ourself in the windowMap
        HWND m_createdHandle{ 0 };
        // The range WM_GETMINMAXINFO answers with - see setSizeRange. Zero on an axis is nothing
        // stated, and the system's own default stands.
        IntPoint m_minTrackSize{};
        IntPoint m_maxTrackSize{};
        WindowFrame m_frameDesign{};
        float m_scaleFactor{ Platform::globalScaleFactor() };
        // The rect last asked for, which is the only record of where a window that has never
        // been shown stands - see place.
        IntRect m_placedBounds{};
        std::optional<NormalPlacement> m_normalPlacement{};
    };

    // A MESSAGE-ONLY WINDOW: an HWND of the message class under HWND_MESSAGE, never shown, with
    // none of what a platform window has - no frame, no placement, no alpha - so it stands on
    // WindowBase alone. The timers tick through one and the clipboard listens on one; its
    // messages go to the sink it is given, and what the sink leaves unhandled goes to the
    // system's default. See Platform
    export class MessageWindow : public WindowBase, public IMessageSink
    {
    public:
        explicit MessageWindow(WindowsManager& owner);
        ~MessageWindow() override;
        MessageWindow(const MessageWindow&) = delete;
        MessageWindow& operator=(const MessageWindow&) = delete;
        void setSink(IMessageSink* value) { m_sink = value; }
        void wndProc(WinApiMsg&) override;
    private:
        IMessageSink* m_sink{ nullptr };
    };

    // WHAT STANDS BEHIND IPlatformServices ON WIN32. The one platform of a Windows build derives
    // from this, so a service handed the platform under its neutral name casts to it here and
    // asks for a message window. See Platform
    export class IMessageWindowFactory : public IPlatformServices
    {
    public:
        [[nodiscard]] virtual std::unique_ptr<MessageWindow> createMessageWindow() = 0;
    };

}

//-----------------------------------------------------------------------------

namespace ClaFi::PlatformImplementation::Windows
{
    // Windows classes names
    constexpr wchar_t k_dialogClassName[] = L"ClaFi_Dlg";
    constexpr wchar_t k_popupClassName[] = L"ClaFi_Popup";
    // Message-only windows - a timer has no pixels and no styles worth sharing with a form.
    constexpr wchar_t k_messageClassName[] = L"ClaFi_Msg";
    constexpr wchar_t k_shadowClassName[] = L"ClaFi_Shadow";

    // WindowsManager

    WindowsManager::WindowsManager(HINSTANCE appHandle)
        :
        m_appHandle{ appHandle },
        m_appIcon{ ::LoadIcon(appHandle, MAKEINTRESOURCE(101)) } //IDI_APP_ICON;
    {
        ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        // THE APARTMENT THE CLIPBOARD LIVES IN. OleSetClipboard and OleGetClipboard work on an
        // OLE-initialised thread and nowhere else, and this is the thread that runs the message
        // loop. Failure is not fatal: everything except the clipboard carries on without it.
        ::OleInitialize(nullptr);

        ::WNDCLASSW wndClass;

        // One struct filled three times, so each class states its whole style rather than adding
        // to the one before it. CS_DBLCLKS is per class and is what makes Windows send
        // WM_LBUTTONDBLCLK at all: a class without it is told of the second press as another
        // WM_LBUTTONDOWN, and every gesture built on a double click is dead in that window.
        wndClass.style = CS_DBLCLKS;
        wndClass.lpfnWndProc = reinterpret_cast<WNDPROC>(WindowsManager::staticWndProc);
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra =  sizeof(Window*);
        wndClass.hInstance = appHandle;
        wndClass.hIcon = m_appIcon;
        wndClass.hCursor = 0;
        wndClass.hbrBackground = 0;
        wndClass.lpszMenuName = nullptr;
        wndClass.lpszClassName = k_dialogClassName;
        check(::RegisterClassW(&wndClass));

        wndClass.style = 0;
        wndClass.hIcon = 0;
        wndClass.lpszClassName = k_messageClassName;
        check(::RegisterClassW(&wndClass));

        // A popup holds controls that read a double click of their own - the in-place editor is a
        // text box in one, and selects a word on it. No system drop shadow: a popup casts its own
        // from its shadow window.
        wndClass.style = CS_DBLCLKS;
        wndClass.lpszClassName = k_popupClassName;
        check(::RegisterClassW(&wndClass));

        wndClass.style = 0;
        wndClass.lpfnWndProc = ::DefWindowProcW;
        wndClass.cbWndExtra = 0;
        wndClass.lpszClassName = k_shadowClassName;
        check(::RegisterClassW(&wndClass));
    }

    WindowsManager::~WindowsManager()
    {
        // The clipboard has gone by now, its package rendered onto the clipboard where this
        // process still owned it - see Win32Platform, which states the order.
        ::OleUninitialize();
        PostQuitMessage(0);
        ::UnregisterClassW(k_dialogClassName, m_appHandle);
        ::UnregisterClassW(k_messageClassName, m_appHandle);
        ::UnregisterClassW(k_popupClassName, m_appHandle);
        ::UnregisterClassW(k_shadowClassName, m_appHandle);
    }

    const wchar_t* WindowsManager::shadowClassName()
    {
        return k_shadowClassName;
    }

    // THE APP MODE, the half of the setting that speaks to applications - the system mode beside
    // it colours the taskbar and the Start menu. A value that cannot be read is a desktop stating
    // no preference, and that reads as Light, the mode Windows starts in.
    ColorMode WindowsManager::appsColorMode()
    {
        DWORD appsUseLightTheme = 1;
        DWORD size = sizeof(appsUseLightTheme);
        const LSTATUS status = ::RegGetValueW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"AppsUseLightTheme",
            RRF_RT_REG_DWORD,
            nullptr,
            &appsUseLightTheme,
            &size);
        s_appsColorMode = status == ERROR_SUCCESS && appsUseLightTheme == 0
            ? ColorMode::Dark
            : ColorMode::Light;
        return *s_appsColorMode;
    }

    LRESULT WindowsManager::staticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_SETTINGCHANGE)
            settingChanged(lParam);

        const LONG_PTR userData = ::GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (const auto wnd = reinterpret_cast<IMessageSink*>(userData))
        {
            WinApiMsg m{ message , wParam, lParam, false, 0 };
            wnd->wndProc(m);
            if (message == WM_DESTROY)
            {
                // don't delete the object on WM_DESTROY -
                // after a dialog executes we may want to read it
                return 0;
            }
            if (m.handled)
                return m.result;
        }
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }

    // ONE BROADCAST REACHES EVERY TOP-LEVEL WINDOW, so the mode is announced where it has moved
    // and not once for each window told. ImmersiveColorSet names a change of the accent colours
    // as well as of the mode, and the accent alone announces nothing.
    void WindowsManager::settingChanged(const LPARAM area)
    {
        const wchar_t* name = reinterpret_cast<const wchar_t*>(area);
        if (!name || std::wstring_view{ name } != L"ImmersiveColorSet")
            return;
        const std::optional<ColorMode> announced = s_appsColorMode;
        if (appsColorMode() == announced)
            return;
        Platform::events().emit<SystemColorModeEvent>();
    }

    // WindowBase

    WindowBase::WindowBase() :
        m_handle(0),
        m_processId(0)
    {
    }

    WindowBase::WindowBase(const HWND hWnd) : WindowBase()
    {
        setHandle(hWnd);
    }

    void WindowBase::setHandle(const HWND value)
    {
        m_handle = value;
        m_processId = 0;
        handleChanged();
    }

    DWORD WindowBase::processId()
    {
        if (m_handle != 0 && m_processId == 0)
            if (::IsWindow(m_handle))
                ::GetWindowThreadProcessId(m_handle, &m_processId);
        return m_processId;
    }

    bool WindowBase::checkStopped()
    {
        if (!m_handle)
            return true;
        if (!::IsWindow(m_handle))
            setHandle(0);
        return !m_handle;
    }

    std::wstring WindowBase::text() const
    {
        std::wstring result{};
        if (m_handle)
            if (int L = ::GetWindowTextLengthW(m_handle))
            {
                result.resize(L);
                ::GetWindowTextW(handle(), result.data(), L + 1);
            }
        return result;
    }

    void WindowBase::setText(std::wstring_view value) const
    {
        ::SetWindowTextW(m_handle, value.data());
    }

    IntRect WindowBase::bounds() const
    {
        IntRect result;
        ::GetWindowRect(handle(), reinterpret_cast<RECT*>(&result));
        return result;
    }

    IntRect WindowBase::clientRect() const
    {
        IntRect result;
        ::GetClientRect(handle(), reinterpret_cast<RECT*>(&result));
        return result;
    }

    void WindowBase::handleChanged()
    {
    }

    // Window

    Window::Window(WindowsManager& owner, WindowRole role, HWND hWndParent)
        :
        WindowBase{ createHandle(owner, role, hWndParent) },
        m_owner{ &owner },
        m_Role{ role }
    {
        m_createdHandle = handle();
        ::SetWindowLongPtr(m_createdHandle, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(static_cast<IMessageSink*>(this)));
    }

    Window::~Window()
    {
        if (!m_createdHandle)
            return;

        // The handle is this object's only while its user data still names this object. A handle
        // value is reused once its window has gone - close() destroys the window and leaves the
        // value behind - and destroying a stranger's window is worse than leaving one standing.
        if (reinterpret_cast<IMessageSink*>(::GetWindowLongPtr(m_createdHandle, GWLP_USERDATA))
            != static_cast<IMessageSink*>(this))
        {
            return;
        }

        // Cleared first, so the WM_DESTROY that follows reaches DefWindowProc rather than an
        // object half way through going down.
        ::SetWindowLongPtr(m_createdHandle, GWLP_USERDATA, 0);
        ::DestroyWindow(m_createdHandle);
    }

    void Window::doLoop()
    {
        ::MSG msg;
        while (handle() && ::GetMessageW(&msg, 0, 0, 0))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    void Window::show()
    {
        DWORD flags = 0;
        switch (m_Role)
        {
        case WindowRole::Tooltip:
            [[fallthrough]];
        case WindowRole::Menu:
            flags = SW_SHOWNA;
            break;
        case WindowRole::Dialog:
            if (isMaximized())
                flags = SW_SHOWMAXIMIZED;
            else
                flags = SW_SHOW;
            // THE REMEMBERED PLACEMENT ENDS HERE: what was placed from it is the window now, and
            // a placement asked for later starts from the window rather than from the config.
            if (m_normalPlacement)
            {
                if (m_normalPlacement->maximized)
                    flags = SW_SHOWMAXIMIZED;
                if (m_normalPlacement->alwaysOnTop)
                    setAlwaysOnTop(true);
                m_normalPlacement.reset();
            }
            break;
        }
        ::ShowWindow(handle(), flags);
    }

    void Window::hide()
    {
        ::ShowWindow(handle(), SW_HIDE);
    }

    // DESTROYED DIRECTLY, NOT THROUGH WM_CLOSE. That message is the system asking, and a form
    // window answers it by asking its form - which closes through this call. Sent from here it
    // would come round again.
    void Window::close()
    {
        ::DestroyWindow(handle());
        setHandle(0);
    }

    void Window::minimize()
    {
        SendMessageW(handle(), WM_SYSCOMMAND, SC_MINIMIZE, 0);
    }

    void Window::maximize()
    {
        SendMessageW(handle(), WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    }

    void Window::restore()
    {
        SendMessageW(handle(), WM_SYSCOMMAND, SC_RESTORE, 0);
    }

    bool Window::isMaximized()
    {
        ::WINDOWPLACEMENT data;
        memzero(data);
        data.length = sizeof ::WINDOWPLACEMENT;
        ::GetWindowPlacement(handle(), &data);
        return data.showCmd == SW_MAXIMIZE;
    }

    FloatRect Window::clientRect()
    {
        IntRect result;
        ::GetClientRect(handle(), reinterpret_cast<RECT*>(&result));
        return result.toFloat();
    }

    void Window::invalidateRect(const IntRect* value)
    {
        if (HWND wnd = handle())
            ::InvalidateRect(wnd, reinterpret_cast<const RECT*>(value), false);
    }

    void Window::setSizeRange(ScaledDimensions minSize, ScaledDimensions maxSize)
    {
        m_minTrackSize = minSize.toInt();
        // No maximum is the sentinel rather than a number, and zero here says the same thing to
        // WM_GETMINMAXINFO: leave the system's own default standing.
        m_maxTrackSize = {
            maxSize.x >= k_maxFloat ? 0 : static_cast<int>(maxSize.x),
            maxSize.y >= k_maxFloat ? 0 : static_cast<int>(maxSize.y)
        };
    }

    WindowFrame Window::appliedFrame() const
    {
        WindowFrame result{};
        if (::IsZoomed(handle()))
            return result;
        result.margins = m_frameDesign.margins;
        if (isArranged())
            return result;
        result.radius = m_frameDesign.radius;
        return result;
    }

    // A snapped window is exactly the rect it was snapped to, and its shadow lies outside it.
    FrameMargins Window::ring() const
    {
        if (isArranged())
            return {};
        return ringWithin(appliedFrame().margins);
    }

    IntPoint Window::surfaceOrigin() const
    {
        const FrameMargins margins = appliedFrame().margins;
        const FrameMargins band = ring();
        return { margins.left - band.left, margins.top - band.top };
    }

    IntSize Window::surfaceSize(IntSize clientSize) const
    {
        const IntSize margins = appliedFrame().margins.total();
        const IntSize band = ring().total();
        return {
            std::max(0, clientSize.x - band.x + margins.x),
            std::max(0, clientSize.y - band.y + margins.y)
        };
    }

    // SNAPPED TO A SIDE OR A CORNER OF THE SCREEN, which is Win32's tiled. Looked up by name: the
    // function arrived with Windows 10 1903 and the header hides it behind a version macro.
    bool Window::isArranged() const
    {
        using IsWindowArrangedFunc = BOOL(__stdcall*)(HWND);
        static const IsWindowArrangedFunc isWindowArranged = reinterpret_cast<IsWindowArrangedFunc>(
            ::GetProcAddress(::GetModuleHandleW(L"user32"), "IsWindowArranged"));
        return isWindowArranged && isWindowArranged(handle());
    }

    FrameMargins Window::normalRing() const
    {
        return ringWithin(m_frameDesign.margins);
    }

    FrameMargins Window::ringWithin(const FrameMargins& margins) const
    {
        if (m_Role != WindowRole::Dialog)
            return {};
        const int grab = static_cast<int>(std::lround(k_resizeGrab * m_scaleFactor));
        return {
            std::min(margins.left, grab),
            std::min(margins.top, grab),
            std::min(margins.right, grab),
            std::min(margins.bottom, grab)
        };
    }

    // WHERE A WINDOW GOES INSIDE THE RECTANGLE IT MAY STAND IN. The room above the anchor
    // against the room below it, the flip, and holding the result inside that rectangle.
    //
    // Pure geometry, and it knows nothing about screens: the anchor and the bound arrive in one
    // space and the answer comes back in it. That the caller passes a monitor work area is the
    // caller's business. File-local because it is Win32's alone - a display server that places
    // windows on the client's behalf answers all of this itself and is asked instead.
    static FloatRect placeWithin(const WindowPlacement& placement, const FloatRect& anchor,
        const FloatRect& bounds)
    {
        // Where a window is allowed to stand: the work area, held off its edges by the margin
        // the window states.
        const FloatRect screenArea{ bounds, -placement.screenMargin };

        FloatRect result;
        switch (placement.placement)
        {
        case FormPlacement::Default:
            result = screenArea.centerRect(placement.size.x, placement.size.y);
            break;

        case FormPlacement::ScreenRight:
            // The width is the window's own; the height and the right edge are the room's.
            result = {
                screenArea.right - placement.size.x,
                screenArea.top,
                screenArea.right,
                screenArea.bottom
            };
            break;

        case FormPlacement::OverText:
            // OverText places the window by its TEXT rather than by its box. The anchor is the
            // text being covered, and textOrigin is where this window's own text starts inside
            // it, so putting the second on the first lands the two texts on top of each other.
            result = FloatRect::fromDimensions(
                anchor.topLeft() - placement.textOrigin, placement.size);
            break;

        default: // top, bottom, mouse, mousePoint(contextMenu) cases
        {
            // The edge the window grows from on each side, and the room that side has between
            // that edge and the screen. A window on the pointer states no clearance and hangs
            // off the point in either direction; an anchored one stands clear of the rect it
            // drops from, so its two edges are the rect's own with the clearance outside them.
            const float belowY = anchor.bottom + placement.clearance;
            const float aboveY = anchor.top - placement.clearance;
            const float roomBelow = screenArea.bottom - belowY;
            const float roomAbove = aboveY - screenArea.top;

            FloatPoint size = placement.size;
            const bool preferUp = placement.placement == FormPlacement::Top;
            const bool fitsAbove = roomAbove >= size.y;
            const bool fitsBelow = roomBelow >= size.y;

            // One side holds it and the other does not: that one takes it. Both hold it: the
            // preference decides. Neither holds it: the larger side takes it, and the
            // preference is left to settle two sides of the same size.
            bool placeAbove{};
            if (fitsAbove != fitsBelow)
                placeAbove = fitsAbove;
            else if (fitsAbove || roomAbove == roomBelow)
                placeAbove = preferUp;
            else
                placeAbove = roomAbove > roomBelow;

            // Neither side holds it: the window takes the room that side had, down to the floor
            // its content states and no further. A window held up by a MinSize keeps that height
            // and stands over what it is placed on - the smaller fault when the alternative is
            // losing what falls past the cut - and one that states no floor takes the room it
            // was left. Nothing has to ask it: minSize is the answer.
            if (!fitsAbove && !fitsBelow)
            {
                const float room = placeAbove ? roomAbove : roomBelow;
                size.y = room > placement.minSize.y ? room : placement.minSize.y;
            }

            const float top = placeAbove ? aboveY - size.y : belowY;
            result = FloatRect::fromDimensions({ anchor.left, top }, size);
            break;
        }
        }
        // NEVER LARGER THAN THE ROOM IT MAY STAND IN. clampTo MOVES a rect inside its boundary
        // and leaves an oversized one oversized - deliberately, since something covering a whole
        // screen is meant to - so a size the work area cannot hold is cut here first. A window
        // wider or taller than the work area has its title bar off the top and its edges out of
        // reach, which no size a form asks for is worth.
        if (result.width() > screenArea.width())
            result.right = result.left + screenArea.width();
        if (result.height() > screenArea.height())
            result.bottom = result.top + screenArea.height();
        result.clampTo(screenArea);
        return result;
    }

    PlacedWindow Window::place(const WindowPlacement& placement)
    {
        // THE ANCHOR IS IN THE PARENT FORM'S SURFACE, whose top left stands surfaceOrigin before
        // the parent's window rect. A window is told where it stands and the windows standing on
        // it are not, so this is the one place the two meet. GetParent answers the OWNER for a
        // popup, which is the window the anchor was measured in.
        ::RECT parentRect{};
        IntPoint parentOrigin{};
        const HWND parentHandle = ::GetParent(handle());
        if (parentHandle)
        {
            ::GetWindowRect(parentHandle, &parentRect);
            // The user data names the SINK, and the owner is a Window - see FormWindow's
            // constructor - so the cast walks back through the base it was stored as.
            const auto sink = reinterpret_cast<IMessageSink*>(
                ::GetWindowLongPtr(parentHandle, GWLP_USERDATA));
            if (const Window* parent = static_cast<Window*>(sink))
                parentOrigin = parent->surfaceOrigin();
        }
        FloatRect anchor = placement.anchorRect;
        anchor.offset({
            static_cast<float>(parentRect.left - parentOrigin.x),
            static_cast<float>(parentRect.top - parentOrigin.y) });

        // The monitor it lands on is the one the anchor stands on. A toplevel stands on
        // nothing, so it takes the monitor of the window that raised it, and the primary one
        // when nothing raised it - or the monitor its remembered geometry stands on.
        const bool onAnchor = placement.placement != FormPlacement::Default
            && placement.placement != FormPlacement::ScreenRight;
        const bool remembered = m_normalPlacement && placement.placement == FormPlacement::Default;
        ::HMONITOR hMon{};
        if (remembered)
            hMon = ::MonitorFromRect(
                reinterpret_cast<const ::RECT*>(&m_normalPlacement->geometry),
                MONITOR_DEFAULTTONEAREST);
        else if (onAnchor)
        {
            const FloatPoint centre = anchor.center();
            const ::POINT pt{ static_cast<LONG>(centre.x), static_cast<LONG>(centre.y) };
            hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        }
        else
            hMon = ::MonitorFromWindow(
                parentHandle, parentHandle ? MONITOR_DEFAULTTONEAREST : MONITOR_DEFAULTTOPRIMARY);

        ::MONITORINFO monInfo{};
        monInfo.cbSize = sizeof ::MONITORINFO;
        ::GetMonitorInfoW(hMon, &monInfo);

        // THE RING IS SCALED FOR THE MONITOR IT LANDS ON, before that monitor's WM_DPICHANGED
        // has said so: the rect stated here is measured in that monitor's pixels.
        UINT dpiX{};
        UINT dpiY{};
        if (SUCCEEDED(::GetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) && dpiY)
            m_scaleFactor = dpiY / 96.0f;

        // What is placed is the geometry; the window rect reaches past it by the ring. A
        // remembered geometry stands where it was, held up to the floor the content states now;
        // a placement that would leave it wholly off screen is moved on by SetWindowPlacement.
        FloatRect result{};
        if (remembered)
        {
            result = m_normalPlacement->geometry.toFloat();
            result.right = std::max(result.right, result.left + placement.minSize.x);
            result.bottom = std::max(result.bottom, result.top + placement.minSize.y);
        }
        else
            result = placeWithin(
                placement, anchor, reinterpret_cast<IntRect&>(monInfo.rcWork).toFloat());
        IntRect windowRect = result.toInt();
        const FrameMargins band = ring();
        windowRect.left -= band.left;
        windowRect.top -= band.top;
        windowRect.right += band.right;
        windowRect.bottom += band.bottom;
        // The scale of wherever it lands reaches the form through WM_DPICHANGED, which the move
        // below sends while this call runs - see FormWindow::wndProc.
        // A PLACEMENT THAT AGREES WITH THE WINDOW IS THE NEGOTIATION SETTLING, and stating it
        // again is not free: SetWindowPlacement reports the position whether or not it moved,
        // and the resize reporting it puts the form's alignment back in question - see
        // FormBase::wnd_resize, which is where a pass that changed nothing costs the pass after
        // it. A maximized window answers with the rect it is maximized to and never matches,
        // which leaves that case as it was.
        //
        // WHAT IT IS COMPARED WITH DEPENDS ON WHETHER ANYONE CAN SEE IT. setBounds states the
        // NORMAL position, and a window that has never been shown goes on answering GetWindowRect
        // with the rect it was created at however often that is stated - so for one of those the
        // rect last asked for is the only record there is. A shown window cannot be held to that
        // record: the user may have moved it since, and only the window itself knows.
        const bool settled = ::IsWindowVisible(handle())
            ? windowRect == bounds()
            : windowRect == m_placedBounds;
        m_placedBounds = windowRect;
        if (!settled)
            setStoredBounds(windowRect);
        return { result.dimensions() };
    }

    ColorByte Window::alpha()
    {
        ColorByte result{ 255 };
        ::GetLayeredWindowAttributes(handle(), nullptr, &result, nullptr);
        return result;
    }

    void Window::setAlpha(ColorByte value)
    {
        ::SetLayeredWindowAttributes(handle(), {}, value, LWA_ALPHA);
    }

    // The stamp is unread here. Windows weighs a request like this against its own record of what
    // the user last did, so there is no number for a caller to name.
    void Window::initiateWindowDrag(IntPoint pt, InputStamp)
    {
        // The initiateWindowDrag() is used to process the MouseDown event before the window dragging.
        // The difference from just using HTCAPTION is that with just using HTCAPTION,
        // the MouseDown event you will not receive – and hence can't do anything before that.
        // It is used when the tabs are on the window title - clicking on them
        // first selects the tab and you still able to drag the window holding the mouse button.

        HWND hwnd = handle();
        const IntPoint origin = surfaceOrigin();
        LPARAM lParam = MAKELPARAM(pt.x - origin.x, pt.y - origin.y);

        // 1. let's 'release' the pressed mouse button.
        // It forces our control to quit off dragging state (if it was)
        ::SendMessageW(hwnd, WM_LBUTTONUP, 0, lParam);

        // 2. Now let's reenter the left button down state, and make Windows think that that's happened on the window caption.
        // And if the mouse is moving, now the window initiates the window dragging
        ::SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
    }

    // TRACKED FROM HERE, NOT BY THE SYSTEM. DefWindowProc raises the window menu for a right
    // click on a caption it measures itself, and WM_NCCALCSIZE has made the whole of this window
    // client area - it has no caption by that measure, whatever WM_NCHITTEST answers. So the menu
    // is the system's but the tracking is this window's, and the item states with it: the system
    // keeps them right only for a menu it raises. The command comes back as a WM_SYSCOMMAND, the
    // same message the system's own menu sends.
    void Window::showWindowMenu(PointInForm pt, InputStamp)
    {
        HMENU menu = ::GetSystemMenu(handle(), FALSE);
        if (!menu)
            return;

        const bool zoomed = ::IsZoomed(handle());
        const bool iconic = ::IsIconic(handle());
        const DWORD style = ::GetWindowLongW(handle(), GWL_STYLE);
        auto enable = [menu](UINT command, bool enabled) {
            ::EnableMenuItem(menu, command, MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
        };
        enable(SC_RESTORE, zoomed || iconic);
        enable(SC_MOVE, !zoomed && !iconic);
        enable(SC_SIZE, (style & WS_SIZEBOX) != 0 && !zoomed && !iconic);
        enable(SC_MINIMIZE, (style & WS_MINIMIZEBOX) != 0 && !iconic);
        enable(SC_MAXIMIZE, (style & WS_MAXIMIZEBOX) != 0 && !zoomed);
        ::SetMenuDefaultItem(menu, SC_CLOSE, FALSE);

        IntPoint screenPoint = pt.toInt() - surfaceOrigin();
        ::ClientToScreen(handle(), reinterpret_cast<::POINT*>(&screenPoint));
        const UINT command = static_cast<UINT>(::TrackPopupMenu(menu,
            TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0,
            handle(), nullptr));
        if (command)
            ::SendMessageW(handle(), WM_SYSCOMMAND, command,
                MAKELPARAM(screenPoint.x, screenPoint.y));
    }

    // A WINDOW PLACEMENT IS IN WORKSPACE COORDINATES: its origin is the top left of the work
    // area, not of the monitor, so a taskbar on the left or the top stands between that space and
    // the screen one every other call takes. This is the distance between the two - taken off a
    // screen rect on the way into WINDOWPLACEMENT and put back by the system on the way out. A
    // window with WS_EX_TOOLWINDOW is placed in screen coordinates and takes none.
    [[nodiscard]] static IntPoint workspaceOffset(HWND hwnd, const IntRect& rect)
    {
        const DWORD exStyle = ::GetWindowLongW(hwnd, GWL_EXSTYLE);
        if (WS_EX_TOOLWINDOW & exStyle)
            return {};

        const ::HMONITOR hMon = ::MonitorFromRect(
            reinterpret_cast<const ::RECT*>(&rect), MONITOR_DEFAULTTONEAREST);
        ::MONITORINFO monInfo{};
        monInfo.cbSize = sizeof ::MONITORINFO;
        if (!::GetMonitorInfoW(hMon, &monInfo))
            return {};

        return {
            static_cast<int>(monInfo.rcWork.left - monInfo.rcMonitor.left),
            static_cast<int>(monInfo.rcWork.top - monInfo.rcMonitor.top)
        };
    }

    bool Window::isAlwaysOnTop() const
    {
        const DWORD exStyle = ::GetWindowLongW(handle(), GWL_EXSTYLE);
        return WS_EX_TOPMOST == (WS_EX_TOPMOST & exStyle);
    }

    void Window::setAlwaysOnTop(const bool value)
    {
        ::SetWindowPos(
            handle(),
            value ? HWND_TOPMOST : HWND_NOTOPMOST,
            0, 0, 0, 0,
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    // THE NORMAL RECT IS IN WORKSPACE COORDINATES and reaches past the geometry by the ring an
    // ordinary window wears - both taken off here, so what is answered is what setBounds takes
    // back. A minimized window that was maximized is maximized; on top is read off the style.
    NormalPlacement Window::normalPlacement() const
    {
        ::WINDOWPLACEMENT pl;
        memzero(pl);
        pl.length = sizeof ::WINDOWPLACEMENT;
        ::GetWindowPlacement(handle(), &pl);

        IntRect geometry = reinterpret_cast<const IntRect&>(pl.rcNormalPosition);
        const IntPoint workspace = workspaceOffset(handle(), geometry);
        geometry.offset(workspace.x, workspace.y);
        const FrameMargins band = normalRing();
        geometry.left += band.left;
        geometry.top += band.top;
        geometry.right -= band.right;
        geometry.bottom -= band.bottom;

        const bool maximized = pl.showCmd == SW_SHOWMAXIMIZED
            || (pl.showCmd == SW_SHOWMINIMIZED && (pl.flags & WPF_RESTORETOMAXIMIZED));
        return { geometry, maximized, isAlwaysOnTop() };
    }

    void Window::setBounds(const IntRect& value)
    {
        const IntPoint workspace = workspaceOffset(handle(), value);
        IntRect normal = value;
        normal.offset(-workspace.x, -workspace.y);

        ::WINDOWPLACEMENT pl;
        pl.length = sizeof ::WINDOWPLACEMENT;
        pl.ptMaxPosition = { -1, -1 };
        pl.ptMinPosition = { -1, -1 };
        pl.rcNormalPosition = reinterpret_cast<const ::RECT&>(normal);
        bool wasMaximized = isMaximized();
        if (::IsWindowVisible(handle()))
        {
            pl.showCmd = wasMaximized ? SW_SHOWMAXIMIZED : SW_NORMAL;
        }
        else
            pl.showCmd = SW_HIDE;
        pl.flags = 0;
        ::SetWindowPlacement(handle(), reinterpret_cast<::WINDOWPLACEMENT*>(&pl));
    }

    void Window::wndProc(WinApiMsg& msg)
    {
        switch (msg.msg)
        {
        case WM_GETMINMAXINFO:
        {
            // WHERE A USER'S DRAG STOPS. The system fills the structure with its own defaults
            // before sending this, so an axis nothing was stated for is left exactly as it
            // arrived. The frame is the framework's own - WS_POPUP with a WM_NCCALCSIZE that
            // returns zero - so the window rectangle the system is asking about is the client
            // area, which is the geometry the sizes were measured against plus the ring.
            ::MINMAXINFO& info = *reinterpret_cast<::MINMAXINFO*>(msg.lParam);
            const IntSize band = ring().total();
            if (m_minTrackSize.x > 0)
                info.ptMinTrackSize.x = m_minTrackSize.x + band.x;
            if (m_minTrackSize.y > 0)
                info.ptMinTrackSize.y = m_minTrackSize.y + band.y;
            if (m_maxTrackSize.x > 0)
                info.ptMaxTrackSize.x = m_maxTrackSize.x + band.x;
            if (m_maxTrackSize.y > 0)
                info.ptMaxTrackSize.y = m_maxTrackSize.y + band.y;
            msg.result = 0;
            msg.handled = true;
            break;
        }

        case WM_MOUSEACTIVATE:
            switch (m_Role)
            {
            case WindowRole::Menu:
                msg.result = MA_NOACTIVATE;
                msg.handled = true;
                break;
            }
            break;

        case WM_DESTROY:
            setHandle(0);
            break;

        case WM_SETFOCUS:
            if (m_Role == WindowRole::Dialog)
            {
                Window::s_focusedWnd = handle();
                focusChanged();
            }
            break;

        case WM_KILLFOCUS:
            if (m_Role == WindowRole::Dialog)
                if (m_createdHandle == Window::s_focusedWnd)
                {
                    Window::s_focusedWnd = 0;
                    focusChanged();
                }
            break;
        }
    }

    void Window::setFocusedWnd(const HWND value)
    {
        s_focusedWnd = value;
    }

    HWND Window::createHandle(const WindowsManager& owner, WindowRole role, HWND hWndParent)
    {
        // NO REDIRECTION SURFACE - see CompositionPresenter.
        DWORD exStyle = WS_EX_NOREDIRECTIONBITMAP;
        DWORD style = WS_POPUP;

        const wchar_t* className = k_dialogClassName;

        switch (role)
        {
        case WindowRole::Dialog:
            style |= WS_SIZEBOX | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;
            break;
        case WindowRole::Menu:
            className = k_popupClassName;
            exStyle |= WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
            break;
        case WindowRole::Tooltip:
            className = k_popupClassName;
            // NOT TOPMOST. A tooltip window is OWNED by the window whose control it is about -
            // every form builds one - and an owned popup always stands above its owner, which is
            // the whole of what a hint needs. Topmost is what an unowned window needed instead,
            // and it put the hint over every other application on the screen as well.
            //
            // AND NOT WS_EX_TRANSPARENT. That style is two things at once: the pointer falls
            // through the window, and the window is not painted until every sibling beneath it
            // that this thread made has been painted. The second is what a hint cannot have - it
            // stands over a form that repaints continuously while a slider is dragged under it,
            // and its own paint is held back for as long as that lasts, however often it is
            // invalidated and updated. The pointer falls through by the hit test instead, which
            // is the half of it a hint wanted - see FormWindow's WM_NCHITTEST. Its fade goes
            // through the composition visual - see FormWindow::setAlpha.
            exStyle |= WS_EX_NOACTIVATE;
            break;
        }

        HWND h = ::CreateWindowExW(exStyle, className, nullptr, style,
                                           //
            CW_USEDEFAULT, SW_HIDE, CW_USEDEFAULT, CW_USEDEFAULT, // x, y, w, h
                                           // If an overlapped window is created with the WS_VISIBLE style bit set
                                           // and the x parameter is set to CW_USEDEFAULT, then the y parameter
                                           // determines how the window is shown. If the y parameter is CW_USEDEFAULT,
                                           // then the window manager calls ShowWindow with the SW_SHOW flag
                                           // after the window has been created. If the y parameter is some other value,
                                           // then the window manager calls ShowWindow with that value as the nCmdShow parameter.
                                           // If nWidth is set to c, the system ignores nHeight.
                                           // CW_USEDEFAULT is valid only for overlapped windows
                                           //
                                           hWndParent,
                                           0,  // hMenu,
                                           owner.appHandle(),
                                           nullptr // lpParam
        );
        check(h);
        return h;
    }

    HWND Window::s_focusedWnd{ 0 };

    // MessageWindow

    MessageWindow::MessageWindow(WindowsManager& owner)
        :
        WindowBase{ ::CreateWindowExW(0, k_messageClassName, nullptr, WS_POPUP, 0, 0, 0, 0,
            HWND_MESSAGE, nullptr, owner.appHandle(), nullptr) }
    {
        check(handle());
        ::SetWindowLongPtr(handle(), GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(static_cast<IMessageSink*>(this)));
    }

    // Cleared first, so the WM_DESTROY that follows reaches DefWindowProc rather than an object
    // half way through going down.
    MessageWindow::~MessageWindow()
    {
        ::SetWindowLongPtr(handle(), GWLP_USERDATA, 0);
        ::DestroyWindow(handle());
    }

    void MessageWindow::wndProc(WinApiMsg& msg)
    {
        if (m_sink)
            m_sink->wndProc(msg);
    }

}
