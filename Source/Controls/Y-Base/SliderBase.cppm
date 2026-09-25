module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.SliderBase;

import ClaFi.Controls.Panel;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class SliderBase;

    // The slider has come to rest on a new value.
    export struct SliderChangeEvent : public Event
    {
        SliderChangeEvent(const SliderBase& slider, float newPosition, float previousPosition);
        const SliderBase& slider;
        const float newPosition;
        const float previousPosition;
    };

    using SliderBaseClass = PanelBase;

    // Base for ScrollBars and Sliders.
    export class SliderBase : public SliderBaseClass
    {
    public:
        class ChildItem : public Control
        {
        public:
            explicit ChildItem(const CreateParams& params);
            SliderBase* parent() const { return static_cast<SliderBase*>(Control::parent()); }
            ScaledDimensions calculateContent(AlignEvent&) override;
        protected:
            void adjustPaint(AdjustPaintEvent&) override;
        };
        class ScrollButton : public ChildItem
        {
            friend SliderBase;
        public:
            ScrollButton(const CreateParams&, ScrollDirection);
        public:
            std::wstring_view diagnosticText() const override { return L"ScrollButton"; }
            [[nodiscard]] ScrollDirection direction() const { return m_direction; }
            // A host scrolling its own content while the pointer drags near an edge drives the
            // button, so the button shows the movement it is making. This is not a click: the
            // pointer is already where the content has to arrive, and it arrives in the same
            // frame rather than travelling there. The speed multiplies a fixed rate, and the
            // ground covered is measured in time rather than in repeats, so it is the same rate
            // on a machine that keeps up with the timer and on one that does not.
            void startAutoScroll(float speed);
            void stopAutoScroll();
            // What it has always been: a mouse target, hovered and pressed, never focused. Saying
            // so is what puts it in reach of the core hover and press animations - and, on the
            // other side of the same rule, what lets a child tell a parent worth following from a
            // plain container that merely happens to contain the pointer.
            Interactivity interactivity() const override { return Interactivity::MouseOnly; }
        protected:
            void adjustMetrics(AdjustMetricsEvent& event) const override { parent()->adjustButtonMetrics(event); }
            void adjustPaint(AdjustPaintEvent&) override;
            void click(ClickEvent&) override;
            void pressDown(PressDownEvent&) override;
            void pressUp(PressUpEvent&) override;
            // 'lockHoveredControl' prevents the hovered control from switching
            // when the mouse leaves the button area while the button is still pressed.
            // It kind of emulates the mouse capturing.
            // Otherwise the click repeater stucks in the active state
            void drag(DragEvent& event) override { event.lockHoveredControl(); }
            void paintSurface(PaintEvent&) override;
        private:
            void doIt();
            void repeat(RepeatEvent&);
            void autoScroll(float elapsedSeconds);
        private:
            // Design units a second at speed 1. The pointer just inside the trigger zone scrolls
            // at this rate, and the multiplier the host passes takes it up from there.
            static constexpr float k_autoScrollUnitsPerSecond = 180.0f;
            static constexpr MilliSeconds k_autoScrollInterval = { 0 }; // Maximum possible frame rate
            ScrollDirection m_direction;
            bool m_autoScrolling{ false };
            float m_autoScrollSpeed{ 1.0f };
            // Bound in the constructor rather than by a default member initializer: the handler
            // captures this and calls repeat(), whose body lives in the implementation unit, so
            // the closure has no business being in this interface.
            EventRepeater m_clickRepeater;
        };

        class Thumb : public ChildItem
        {
        public:
            using ChildItem::ChildItem;
            std::wstring_view diagnosticText() const override { return L"Thumb"; }
            // Dragged, so it is a mouse target on the same terms as the buttons.
            Interactivity interactivity() const override { return Interactivity::MouseOnly; }
        protected:
            void adjustMetrics(AdjustMetricsEvent& event) const override { parent()->adjustThumbMetrics(event); }
            void getControlState(GetStateEvent&) const override;
            void adjustPaint(AdjustPaintEvent&) override;
            void paintSurface(PaintEvent&) override;
            void getTooltip(GetTooltipEvent&) override;
            void pressDown(PressDownEvent&) override;
            void drag(DragEvent&) override;
        private:
            float m_startTop;
        };

        class BodySpacer : public ChildItem
        {
        public:
            using ChildItem::ChildItem;
            // Used to ensure minimum size when there are no scroll buttons.
            // It calculates by PanelBase, but then we set its size to 0
            // so that it do not overlap the slot neither the thumb;

            // Alternatively, the thumb itsel can perform this task,
            // we just need it to be created as a body,
            // and realign it in the alignContent (which is already done actually),
            // so this BodySpacer probably is not needed at all
            //
            // Upd: But what if the thumb is hidden? There's no such a case in the wild at the moment, but what if?
            // Then we still need to have that separate BodySpacer, so let's leave it alone.
        };
    public:
        template<typename... Args>
        explicit SliderBase(const CreateParams&, SliderViewMode, Args&&...);
    public:
        // Which way the slider runs.
        DECLARE_WRITABLE_PROPERTY(ScrollAxis, axis, setAxis, ScrollAxis::Vertical)
    public:
        // The slider has come to rest on a new value.
        DECLARE_EVENT(SliderChangeEvent, OnChange, onChange)
    public:
        void setScrollAxis(ScrollAxis);
        Thumb& thumb() const { return m_thumb; }
        ScrollButton& beginButton() const { return m_beginButton; }
        ScrollButton& endButton() const { return m_endButton; }
        void setScrollButtons(ScrollButtons);
        float position() const { return m_position; }
        // Where the position is heading. It equals the position whenever no glide is in
        // flight, so a caller measuring against it is measuring against the position at rest.
        [[nodiscard]] float positionTarget() const { return m_positionTarget; }
        void setPosition(float, bool triggerChange = true);
        void animatePosition(float);
        void animatePositionBySteps(float steps);
        void stopPositionAnimation();
        void offsetPosition(float);
        void offsetPositionBySteps(float);
        // The pointer is naming the position itself - it is holding the thumb or the slot, and
        // it holds them until the button comes up. A host that carries its own anchors along
        // with its content leaves them where they are while this is true: the content moves
        // because the pointer moves, and an anchor following the content would add the content's
        // travel to the pointer's own, so every move would name a larger one.
        [[nodiscard]] bool positionHeldByPointer() const { return m_thumbPressed || m_slotPressed; }
    protected:
        using ContainerBase::add;
        // Fixed for the whole life of the control, and known before its parts are built: which
        // end of a vertical range sits at the top is what places the scroll buttons, and they are
        // placed as they are created.
        [[nodiscard]] SliderViewMode viewMode() const { return m_viewMode; }
        // A scroll bar's slot and buttons name a place to send the view to, so the content travels
        // there and is watched on the way. A slider's name a value, and a value takes effect when
        // it is set.
        [[nodiscard]] bool glidesToPosition() const { return viewMode() == SliderViewMode::ScrollBar; }
        void setAxis(ScrollAxis value) { m_axis = value; }
        void paintButton(ScrollButton&, PaintEvent&);
        virtual void changed(SliderChangeEvent&);
        ScrollDirection verticalDirection() const { return viewMode() == SliderViewMode::ScrollBar ? ScrollDirection::ToEnd : ScrollDirection::ToBegin; }
        virtual ScrollInfo controlScrollInfo() const = 0;
        // Brings the position back inside a range that has just changed. It glides, so a section
        // collapsing above the view carries the content up with it rather than snapping it there.
        void revalidatePosition();
        virtual float stepSize() = 0;
        virtual float buttonSize(const AppTheme&) = 0;
        void adjustNestedControlVisualState(const Control&, VisualState&) const override;
        virtual void adjustButtonMetrics(AdjustMetricsEvent&) const = 0;
        virtual void adjustButtonPaint(AdjustPaintEvent&) = 0;
        // How far a scroll button's surface sits inside its bounds. A design value.
        [[nodiscard]] virtual FloatPoint buttonInset() const { return {}; }
        virtual void adjustThumbMetrics(AdjustMetricsEvent&) const = 0;
        virtual void adjustThumbPaint(AdjustPaintEvent&) = 0;
        virtual void paintButtonMark(PaintEvent&, float size, ScrollDirection) = 0;
        virtual void paintThumb(PaintEvent&) = 0;
        virtual void getThumbTooltip(GetTooltipEvent&) {}
        virtual void thumbPressDown() {}
        void controlFeedBack();
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        void alignThumb() const;
        void getControlState(GetStateEvent&) const override;
        void pressDown(PressDownEvent&) override;
        void pressUp(PressUpEvent&) override;
        void drag(DragEvent&) override;
        static constexpr float k_thumbSize = 20.0f;
    private:
        void applyPosition(float value, bool triggerChange);
        // The travel the thumb runs along, in this control's own space.
        [[nodiscard]] FloatRect slotArea() const;
        // A point on the form in that same space.
        [[nodiscard]] FloatPoint slotPoint(FloatPoint pointOnForm) const;
        // The position the slot names at a point in it. The point names where the middle of the
        // thumb goes, so half a thumb comes off each end of the travel, and a point past either
        // end names that end.
        [[nodiscard]] float slotPosition(FloatPoint point) const;
        // The bar a button of this direction belongs in.
        [[nodiscard]] PanelSlot barSlotFor(ScrollDirection) const;
        float calcButtonSize(const AppTheme&, const Scaler& scaler);
    private:
        SliderViewMode m_viewMode;
        float m_position{ 0.0f };
        // Where the position is heading. It equals m_position whenever no glide is in flight, so
        // a request naming a target already in hand is dropped instead of restarted. A glide asks
        // for an alignment on every tick, and that pass asks for the glide back; dropping the
        // repeat is what lets the glide reach its end instead of starting over each frame.
        float m_positionTarget{ 0.0f };
        // A press that landed on the slot rather than on the thumb or a button. It is what the
        // drag steers by: the press sends the position to the point under the pointer, and the
        // thumb is still travelling there, so it is not under the pointer to take the drag over.
        bool m_slotPressed{ false };
        // A pointer is holding the thumb. Set where the press lands, and cleared on the
        // press-up, which is dispatched from the down item and so reaches this control however
        // far the pointer has travelled off the bar in between.
        bool m_thumbPressed{ false };
        FloatRect m_sliderArea{};
        ScrollButton& m_beginButton{ createBar<ScrollButton>(
            barSlotFor(ScrollDirection::ToBegin), ScrollDirection::ToBegin) };
        BodySpacer& m_bodySpacer{ createBody<BodySpacer>() };
        Thumb& m_thumb{ add<Thumb>() };
        ScrollButton& m_endButton{ createBar<ScrollButton>(
            barSlotFor(ScrollDirection::ToEnd), ScrollDirection::ToEnd) };
    };


//-----------------------------------------------------------------------------


    // SliderBase
    //
    // The constructor is the one definition that has to stay here: it is a template, so every
    // caller instantiates it from this interface. Every other body lives in SliderBase.cpp.

    template<typename ...Args>
    SliderBase::SliderBase(const CreateParams& params, SliderViewMode viewMode, Args&& ... args )
        :
        SliderBaseClass{ params, TextPlacement::Top, std::forward<Args>(args)... },
        m_viewMode{ viewMode },
        INIT_PROPERTY(axis)
    {
        Props::ifThereIs<ScrollButtons>([&](const auto& value) {
            setScrollButtons(value);
            }, std::forward<Args>(args)...);
    }

}
