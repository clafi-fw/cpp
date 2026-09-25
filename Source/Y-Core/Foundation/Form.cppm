module;
#include "../System/EventBindings.h"

export module ClaFi.Core.Foundation :Form;

import :Control;
import :RichControl;
import :Action;
import :PaintEvent;
import :Tooltip;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.Graphics.ShadowPainter;
import ClaFi.Core.Graphics.Canvas;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.Graphics.Types;


namespace ClaFi
{
    struct SearchControlResult
    {
        Control* control{};
        FloatPoint relativePt{};
        HitTest hitZone{ HitTest::Client };
        // The pointer stands on the control's own text.
        bool overText{};
        operator bool() { return control; }
    };

    /// @brief The form is closing. sender() is the form.
    export class FormCloseEvent : public EventOf<FormBase>
    {
    public:
        using EventOf<FormBase>::EventOf;
    };

    /// @brief The form gained or lost the window focus. sender() is the form.
    export class FormFocusChangeEvent : public EventOf<FormBase>
    {
    public:
        using EventOf<FormBase>::EventOf;
    };

    /// @brief The form was moved, resized, maximized or restored. sender() is the form.
    export class FormPositionChangeEvent : public EventOf<FormBase>
    {
    public:
        using EventOf<FormBase>::EventOf;
    };

    // The form's content is laid out and nothing asked for another pass. See Control-Foundation
    export class FormAlignedEvent : public EventOf<FormBase>
    {
    public:
        using EventOf<FormBase>::EventOf;
    };

    // The form has painted: the rect, and when the paint began and ended. See Control-Foundation
    export class FormPaintedEvent : public EventOf<FormBase>
    {
    public:
        using TimePoint = std::chrono::steady_clock::time_point;
        FormPaintedEvent(FormBase&, const IntRect& dirtyRect, TimePoint startedAt, TimePoint finishedAt);
        /// @brief What was painted, in surface pixels: the whole surface for a frame of the
        /// window, one control's rect for a repaint of that control.
        [[nodiscard]] const IntRect& dirtyRect() const { return m_dirtyRect; }
        [[nodiscard]] TimePoint startedAt() const { return m_startedAt; }
        [[nodiscard]] TimePoint finishedAt() const { return m_finishedAt; }
    private:
        IntRect m_dirtyRect;
        TimePoint m_startedAt;
        TimePoint m_finishedAt;
    };

    // What closing a form does to it.
    export enum class CloseAction {
        Close,   // the form is destroyed
        Hide     // the form stays alive and leaves the screen
    };

    // The root control of a form, and the other half of the FormBase pair. See Control-Foundation
    export class FormControlBase : public RichControl
    {
        // The other half of the pair. FormBase holds its root as one of these and asks it for
        // Control::focusDelegate, which is protected: a friend of this class reaches an
        // inherited protected member through an object of this class, and Control's own
        // friendship with FormBase does not.
        friend FormBase;
    public:
        template<typename ...Args>
        FormControlBase(const CreateParams& params, Args&& ...args);
        // STOPS THE ROOT'S ANIMATIONS WHILE IT CAN STILL NAME ITS FORM. Control's own destructor
        // stops them too, but by then getForm is Control's, which walks the parents - and a root
        // has none - so the root's are stopped here, where getForm still answers m_form.
        ~FormControlBase() override;
    public:
        DECLARE_PROPERTY(WindowShadow, windowShadow, WindowShadow{}) // the shadow its window casts
    public:
        // Where this root's text begins, measured from THIS ROOT'S top left. A form placed
        // OverText is placed so that this point lands on the text it covers, which is what puts
        // the two texts on top of each other.
        //
        // The root's top left, not the form's: the root is laid out into the window's geometry,
        // and the geometry is what the platform places. The form's origin is the surface's, a
        // frame margin outside the geometry, and a point measured from it lands the window that
        // margin up and to the left of the text.
        //
        // A root that paints its own text answers with its content padding, since that is where
        // its text starts. A root showing a CHILD's text - an in-place editor holds a text box -
        // answers for the child: the child sits inside this control's inset and adds an inset of
        // its own, and neither of those is in this control's padding.
        [[nodiscard]] virtual FloatPoint textOrigin() const;
    protected:
        using RichControl::getForm;
        FormBase* getForm() override;
    private:
        FormBase& m_form;
    };

