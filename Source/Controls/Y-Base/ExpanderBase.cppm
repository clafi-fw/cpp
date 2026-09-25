module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Icons.ExpanderMark;
import ClaFi.Core.AppTheme_AnimationSlots;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;
import ClaFi.Core.Context.PaintIconEvent;

namespace ClaFi::Controls
{
    // Only a section draws itself. The other two name no colour rules, and a control that names
    // none paints nothing in any state, which is what leaves it standing on the surface it was
    // put on.
    export [[nodiscard]] inline OptionalUiElement expanderColorRules(
        UiElement rules, ExpanderViewMode viewMode)
    {
        if (viewMode == ExpanderViewMode::Section)
            return rules;
        return std::nullopt;
    }

    // The chevron a header carries, whose click is what opens the expander.
    export class ExpanderButton : public ToolButton
    {
    public:
        template<typename... Args>
        explicit ExpanderButton(const CreateParams&, Args&&...);
    protected:
        // The button opens and closes the section its header heads, so it acts for the whole
        // header: a key meets it from anywhere along that strip rather than from the corner it
        // occupies, and a key leaving it starts from the same strip. It is built as the header's
        // own bar, so the header is its parent; a button given no header answers with its own
        // rect.
        [[nodiscard]] const Control* navigationExtent() const override { return parent(); }
    };

    template<typename ...Args>
    ExpanderButton::ExpanderButton(const CreateParams& params, Args &&... args)
        :
        ToolButton{
            params,
            ButtonViewMode::IconOnly,
            IconSize{ 12.0f },
            ShowSelectionOnSurface::No, // visible in grids
            std::forward<Args>(args)...
        }
    {
    }

    export class ToggleExpandedEvent: public Event
    {
    public:
        explicit ToggleExpandedEvent(bool expanded) : m_expanded{ expanded }{}
        bool expanded() const { return m_expanded; }
    private:
        bool m_expanded;
    };

    // The strip an expander is picked and opened by.
    export class ExpanderHeader : public PanelBase
    {
    public:
        template<typename... Args>
        explicit ExpanderHeader(const CreateParams& params, Args&&...);
    public:
        void invalidateChevron() const { m_button.invalidate(); }
        void toggleExpanded() { setExpanded(!expanded()); }
        bool expanded() const { return m_expanded; }
        float expandedFactor() const { return m_expandedFactor; }
        void setExpanded(bool value);
        [[nodiscard]] ExpanderViewMode viewMode() const { return m_viewMode; }
        // The header shows the label in every look, so a host spells the header text one way and
        // the button stays a target of its own.
        void setHeaderText(const Text& value) { text() << value; }
        ExpanderButton& button() { return m_button; }
        const ExpanderButton& button() const { return m_button; }
        // The shape a held header is laid on: its rect with the corners it turns while open - the
        // pair at the top, since the pair at the bottom is squared while the body is showing. See
        // PaintEvent::paintHeldBackdrop.
        [[nodiscard]] RoundedRectangleParts silhouette(const PaintEvent& hostEvent, const FloatRect& headerRect) const;
    protected:
        void adjustPaint(AdjustPaintEvent&) override;
        void doubleClick(DoubleClickEvent&) override;
        void paintSurface(PaintEvent& event) override
        {
            PanelBase::paintSurface(event);
        };
    private:
        [[nodiscard]] static TextPlacement textPlacementFor(ExpanderViewMode);
        ExpanderButton& createChevronButton();
        void createDividerLine();
        void paintButton(PaintIconEvent&) const;
        void scrollSectionIntoView() const;
    private:
        bool m_expanded{ true };
        float m_expandedFactor{ 1.0f };
        // Declared before the button: the look names the slot the button is built into, and a
        // member initializer runs in declaration order.
        ExpanderViewMode m_viewMode;
        ExpanderButton& m_button{ createChevronButton() };
    };

