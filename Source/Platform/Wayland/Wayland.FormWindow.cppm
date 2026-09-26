module;
#include "Wayland.Headers.h"
export module ClaFi.Platform.Wayland.FormWindow;

import ClaFi.Platform.Wayland.Window;
import ClaFi.Platform.Wayland.ShmBuffer;

import ClaFi.Platform.Wayland.Display;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Wayland
{
    // Where a compositor event becomes something the framework understands. See Platform
    export class FormWindow : public Window
    {
    public:
        FormWindow(DisplayManager&, IForm&, WindowRole, IForm* parentForm);
        // THE FRAMEWORK'S OWN ASK - a tab on the title bar makes it on every drag event. Routed
        // through handOverToCompositor, so the form is told the button went up first: the
        // compositor swallows the real release, and a tab still in its drag would ask again on
        // every motion after the move, and be granted, the seat never having seen the release.
        void initiateWindowDrag(IntPoint, InputStamp) override;
    protected:
        // The compositor's close request goes to the form, which closes the way its own close
        // button does, so what happens on a close happens on every close.
        void closeRequested() override { m_form.wnd_closeRequested(); }
        void pointerMoved(IntPoint) override;
        void pointerButton(IntPoint, std::uint32_t time, InputStamp, bool down) override;
        void pointerContextMenu(IntPoint, InputStamp) override;
        void pointerWheel(IntPoint, float delta, bool horizontal) override;
        void pointerLeft() override;
        void keyPressed(const KeyPress&) override;
        void character(wchar_t) override;
        void keyReleased() override;
        void resized(IntSize, const WindowFrame&) override;
        void focusChanged(bool focused) override;
        void scaleChanged() override;
        void paint() override;
    private:
        // WHICH PRESS OF A RUN THIS IS. No display server counts them: Wayland says a button went
        // down and nothing more, so a double and a triple click are recognised here or nowhere.
        // Two limits, the same two every platform uses - a time since the last press and a box
        // around it.
        [[nodiscard]] int takeClickCount(IntPoint, std::uint32_t time);
        // WHICH EDGE OR CORNER THIS POINT SIZES THE WINDOW FROM, as an xdg_toplevel resize edge,
        // or zero for a point that sizes nothing. Only a window the user is allowed to size has
        // any: a menu and a hint are whatever their content came to.
        [[nodiscard]] std::uint32_t borderResizeEdges(IntPoint) const;
        // A length stated at 100%, in the real pixels this window is drawn in.
        [[nodiscard]] int scaledLength(int length) const
        {
            return length * scale120() / k_scaleUnit;
        }
        // GIVES THE PRESS TO THE COMPOSITOR, having first told the form it is over. See the
        // definition - the order of those two is the whole point.
        void handOverToCompositor(IntPoint, InputStamp, std::uint32_t edges);
    private:
        IForm& m_form;
        // The run of clicks in progress: when the last press landed, where, and how many have
        // been counted so far.
        std::uint32_t m_lastClickTime{ 0 };
        IntPoint m_lastClickPos{};
        int m_clickCount{ 0 };
        // A press landed on the title bar and has not moved yet. The drag does not start here:
        // a press on the title is also a click on whatever is drawn there, and starting a drag
        // on the press would mean no button in the title bar could ever be clicked.
        bool m_titlePressed{ false };
        IntPoint m_titlePressPos{};
        InputStamp m_titleStamp{};
        // The size the form was last told about, which is not the same question as the size the
        // window is. A configure names a size the window takes at once, so comparing the next one
        // against the window would find them equal and report nothing, leaving the form holding a
        // canvas of the size it was created at. This is what the comparison is against.
        IntSize m_reportedSize{ -1, -1 };
        WindowFrame m_reportedFrame{};
    };
}


//-----------------------------------------------------------------------------


