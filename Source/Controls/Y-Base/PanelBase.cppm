module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.PanelBase;

import ClaFi.Controls.Base.Container;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class PanelBase;

    // The line between a bar and the body, dragged to resize the bar. See Controls-Base
    export class Splitter : public Control
    {
        friend PanelBase;
    public:
        Splitter(const CreateParams&, PanelSlot bar);
    public:
        std::wstring_view diagnosticText() const override { return L"Splitter"; }
        Interactivity interactivity() const override { return Interactivity::MouseOnly; }
        [[nodiscard]] CursorShape cursor() const override;
        [[nodiscard]] PanelSlot bar() const { return m_bar; }
        // Whether the bar is a side one, so the line stands upright and a drag moves it sideways.
        [[nodiscard]] bool upright() const;
    protected:
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
        void paintFrame(PaintEvent&) override;
        ScaledSize calculateContent(AlignEvent&) override;
        void nestedPressDown(PressDownEvent&) override;
        void nestedDrag(DragEvent&) override;
        void doubleClick(DoubleClickEvent&) override;
    private:
        [[nodiscard]] PanelBase& panel() const;
        // How far the line stands in from the band's edges across it, in design units.
        [[nodiscard]] FloatPoint lineInset(const PaintEvent&) const;
    private:
        static constexpr Thickness k_thickness = Thickness::Thin;
        // The band the pointer takes the line by, in design units.
        static constexpr float k_grabWidth = 7.0f;
        const PanelSlot m_bar;
        float m_startExtent{ 0.0f };   // the bar's extent when the press began, design units
    };


    // A bar resized by the pointer - dragged, or fitted to its content by a double click.
    export struct BarResizeEvent : public Event
    {
        BarResizeEvent(PanelBase&, PanelSlot, float extent);
        PanelBase& panel;   // the panel whose bar it is
        PanelSlot slot;     // the slot the bar stands in
        float extent;       // what the bar comes to, in design units
    };


    // A container with bars around a body, and text of its own.
    export class PanelBase : public Container
    {
        friend Splitter;
    public:
        template<typename... Args>
        explicit PanelBase(const CreateParams&, Args&&...);
    public:
        // Where the panel puts its own text.
        DECLARE_WRITABLE_PROPERTY(TextPlacement, textPlacement, setTextPlacement, TextPlacement::Body)
    public:
        // Raised as the pointer resizes a bar, with what the bar comes to.
        DECLARE_EVENT(BarResizeEvent, OnBarResize, onBarResize)
    public:
        std::wstring_view diagnosticText() const override { return L"PanelBase"; }
        const FloatRect& bodyRect() const { return m_bodyRect; }
        void setTextPlacement(TextPlacement value) { m_textPlacement = value; }
    protected:
        Control* topBar() const { return slot(PanelSlot::Top); }
        Control* leftBar() const { return slot(PanelSlot::Left); }
        Control* body() const { return slot(PanelSlot::Body); }
        template<IsControl T>
        T& bodyAs() const { return *static_cast<T*>(body()); }
        Control* rightBar() const { return slot(PanelSlot::Right); }
        Control* bottomBar() const { return slot(PanelSlot::Bottom); }
        // The bar in a named slot. A panel picking the slot at run time - a slider placing a
        // button by which end of its range the button names - asks for it directly.
        template<IsControl ControlClass, typename...Args>
        ControlClass& createBar(PanelSlot slotId, Args&&... args)
        {
            return setField<ControlClass>(slot(slotId), add<ControlClass>(std::forward<Args>(args)...));
        }
#define DECLARE_CREATE_BAR(name, slotId)                                                            \
        template<IsControl ControlClass, typename...Args>                                           \
        ControlClass& name(Args&&... args){                                                         \
            return createBar<ControlClass>(slotId, std::forward<Args>(args)...);                    \
        }
        DECLARE_CREATE_BAR(createTopBar, PanelSlot::Top)
        DECLARE_CREATE_BAR(createLeftBar, PanelSlot::Left)
        DECLARE_CREATE_BAR(createBody, PanelSlot::Body)
        DECLARE_CREATE_BAR(createRightBar, PanelSlot::Right)
        DECLARE_CREATE_BAR(createBottomBar, PanelSlot::Bottom)
#undef DECLARE_CREATE_BAR
        // Puts a splitter between the bar in slotId and the body, for the pointer to size the bar.
        Splitter& createSplitter(PanelSlot slotId);
        // What the bar in slotId stands at - a side bar's width, a top or bottom bar's height -
        // in design units.
        [[nodiscard]] float barExtent(PanelSlot slotId) const;
        // States the extent the bar in slotId is handed as its preference from the next pass on.
        void setBarExtent(PanelSlot slotId, float extent);
        // Drops a stated extent, so the bar in slotId is what its own metrics make it again.
        void resetBarExtent(PanelSlot slotId);
        // Sizes the bar in slotId to what its content measures, and keeps that as its extent.
        void fitBarToContent(PanelSlot slotId);
        ScaledSize calculateContent(AlignEvent&) override;
        // **What the parts of this panel can be cut to**, composed by the panel's own arrangement:
        // a strip above, a strip below, and a body between two side bars. The panel's text is no
        // part of it - a floor is a MinSize somebody set, and a text carries none.
        //
        // The body's floor is handed in rather than read off the body, because a panel that
        // scrolls its body answers for it differently - see ScrollBox::stateSizeGivenWay.
        [[nodiscard]] ScaledSize composeMinSize(const AlignEvent&, ScaledSize bodyMin) const;
        // The floor the body states, and nothing where there is no body to state one.
        [[nodiscard]] ScaledSize bodyMinSize() const;
        void alignContent(AlignEvent&, ScaledPosition, ScaledSize& contentSize) override;
        // The body's slot is settled and the body has not been laid out into it yet.
        virtual void bodySlotSettled(AlignEvent&) {}
        // The box the body is laid out into - its slot, unless stated larger.
        [[nodiscard]] virtual ScaledSize bodyExtent() const
            { return m_bodyRect.dimensions(); }
        const FloatRect& contentRect() const { return m_contentRect; }
        void adjustTextRect(AdjustTextRectEvent&) const override;
        void adjustChildMetrics(AdjustMetricsEvent&) const override;
        void childVisibilityChanged(Control&) override;
        void swapBarsClockWise();
        void swapBarsCounterClockWise();
        bool isChildBody(const Control& child) const override { return body() == &child; }
        // **A left or right bar is as wide as it measured.** Everything else in a panel is handed a
        // width: the body gets the body slot, a top or bottom bar spans the content. alignContent
        // is where each of those is handed out, and this says the same thing before the fact.
        bool isChildWidthGiven(const Control& child) const override
            { return &child != leftBar() && &child != rightBar(); }
        // A top or bottom bar is as tall as it measured; the rest are handed the slot's height.
        [[nodiscard]] bool isChildHeightGiven(const Control& child) const override
            { return &child != topBar() && &child != bottomBar(); }
        [[nodiscard]] ControlSlots navigationSlots() const override { return m_slots; }
        [[nodiscard]] Control* navigationNeighbour(const Control& child,
            const NavigationStep&) override;
        [[nodiscard]] Control* navigationEntry(const NavigationStep&) override;
    private:
        template<typename T>
        using PerSlot = std::array<T, static_cast<std::size_t>(PanelSlot::Count)>;
        // A bar the pointer resizes - its splitter, and the extent the last drag stated.
        struct BarSizing
        {
            Splitter* splitter{ nullptr };   // non-owning, the control lives in the collection
            std::optional<float> extent{};   // design units, empty while the bar measures itself
            bool fitAsked{ false };   // the next pass measures the bar from its content
        };
        // The least and the most a bar may be dragged to, in design units.
        struct ExtentRange
        {
            float least{ 0.0f };
            float most{ 0.0f };
        };
    private:
        static bool isNotNullAndVisible(const Control* item) { return item && item->visible(); }
        static bool isSideBar(PanelSlot value)
            { return value == PanelSlot::Left || value == PanelSlot::Right; }
        // Puts one part's floor onto a lane's, with the gap that stands between two parts that
        // both have one. A part that gives way to nothing takes the gap beside it with it.
        static void addAlongLane(float& lane, float part, float spacing);
        [[nodiscard]] Control*& slot(PanelSlot value) { return m_slots[static_cast<std::size_t>(value)]; }
        [[nodiscard]] Control* slot(PanelSlot value) const { return m_slots[static_cast<std::size_t>(value)]; }
        [[nodiscard]] std::optional<PanelSlot> slotOf(const Control& child) const;
        [[nodiscard]] BarSizing& barSizing(PanelSlot value)
            { return m_barSizing[static_cast<std::size_t>(value)]; }
        [[nodiscard]] const BarSizing& barSizing(PanelSlot value) const
            { return m_barSizing[static_cast<std::size_t>(value)]; }
        // The splitter of the bar in slotId, and nullptr where the bar has none or is hidden.
        [[nodiscard]] Splitter* shownSplitter(PanelSlot slotId);
        // The gap between the bar in slotId and the body: the spacing, or the splitter's line
        // where that is thicker.
        [[nodiscard]] float splitterGap(PanelSlot slotId, float spacing);
        // Lays the splitter of the bar in slotId into the gap that starts at gapStart.
        void placeSplitter(PanelSlot slotId, AlignEvent&, float gapStart, float gap);
        // What the bar in slotId is laid out at, the bar in oppositeSlot still to be laid out.
        [[nodiscard]] float laidBarExtent(PanelSlot slotId, std::optional<PanelSlot> oppositeSlot,
            const AlignEvent&);
        [[nodiscard]] ExtentRange barExtentRange(PanelSlot slotId) const;
        // What a drag states for the bar in slotId, and the word that it has.
        void dragBarTo(PanelSlot slotId, float extent);
        // A bar measured from its content on a fit's behalf keeps that measurement as its extent.
        void keepFittedExtents();
        // The row a slot stands in: 0 the top bar, 1 the side bars with the body, 2 the bottom bar.
        [[nodiscard]] static std::size_t rowOf(PanelSlot);
        // The slot of a row holding a stop that is nearest the line along the row.
        [[nodiscard]] Control* rowStop(std::size_t row, float lineX) const;
        // How far down a row's slots holding a stop reach, none where no slot holds one.
        [[nodiscard]] std::optional<AxisSpan> rowSpan(std::size_t row) const;
        // The next slot of the middle row past `from` holding a stop, the way forward says.
        [[nodiscard]] Control* middleStopBeside(PanelSlot from, bool forward) const;
    private:
        // Every child a panel has, each in the slot it was given. That is the order they are
        // laid out in and the order navigation reads them in, and it is not the order they were
        // created in: the code building a panel is free to ask for the right bar before the left
        // one, and DialogTitle does exactly that, so the control collection says nothing about
        // where a bar sits.
        //
        // Non-owning - the controls themselves live in the collection, like every other child -
        // and an unfilled slot is nullptr. Nothing else stands in a slot: add() is protected and
        // no panel republishes it, so a child can only arrive through one of the createXxxBar
        // calls or as a splitter, and setField refuses a slot that is already taken.
        PerSlot<Control*> m_slots{};
        PerSlot<BarSizing> m_barSizing{};
        FloatRect m_bodyRect;
        FloatRect m_contentRect;
        FloatPoint m_textSize;
        std::optional<float> m_fixedTextWidth;
    };


    // A static type over a host's body slot, built from its own props bag. See Controls-Base
    export template <IsControl HostClass, IsControl BodyType>
        class WithBody : public HostClass
    {
    public:
        template<typename... HProps, typename... BProps>
        WithBody(const CreateParams&, HostProps<HProps...>&&, BodyProps<BProps...>&&);
        [[nodiscard]] BodyType& body() { return static_cast<BodyType&>(*HostClass::body()); }
        [[nodiscard]] const BodyType& body() const { return static_cast<const BodyType&>(*HostClass::body()); }
    };


    //----------------------------------------------------------------------------


    // PanelBase

    template<typename ...Args>
    PanelBase::PanelBase(const CreateParams& params, Args && ... args)
        :
        Container{ params, std::forward<Args>(args)... },
        INIT_PROPERTY(textPlacement)
    {
        Props::ifThereIs<FixedTextWidth>([this](FixedTextWidth& p) {
            m_fixedTextWidth = p.value;
            }, std::forward<Args>(args)...);
    }

    // WithBody

    template <IsControl HostClass, IsControl BodyType>
    template <typename... HProps, typename... BProps>
    WithBody<HostClass, BodyType>::WithBody(
        const CreateParams& params,
        HostProps<HProps...>&& hostProps,
        BodyProps<BProps...>&& bodyProps
    )
        :
        HostClass{ params, std::get<HProps>(std::move(hostProps.data))... }
    {
        std::apply(
            [this](auto&&... props) {
                this->template createBody<BodyType>(std::forward<decltype(props)>(props)...);
            },
            bodyProps.data
        );
    }

}
