module;
#include "Windows.Headers.h"
module ClaFi.Platform.Windows.FormWindow;

import ClaFi.App.Application;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine;

namespace ClaFi
{
    using namespace ClaFi::PlatformImplementation::Windows;

    // DWM attributes newer than the SDK this may build against, stated by number. A system that
    // does not know one refuses it, and the refusal changes nothing.
    constexpr DWORD k_dwmCornerPreference = 33;
    constexpr DWORD k_dwmBorderColor = 34;
    constexpr DWORD k_dwmCornerDoNotRound = 1;
    constexpr DWORD k_dwmColorNone = 0xFFFFFFFE;

    FormWindow::FormWindow(WindowsManager& owner, IForm& form, WindowRole role, IForm* parentForm)
        :
        Window{ owner, role,
            parentForm ? static_cast<FormWindow&>(parentForm->wnd_window()).handle() : nullptr },
        m_form{ form }
    {
        dwmEnableDarkMode();
        // MADE BEFORE ANY OTHER WINDOW THIS ONE OWNS, so it stands lowest among them: a hint or a
        // menu raised later stands above the shadow rather than under it.
        m_shadow = std::make_unique<ShadowWindow>(
            owner.appHandle(), WindowsManager::shadowClassName(), handle());
    }

    FormWindow::~FormWindow() = default;

    void FormWindow::show()
    {
        if (!m_dwmFrameDisabled)
        {
            m_dwmFrameDisabled = true;
            dwmDisableFrame();
        }
        if (m_frameStale)
        {
            m_frameStale = false;
            reframe();
        }
        Window::show();
    }

    void FormWindow::update()
    {
        paint();
    }

