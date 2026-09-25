module;
#include "../System/EventBindings.h"

export module ClaFi.Core.Foundation :Control;

import :PaintEvent;

import ClaFi.Core.Context.PaintIconEvent;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.Utils;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine;

import ClaFi.StdLib;
import ClaFi.Core.Context.AppContext;

namespace ClaFi
{
    export class FormBase;
    export class Control;
    export class AlignEvent;
    export class TraversalContext;
    export class Action;
    // Defined in :Action, which imports this partition. A control carries the connectors for
    // them; a connector's body is instantiated at the call site, where the definitions are.
    export class GetActionStateEvent;
    export class ActionClickEvent;
    export class GetShortcutEvent;

    // The commands a menu shows, in order. A null entry is a separator.
    export using ActionList = std::vector<Action*>;

    // Takes an action given to a control as a construction property. Declared here and defined
    // in the implementation unit, so this interface needs no more than the name.
    export void attachAction(Action&, Control&, PresenterRole = PresenterRole::Button);

    export using ControlPtr = std::unique_ptr<Control>;
    export using ControlSpan = std::span<ControlPtr>;
    export using ControlSpanC = std::span<const ControlPtr>;
    export using ControlCollection = std::vector<ControlPtr>;
    // A container's children in an order of its own keeping, which cannot be the control
    // collection: these do not own, and an entry may be null. See Control::navigationSlots.
    export using ControlSlots = std::span<Control* const>;

    // How a container orders its children, for narrowing a viewport range. See Control-Foundation
    export struct TraversalOrder
    {
        bool x{};
        bool y{};
        // Distance from a child's leading edge to the far edge of the lane holding it.
        // Zero means the container has a single lane, so a child's own trailing edge
        // is the exact cull key. A wrapping container reports its widest lane, which
        // bounds every child's trailing edge from above: a child's own trailing edge
        // is not usable there, because items sharing a lane are aligned individually
        // within it and so end at different offsets.
        float laneExtent{};
    };

    export template<typename T>
        concept IsControl = std::derived_from<T, Control>;

    export class AdjustMetricsEvent : public ControlEventBaseC
    {
    public:
        AdjustMetricsEvent(const FormContext&, const Control&, ControlMetrics&);
        const Control& control;
        ControlMetrics& metrics;
    };

    export struct AdjustViewportEvent : public ControlEventBaseC
    {
    public:
        AdjustViewportEvent(const FormContext&, const Control& control, FloatRect& viewport);
        const Control& control;
        FloatRect& viewport;
        bool clipIntoParent{ true };
    private:
        using ControlEventBaseC::stopPropagation;
        using ControlEventBaseC::propagationStopped;
    };

    // One layout pass, and the channel a control asks for another. See Control-Foundation
    export class LayoutPass
    {
        friend FormBase;
        friend AlignEvent;
    public:
        // Whether everything laid out so far stands at a size this pass measured it against.
        [[nodiscard]] bool valid() const { return m_valid; }
    private:
        void invalidate() { m_valid = false; }
        void revalidate() { m_valid = true; }
    private:
        bool m_valid{ true };
    };

    export class AlignEvent : public ControlEventBase
    {
    public:
        AlignEvent(FormContext&, LayoutPass&, const ControlMetrics&);
        AlignEvent(AlignEvent&, const ControlMetrics&);
    public:
        void calculateScaledMetrics(const ControlMetrics&);
    public:
        // Says this pass laid a control out at a size it never measured. See Control-Foundation
        void invalidatePass() { m_pass.invalidate(); }
        // Whether nothing has said that yet.
        [[nodiscard]] bool passValid() const { return m_pass.valid(); }
        // The pass this event belongs to, for an event built out of this one.
        [[nodiscard]] LayoutPass& pass() { return m_pass; }
    public:
        float minContentWidth() const;
        float minContentHeight() const;
        float maxContentWidth() const;
        float maxContentHeight() const;
        float totalSpacingX(std::size_t itemsNum) const;
        float totalSpacingY(std::size_t itemsNum) const;
    public:
        ScaledPadding padding;
        ScaledSpacing spacing;
        ScaledDimensions minSize;
        // Zero on an axis means the content decides it - see ControlMetrics::preferredSize.
        ScaledDimensions preferredSize;
        ScaledDimensions maxSize;
        // WHAT THE CONTENT OF THIS CONTROL NEEDS, written by whoever composes that content out
        // of what its children answered, by the same rule their sizes were composed by. Zero says
        // nothing inside states a floor, and the control is then held up by its own MinSize
        // alone. WHAT THE CONTENT MEASURED IS NO PART OF THIS - see Control::calculatedMinSize.
        ScaledDimensions calculatedMinSize{ 0.0f, 0.0f };
    private:
        // Borrowed - the form owns the pass and outlives every event raised inside it.
        LayoutPass& m_pass;
    };

    // The pointer has entered the control.
    export class HoverEnterEvent : public Event
    {
    public:
        HoverEnterEvent(Control&);
        Control& control;
    };

    // The pointer has left the control.
    export class HoverLeaveEvent : public Event
    {
    public:
        HoverLeaveEvent(Control&);
        const Control& control;
    };

    export class ClickEventBase : public Event
    {
    public:
        // The second names the input event this click answers. The first is the shorthand for
        // the press the form is acting on, which is what a pointer gesture reaches here as; a
        // click made by a key is not that, and states its own.
        ClickEventBase(Control&, FormBase&);
        ClickEventBase(Control&, FormBase&, InputStamp);
        void closeForm();
        float scaleFactor() const;
        PointInForm clickPos() const;
        const AppContext& appContext() const;
    public:
        Control* control;
        FormBase& form;
        KeyModifiers modifiers;
        // WHAT THE USER DID THAT LED HERE. A command run from a button or a menu line is
        // authorised by this, exactly as one run from a key is authorised by the key's.
        InputStamp stamp;
    };

    // The control was clicked.
    export class ClickEvent : public ClickEventBase
    {
    public:
        using ClickEventBase::ClickEventBase;
    };

    // The second press of one run of clicks.
    export class DoubleClickEvent : public ClickEventBase
    {
    public:
        using ClickEventBase::ClickEventBase;
    };

    // The third press of one run of clicks. See Control-Foundation
    export class TripleClickEvent : public ClickEventBase
    {
    public:
        using ClickEventBase::ClickEventBase;
    };

    // Asked what state the control is in, so that whatever draws or reads it agrees.
    export class GetStateEvent : public Event
    {
    public:
        GetStateEvent(const Control&);
        const FormBase& form() const;
        const Control& control;
        ActionState state{};
    };

    // The control was asked for a context menu.
    export class ContextPopupEvent : public ClickEventBase
    {
    public:
        ContextPopupEvent(Control&, FormBase&, FloatPoint* = nullptr);
        FloatPoint* mousePos;
    };

    // A control is about to show its own built-in edit menu. See Control-Foundation
    export class EditContextPopupEvent : public ContextPopupEvent
    {
    public:
        EditContextPopupEvent(Control&, FormBase&, ActionList&, FloatPoint* = nullptr);
        // The commands the menu is about to show. Add to it, drop from it or reorder it - what
        // is left is what opens. A separator that would lead, trail or double up is dropped as
        // the menu is built, so an edit here does not have to tidy them.
        ActionList& actions;
    };

    // The control is being destroyed. See Control-Foundation
    export class DestroyEvent : public Event
    {
    public:
        DestroyEvent(const Control&);
        const Control& control;
    };

    class ControlEventBaseCC : public ControlEventBaseC
    {
    public:
        ControlEventBaseCC(const FormContext&, const Control&);
        const Control& control() { return m_control; }
    private:
        const Control& m_control;
    };

    export class CharPressEvent : public ControlEventBaseCC
    {
    public:
        CharPressEvent(const FormContext&, const Control&, wchar_t);
        const wchar_t& character() const { return m_character; }
    private:
        const wchar_t m_character;
    };