    export template<typename ControlClass>
        concept ClassOfFormControl = std::derived_from<ControlClass, FormControlBase>;

    export template <ClassOfFormControl ControlClass> class Form;

    export class FormBase : public IForm
    {
        // The root control reports itself as the content while it is being constructed.
        friend FormControlBase;
    public:
        // The role is what kind of window this is, and it is named by whoever builds the form.
        // Nothing about the root is read to arrive at it: what the WINDOW does with activation
        // and how the ROOT CONTROL takes part in focus and selection are separate questions,
        // and the root answers its own with its Interactivity. A popup list whose root tracks a
        // current item is the case that needs them apart - WindowRole::Menu, root
        // ActiveContainer - and neither value can be inferred from the other.
        FormBase(AppContext&, WindowRole, FormControlBase&, Control* popupTarget, FormPlacement);
        // The form this one's window is OWNED BY, named outright rather than read off a control.
        // A popup stands on a control and its form is whatever holds that control; a tooltip is a
        // member of the form it belongs to and is built with it, before it is about any control at
        // all - so it has a form to name and no control to name it through.
        FormBase(AppContext&, WindowRole, FormControlBase&, FormBase& ownerForm, FormPlacement);
        ~FormBase() override;
    public:
        DECLARE_EVENT(FormCloseEvent, OnClose, onClose)
        DECLARE_EVENT(FormFocusChangeEvent, OnFocusChange, onFocusChange)
        DECLARE_EVENT(FormPositionChangeEvent, OnPositionChange, onPositionChange)
        DECLARE_EVENT(FormAlignedEvent, OnAligned, onAligned)
    public:
        // A form has no dispatcher of its own. Its root control is the other half of the same
        // object - Form<T> derives from both - so a form raises its events on the control's
        // dispatcher. That is what lets a handler given as a construction property reach a form
        // event: the pack goes to the root control, and this is the dispatcher it went into.
        //
        // These two forwarders exist so DECLARE_EVENT and the emit sites below can be written the
        // usual way. m_content is set by the root control itself, in FormControlBase's
        // constructor, so it is already there for a child that connects a form event on its way
        // up - FormTitle does exactly that.
        template <IsEvent TEvent>
        EventConnection connectEvent(auto&& callback) {
            return m_content.connectEvent<TEvent>(std::forward<decltype(callback)>(callback));
        }
        template <IsEvent TEvent>
        void emitEvent(TEvent& event) const { m_content.emitEvent(event); }
        FormContext& context() { return m_context; }
        const FormContext& context() const { return m_context; }
        AppContext& appContext() { return m_appContext; }
        const AppContext& appContext() const { return m_appContext; }
        const AppTheme& theme() const { return m_context.theme(); }
        const ThemeMetrics& themeMetrics() const { return m_context.themeMetrics(); }
        const BakedColors& bakedColors() const { return m_context.bakedColors(); }
        // THE SCALE THIS FORM IS DRAWN AT, and it is read-only from the outside: what a form
        // is drawn at is decided by the monitor it was placed on, or by the form it stands on,
        // and by nothing that merely holds it - see m_scaler.
        const Scaler& scaler() const { return *m_scaler; }
        // Stops this form following the scale of the form under it - see the definition.
        void holdScale();
        // Puts it back to being drawn at the scale of the form under it - see the definition.
        void followScale();
        [[nodiscard]] Control& content() { return m_content; }
        // The control the focus goes to when this form is entered. THE ROOT ANSWERS, and only a
        // root that keeps a current item has anything but itself to answer with: a StackPanel or
        // a Grid names the item it is on, and a plain Panel names itself. The form stores
        // nothing of its own - remembering where the focus was is what a container does, and a
        // form whose root is not one has no second place to keep it.
        [[nodiscard]] Control* currentItem() { return m_content.focusDelegate(); }
        // The actions whose shortcuts this form answers, asked before the application's and
        // both only after the focused control has had the key.
        [[nodiscard]] Actions& actions() { return m_actions; }
        [[nodiscard]] float contentHeight() const;
        IPlatformWindow& window() const { return *m_window; }
        // THE FRAME THE WINDOW IS WEARING - what the platform applies, not what the root states.
        [[nodiscard]] const WindowFrame& frame() const { return m_frame; }
        // Where the window stands inside its surface: the surface less the frame's margins, in
        // surface coordinates. The root is laid out into it, or into as much of it as the content
        // asked for - see contentExtent.
        [[nodiscard]] FloatRect geometry() const;
        // What kind of window this is. It says nothing about the root control, which answers for
        // itself with Control::interactivity().
        [[nodiscard]] WindowRole windowRole() const { return m_windowRole; }
        // This form's tooltip. A control raises one about itself through the form it is in -
        // control.form().tooltip().showRightNow(control) - because the window a hint shows in is
        // owned by the window holding the control, and the form is what knows which that is.
        [[nodiscard]] Tooltip& tooltip() { return m_tooltip; }
        void invalidateAlign();
        // Brings a control into view once the alignment it is waiting for has run. A control
        // that has just been shown, or one whose host has just grown, still measures as it did
        // before, so a scroll asked for now would ask for a rect that no longer applies. Only
        // one request is held: a second replaces the first, and the pass that answers it clears
        // it. Laying the form out on the spot instead is what this exists to avoid - a view
        // holding a million items is aligned once per frame, not once per change.
        void scrollIntoViewOnAlign(Control&);
        // The pass being run. A control asks for another through AlignEvent::invalidatePass.
        [[nodiscard]] LayoutPass& layoutPass() { return m_layoutPass; }
        void validateAlign();
        bool contentAligned() const { return m_aligned; }
        // Whether the pass running is measuring to ASK for a window rather than laying out into
        // the one this form has - its align included. A bound a control carries over from the
        // last layout does not hold over this pass, and a width this pass lays out at is not one
        // to carry over - it is the one the form finds out what it wants in.
        [[nodiscard]] bool isMeasuringPlacement() const { return m_measuringPlacement; }
        void invalidate() const;
        // The content has moved or been laid out again, so anything holding a picture of where it
        // was is holding a picture of somewhere else. An alignment pass says this for itself; a
        // scroll has to say it, because moving the content by its top invalidates the paint and
        // nothing more - see ScrollBox::clientScrolled.
        void contentMoved(const Control&) const;
        // A control has been hidden, and a popup standing on it or on anything inside it goes
        // down with it, the way forgetControl takes one down with a deleted control.
        void controlHidden(const Control&);
        void invalidateControl(const Control*) const;
        void invalidateRect(const FloatRect& value) const { m_window->invalidateRect(value); }
        // Where a control's own box starts in form space, and where its children start. Both are
        // summed from the root down, which is the order the paint traversal sums them in.
        // Float addition does not associate, so the same terms added from the leaf up land on a
        // different pixel once the coordinates run into the millions - and an invalidated rect
        // that disagrees with the bounds the control then paints at shaves whatever falls
        // outside it. Everything that needs a form-space position goes through here.
        [[nodiscard]] FloatPoint boundsOriginOfControl(const Control*) const;
        [[nodiscard]] FloatPoint contentOriginOfControl(const Control*) const;
        [[nodiscard]] FloatRect rectOfControl(const Control*, bool clipIntoParents = false) const;
        // THE WINDOW IS A CEILING, NOT A SETTING. What the root may grow to is the window it has,
        // and Control::doAdjustMetrics asks this of the form because a root is the one control
        // with no parent to adjust its metrics. MaxSize is left alone - it is what the form was
        // BUILT with, and nothing in the framework writes it. See the definition.
        void adjustRootMetrics(AdjustMetricsEvent&) const;
        [[nodiscard]] Control* popupTarget() const { return m_popupTarget; }
        [[nodiscard]] FormBase* popupTargetForm();
        [[nodiscard]] FormBase& rootForm();
        FormBase* activePopup() const { return m_activePopup; }
        void closeActivePopup();
        bool isKeyboardClick() const { return m_isKeyboardClick; }
        void close();
        [[nodiscard]] CloseAction closeAction() const { return m_closeAction; }
        void setCloseAction(CloseAction value) { m_closeAction = value; }
        int execute();
        // Whether this form's loop is the one the application stands on. See Application
        [[nodiscard]] bool holdsRootLoop() const;
        void setPlacementRect(const FloatRect&);
        const FormPlacement placement() const { return m_placement; }
        void setPlacement(FormPlacement);
        void setPlacement(FormPlacement, const FloatRect&);
        float dropdownClearance() const { return m_dropdownClearance; }
        void setDropdownClearance(float value) { m_dropdownClearance = value; }
        // The least width the window takes, in design units - a dropdown states the width of the
        // control it fell from. Zero states nothing. Written into the root as a MinSize, which is
        // the floor the placement reads - see adjustRootMetrics.
        [[nodiscard]] float minWidth() const { return m_minWidth; }
        void setMinWidth(float);
        // Whether the window is sized by what is in it. Under AutoFit::Yes every alignment of
        // this form is a placement: the bounds are recomputed from the content each time instead
        // of the content being cut to fit what the window already is.
        [[nodiscard]] AutoFit autoFit() const { return m_autoFit; }
        void setAutoFit(AutoFit value) { m_autoFit = value; }
        // Asked before something OUTSIDE this form closes it - a click on the form behind a
        // popup. False keeps the window up, and the press that asked does nothing else either.
        //
        // IT IS ALLOWED TO ACT, which is why it is not spelled canClose(): a form holding work
        // the user has not finished settles that work here and answers by whether it could. That
        // is what lets an in-place editor whose value nothing will take stay open and say so,
        // rather than losing the edit to a click somewhere else. close() does not ask - Escape
        // has to work whatever state the form is in.
        [[nodiscard]] virtual bool readyToClose() { return true; }
    public:
        // Builds a popup window over this form and gives it back to the caller to run. What the
        // root control is - a list that tracks a current item, a panel that tracks nothing - is
        // the root's own business; this names the WINDOW, and the pack reaches the root
        // untouched.
        template <ClassOfFormControl ControlClass, typename... Args>
        Form<ControlClass> createPopup(Control* ownerButton, Args&&... args);
        void maximize();
        void restore();
        void minimize() { m_window->minimize(); }
        [[nodiscard]] bool isMaximized() { return m_window->isMaximized(); }
        [[nodiscard]] bool isMinimized() { return m_window->isMinimized(); }
        // Held above the other windows. Stored with the placement where the platform can set it.
        [[nodiscard]] bool canSetAlwaysOnTop() const { return m_window->canSetAlwaysOnTop(); }
        [[nodiscard]] bool isAlwaysOnTop() const { return m_window->isAlwaysOnTop(); }
        void setAlwaysOnTop(bool value) { m_window->setAlwaysOnTop(value); }
        // The title the window wears, as this form last stated it. Kept here because the platform
        // window takes a title and does not answer for it.
        [[nodiscard]] std::wstring_view windowTitle() const { return m_windowTitle; }
        void setWindowTitle(std::wstring_view);
        // The name this form's placement is kept under, set before show(). See Control-Foundation
        [[nodiscard]] std::wstring_view configName() const { return m_configName; }
        void setConfigName(std::wstring_view value) { m_configName = value; }
        void update();
        [[nodiscard]] PointInForm mouseDownPos() const { return m_mouseDownPos; }
        [[nodiscard]] InputStamp mouseDownStamp() const { return m_mouseDownStamp; }
        void setMouseDownPos(PointInForm value) { m_mouseDownPos = value; }
        void offsetMouseDownPos(FloatPoint value) { m_mouseDownPos += value; }
        void mouseTick(bool allowDrag = true);
        void mouseTick(PointInForm, bool allowDrag = true);

