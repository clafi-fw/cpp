export module ClaFi.Core.Context.FormContext;

// FormContext has to be its own subsystem - it is used from Foundation and from TextEngine.
//
// It deliberately does not know about AppContext. Holding one would put the whole config and DOM
// stack underneath every module that reaches FormContext - which is the entire TextEngine and, above
// it, all of Foundation. What FormContext actually needs from the application is the theme, so that
// is what it takes. Whoever owns the AppContext (FormBase) hands out access to it.

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.Transfer.Clipboard;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;

import ClaFi.Core.Graphics.ShadowPainter;
import ClaFi.Core.Graphics.Canvas;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;

namespace ClaFi
{
    export struct Platform;

    export class FormContext
    {
    public:
        FormContext(const Platform&, const AppTheme&, const BakedColors&, Graphics::Canvas&,
            const Scaler&);
        FormContext(const FormContext&);
        //
        void setTheme(const AppTheme& value) { m_theme = &value; }
        void setBakedColors(const BakedColors& value) { m_bakedColors = &value; }
        // The scaler this context reads. Named rather than fixed at construction because a form
        // that is drawn at another form's scale outlives that form in one case - see
        // FormBase::detachScaler.
        void setScaler(const Scaler& value) { m_scaler = &value; }
        const AppTheme*& themePtrRef() { return m_theme; }
        const BakedColors*& bakedColorsPtrRef() { return m_bakedColors; }
        const AppTheme& theme() const { return *m_theme; }
        const ThemeMetrics& themeMetrics() const { return m_theme->metrics; }
        const BakedColors& bakedColors() const { return *m_bakedColors; }
        //
        const Scaler& scaler() const {  return *m_scaler; }
        const Scaler& scaler() { return *m_scaler; }
        FloatPoint scale(FloatPoint value) const { return m_scaler->scale(value); }
        FloatPoint scaleF(FloatPoint value) const { return m_scaler->scaleF(value); }
        float scaleF(float value) const { return m_scaler->scaleF(value); }
        float scaleBorder(float value) const { return m_scaler->scaleBorder(value); }
        float scaledStrokeWidth(Thickness thickness) const { return m_scaler->scaledStrokeWidth(thickness); }
        ScaleFactor scaleFactor() const { return m_scaler->factor(); }
        //
        Graphics::Canvas& canvas() { return m_canvas; }
        const Graphics::Canvas& canvas() const { return m_canvas; }
        // The clipboard the platform owns. Not const: it is the process's rather than this
        // context's state, and a copy made from a const context is still a copy.
        [[nodiscard]] Transfer::Clipboard& clipboard() const;
    private:
        const Platform& m_platform;
        const AppTheme* m_theme;
        const BakedColors* m_bakedColors;
        const Scaler* m_scaler;
        Graphics::Canvas& m_canvas;
    };

    export class ControlEventBase : public Event
    {
    public:
        ControlEventBase(FormContext&);
        //
        FormContext& formContext() { return m_formContext; }
        const FormContext& formContext() const { return m_formContext; }
        //
        const AppTheme& theme() const { return m_formContext.theme(); }
        const ThemeMetrics& themeMetrics() const { return m_formContext.themeMetrics(); }
        const BakedColors& bakedColors() const { return m_formContext.bakedColors(); }
        //
        const Scaler& scaler() const { return m_formContext.scaler(); }
        ScaleFactor scaleFactor() const { return m_formContext.scaler().factor(); }
        float scale(float value) const { return m_formContext.scaler().scale(value); }
        FloatPoint scale(FloatPoint value) const { return m_formContext.scaler().scale(value); }
        float scaleF(float value) const { return m_formContext.scaler().scaleF(value); }
        FloatPoint scaleF(FloatPoint value) const { return m_formContext.scaler().scaleF(value); }
        float scaleBorder(float value) const { return m_formContext.scaler().scaleBorder(value); }
        float scaledStrokeWidth(Thickness thickness) const { return m_formContext.scaler().scaledStrokeWidth(thickness); }
        //
        Graphics::Canvas& canvas() { return m_formContext.canvas(); }
    private:
        // The only reason why it's stored by value here, is because that way
        // it's possible to swap the theme for the preview panel in the Themes editor app
        FormContext m_formContext;
    };

