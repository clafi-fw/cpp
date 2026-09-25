module;
#include "Wayland.Headers.h"
#include <cstdio>
export module ClaFi.Platform.Wayland.Window;

import ClaFi.Platform.Wayland.Display;
import ClaFi.Platform.Wayland.ShmBuffer;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Wayland
{
    // THE UNIT A SCALE IS STATED IN, here and in the protocol: 120 is 100%, so 175% is 210 and
    // 250% is 300. Every scale a compositor names is a whole number of these, which is why none
    // of this needs a float and why two windows on one screen cannot disagree by a rounding
    // error.
    export constexpr int k_scaleUnit = 120;

    // HOW FAR INTO THE SHADOW A WINDOW CAN BE GRABBED TO BE SIZED, in surface pixels. It is the
    // reach of the input region past the geometry, and the reach of the form window's edge test.
    export constexpr int k_resizeGrab = 8;

    // A toplevel's ordinary size in surface pixels, and whether it stands maximized. See Platform
    export struct RememberedPlacement
    {
        IntSize size{};
        bool maximized{};
    };

    // ONE WINDOW, AND WHAT A COMPOSITOR WILL AND WILL NOT ANSWER ABOUT IT. See Platform
    export class Window : public IPlatformWindow, public INativeEventSink
    {
    public:
        Window(DisplayManager&, WindowRole, Window* parent);
        ~Window() override;
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
    public:
        [[nodiscard]] WindowRole role() const { return m_role; }
        // A MENU OR A HINT: a window that stands on another one and is placed by a positioner
        // against it. A dialog is a toplevel and is placed by the compositor's own rule.
        [[nodiscard]] bool isPopupRole() const
        {
            return m_role == WindowRole::Menu || m_role == WindowRole::Tooltip;
        }
        // A WINDOW ON THE SCREEN'S EDGE: a layer surface rather than a toplevel, anchored by the
        // compositor to the edges it named and sized between them. It is what a Dialog placed
        // ScreenRight becomes on a compositor with the layer shell - decided at its first
        // placement, since a surface takes one role and keeps it. Nothing above this layer knows
        // the difference; the placement kind is the whole of what the form said.
        [[nodiscard]] bool isLayerRole() const { return m_layerSurface != nullptr; }
        [[nodiscard]] wl_surface* surface() const { return m_surface; }
        // A WINDOW IS COMPOSITED OVER WHAT IT STANDS ON when something of it is not a filled
        // rectangle: a popup fades and rounds its corners, and a toplevel whose frame has a corner
        // to round or a shadow to cast is the same case. Its frames then carry an alpha channel
        // and start cleared. A toplevel wearing no frame is opaque, keeps XRGB, and pays for
        // neither.
        //
        // EVERYTHING THE TRANSLUCENCY WORK TURNS ON HANGS OFF THIS ONE ANSWER: the ARGB frames,
        // the cleared base the backend paints from, and the premultiplied copy out.
        [[nodiscard]] bool isTranslucent() const
        {
            return isPopupRole() || m_frameDesign.radius > 0.0f || !m_frameDesign.margins.empty();
        }
        [[nodiscard]] bool wantsAlphaChannel() const override { return isTranslucent(); }
    public:
        void doLoop() override;
        void show() override;
        void hide() override;
        void close() override;
        // Sent and never confirmed: xdg_toplevel has no minimized state and the compositor never
        // reports one back, so this is a request with no answer and isMinimized cannot read it.
        void minimize() override;
        // A REQUEST, NOT A COMMAND. The compositor may refuse - a tiling one generally will - and
        // nothing is true until the configure that answers it arrives. That is why isMaximized
        // reads state written by the configure and never by this call, and why a title bar glyph
        // is one frame behind the click that changed it. One frame behind is correct: the window
        // is not maximized yet.
        void maximize() override;
        void restore() override;
        [[nodiscard]] bool isFocused() override { return m_activated; }
        [[nodiscard]] bool isMaximized() override { return m_maximized; }
        // ALWAYS FALSE. xdg_toplevel carries no minimized state; a compositor that iconifies a
        // window simply stops asking it for frames and never says why.
        [[nodiscard]] bool isMinimized() override { return false; }
        // Asks the compositor to raise this toplevel, and is refused here with no stamp.
        void setFocus(InputStamp) override;
        void invalidateRect(const IntRect*) override;
        void setTitle(const std::wstring_view) override;
        void update() override;
        void setSizeRange(ScaledDimensions minSize, ScaledDimensions maxSize) override;
        void setFrame(const WindowFrame&) override;
        // THE FRAME THIS WINDOW APPLIES, in real pixels: the design, less what the window's state
        // takes away. Maximized and fullscreen wear no frame at all; tiled keeps the shadow and
        // loses the corners; a layer surface loses both.
        [[nodiscard]] WindowFrame appliedFrame() const;
        PlacedWindow place(const WindowPlacement&) override;
        // NO PER-SURFACE OPACITY, so a window here is either up or it is not - a translucent one
        // would be a buffer carrying the alpha, which is the backend's business and not this
        // window's. The number is REMEMBERED AND ANSWERED even so, because what asks for it is a
        // fade: a tooltip is put away by animating its alpha to zero and is never told to hide,
        // so a platform that answers a constant leaves the window standing at full strength for
        // ever. Zero unmaps it and anything else maps it - the fade snaps at each end, and the
        // window going away is the half that matters.
        [[nodiscard]] ColorByte alpha() override { return m_alpha; }
        void setAlpha(ColorByte value) override;
        void initiateWindowDrag(IntPoint pt, InputStamp) override;
        void showWindowMenu(PointInForm, InputStamp) override;
        // KEEP ABOVE IS THE COMPOSITOR'S. No xdg request states it, the compositor toggles it from
        // its own menu and never tells the client, so nothing is set here and nothing is stored.
        [[nodiscard]] bool canSetAlwaysOnTop() const override { return false; }
        [[nodiscard]] bool isAlwaysOnTop() const override { return false; }
        void setAlwaysOnTop(bool) override {}
        // THE NAME THIS TOPLEVEL RESTORES BY in the application's session. Stated before the
        // first placement, which makes the toplevel and names it - see createToplevel.
        void setSessionName(std::string_view value) { m_sessionName = value; }
        // Named to a session opened after this window was made - see Platform
        void addToSession(std::string_view name);
        // What the config kept, asked for by the first placement and the first commit.
        void setRememberedPlacement(const RememberedPlacement& value) { m_remembered = value; }
        // What the config would keep now.
        [[nodiscard]] RememberedPlacement rememberedPlacement() const;
    public:
        // FROM THE DISPLAY, IN SURFACE COORDINATES. Each of these converts and hands on to the
        // protected twin below it, so the conversion happens once, in the class that owns the
        // scale, and a derived window never sees a surface coordinate at all. Final for the same
        // reason: overriding one of these would be overriding the conversion.
        void onPointerMoved(IntPoint surfacePoint) final;
        void onPointerButton(IntPoint surfacePoint, std::uint32_t time, InputStamp, bool down) final;
        void onPointerContextMenu(IntPoint surfacePoint, InputStamp) final;
        void onPointerWheel(IntPoint surfacePoint, float delta, bool horizontal) final;
        void onPointerLeft() final;
        // A KEY HAS NO COORDINATES TO CONVERT, and is routed the same way even so, so that a
        // derived window overrides one family of methods and not two.
        void onKeyPressed(const KeyPress&) final;
        void onCharacter(wchar_t) final;
        void onKeyReleased() final;
        void onIdle() final;
        // HOW MANY REAL PIXELS ONE SURFACE PIXEL IS, IN 120ths. Everything the compositor states
        // - a configure size, a pointer position - is in SURFACE coordinates; everything the
        // framework works in is real pixels. This is the one number between them.
        //
        // 120ths because a screen at 175% is not an integer and the protocol says so in the same
        // unit. Held as a whole number of them rather than as a float so that two windows on one
        // screen cannot disagree by a rounding error.
        [[nodiscard]] int scale120() const { return m_scale120; }
        [[nodiscard]] int scalePercent() const { return m_scale120 * 100 / k_scaleUnit; }
        // ROUNDED TO NEAREST. A point between two real pixels belongs to one of them, and the
        // hit test that reads this has to land on the same one every time - which is what makes
        // the rule matter more than the choice.
        [[nodiscard]] IntPoint toBufferPoint(IntPoint surfacePoint) const
        {
            return {
                (surfacePoint.x * m_scale120 + k_scaleUnit / 2) / k_scaleUnit,
                (surfacePoint.y * m_scale120 + k_scaleUnit / 2) / k_scaleUnit
            };
        }
    protected:
        // WHAT A WINDOW IS TOLD, ALL IN REAL PIXELS. These are the extension points; the ones
        // above are the boundary.
        virtual void pointerMoved(IntPoint) {}
        virtual void pointerButton(IntPoint, std::uint32_t time, InputStamp, bool down) {}
        virtual void pointerContextMenu(IntPoint, InputStamp) {}
        virtual void pointerWheel(IntPoint, float delta, bool horizontal) {}
        virtual void pointerLeft() {}
        virtual void keyPressed(const KeyPress&) {}
        virtual void character(wchar_t) {}
        virtual void keyReleased() {}
        virtual void resized(IntSize, const WindowFrame&) {}
        virtual void focusChanged(bool focused) {}
        virtual void scaleChanged() {}
        // The compositor has been asked to close this window. Taking it at its word is the
        // default; a form with unsaved work is what would refuse.
        virtual void closeRequested() { close(); }
    protected:
        [[nodiscard]] const IntRect& dirtyRect() const { return m_dirtyRect; }
        void clearDirtyRect() { m_dirtyRect = {}; }
        virtual void paint() {}
        // ATTACHES A FRAME AND ASKS FOR IT TO BE SHOWN. The damage is in BUFFER coordinates,
        // which is what survives a scale change - surface coordinates would not.
        void presentFrame(ShmBuffers::Frame&, const IntRect& damage);

        // THE TWO REQUESTS THAT HAND THE WINDOW TO THE COMPOSITOR - to be moved, or to be sized
        // from the edge or corner named. Both are refused on the same terms: a request naming no
        // input event is one the compositor would reject, so it is not sent. From the moment
        // either is accepted the compositor holds the pointer and this client sees nothing of the
        // gesture, THE RELEASE INCLUDED, so a caller has to have settled the form's own press
        // before asking - see FormWindow::handOverToCompositor.
        void requestWindowMove(InputStamp);
        void requestWindowResize(std::uint32_t edges, InputStamp);
        [[nodiscard]] DisplayManager& display() const { return m_display; }
        wl_surface* m_surface{ nullptr };
        // THE SIZE IN REAL PIXELS, which is what the buffer is and what the framework lays out
        // into. Not what a configure names - see m_surfaceSize.
        IntSize m_size{ 0, 0 };
        ShmBuffers m_buffers;
    private:
        // THE ROLE A DIALOG'S SURFACE TAKES, made at its first placement rather than in the
        // constructor: that is the first moment the window knows what it is. The popup roles
        // follow the same rule in placePopup, for the same reason.
        void createToplevel();
        void createLayerSurface(int surfaceWidth);
        // Whether the surface has been given a role at all. Nothing with none is on screen.
        [[nodiscard]] bool hasRole() const
        {
            return m_toplevel || m_popup || m_layerSurface;
        }
        void ackConfigure(std::uint32_t serial);
        PlacedWindow placePopup(const WindowPlacement&);
        void createPopup();
        void destroyPopupRole();
        // What the compositor is told to place this window by. The caller owns the result and
        // destroys it once the popup has been made from it.
        [[nodiscard]] xdg_positioner* buildPositioner(const WindowPlacement&) const;
        // THE MARGINS IN SURFACE PIXELS, whole ones - the design's real pixels rounded up at this
        // scale, or none for a window that fills its screen or is docked to its edges.
        [[nodiscard]] FrameMargins surfaceMargins() const;
        // The surface is the geometry and its margins; the buffer is the surface in real pixels.
        void updateSurfaceSize();
        // The geometry's size in real pixels - what the form is placed at.
        [[nodiscard]] IntSize geometryPixels() const;
        // The surface size a size asked for in real pixels is stated as - see toSurfaceHolding.
        [[nodiscard]] IntSize surfaceSizeFor(ScaledDimensions asked) const;
        // Where the window stands in its surface, what of the surface is solid, and what of it
        // takes the pointer. Double-buffered state, applied by the commit that carries the frame.
        void stateGeometry();
        // THE SIZE THIS WINDOW HOLDS, MADE TRUE EVERYWHERE IT IS HELD: destination, geometry and
        // regions stated, frames re-cut, form told. For the paths that move m_size without a
        // configure, so that the configure following one of them reads m_size as the size
        // everything already is.
        void applySize();
        // A design that arrived once the frame was cut: the surface and the buffer follow it here
        // rather than waiting for a configure.
        void reframe();
        // THE PARENT'S SCALE. A positioner states everything in the parent surface's coordinates,
        // and a popup has no scale of its own until its first configure.
        [[nodiscard]] int parentScale120() const;
        [[nodiscard]] bool canReposition() const;
        void applyConfigure(std::uint32_t serial);
        void updateScaleFromOutputs();
        void applyConfigureless();
        void applyScaleToSurface();
        static void onSurfaceConfigure(void* data, xdg_surface*, std::uint32_t serial);
        static void onToplevelConfigure(void* data, xdg_toplevel*, std::int32_t width, std::int32_t height,
            wl_array* states);
        static void onToplevelClose(void* data, xdg_toplevel*);
        static void onToplevelSessionRestored(void* data, xdg_toplevel_session_v1*);
        static void onPopupConfigure(void* data, xdg_popup*, std::int32_t x, std::int32_t y,
            std::int32_t width, std::int32_t height);
        static void onPopupDone(void* data, xdg_popup*);
        static void onPopupRepositioned(void* data, xdg_popup*, std::uint32_t token);
        static void onLayerConfigure(void* data, zwlr_layer_surface_v1*, std::uint32_t serial,
            std::uint32_t width, std::uint32_t height);
        static void onLayerClosed(void* data, zwlr_layer_surface_v1*);
        static void onFrameDone(void* data, wl_callback*, std::uint32_t time);
        static void onSurfaceEnter(void* data, wl_surface*, wl_output*);
        static void onSurfaceLeave(void* data, wl_surface*, wl_output*);
        static void onPreferredBufferScale(void* data, wl_surface*, std::int32_t factor);
        static void onPreferredFractionalScale(void* data, wp_fractional_scale_v1*,
            std::uint32_t scale);
        static void onPreferredBufferTransform(void* data, wl_surface*, std::uint32_t transform);
    private:
        DisplayManager& m_display;
        const WindowRole m_role;
        Window* const m_parent;
        xdg_surface* m_xdgSurface{ nullptr };
        xdg_toplevel* m_toplevel{ nullptr };
        xdg_toplevel_session_v1* m_toplevelSession{ nullptr };
        std::string m_sessionName{};
        std::optional<RememberedPlacement> m_remembered{};
        xdg_popup* m_popup{ nullptr };
        zwlr_layer_surface_v1* m_layerSurface{ nullptr };
        // THE WIDTH THE LAYER SURFACE WAS LAST GIVEN, in surface pixels - stated again only when
        // a placement asks for another, since each statement is a configure round trip.
        int m_layerWidth{ 0 };
        // THE RANGE STATED BEFORE THERE WAS A TOPLEVEL TO STATE IT TO. A form measures before it
        // is placed, and the role is made at the placement; what it said is said again then.
        ScaledDimensions m_pendingMinSize{};
        ScaledDimensions m_pendingMaxSize{ k_maxFloat, k_maxFloat };
        // THE LAST PLACEMENT THIS WINDOW WAS GIVEN, which is the only one worth building a
        // positioner from - see onIdle.
        WindowPlacement m_placement{};
        // Counts the reposition requests, so the event answering one names which. A popup here
        // has a single placement in flight, so it is traced rather than matched.
        std::uint32_t m_repositionToken{ 0 };
        zxdg_toplevel_decoration_v1* m_decoration{ nullptr };
        // PRESENT TOGETHER OR NOT AT ALL. With them the compositor states the real ratio and the
        // buffer is presented at the size it asked for; without them the scale is whatever whole
        // number the outputs report.
        wp_viewport* m_viewport{ nullptr };
        wp_fractional_scale_v1* m_fractionalScale{ nullptr };
        // OUTSTANDING WHILE A FRAME IS OWED. The compositor answers it when this surface is about
        // to be drawn, which is the only moment another frame is worth producing. Painting on any
        // other schedule draws frames that are overwritten before a screen refresh reaches them.
        wl_callback* m_frameCallback{ nullptr };
        std::wstring m_title;
        // What a caller last asked for, and the whole of what opacity means here - see setAlpha.
        ColorByte m_alpha{ 255 };
        IntRect m_dirtyRect{};
        // WHAT THE LAST CONFIGURE PROPOSED, and not yet what is true. A configure is a proposal
        // in two halves - the toplevel names a size and a set of states, the surface then says
        // that is the whole of it - and none of it takes effect until the second half is acked.
        // WHAT THE COMPOSITOR NAMES, in surface coordinates: the window geometry, which is the
        // window as the user sees it. A configure sizes it, a positioner places it, the size range
        // bounds it.
        IntSize m_geometrySize{ 0, 0 };
        // THE GEOMETRY AND ITS MARGINS, in surface coordinates - what the viewport shows and what
        // the buffer covers. The buffer is this times the scale, and the two are the same number
        // only while the scale is 1.
        IntSize m_surfaceSize{ 0, 0 };
        int m_scale120{ k_scaleUnit };
        int m_pendingScale120{ k_scaleUnit };
        // THE OUTPUTS THIS SURFACE OVERLAPS. Read only on a compositor too old to name the scale
        // itself; the largest scale among them is what this window should draw at.
        std::vector<wl_output*> m_outputs;
        IntSize m_pendingSize{ 0, 0 };
        WindowFrame m_frameDesign{};
        // THE SIZE TO COME BACK TO. A compositor that un-maximizes a window states no size for
        // it - the client chose the size it had before, so the client is the one that remembers
        // it. Written on every configure that leaves the window ordinary, which is what makes it
        // the size before whatever came next.
        IntSize m_restoreSize{ 0, 0 };
        bool m_pendingMaximized{ false };
        bool m_pendingTiled{ false };
        bool m_pendingFullscreen{ false };
        bool m_pendingActivated{ false };
        bool m_configured{ false };
        bool m_unmapped{ false };   // off the screen after having been on it - see hide
        bool m_visible{ false };
        bool m_activated{ false };
        bool m_maximized{ false };
        bool m_tiled{ false };
        bool m_fullscreen{ false };
        bool m_registered{ false };
    };
}