    void FormWindow::setFrame(const WindowFrame& value)
    {
        if (frameDesign() == value)
            return;
        const FrameMargins ringBefore = ring();
        const WindowFrame before = appliedFrame();
        Window::setFrame(value);
        const FrameMargins band = ring();
        if (band == ringBefore && appliedFrame() == before)
            return;
        // A WINDOW NOT SHOWN ONLY KEEPS THE DESIGN, and applies it on the show. The first design
        // arrives from the form's own constructor, before the form has its content, and a window
        // is placed before it is shown - the placement reads the ring, and the resize it sends
        // tells the form the frame.
        if (!::IsWindowVisible(handle()))
        {
            m_frameStale = true;
            return;
        }

        // A RING THAT GREW OR SHRANK MOVES THE WINDOW RECT and leaves the geometry where it was.
        if (band != ringBefore)
        {
            IntRect rect = bounds();
            rect.left += ringBefore.left - band.left;
            rect.top += ringBefore.top - band.top;
            rect.right += band.right - ringBefore.right;
            rect.bottom += band.bottom - ringBefore.bottom;
            ::SetWindowPos(handle(), nullptr, rect.left, rect.top, rect.width(), rect.height(),
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        reframe();
    }

    // THE WINDOW KEEPS ITS OWN DIRTY RECT AND RAISES ITS OWN PAINT. It has no redirection surface,
    // so the update region the system keeps for it and the rect BeginPaint answers with are not
    // about anything the window shows; what the form invalidated is recorded here, in the
    // surface, and painted when the posted paint message arrives or update() asks.
    void FormWindow::invalidateRect(const IntRect* value)
    {
        if (!handle())
            return;
        if (!value)
            m_surfaceWholeDirty = true;
        else
            m_surfaceDirty.unionWith(*value);
        schedulePaint();
    }

    ColorByte FormWindow::alpha()
    {
        return m_alpha;
    }

    void FormWindow::setAlpha(ColorByte value)
    {
        m_alpha = value;
        if (m_presenter)
            m_presenter->setOpacity(value / 255.0f);
        if (m_shadow)
            m_shadow->setAlpha(value);
    }

    void FormWindow::setStoredBounds(const IntRect& value)
    {
        m_lockDpiChange = true;
        setBounds(value);
        m_lockDpiChange = false;
    }

    // THE EDGES ARE THE GEOMETRY'S: the client area reaches past it by the ring, and a point in
    // the ring is on the shadow, outside the window the user sees. Each edge's zone runs from the
    // ring outside it to a frame's width inside it; a corner reaches further along both edges
    // that meet in it, being the smaller target.
    LRESULT FormWindow::getBorderSizingHitTest(IntPoint mp)
    {
        LRESULT result = HTCLIENT;
        if (WindowRole::Dialog == role() || WindowRole::Menu == role())
        {
            const int inside = ::GetSystemMetrics(SM_CXFRAME) * 2;
            const int corner = inside * 4;
            const IntRect client = WindowBase::clientRect();
            const FrameMargins band = ring();
            const IntRect window{
                band.left, band.top, client.right - band.right, client.bottom - band.bottom };
            const bool nearLeft = mp.x < window.left + corner;
            const bool nearRight = mp.x >= window.right - corner;
            const bool nearTop = mp.y < window.top + corner;
            const bool nearBottom = mp.y >= window.bottom - corner;

            if (mp.y < window.top + inside)
            {
                if (nearLeft)
                    result = HTTOPLEFT;
                else if (nearRight)
                    result = HTTOPRIGHT;
                else
                    result = HTTOP;
            }
            else if (mp.y >= window.bottom - inside)
            {
                if (nearLeft)
                    result = HTBOTTOMLEFT;
                else if (nearRight)
                    result = HTBOTTOMRIGHT;
                else
                    result = HTBOTTOM;
            }
            else if (mp.x < window.left + inside)
            {
                if (nearTop)
                    result = HTTOPLEFT;
                else if (nearBottom)
                    result = HTBOTTOMLEFT;
                else
                    result = HTLEFT;
            }
            else if (mp.x >= window.right - inside)
            {
                if (nearTop)
                    result = HTTOPRIGHT;
                else if (nearBottom)
                    result = HTBOTTOMRIGHT;
                else
                    result = HTRIGHT;
            }
        }
        // A menu is not sized: its edges are non-client and nothing more.
        if (WindowRole::Menu == role() && result != HTCLIENT)
            result = HTBORDER;
        return result;
    }

    void FormWindow::wndProc(WinApiMsg& msg)
    {
        Window::wndProc(msg);
        switch (msg.msg)
        {
        // THE SYSTEM ASKING - Alt+F4, the taskbar, a shutdown. The form closes itself the way
        // its own close button does, so what happens on a close happens on every close.
        case WM_CLOSE:
            m_form.wnd_closeRequested();
            msg.handled = true;
            msg.result = 0;
            break;

        case WM_DESTROY:
            // PostQuitMessage is called from WindowsManager.windowDeleted();
            //if (&m_form == appContext().mainForm())
            //    ::PostQuitMessage(0);
            break;

        case WM_SYSCOMMAND:
            // The low four bits belong to the system, so the command is what is left of wParam
            // above them. A maximize from a double-click on the caption carries something in
            // them; one sent from our own code does not.
            switch (msg.wParam & 0xFFF0)
            {
            case SC_MINIMIZE:
                m_form.wnd_minimize();
                break;
            case SC_RESTORE:
                m_form.wnd_restore();
                break;
            case SC_MAXIMIZE:
                m_form.wnd_maximize();
                break;
            case SC_KEYMENU:
                // Alt or F10 on its own, with lParam carrying the character when one came with
                // it. DefWindowProc answers the bare press by entering the menu loop, which is
                // modal: it takes the messages the window would otherwise get, so the pointer
                // stops moving the hover until the next Alt leaves the loop again. There is no
                // menu bar for it to open - the title bar is the form's own - so the loop has
                // nothing to do but swallow input, and Alt is left free to do what every other
                // key does and show where the focus is. Alt with a character still goes to
                // DefWindowProc, which is what keeps Alt+Space opening the system menu, and
                // Alt+F4 arrives as SC_CLOSE rather than through here at all.
                if (!msg.lParam)
                    msg.handled = true;
                break;
            }
            break;

        // THE WHOLE WINDOW IS THE CLIENT AREA: the frame is the framework's own, drawn in the
        // surface. Maximized included, on purpose: the system sizes a maximized window to the
        // work area plus its resize borders, so the client hangs off the monitor by that much,
        // and a control at the window's edge reaches past the screen's. A pointer driven into a
        // corner or against an edge is then inside the control standing there - the close button,
        // a scrollbar's thumb - rather than a few pixels short of it.
        case WM_NCCALCSIZE:
            msg.result = 0;
            msg.handled = true;
            return;

        case WM_NCLBUTTONDOWN:
        {
            // The caption and the frame are the system's to act on, and the framework is told
            // rather than asked: it has a popup to take down before the window moves. The
            // position is not passed on - the press landed outside everything the form owns.
            if (isFrameHitTest(msg.wParam) || isSystemHitTest(msg.wParam))
            {
                if (!m_form.wnd_systemMouseDown())
                {
                    // Refused, so the press does nothing at all: the window neither moves nor
                    // sizes while the question standing over it is unanswered.
                    msg.result = 0;
                    msg.handled = true;
                }
                break;
            }

            IntPoint mp = msg.mousePos();
            ::ScreenToClient(handle(), reinterpret_cast<POINT*>(&mp));
            // Windows puts no number on a message for a client to name a request by, so every
            // press raised from here carries an empty stamp.
            m_form.wnd_ncMouseDown(toSurface(mp).toFloat(), InputStamp{});
            msg.result = 0;
            msg.handled = true;
            break;
        }

        case WM_NCLBUTTONUP:
        {
            if (!isFrameHitTest(msg.wParam))
            {
                IntPoint mp = msg.mousePos();
                ::ScreenToClient(handle(), reinterpret_cast<POINT*>(&mp));
                m_form.wnd_mouseUp(toSurface(mp).toFloat());
                if (!isSystemHitTest(msg.wParam))
                {
                    msg.result = 0;
                    msg.handled = true;
                }
            }
            break;
        }

        case WM_NCMOUSEMOVE:
        {
            if (!isFrameHitTest(msg.wParam))
            {
                IntPoint pt = msg.mousePos();
                pt -= bounds().topLeft();
                m_form.wnd_mouseMove(toSurface(pt).toFloat());
                // TODO: WM_NCMOUSELEAVE
                if (!isSystemHitTest(msg.wParam))
                {
                    msg.result = 0;
                    msg.handled = true;
                }
            }
            break;
        }

        // A RIGHT CLICK ON THE CAPTION IS THE WINDOW MENU, and it is raised from here rather than
        // left to DefWindowProc, which measures the caption itself and finds none on a window that
        // is all client area. The press is remembered and kept from DefWindowProc; the release
        // over the caption is handed to the form as a context menu, which raises the window menu
        // when nothing on the title bar claims the point - see FormBase::wnd_contextMenu.
        case WM_NCRBUTTONDOWN:
            if (msg.wParam == HTCAPTION)
            {
                m_captionRightPressed = true;
                msg.result = 0;
                msg.handled = true;
            }
            break;

        case WM_NCRBUTTONUP:
        {
            const bool pressed = m_captionRightPressed;
            m_captionRightPressed = false;
            if (pressed && msg.wParam == HTCAPTION)
            {
                IntPoint mp = msg.mousePos();
                ::ScreenToClient(handle(), reinterpret_cast<POINT*>(&mp));
                PointInForm pt = toSurface(mp).toFloat();
                m_form.wnd_contextMenu(&pt, InputStamp{});
                msg.result = 0;
                msg.handled = true;
            }
            break;
        }

        case WM_RBUTTONUP:
            m_captionRightPressed = false;
            break;

        case WM_NCHITTEST:
        {
            // A HINT TAKES NO POINTER, and this is where it says so. WS_EX_TRANSPARENT says the
            // same thing and one more besides - it holds the window's own painting back until
            // every sibling beneath it has been painted, which starves a hint standing over a
            // form that is repainting under a drag. Said here, the pointer falls through and the
            // painting is the window's own business.
            if (role() == WindowRole::Tooltip)
            {
                msg.result = HTTRANSPARENT;
                msg.handled = true;
                break;
            }

            IntPoint mp = msg.mousePos();
            ::ScreenToClient(handle(), reinterpret_cast<POINT*>(&mp));

            msg.result = getBorderSizingHitTest(mp);
            msg.handled = msg.result != HTCLIENT;
            if (!msg.handled)
            {
                switch (m_form.wnd_hitTest(toSurface(mp).toFloat()))
                {
                case HitTest::Client:
                    msg.result = HTCLIENT;
                    break;
                case HitTest::Title:
                    msg.result = HTCAPTION;
                    break;
                case HitTest::MaxButton:
                    //debugBeep();
                    msg.result = HTMAXBUTTON;
                    break;
                case HitTest::Transparent:
                    msg.result = HTTRANSPARENT;
                    break;
                }
                msg.handled = msg.result != HTCLIENT;
            }
            break;
        }

        case WM_ERASEBKGND:
        {
            msg.result = 1;
            msg.handled = true;
            break;
        }

        case WM_SHOWWINDOW:
        {
            invalidateRect(nullptr);
            break;
        }

        // WM_PAINT IS THE SYSTEM'S, raised by a show or an uncover; it is validated, and whatever
        // the form has invalidated by then is painted.
        case WM_PAINT:
        {
            ::PAINTSTRUCT ps;
            ::BeginPaint(handle(), &ps);
            ::EndPaint(handle(), &ps);
            paint();
            msg.handled = true;
            break;
        }

        case WM_PRINT:
        case WM_PRINTCLIENT:
            {
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                m_form.wnd_beforePaint();
                const IntRect rcPaint = clientRect().toInt();
                //m_form.wnd_paint(rcPaint.toFloat());
                //m_form.wnd_canvas().paintTo(reinterpret_cast<CanvasHandle>(msg.wParam), rcPaint, rcPaint.topLeft());
            }
            msg.handled = true;
            break;

        case WM_MOUSEMOVE:
        {
            m_form.wnd_mouseMove(toSurface(msg.mousePos()).toFloat());
            ::TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof ::TRACKMOUSEEVENT;
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = handle();
            tme.dwHoverTime = 0;
            ::TrackMouseEvent(&tme);
            break;
        }
        case WM_MOUSELEAVE:
            m_form.wnd_mouseLeave();
            break;

        case WM_LBUTTONDOWN:
        {
            bool msgHandled = false;
            // The capture is taken either way: a third press can start a drag like any other, and
            // a form that reads it as a triple click still has to hear the moves that follow.
            if (takeTripleClick(msg.mousePos()))
                m_form.wnd_tripleClick(toSurface(msg.mousePos()).toFloat(), InputStamp{});
            else
                m_form.wnd_mouseDown(toSurface(msg.mousePos()).toFloat(), InputStamp{}, msgHandled);
            if (!msgHandled)
                ::SetCapture(handle());
            break;
        }

        case WM_LBUTTONUP:
            ::ReleaseCapture();
            m_form.wnd_mouseUp(toSurface(msg.mousePos()).toFloat());
            break;

        case WM_LBUTTONDBLCLK:
            m_doubleClickPending = true;
            m_doubleClickTime = ::GetMessageTime();
            m_doubleClickPos = msg.mousePos();
            m_form.wnd_doubleClick(toSurface(msg.mousePos()).toFloat(), InputStamp{});
            break;

        case WM_CONTEXTMENU:
        {
            IntPoint intPt = msg.mousePos();
            if (bool isMouseInput = intPt.x != -1 && intPt.y != -1)
            {
                // Mouse Input
                PointInForm pt = intPt.toFloat();
                pt.offset(-bounds().topLeft().toFloat());
                pt = toSurface(pt);
                m_form.wnd_contextMenu(&pt, InputStamp{});
            }
            else
            {
                // Keyboard Input
                m_form.wnd_contextMenu(nullptr, InputStamp{});
            }
            msg.handled = true;
            break;
        }

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        {
            constexpr float k_wheelDelta = 120.0f;

            IntPoint pt = msg.mousePos();
            ::ScreenToClient(handle(), reinterpret_cast<POINT*>(&pt));
            pt = toSurface(pt);
            float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(msg.wParam)) / k_wheelDelta;
            switch (msg.msg)
            {
            case WM_MOUSEWHEEL:
                m_form.wnd_mouseWheel(pt.toFloat(), delta);
                break;
            case WM_MOUSEHWHEEL:
                m_form.wnd_mouseHWheel(pt.toFloat(), delta);
                break;
            }
            break;
        }

        // Alt combinations arrive as WM_SYSKEYDOWN rather than WM_KEYDOWN, so both are routed the
        // same way. msg.handled stays false unless a control consumes the key, and staticWndProc
        // hands anything unhandled to DefWindowProc - which is what keeps Alt+F4 and Alt+Space
        // working. A bare Alt or F10 gets that far too, and is stopped at the WM_SYSCOMMAND it
        // turns into: see SC_KEYMENU above.
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            // Bit 30 of lParam is the previous key state: set means the key was already down,
            // which is the system repeating it rather than the user pressing it again.
            constexpr LPARAM k_previousKeyDown = 1 << 30;
            KeyDownEvent event{
                LOWORD(msg.wParam),
                Platform::keyModifiers(),
                (msg.lParam & k_previousKeyDown) != 0,
                msg.handled };
            m_form.wnd_keyDown(event);
            break;
        }

        case WM_SYSKEYUP:
        case WM_KEYUP:
            m_form.wnd_keyUp();
            break;

        case WM_CHAR:
            m_form.wnd_char(LOWORD(msg.wParam));
            break;

        // THE SURFACE FOLLOWS THE CLIENT AREA: less the ring, plus the margins. Minimized has no
        // area and nothing to lay out into. The form marks the whole surface and paints it inside
        // the call - see FormBase::wnd_resize - so nothing is invalidated after it: that would
        // post a second whole paint behind the one just made.
        //
        // A STEP IS COMPOSED BEFORE THE NEXT ONE BEGINS. The paint the form posted arrives ahead
        // of the next pointer position and waits there for the compositor to show this step, so
        // every step of a drag begins right after a composition with a whole period ahead of it.
        // Steps begun at any moment land after the compositor has started its frame as often as
        // not and are shown a frame late, which is a resize that stutters - on a window whose
        // step costs less than a period, where the compositor is what sets the pace. A step that
        // changed nothing posted no paint and made no frame, and one nobody can see made none
        // either; neither has anything to wait for.
        case WM_SIZE:
            if (msg.wParam != SIZE_MINIMIZED)
            {
                const IntSize client{ LOWORD(msg.lParam), HIWORD(msg.lParam) };
                m_form.wnd_resize(surfaceSize(client), appliedFrame());
                if (m_paintScheduled && ::IsWindowVisible(handle()))
                    m_awaitComposition = true;
            }
            syncShadow(nullptr);
            break;

        // THE SHADOW MOVES FIRST. Placed from the position about to be taken, it reaches the
        // compositor in the same frame as the window; placed afterwards it trails by one.
        case WM_WINDOWPOSCHANGING:
        {
            const ::WINDOWPOS& pos = *reinterpret_cast<const ::WINDOWPOS*>(msg.lParam);
            if (m_shadow && !(pos.flags & SWP_NOMOVE))
            {
                const IntRect current = bounds();
                const IntSize size = pos.flags & SWP_NOSIZE ? current.dimensions() : IntSize{ pos.cx, pos.cy };
                const IntRect next = IntRect::fromDimensions({ pos.x, pos.y }, size);
                syncShadow(&next);
            }
            break;
        }

        case WM_WINDOWPOSCHANGED:
            syncShadow(nullptr);
            m_form.wnd_posChanged();
            break;

        // THE POSTED PAINT IS THE ONE THAT CLEARS THE FLAG. A paint made directly - update()
        // inside a resize step - leaves the posted one standing, so it is not posted twice.
        case k_paintMessage:
            m_paintScheduled = false;
            paint();
            // The wait is made after the paint, so a frame the paint made is waited for too.
            if (m_awaitComposition)
            {
                m_awaitComposition = false;
                ::DwmFlush();
            }
            msg.handled = true;
            break;

        case WM_DPICHANGED:
        {
            // THE SCALE FIRST, THEN THE RECT. The ring is scaled, and the resize the new rect
            // sends measures the surface with it; the form's own design follows through
            // wnd_setScalePercent and reframes once more with its new margins.
            const IntRect& r = *(IntRect*)msg.lParam;
            msg.result = 0;
            setScaleFactor(static_cast<float>(HIWORD(msg.wParam)) / 96.0f);
            if (!m_lockDpiChange)
                setBounds(r);
            const int newPercent = static_cast<int>(std::round(100.0f * scaleFactor()));
            m_form.wnd_setScalePercent(newPercent);
            break;
        }
        }
    }