    // A CONTAINER THAT HOLDS ITS EXPANDER HEADER AGAINST THE TOP OF THE VIEW while the body it
    // heads is scrolled under it, the way a grid holds its header row - see Control::heldHeader.
    // The header is an overlay control whose entry goes to the control this one stands in, as the
    // grid's does: what the header draws around itself then lands on that control rather than
    // stopping at this one's edge, and a header nested in another's body is painted in a pass that
    // runs inside the outer header's standard pass - under the outer header, which is what a
    // nested header being pushed away slides beneath. A parent with no overlay list leaves this
    // control hosting the entry itself.
    //
    // The header is painted here by name after the other children and skipped in every pass but
    // its own - the TabStripBase shape - so nothing below chains to the base for it.
    export template<IsControl Base>
        // A panel whose header stays put while its body scrolls under it.
        class WithHeldHeader : public Base
    {
    public:
        using Base::Base;
    protected:
        // Takes the header this control holds, once whoever builds it has.
        void holdHeader(ExpanderHeader&);
        // What the header heads: the body, whose bottom edge is what pushes the header away.
        // Null while there is none.
        [[nodiscard]] virtual const Control* headedBody() const = 0;
        [[nodiscard]] const Control* heldHeader() const override;
        [[nodiscard]] FloatPoint overlayChildOffset(const Control&) const override;
        void paintChildren(PaintEvent&) override;
        void scrollChildIntoView(Control&, FloatRect) override;
    private:
        ExpanderHeader* m_heldHeader{};
        // The container holding the overlay entry that names the header - see holdHeader. Null
        // where none would take it, which leaves the header a plain child.
        Control* m_headerHost{};
    };

    // ExpanderHeader

    template<typename ...Args>
    ExpanderHeader::ExpanderHeader(const CreateParams& params, Args&&... args)
        :
        PanelBase{
            params,
            Padding{ 4.0f },
            Spacing{ 4.0f },
            // What the expander draws of itself, which is what places its text.
            textPlacementFor(READ_PROPERTY(ExpanderViewMode, ExpanderViewMode::Section)),
            expanderColorRules(UiElement::Header,
                // The same, for the colour rules the header paints from.
                READ_PROPERTY(ExpanderViewMode, ExpanderViewMode::Section)),
            std::forward<Args>(args)...
        },
        // What the expander draws of itself.
        m_viewMode{ READ_PROPERTY(ExpanderViewMode, ExpanderViewMode::Section) }
    {
        m_button.connectEvent<PaintIconEvent>(this, &ExpanderHeader::paintButton);
        m_button.onClick([this](ClickEvent&) { toggleExpanded(); });
        createDividerLine();
    }

    void ExpanderHeader::setExpanded(bool value)
    {
        if (m_expanded == value)
            return;
        // Read before the body goes: a header is held over its body, so once the body is hidden
        // nothing says how far the header had been carried.
        const float travel = floatOffset().y;
        m_expanded = value;
        float newValue = value ? 1.0f : 0.0f;
        animate(AnimationSlots::expander, m_expandedFactor, newValue,
            [this](AnimateParams& params) {
                m_expandedFactor = params.value;
                invalidateChevron();
            });
        ToggleExpandedEvent event{ m_expanded };
        emitEvent(event);
        if (m_expanded)
        {
            scrollSectionIntoView();
        }
        else if (travel != 0.0f)
        {
            // A held header goes home on collapse, and home is above the window: the strip under
            // the pointer would vanish and everything below it jump up. Scrolling back by the
            // travel puts home on the line the header was held on, so it stays where it was
            // clicked and only what is under it changes.
            scrollViewBy({ 0.0f, -travel });
        }
    }

    RoundedRectangleParts ExpanderHeader::silhouette(const PaintEvent& hostEvent, const FloatRect& headerRect) const
    {
        const float radius = hostEvent.scaleF(designMetrics().radius);
        return { .bounds = headerRect, .radii = { radius, radius, 0.0f, 0.0f } };
    }

    void ExpanderHeader::adjustPaint(AdjustPaintEvent& event)
    {
        PanelBase::adjustPaint(event);
        if (m_expanded)
        {
            event.setCornerRadius(Corner::BottomRight, 0.0f);
            event.setCornerRadius(Corner::BottomLeft, 0.0f);
        }
    }

    // The label is the header's in every look, and nothing else along the strip claims a double
    // click, so the whole header answers one. A double click landing on the button toggles twice
    // over - once from the click inside it, once from here - and lands back where it started,
    // which is what a double click on a chevron already means.
    void ExpanderHeader::doubleClick(DoubleClickEvent& event)
    {
        PanelBase::doubleClick(event);
        toggleExpanded();
    }