//-----------------------------------------------------------------------------


namespace ClaFi::PlatformImplementation::Wayland
{
    // SURFACE PIXELS TO REAL ONES, ROUNDED UP. A buffer a fraction of a pixel short is a row the
    // compositor has to invent the edge of, so the fraction is always paid for.
    constexpr int toBufferLength(int surfaceLength, int scale120)
    {
        return (surfaceLength * scale120 + k_scaleUnit - 1) / k_scaleUnit;
    }

    // REAL PIXELS TO SURFACE ONES, ROUNDED TO NEAREST, for where something stands: a position has
    // no side to be rounded towards. A size is stated through toSurfaceHolding.
    constexpr int toSurfaceLength(int bufferLength, int scale120)
    {
        return (bufferLength * k_scaleUnit + scale120 / 2) / scale120;
    }

    // THE FEWEST SURFACE PIXELS THAT COME BACK AS NO LESS THAN THE REAL ONES ASKED FOR, between
    // margins of the surface lengths given - geometryPixels' arithmetic run backwards. The nearest
    // comes back a pixel short at some scales, and a grant short of the ask is content laid out
    // narrower than it measured; a grant past it is left transparent - see FormBase::contentExtent.
    static int toSurfaceHolding(float asked, int before, int after, int scale120)
    {
        const int pixels = static_cast<int>(std::ceil(asked));
        if (pixels <= 0)
            return 0;
        auto granted = [before, after, scale120](int length) {
            return toBufferLength(length + before + after, scale120)
                - toBufferLength(before, scale120) - toBufferLength(after, scale120);
        };
        // Fewer cannot hold it: a grant is less than a real pixel past its surface length's worth.
        int result = (pixels - 1) * k_scaleUnit / scale120 + 1;
        while (granted(result) < pixels)
            ++result;
        return result;
    }

    static const char* roleName(WindowRole value)
    {
        switch (value)
        {
        case WindowRole::Dialog:
            return "dialog";
        case WindowRole::Menu:
            return "menu";
        case WindowRole::Tooltip:
            return "tooltip";
        }
        return "?";
    }