    void FormWindow::focusChanged()
    {
        m_form.wnd_focusChanged();
    }

    bool FormWindow::isSystemHitTest(WPARAM value)
    {
        // Areas with HTCLIENT and HTMAXBUTTON hittest are processed by the framework.
        //
        // HTMAXBUTTON we only use to trigger the Windows 11 snap menu,
        // which popups when the mouse is hovered over the button,
        // but the clicks of that button we're handling manually.
        // Everything else, including the window resizing or dragging, is processed by the system
        return !(value == HTCLIENT || value == HTMAXBUTTON);
    }

    bool FormWindow::isFrameHitTest(WPARAM value)
    {
        // if the mouse is on the size box we ignoring the message and let the system handle it.
        return inRange(value, HTLEFT, HTBOTTOMRIGHT);
    }

    void FormWindow::dwmEnableDarkMode()
    {
        switch (role())
        {
        case WindowRole::Dialog:
        case WindowRole::Menu:
        case WindowRole::Tooltip:
            HWND h = handle();
            BOOL trueBool = TRUE;
            ::DwmSetWindowAttribute(h, DWMWA_USE_IMMERSIVE_DARK_MODE, &trueBool, sizeof BOOL);
            break;
        }
    }

    void FormWindow::dwmDisableFrame()
    {
        HWND h = handle();
        DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
        ::DwmSetWindowAttribute(h, DWMWA_NCRENDERING_POLICY, &policy, sizeof policy);
        ::DwmSetWindowAttribute(h, k_dwmCornerPreference, &k_dwmCornerDoNotRound, sizeof k_dwmCornerDoNotRound);
        ::DwmSetWindowAttribute(h, k_dwmBorderColor, &k_dwmColorNone, sizeof k_dwmColorNone);
    }

