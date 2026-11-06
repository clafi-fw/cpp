export module ClaFi.Controls.Base.WithHeldHeader;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A control a container holds against the top of the view: it states the shape it is laid on.
    export template<typename T>
        concept IsHeldHeader = IsControl<T> && requires(const T& header, const PaintEvent& event,
            const FloatRect& rect)
        {
            { header.silhouette(event, rect) } -> std::same_as<RoundedRectangleParts>;
        };

    export template<IsControl Base, IsHeldHeader Header>
        // A container whose header stays put while its body scrolls under it. See Controls-Base
        class WithHeldHeader : public Base
    {
    public:
        using Base::Base;
    protected:
        // Takes the header this control holds, once whoever builds it has.
        void holdHeader(Header&);
        // What the header heads: the body whose bottom edge pushes the header away; null for none.
        [[nodiscard]] virtual const Control* headedBody() const = 0;
        [[nodiscard]] const Control* heldHeader() const override;
        [[nodiscard]] FloatPoint overlayChildOffset(const Control&) const override;
        void paintChildren(PaintEvent&) override;
        void scrollChildIntoView(Control&, FloatRect) override;
    private:
        Header* m_heldHeader{};
        // What holds the overlay entry naming the header; null leaves the header a plain child.
        Control* m_headerHost{};
    };


    //----------------------------------------------------------------------------


    template<IsControl Base, IsHeldHeader Header>
    void WithHeldHeader<Base, Header>::holdHeader(Header& header)
    {
        m_heldHeader = &header;
        m_headerHost = this->heldHeaderHost();
        if (m_headerHost && m_headerHost->addOverlayControl(header, ClippingMode::Standard))
            return;
        m_headerHost = this->addOverlayControl(header, ClippingMode::Standard) ? this : nullptr;
    }

    template<IsControl Base, IsHeldHeader Header>
    const Control* WithHeldHeader<Base, Header>::heldHeader() const
    {
        if (!m_headerHost || !m_heldHeader->visible())
            return nullptr;
        const Control* body = headedBody();
        if (!body || !body->visible())
            return nullptr;
        return m_heldHeader;
    }

    template<IsControl Base, IsHeldHeader Header>
    FloatPoint WithHeldHeader<Base, Header>::overlayChildOffset(const Control& child) const
    {
        if (&child != heldHeader())
            return {};
        return this->heldHeaderOffset(child, headedBody()->bottom());
    }

    template<IsControl Base, IsHeldHeader Header>
    void WithHeldHeader<Base, Header>::paintChildren(PaintEvent& event)
    {
        if (!m_headerHost)
            return Base::paintChildren(event);
        for (ControlPtr& child : this->controls())
        {
            if (child.get() != m_heldHeader && child->visible())
                event.paintChild(*child);
        }
        if (!event.overlayStage() || event.overlayHost() != m_headerHost)
            return;
        if (!m_heldHeader->visible())
            return;
        const float travel = overlayChildOffset(*m_heldHeader).y;
        if (travel != 0.0f)
        {
            const FloatRect rect = FloatRect::FromDimensions(
                event.contentPosition() + m_heldHeader->topLeft() + FloatPoint{ 0.0f, travel },
                m_heldHeader->scaledSize());
            event.paintHeldBackdrop(m_heldHeader->silhouette(event, rect), travel);
        }
        event.paintChild(*m_heldHeader);
    }

    template<IsControl Base, IsHeldHeader Header>
    void WithHeldHeader<Base, Header>::scrollChildIntoView(Control& control, FloatRect controlRect)
    {
        if (&control != m_heldHeader)
            controlRect.top -= this->heldHeaderStrip();
        Base::scrollChildIntoView(control, controlRect);
    }
}