    class GetTextEventBase : public ControlEventBaseCC
    {
    public:
        // The buffer a gather builds its answer in. Everything that reads or writes it comes
        // through here, which is what lets a text be NAMED rather than copied: a name is put into
        // the buffer by the first access of any kind, so nothing can see a buffer missing it.
        class Buffer
        {
        public:
            explicit Buffer(GetTextEventBase& owner) : m_owner{ owner } {}
            // Both of these REPLACE what the buffer holds, so a named text is dropped rather than
            // put into it: what was named is no part of the answer any more.
            Buffer& operator=(const Text& text)
            {
                m_owner.discardName();
                m_owner.m_buffer = text;
                return *this;
            }
        public:
            template<typename Arg>
            Buffer& operator<<(Arg&& arg)
            {
                built() << std::forward<Arg>(arg);
                return *this;
            }
            void clear()
            {
                m_owner.discardName();
                m_owner.m_buffer.clear();
            }
            operator Text&() { return built(); }
            [[nodiscard]] bool empty() { return built().empty(); }
            [[nodiscard]] const std::wstring& plainText() { return built().plainText(); }
        private:
            Text& built() const;
        private:
            GetTextEventBase& m_owner;
        };
    public:
        GetTextEventBase(const FormContext&, const Control&, Text& text, EventPhase);
        GetTextEventBase(GetTextEventBase& other, bool dummy);
        Buffer text{ *this };
        EventPhase phase() const { return m_phase; }
        // A control whose own text is the whole of its answer NAMES it here instead of writing it
        // into the buffer. A name is a pointer where a copy is the plain text and every marker in
        // it, and a measurement gathers once for every question it asks.
        //
        // The first contribution to arrive with nothing written yet is the one that can be named.
        // Anything after it - a second contribution, or a handler streaming into `text` - puts the
        // named text into the buffer first, so the answer comes out as it would have anyway.
        void contribute(const Text&);
        // The answer: the named text, or the buffer when one was built. Asked for once, by
        // whoever owns the buffer. A reader that keeps the address and Text::revision of what this
        // returns can tell an unchanged answer from a new one without reading either - a built
        // answer never matches, because the buffer is cleared per gather and that moves it on.
        [[nodiscard]] const Text& result();
        // The buffer holds the whole answer after this, named or not. Every route through Buffer
        // calls it; a caller holding the buffer itself asks for result() instead.
        void materialise();
    private:
        // The name forgotten rather than honoured, for a route that replaces the buffer outright.
        void discardName() { m_source = nullptr; }
    private:
        Text& m_buffer;
        EventPhase m_phase;
        // Where a name is kept, and the slot it is kept in. A child event names into the event it
        // was built from: Control::getChildText asks the child through one of its own, and the
        // handlers that follow write into the buffer both of them share, so the name has to
        // outlive the child event.
        const Text* m_ownSource{ nullptr };
        const Text*& m_source;
    };

    // Asked for the control's own text.
    export class GetTextEvent : public GetTextEventBase {
    public:
        using GetTextEventBase::GetTextEventBase;
    };

    // Asked for the text of a child, by a control that shows what its child says.
    export class GetChildTextEvent : public GetTextEventBase {
    public:
        using GetTextEventBase::GetTextEventBase;
    };

    // TODO: derive GetTooltipEvent from GetTextEventBase, as GetTextEvent and GetChildTextEvent are.
    // Asked for the tooltip the control shows.
    export class GetTooltipEvent : public ControlEventBaseC
    {
    public:
        GetTooltipEvent(const FormContext&, const Control&, Text&, EventPhase);
    public:
        const Control& control;
        Text& text;
        // if the placement is overText, set the anchorRect to the item's textRect,
        // otherwise it should hold the item's bounds (initialized in the constructor).
        // anchorRect is in the coordinates of the form the control stands in
        FloatRect anchorRect{};
        TooltipPlacement placement{ TooltipPlacement::Mouse };
        // Whether the words this hint repeats were broken to the box they were drawn in. Read
        // under OverText alone, where the hint is that same layout uncut - see
        // TooltipForm::setControl.
        bool wordWrap{ false };
        const EventPhase phase;
        bool hideOnUserInput{ true };
    };

    export struct HitTestEvent
    {
        const FormContext& formContext;
        const PointInControl point;
        HitTest zone{ HitTest::Client };
    };

    // Where a control's children begin, as an inset from its top left. See Control-Foundation
    export struct AdjustChildInsetEvent : ControlEventBaseC
    {
        AdjustChildInsetEvent(const FormContext&, const Control& control, ScaledPadding contentPadding);
        const Control& control;
        // The content padding this started from, kept so an override can measure against it.
        const ScaledPadding contentPadding;
        ScaledPadding inset;
    };

    export struct AdjustTextRectEvent : ControlEventBaseC
    {
        AdjustTextRectEvent(const Control& control, PaintEvent&);
        AdjustTextRectEvent(const FormContext&, const Control& control, const FloatRect&);
        const Control& control;
        const FloatPoint padding;
        const FloatPoint spacing;
        FloatRect textBounds;
    private:
        AdjustTextRectEvent(const FormContext&, const Control& control, const ControlMetrics&, const FloatRect&);
    };

    export class MouseInputEvent: public Event
    {
    public:
        MouseInputEvent(const FormBase& form, Control& control, PointInControl posOnControl, PointInForm posOnForm);
        const FormBase& form;
        Control& control;
        const PointInControl posOnControl{};
        const PointInForm posOnForm{};
    };
    // The pointer moved over the control.
    export class MouseMoveEvent : public MouseInputEvent
    {
    public:
        using MouseInputEvent::MouseInputEvent;
    };

    export struct MouseWheelEvent
    {
        const FormBase& form;
        const Control& control;
        const float delta;
        bool handled{};
    };

    // A button went down on the control, before any click is settled.
    export class PressDownEvent : public ClickEventBase
    {
    public:
        using ClickEventBase::ClickEventBase;
    public:
        void updateDownControl() { m_downControlDirty = true; }
        bool downControlDirty() { return m_downControlDirty; }
    private:
        bool m_downControlDirty{};
    };

    export struct FocusEvent : public ClickEventBase
    {
        using ClickEventBase::ClickEventBase;
    };

    // TODO: should these three flags live on the event, as every other input result does?
    export struct PressUpHandled {
        bool propagationStopped{};
        bool preventClick{};
        bool downControlDirty{};
    };

    export struct PressUpEvent
    {
        const FormBase& form;
        Control& control;
        PressUpHandled& handled;
        bool& scrollIntoView;
        KeyModifiers modifiers;
    };

    export class DragEvent : public Event
    {
    public:
        DragEvent(Control& control, FormBase& form, PointInForm startPos, PointInForm currentPos,
            InputStamp stamp);
        Control& control() { return m_control; }
        FormBase& form() { return m_form; }
        FloatRect selectionRect() const;
        const PointInForm startPos() const { return m_startPos; }
        const PointInForm currentPos() const { return m_currentPos; }
        // What the display server called the PRESS this drag started from, which is what a
        // request made on the strength of the gesture is weighed against.
        [[nodiscard]] InputStamp stamp() const { return m_stamp; }
        float scaledDistance() const;
        float unscaledDistance() const;
        void offsetStartPos(FloatPoint) const;
        void lockHoveredControl() { m_hoveredControlLocked = true; }
        bool hoveredControlLocked() const {return m_hoveredControlLocked; }
    private:
        Control& m_control;
        FormBase& m_form;
        const PointInForm m_startPos;
        const PointInForm m_currentPos;
        const InputStamp m_stamp;
        bool m_hoveredControlLocked{};
    };

    // Whether a change animates or takes effect at once.
    export enum class AnimationMode {
        On,
        Off
    };

    // Whether a walk over a control's subtree counts the control itself.
    export enum class CheckSelf
    {
        Yes,
        No
    };