    export class ControlEventBaseC : public Event
    {
    public:
        explicit ControlEventBaseC(const FormContext&);
        //
        const FormContext& formContext() const { return m_formContext; }
        //
        const AppTheme& theme() const { return m_formContext.theme(); }
        const ThemeMetrics& themeMetrics() const { return m_formContext.themeMetrics(); }
        const BakedColors& bakedColors() const { return m_formContext.bakedColors(); }
        //
        const Scaler& scaler() const { return m_formContext.scaler(); }
        ScaleFactor scaleFactor() const { return m_formContext.scaler().factor(); }
        float scale(float value) const { return m_formContext.scaler().scale(value); }
        FloatPoint scale(FloatPoint value) const { return m_formContext.scaler().scale(value); }
        float scaleF(float value) const { return m_formContext.scaler().scaleF(value); }
        float scaleBorder(float value) const { return m_formContext.scaler().scaleBorder(value); }
        float scaledStrokeWidth(Thickness thickness) const { return m_formContext.scaler().scaledStrokeWidth(thickness); }
        //
        const Graphics::Canvas& canvas2() const { return m_formContext.canvas(); }
    private:
        const FormContext& m_formContext;
    };

    export struct KeyDownEvent : public Event
    {
        KeyDownEvent(KeyCode key, KeyModifiers modifiers, bool isRepeat, bool& handled,
            InputStamp stamp = {})
            :
            key{ key },
            modifiers{ modifiers },
            isRepeat{ isRepeat },
            handled{ handled },
            stamp{ stamp }
        {
        }
        KeyCode key;
        KeyModifiers modifiers;
        bool isRepeat; // the key was already down and the system is repeating it. See Context
        bool& handled;
        // What the display server called this press, for a request the key justifies. See Context
        InputStamp stamp;
    };

    // THE ROOM A SURFACE KEEPS AROUND THE WINDOW IT SHOWS, in real pixels. See Context
    export struct FrameMargins
    {
        int left{};
        int top{};
        int right{};
        int bottom{};
        [[nodiscard]] bool empty() const { return !(left || top || right || bottom); }
        [[nodiscard]] IntSize total() const { return { left + right, top + bottom }; }
        [[nodiscard]] bool operator==(const FrameMargins&) const = default;
    };

    // The frame a window wears, in real pixels: margins and corner radius. See Context
    export struct WindowFrame
    {
        FrameMargins margins{};
        float radius{};
        [[nodiscard]] bool operator==(const WindowFrame&) const = default;
    };

    export class IPlatformWindow;

    export class IForm
    {
    public:
        virtual ~IForm() = default;
        virtual IPlatformWindow& wnd_window() = 0;
        const IPlatformWindow& wnd_window() const { return const_cast<IForm*>(this)->wnd_window(); }
        virtual void wnd_beforePaint() = 0;
        virtual void wnd_paint(void* nativeContext, IntRect& dirtyRect, Graphics::Bitmap*&) = 0;
        virtual HitTest wnd_hitTest(PointInForm) = 0;
        virtual void wnd_mouseMove(PointInForm) = 0;
        virtual void wnd_ncMouseDown(PointInForm, InputStamp) = 0;
        // A press on a part of the window the SYSTEM acts on itself - the caption it drags the
        // window by, the frame it sizes it from. No position is passed: the form is told that a
        // press landed somewhere it does not own, which is all it can act on.
        //
        // Answers whether the system may go on with it. False keeps the press.
        virtual bool wnd_systemMouseDown() = 0;
        virtual void wnd_mouseDown(PointInForm, InputStamp, bool& handled) = 0;
        virtual void wnd_mouseUp(PointInForm) = 0;
        virtual void wnd_doubleClick(PointInForm, InputStamp) = 0;
        virtual void wnd_tripleClick(PointInForm, InputStamp) = 0;
        virtual void wnd_mouseLeave() = 0;
        // A menu asked for at the point, or from the keyboard with none; the stamp is the press's.
        virtual void wnd_contextMenu(PointInForm*, InputStamp) = 0;
        virtual void wnd_mouseWheel(PointInForm, float wheelDelta) = 0;
        virtual void wnd_mouseHWheel(PointInForm, float wheelDelta) = 0;
        virtual void wnd_keyDown(KeyDownEvent& key) = 0;
        virtual void wnd_keyUp() = 0;
        virtual void wnd_char(wchar_t value) = 0;
        // THE SURFACE THE BITMAP COVERS, and the frame the platform is applying inside it. The
        // window the user sees is the surface less the frame's margins.
        virtual void wnd_resize(IntSize surface, const WindowFrame& frame) = 0;
        // The shadow this form casts, for a platform that draws it from a window of its own.
        [[nodiscard]] virtual const Graphics::ShadowPainter& wnd_shadowPainter() const = 0;
        virtual void wnd_posChanged() = 0;
        virtual void wnd_focusChanged() = 0;
        virtual void wnd_minimize() = 0;
        virtual void wnd_maximize() = 0;
        virtual void wnd_restore() = 0;
        virtual void wnd_setScalePercent(int value) = 0;
        // The system asked for this window to be closed - Alt+F4, the taskbar, the compositor.
        virtual void wnd_closeRequested() = 0;
    };