        void show() { setVisible(true); }
        void hide() { setVisible(false); }
        bool visible() const { return m_visible; }
        void setVisible(bool value);
    public:
        // The layout laid out on the spot, for code whose subject IS the alignment pass - a page
        // measuring what one costs, with a wait cursor over it. Work that merely needs a finished
        // layout asks for a pass instead: invalidateAlign, or Control::scrollIntoViewOnAlign for a
        // scroll. One pass answers every invalidation made before it, and forcing one here turns N
        // changes into N layouts.
        //void reAlign()
        //{
        //    invalidateAlign();
        //    updateAlign(false);
        //}
    public:
        // IForm ->
        IPlatformWindow& wnd_window() override;
        void wnd_beforePaint() override;
        void wnd_paint(void* nativeContext, IntRect& dirtyRect, Graphics::Bitmap*&) override;
        HitTest wnd_hitTest(PointInForm) override;
        void wnd_mouseMove(PointInForm) override;
        void wnd_ncMouseDown(PointInForm, InputStamp) override;
        bool wnd_systemMouseDown() override;
        void wnd_mouseDown(PointInForm, InputStamp, bool& handled) override;
        void wnd_mouseUp(PointInForm) override;
        void wnd_doubleClick(PointInForm, InputStamp) override;
        void wnd_tripleClick(PointInForm, InputStamp) override;
        void wnd_mouseLeave() override;
        void wnd_contextMenu(PointInForm*, InputStamp) override;
        void wnd_mouseWheel(PointInForm, float wheelDelta) override;
        void wnd_mouseHWheel(PointInForm, float wheelDelta) override;
        void mouseWheelOrHwheel(PointInForm, float wheelDelta, bool h);
        void wnd_keyDown(KeyDownEvent&) override;
        void wnd_keyUp() override;
        void wnd_char(wchar_t value) override;
        // Whether a form is this form's popup or one nested on it at any depth.
        [[nodiscard]] bool isNestedPopup(const FormBase*) const;
        void wnd_resize(IntSize surface, const WindowFrame&) override;
        const Graphics::ShadowPainter& wnd_shadowPainter() const override;
        void wnd_posChanged() override;
        void wnd_focusChanged() override;
        void wnd_minimize() override;
        void wnd_maximize() override;
        void wnd_restore() override;
        void wnd_setScalePercent(int) override;
        void wnd_closeRequested() override;
        // <- IForm
    protected:
        void forgetControl(Control*);
        // How much of the monitor's work area this form may take, top to bottom. Nothing reads
        // it: a form is cut to the room its side has, and only when neither side holds it.
        // TODO: this is a share of something only the placement can see. Should it go into
        // WindowPlacement, so a long list scrolls on any screen rather than only where it does
        // not fit?
        virtual float monitorHeightShare() { return 1.0f; }
        void updatePlacement();
        virtual void updateVisibility(); // overriden in TooltipForm for alpha animation
        void clearPopupTarget() { m_popupTarget = nullptr; }
    private:
        // What both of the above delegate to. They differ in nothing but how the owner form was
        // arrived at - through the control the popup stands on, or named outright.
        FormBase(AppContext&, WindowRole, FormControlBase&, FormBase* ownerForm, Control* popupTarget, FormPlacement);
        void initPlacement();
    protected:
        void initialize();
    private:
        // Puts the focus where entering this form should land it - the item the form is on, or
        // the first item when it is on none. Called as the window is shown, for every role that
        // takes the focus at all.
        void focusEntryPoint();
        // The frame this form asks for: the root's, at the scale the form is drawn at. Stated to
        // the window whenever either changes; the window answers through wnd_resize.
        [[nodiscard]] WindowFrame designFrame() const;
        // The shadow the root states, at the scale the form is drawn at.
        [[nodiscard]] Graphics::ShadowPainter::Design shadowDesign() const;
        void stateFrame();
        // Puts the backend the application is on into the canvas - see the definition for why
        // this is called where it is.
        void stateBackend();
        [[nodiscard]] ScaledPosition frameOrigin() const;
        // What the root is laid out into: the window, cut to what the content measured on a form
        // sized by its content - a grant past that is the platform's rounding.
        [[nodiscard]] ScaledDimensions contentExtent() const;
        // Where the root stands in the surface, which is what the shadow is cast around.
        [[nodiscard]] FloatRect rootRect() const;
        // Everything one wnd_paint paints: the dirty rect, and the corner squares it touches
        // painted again under a clip - see the definition.
        void paintWindow(void* nativeContext, const IntRect& dirtyRect, Graphics::Bitmap*&);
        // One paint into a rect of the surface. Where a clip path is given, the root's content is
        // painted under it; the shadow and the root's own ring are not.
        void paintPass(void* nativeContext, const IntRect& rect, const Graphics::PixelPath* contentClip, Graphics::Bitmap*&);
        void paintShadow(const FloatRect& dirtyRect, bool cornerPass);
        void scaleFactorChanged(ScaleFactorChangeEvent&);
        void themeSwitched(ThemeSwitchEvent&);
        void backendSwitched(BackendSwitchEvent&);
        void scaleSwitched(ScaleSwitchEvent&);
        // The control tree, painted in whatever the application is wearing - over the image it is
        // crossing from while it is between two themes. See the definition.
        void paintContent(const FloatRect& dirtyRect, const Graphics::PixelPath* contentClip);
        void reAlign(bool initPlacementMode);
        void updateAlign(bool initPlacementMode);
        // What this form is placed on, in the parent form's coordinates: the rect it was given,
        // or the pointer itself for a menu the pointer raised.
        [[nodiscard]] FloatRect placementAnchor() const;
        // Tells the window the range a drag is held inside, when a measurement has changed it.
        void stateSizeRange();
        // Takes this form off the scaler it is being drawn at, called on it by the form that
        // owns that scaler as that form goes down. What is left is a scaler of this form's own,
        // carrying the factor that was on screen: the picture stands still, and it stops
        // following anything. A form drawn at a scaler further up is not affected, which is what
        // the comparison inside is for.
        void detachScaler(const Scaler& goingDown);
        // Which scaler this form is drawn at from now on - see the definition.
        void stateScaler(Scaler&);
        // Whether this form has taken a scale of its own while still standing on another form -
        // see holdScale.
        [[nodiscard]] bool holdingScale() const
            { return m_popupTargetForm && m_scaler == &m_ownScaler; }
        void followPopupTarget2();
        void rememberPlacementTarget();
        // Takes the active popup down with the control it stood on - see forgetControl.
        void dropActivePopup();
        SearchControlResult controlAt(Control&, PointInControl, FloatRect clipRect);
        SearchControlResult controlAt(PointInForm);
    private:
        FormControlBase& m_content;