    // Why a run of states is invalidated, and what each control hears. See Control-Foundation
    export enum class InvalidateEvent {
        None,
        HoverEnter,
        HoverLeave,
        PressUp,
        PressDown,
        InputDevice
    };

    export struct CreateParams
    {
        explicit CreateParams(Control& parent);
        explicit CreateParams(FormBase& form);
        Control* parent;
        FormBase& form;
        const AppTheme& theme() const;
        const ThemeMetrics& themeMetrics() const;
        const BakedColors& bakedColors() const;
        const AppContext& appContext() const;
    };

    // Core control functionality, with a minimal memory footprint. See Control-Foundation
    export class Control : public EventComponent
    {
        friend FormBase;
        friend class Input;
        friend TraversalContext;
        friend PaintEvent;
        friend class Tooltip;
        friend class TooltipLabel;
        friend class FocusNavigator;
    public:
        template <typename... Args>
        explicit Control(const CreateParams& params, Args&&... args)
            :
            EventComponent{ args... },
            INIT_PROPERTY(tag),
            m_parent{ params.parent }
        {
            // Where the control sits across the space its parent gives it.
            BIND_PROPERTY_CALL(HorizontalAlign, storeHorizontalAlign);
            // Where the control sits down the space its parent gives it.
            BIND_PROPERTY_CALL(VerticalAlign, storeVerticalAlign);
            // Where the control's text sits down its own box.
            BIND_PROPERTY_CALL(VerticalTextAnchor, setVerticalTextAnchor);
            // Where the control's text sits across its own box.
            BIND_PROPERTY_CALL(HorizontalTextAnchor, setHorizontalTextAnchor);
            // Whether the control's text wraps rather than being trimmed.
            BIND_PROPERTY_CALL(WordWrap, setWordWrap);
            // What an Action given as a property takes this control for.
            const PresenterRole presenterRole = READ_PROPERTY(PresenterRole, PresenterRole::Button);
            // An action this control presents, taking it as a presenter. See Control-Foundation
            BIND_PROPERTY_ACTION(Action, attachAction(p, *this, presenterRole));
        }
        virtual ~Control();
        Control(const Control&) = delete;
        Control(Control&&) = delete;
        Control& operator=(const Control&) = delete;
        Control& operator=(Control&&) = delete;
    public:
        // Whatever the caller hangs on this control, and nothing the framework reads.
        DECLARE_PROPERTY_STORAGE(Tag, tag, Tag{})
    public:
        DECLARE_EVENT(ClickEvent, OnClick, onClick)   // a click on this control
        // The second press of a run of clicks.
        DECLARE_EVENT(DoubleClickEvent, OnDoubleClick, onDoubleClick)
        // The third press of a run of clicks.
        DECLARE_EVENT(TripleClickEvent, OnTripleClick, onTripleClick)
        // A button going down on this control.
        DECLARE_EVENT(PressDownEvent, OnPressDown, onPressDown)
        // The pointer entering this control.
        DECLARE_EVENT(HoverEnterEvent, OnHoverEnter, onHoverEnter)
        // The pointer leaving this control.
        DECLARE_EVENT(HoverLeaveEvent, OnHoverLeave, onHoverLeave)
        // The pointer moving over this control.
        DECLARE_EVENT(MouseMoveEvent, OnMouseMove, onMouseMove)
        // The control being asked for a context menu.
        DECLARE_EVENT(ContextPopupEvent, OnContextPopup, onContextPopup)
        // The control about to show its own edit menu.
        DECLARE_EVENT(EditContextPopupEvent, OnEditContextPopup, onEditContextPopup)
        // The control being asked what state it is in.
        DECLARE_EVENT(GetStateEvent, OnGetState, onGetState)
        // The control being asked for its own text.
        DECLARE_EVENT(GetTextEvent, OnGetText, onGetText)
        // The control being asked for a child's text.
        DECLARE_EVENT(GetChildTextEvent, OnGetChildText, onGetChildText)
        // The control being asked for its tooltip.
        DECLARE_EVENT(GetTooltipEvent, OnGetTooltip, onGetTooltip)
        DECLARE_EVENT(PaintEvent, OnPaint, onPaint)   // the control painting itself
        // The paint being set up, before anything is drawn.
        DECLARE_EVENT(AdjustPaintEvent, OnAdjustPaint, onAdjustPaint)
        // Painting an icon is not a button's privilege - see paintIcon, which emits it.
        DECLARE_EVENT(PaintIconEvent, OnPaintIcon, onPaintIcon)
        DECLARE_EVENT(DestroyEvent, OnDestroy, onDestroy)   // the control being destroyed
        // An action asking whether this control is its subject. See Control-Foundation
        DECLARE_EVENT(GetActionStateEvent, OnGetActionState, onGetActionState)
        // An action running on this control as its subject.
        DECLARE_EVENT(ActionClickEvent, OnActionClick, onActionClick)
        // What key runs this control, for a control that shows the key beside the command.
        DECLARE_EVENT(GetShortcutEvent, OnGetShortcut, onGetShortcut)
    public:
        void deleteSelf();
        // Container interface
        virtual void deleteControl(Control&);
        // Takes `control` into this control's overlay list, and answers whether there was a list
        // to take it into. Only a container holds one; everything else answers no, which is how a
        // control looking for a host learns to keep looking or to host the entry itself.
        //
        // The control named may sit at any depth below this one - see ContainerBase for what an
        // entry buys, and Control::floatOffset for what it deliberately does not.
        virtual bool addOverlayControl(const Control&, ClippingMode) { return false; }
        virtual void removeOverlayControl(const Control&) {}
    public:
        Control* parent() const { return m_parent; }
        template <IsControl ControlClass>
        const ControlClass& parentAs() const { return *static_cast<ControlClass*>(m_parent); }
        template <IsControl ControlClass>
        ControlClass& parentAs() { return *static_cast<ControlClass*>(m_parent); }
        bool isFirstInParent() const { return &*m_parent->controls().front() == this; }
        // This control fills its parent's body slot, so its size is the parent's to give. A
        // control that is not a body owns its own, and may grow past what it was offered.
        [[nodiscard]] bool isHostedAsBody() const { return m_parent && m_parent->isChildBody(*this); }
        // WHOSE ANSWER THIS CONTROL'S WIDTH IS: what holds it, or what is in it. A control laid
        // across its parent's lane or into a slot is handed a width and has to live in it; one
        // whose parent measures it and then gives it back exactly that - a panel's left bar, a
        // control with no parent at all - is as wide as what it contains, and nothing outside it
        // has an opinion.
        //
        // The difference matters wherever a control would state its own width as a bound on what
        // is inside it: on a control that is measured FROM its content, such a bound is derived
        // from the thing it bounds, and anything that narrows the content once narrows it for
        // good. See ScrollBox::adjustChildMetrics, which is what asked for this.
        [[nodiscard]] bool isWidthGivenByParent() const
            { return m_parent && m_parent->isChildWidthGiven(*this); }
        // Whether the host scrolls this control along the axis, so its extent there is its own.
        [[nodiscard]] bool isScrolledByParent(ScrollAxis axis) const
            { return m_parent && m_parent->scrollsChild(*this, axis); }
        // THE SAME QUESTION, ASKED ALL THE WAY UP, and the one a bound carried over from the last
        // pass has to answer. One level is not enough: a lane handed down through five stacks is
        // still the content's own answer where the form at the top is measured from what it
        // holds, and a bound stated under one of those narrows the very thing it came from. The
        // walk ends at the first control that is not handed its width - that one's content
        // decides it - and at the root, where the window does, unless the window was asked for
        // out of the content too. See StackPanel::wrapWidthLimit
        [[nodiscard]] bool isWidthGivenFromOutside() const;
        virtual std::wstring_view diagnosticText() const { return {}; }
        FormBase& form();
        const FormBase& form() const;
        FormContext& formContext();
        const FormContext& formContext() const;
        AppContext& appContext();
        const AppContext& appContext() const;
        // The application's animation controller, or null while this control stands in no
        // form - there is no application to reach then. See Control-Foundation
        [[nodiscard]] AnimationController* animator();
        // Runs this control's animation on the slot, from the current value to the end value,
        // through the application's controller. A control standing in no form has nothing to
        // run it on and takes the end value outright: nobody sees it, and the value is what it
        // is drawn from once it is seen. See Control-Foundation
        void animate(const AnimationSlot&, float currentValue, float endValue, const OnAnimate&);
        void stopAnimation(const AnimationSlot&);
        void stopAnimations();
        const AppTheme& theme() const;
        const ThemeMetrics& themeMetrics() const;
        const BakedColors& bakedColors() const;
        const Scaler& scaler() const;
        //
        ActionState state() const;
        const StateFactors& factors() const { return m_factors; }
        void setStateFactor(VisualStateIndex, float value, bool triggerInvalidate = true);
        float hoveredFactor() const { return m_factors.hovered(); }
        float selectedFactor() const { return m_factors.selected(); }
        float pressedFactor() const { return m_factors.pressed(); }
        float enabledFactor() const { return m_factors.enabled(); }
        float focusedFactor() const { return m_factors.focused(); }
        float currentFactor() const { return m_factors.current(); }
        //
        void invalidateState(AnimationMode = AnimationMode::On);
        void invalidateChildrenStates();
        void invalidateSiblingStates() const;
        void update();
        void invalidate() const;
        void invalidateFormAlign();