    // What a window asks to be placed by, all of it the form's own knowledge. See Context
    export struct WindowPlacement
    {
        FormPlacement placement{ FormPlacement::Default };
        // What this window stands on, in the PARENT WINDOW's coordinates. A window raised at the
        // POINTER names an empty rect where the pointer is, so a placement reads one field for
        // every kind and never asks where the mouse is. Default states nothing here.
        FloatRect anchorRect{};
        // What the window is asking for: the preferred size where its root states one, and what
        // the content came to where it does not - see ControlMetrics::preferredSize.
        ScaledDimensions size{};
        // The bounds it will accept. minSize is the floor the window's content states - the
        // MinSize set on its root or anywhere below it, composed up the tree, and zero where
        // nobody set one. A window that would rather stand over what it is placed on than lose
        // what falls past a cut says so with a MinSize. maxSize is what it may ever be grown to:
        // nothing in the arithmetic here reads it, and it is what a platform that states a
        // window's range to the system - xdg_toplevel::set_max_size - says.
        ScaledDimensions minSize{};
        ScaledDimensions maxSize{};
        // How far an anchored window stands clear of what it drops from. Zero for a window on
        // the pointer, which hangs off the point itself.
        float clearance{};
        // How far a window is held off the edges of the work area. It is what the room on each
        // side is measured against as well as what the result is held inside, so a window given
        // exactly the room a side has is not then pushed back over its own anchor by it.
        float screenMargin{};
        // Where this window's own text starts inside it. OverText lands the two texts on top of
        // each other, and it is the only placement that reads this.
        FloatPoint textOrigin{};
    };

    // What the placement did. See Context
    export struct PlacedWindow
    {
        // What the window got. Shorter than what was asked for only when the window said it may
        // be made shorter, or when the work area could not hold what it asked for.
        ScaledDimensions size{};
    };

    export class IPlatformWindow
    {
    public:
        virtual ~IPlatformWindow() = default;
        // Helpers
        void invalidateRect(const FloatRect& value) { invalidateRect(value.roundedOut()); }
        void invalidateRect(const IntRect& value) { invalidateRect(&value); }
    public:
        // WHETHER THIS WINDOW'S ALPHA CHANNEL MEANS ANYTHING. False is the standing answer and the
        // cheap one: a frame is opaque, nothing clears it, and every pixel is expected to be
        // covered by whatever draws over it. A window the display server COMPOSITES - one with a
        // rounded corner, or one that fades - answers true, and its frames then start from
        // nothing so that a pixel nobody covered reads as absent rather than as stale.
        [[nodiscard]] virtual bool wantsAlphaChannel() const { return false; }
        virtual void doLoop() = 0;
        virtual void show() = 0;
        virtual void hide() = 0;
        virtual void close() = 0;
        virtual void minimize() = 0;
        virtual void maximize() = 0;
        virtual void restore() = 0;
        [[nodiscard]] virtual bool isFocused() = 0;
        [[nodiscard]] virtual bool isMaximized() = 0;
        [[nodiscard]] virtual bool isMinimized() = 0;
        // Brings the window forward with the keyboard, authorised by the input that asked for it.
        virtual void setFocus(InputStamp) = 0;
        virtual void invalidateRect(const IntRect*) = 0;