namespace ClaFi::PlatformImplementation::Wayland
{
    constexpr std::uint32_t k_multiClickTimeMs = 400;
    constexpr int k_multiClickSlopPx = 4;
    // How far into the window an edge reaches, and how far along an edge counts as its corner.
    // AT 100%, AND SCALED WITH THE WINDOW. These are hit targets, so what matters is how big they
    // are to the person aiming at them - and every other thing they are aiming at grows with the
    // scale. Left fixed, the border a user can grab halves on a screen at 200%.
    constexpr int k_resizeBorder = 6;
    constexpr int k_resizeCornerFactor = 4;
    // How far a press on the title must travel before it is a drag rather than a click. Scaled
    // for the same reason: a hand that moves a millimetre has moved twice as many pixels.
    constexpr int k_dragThreshold = 4;

    FormWindow::FormWindow(DisplayManager& display, IForm& form, WindowRole role,
        IForm* parentForm)
        :
        Window{ display, role,
            parentForm ? static_cast<Window*>(&parentForm->wnd_window()) : nullptr },
        m_form{ form }
    {
    }

    void FormWindow::initiateWindowDrag(IntPoint pt, InputStamp stamp)
    {
        handOverToCompositor(pt, stamp, 0);
    }

    void FormWindow::pointerMoved(IntPoint pt)
    {
        // A PRESS ON THE TITLE THAT HAS TRAVELLED IS A DRAG. It is recognised here rather than on
        // the press, and against the press's own position, so the threshold measures the whole
        // gesture rather than the last step of it.
        if (m_titlePressed)
        {
            const IntPoint travel = pt - m_titlePressPos;
            const int threshold = scaledLength(k_dragThreshold);
            if (std::abs(travel.x) > threshold || std::abs(travel.y) > threshold)
            {
                handOverToCompositor(m_titlePressPos, m_titleStamp, 0);
                return;
            }
        }
        m_form.wnd_mouseMove(pt.toFloat());

        // THE EDGE NAMES THE SHAPE, AFTER THE FORM HAS. The form sets the cursor for the control
        // under the pointer on every move, and a point on the border is over some control too -
        // the frame is drawn by one - so the resize arrow is stated after it and is the one shown.
        // Win32 arranges the same precedence by answering HTLEFT to WM_NCHITTEST, which outranks
        // the client area's cursor. Only the border is stated here: off it, the form's shape is
        // the right one and stands.
        if (const std::uint32_t edges = borderResizeEdges(pt))
            display().setResizeCursor(edges);
    }

    void FormWindow::pointerButton(IntPoint pt, std::uint32_t time, InputStamp stamp, bool down)
    {
        if (!down)
        {
            m_titlePressed = false;
            m_form.wnd_mouseUp(pt.toFloat());
            return;
        }

        // AN EDGE IS THE COMPOSITOR'S TO DRAG, and the form is asked first because a form with
        // something open - a menu standing over the frame - answers that the press is its own.
        if (const std::uint32_t edges = borderResizeEdges(pt))
            if (m_form.wnd_systemMouseDown())
            {
                handOverToCompositor(pt, stamp, edges);
                return;
            }

        // A SECOND AND A THIRD PRESS ARE PRESSES. Each carries the stamp of the press it actually
        // is, because a drag begun from a double click is a real gesture and the compositor
        // weighs it against that press and no other.
        bool handled = false;
        const int clicks = takeClickCount(pt, time);

        // A DOUBLE CLICK ON THE TITLE TOGGLES MAXIMIZED. On Win32 that is DefWindowProc's answer
        // to WM_NCLBUTTONDBLCLK over HTCAPTION, and every dialog there carries WS_MAXIMIZEBOX;
        // here the compositor never sees the title, the frame being the client's, so the window
        // answers itself. The form is asked as for any press the system takes - see
        // wnd_systemMouseDown - and the press reaches nothing else: it is the window's.
        if (clicks == 2 && m_form.wnd_hitTest(pt.toFloat()) == HitTest::Title)
        {
            if (m_form.wnd_systemMouseDown())
            {
                if (isMaximized())
                    restore();
                else
                    maximize();
            }
            return;
        }

        if (clicks >= 3)
            m_form.wnd_tripleClick(pt.toFloat(), stamp);
        else if (clicks == 2)
            m_form.wnd_doubleClick(pt.toFloat(), stamp);
        else
            m_form.wnd_mouseDown(pt.toFloat(), stamp, handled);

        // ARMED, NOT ACTED ON. The press has already reached whatever is drawn on the title bar;
        // the drag begins only if the pointer then travels - see pointerMoved.
        if (!handled && m_form.wnd_hitTest(pt.toFloat()) == HitTest::Title)
        {
            m_titlePressed = true;
            m_titlePressPos = pt;
            m_titleStamp = stamp;
        }
    }