        virtual Interactivity interactivity() const { return Interactivity::None; }
        // WHETHER THIS CONTROL TAKES WHAT ITS LANE HAS OVER. A stack places its items at the
        // sizes they measured and the room left sits at the lane's end; a control answering true
        // is handed that room instead, shared evenly with any other item in the lane that asked.
        // It is what puts the items AFTER it at the end of the lane - see FlexSpacer. A wrapping
        // stack answers nothing here: a lane it broke to fit has no room left by definition.
        [[nodiscard]] virtual bool fillsLane() const { return false; }
        // Whether the pointer ADDRESSES this control - MouseOnly or Focusable, the two answers that
        // mean a press here is a press on something. ActiveContainer is not one of them: a
        // container is what a press lands inside rather than what it was aimed at, and None is
        // decoration. Everything a press spends on its way out asks this - the state invalidation
        // that walks up from the pressed control, and the scroll a release asks for - so a click on
        // a label neither lights the host it stands in nor moves the view under it.
        [[nodiscard]] bool respondsToPointer() const;
        // The pointer shape over this control. It stands for what a press here would do, so a
        // control that takes text answers IBeam and everything else keeps the arrow. Asked of the
        // control the pointer is actually over - the deepest one hit - so a child answers for
        // itself rather than inheriting whatever its host would say.
        [[nodiscard]] virtual CursorShape cursor() const { return CursorShape::Arrow; }
        // Whether the focus may rest here. Not the same question as whether this control is
        // interactive: MouseOnly means hovered and clicked but never focused, which is exactly the
        // distinction a test for "not None" throws away. Everything that moves the focus asks this
        // - the focus navigator, and the walk that runs on mouse down - so that a scroll bar or a
        // scroll button is clicked without the focus leaving whatever the pointer came from.
        //
        // Being enabled is part of the answer rather than a check each caller adds afterwards. A
        // caller that searches for the nearest focus target has to skip a disabled one and keep
        // looking; testing it separately stops the search at the disabled control instead, and
        // leaves the focus nowhere.
        //
        // Deep, because disabling a container disables what it holds. Nothing inside a disabled
        // control is interactive - the one thing that still works there is the tooltip, which is
        // how a disabled control gets to say why it is disabled, and tooltips follow hover rather
        // than focus. See also canClick, which answers the same way for the same reason.
        [[nodiscard]] bool canTakeFocus() const;
        // Whether this control moves in Z - towards the viewer as the pointer arrives, away from it
        // while held. It is one answer with two consequences, and they must not be given separately:
        // whether the press scale is applied at all, and whether the control's text is rasterized in
        // a way that survives being moved. A control that moves while its text is placed as though
        // it were still is the stutter; a control that does not move but pays for movable text is
        // soft for nothing.
        //
        // Off by default. Depth is a statement that a control is a thing to push, so it is opted
        // into by the controls that are - a button, and the indicator inside one - rather than
        // being inherited by everything with a pointer over it.
        [[nodiscard]] virtual bool allowZAnimation() const { return false; }
        // How much of the parent's state that depth answers to is not asked here - it is the host's
        // to say, through AdjustPaintEvent::setParentZAmount, because the same child class serves
        // hosts that answer differently. An indicator in a check box stands for its host entirely;
        // the same indicator on a list item is clickable in its own right and keeps a share of the
        // movement for its own hover.

        // Asks for the focus. Every ancestor gets to redirect it on the way up - see
        // adjustFocus - so the control that ends up focused may not be this one.
        void setFocus();
        //
        // Runs the click a key asked for, which names that key rather than whatever the pointer
        // last pressed on this form.
        void animatedClick(FormBase&, InputStamp);
        //
        [[nodiscard]] bool visible() const { return !(cfHidden & m_flags); }
        void setVisible(const bool value);
        void toggleVisible() { setVisible(!visible()); }
        void show() { setVisible(true); }
        void hide() { setVisible(false); }
        //
        // WHAT IS HELD IS NOT SCROLLED TO. A control carried by its parent's float offset stands
        // where the viewport is rather than where it was laid out, and that offset is measured off
        // the scroll: a scroll made to reveal it hands the whole distance back to the hold, and
        // what moves is the content behind it. Both requests below answer nothing for a held
        // control, and nothing for anything inside one - a child is held with its host.
        //
        // Brings this control's scrollHotspot() into view.
        void scrollIntoView();
        // Brings one rectangle of this control into view rather than the whole of it. A control
        // larger than the viewport is fully visible the moment any part of it is, so a control
        // that paints its own addressable parts - grid cells, text lines - names the part it
        // wants seen, in its own coordinates.
        void scrollIntoView(const FloatRect& rectInControl);
        // The same, once the alignment this control is waiting for has run. A control that has
        // just been shown, or one whose host has just grown, still measures as it did before, so
        // asking now would scroll to a rect that no longer applies.
        void scrollIntoViewOnAlign();
        //
        // data driven states
        //
        // Virtual for the one control that is not disabled by what disables its parent: a
        // dropdown strip is a second command beside the button it sits on, and the button having
        // nothing to do says nothing about it. Every other control takes this as it stands.
        [[nodiscard]] virtual bool enabled(const bool deep = false) const;
        [[nodiscard]] bool selected() const { return state().selected; }
        // framework driven states
        [[nodiscard]] bool isHovered() const;
        // Whether this control has reached its first paint. Anything that moves from what is on
        // screen has nothing to move from until it has.
        [[nodiscard]] bool isPainted() const { return m_flags2 & cfPainted; }
        [[nodiscard]] bool isTextDrawn() const { return m_flags2 & cfTextDrawn; }
        [[nodiscard]] bool isTextHovered() const;
        [[nodiscard]] bool isPressed() const;
        [[nodiscard]] bool isFocused() const;
        [[nodiscard]] bool isDroppedDown() const;
        //
        [[nodiscard]] float left() const { return m_topLeft.x; }
        [[nodiscard]] float top()   const { return m_topLeft.y; }
        [[nodiscard]] FloatPoint topLeft() const { return m_topLeft; }
        [[nodiscard]] float width() const { return m_dimensions.x; }
        [[nodiscard]] float height() const { return m_dimensions.y; }
        [[nodiscard]] ScaledDimensions dimensions() const { return m_dimensions; }
        // THE LEAST THIS CONTROL MAY BE MADE. Answered by the measuring pass beside the size
        // itself, out of the MinSize this control was given and the floor its content composed
        // from its children's. Never larger than the size it came to.
        //
        // A MEASUREMENT NEVER RAISES IT. A control is as wide as it is because of what it holds
        // today, and cutting it back is what leaves a strip of tabs or a run of columns the room
        // to fit itself into a narrower box. So the floor is a MinSize somebody set, here or
        // below: a control that must not lose what it shows states one and is held up by it, and
        // a tree nobody set one in can be cut to nothing.
        //
        // What reads it is whoever divides space and has less of it than was asked for - today
        // the placement, which cuts a window to the room its side has and no further than this.
        [[nodiscard]] ScaledDimensions calculatedMinSize() const { return m_minDimensions; }