    // The divider look holds its label against the left edge, which is what frees the body slot
    // for the line. The other two leave the label in the body, where a panel puts it by default -
    // and a panel places its bars before its body, so a label in the body follows the chevron
    // that a tree node leads with. A label placed Left would precede it.
    TextPlacement ExpanderHeader::textPlacementFor(ExpanderViewMode viewMode)
    {
        if (viewMode == ExpanderViewMode::Divider)
            return TextPlacement::Left;
        return TextPlacement::Body;
    }

    // A bar field takes its control once, so the look is settled here and holds for the life of
    // the header. TreeNode leads the strip with the button; the other two put it at the far end,
    // past the label and past whatever the header carries between them.
    ExpanderButton& ExpanderHeader::createChevronButton()
    {
        ShowSurfaceAtRest showSurfaceAtRest = m_viewMode == ExpanderViewMode::Divider ?
            ShowSurfaceAtRest::Yes
            :
            ShowSurfaceAtRest::No;
        if (m_viewMode == ExpanderViewMode::TreeNode)
            return createLeftBar<ExpanderButton>(showSurfaceAtRest);
        return createRightBar<ExpanderButton>(showSurfaceAtRest);
    }

    // The line takes the body slot, stretched to what the label and the chevron leave, so it runs
    // between the two. Only the divider look carries one: a section's header is a filled strip,
    // and a tree node's label has that slot.
    void ExpanderHeader::createDividerLine()
    {
        if (m_viewMode != ExpanderViewMode::Divider)
            return;
        createBody<Controls::Divider>(VerticalAlign::Center);
    }

    void ExpanderHeader::paintButton(PaintIconEvent& event) const
    {
        Icons::ExpanderMark::paint(event, m_expandedFactor);
    }

    // A section is its header and its body together, so what is brought into view is the host
    // that holds both. The host shows its body in answer to ToggleExpandedEvent, and a control
    // that has just been shown still measures as it did while hidden, so the scroll waits for
    // the alignment rather than forcing one: asked now it would scroll to the rect the section
    // had while collapsed. Collapsing needs none of this - what is left of the section is where
    // it already was.
    void ExpanderHeader::scrollSectionIntoView() const
    {
        if (Control* host = parent())
            host->scrollIntoViewOnAlign();
    }

    // WithHeldHeader

    template<IsControl Base>
    void WithHeldHeader<Base>::holdHeader(ExpanderHeader& header)
    {
        m_heldHeader = &header;
        m_headerHost = this->parent();
        if (m_headerHost && m_headerHost->addOverlayControl(header, ClippingMode::Standard))
            return;
        m_headerHost = this->addOverlayControl(header, ClippingMode::Standard) ? this : nullptr;
    }

    template<IsControl Base>
    const Control* WithHeldHeader<Base>::heldHeader() const
    {
        if (!m_headerHost || !m_heldHeader->visible())
            return nullptr;
        const Control* body = headedBody();
        if (!body || !body->visible())
            return nullptr;
        return m_heldHeader;
    }

    template<IsControl Base>
    FloatPoint WithHeldHeader<Base>::overlayChildOffset(const Control& child) const
    {
        if (&child != heldHeader())
            return {};
        return this->heldHeaderOffset(child, headedBody()->bottom());
    }

    template<IsControl Base>
    void WithHeldHeader<Base>::paintChildren(PaintEvent& event)
    {
        if (!m_headerHost)
            return Base::paintChildren(event);
        for (ControlPtr& child : this->controls())
        {
            if (child.get() != m_heldHeader && child->visible())
                event.paintChild(*child);
        }
        if (!event.overlayStage() || event.overlayHost() != m_headerHost || !m_heldHeader->visible())
            return;
        const float travel = overlayChildOffset(*m_heldHeader).y;
        if (travel != 0.0f)
        {
            const FloatRect rect = FloatRect::fromDimensions(
                event.contentPosition() + m_heldHeader->topLeft() + FloatPoint{ 0.0f, travel },
                m_heldHeader->dimensions());
            event.paintHeldBackdrop(m_heldHeader->silhouette(event, rect), travel);
        }
        event.paintChild(*m_heldHeader);
    }

    template<IsControl Base>
    void WithHeldHeader<Base>::scrollChildIntoView(Control& control, FloatRect controlRect)
    {
        if (&control != m_heldHeader)
            controlRect.top -= this->heldHeaderStrip();
        Base::scrollChildIntoView(control, controlRect);
    }

}