        // A FORM WITH A PARENT HAS NO SCALE OF ITS OWN. It stands inside the picture the form
        // under it is drawn in, so it is drawn at that form's scale and takes nothing from the
        // monitor it lands on or from the system - a menu at one scale over a window at another
        // is two sizes of the same design on one screen. m_scaler names whichever scaler this
        // form is drawn at, and a stack of popups all name the one the root carries.
        //
        // m_ownScaler is that scaler for a form with nobody under it, and nothing else reads it:
        // it is also what a child is left holding if the form it stands on goes down first. Every
        // place a scale is stated writes to it by name and is gated on having nobody under it, so
        // none of them can reach a parent's - initPlacement and wnd_setScalePercent for the
        // machine's, initialize and scaleSwitched for the application's.
        Scaler m_ownScaler{ Platform::globalScaleFactor() };
        Scaler* m_scaler;
        // SCOPED, because a child form is connected to another form's scaler: a connection left
        // in that form's dispatcher would be dispatched to this form after it has gone.
        ScopedEventConnection m_scalerConnection;
        WindowRole m_windowRole;
        FormPlacement m_placement;
        // WHAT THE WINDOW GOT, as a size: the geometry, inside the frame's margins.
        PlacedWindow m_placed{};
        // The frame the root states, and the frame the platform applies - see WindowFrame.
        WindowFrame m_frameDesign{};
        WindowFrame m_frame{};
        // The shadow around this window's geometry, baked once per design.
        Graphics::ShadowPainter m_shadowPainter;
        // TWO LINKS BACK, AND THEY GO DOWN AT DIFFERENT TIMES. The form is where this popup
        // de-registers itself, and it must still be reachable then; the target is only who gets
        // the focus back, and it may be deleted while the popup is still up - a list rebuilt
        // under an open in-place editor does exactly that. Reading the form out of the target,
        // as one line, means losing the target loses the de-registration with it, and the owner
        // is left holding a pointer to a popup that has gone.
        FormBase* m_popupTargetForm;
        Control* m_popupTarget;
        // WHAT THIS FORM IS PLACED ON, IN THE PARENT FORM'S COORDINATES - see FormPlacement.
        // Under OverText it is the target's text rect, otherwise the target's bounds. A form with
        // no target has none, and Default is placed without reading one.
        FloatRect m_placementRect;
        // Where the target stood when m_placementRect was measured off it. The DIFFERENCE between
        // this and where the target stands now is what carries the popup along with it - see
        // followPopupTarget. Recorded rather than the rect being derived again, because an opener
        // places on something SMALLER than the target as often as not: the run of text inside a
        // tile, a cell inside a grid. Only a shift keeps a rect like that intact.
        //
        // In the parent form's coordinates, as the rect itself is, so a step is what the target
        // moved INSIDE that form: the form's own window moving leaves it at zero and the rect
        // standing.
        FloatRect m_placementTargetBounds{};
        float m_dropdownClearance{ 4.0f };
        float m_minWidth{ 0.0f };
        // Whether the pass running is measuring to ASK for a window rather than laying out into
        // the one this form has. Read by adjustRootMetrics, and true only inside updateAlign.
        bool m_measuringPlacement{};
        // What the content measured in the last pass, before it was laid out into anything - see
        // initPlacement and contentExtent, which read it.
        ScaledDimensions m_calculatedSize{};
        // Which axes the placement gave less on than the content asked for. Only those are a
        // bound on a form whose window came from its content - see adjustRootMetrics.
        bool m_placedShortX{};
        bool m_placedShortY{};
        // The range last given to the window. Negative until one has been, so the first
        // measurement states it whatever it comes to.
        ScaledDimensions m_statedMinSize{ -1.0f, -1.0f };
        ScaledDimensions m_statedMaxSize{ -1.0f, -1.0f };
        AutoFit m_autoFit{ AutoFit::No };
        bool m_visible{};
        // The form is where the application context stops being carried down. FormContext below it
        // holds only the theme, so that the TextEngine and everything above it stay clear of the
        // config and DOM stack that AppContext brings with it.
        AppContext& m_appContext;
        std::unique_ptr<IPlatformWindow> m_window{
            m_appContext.platform().createWindow(*this, m_windowRole, popupTargetForm())
        };
        std::wstring m_windowTitle{};
        std::wstring m_configName{};
        Graphics::Canvas m_canvas;
        FormContext m_context;
        ScopedEventConnection m_themeSwitchConnection;
        ScopedEventConnection m_backendSwitchConnection;
        ScopedEventConnection m_scaleSwitchConnection;
        // The application has moved to another backend and this form has not taken it yet.
        bool m_backendPending{ false };