        [[nodiscard]] float right() const { return m_topLeft.x + m_dimensions.x; }
        [[nodiscard]] float bottom() const { return m_topLeft.y + m_dimensions.y; }
        [[nodiscard]] FloatPoint bottomRight() const { return m_topLeft + m_dimensions; }
        //
        [[nodiscard]] FloatRect boundsInParent() const { return { m_topLeft.x, m_topLeft.y, right(), bottom() }; }
        // How far this control is drawn from where it was laid out. Zero unless its parent
        // answers overlayChildOffset with something - a grid whose header stays at the top of the
        // viewport does.
        //
        // Asked of the PARENT, and of nothing else. Where a control is drawn is a question about
        // the space its topLeft is measured in, which is its parent's content space and no one
        // else's. Which container holds the overlay entry naming it is a separate question -
        // that one says when the control is painted and what clips it, and its answer may be an
        // ancestor several levels up.
        //
        // Everything that says where the control IS carries it: form space, through
        // FormBase::boundsOriginOfControl; the bounds the paint traversal builds; and the hit
        // test. Everything that says where the control BELONGS does not: topLeft() and
        // boundsInParent(), what the layout writes and what the content extent is read off, and
        // the leading edges firstChildInViewport and isViewportEnd compare. Those last two are
        // the reason the offset is not simply written into topLeft: they range over children
        // sorted by that edge, and a child moved out of that order leaves the range partitioned
        // by nothing, so the binary search over it answers arbitrarily rather than merely
        // conservatively.
        [[nodiscard]] FloatPoint floatOffset() const;
        // Whether this control is drawn away from where it was laid out, which is what a container
        // holding a control against the viewport does to it - a grid's header, for as long as the
        // rows it heads are scrolled past it. Such a control is on screen for as long as the hold
        // lasts, and the place it is held away from says nothing about where it is.
        [[nodiscard]] bool isHeldInView() const { return floatOffset() != FloatPoint{}; }
        // Carries the view this control is scrolled in by `delta`, applied at once rather than as
        // a glide: what stood on one line of the view now stands `delta` further along the
        // content. Nothing when nothing scrolls this control. See ExpanderHeader::setExpanded,
        // which keeps a held header on the line it was collapsed on.
        void scrollViewBy(FloatPoint delta);
        // The point this control's own topLeft is measured from, in form space: the parent's
        // content origin, or the form origin when there is no parent.
        [[nodiscard]] FloatPoint parentContentOrigin() const;
        // WHICH PART OF THIS CONTROL A MENU IS ABOUT, in form coordinates. A menu raised by the
        // keyboard has no pointer to drop at, and the whole control is the wrong answer for
        // anything that has a place inside itself: a text box's menu belongs at the caret, which
        // is where every command in it acts. The default is the whole control, which is what a
        // menu under a button wants.
        //
        // The same distinction scrollHotspot draws, and asked the same way - of the control, at
        // the moment the menu opens.
        [[nodiscard]] virtual FloatRect contextMenuAnchor() const { return boundsInForm(); }
        [[nodiscard]] FloatRect boundsInForm() const;
        [[nodiscard]] FloatRect boundsInForm(const FormBase&) const;
        [[nodiscard]] virtual FloatRect viewPort(FormBase&) const;
        // The part of this control that lies inside every viewport it sits in, so a control
        // larger than the viewport scrolling it is measured by what is on screen. A clip that
        // would leave nothing is skipped: a control scrolled out of view keeps the innermost
        // rect that still had an area, which is the position a search from it starts at.
        [[nodiscard]] FloatRect visibleRectInForm() const;
        [[nodiscard]] FloatRect visibleRectInForm(FormBase&) const;
        // The rect every viewport above this control leaves open, whether or not this control
        // reaches into it. visibleRectInForm is this narrowed by the control's own bounds, and is
        // the wrong rect to measure how far a control has been carried past an edge by: that edge
        // is exactly what the narrowing clamps away.
        [[nodiscard]] FloatRect windowInForm() const;
        [[nodiscard]] FloatRect windowInForm(FormBase&) const;
        // Where the content of the nearest box that scrolls this control begins, as an inset from
        // the view that box shows it through. Zero when nothing scrolls it. It is where the first
        // item sits while nothing has been scrolled, so a control that holds itself against the
        // top of the view holds the place it already had rather than moving to a line of its own.
        [[nodiscard]] ScaledPadding scrollContentInset() const;
        // How far the box that scrolls this control has yet to carry it before it comes to
        // rest. Zero when nothing scrolls it, and zero while nothing is gliding. A distance
        // measured on screen and acted on across a press has to add it: the view the press
        // reads is still on its way to where the press before it sent it, and measuring from
        // the view alone asks a second time for ground the previous press already claimed.
        [[nodiscard]] FloatPoint viewTravelRemaining() const;
        //
        [[nodiscard]] ControlMetrics designMetrics() const;
        [[nodiscard]] Padding designPadding() const;
        [[nodiscard]] ScaledPadding scaledPadding(const Scaler&) const;
        [[nodiscard]] ScaledPadding scaledPadding() const;
        // Where this control's children begin, as an inset from its top left. The content padding
        // unless the control moves it, so text, icons and children stay together for everything
        // that does not separate them. A control that wants a child closer to its edge - a split
        // button's dropdown strip, a header's close box - moves this alone and leaves its own
        // content inset intact.
        // Everything that converts between child and parent coordinates goes through here:
        // traversal, hit testing, rectOfControl, scrollChildIntoView and align.
        [[nodiscard]] ScaledPadding childInset(const FormContext&, ScaledPadding contentPadding) const;
        //
        [[nodiscard]] HorizontalAlign horizontalAlign() const;
        void setHorizontalAlign(HorizontalAlign value);
        //
        [[nodiscard]] VerticalAlign verticalAlign() const;
        void setVerticalAlign(VerticalAlign);
        //
        [[nodiscard]] VerticalTextAnchor verticalTextAnchor() const;
        void setVerticalTextAnchor(VerticalTextAnchor);
        //
        [[nodiscard]] HorizontalTextAnchor horizontalTextAnchor() const;
        void setHorizontalTextAnchor(HorizontalTextAnchor);
        //
        [[nodiscard]] bool wordWrap() const { return !(m_flags2 & cfNoWordWrap); }
        void setWordWrap(WordWrap);
        // The pair, which is what the text engine asks for: anything that draws this control's
        // text or measures a position in it needs both axes to name the same block.
        [[nodiscard]] TextAnchor textAnchor() const;

        [[nodiscard]] Tag tag() const { return m_tag; }

        template <typename T>
        [[nodiscard]] T tag() const { return m_tag.get<T>(); }

        template <typename T>
        [[nodiscard]] T tag() { return m_tag.get<T>(); }

        template <std::size_t N, std::size_t Total, typename T>
        [[nodiscard]] T tag() const { return m_tag.get<N, Total, T>(); }