        // [[nodiscard]] virtual std::wstring title() = 0;
        virtual void setTitle(const std::wstring_view) = 0;
        virtual void update() = 0;
        // THE RANGE A USER'S DRAG IS HELD INSIDE: the floor the content states, and the most it
        // may ever grow to. Stated whenever a measuring pass changes either, because that pass is
        // what answers both - what holds a window up changes with what is in it. k_maxFloat on an
        // axis is no maximum at all, and zero is no minimum: the system's own default stands.
        //
        // It is the same range a placement gives way inside, and on a platform that states a
        // window's range to the system rather than enforcing it itself - xdg_toplevel's
        // set_min_size and set_max_size - this call is that one.
        virtual void setSizeRange(ScaledDimensions minSize, ScaledDimensions maxSize) = 0;
        // THE FRAME THE FORM WANTS, from its theme and scale. The platform keeps it and answers
        // with the frame it applies on every resize; a platform that cannot composite a frame
        // answers zero and the form paints none.
        virtual void setFrame(const WindowFrame&) = 0;
        // PLACES THIS WINDOW ON WHAT IT STANDS ON, and answers what it got. The window is moved
        // and sized by this call: which monitor, the room above against the room below, flipping
        // or sliding, and holding the result inside the work area are all the platform's, which
        // is the only layer that can answer them at all.
        virtual PlacedWindow place(const WindowPlacement&) = 0;
        virtual ColorByte alpha() = 0;
        virtual void setAlpha(ColorByte value) = 0;
        // HANDS THE WINDOW TO THE SYSTEM TO BE DRAGGED, on the press that asked for it. The stamp
        // is the PRESS's and not the move's: a display server that authorises this weighs the
        // request against the event that started the gesture.
        virtual void initiateWindowDrag(IntPoint pt, InputStamp) = 0;
        // The system's own menu for this window, at the point, on the press that asked. See Context
        virtual void showWindowMenu(PointInForm, InputStamp) = 0;
        // WHETHER THE CLIENT MAY HOLD THIS WINDOW ABOVE THE OTHERS. A display server that keeps
        // that state to itself answers false, and the two calls below then state nothing.
        [[nodiscard]] virtual bool canSetAlwaysOnTop() const = 0;
        [[nodiscard]] virtual bool isAlwaysOnTop() const = 0;
        virtual void setAlwaysOnTop(bool) = 0;
    };

    // A backend answers to one of two contracts, and the contract is what the core knows about - not
    // the implementation behind it. A GPU backend draws into a window owned by the platform, so it
    // is handed the window and reads whatever native object it needs out of it. A CPU backend owns
    // its own pixel buffer and needs nothing but a size.
    //
    // The two are mutually exclusive by construction: IsCpuBackend excludes IsGpuBackend, so a type
    // that happens to accept both argument lists is treated as a GPU backend rather than silently
    // resolving to whichever branch is tested first.

    export template <class B>
        concept IsGpuBackend = std::derived_from<B, Graphics::IBackend>
            && requires (IPlatformWindow& window, IntSize size) { B{ window, size }; };

    export template <class B>
        concept IsCpuBackend = std::derived_from<B, Graphics::IBackend>
            && !IsGpuBackend<B>
            && requires (IntSize size) { B{ size }; };

    export template <class B>
        concept IsBackend = IsGpuBackend<B> || IsCpuBackend<B>;

    // How the backend choice reaches a form. Application is a template and FormBase is not, so the
    // chosen type cannot travel down as a type - it travels as the address of makeBackend<Backend>,
    // which is a plain function pointer and carries no state.
    export using BackendFactory = std::unique_ptr<Graphics::IBackend> (*)(IPlatformWindow&, IntSize);

    export template <IsBackend Backend>
        [[nodiscard]] std::unique_ptr<Graphics::IBackend> makeBackend(IPlatformWindow& window, IntSize size)
    {
        std::unique_ptr<Graphics::IBackend> backend;
        if constexpr (IsGpuBackend<Backend>)
        {
            backend = std::make_unique<Backend>(window, size);
        }
        else
        {
            // A CPU BACKEND IS BUILT FROM A SIZE ALONE - see IsCpuBackend, which states that.
            backend = std::make_unique<Backend>(size);
        }
        // THE ONE THING A BACKEND IS TOLD ABOUT ITS WINDOW BESIDES ITS SIZE, said here rather than
        // taken through either constructor, so that a backend arrives ready whichever branch built
        // it - a backend put into a canvas that is already running is built through here too. A
        // backend that does not care ignores it.
        backend->setTransparentBase(window.wantsAlphaChannel());
        return backend;
    }

    // The GPU backend an application names, or void where it names none. The CPU backend is the
    // core's own and is always there - see AppContext::createBackend - so this is the whole of
    // the choice an application makes.
    export template <class B>
        concept IsOptionalGpuBackend = std::is_void_v<B> || IsGpuBackend<B>;