        Actions m_actions{};

        FormBase* m_activePopup{ nullptr };
        CloseAction m_closeAction{ CloseAction::Close };
        //
        bool m_aligned{ false };
        Control* m_scrollIntoViewOnAlign{ nullptr };
        LayoutPass m_layoutPass{};
        bool m_placementValid{ false };
        // The stored placement is given to the window once, ahead of the first placement.
        bool m_placementRestored{ false };
        // m_layoutInProgress is true by default, so during the construction stage
        // the controlInvalidate() is suppresed.
        bool m_layoutInProgress{ true };
        //
        PointInForm m_mousePos{ k_maxFloat, k_maxFloat };
        PointInForm m_mouseDownPos{ k_maxFloat, k_maxFloat };
        // What the display server called the press the pointer is still down from. A drag is
        // weighed against the press that began it and never against the move that carries it, so
        // this is taken where the press arrives and read where a request needs it.
        InputStamp m_mouseDownStamp{};
        Control* m_downItem{};
        bool m_isKeyboardClick{};
        // A press this form handed to a popup. THE CHARACTER OF A PRESS IS QUEUED BEFORE THE
        // PRESS IS ANSWERED - TranslateMessage runs ahead of the dispatch - so if that popup goes
        // down on the press, its character still arrives here, at the control the focus has just
        // been handed back to, and marking the press handled cannot stop it. It stands for one
        // press and no other; see wnd_keyDown, which raises it and clears it, and wnd_char, which
        // spends it once the popup has had its chance at the character.
        bool m_popupTookKey{};
        // THE LAST MEMBER, AND IT HAS TO STAY LAST. It is a form of its own that takes this
        // form's window as its owner, so it is built once everything that window is made of is
        // standing - and it goes down before any of that, which is why ~FormBase takes it down at
        // the top of its body rather than leaving it to the teardown that follows.
        Tooltip m_tooltip{ *this };
    };