        void setTag(const Tag value) { m_tag = value; }
        // PUTS A RANGE OF THIS CONTROL'S TEXT ON THE CLIPBOARD, as plain text. What a control has
        // in hand here is plain text, and the framework's rich format is declared above this layer,
        // so a control wanting to put one there builds the package itself - see TextBox.
        //
        // The stamp is the input event that asked for the copy. A display server that authorises
        // the request refuses one naming no event.
        void copyText(TextRange, InputStamp);
        //
        bool containsNested(const Control&, const CheckSelf = CheckSelf::Yes) const;
        // null safe
        bool containsNested(const Control*, const CheckSelf = CheckSelf::Yes) const;
        //
        virtual TraversalOrder traversalOrder() const { return {}; }
        virtual bool isLeaf() const { return true; }
        ControlSpan::iterator firstChildInViewport(const FloatRect&);
        [[nodiscard]] bool isViewportEnd(int vRight, int vBottom) const;
        [[nodiscard]] bool isViewportEnd(float vRight, float vBottom) const;
        //
        static TextEngine& textEngine() { return s_textEngine; }
    protected:
        explicit Control(Control* parent);
        // Container interface
        virtual ControlSpan controls() { return {}; }
        ControlSpanC controls() const { return const_cast<Control*>(this)->controls(); }
        // The children in the order navigation reads them, for a container that keeps them in
        // an order of its own. Empty - the default - says it does not, and the control
        // collection is already that order.
        //
        // A container that places its children in named slots is what this is for. It holds
        // them in layout order, which the collection cannot be: the collection owns, and its
        // order is whichever order the code building the container happened to ask for them in.
        // A slot may be unfilled, so an entry may be null.
        [[nodiscard]] virtual ControlSlots navigationSlots() const { return {}; }
        virtual void nestedControlDeleted(Control*) {};
        virtual void childVisibilityChanged(Control&) {}
        virtual void adjustNestedControlVisualState(const Control&, VisualState&) const {}
        virtual void adjustChildMetrics(AdjustMetricsEvent& event) const { event.control.adjustMetrics(event); }
        virtual void adjustChildViewport(AdjustViewportEvent& event) const { event.control.adjustViewPort(event); }
        // The part of this control's content box the given child is seen through, in the
        // coordinates the child's own topLeft is measured in - the same rect adjustChildViewport
        // states in form coordinates. A control leaves the clip alone unless it shows a child
        // through a window smaller than that child, which is what a box scrolling one does: the
        // child's bounds then reach outside the window on every side the content overruns, and
        // the bounds alone would answer for the whole of it. The hit test asks this so it meets
        // a control where the paint drew it. See FormBase::controlAt.
        virtual void adjustChildClip(const Control&, FloatRect&) const {}
        virtual void adjustChildPaint(AdjustPaintEvent& event);

        bool hasOverlayControls() const;
        // Whether any overlay control here is allowed to paint outside its own bounds. What that
        // costs is stated on Control::invalidate, which is the one caller.
        [[nodiscard]] bool hasUnboundedOverlayControls() const;
        bool hasInOverlayControls(const Control&) const;
        ClippingMode getOverlayClippingMode(const Control&) const;
        virtual std::span<const OverlayEntry> overlayControls() const { return {}; }
        // The control an entry names. OverlayEntry keeps the pointer typed away because UiTypes
        // cannot see Control; every entry was a Control* on the way in, so the cast back is exact.
        [[nodiscard]] static Control* overlayControlOf(const OverlayEntry&);
        // Where one of this container's overlay controls is drawn, as an offset from where it was
        // laid out. Zero - the default - is a container whose overlay controls only reorder the
        // painting and stay where the layout put them.
        //
        // Asked of the container each time the answer is needed rather than written onto the
        // child, so a scroll that moves the viewport needs to tell nobody: the offset is a
        // function of where the viewport is now. See Control::floatOffset for what reads it.
        [[nodiscard]] virtual FloatPoint overlayChildOffset(const Control&) const { return {}; }
        // THE CHILD THIS CONTROL HOLDS AGAINST THE TOP OF THE VIEW for as long as what it heads is
        // scrolled under it - a grid's header row, an expander's header strip. Null - the default -
        // is a container that holds nothing, and a holder answers null too while the header or
        // what it heads is hidden: nothing is under it then. What reads it is restLineInForm on
        // the containers below, which is what stacks one header on another, and heldHeaderStrip.
        [[nodiscard]] virtual const Control* heldHeader() const { return nullptr; }
        // THE LINE A HEADER HELD OVER THIS CONTROL'S CONTENT COMES TO REST ON, in form space: where
        // the first item of the scrolled content sits while nothing has been scrolled - the slot of
        // the box scrolling this control plus the inset its body lays that content in, see
        // scrollContentInset - so a header holds the place it had rather than moving to a line of
        // its own; the top of the window this control is seen through, where that is lower; or,
        // where an ancestor already holds a header there, the bottom edge of that header as drawn.
        // That last line is the whole stack: a nested header rests on the one above it, and when
        // that one is being pushed away its bottom moves up and takes the nested header under it.
        // The window still wins where the ancestor's header stands above it: a box scrolling
        // inside a held section shows its content from its own top edge.
        //
        // Asked by the container holding a header, for that header, so the walk starts above the
        // container and its own header is not the answer. windowInForm rather than
        // visibleRectInForm: the edge measured from is the one visibleRectInForm would clamp into
        // this control, and how far the control has been carried past it is what is measured.
        [[nodiscard]] float restLineInForm() const;
        // Where a control held against the bottom of the view comes to rest, in form space:
        // restLineInForm read from the bottom. Nothing is held at the bottom, so nothing stacks.
        [[nodiscard]] float footLineInForm() const;
        // Where this control's held header is drawn, as the offset from where it was laid out
        // that overlayChildOffset answers with. It comes to rest on restLineInForm and holds there
        // for as long as what it heads is still under it, then leaves with the bottom edge of the
        // last of that - `headedBottom`, in the space the header's own topLeft is measured in - so
        // a header never stands with nothing under it. Zero until the scroll has carried the
        // header to its rest line.
        [[nodiscard]] FloatPoint heldHeaderOffset(const Control& header, float headedBottom) const;
        // What this control's held header takes off the top of the view: its own height, and the
        // inset it rests at where no container above this one - within the same scroll - holds a
        // header of its own, since the inset is the first header's to add and every header below
        // rests on the one above it. Nothing that reaches the top of the view can be read through
        // the strip, so a scroll has to clear it and a page press must not count a stop standing
        // behind it as one the eye has read.
        //
        // Not asked whether the header is being held at the moment. A header at rest sits at the
        // top of what it heads and covers the same strip, and a scroll that is about to carry the
        // content past the window's edge is exactly what puts the header there - measuring first
        // and scrolling second would answer for the view being left rather than the one arriving.
        [[nodiscard]] float heldHeaderStrip() const;

        // TODO: are both of these needed?
        virtual void paintChildSurface(PaintEvent& event);
        virtual void childPainted(PaintEvent&) {}