    void FormWindow::pointerContextMenu(IntPoint pt, InputStamp stamp)
    {
        // Taken by address because the pointer is what NAMES the control the menu is about. A
        // menu raised from the keyboard passes none, and the form asks the focus instead.
        PointInForm point = pt.toFloat();
        m_form.wnd_contextMenu(&point, stamp);
    }

    void FormWindow::pointerWheel(IntPoint pt, float delta, bool horizontal)
    {
        if (horizontal)
            m_form.wnd_mouseHWheel(pt.toFloat(), delta);
        else
            m_form.wnd_mouseWheel(pt.toFloat(), delta);
    }

    void FormWindow::pointerLeft()
    {
        m_form.wnd_mouseLeave();
    }

    void FormWindow::keyPressed(const KeyPress& press)
    {
        // The stamp travels with the press so a menu the key opens can be authorised by it - the
        // same reason a pointer press carries one. The form answers `handled` and nothing here
        // reads it: a compositor has already taken the keys that are its own before this client
        // is told anything, so there is no DefWindowProc to hand the rest to.
        bool handled = false;
        KeyDownEvent event{ press.key, press.modifiers, press.isRepeat, handled, press.stamp };
        m_form.wnd_keyDown(event);
    }

    void FormWindow::character(wchar_t value)
    {
        m_form.wnd_char(value);
    }

    void FormWindow::keyReleased()
    {
        m_form.wnd_keyUp();
    }

    void FormWindow::resized(IntSize value, const WindowFrame& frame)
    {
        // COMPARED AGAINST WHAT THE FORM WAS TOLD, not against the window. A configure resizes
        // the window at once, so the next one measured against the window would find them equal
        // and report nothing, leaving the form holding a canvas of the size it was created at.
        if (value == m_reportedSize && frame == m_reportedFrame)
            return;

        m_reportedSize = value;
        m_reportedFrame = frame;
        m_form.wnd_resize(value, frame);
    }

    void FormWindow::focusChanged(bool)
    {
        m_form.wnd_focusChanged();
    }

    void FormWindow::scaleChanged()
    {
        // The form re-measures everything it holds against the new scale. It is told in percent
        // because that is what every platform states it in - so 175% is held here as 210/120 and
        // leaves as 175.
        m_form.wnd_setScalePercent(scalePercent());
    }