    Window::Window(DisplayManager& display, WindowRole role, Window* parent)
        :
        m_buffers{ display.shm() },
        m_display{ display },
        m_role{ role },
        m_parent{ parent }
    {
        DisplayManager& manager = m_display;
        if (!manager.compositor())
            return;

        static const wl_surface_listener k_surfaceEventListener = {
            .enter = &Window::onSurfaceEnter,
            .leave = &Window::onSurfaceLeave,
            .preferred_buffer_scale = &Window::onPreferredBufferScale,
            .preferred_buffer_transform = &Window::onPreferredBufferTransform
        };
        m_surface = ::wl_compositor_create_surface(manager.compositor());
        ::wl_surface_add_listener(m_surface, &k_surfaceEventListener, this);

        // A HINT IS NOT SOMETHING THE POINTER CAN REACH. It stands over the very control it
        // is about, so a pointer that entered it would leave that control - and the form the
        // hint belongs to is the hint's own, whose tooltip is where the recursion stops and
        // has no form at all. An EMPTY input region tells the compositor to deliver the
        // pointer to whatever is underneath instead. Win32 says the same with
        // WS_EX_TRANSPARENT, which is why this never had to be said before.
        //
        // A MENU IS THE OPPOSITE and states nothing here: it is exactly what the pointer is
        // meant to reach, and its region is its geometry, stated with it - see stateGeometry.
        if (m_role == WindowRole::Tooltip)
        {
            wl_region* inputRegion = ::wl_compositor_create_region(manager.compositor());
            ::wl_surface_set_input_region(m_surface, inputRegion);
            ::wl_region_destroy(inputRegion);
        }

        // THE REAL RATIO, WHERE THE COMPOSITOR WILL STATE ONE. The viewport is what makes it
        // usable: a buffer drawn at 175% has to be presented at the size the compositor
        // asked for, and set_buffer_scale cannot say a fraction. Where neither exists the
        // whole-number scale from the outputs stands instead.
        if (manager.fractionalScaleManager())
        {
            static const wp_fractional_scale_v1_listener k_fractionalScaleListener = {
                .preferred_scale = &Window::onPreferredFractionalScale
            };
            m_viewport = ::wp_viewporter_get_viewport(manager.viewporter(), m_surface);
            m_fractionalScale = ::wp_fractional_scale_manager_v1_get_fractional_scale(
                manager.fractionalScaleManager(), m_surface);
            ::wp_fractional_scale_v1_add_listener(m_fractionalScale,
                &k_fractionalScaleListener, this);
        }

        // A POPUP IS DRAWN AT ITS PARENT'S SCALE AND TAKES NONE OF ITS OWN - see placePopup,
        // which reads it. NOT HERE: a hint's window is a member of the form it belongs to and is
        // built with it, which is before that form's own window has been configured and so before
        // the parent knows what scale it is drawn at. Copied here it would be the default for the
        // life of the application.

        // A HINT FADES, so its frames need a channel to fade in. Stated for every popup rather
        // than for the hint alone: a menu is small, its frames are written the same way, and one
        // rule about which windows carry alpha beats two. A toplevel's answer comes with its frame
        // - see setFrame, which restates this while no frame has been cut.
        m_buffers.useAlphaChannel(isTranslucent());

        // REGISTERED WITH ITS SURFACE, because that is what a pointer event names. The surface
        // has to exist first, which is why this is not the first thing the constructor does.
        manager.registerSink(*this, m_surface);
        m_registered = true;

        // NO ROLE YET. A surface takes one role and keeps it, and which one this window wants is
        // not known here: a menu or a hint stands on something and is placed against it by a
        // positioner, a dialog is a toplevel unless its placement names a screen edge - and the
        // form states its placement after the window is built, at its first placement. So every
        // role is made there: the popups in placePopup, the rest in place(). Until then the
        // surface shows nothing.
    }

    Window::~Window()
    {
        close();
    }

    void Window::doLoop()
    {
        DisplayManager& manager = m_display;
        while (m_registered && !manager.quitRequested())
        {
            manager.dispatchPending();
            manager.waitForEvent();
        }
    }