    // The root control is a base rather than a member, so a form is called like the control it
    // hosts - form.createTopBar<FormTitle>() instead of form.content().createTopBar<FormTitle>().
    // Bases initialize in declaration order, so FormBase is complete - window, canvas, context -
    // before CreateParams is formed from it. Nothing about the window moves into Control, so the
    // same class stays usable as a nested child.
    export template <ClassOfFormControl ControlClass>
        class Form : public FormBase, public ControlClass
    {
    public:
        // Where FormBase and Control declare the same name, the window meaning wins. The root
        // control fills the client area and has no sibling to be hidden against, so its own
        // visibility, invalidation and update carry nothing the window's do not. A control that
        // later declares one of these names would be silently overridden here rather than
        // ambiguous, so the list is exhaustive by intent.
        using FormBase::show;
        using FormBase::hide;
        using FormBase::visible;
        using FormBase::setVisible;
        using FormBase::invalidate;
        using FormBase::update;
        // Both bases declare connectEvent and emitEvent - FormBase's are the forwarders that
        // reach the root control's dispatcher, which is the one the control declares for real.
        // They arrive at the same place, so this only says which of the two spellings to take,
        // and the direct one is cheaper to read.
        using ControlClass::connectEvent;
        using ControlClass::emitEvent;
        // Reached through either base, same value both ways.
        using FormBase::appContext;
        using FormBase::theme;
        using FormBase::bakedColors;
        using FormBase::themeMetrics;
        using FormBase::scaler;
    public:
        // A form takes the focus unless it says otherwise, so the role is optional and a dialog
        // is what leaving it out means.
        template <typename... Args>
        Form(AppContext& appContext, Control* ownerButton, Args&&... args)
            :
            Form{ appContext, WindowRole::Dialog, ownerButton, std::forward<Args>(args)... }
        {
        }

