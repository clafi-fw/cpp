module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.PanelBase;

import ClaFi.Controls.Base.Container;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A container with bars around a body, and text of its own.
    export class PanelBase : public Container
    {
    public:
        template<typename... Args>
        explicit PanelBase(const CreateParams&, Args&&...);
    public:
        // Where the panel puts its own text.
        DECLARE_WRITABLE_PROPERTY(TextPlacement, textPlacement, setTextPlacement, TextPlacement::Body)
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
        ScaledDimensions calculateContent(AlignEvent&) override;
        // WHAT THE PARTS OF THIS PANEL CAN BE CUT TO, composed by the panel's own arrangement:
        // a strip above, a strip below, and a body between two side bars. The panel's text is no
        // part of it - a floor is a MinSize somebody set, and a text carries none.
        //
        // The body's floor is handed in rather than read off the body, because a panel that
        // scrolls its body answers for it differently - see ScrollBox::stateSizeGivenWay.
        [[nodiscard]] ScaledDimensions composeMinSize(const AlignEvent&, ScaledDimensions bodyMin) const;
        // The floor the body states, and nothing where there is no body to state one.
        [[nodiscard]] ScaledDimensions bodyMinSize() const;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions& contentSize) override;
        // The body's slot is settled and the body has not been laid out into it yet, so anything a
        // panel states ABOUT that slot reaches the body on the pass that settled it. See
        // Controls-Base
        virtual void bodySlotSettled(AlignEvent&) {}
        const FloatRect& contentRect() const { return m_contentRect; }
        void adjustTextRect(AdjustTextRectEvent&) const override;
        void swapBarsClockWise();
        void swapBarsCounterClockWise();
        bool isChildBody(const Control& child) const override { return body() == &child; }
        // A LEFT OR RIGHT BAR IS AS WIDE AS IT MEASURED. Everything else in a panel is handed a
        // width: the body gets the body slot, a top or bottom bar spans the content. alignContent
        // is where each of those is handed out, and this says the same thing before the fact.
        bool isChildWidthGiven(const Control& child) const override
            { return &child != leftBar() && &child != rightBar(); }
        [[nodiscard]] ControlSlots navigationSlots() const override { return m_slots; }
    private:
        static bool isNotNullAndVisible(const Control* item) { return item && item->visible(); }
        // Puts one part's floor onto a lane's, with the gap that stands between two parts that
        // both have one. A part that gives way to nothing takes the gap beside it with it.
        static void addAlongLane(float& lane, float part, float spacing);
        [[nodiscard]] Control*& slot(PanelSlot value) { return m_slots[static_cast<std::size_t>(value)]; }
        [[nodiscard]] Control* slot(PanelSlot value) const { return m_slots[static_cast<std::size_t>(value)]; }
    private:
        // Every child a panel has, each in the slot it was given. That is the order they are
        // laid out in and the order navigation reads them in, and it is not the order they were
        // created in: the code building a panel is free to ask for the right bar before the left
        // one, and FormTitle does exactly that, so the control collection says nothing about
        // where a bar sits.
        //
        // Non-owning - the controls themselves live in the collection, like every other child -
        // and an unfilled slot is nullptr. There is nothing outside these five: add() is
        // protected and no panel republishes it, so a child can only arrive through one of the
        // createXxxBar calls, and setField refuses a slot that is already taken.
        std::array<Control*, static_cast<std::size_t>(PanelSlot::Count)> m_slots{};
        //
        FloatRect m_bodyRect;
        FloatRect m_contentRect;
        FloatPoint m_textSize;
        std::optional<float> m_fixedTextWidth;
    };


    // Names the type of a host's body slot and builds that body from its own props bag.
    // The body stays a distinct child - the host positions, clips and hides it independently -
    // so this puts a static type over the slot rather than folding the two into one control.
    // That is what separates it from Form<ControlClass>, where the content is the whole of the
    // client area and the form is not itself a node in the control tree.
    // Each props bag routes by type, so a bag must not carry the same prop type twice.
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

    ScaledDimensions PanelBase::calculateContent(AlignEvent& event)
    {
        m_textSize = Container::calculateContent(event).toFloat();
        if (m_fixedTextWidth.has_value())
            m_textSize.x = event.scale(m_fixedTextWidth.value());
        ScaledDimensions result{ 0, 0 };

        switch (m_textPlacement)
        {
        case TextPlacement::Top:
            result = m_textSize;
            break;
        case TextPlacement::Body:
        case TextPlacement::Left:
            break;
        }

        if (isNotNullAndVisible(topBar()))
        {
            result.x = std::max(result.x, topBar()->width());
            if (result.y)
                result.y += event.spacing.y;
            result.y += topBar()->height();
        }

        float lrcWidth = 0.0f;
        float lrcHeight = 0.0f;

        if (isNotNullAndVisible(leftBar()))
        {
            lrcWidth = leftBar()->width();
            lrcHeight = leftBar()->height();
        }

        m_bodyRect.clear();
        switch (m_textPlacement)
        {
        case TextPlacement::Body:
            m_bodyRect.setDimensions(m_textSize);
            break;
            // TODO: process TextPlacement::Left.
        default:
            m_bodyRect.setDimensions({ 0.0f, 0.0f });
        }

        if (isNotNullAndVisible(body()))
        {
            m_bodyRect.right = std::max(m_bodyRect.right, body()->width());
            m_bodyRect.bottom = std::max(m_bodyRect.bottom, body()->height());
        }

        if (m_bodyRect.right)
        {
            if (lrcWidth)
                lrcWidth += event.spacing.x;
            lrcWidth += m_bodyRect.right;
        }
        if (m_bodyRect.bottom)
            lrcHeight = std::max(lrcHeight, m_bodyRect.bottom);

        if (isNotNullAndVisible(rightBar()))
        {
            if (lrcWidth)
                lrcWidth += event.spacing.x;
            lrcWidth += rightBar()->width();
            lrcHeight = std::max(lrcHeight, rightBar()->height());
        }

        if (lrcHeight)
        {
            if (result.y)
                result.y += event.spacing.y;
            result.y += lrcHeight;
        }
        result.x = std::max(result.x, lrcWidth);

        if (isNotNullAndVisible(bottomBar()))
        {
            if (result.y)
                result.y += event.spacing.y;
            result.y += bottomBar()->height();
            result.x = std::max(result.x, bottomBar()->width());
        }

        switch (m_textPlacement)
        {
        case TextPlacement::Left:
            if (result.x && m_textSize.x)
                result.x += event.spacing.x;
            result.x += m_textSize.x;
            result.y = std::max(result.y, m_textSize.y);
            break;
        case TextPlacement::Body:
        case TextPlacement::Top:
            break;
        }

        event.calculatedMinSize = composeMinSize(event, bodyMinSize());

        return result;
    }

    ScaledDimensions PanelBase::composeMinSize(const AlignEvent& event, ScaledDimensions bodyMin) const
    {
        const ScaledDimensions topMin = isNotNullAndVisible(topBar())
            ? topBar()->calculatedMinSize()
            : ScaledDimensions{ 0.0f, 0.0f };
        const ScaledDimensions leftMin = isNotNullAndVisible(leftBar())
            ? leftBar()->calculatedMinSize()
            : ScaledDimensions{ 0.0f, 0.0f };
        const ScaledDimensions rightMin = isNotNullAndVisible(rightBar())
            ? rightBar()->calculatedMinSize()
            : ScaledDimensions{ 0.0f, 0.0f };
        const ScaledDimensions bottomMin = isNotNullAndVisible(bottomBar())
            ? bottomBar()->calculatedMinSize()
            : ScaledDimensions{ 0.0f, 0.0f };

        float middleWidth = 0.0f;
        addAlongLane(middleWidth, leftMin.x, event.spacing.x);
        addAlongLane(middleWidth, bodyMin.x, event.spacing.x);
        addAlongLane(middleWidth, rightMin.x, event.spacing.x);
        const float middleHeight = std::max(leftMin.y, std::max(bodyMin.y, rightMin.y));

        ScaledDimensions result = { 0.0f, 0.0f };
        addAlongLane(result.y, topMin.y, event.spacing.y);
        addAlongLane(result.y, middleHeight, event.spacing.y);
        addAlongLane(result.y, bottomMin.y, event.spacing.y);
        result.x = std::max(topMin.x, std::max(middleWidth, bottomMin.x));
        return result;
    }

    ScaledDimensions PanelBase::bodyMinSize() const
    {
        if (!isNotNullAndVisible(body()))
            return { 0.0f, 0.0f };
        return body()->calculatedMinSize();
    }

    void PanelBase::alignContent(AlignEvent& event, ScaledPosition, ScaledDimensions& contentSize)
    {
        m_contentRect = { 0, 0, contentSize.x, contentSize.y };
        switch (m_textPlacement)
        {
        case TextPlacement::Top:
        {
            CalculatedDimensions newTextSize = calculateText(event, contentSize);
            m_contentRect.top += newTextSize.y;
            float delta = newTextSize.y - m_textSize.y;
            m_contentRect.bottom += delta;
            contentSize.y += delta;
            m_textSize = newTextSize;
            break;
        }
        case TextPlacement::Left:
            m_contentRect.left += m_textSize.x;
            if (m_contentRect.left)
                m_contentRect.left += event.spacing.x;
            break;
        case TextPlacement::Body:
            break;
        }
        m_bodyRect = m_contentRect;
        if (isNotNullAndVisible(topBar()))
        {
            if (m_contentRect.top)
                m_contentRect.top += event.spacing.y;
            alignControl(
                topBar(),
                event,
                m_contentRect.topLeft(),
                { m_contentRect.width(), topBar()->height() }
            );
            m_bodyRect.top = m_contentRect.top + topBar()->height();
        }
        if (m_bodyRect.top)
            m_bodyRect.top += event.spacing.y;
        if (isNotNullAndVisible(bottomBar()))
        {
            FloatRect itsRect = m_contentRect;
            m_bodyRect.bottom = m_contentRect.bottom - bottomBar()->height();
            alignControl(
                bottomBar(),
                event,
                { m_contentRect.left, m_bodyRect.bottom },
                { m_contentRect.right, bottomBar()->height() }
            );
            if (bottomBar()->height())
                m_bodyRect.bottom -= event.spacing.y;
        }
        if (isNotNullAndVisible(leftBar()))
        {
            alignControl(
                leftBar(),
                event,
                m_bodyRect.topLeft(),
                { leftBar()->width(), m_bodyRect.height() }
            );
            m_bodyRect.left += leftBar()->width();
            if (leftBar()->width())
                m_bodyRect.left += event.spacing.x;
        }
        if (isNotNullAndVisible(rightBar()))
        {
            FloatRect itsRect = m_bodyRect;
            alignControl(
                rightBar(),
                event,
                { m_bodyRect.right, m_bodyRect.top },
                { rightBar()->width(), m_bodyRect.height() }
            );
            float w = rightBar()->width();
            if (w)
            {
                offsetControl(rightBar(), { -w, 0.0f });
                m_bodyRect.right -= w;
                m_bodyRect.right -= event.spacing.x;
            }
        }

        // The slot is settled here whether or not anything stands in it: its width takes no more
        // changes below, and the only one its height takes is the grow a body asks for.
        bodySlotSettled(event);

        if (isNotNullAndVisible(body()))
        {
            float delta = body()->height();
            alignControl(body(), event, m_bodyRect.topLeft(), m_bodyRect.dimensions());
            delta = body()->height() - delta;

            // The body came out taller than the slot it was offered, so this panel grows to hold
            // it - unless its own height is dictated from outside, in which case growing fights
            // whoever set it. Two separate things dictate a height, and either one is enough:
            //
            //   Fill            - a parent stretches this panel to the space it was given, and
            //                     the form's root control is stretched to the window this way.
            //                     The root has no parent to ask about a body slot at all.
            //   isHostedAsBody  - a host handed this panel exact dimensions for its body slot,
            //                     whatever this panel's own alignment says, and has already laid
            //                     the rest of itself out around the height it gave.
            bool sizeComesFromOutside = verticalAlign() == VerticalAlign::Fill || isHostedAsBody();
            bool needExpand = (delta > 0.0f) && !sizeComesFromOutside;
            if (needExpand)
            {
                m_contentRect.bottom += delta;
                m_bodyRect.bottom += delta;
                contentSize.y += delta;
                if (isNotNullAndVisible(bottomBar()))
                    offsetControl(bottomBar(), { 0.0f, delta });
            }
        }
    }

    void PanelBase::adjustTextRect(AdjustTextRectEvent& event) const
    {
        switch (m_textPlacement)
        {
        case TextPlacement::Body:
            event.textBounds = FloatRect::fromDimensions(
                event.textBounds.topLeft() + m_bodyRect.topLeft(),
                m_bodyRect.dimensions());
            break;
        case TextPlacement::Top:
            break;
        case TextPlacement::Left:
            event.textBounds.top = event.textBounds.top + m_bodyRect.top;
            event.textBounds.setDimensions(m_textSize);
            break;
        }
    }

    void PanelBase::swapBarsClockWise()
    {
        std::swap(slot(PanelSlot::Left), slot(PanelSlot::Top));
        std::swap(slot(PanelSlot::Right), slot(PanelSlot::Bottom));
        form().invalidateAlign();
    }

    void PanelBase::swapBarsCounterClockWise()
    {
        std::swap(slot(PanelSlot::Left), slot(PanelSlot::Bottom));
        std::swap(slot(PanelSlot::Right), slot(PanelSlot::Top));
        form().invalidateAlign();
    }

    void PanelBase::addAlongLane(float& lane, float part, float spacing)
    {
        if (part <= 0.0f)
            return;
        if (lane > 0.0f)
            lane += spacing;
        lane += part;
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