    // What an application hands down for its GPU backend. Null where it named none, which is the
    // answer AppContext::gpuAvailable stands on.
    export template <IsOptionalGpuBackend Backend>
        [[nodiscard]] constexpr BackendFactory gpuBackendFactory()
    {
        if constexpr (std::is_void_v<Backend>)
        {
            return nullptr;
        }
        else
        {
            return &makeBackend<Backend>;
        }
    }

    // The desktop has changed the mode it asks applications to be drawn in. See Context
    export struct SystemColorModeEvent : public Event
    {
    };

    // What an application is built against - the statics every platform answers, the window
    // factory and the clipboard the platform built for the display server owns. See Context
    export struct Platform
    {
        using FormNames = std::initializer_list<std::wstring_view>;
        // Names the clipboard the derived platform owns as a member. See Transfer
        explicit Platform(Transfer::Clipboard&);
        virtual ~Platform() = default;
        // The one clipboard, owned by the platform built for the display server. See Transfer
        [[nodiscard]] Transfer::Clipboard& clipboard() const { return m_clipboard; }
        // A form's window. The derived platform answers, holding what a window is made from.
        [[nodiscard]] virtual std::unique_ptr<IPlatformWindow> createWindow(IForm&, WindowRole,
            IForm* parentForm) = 0;
        // What the platform found the display server offering, one diagnostic log line each.
        [[nodiscard]] virtual std::vector<std::wstring> diagnosticLines() const { return {}; }
        // What the desktop knows this application by - the two names its config is kept under.
        static void setApplicationId(std::wstring_view publisher, std::wstring_view name);
        // The Forms section's shape for these forms on this platform. See Context
        static Dom::Dt::Section createFormsConfigSchema(FormNames);
        // Gives a window the placement kept under this name, before it is first placed. See Context
        static void restoreFormPlacement(Dom::Section& forms, std::wstring_view name, IForm&);
        // Writes a window's placement under this name, while the window is still up. See Context
        static void storeFormPlacement(Dom::Section& forms, std::wstring_view name, const IForm&);
        // Names a standing window to the session, storing being allowed now. See Context
        static void addFormToSession(std::wstring_view name, IForm&);
        //
        static std::wstring keyName(const KeyCode key);
        static KeyModifiers keyModifiers();
        //
        static void beep(Frequency frequency = { 2400 }, MilliSeconds duration = { 8 });
        static ScaleFactor globalScaleFactor();
        static MilliSeconds caretBlinkTime();
        // The CPU time this process has consumed since it started, kernel and user together. The
        // step between two readings over the wall time between them is the share of one core the
        // process took - see Diagnostic::CpuUsage, which is what asks.
        static std::chrono::nanoseconds processCpuTime();
        //
        // The shape the pointer shows inside the window.
        static void setCursor(CursorShape);
        // The cursor shown now, read from the platform - one the system set, a sizing arrow,
        // included - and the same cursor put back. What ScopedWaitCursor stands on.
        [[nodiscard]] static Cursor cursor();
        static void setCursor(Cursor);
        static CursorInfo getCursorInfo(CursorShape);
        //
        static void shellExecute(const IForm*, const std::wstring_view file,
            const std::wstring_view params = {});
        static void shellExecute(const IForm& form, const std::wstring_view file,
            const std::wstring_view params = {}) { shellExecute(&form, file, params); }
        //
        static std::wstring appDataPath();
        //
        // A line where a debugger shows it - the debugger's output on Win32, stderr elsewhere.
        static void debugOutput(std::wstring_view line);
        //
        // The mode the desktop asks applications to be drawn in. See Context
        [[nodiscard]] static ColorMode systemColorMode();
        // Where a change of the mode the desktop asks for is announced. See Context
        [[nodiscard]] static EventDispatcher& events();
    private:
        Transfer::Clipboard& m_clipboard;
    };

    // Shows the wait shape while alive and puts back the shape the platform was showing when it
    // was made - a sizing arrow the system set included - so scopes nest and a drag keeps its
    // arrow.
    export class ScopedWaitCursor
    {
    public:
        ScopedWaitCursor();
        ~ScopedWaitCursor();
        ScopedWaitCursor(const ScopedWaitCursor&) = delete;
        ScopedWaitCursor& operator=(const ScopedWaitCursor&) = delete;
    private:
        Cursor m_previous;
    };

}