        virtual void scrollChildIntoView(Control&, FloatRect);
        // TODO: does this need to be virtual?
        virtual bool isChildDroppedDown(const Control&) const;
        // Whether a child fills this control's body slot. Only a host with a body slot answers
        // yes - see PanelBase. Asked through isHostedAsBody() above rather than read directly,
        // so the answer stays with the host that owns the slot.
        virtual bool isChildBody(const Control&) const { return false; }
        // Whether this control decides that child's width rather than reading it off the child.
        // A container that lays its children across a lane or into a slot does - which is the
        // usual case, and the default.
        virtual bool isChildWidthGiven(const Control&) const { return true; }
        virtual void getChildText(GetChildTextEvent& event) const;
        virtual void childHoverEnter(Control&) {}
        virtual void childHoverLeave(Control&) {}
        virtual bool controlIsOnScrollBox(Control&) { return false; }
        // Whether this control scrolls that child along the axis - a scroll box, for its body.
        [[nodiscard]] virtual bool scrollsChild(const Control&, ScrollAxis) const { return false; }
        // Moves what this control scrolls by `delta`, at once. A host that scrolls answers from its
        // bars; everything else moves nothing. Reached through Control::scrollViewBy.
        virtual void scrollBy(FloatPoint) {}
        // What this control's own scrolling has yet to carry its content by. A host that
        // scrolls answers from its bars; everything else carries nothing.
        [[nodiscard]] virtual FloatPoint scrollTravelRemaining() const { return {}; }
    protected:
        virtual FormBase* getForm();
        const FormBase* getForm() const;
        Control* navigationContainer(CheckSelf);
        virtual void visibilityChanged() {}
        // The control this one answers for while it holds the focus. A container answers with the
        // item inside it; everything else answers with itself, which is the default. It is what
        // isFocused() is tested against, so a container that delegates does not read as focused.
        virtual Control* focusDelegate();
        // The focus is on its way to event.control. This walks from the target up to the root,
        // and any control on that path may retarget the event - a container that owns its items
        // points it at itself - so the write at the end of the walk is what settles it. Acting on
        // the way through is expected: this is where a container brings its focused item in line.
        virtual void nestedControlFocusing(FocusEvent&);
        // What the pointer is on has changed, and it is this control or something inside it. Told
        // to EVERY ancestor of the new one, which is what tells this apart from childHoverEnter:
        // that pair rides the walk which invalidates state, and that walk stops at the common
        // parent of the control being left and the one being entered. For a pointer crossing from
        // one item to another the common parent is the panel holding them both, so a container
        // standing above that panel - one holding its items in groups - hears nothing at all.
        //
        // hovered is what Input now reports, and is never null: the hover being given up
        // altogether is not a control being hovered, and hoverLeave is what says it.
        virtual void nestedControlHovered(Control* hovered);
        // The focus has arrived at this control or left it. Both sides of every change are told,
        // AFTER the move, so isFocused() answers for the control being told whichever side it is
        // on - which is also why nothing is passed in.
        //
        // A control that only looks different while focused needs none of this: the state is
        // invalidated alongside the call and the focused factor animates itself. This is for a
        // control that DOES something - draws a caret, runs a timer - and would otherwise go on
        // doing it, since adjustFocus reports only the arrival.
        virtual void focusChanged() {}
        virtual void sizeChanged();
        void setTopLeft(float x, float y);
        void setDimensions(const ScaledDimensions);
        void setTop(float value);
        void setLeft(float value);
        void setWidth(float value);
        void setHeight(float value);
        //
        virtual void getControlState(GetStateEvent&) const;
        virtual void adjustMetrics(AdjustMetricsEvent&) const {};

        // Input processing
        // What the platform does with a press here. Whether the pointer stands on the control's
        // own text is a separate answer - see Control::isTextHovered.
        virtual void hitTest(HitTestEvent&) const {}
        virtual NavigationWrap navigationWrap() const;
        // The control whose rect places this one for a directional key, when that is not this
        // control itself. A control that acts for a whole strip - an expander's button, which
        // opens and closes the section its header heads - is met from anywhere along that strip
        // rather than from the corner it occupies, so it answers with the strip. It stays the
        // control that takes the focus; only what scores it changes. Nullptr is its own rect.
        [[nodiscard]] virtual const Control* navigationExtent() const { return nullptr; }
        // The part of this control a scroll has to show, in this control's own coordinates. The
        // whole of it, unless something smaller inside it is what the scroll is for: a control
        // larger than the viewport is fully visible the moment any part of it is, so a request to
        // show a text box taller than the box scrolling it can end anywhere and still count as
        // answered. A text box names its caret instead.
        //
        // Asked at the moment the scroll runs rather than carried with the request, so a deferred
        // one - FormBase::scrollIntoViewOnAlign - is measured against the layout the pass
        // produced. That is what a caret typed past the bottom of the text depends on: the box's
        // new height and the scroll range that follows it are both the alignment's to give.
        [[nodiscard]] virtual FloatRect scrollHotspot() const;
        //
        virtual void mouseMove(MouseMoveEvent&);
        virtual void nestedMouseMove(const MouseMoveEvent&) {};
        virtual void mouseWheel(MouseWheelEvent&) {}
        virtual void mouseHWheel(MouseWheelEvent&) {}
        virtual void drag(DragEvent&);
        //virtual void mouseDown(const MouseDownEvent&) {}
        virtual void pressDown(PressDownEvent&);
        virtual void pressUp(PressUpEvent&) {}
        virtual void click(ClickEvent&);
        virtual void doubleClick(DoubleClickEvent&);
        virtual void tripleClick(TripleClickEvent&);
        //
        virtual void hoverEnter();
        virtual void hoverLeave();
        //
        virtual void contextPopup(ContextPopupEvent&);
        virtual void editContextPopup(EditContextPopupEvent&);
        //
        virtual void keyDown(KeyDownEvent&);
        virtual void keyUp() {}
        virtual void charPress(CharPressEvent&) {};

        // Painting
        virtual void adjustPaint(AdjustPaintEvent&);
        virtual void adjustViewPort(AdjustViewportEvent&) const {}
        //
        // Text routine (protected)
        virtual void getTooltip(GetTooltipEvent&);
        virtual void getText(GetTextEvent&) const;
        // Moves where the children begin. The event arrives holding the content padding and
        // carries the scaler with it, so an override states its inset in design units.
        virtual void adjustChildInset(AdjustChildInsetEvent&) const {}
        virtual void adjustTextRect(AdjustTextRectEvent&) const {}
        // How this control's text should be rasterized, asked each time the text is drawn so a
        // control can answer differently as its state changes. Nothing is stored and there is no
        // setter: a control states its case by overriding this, which keeps the answer next to the
        // reason.
        //
        // Static for anything whose text stays where it was laid out, which then reads as crisply
        // as the platform can manage. Movable for a control that allows the Z animation, since that
        // is what moves text; the answer holds for the control's whole life, so nothing about its
        // rasterization changes under the pointer. Moving only while a hover or press is actually
        // in play, because that is the only part that is worth its cost.
        //
        // The base answer follows allowZAnimation() and needs no overriding to stay in step with
        // it. Override only for a control that allows the depth and still wants its text left
        // alone, which means accepting that the text will be moved while placed as if still.
        [[nodiscard]] virtual TextRenderMode textRenderMode() const;
        FloatRect textBounds(const FormContext&, const FloatRect&) const;
        FloatRect textBounds(PaintEvent&) const;

        // Painting an icon is not a button's privilege. The default hands the event to whatever
        // OnPaintIcon was connected, so a control can take an icon as a property; a control that
        // always draws the same mark overrides this instead and pays nothing for it - no member,
        // no connection, just the vtable slot it already has.
        virtual void paintIcon(PaintIconEvent&);
        virtual void paintSurface(PaintEvent&);
        virtual void paintChildren(PaintEvent&);
        virtual void paintText(PaintEvent&);
        // The draw itself, split off so a control holding a layout of its own replaces this and
        // nothing else: the rect, the text and the flags the result raises stay with paintText,
        // which is the whole of what the framework needs to be the same for every control.
        virtual DrawTextResult drawText(PaintEvent&, const FloatRect& textBounds, const Text&);
        virtual const EditProps* editProps() const { return nullptr; }
        //
        void calculate(FormBase&);
        ScaledDimensions calculateText(AlignEvent&, ScaledDimensions);
        // The measurement itself, split off so a control holding a layout of its own replaces this
        // and nothing else: the gather, the width a run that does not wrap is measured against and
        // the clamp to the box stay with calculateText, which is the whole of what the framework
        // needs to be the same for every control. The mirror of drawText above - a control that
        // overrides one and not the other measures on one layout and draws on another.
        virtual CalculatedDimensions measureText(AlignEvent&, ScaledDimensions asked, const Text&);
        void calculateChildrenSequentially(FormBase&);
        virtual void calculateChildren(FormBase&);
        virtual ScaledDimensions calculateContent(AlignEvent&);
        virtual void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&);
        virtual void adjustPlacement(const Scaler&, ScaledPosition&, ScaledDimensions&) {}
        void align(FormContext&, LayoutPass&, ScaledPosition, ScaledDimensions);
        //
    protected:
        //
        // helpers to control children from within a derived class
        static void setControlDimensions(Control& control, ScaledDimensions dimensions) { control.setDimensions(dimensions); }
        static void setControlWidth(Control& control, float value) { control.setWidth(value); };
        static void setControlHeight(Control& control, float value) { control.setHeight(value); };
        static void setControlTop(Control& control, float value) { control.setTop(value); };
        static void setControlLeft(Control& control, float value) { control.setLeft(value); };