    bool FormWindow::takeTripleClick(IntPoint mousePos)
    {
        if (!m_doubleClickPending)
            return false;

        m_doubleClickPending = false;

        // The user's own settings, so a run of clicks is as fast and as steady as the one the
        // system already required of the second press. The drag box is stated as its whole width
        // and is centred on the press it is measured from, so half of it reaches each way.
        const LONG elapsed = ::GetMessageTime() - m_doubleClickTime;
        if (elapsed < 0 || static_cast<DWORD>(elapsed) > ::GetDoubleClickTime())
            return false;

        const int slackX = ::GetSystemMetrics(SM_CXDOUBLECLK) / 2;
        const int slackY = ::GetSystemMetrics(SM_CYDOUBLECLK) / 2;
        return std::abs(mousePos.x - m_doubleClickPos.x) <= slackX
            && std::abs(mousePos.y - m_doubleClickPos.y) <= slackY;
    }

    IntPoint FormWindow::toSurface(IntPoint clientPoint) const
    {
        return clientPoint + surfaceOrigin();
    }

    FloatPoint FormWindow::toSurface(FloatPoint clientPoint) const
    {
        return clientPoint + surfaceOrigin().toFloat();
    }

    IntRect FormWindow::clientArea() const
    {
        return IntRect::fromDimensions(surfaceOrigin(), WindowBase::clientRect().dimensions());
    }