    // A WINDOW THAT HAS BEEN UNMAPPED IS MAPPED AGAIN THE WAY IT WAS MAPPED FIRST: a commit with
    // no buffer, which asks the compositor for the configure that the next frame answers. The
    // role stands this whole time - it is the surface's content that went and comes back - and
    // nothing is painted between here and that configure, which is what m_unmapped holds off.
    void Window::show()
    {
        m_visible = true;

        if (m_unmapped && hasRole())
            ::wl_surface_commit(m_surface);

        invalidateRect(nullptr);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] show    %p %-7s size=%dx%d  configured=%d\n",
                static_cast<void*>(this), roleName(m_role), m_size.x, m_size.y,
                m_configured ? 1 : 0);
    }

    // A SURFACE WITH NO BUFFER IS NOT ON SCREEN. There is no hide request: a window is unmapped
    // by taking its content away, and mapped again by giving it back.
    void Window::hide()
    {
        m_visible = false;

        // NOTHING WITH NO ROLE IS ON SCREEN TO BE TAKEN OFF IT. A tooltip's window is a member of
        // the form it belongs to, so it is built with that form and told its alpha is zero at
        // once - before anything has placed it and before the form's own window has been mapped.
        // A commit on a bare surface says nothing the protocol defines, and it is the moment a
        // compositor first hears that this surface exists: on a display server that gives every
        // surface a window of its own and orders them by age, it buys the hint a place at the
        // bottom of the stack for the life of the application.
        if (!m_surface || !hasRole())
            return;

        // AN UNMAPPED POPUP IS FINISHED. The protocol has no way to map one a second time, so
        // hiding a menu or a hint destroys its role and the next placement builds another - which
        // is what placePopup does whenever it finds none. The wl_surface itself stands: it is
        // what the sink is registered against, and what the next popup is made from. Taking the
        // buffer off goes with the role and lives in there, so that EVERY caller gets it.
        if (isPopupRole())
        {
            destroyPopupRole();
            return;
        }

        // A FRAME CALLBACK DOES NOT SURVIVE THE UNMAP. The compositor asks for frames of what is
        // on the screen, so one left outstanding here is never answered - and the idle holds off
        // every paint while a frame is owed. The window would come back to nothing being drawn in
        // it for the rest of the run.
        if (m_frameCallback)
        {
            ::wl_callback_destroy(m_frameCallback);
            m_frameCallback = nullptr;
        }

        ::wl_surface_attach(m_surface, nullptr, 0, 0);
        ::wl_surface_commit(m_surface);

        // AN UNMAPPED SURFACE IS UNCONFIGURED AGAIN. The protocol puts the role back where it
        // stood before the first buffer: another bufferless commit, another configure and another
        // acknowledgement are owed before anything may be attached to it - see show. A buffer
        // attached without them maps nothing, which is a window that never comes back.
        m_unmapped = true;
    }

    void Window::close()
    {
        if (!m_registered)
            return;

        m_registered = false;
        m_visible = false;

        // DESTROYED INNERMOST FIRST. Each of these was made from the one below it, and a
        // compositor is entitled to treat a parent destroyed before its child as a protocol
        // error rather than as tidying up.
        if (m_fractionalScale)
            ::wp_fractional_scale_v1_destroy(m_fractionalScale);
        if (m_viewport)
            ::wp_viewport_destroy(m_viewport);
        if (m_frameCallback)
            ::wl_callback_destroy(m_frameCallback);
        if (m_decoration)
            ::zxdg_toplevel_decoration_v1_destroy(m_decoration);
        if (m_toplevelSession)
            ::xdg_toplevel_session_v1_destroy(m_toplevelSession);
        if (m_toplevel)
            ::xdg_toplevel_destroy(m_toplevel);
        if (m_popup)
            ::xdg_popup_destroy(m_popup);
        if (m_xdgSurface)
            ::xdg_surface_destroy(m_xdgSurface);
        if (m_layerSurface)
            ::zwlr_layer_surface_v1_destroy(m_layerSurface);
        if (m_surface)
            ::wl_surface_destroy(m_surface);

        m_fractionalScale = nullptr;
        m_viewport = nullptr;
        m_frameCallback = nullptr;
        m_decoration = nullptr;
        m_toplevelSession = nullptr;
        m_toplevel = nullptr;
        m_popup = nullptr;
        m_xdgSurface = nullptr;
        m_layerSurface = nullptr;
        m_surface = nullptr;

        m_display.unregisterSink(*this);
    }

    void Window::minimize()
    {
        if (m_toplevel)
            ::xdg_toplevel_set_minimized(m_toplevel);
    }

    void Window::maximize()
    {
        if (traceEnabled())
            std::fprintf(stderr, "[wayland] ask     %p set_maximized\n", static_cast<void*>(this));
        if (m_toplevel)
            ::xdg_toplevel_set_maximized(m_toplevel);
    }

    void Window::restore()
    {
        if (traceEnabled())
            std::fprintf(stderr, "[wayland] ask     %p unset_maximized\n", static_cast<void*>(this));
        if (m_toplevel)
            ::xdg_toplevel_unset_maximized(m_toplevel);
    }

    void Window::setFocus(InputStamp stamp)
    {
        if (!m_toplevel)
            return;
        if (stamp.empty())
        {
            if (traceEnabled())
                std::fprintf(stderr, "[wayland] activate refused: no input stamp\n");
            return;
        }

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] ask     %p activate serial=%u\n",
                static_cast<void*>(this), stamp.serial);
        m_display.activate(m_surface, stamp);
    }

    // THE DAMAGE IS ACCUMULATED, NOT SENT. A commit is what asks for a frame, and committing on
    // every invalidate would ask twenty times for one. It goes once, from the idle callback,
    // when there is nothing else waiting.
    void Window::invalidateRect(const IntRect* value)
    {
        const IntRect whole = IntRect::fromDimensions({ 0, 0 }, m_size);
        const IntRect rect = value ? *value : whole;

        // TOLD TO THE FRAMES AS WELL AS TO THE WINDOW, and the two mean different things. This
        // rect says a paint is owed; what each frame carries says what that frame has missed. A
        // paint answers the first and clears it, and answers only the frame it happens to take.
        m_buffers.addDamage(rect);

        if (m_dirtyRect.empty())
        {
            m_dirtyRect = rect;
            return;
        }
        m_dirtyRect.unionWith(rect);
    }

    void Window::setTitle(const std::wstring_view value)
    {
        m_title = value;
        if (!m_toplevel)
            return;

        // A TITLE CROSSES THE WIRE AS UTF-8, whatever the platform holds it in.
        ::xdg_toplevel_set_title(m_toplevel, toUtf8(m_title).c_str());
    }

    // A REQUEST TO PAINT NOW, WHICH IS NOT A THING ON THIS PLATFORM. Win32 answers it inside the
    // call - UpdateWindow - and a caller can rely on the pixels being up when it returns. A
    // Wayland client paints when the compositor asks it to and at no other time: painting here
    // would produce a frame nobody asked for and take the buffer the frame that WAS asked for
    // needs, and it would leave a second callback outstanding behind the first.
    //
    // So this marks the window and lets the frame callback drive. Where nothing is owed the idle
    // that follows takes it up at once, which is as near to now as there is.
    void Window::update()
    {
        invalidateRect(nullptr);
    }

    // THE RANGE THE COMPOSITOR HOLDS A USER'S DRAG INSIDE. Stated to the system rather than
    // enforced here: the window is sized by whoever drags its edge, and the only way to be held
    // up is to have said where the floor is. Zero on an axis says there is no limit, which is
    // what k_maxFloat means on the way in.
    void Window::setSizeRange(ScaledDimensions minSize, ScaledDimensions maxSize)
    {
        // KEPT FOR THE TOPLEVEL STILL TO BE MADE - see createToplevel, which states it then. A
        // layer surface has no range: the compositor sizes it between its anchors.
        m_pendingMinSize = minSize;
        m_pendingMaxSize = maxSize;
        if (!m_toplevel)
            return;

        // THE CONTENT MEASURES IN REAL PIXELS and a toplevel's range is stated in surface ones -
        // the space a configure names, and the space place() converts into for the same reason.
        // Handed the real numbers, a window on a 200% screen is held at twice the floor its
        // content asked for, which is a size it cannot be dragged back down from. The Win32 twin
        // answers WM_GETMINMAXINFO in physical pixels and needs no conversion, which is why the
        // range crosses this interface in real ones.
        const IntSize surfaceMin = surfaceSizeFor(minSize);
        ::xdg_toplevel_set_min_size(m_toplevel, surfaceMin.x, surfaceMin.y);
        // Converted as the floor is, so a floor and a ceiling that agree stay one surface length.
        const FrameMargins margins = surfaceMargins();
        ::xdg_toplevel_set_max_size(m_toplevel,
            maxSize.x >= k_maxFloat ? 0
                : toSurfaceHolding(maxSize.x, margins.left, margins.right, m_scale120),
            maxSize.y >= k_maxFloat ? 0
                : toSurfaceHolding(maxSize.y, margins.top, margins.bottom, m_scale120));
    }

    // THE ALPHA CHANNEL IS DECIDED BEFORE THE FIRST FRAME IS CUT and never after; a design arriving
    // once frames exist changes the margins and the buffer, and the form is told through reframe.
    void Window::setFrame(const WindowFrame& value)
    {
        if (m_frameDesign == value)
            return;
        m_frameDesign = value;
        if (m_buffers.size().x == 0)
        {
            m_buffers.useAlphaChannel(isTranslucent());
            return;
        }
        reframe();
    }

    WindowFrame Window::appliedFrame() const
    {
        const FrameMargins margins = surfaceMargins();
        WindowFrame result{};
        result.margins = {
            toBufferLength(margins.left, m_scale120),
            toBufferLength(margins.top, m_scale120),
            toBufferLength(margins.right, m_scale120),
            toBufferLength(margins.bottom, m_scale120)
        };
        result.radius = m_buffers.hasAlphaChannel() ? m_frameDesign.radius : 0.0f;
        if (m_maximized || m_fullscreen || m_tiled || isLayerRole())
            result.radius = 0.0f;
        return result;
    }

    // ASKS FOR A SIZE AND IS TOLD ONE. A toplevel is not positioned by its client and cannot be:
    // the compositor weighs the room on each side, knows about panels this process cannot see,
    // and answers with a size in a configure. Until it has answered, what the content measured
    // is the best answer there is.
    //
    // A WINDOW AGAINST THE SCREEN'S EDGE IS A LAYER SURFACE, where the compositor has the layer
    // shell: it states the edges and its width, and the compositor answers the height and puts
    // it there. Where the compositor has not, the window is a toplevel that gets the height and
    // not the edge: it asks for its width and the tallest output's rows, read as surface pixels,
    // which at any real scale is more than the output holds - KWin cuts a new window to the work
    // area and says so in the configure that follows the first commit - and where it stands is
    // the compositor's, xdg-shell having no request for an edge.
    //
    // THE ROLE IS MADE HERE, at the first placement, because this is the first moment the window
    // knows which one it wants.
    PlacedWindow Window::place(const WindowPlacement& placement)
    {
        if (isPopupRole())
            return placePopup(placement);

        // A layer surface stands flush to its edges, with no margins to stand between.
        if (!hasRole())
        {
            if (placement.placement == FormPlacement::ScreenRight)
                createLayerSurface(toSurfaceHolding(placement.size.x, 0, 0, m_scale120));
            if (!m_layerSurface)
                createToplevel();
        }

        // THE CONTENT MEASURES IN REAL PIXELS and the compositor is told surface ones, so what
        // the form asked for is divided by the scale on the way out and multiplied by it again
        // on the way back in. What it asked for is the geometry; the surface adds the margins,
        // which the role just made decides.
        const IntSize askedSurface = surfaceSizeFor(placement.size);

        if (!m_configured)
        {
            m_geometrySize = askedSurface;
            // THE SIZE THE CONFIG KEPT, held up to the floor the content states now. It is what a
            // configure naming no size is answered with, and the size an un-maximize comes back
            // to; a compositor that remembered the window itself names its own in the configure.
            if (m_remembered && placement.placement == FormPlacement::Default)
            {
                const IntSize surfaceMin = surfaceSizeFor(placement.minSize);
                m_geometrySize = {
                    std::max(m_remembered->size.x, surfaceMin.x),
                    std::max(m_remembered->size.y, surfaceMin.y)
                };
                m_restoreSize = m_geometrySize;
            }
            if (placement.placement == FormPlacement::ScreenRight && !isLayerRole())
                m_geometrySize.y = std::max(m_geometrySize.y,
                    m_display.tallestOutputHeight());
            updateSurfaceSize();
        }
        else if (isLayerRole() && askedSurface.x != m_layerWidth)
        {
            // A WIDTH ASKED FOR AGAIN, at another scale or by other content. The height stays
            // the compositor's. Committed at once so the configure that answers it is not left
            // waiting for a frame nothing has asked for.
            m_layerWidth = std::max(1, askedSurface.x);
            ::zwlr_layer_surface_v1_set_size(m_layerSurface, m_layerWidth, 0);
            ::zwlr_layer_surface_v1_set_exclusive_zone(m_layerSurface, m_layerWidth);
            ::wl_surface_commit(m_surface);
        }

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] place   %p %-7s ask=%dx%d -> %dx%d  configured=%d\n",
                static_cast<void*>(this), roleName(m_role),
                placement.size.toInt().x, placement.size.toInt().y, m_size.x, m_size.y,
                m_configured ? 1 : 0);

        return { geometryPixels().toFloat() };
    }

    // THE OPACITY IS CARRIED IN THE PIXELS. There is no per-surface opacity protocol, so a frame
    // is written at the window's alpha instead - see FormWindow::paint - and a new value is a
    // repaint and nothing more. ZERO IS STILL SPECIAL: a frame nobody can see is not worth
    // drawing, and an unmapped window is the honest way to say a hint is not there.
    void Window::setAlpha(ColorByte value)
    {
        if (m_alpha == value)
            return;

        const bool wasUp = m_alpha != 0;
        const bool wasOpaque = m_alpha == 255;
        m_alpha = value;

        // WHAT IS SOLID CHANGES WITH THE OPACITY: a window at less than full strength has no
        // opaque region, whatever its pixels would be at full.
        if (wasOpaque != (m_alpha == 255))
            stateGeometry();

        if (wasUp != (m_alpha != 0))
        {
            if (traceEnabled())
                std::fprintf(stderr, "[wayland] alpha   %p %-7s crossed to %u - %s\n",
                    static_cast<void*>(this), roleName(m_role), m_alpha,
                    m_alpha ? "mapping" : "unmapping");

            if (m_alpha)
                show();
            else
                hide();
            return;
        }

        invalidateRect(nullptr);
    }

    // ADDED, NOT RESTORED, AND MAPPED THIS WHOLE TIME. A session opened while this window stood
    // cannot restore it - restore_toplevel is refused after the first commit - and has nothing to
    // restore into it either, the window being where the user put it. What is wanted is that the
    // compositor start keeping this window's state, which is what add_toplevel says and which it
    // may be told at any time. See Platform
    void Window::addToSession(const std::string_view name)
    {
        if (m_toplevelSession || !m_toplevel || name.empty())
            return;

        xdg_session_v1* session = m_display.session();
        if (!session)
            return;

        m_sessionName = name;
        m_toplevelSession = ::xdg_session_v1_add_toplevel(session, m_toplevel,
            m_sessionName.c_str());

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] session added toplevel \"%s\"\n",
                m_sessionName.c_str());
    }

    RememberedPlacement Window::rememberedPlacement() const
    {
        return { m_restoreSize, m_maximized };
    }

    // AN ORDINARY WINDOW, placed by the compositor's own rule.
    void Window::createToplevel()
    {
        DisplayManager& manager = m_display;
        if (!m_surface || !manager.windowManager())
            return;

        static const xdg_surface_listener k_surfaceListener = {
            .configure = &Window::onSurfaceConfigure
        };
        static const xdg_toplevel_listener k_toplevelListener = {
            .configure = &Window::onToplevelConfigure,
            .close = &Window::onToplevelClose
        };

        m_xdgSurface = ::xdg_wm_base_get_xdg_surface(manager.windowManager(), m_surface);
        ::xdg_surface_add_listener(m_xdgSurface, &k_surfaceListener, this);
        m_toplevel = ::xdg_surface_get_toplevel(m_xdgSurface);
        ::xdg_toplevel_add_listener(m_toplevel, &k_toplevelListener, this);
        ::xdg_toplevel_set_app_id(m_toplevel, manager.appId());

        // THE FRAME IS THE FRAMEWORK'S OWN, and a compositor that would otherwise add one has to
        // be told. Where the global is absent - Mutter does not advertise it - nobody was going
        // to add a frame anyway, so its absence needs no fallback.
        if (manager.decorationManager())
        {
            m_decoration = ::zxdg_decoration_manager_v1_get_toplevel_decoration(
                manager.decorationManager(), m_toplevel);
            ::zxdg_toplevel_decoration_v1_set_mode(m_decoration,
                ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
        }

        // WHAT WAS SAID BEFORE THERE WAS A TOPLEVEL TO SAY IT TO: the title a form sets on
        // creation, the range its first measuring pass stated.
        if (!m_title.empty())
            ::xdg_toplevel_set_title(m_toplevel, toUtf8(m_title).c_str());
        setSizeRange(m_pendingMinSize, m_pendingMaxSize);

        // NAMED TO THE SESSION BEFORE THE FIRST COMMIT, which is the protocol's rule: a toplevel
        // once committed cannot be restored. A compositor holding a record of the name answers
        // the first configure with the size, the state and, unseen by the client, the place it
        // remembered. What the config kept is stated as well - the size as the first placement's
        // ask, the maximized state as a request here - and is the whole of the memory where no
        // compositor keeps one.
        if (!m_sessionName.empty())
        {
            if (xdg_session_v1* session = manager.session())
            {
                static const xdg_toplevel_session_v1_listener k_toplevelSessionListener = {
                    .restored = &Window::onToplevelSessionRestored
                };
                m_toplevelSession = ::xdg_session_v1_restore_toplevel(session, m_toplevel,
                    m_sessionName.c_str());
                ::xdg_toplevel_session_v1_add_listener(m_toplevelSession,
                    &k_toplevelSessionListener, this);
            }
            if (m_remembered && m_remembered->maximized)
                ::xdg_toplevel_set_maximized(m_toplevel);
        }

        // COMMITTED WITH NO BUFFER, AND THAT IS THE POINT. This asks the compositor what size it
        // would like the window to be; attaching anything before its answer arrives is a protocol
        // error and takes the connection down.
        ::wl_surface_commit(m_surface);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] create  %p %-7s toplevel\n",
                static_cast<void*>(this), roleName(m_role));
    }

    // A STRIP DOWN THE RIGHT EDGE OF THE SCREEN. Anchored to the top, the bottom and the right,
    // its width is its own and its height is the compositor's - a size of zero on the axis
    // between two anchors says so - and the configure names both. THE STRIP IS RESERVED: the
    // exclusive zone is its width, so other windows are placed and maximized beside it rather
    // than under it, which is what a panel says and what a log meant to be read while the
    // application runs wants. The top layer keeps it over ordinary windows; no keyboard, because
    // nothing in it is typed into. Which output is left to the compositor.
    void Window::createLayerSurface(int surfaceWidth)
    {
        DisplayManager& manager = m_display;
        if (!m_surface || !manager.layerShell())
            return;

        static const zwlr_layer_surface_v1_listener k_layerListener = {
            .configure = &Window::onLayerConfigure,
            .closed = &Window::onLayerClosed
        };

        m_layerSurface = ::zwlr_layer_shell_v1_get_layer_surface(manager.layerShell(),
            m_surface, nullptr, ZWLR_LAYER_SHELL_V1_LAYER_TOP, manager.appId());
        ::zwlr_layer_surface_v1_add_listener(m_layerSurface, &k_layerListener, this);
        ::zwlr_layer_surface_v1_set_anchor(m_layerSurface,
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
            | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
            | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
        ::zwlr_layer_surface_v1_set_keyboard_interactivity(m_layerSurface,
            ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
        m_layerWidth = std::max(1, surfaceWidth);
        ::zwlr_layer_surface_v1_set_size(m_layerSurface, m_layerWidth, 0);
        ::zwlr_layer_surface_v1_set_exclusive_zone(m_layerSurface, m_layerWidth);

        // Bufferless, as for a toplevel: the commit asks for the configure that names the size.
        ::wl_surface_commit(m_surface);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] create  %p %-7s layer width=%d\n",
                static_cast<void*>(this), roleName(m_role), m_layerWidth);
    }

    // The role and everything owed on it. THE FRAME CALLBACK GOES WITH IT: it belongs to the
    // surface rather than to the role, so one left outstanding fires on a surface that no longer
    // has a role, and the frame it asks for would attach a buffer to it - which is a protocol
    // error and takes the connection down.
    void Window::destroyPopupRole()
    {
        if (!m_surface)
            return;

        // THE BUFFER COMES OFF BEFORE THE ROLE DOES, AND IT IS DONE HERE so that every caller
        // gets it - hiding a hint, the compositor dismissing one, and a placement that moved far
        // enough to want a new positioner all arrive at this one function. A fresh xdg_surface
        // may not be made from a surface that still has a buffer attached, because the initial
        // commit of the next popup has to be bufferless: get_xdg_surface answers that with
        // unconfigured_buffer, and a protocol error takes down the connection and every window
        // on it.
        if (m_popup || m_xdgSurface)
        {
            ::wl_surface_attach(m_surface, nullptr, 0, 0);
            ::wl_surface_commit(m_surface);
        }

        if (m_frameCallback)
            ::wl_callback_destroy(m_frameCallback);
        if (m_popup)
            ::xdg_popup_destroy(m_popup);
        if (m_xdgSurface)
            ::xdg_surface_destroy(m_xdgSurface);

        m_frameCallback = nullptr;
        m_popup = nullptr;
        m_xdgSurface = nullptr;
        m_dirtyRect = {};
        // The next one is placed and configured from nothing, exactly as the first was.
        m_configured = false;
    }

    FrameMargins Window::surfaceMargins() const
    {
        // A LAYER SURFACE IS PLACED FLUSH TO THE EDGES IT NAMED, and margins would stand it off
        // them. A tiled window keeps its shadow: the compositor tiles the geometry inside it.
        if (m_maximized || m_fullscreen || isLayerRole())
            return {};

        auto toSurface = [this](int length) {
            return (length * k_scaleUnit + m_scale120 - 1) / m_scale120;
        };
        const FrameMargins& design = m_frameDesign.margins;
        return {
            toSurface(design.left),
            toSurface(design.top),
            toSurface(design.right),
            toSurface(design.bottom)
        };
    }

    void Window::updateSurfaceSize()
    {
        const FrameMargins margins = surfaceMargins();
        m_surfaceSize = {
            m_geometrySize.x + margins.left + margins.right,
            m_geometrySize.y + margins.top + margins.bottom
        };
        m_size = {
            toBufferLength(m_surfaceSize.x, m_scale120),
            toBufferLength(m_surfaceSize.y, m_scale120)
        };
    }

    IntSize Window::geometryPixels() const
    {
        const FrameMargins margins = appliedFrame().margins;
        return {
            m_size.x - margins.left - margins.right,
            m_size.y - margins.top - margins.bottom
        };
    }

    IntSize Window::surfaceSizeFor(ScaledDimensions asked) const
    {
        const FrameMargins margins = surfaceMargins();
        return {
            toSurfaceHolding(asked.x, margins.left, margins.right, m_scale120),
            toSurfaceHolding(asked.y, margins.top, margins.bottom, m_scale120)
        };
    }

    // THE OPAQUE REGION IS A SAVING AND NOT A PROMISE: what it names the compositor need not
    // blend, and what it leaves out is blended as it always was. So it leaves out everything that
    // might not be solid - the corner squares, rather than tracing the arc, since a region is
    // rectangles; and one surface pixel along every side, because at a fractional scale a surface
    // pixel on the edge is made of a solid buffer pixel and a shadow one, and claimed solid it
    // would be shown unblended. A window at less than full opacity states none. THE INPUT REGION
    // IS THE GEOMETRY, reaching k_resizeGrab into the shadow on a window the user may size; a hint
    // keeps the empty region its constructor gave it and takes no pointer at all.
    void Window::stateGeometry()
    {
        if (!m_surface || !hasRole() || m_geometrySize.x <= 0 || m_geometrySize.y <= 0)
            return;

        const FrameMargins margins = surfaceMargins();
        const IntRect geometry = IntRect::fromDimensions({ margins.left, margins.top }, m_geometrySize);
        // A layer surface has no window geometry to state: it wears no margins, so the surface
        // is the window.
        if (m_xdgSurface)
            ::xdg_surface_set_window_geometry(m_xdgSurface,
                geometry.left, geometry.top, geometry.width(), geometry.height());

        wl_compositor* compositor = m_display.compositor();
        if (isTranslucent())
        {
            wl_region* opaque = nullptr;
            if (m_alpha == 255)
            {
                const int radiusPixels = static_cast<int>(std::ceil(appliedFrame().radius));
                const int radius = (radiusPixels * k_scaleUnit + m_scale120 - 1) / m_scale120;
                const IntRect solid = geometry.inflated(-1);
                opaque = ::wl_compositor_create_region(compositor);
                if (solid.width() > 2 * radius && solid.height() > 2 * radius)
                {
                    ::wl_region_add(opaque, solid.left + radius, solid.top,
                        solid.width() - 2 * radius, solid.height());
                    if (radius > 0)
                        ::wl_region_add(opaque, solid.left, solid.top + radius,
                            solid.width(), solid.height() - 2 * radius);
                }
            }
            ::wl_surface_set_opaque_region(m_surface, opaque);
            if (opaque)
                ::wl_region_destroy(opaque);
        }

        if (m_role != WindowRole::Tooltip)
        {
            IntRect input = geometry;
            // A layer surface is not sized by the user, so there is no grab to reach for.
            if (m_role == WindowRole::Dialog && !isLayerRole())
            {
                input.inflate(k_resizeGrab);
                input.intersectWith({ 0, 0, m_surfaceSize.x, m_surfaceSize.y });
            }
            wl_region* inputRegion = ::wl_compositor_create_region(compositor);
            ::wl_region_add(inputRegion, input.left, input.top, input.width(), input.height());
            ::wl_surface_set_input_region(m_surface, inputRegion);
            ::wl_region_destroy(inputRegion);
        }
    }

    void Window::applySize()
    {
        applyScaleToSurface();
        stateGeometry();
        m_buffers.resize(m_size);
        resized(m_size, appliedFrame());
        invalidateRect(nullptr);
    }

    void Window::reframe()
    {
        updateSurfaceSize();
        applySize();
    }

    // FLIP, THEN SLIDE, THEN SHRINK, and the flip only on the axis a menu grows along. These are
    // the same three answers placeWithin works through by hand in the Win32 twin: a menu that does
    // not fit below is put above, one that fits neither way takes the room the larger side has,
    // and the width slides along the edge rather than flipping across it.
    constexpr std::uint32_t k_menuConstraints =
        XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y
        | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X
        | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_Y;

    // THE BARE REQUEST. A form's window overrides this to release the form's press first, since
    // the compositor will swallow the real release; a window with no form has no press to release.
    void Window::initiateWindowDrag(IntPoint, InputStamp stamp)
    {
        requestWindowMove(stamp);
    }

    // HANDS THE WINDOW TO THE COMPOSITOR TO BE DRAGGED. No point is taken: this platform never
    // learns where its window is, so a drag is a gesture handed over rather than a position
    // tracked. What it does read is the stamp - the compositor weighs this against the press that
    // asked for it and refuses a request that names no event.
    // THE COMPOSITOR'S MENU, at a point in surface coordinates. Refused unstamped on the same
    // terms as a move: the request names the press it answers. The menu takes the pointer as a
    // popup of its own, with the leave and the enter that go with one, so nothing here has to be
    // restated afterwards.
    void Window::showWindowMenu(PointInForm pt, InputStamp stamp)
    {
        wl_seat* seat = m_display.seat();
        if (!m_toplevel || !seat)
            return;
        if (stamp.empty())
        {
            if (traceEnabled())
                std::fprintf(stderr, "[wayland] menu refused: no input stamp\n");
            return;
        }

        const IntPoint bufferPoint = pt.toInt();
        const IntPoint surfacePoint = {
            toSurfaceLength(bufferPoint.x, m_scale120),
            toSurfaceLength(bufferPoint.y, m_scale120)
        };
        if (traceEnabled())
            std::fprintf(stderr, "[wayland] ask     %p show_window_menu at %d,%d serial=%u\n",
                static_cast<void*>(this), surfacePoint.x, surfacePoint.y, stamp.serial);

        ::xdg_toplevel_show_window_menu(m_toplevel, seat, stamp.serial, surfacePoint.x,
            surfacePoint.y);
    }

    void Window::onPointerMoved(IntPoint surfacePoint)
    {
        pointerMoved(toBufferPoint(surfacePoint));
    }

    void Window::onPointerButton(IntPoint surfacePoint, std::uint32_t time, InputStamp stamp, bool down)
    {
        pointerButton(toBufferPoint(surfacePoint), time, stamp, down);
    }

    void Window::onPointerContextMenu(IntPoint surfacePoint, InputStamp stamp)
    {
        pointerContextMenu(toBufferPoint(surfacePoint), stamp);
    }

    void Window::onPointerWheel(IntPoint surfacePoint, float delta, bool horizontal)
    {
        pointerWheel(toBufferPoint(surfacePoint), delta, horizontal);
    }

    void Window::onPointerLeft()
    {
        pointerLeft();
    }

    void Window::onKeyPressed(const KeyPress& press)
    {
        keyPressed(press);
    }

    void Window::onCharacter(wchar_t value)
    {
        character(value);
    }

    void Window::onKeyReleased()
    {
        keyReleased();
    }

    // A POPUP'S ROLE IS MADE HERE, NOT IN place(). A FORM IS PLACED MORE THAN ONCE BEFORE IT IS
    // SHOWN: initPlacement runs its passes, and a tooltip is placed again afterwards when the
    // control it is about hands over the rect it stands on - Tooltip::showOrHide calls show()
    // first and updatePosition second, so THE FIRST PLACEMENT CARRIES AN ANCHOR THE FORM HAS NOT
    // FILLED IN YET, an empty rect at the origin. A positioner is read once, when the popup is
    // made from it, and a compositor without xdg_popup.reposition will never hear a correction -
    // so the popup is built from the LAST placement rather than the first. The idle is when the
    // placing is over and a frame is wanted, which is exactly that moment.
    void Window::onIdle()
    {
        // THE ALPHA IS PART OF THE QUESTION, not decoration on it. TooltipForm::updateVisibility
        // calls window().show() UNCONDITIONALLY - on the way out as well as the way in - so
        // m_visible goes true again after the fade has already reached zero and taken the role
        // down. Without the alpha in this condition the next idle builds the popup straight back
        // and the hint never leaves the screen.
        if (isPopupRole() && !m_popup && m_visible && m_alpha)
            createPopup();

        // NOTHING IS PAINTED BEFORE THE FIRST CONFIGURE, OR BEFORE THE ONE A REMAP ASKED FOR. A
        // buffer attached before either is a protocol error, and a frame drawn for a window
        // nobody has asked to see has no reader.
        if (!m_configured || m_unmapped || !m_visible || m_dirtyRect.empty())
            return;

        // A FRAME IS ALREADY OWED. Painting now would produce one the compositor has not asked
        // for and will not show, and would take the free buffer that the frame it DID ask for
        // needs. The damage stands until the callback comes.
        if (m_frameCallback)
            return;

        paint();
    }

    void Window::presentFrame(ShmBuffers::Frame& frame, const IntRect& damage)
    {
        if (traceEnabled())
            std::fprintf(stderr, "[wayland] present %p %-7s damage=%d,%d %dx%d\n",
                static_cast<void*>(this), roleName(m_role),
                damage.left, damage.top, damage.width(), damage.height());

        static const wl_callback_listener k_frameListener = {
            .done = &Window::onFrameDone
        };
        m_frameCallback = ::wl_surface_frame(m_surface);
        ::wl_callback_add_listener(m_frameCallback, &k_frameListener, this);

        ::wl_surface_attach(m_surface, frame.handle, 0, 0);
        ::wl_surface_damage_buffer(m_surface, damage.left, damage.top,
            damage.width(), damage.height());
        ::wl_surface_commit(m_surface);
        frame.busy = true;
    }

    void Window::requestWindowMove(InputStamp stamp)
    {
        wl_seat* seat = m_display.seat();
        if (!m_toplevel || !seat)
            return;

        // REFUSED RATHER THAN GUESSED AT. Reaching for the last serial the compositor sent would
        // let a drag start that no press asked for, which is the hole the stamp exists to close.
        // An unstamped request here is a wiring fault, so it is worth saying so.
        if (stamp.empty())
        {
            if (traceEnabled())
                std::fprintf(stderr, "[wayland] move refused: no input stamp\n");
            return;
        }

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] move    %p serial=%u\n",
                static_cast<void*>(this), stamp.serial);

        ::xdg_toplevel_move(m_toplevel, seat, stamp.serial);
        m_display.pointerHandedToCompositor();
    }

    void Window::requestWindowResize(std::uint32_t edges, InputStamp stamp)
    {
        wl_seat* seat = m_display.seat();
        if (!m_toplevel || !seat || !edges)
            return;

        if (stamp.empty())
        {
            if (traceEnabled())
                std::fprintf(stderr, "[wayland] resize refused: no input stamp\n");
            return;
        }

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] resize  %p edges=%u serial=%u\n",
                static_cast<void*>(this), edges, stamp.serial);

        ::xdg_toplevel_resize(m_toplevel, seat, stamp.serial, edges);
        m_display.pointerHandedToCompositor();
    }

    // Which object the acknowledgement goes to is the role's.
    void Window::ackConfigure(std::uint32_t serial)
    {
        if (m_layerSurface)
            ::zwlr_layer_surface_v1_ack_configure(m_layerSurface, serial);
        else
            ::xdg_surface_ack_configure(m_xdgSurface, serial);
    }

    // A POPUP IS PLACED BY THE COMPOSITOR AND THE CLIENT IS NEVER TOLD WHERE. What this states is
    // an anchor, a gravity and what may be done when the window will not fit; the answer comes
    // back as a size and nothing else - see PlacedWindow, which holds no position for exactly
    // this reason.
    PlacedWindow Window::placePopup(const WindowPlacement& placement)
    {
        // THE PARENT'S SCALE, READ AT EVERY PLACEMENT. A popup is drawn at what its parent is
        // drawn at and never at what its output says - the framework states the same rule, a popup
        // form sharing its parent's Scaler outright. Read here rather than at construction because
        // a hint is built with its form, long before that form's window has been configured: taken
        // then it is 120 for ever, the surface size is turned back into a buffer half as big as it
        // should be, and the form lays out at 200% into a canvas meant for 100%.
        const bool scaleMoved = m_scale120 != parentScale120();
        if (scaleMoved)
        {
            m_scale120 = parentScale120();
            m_pendingScale120 = m_scale120;
        }

        // A HINT IS ONE WINDOW FOR THE WHOLE FORM, shown about a different control every time and
        // asked for a new size and a new anchor each time - see tooltip_is_a_form_member. So a
        // popup already on screen being asked for something else is the ORDINARY case here, not an
        // edge of one, and the question every time is whether what is being asked for has moved.
        const IntSize askedSurface = surfaceSizeFor(placement.size);
        const bool changed = scaleMoved
            || askedSurface != surfaceSizeFor(m_placement.size)
            || placement.anchorRect != m_placement.anchorRect
            || placement.placement != m_placement.placement;

        m_placement = placement;

        // THE ASKED SIZE STANDS UNTIL A CONFIGURE ANSWERS. That answer is asynchronous and this
        // call is not, which is the same bargain the toplevel path makes. Recomputed whenever the
        // ask moves and NOT only before the first configure: held to that, the first hint of a
        // session fixes the window's size for the rest of it and every later ask is answered with
        // the size of a button's hint.
        const IntSize sizeBefore = m_size;
        if (changed || !m_configured)
        {
            m_geometrySize = askedSurface;
            // ANSWERED FROM THE ROUND TRIP AND NOT FROM WHAT WAS ASKED. A surface size is what a
            // configure will state back, and at a scale of 2 an odd number of real pixels does
            // not survive the journey out and home. Told what it asked for, the form lays out at
            // a size it is about to lose and again at the one the configure names; told this, it
            // lays out once.
            updateSurfaceSize();
        }

        // TOLD AFTER THE SIZE AND NOT WITH THE SCALE IT BELONGS TO. What a viewport is told is the
        // surface size, which the block above is what computes - and a scale that moved is one of
        // the things that sends it through, so the size read here is the one this placement asked
        // for.
        if (scaleMoved)
            applyScaleToSurface();

        // A POSITIONER IS READ ONCE, when the popup is made from it. Moving one that is already up
        // therefore means either handing the compositor a new positioner to read - reposition,
        // which arrived in xdg_wm_base 3 - or taking the popup down and letting the idle build
        // another from the placement just recorded. The second unmaps and remaps, which shows; it
        // is what there is on a compositor that binds less.
        if (m_popup && changed)
        {
            if (canReposition())
            {
                xdg_positioner* positioner = buildPositioner(m_placement);
                ::xdg_popup_reposition(m_popup, positioner, ++m_repositionToken);
                ::xdg_positioner_destroy(positioner);

                if (traceEnabled())
                    std::fprintf(stderr, "[wayland] repos   %p %-7s asked token=%u\n",
                        static_cast<void*>(this), roleName(m_role), m_repositionToken);
            }
            else
            {
                destroyPopupRole();
            }
        }

        // A POPUP THAT IS UP TAKES THE SIZE IT ASKED FOR AT ONCE, as a Win32 window does when its
        // bounds are set; the configure that answers the reposition then finds m_size already
        // true and has nothing to do. Left to that configure, the answer would be compared with
        // an m_size this call has already moved, and the geometry, the input region, the frames
        // and the form's canvas would keep the size before the ask - a scrollbar the second ask
        // added drawn past the input region, its presses reaching the window underneath. Before
        // the first configure there is nothing to apply to; that configure applies everything.
        if (m_configured && m_size != sizeBefore)
            applySize();

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] place   %p %-7s ask=%dx%d -> %dx%d  configured=%d\n",
                static_cast<void*>(this), roleName(m_role),
                placement.size.toInt().x, placement.size.toInt().y, m_size.x, m_size.y,
                m_configured ? 1 : 0);

        return { geometryPixels().toFloat() };
    }

    void Window::createPopup()
    {
        DisplayManager& manager = m_display;
        // A POPUP STANDS ON A PARENT'S XDG SURFACE, or on a parent's layer surface, which is the
        // one other thing a popup may stand on. With neither there is nothing to anchor against,
        // and the surface keeps no role - which shows nothing rather than the wrong thing.
        if (!m_surface || !manager.windowManager() || !m_parent)
            return;
        if (!m_parent->m_xdgSurface && !m_parent->m_layerSurface)
            return;

        static const xdg_surface_listener k_surfaceListener = {
            .configure = &Window::onSurfaceConfigure
        };
        static const xdg_popup_listener k_popupListener = {
            .configure = &Window::onPopupConfigure,
            .popup_done = &Window::onPopupDone,
            .repositioned = &Window::onPopupRepositioned
        };

        xdg_positioner* positioner = buildPositioner(m_placement);
        m_xdgSurface = ::xdg_wm_base_get_xdg_surface(manager.windowManager(), m_surface);
        ::xdg_surface_add_listener(m_xdgSurface, &k_surfaceListener, this);
        // A LAYER SURFACE IS NOT AN XDG SURFACE, so the popup is made with no parent and the
        // layer surface then claims it - before the initial commit, which is the protocol's one
        // condition on the order.
        m_popup = ::xdg_surface_get_popup(m_xdgSurface, m_parent->m_xdgSurface, positioner);
        ::xdg_popup_add_listener(m_popup, &k_popupListener, this);
        if (!m_parent->m_xdgSurface)
            ::zwlr_layer_surface_v1_get_popup(m_parent->m_layerSurface, m_popup);
        // ITS WHOLE PURPOSE IS SPENT. A positioner is read when the popup is made from it and
        // holds nothing afterwards; reposition builds another.
        ::xdg_positioner_destroy(positioner);
        stateGeometry();

        // NO xdg_popup_grab, AND THAT IS A DECISION. A grab takes the pointer away from this
        // client altogether: a press outside the popup goes to the compositor, which dismisses
        // the popup and tells this window - while the framework, which raised the menu and is the
        // thing holding it open, never sees the press it closes menus on. Ungrabbed, that press
        // reaches the parent surface and the framework closes the menu exactly as it does on
        // Win32, which is one code path rather than two. What it costs is dismissal by a click in
        // ANOTHER application. Taking the grab wants a way to tell a form its popup is gone.
        ::wl_surface_commit(m_surface);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] create  %p %-7s popup on %p\n",
                static_cast<void*>(this), roleName(m_role), static_cast<void*>(m_parent));
    }

    // WHAT THE COMPOSITOR IS TOLD, AND ALL IT IS TOLD: where this window stands, which way it
    // grows, and what may be done to it when it will not fit. It decides the rest, because it is
    // the only one that knows where the panels and the screen edges are.
    //
    // screenMargin is unread. The compositor holds a popup inside the room it knows about and
    // there is no request that widens or narrows that boundary; the margin is for the platforms
    // that do this arithmetic themselves.
    xdg_positioner* Window::buildPositioner(const WindowPlacement& placement) const
    {
        const int scale = parentScale120();
        auto toSurface = [scale](float value){
            return toSurfaceLength(static_cast<int>(std::lround(value)), scale);
        };

        xdg_positioner* positioner =
            ::xdg_wm_base_create_positioner(m_display.windowManager());

        // NEITHER MAY BE ZERO. A positioner with no size is a protocol error and takes the
        // connection down with it, and a window measured before its content has any is how one
        // would arrive here.
        const IntSize size = surfaceSizeFor(placement.size);
        ::xdg_positioner_set_size(positioner, std::max(1, size.x), std::max(1, size.y));

        // THE ANCHOR IS IN THE PARENT FORM'S COORDINATES - its surface, margins and all - and a
        // positioner states it against the parent's window geometry, so the parent's margins come
        // off before the change of unit.
        //
        // NOT EMPTY, EVER. A window raised at the pointer names an empty rect there, which is a
        // point and reads as one everywhere above this line - but a positioner whose anchor rect
        // has no width is INCOMPLETE, and get_popup answers that with a protocol error that takes
        // the connection down. A point is therefore sent as the smallest rectangle there is. What
        // it costs is one surface pixel: the popup hangs off the bottom left of a 1x1 rect rather
        // than off the point, and a pointer placement has already stood itself clear of the
        // cursor by more than that.
        FloatRect anchor = placement.anchorRect;
        if (m_parent)
        {
            const FrameMargins parentMargins = m_parent->appliedFrame().margins;
            anchor.offset(-static_cast<float>(parentMargins.left), -static_cast<float>(parentMargins.top));
        }
        ::xdg_positioner_set_anchor_rect(positioner,
            toSurface(anchor.left), toSurface(anchor.top),
            std::max(1, toSurface(anchor.width())), std::max(1, toSurface(anchor.height())));

        switch (placement.placement)
        {
        case FormPlacement::OverText:
            // The two texts land on top of each other: the window is anchored by the top left of
            // the text it covers and offset back by where its own text starts inside it. IT MUST
            // NOT FLIP - a flipped OverText stands where its text is not, which is the one thing
            // this placement exists to prevent.
            ::xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
            ::xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
            ::xdg_positioner_set_offset(positioner,
                -toSurface(placement.textOrigin.x), -toSurface(placement.textOrigin.y));
            ::xdg_positioner_set_constraint_adjustment(positioner,
                XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X
                | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y);
            break;

        case FormPlacement::Top:
            // Grows upwards from the anchor's top edge, standing clear of it by the clearance.
            ::xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
            ::xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_TOP_RIGHT);
            ::xdg_positioner_set_offset(positioner, 0, -toSurface(placement.clearance));
            ::xdg_positioner_set_constraint_adjustment(positioner, k_menuConstraints);
            break;

        default:
            // Bottom, ContextMenu and Mouse all grow downwards from the anchor's bottom edge. The
            // two pointer placements state an empty anchor and no clearance, so they hang off the
            // point itself - which is what this one rule already says.
            ::xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_BOTTOM_LEFT);
            ::xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
            ::xdg_positioner_set_offset(positioner, 0, toSurface(placement.clearance));
            ::xdg_positioner_set_constraint_adjustment(positioner, k_menuConstraints);
            break;
        }

        // WHAT WENT OUT, in both spaces. A popup that lands somewhere unexpected is either asking
        // for the wrong thing or being constrained, and only the pair of these against the
        // xdg_popup.configure that answers them says which.
        if (traceEnabled())
            std::fprintf(stderr,
                "[wayland] posn    %p place=%d scale=%d/120"
                "  anchor real=%.0f,%.0f %.0fx%.0f -> surf=%d,%d %dx%d"
                "  size real=%.0fx%.0f -> surf=%dx%d  clearance=%.0f\n",
                static_cast<const void*>(this),
                static_cast<int>(placement.placement), scale,
                anchor.left, anchor.top, anchor.width(), anchor.height(),
                toSurface(anchor.left), toSurface(anchor.top),
                std::max(1, toSurface(anchor.width())), std::max(1, toSurface(anchor.height())),
                placement.size.x, placement.size.y,
                std::max(1, size.x), std::max(1, size.y),
                placement.clearance);

        return positioner;
    }

    int Window::parentScale120() const
    {
        return m_parent ? m_parent->m_scale120 : m_scale120;
    }

    // xdg_popup.reposition arrived in xdg_wm_base 3. A compositor offering less places a popup
    // once, where it was created, and a placement asked for a second time keeps the first answer.
    bool Window::canReposition() const
    {
        xdg_wm_base* windowManager = m_display.windowManager();
        return windowManager && ::xdg_wm_base_get_version(windowManager) >= 3;
    }

    // THE PROPOSAL BECOMES THE TRUTH HERE AND NOWHERE ELSE. Everything the toplevel named was
    // pending until this arrived; acknowledging it is what makes it so, and the frame committed
    // afterwards is the answer to it.
    void Window::applyConfigure(std::uint32_t serial)
    {
        // THE FIRST CONFIGURE ALWAYS COUNTS AS A CHANGE, whatever it names. A compositor with no
        // requirement sends 0x0, which is read as the size the content measured - so the pending
        // size is one this window already held, and comparing the two finds nothing to do. There
        // is everything to do: no buffer exists yet, and until one is attached the window is not
        // on screen at all.
        const bool firstConfigure = !m_configured;
        const bool scaleWasChanged = m_pendingScale120 != m_scale120;
        const bool focusWasChanged = m_pendingActivated != m_activated;
        const WindowFrame frameBefore = appliedFrame();
        const IntSize sizeBefore = m_size;

        m_geometrySize = m_pendingSize;
        m_scale120 = m_pendingScale120;
        m_activated = m_pendingActivated;
        m_maximized = m_pendingMaximized;
        m_tiled = m_pendingTiled;
        m_fullscreen = m_pendingFullscreen;

        // REAL PIXELS, which is what the buffer holds and what the form lays out into. The
        // compositor names a geometry and is told a scale; the geometry, its margins and the
        // scale make the only size anything above this line has ever meant.
        updateSurfaceSize();
        const bool sizeChanged = m_size != sizeBefore;
        // THE FRAME IS PART OF THE STATE: a window maximized at the size it already had has
        // lost its shadow, its corners and its ring, and the form has to be told as for a resize.
        const WindowFrame frame = appliedFrame();
        const bool frameChanged = frame != frameBefore;

        // TAKEN WHILE THE WINDOW IS ORDINARY, which is the only time this size means anything:
        // taken while it is maximized it would record the screen, and restoring would restore
        // nothing. IN SURFACE COORDINATES, because that is the space a configure will ask this
        // window to choose in - remembered in real pixels it would come back multiplied by the
        // scale a second time.
        if (!m_maximized && !m_fullscreen)
            m_restoreSize = m_geometrySize;

        // ACKED BEFORE THE COMMIT THAT ANSWERS IT. The other order hands the compositor a frame
        // it has not asked for, followed by an acknowledgement of a proposal already superseded.
        ackConfigure(serial);
        m_configured = true;
        // AND THE MAP IS PAID FOR. Whatever asked for this configure, the surface may hold a
        // buffer again from here.
        m_unmapped = false;

        if (traceEnabled())
            std::fprintf(stderr,
                "[wayland] config  %p %-7s geometry=%dx%d surface=%dx%d buffer=%dx%d scale=%d/120"
                " act=%d max=%d tiled=%d full=%d\n",
                static_cast<void*>(this), roleName(m_role), m_geometrySize.x, m_geometrySize.y,
                m_surfaceSize.x, m_surfaceSize.y, m_size.x, m_size.y, m_scale120,
                m_activated ? 1 : 0, m_maximized ? 1 : 0, m_tiled ? 1 : 0, m_fullscreen ? 1 : 0);

        // THE COMPOSITOR IS TOLD WHAT THE BUFFER MEANS. Without this it reads a buffer of real
        // pixels as one of surface pixels and puts a window twice the size on screen. The
        // destination has to be restated whenever the surface size changes, not only the scale.
        if (firstConfigure || scaleWasChanged || sizeChanged)
            applyScaleToSurface();
        if (firstConfigure || sizeChanged || frameChanged)
            stateGeometry();

        if (firstConfigure || sizeChanged)
            m_buffers.resize(m_size);
        if (firstConfigure || sizeChanged || frameChanged)
            resized(m_size, frame);
        // TOLD AFTER THE SIZE, because laying out at a new scale needs the canvas that scale
        // asked for. The form re-measures everything it holds against it.
        if (scaleWasChanged)
            scaleChanged();
        if (focusWasChanged)
            focusChanged(m_activated);
        if (firstConfigure || sizeChanged || frameChanged || focusWasChanged || scaleWasChanged)
            invalidateRect(nullptr);
    }

    // THE SCALE A CHANGE OF OUTPUTS IMPLIES. A compositor that moves a window between screens
    // changes what its pixels are worth without changing the size it asked for, so no configure
    // follows and there is nothing to wait for - applyConfigureless does the work instead.
    void Window::updateScaleFromOutputs()
    {
        // A POPUP'S SCALE IS ITS PARENT'S AND NOT ITS SCREEN'S. It still enters and leaves
        // outputs, and what they say is not read.
        if (isPopupRole())
            return;

        // OVERRULED WHERE THE COMPOSITOR STATES THE RATIO ITSELF. The outputs report whole
        // numbers, so reading them over a fractional scale would round 175% down to 100%.
        if (m_fractionalScale)
            return;

        DisplayManager& manager = m_display;
        int scale = 1;
        for (wl_output* output : m_outputs)
            scale = std::max(scale, manager.scaleForOutput(output));

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] outputs %p on=%zu scale=%d (was %d)\n",
                static_cast<void*>(this), m_outputs.size(), scale, m_scale120 / k_scaleUnit);

        if (scale * k_scaleUnit == m_pendingScale120)
            return;

        m_pendingScale120 = scale * k_scaleUnit;
        if (m_configured)
            applyConfigureless();
    }

    // A SCALE CHANGE WITH NO CONFIGURE BEHIND IT. Moving a window to a denser screen changes what
    // its pixels mean without changing the size the compositor asked for, so there is no serial
    // to acknowledge and nothing to ack - only a buffer to rebuild and a form to re-measure.
    void Window::applyConfigureless()
    {
        if (m_pendingScale120 == m_scale120)
            return;

        m_scale120 = m_pendingScale120;
        updateSurfaceSize();

        if (traceEnabled())
            std::fprintf(stderr,
                "[wayland] rescale %p surface=%dx%d buffer=%dx%d scale=%d/120\n",
                static_cast<void*>(this), m_surfaceSize.x, m_surfaceSize.y,
                m_size.x, m_size.y, m_scale120);

        applySize();
        scaleChanged();
    }

    // HOW THE BUFFER IS TO BE READ, stated in whichever of the two ways this compositor offers.
    // With a viewport the buffer scale stays 1 and the destination carries the ratio, which is
    // the only way to state a fraction; without one the scale must be a whole number, and a
    // fractional one has already been rounded away before it reaches here.
    void Window::applyScaleToSurface()
    {
        if (!m_surface)
            return;

        if (m_viewport)
        {
            ::wl_surface_set_buffer_scale(m_surface, 1);
            // A DESTINATION OF ZERO IS A PROTOCOL ERROR, and a protocol error takes the connection
            // down with it. A window reaching here before it has been given a size states its
            // destination at the first configure instead, which is where a size arrives.
            if (m_surfaceSize.x > 0 && m_surfaceSize.y > 0)
                ::wp_viewport_set_destination(m_viewport, m_surfaceSize.x, m_surfaceSize.y);
            return;
        }

        ::wl_surface_set_buffer_scale(m_surface, m_scale120 / k_scaleUnit);
    }

    void Window::onSurfaceConfigure(void* data, xdg_surface*, std::uint32_t serial)
    {
        static_cast<Window*>(data)->applyConfigure(serial);
    }

    void Window::onToplevelConfigure(void* data, xdg_toplevel*, std::int32_t width, std::int32_t height,
        wl_array* states)
    {
        Window& window = *static_cast<Window*>(data);
        window.m_pendingMaximized = false;
        window.m_pendingTiled = false;
        window.m_pendingFullscreen = false;
        window.m_pendingActivated = false;

        // THE STATES ARE THE WHOLE TRUTH ABOUT THIS WINDOW, and they arrive as a list rather than
        // as flags: one not named is one this window is not in. ACTIVATED is among them rather
        // than on the keyboard, which is why focus is known with no seat bound at all.
        const std::uint32_t* first = static_cast<const std::uint32_t*>(states->data);
        const std::size_t count = states->size / sizeof(std::uint32_t);

        if (traceEnabled())
        {
            std::fprintf(stderr, "[wayland] states  %p %dx%d [", static_cast<void*>(&window),
                width, height);
            for (std::size_t i = 0; i < count; ++i)
                std::fprintf(stderr, " %u", first[i]);
            std::fprintf(stderr, " ]  (1=max 2=full 3=resizing 4=activated 5..8=tiled)\n");
        }

        for (std::size_t i = 0; i < count; ++i)
        {
            switch (first[i])
            {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:
                window.m_pendingMaximized = true;
                break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:
                window.m_pendingFullscreen = true;
                break;
            case XDG_TOPLEVEL_STATE_TILED_LEFT:
            case XDG_TOPLEVEL_STATE_TILED_RIGHT:
            case XDG_TOPLEVEL_STATE_TILED_TOP:
            case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
                window.m_pendingTiled = true;
                break;
            case XDG_TOPLEVEL_STATE_ACTIVATED:
                window.m_pendingActivated = true;
                break;
            default:
                break;
            }
        }

        // ZERO IS NOT A SIZE, IT IS A QUESTION - the compositor saying it has no requirement and
        // the window may be whatever it wants. WHICH SIZE THAT IS DEPENDS ON WHAT IT IS BECOMING.
        // A window being un-maximized is asked to choose, and the only right answer is the size
        // it had before it was maximized; answering with the size it has now leaves it filling
        // the screen with the state of an ordinary window, which is a window that cannot be
        // restored. Anything else keeps what it has, which on the first configure is what the
        // content measured.
        //
        // AN UN-MAXIMIZE THAT NAMES THE MAXIMIZED SIZE IS THE SAME QUESTION. KWin's session
        // record of a window closed maximized holds the maximized rect, and a window restored
        // from it is un-maximized into that rect - the size it already fills, with the state of
        // an ordinary window. Outside the maximized, fullscreen and tiled states the named size
        // is a hint, and this window's own memory is the size to come back to.
        const IntSize named = { width, height };
        const bool unMaximizedInPlace = window.m_maximized && !window.m_pendingMaximized
            && !window.m_pendingFullscreen && !window.m_pendingTiled
            && named == window.m_geometrySize && window.m_restoreSize.x > 0;
        if (width > 0 && height > 0 && !unMaximizedInPlace)
            window.m_pendingSize = named;
        else if (!window.m_pendingMaximized && !window.m_pendingFullscreen && window.m_restoreSize.x > 0)
            window.m_pendingSize = window.m_restoreSize;
        else
            window.m_pendingSize = window.m_geometrySize;
    }

    void Window::onToplevelClose(void* data, xdg_toplevel*)
    {
        static_cast<Window*>(data)->closeRequested();
    }

    // The compositor had a record of this toplevel and the configure that follows carries it.
    void Window::onToplevelSessionRestored(void* data, xdg_toplevel_session_v1*)
    {
        if (traceEnabled())
            std::fprintf(stderr, "[wayland] session %p restored toplevel\n", data);
    }

    // THE POSITION IS STATED AND UNREAD. x and y are where the compositor put this popup in its
    // parent's coordinates; nothing above the platform layer is told where a window is, and the
    // size is the whole of what a form needs back.
    void Window::onPopupConfigure(void* data, xdg_popup*, std::int32_t x, std::int32_t y,
        std::int32_t width, std::int32_t height)
    {
        Window& window = *static_cast<Window*>(data);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] popup   %p at %d,%d %dx%d\n",
                static_cast<void*>(&window), x, y, width, height);

        if (width > 0 && height > 0)
            window.m_pendingSize = { width, height };
        else
            window.m_pendingSize = window.m_geometrySize;
    }

    // THE COMPOSITOR HAS TAKEN IT AWAY, and it is gone whether this client agrees or not - the
    // parent unmapped, this popup unmapped, or a grab broken. There is nothing here to refuse.
    //
    // THE ROLE GOES AND THE WINDOW STAYS. Closing outright would destroy the wl_surface and
    // unregister the sink, which is unrecoverable - and a tooltip is ONE window per form, shown
    // and hidden for the life of the application. It is the same teardown hide() does, and the
    // next placement builds a new popup on the surface that is still standing.
    void Window::onPopupDone(void* data, xdg_popup*)
    {
        Window& window = *static_cast<Window*>(data);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] done    %p %-7s dismissed\n",
                static_cast<void*>(&window), roleName(window.m_role));

        window.m_visible = false;
        window.destroyPopupRole();
    }

    // Which reposition this answers. One placement is ever in flight on a popup here, so the
    // token is traced rather than matched against anything.
    void Window::onPopupRepositioned(void* data, xdg_popup*, std::uint32_t token)
    {
        if (!traceEnabled())
            return;

        std::fprintf(stderr, "[wayland] repos   %p token=%u\n", data, token);
    }

    // A LAYER SURFACE'S CONFIGURE IS THE WHOLE PROPOSAL IN ONE EVENT - the size, and nothing
    // else, since a layer surface has no states. It answers the width this window asked for and
    // the height the anchors gave it; zero on an axis leaves the choice to this window, which
    // keeps what it has.
    void Window::onLayerConfigure(void* data, zwlr_layer_surface_v1*, std::uint32_t serial,
        std::uint32_t width, std::uint32_t height)
    {
        Window& window = *static_cast<Window*>(data);

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] layer   %p %ux%u\n",
                static_cast<void*>(&window), width, height);

        window.m_pendingSize = {
            width > 0 ? static_cast<int>(width) : window.m_geometrySize.x,
            height > 0 ? static_cast<int>(height) : window.m_geometrySize.y
        };
        window.applyConfigure(serial);
    }

    // The output it stood on is gone, or the user had it removed. Nothing further reaches the
    // screen, so the window is closed as a toplevel is when the compositor asks.
    void Window::onLayerClosed(void* data, zwlr_layer_surface_v1*)
    {
        static_cast<Window*>(data)->closeRequested();
    }

    // ASKED FOR BEFORE THE COMMIT THAT CARRIES IT. The callback belongs to the surface state this
    // commit is about to apply, so requesting it afterwards attaches it to the next frame instead
    // and the answer never comes.
    // DESTROYS THE ONE THAT FIRED, and never whatever the window happens to be holding. The two
    // are not always the same object: a frame can still be owed when the role under it is torn
    // down, which is exactly what hiding a popup does, and a present made while one was already
    // outstanding leaves two alive to answer. Reaching for the member instead destroys the wrong
    // callback, or a null - which is a segfault inside libwayland with this frame on the stack.
    //
    // A STALE ONE ANSWERS NOTHING ELSE. The frame it belonged to is gone, and the callback the
    // window is holding now is the one that will ask for the next.
    void Window::onFrameDone(void* data, wl_callback* callback, std::uint32_t)
    {
        Window& window = *static_cast<Window*>(data);
        ::wl_callback_destroy(callback);
        if (window.m_frameCallback != callback)
            return;

        window.m_frameCallback = nullptr;
        if (!window.m_dirtyRect.empty())
            window.paint();
    }

    // WHICH OUTPUTS THIS SURFACE OVERLAPS. This is how a client learns its scale on a compositor
    // older than wl_compositor 6, which is most of them: the surface is told what it is on, the
    // outputs were each told their own scale, and the client takes the largest. A compositor that
    // names the scale itself sends preferred_buffer_scale instead and overrules all of this.
    void Window::onSurfaceEnter(void* data, wl_surface*, wl_output* output)
    {
        Window& window = *static_cast<Window*>(data);
        if (std::find(window.m_outputs.begin(), window.m_outputs.end(), output)
            == window.m_outputs.end())
            window.m_outputs.push_back(output);

        window.updateScaleFromOutputs();
    }

    void Window::onSurfaceLeave(void* data, wl_surface*, wl_output* output)
    {
        Window& window = *static_cast<Window*>(data);
        std::erase(window.m_outputs, output);
        window.updateScaleFromOutputs();
    }

    // THE SCALE THIS SURFACE SHOULD DRAW AT, named by the compositor. Answering it is what makes
    // the difference between a window drawn at the screen's resolution and one drawn small and
    // stretched up to it - which looks the same on a filled rectangle and does not on a glyph.
    //
    // Nothing is applied here. A scale change resizes the buffer and re-lays-out the form, and
    // both of those belong with the configure that states the size they are about.
    void Window::onPreferredBufferScale(void* data, wl_surface*, std::int32_t factor)
    {
        Window& window = *static_cast<Window*>(data);
        if (factor < 1 || window.m_fractionalScale || window.isPopupRole())
            return;

        window.m_pendingScale120 = factor * k_scaleUnit;
    }

    // ALREADY IN 120ths - the protocol states the ratio in the same unit this layer holds it in,
    // so 175% arrives as 210 and needs no conversion and no rounding.
    void Window::onPreferredFractionalScale(void* data, wp_fractional_scale_v1*, std::uint32_t scale)
    {
        Window& window = *static_cast<Window*>(data);
        if (scale == 0 || window.isPopupRole())
            return;

        if (traceEnabled())
            std::fprintf(stderr, "[wayland] fscale  %p preferred=%u/120\n",
                static_cast<void*>(&window), scale);

        window.m_pendingScale120 = static_cast<int>(scale);
        if (window.m_configured)
            window.applyConfigureless();
    }

    // Rotation and flipping. Nothing here draws pre-transformed, so the compositor is left to do
    // it, which is what not answering means.
    void Window::onPreferredBufferTransform(void*, wl_surface*, std::uint32_t)
    {
    }
}