        // Sizes one child. A host whose calculateChildren() does not walk controls() - ButtonBase
        // places its indicator by hand rather than calculating it - has to size the rest itself,
        // and calculate() is protected, so it cannot be reached through a Control& without this.
        static void calculateControl(Control& control, FormBase& form) { control.calculate(form); }

        // A key press, and the character it produced, for a child whose focus this host holds.
        static void forwardKeyDown(Control& control, KeyDownEvent& event) { control.keyDown(event); }
        static void forwardCharPress(Control& control, CharPressEvent& event) { control.charPress(event); }
        // A menu the keyboard asked of this host, for the same child.
        static void forwardContextPopup(Control& control, ContextPopupEvent& event) { control.contextPopup(event); }

        // A row breaks on the lane count, or on the smaller of the maximum the control states and
        // the width handed in here - see StackPanel::wrapWidthLimit, the one caller that states one.
        static ScaledDimensions calculateRows(AlignEvent&, ControlSpan, const bool autoWrap,
            std::size_t wrapCount = k_maxSize, float maxContentWidth = k_maxFloat);
        static ScaledDimensions calculateColumns(AlignEvent&, ControlSpan, const bool autoWrap, std::size_t wrapCount = k_maxSize);

        static void alignControl(Control*, AlignEvent& parentEvent, ScaledPosition, ScaledDimensions);
        // What a lane has over what its items measured, divided among those that asked for it.
        static float laneSurplusShare(ControlSpan::iterator begin, ControlSpan::iterator end,
            float laneExtent, float spacing, float (Control::*extent)() const);
        static void alignSingleRow(AlignEvent&, ControlSpan::iterator rowBegin, ControlSpan::iterator rowEnd,
            ScaledPosition, ScaledDimensions rowSize, ScaledDimensions& contentDimensions);
        static void alignSingleColumn(AlignEvent&, ControlSpan::iterator colBegin, ControlSpan::iterator colEnd,
            ScaledPosition, ScaledDimensions colSize, ScaledDimensions& contentDimensions);

        static void placeControlToHorizontalCenter(Control&, float boundsLeft, float boundsRight);
        static void placeControlToVerticalCenter(Control&, float boundsTop, float boundsBottom);
        static void setControlPlacement(Control&, ScaledPosition, ScaledDimensions);
        static void offsetControl(Control*, FloatPoint);
        static void setControlParent(Control& control, Control* newParent) { control.m_parent = newParent; }

        // Tells the ancestors above this control that everything inside it is going, deepest
        // first, then detaches those children. The destructor of every class that owns controls
        // is this call and nothing else, and it is the only way a holder above hears about a
        // child: ~Control announces a control while it still has a parent, and the detach here
        // is what takes that parent away.
        //
        // The detach is what the announcement buys. A child announcing itself from inside a
        // half-destroyed owner dispatches nestedControlDeleted on a vtable whose derived
        // overrides are already gone; silencing the child and announcing the subtree from the
        // owner keeps that out and still reaches every holder.
        //
        // The children come from controls(), so the override that answers is the one belonging
        // to the class being destroyed - each owner announces the storage it owns and no other.
        // An owner that detaches on its own leaves its children dying unheard, and a form root's
        // current item or a stack panel's is left naming freed memory.
        void releaseChildren();

        // HOW THE CONTROL READS, which is not always where the framework's pointers are: a
        // control with a popup of its own open is still the one being acted on, and this says so
        // while Input::focusedControl names something inside that popup. state() is the other
        // half - what the control IS - and the two meet here.
        //
        // Reachable by a derived class because a control drawing something the focus decides
        // has to ask the same question the ring and the fill are drawn from, rather than
        // assembling its own answer beside it.
        [[nodiscard]] VisualState visualState() const;

        // EVERY CONTROL FROM `from` UP TO BUT NOT INCLUDING `upTo`, told its visual state may
        // have changed. This is for a state that belongs to a PATH rather than to one control:
        // the pointer entering a control enters everything that control stands in, and a grid's
        // selected cell is in effect in its own row and in every row that row stands in.
        //
        // Reachable by a derived class for the second of those. A control that is on the path
        // without carrying the state answers the same before and after, so it costs the walk
        // one invalidation and nothing else - a grid standing between two rows is walked
        // through like anything else.
        static void invalidateStateUp(Control* from, const Control* upTo, InvalidateEvent);
        // The text the framework would draw for this control, gathered through the parent so that
        // a host supplying its children's text is asked. Reachable by a control that holds a
        // layout of its own: it has to build that layout from the same text paintText hands to
        // drawText, or it measures one text and shows another.
        // The text this control answers with, gathered into the caller's buffer. What comes back
        // is the answer, which is the buffer only when one had to be built - see
        // GetTextEventBase::contribute.
        [[nodiscard]] const Text& doGetText(const FormContext&, Text&, EventPhase) const;
    private:
        // The point the press animation scales about, in form coordinates. Everything the control
        // draws converges on it, so it is the one thing that does not move. The centre of the
        // control is the neutral answer; a control with a fixed landmark in it should name that
        // landmark instead, because a ratio of a wide control moves its far edge a long way, and
        // the eye reads whatever sits still as the anchor.
        [[nodiscard]] FloatPoint pressOrigin(const PaintEvent&) const;
        ControlFlags flags() const { return m_flags; }
        // A pass of its own, so that every handler it runs sees a whole tree. A handler reads the
        // tree - hiding an item, asking a control for its form - and a detach interleaved with
        // the announcements would hand one a sibling whose form is unreachable.
        //
        // The recursion keeps the same ancestors. An owner nested inside this one is going too,
        // so a pointer it holds is one nothing reads again.
        static void announceControlsDeleted(Control& container, Control* ancestors);
        // Every animation running on the container's subtree, stopped through the controller
        // the container can still reach - see releaseChildren.
        static void stopNestedAnimations(Control& container, AnimationController&);
        // The flag bits alone. The constructor binds through these, with nothing laid out yet to
        // ask a layout for; the setters ask on top of them.
        void storeHorizontalAlign(HorizontalAlign);
        void storeVerticalAlign(VerticalAlign);
        void doVisibilityChanged();
        // Keeps cfChildHidden as the children stand - see firstChildInViewport.
        void noteChildVisibility(const Control& child);
        void doPressUp(PressUpEvent&);
        void doPressDown(PressDownEvent&);
        void doClick(ClickEvent&);
        void doDoubleClick(DoubleClickEvent&, PointInForm mousePos);
        void doTripleClick(TripleClickEvent&, PointInForm mousePos);
        [[nodiscard]] bool canClick() { return enabled(true); }
        void doHoverEnter();
        void doHoverLeave();
        void doContextPopup(ContextPopupEvent&);
        void doAdjustMetrics(AdjustMetricsEvent&) const;
        // Paint routine (private)
        void doPaintInitialization();
        void doPaintTextInitialization();
        void doAdjustViewport(AdjustViewportEvent&) const;
        void doAdjustPaint(AdjustPaintEvent&);
        void doPaintSurface(PaintEvent&);
        void doPainted(PaintEvent&) const;
        bool isOnScrollBox();
    private:
        inline static TextEngine s_textEngine{};
        static OnAnimate s_onAnimateState;
        static OnAnimate s_onClickRelease;
    private:
        StateFactors m_factors{};
        ControlFlags m_flags{};
        ControlFlags2 m_flags2{ cfDefaultFlags2 };
        Control* m_parent{};
        FloatPoint m_topLeft{};
        ScaledDimensions m_dimensions{};
        ScaledDimensions m_minDimensions{};
    };


}