        template <typename... Args>
        Form(AppContext& appContext, WindowRole windowRole, Control* ownerButton, Args&&... args)
            :
            // Where the window is put when it is first shown.
            FormBase{ appContext, windowRole, content(), ownerButton, READ_PROPERTY(FormPlacement, FormPlacement::Default) },
            ControlClass{ CreateParams{ static_cast<FormBase&>(*this) }, std::forward<Args>(args)... }
        {
            initialize();
        }

        template <typename... Args>
        Form(AppContext& appContext, WindowRole windowRole, FormBase& ownerForm, Args&&... args)
            :
            // Where the window is put when it is first shown.
            FormBase{ appContext, windowRole, content(), ownerForm, READ_PROPERTY(FormPlacement, FormPlacement::Default) },
            ControlClass{ CreateParams{ static_cast<FormBase&>(*this) }, std::forward<Args>(args)... }
        {
            initialize();
        }
    public:
        [[nodiscard]] ControlClass& content() { return static_cast<ControlClass&>(*this); }
    protected:
        void nestedControlDeleted(Control* value) override
        {
            ControlClass::nestedControlDeleted(value);
            forgetControl(value);
        }
    };

    //-------------------------------------------------------------------------

    // FormControlBase

    template<typename ...Args>
    FormControlBase::FormControlBase(const CreateParams& params, Args && ...args)
        :
        RichControl{ params, std::forward<Args>(args)... },
        INIT_PROPERTY(windowShadow),
        m_form{ params.form }
    {
    }

    template<ClassOfFormControl ControlClass, typename... Args>
    Form<ControlClass> FormBase::createPopup(Control* ownerButton, Args&&... args)
    {
        // A popup takes the pointer without taking the activation from the form that opened it,
        // which is what WindowRole::Menu is. Named here rather than left to the caller, since
        // being a popup is what this function is for - and the pack goes to the root untouched,
        // whatever arguments that root happens to take.
        return Form<ControlClass>{
            appContext(),
            WindowRole::Menu,
            ownerButton,
            std::forward<Args>(args)...
        };
    }

}
