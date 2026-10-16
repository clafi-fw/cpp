module;
#include "../Y-Core/System/Switches.h"

module ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        // Whether a link is written anywhere in the text, so that a label without one costs the
        // pointer no layout lookup.
        [[nodiscard]] bool hasLink(const Text& text)
        {
            for (const auto& marker : text.markers())
            {
                if (std::holds_alternative<PushLink>(marker.second))
                    return true;
            }
            return false;
        }
    }

    // HeaderLinkClickEvent

    HeaderLinkClickEvent::HeaderLinkClickEvent(ExpanderHeader& header, std::wstring target,
        InputStamp stamp)
        :
        EventOf<ExpanderHeader>{ header },
        target{ std::move(target) },
        stamp{ stamp }
    {
    }

    // HeaderLinkContextPopupEvent

    HeaderLinkContextPopupEvent::HeaderLinkContextPopupEvent(ExpanderHeader& header,
        std::wstring target, ContextPopupEvent& popup)
        :
        EventOf<ExpanderHeader>{ header },
        target{ std::move(target) },
        popup{ popup }
    {
    }

    // ExpanderHeader

    CursorShape ExpanderHeader::cursor() const
    {
        return m_hoveredLink.length != 0 ? CursorShape::Hand : PanelBase::cursor();
    }

    void ExpanderHeader::mouseMove(MouseMoveEvent& event)
    {
        PanelBase::mouseMove(event);
        const std::optional<LabelLink> link = linkAt(event.posOnForm);
        hoverLink(link.has_value() ? link->range : TextRange{});
    }

    void ExpanderHeader::hoverLeave()
    {
        PanelBase::hoverLeave();
        hoverLink({});
    }

    // Only a mouse click lands on a link - a key names no point in the label, and Return answers
    // for the keyboard in nestedKeyDown. **A click from a key toggles the section**, as a
    // mouse click on the chevron does; a mouse click on the strip leaves that to the chevron and
    // to a double click. A tree node's click is its row's pick from either, and toggles nothing.
    void ExpanderHeader::click(ClickEvent& event)
    {
        if (Input::mouse().active())
        {
            std::optional<LabelLink> link = linkAt(event.clickPos());
            if (link.has_value())
            {
                event.stopPropagation();
                HeaderLinkClickEvent linkEvent{ *this, std::move(link->target), event.stamp };
                emitEvent(linkEvent);
                return;
            }
        }
        else if (m_viewMode != ExpanderViewMode::TreeNode)
        {
            toggleExpanded();
        }
        PanelBase::click(event);
    }

    // **A link is what the menu is about** where one stands under the pointer or, from the
    // keyboard, where the focus is on the header - the label's first link then. A handler that
    // has shown the link's menu stops the request; one that has not leaves it to go on up as from
    // anywhere else on the header.
    void ExpanderHeader::nestedContextPopup(ContextPopupEvent& event)
    {
        std::optional<std::wstring> target;
        if (event.mousePos)
        {
            std::optional<LabelLink> link = linkAt(*event.mousePos);
            if (link.has_value())
                target = std::move(link->target);
        }
        else if (event.control == this)
        {
            target = firstLink();
        }
        if (target.has_value())
        {
            HeaderLinkContextPopupEvent linkEvent{ *this, std::move(*target), event };
            emitEvent(linkEvent);
            if (event.propagationStopped())
                return;
        }
        PanelBase::nestedContextPopup(event);
    }

    // **Return follows the label's first link** where the label has one, as Return on a grid cell
    // follows the cell's, and is the header's own click where it has none. Handled before the
    // follow, which may take down the page the header stands on.
    //
    // **Right opens the header and Left closes it**, as on a tree node, and the header keeps both
    // keys when there is nothing to open or close. A tree node leaves those to the tree, where
    // Left climbs to the node above and Right steps into the rows.
    void ExpanderHeader::nestedKeyDown(KeyDownEvent& event)
    {
        if (event.key == Keys::Return)
        {
            std::optional<std::wstring> target = firstLink();
            if (target.has_value())
            {
                event.handled = true;
                HeaderLinkClickEvent linkEvent{ *this, std::move(*target), event.stamp };
                emitEvent(linkEvent);
                return;
            }
        }
        const bool arrow = event.key == Keys::Left || event.key == Keys::Right;
        if (arrow && event.modifiers.empty())
        {
            const bool opens = event.key == Keys::Right;
            const bool acts = opens != m_expanded;
            if (acts)
                setExpanded(opens);
            if (acts || m_viewMode != ExpanderViewMode::TreeNode)
            {
                event.handled = true;
                return;
            }
        }
        PanelBase::nestedKeyDown(event);
    }

    DrawTextResult ExpanderHeader::drawText(PaintEvent& event, const FloatRect& textBounds,
        const Text& text)
    {
        return textEngine().drawText(
            event.controlContext(),
            textBounds,
            text,
            textAnchor(),
            editProps(),
#if CLAFI_TEXT_MOVING
            textRenderMode(),
#endif
            wordWrap(),
            textFormat(),
            m_hoveredLink
        );
    }

    // Asked the way paintText draws the label: the same box, and the point measured from where
    // the anchor put the block in it.
    std::optional<ExpanderHeader::LabelLink> ExpanderHeader::linkAt(const PointInForm point) const
    {
        const FormContext& context = formContext();
        Text buffer;
        const Text& label = doGetText(context, buffer, EventPhase::Paint);
        if (!hasLink(label))
            return std::nullopt;
        const FloatRect bounds = textBounds(context, boundsInForm());
        if (bounds.empty())
            return std::nullopt;
        const ScaledSize drawn = textEngine().calculateText(context, label, bounds.dimensions(),
            false, wordWrap(), textFormat());
        const FloatPoint origin = anchoredOrigin(bounds, drawn, textAnchor());
        const std::optional<LinkHit> hit = textEngine().linkAt(context, label,
            bounds.dimensions(), point - origin, wordWrap(), textFormat());
        if (!hit.has_value())
            return std::nullopt;
        return LabelLink{ std::wstring{ hit->link.value }, hit->link.range };
    }

    std::optional<std::wstring> ExpanderHeader::firstLink() const
    {
        Text buffer;
        const Text& label = doGetText(formContext(), buffer, EventPhase::Paint);
        for (const auto& marker : label.markers())
        {
            if (const PushLink* link = std::get_if<PushLink>(&marker.second))
                return link->target;
        }
        return std::nullopt;
    }

    void ExpanderHeader::hoverLink(const TextRange range)
    {
        if (range == m_hoveredLink)
            return;
        m_hoveredLink = range;
        invalidate();
    }
}