    void FormWindow::paint()
    {
        // THE FORM SETTLES FIRST, AND THE FRAME IS TAKEN AFTER IT. A form sized by its content is
        // placed again in here, and a popup whose ask moved re-cuts its frames on the spot - see
        // Window::applySize - so a frame and a size taken earlier would be the ones from before
        // the cut, written at the wrong stride.
        m_form.wnd_beforePaint();

        const IntSize size = m_buffers.size();
        if (size.x <= 0 || size.y <= 0)
            return;

        // BOTH FRAMES ARE WITH THE COMPOSITOR, which is not a fault to work around. It means this
        // client is drawing faster than the screen refreshes, so the frame it would have drawn is
        // one nobody was going to see. The damage stands and the next callback draws it.
        ShmBuffers::Frame* frame = m_buffers.freeFrame();
        if (!frame)
            return;

        // TAKEN AND CLEARED BEFORE THE PAINT, not after. Anything the paint itself invalidates is
        // damage for the NEXT frame, and clearing afterwards would throw it away.
        //
        // THE FRAME'S OWN DAMAGE AND NOT THE WINDOW'S. This frame was last written two presents
        // ago, so what it has missed is everything since then and not everything since the last
        // one. The window's rect is cleared with it because it asks a different question - whether
        // a paint is owed at all - and this paint answers that.
        IntRect damage = frame->damage;
        frame->damage = {};
        clearDirtyRect();
        if (damage.empty())
            return;

        Graphics::Bitmap* backBuffer = nullptr;
        m_form.wnd_paint(nullptr, damage, backBuffer);
        if (!backBuffer || backBuffer->empty())
            return;

        // CLIPPED TO BOTH, because they can disagree for one frame. A configure resizes the pool
        // at once and the backend is resized when the form is laid out into the new size, so a
        // paint caught between the two has a buffer of one size and a bitmap of another.
        const int left = std::max(0, damage.left);
        const int top = std::max(0, damage.top);
        const int right = std::min({ damage.right, size.x, backBuffer->width() });
        const int bottom = std::min({ damage.bottom, size.y, backBuffer->height() });
        if (right <= left || bottom <= top)
            return;

        // ROW BY ROW AND NOT WHOLESALE. The two have different strides - the bitmap is as wide as
        // the backend made it, the frame as wide as the compositor asked for - so only a row is
        // contiguous in both.
        const Color* source = backBuffer->data();
        Color* target = static_cast<Color*>(frame->pixels);

        // THE BITMAP IS ALREADY PREMULTIPLIED, which is what wl_shm's ARGB means, and it is so
        // for free: an opaque colour drawn at coverage c over a CLEARED frame leaves its colour
        // scaled by c and an alpha equal to c, which is the same number. So a full-strength frame
        // needs no conversion at all, whichever format it is in - what XRGB8888 and ARGB8888 name
        // on a little-endian machine is the order Graphics::Color already holds.
        const std::uint32_t opacity = pixelAlpha();
        if (!m_buffers.hasAlphaChannel() || opacity == 255)
        {
            const std::size_t rowBytes = static_cast<std::size_t>(right - left) * sizeof(Color);
            for (int y = top; y < bottom; ++y)
                std::memcpy(target + static_cast<std::size_t>(y) * size.x + left,
                    source + static_cast<std::size_t>(y) * backBuffer->stride() + left,
                    rowBytes);
        }
        else
        {
            // Two channels per multiply, rounded. See Platform#window-opacity
            constexpr ColorAsUint k_evenBytes = 0x00FF00FFu;
            constexpr ColorAsUint k_half = 0x00800080u;
            for (int y = top; y < bottom; ++y)
            {
                const Color* in = source + static_cast<std::size_t>(y) * backBuffer->stride() + left;
                Color* out = target + static_cast<std::size_t>(y) * size.x + left;
                for (int x = left; x < right; ++x, ++in, ++out)
                {
                    const ColorAsUint pixel = in->asUint();
                    ColorAsUint blueRed = (pixel & k_evenBytes) * opacity + k_half;
                    ColorAsUint greenAlpha = ((pixel >> 8) & k_evenBytes) * opacity + k_half;
                    blueRed = ((blueRed + ((blueRed >> 8) & k_evenBytes)) >> 8) & k_evenBytes;
                    greenAlpha = (greenAlpha + ((greenAlpha >> 8) & k_evenBytes)) & ~k_evenBytes;
                    *out = Color{ blueRed | greenAlpha };
                }
            }
        }

        presentFrame(*frame, { left, top, right, bottom });
    }

    int FormWindow::takeClickCount(IntPoint pt, std::uint32_t time)
    {
        const bool inTime = time - m_lastClickTime <= k_multiClickTimeMs;
        const bool inBox =
            std::abs(pt.x - m_lastClickPos.x) <= k_multiClickSlopPx
            && std::abs(pt.y - m_lastClickPos.y) <= k_multiClickSlopPx;

        // The run never goes past three. A fourth press starts a new one, which is what Win32
        // does and what a text box reading triple-click-selects-the-line depends on.
        m_clickCount = (inTime && inBox && m_clickCount < 3) ? m_clickCount + 1 : 1;
        m_lastClickTime = time;
        m_lastClickPos = pt;
        return m_clickCount;
    }