    void FormWindow::reframe()
    {
        m_form.wnd_resize(surfaceSize(WindowBase::clientRect().dimensions()), appliedFrame());
        invalidateRect(nullptr);
        syncShadow(nullptr);
    }

    void FormWindow::syncShadow(const IntRect* windowRect)
    {
        if (!m_shadow)
            return;
        HWND wnd = handle();
        const bool shown = !appliedFrame().margins.empty() && ::IsWindowVisible(wnd) && !::IsIconic(wnd);
        if (!shown)
        {
            m_shadow->show(false);
            return;
        }
        const IntRect rect = windowRect ? *windowRect : bounds();
        const IntPoint origin = surfaceOrigin();
        m_shadow->moveTo({ rect.left - origin.x, rect.top - origin.y });
        // SHOWN RIGHT ABOVE ITS OWNER. Showing puts a window at the top of the owner's group, over
        // a hint or a menu standing in the shadow's band; it is put back under them.
        if (m_shadow->show(true))
            ::SetWindowPos(m_shadow->handle(), wnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    // THE FRAME IS THE WHOLE SURFACE, and the window shows the part inside its client area: from
    // the bitmap a CPU backend painted, or from the presenter's frame texture a GPU backend drew
    // into. The shadow window paints the margins for itself, from the form's painter, and paints
    // them again on every paint that reaches them: a paint reaching the margins is a size or a
    // design the shadow has to follow, a theme switch as much as a resize. The window's own area
    // is the hole.
    void FormWindow::present(const Graphics::Bitmap* bitmap, const IntRect& dirty)
    {
        const IntRect area = clientArea();
        const IntPoint origin = surfaceOrigin();

        const IntRect part = IntRect::intersection(dirty, area);
        if (!part.empty())
        {
            const IntPoint destination{ part.left - origin.x, part.top - origin.y };
            const bool shown = bitmap
                ? m_presenter->present(*bitmap, part, destination)
                : m_presenter->presentSurface(part, destination);
            // The device went: the next paint builds another and needs the whole surface.
            if (!shown)
                invalidateRect(nullptr);
        }

        const bool reachesMargins = dirty.left < area.left || dirty.top < area.top
            || dirty.right > area.right || dirty.bottom > area.bottom;
        const FrameMargins margins = appliedFrame().margins;
        if (!m_shadow || !reachesMargins || margins.empty())
            return;

        const IntSize surface = surfaceSize(area.dimensions());
        const FloatRect geometry = {
            static_cast<float>(margins.left),
            static_cast<float>(margins.top),
            static_cast<float>(surface.x - margins.right),
            static_cast<float>(surface.y - margins.bottom)
        };
        const IntRect window = bounds();
        const IntPoint position{ window.left - origin.x, window.top - origin.y };
        m_shadow->fill(m_form.wnd_shadowPainter(), surface, geometry, area, position, m_alpha);
    }

    // THE DIRTY RECT IS TAKEN BEFORE THE FORM IS CALLED, so what the form invalidates while it
    // paints - a placement that moved, an animation stepping - is kept for the next paint rather
    // than lost with this one. The presenter goes to the form as the paint's native context: a
    // GPU backend draws on its device, a CPU backend ignores it and hands its bitmap back.
    void FormWindow::paint()
    {
        // A WINDOW NOBODY CAN SEE IS NOT PAINTED. Nothing it paints reaches the screen, and the
        // show asks for the whole surface - see WM_SHOWWINDOW.
        if (!::IsWindowVisible(handle()))
            return;
        IntRect dirty = m_surfaceDirty;
        if (m_surfaceWholeDirty)
        {
            const IntSize surface = surfaceSize(WindowBase::clientRect().dimensions());
            dirty = { 0, 0, surface.x, surface.y };
        }
        m_surfaceDirty = {};
        m_surfaceWholeDirty = false;
        if (dirty.empty())
            return;

        if (!m_presenter)
        {
            m_presenter = std::make_unique<CompositionPresenter>(handle());
            m_presenter->setOpacity(m_alpha / 255.0f);
        }
        m_presenter->resize(WindowBase::clientRect().dimensions());

        m_form.wnd_beforePaint();
        // The form paints what this window shows. The margins past it are the shadow window's.
        IntRect shown = IntRect::intersection(dirty, clientArea());
        Graphics::Bitmap* bitmap = nullptr;
        if (!shown.empty())
            m_form.wnd_paint(m_presenter.get(), shown, bitmap);
        present(bitmap, dirty);
    }

    // ONE POSTED MESSAGE FOR ANY NUMBER OF INVALIDATIONS. Posted rather than left to WM_PAINT: a
    // posted message is taken in its turn among the input, where WM_PAINT waits for an empty
    // queue, and the paint it raises covers whatever accumulated by then.
    void FormWindow::schedulePaint()
    {
        if (m_paintScheduled || !handle())
            return;
        m_paintScheduled = true;
        ::PostMessageW(handle(), k_paintMessage, 0, 0);
    }

}