    // THE SAME WIDTH THE SERVER STATES IT IN, deliberately. A display server's clock is 32 bits
    // of milliseconds and wraps every seven weeks; held in a wider type the subtraction below
    // stops wrapping with it, and one run of clicks in seven weeks is read as a single press.
    std::uint32_t FormWindow::borderResizeEdges(IntPoint pt) const
    {
        // A layer surface is sized by the compositor between its anchors, and has no edge the
        // user may take hold of.
        if (role() != WindowRole::Dialog || isLayerRole())
            return XDG_TOPLEVEL_RESIZE_EDGE_NONE;

        const IntSize size = m_buffers.size();
        if (size.x <= 0 || size.y <= 0)
            return XDG_TOPLEVEL_RESIZE_EDGE_NONE;

        // THE EDGES ARE THE WINDOW'S AND NOT THE BUFFER'S. The buffer reaches into the shadow by
        // the frame's margins, and the shadow is not the window: what the user sees as the edge
        // is the geometry, and that is the line the zones are laid along.
        const FrameMargins& margins = m_reportedFrame.margins;
        const IntRect window{ margins.left, margins.top, size.x - margins.right, size.y - margins.bottom };
        if (window.width() <= 0 || window.height() <= 0)
            return XDG_TOPLEVEL_RESIZE_EDGE_NONE;

        // A ZONE REACHES OUT INTO THE SHADOW AND IN OVER THE BORDER, so an edge is taken from
        // either side of the line it is drawn on. Past the outer reach is shadow and nothing else
        // - the input region ends there too, and a pointer that far out is over the window
        // underneath - but the answer is the same without relying on that.
        const int inside = scaledLength(k_resizeBorder);
        const int outside = scaledLength(k_resizeGrab);
        if (pt.x < window.left - outside || pt.x >= window.right + outside
            || pt.y < window.top - outside || pt.y >= window.bottom + outside)
            return XDG_TOPLEVEL_RESIZE_EDGE_NONE;

        // A CORNER REACHES FURTHER THAN THE EDGES THAT MEET IN IT, ALONG BOTH OF THEM. It is the
        // smaller target and the harder one to hit - an edge is a whole side long, a corner is a
        // few pixels square - so the slack goes where the aiming is hard. A pointer coming down
        // the left side finds the corner before it reaches the actual corner, and one coming
        // along the top finds the same corner, which is why both are tested for it.
        const int corner = inside * k_resizeCornerFactor;
        const bool nearLeft = pt.x < window.left + corner;
        const bool nearRight = pt.x >= window.right - corner;
        const bool nearTop = pt.y < window.top + corner;
        const bool nearBottom = pt.y >= window.bottom - corner;

        // TOP AND BOTTOM FIRST, so a point in the region where two of these overlap resolves the
        // same way whichever edge the pointer came in by.
        if (pt.y < window.top + inside)
        {
            if (nearLeft)
                return XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT;
            if (nearRight)
                return XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT;
            return XDG_TOPLEVEL_RESIZE_EDGE_TOP;
        }
        if (pt.y >= window.bottom - inside)
        {
            if (nearLeft)
                return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT;
            if (nearRight)
                return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT;
            return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
        }
        if (pt.x < window.left + inside)
        {
            if (nearTop)
                return XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT;
            if (nearBottom)
                return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT;
            return XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
        }
        if (pt.x >= window.right - inside)
        {
            if (nearTop)
                return XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT;
            if (nearBottom)
                return XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT;
            return XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
        }

        return XDG_TOPLEVEL_RESIZE_EDGE_NONE;
    }

    void FormWindow::handOverToCompositor(IntPoint pt, InputStamp stamp, std::uint32_t edges)
    {
        m_titlePressed = false;

        // THE FORM IS TOLD THE BUTTON WENT UP, AND IT DID NOT.
        //
        // From the moment the compositor takes over it holds the pointer, and every event until
        // the user lets go goes to it. The release among them never arrives here. Without this
        // line the framework is left believing the button is still down for the rest of the
        // session: whatever was pressed stays captured, and nothing responds to the pointer
        // again. The Win32 twin does the same thing inside initiateWindowDrag.
        m_form.wnd_mouseUp(pt.toFloat());

        if (edges)
            requestWindowResize(edges, stamp);
        else
            requestWindowMove(stamp);
    }
}
