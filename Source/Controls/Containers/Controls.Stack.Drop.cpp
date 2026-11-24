module ClaFi.Controls.Stack;

import ClaFi.Controls.Base.StackBase;
import ClaFi.Icons.ErrorIcon;
import ClaFi.Icons.RowIcons;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        void drawDropAction(DrawIconEvent& event)
        {
            const Icons::RowIcons::Drawing drawing{ event };
            Graphics::PixelPath destination;
            if (event.tag() == DropZone::Inside)
            {
                destination.moveTo(4.0f, 4.5f);
                destination.lineTo(4.0f, 1.5f);
                destination.lineTo(12.5f, 1.5f);
                destination.lineTo(12.5f, 12.5f);
                destination.lineTo(4.0f, 12.5f);
                destination.lineTo(4.0f, 9.5f);
            }
            else
            {
                Icons::RowIcons::addLine(destination, { 4.0f, 2.0f }, { 12.5f, 2.0f });
                Icons::RowIcons::addLine(destination, { 4.0f, 12.0f }, { 12.5f, 12.0f });
            }
            drawing.stroke(destination, drawing.inks.body);

            Graphics::PixelPath arrow;
            Icons::RowIcons::addLine(arrow, { 0.75f, 7.0f }, { 9.0f, 7.0f });
            arrow.moveTo(6.5f, 4.5f);
            arrow.lineTo(9.0f, 7.0f);
            arrow.lineTo(6.5f, 9.5f);
            drawing.stroke(arrow, drawing.inks.accent);
        }
    }

    ItemsDropEvent::ItemsDropEvent(const std::span<Control* const> items, Control& target,
        const DropZone zone)
        :
        items{ items },
        target{ target },
        zone{ zone }
    {
    }

    void StackViewBase::nestedDrag(DragEvent& event)
    {
        if (!m_zoneDrag)
        {
            StackPanelBase::nestedDrag(event);
            return;
        }
        StackBase::nestedDrag(event);
        if (event.hoveredControlLocked())
        {
            return;
        }
        if (!m_zoneDrag->pressedControl)
        {
            if (!event.pastThreshold())
            {
                return;
            }
            Control* source = dropItemAt(&event.control());
            if (!source || !canMoveItem(*source))
            {
                return;
            }
            std::vector<Control*> items = carriedItems(*source);
            if (items.empty())
            {
                return;
            }
            m_zoneDrag->pressedControl = &event.control();
            m_zoneDrag->items = std::move(items);
            m_zoneDrag->canceled = false;
            const Graphics::IBackend* backend = form().context().canvas().backend();
            IntRect rect = source->visibleRectInForm().rounded();
            Graphics::Bitmap snapshot;
            if (backend && rect.intersectWith(IntRect::FromDimensions({}, backend->size())))
            {
                snapshot = backend->capturePixels(rect);
            }
            updateDropTarget();
            form().tooltip().showDragPreview(*this, std::move(snapshot),
                event.startPos() - rect.toFloat().topLeft());
        }
        else if (!m_zoneDrag->canceled)
        {
            updateDropTarget();
            form().tooltip().showRightNow(*this);
        }
        Platform::setCursor(CursorShape::Arrow);
        event.lockHoveredControl();
    }

    void StackViewBase::nestedPressUp(PressUpEvent& event)
    {
        StackPanelBase::nestedPressUp(event);
        if (!m_zoneDrag || !m_zoneDrag->pressedControl)
        {
            return;
        }
        event.handled.preventClick = true;
        event.scrollIntoView = false;
        if (!m_zoneDrag->canceled)
        {
            updateDropTarget();
        }
        Control* target = m_zoneDrag->target;
        const DropZone zone = m_zoneDrag->zone;
        std::vector<Control*> items = std::move(m_zoneDrag->items);
        clearDropTarget();
        hideDropTooltip();
        m_zoneDrag->pressedControl = nullptr;
        if (target)
        {
            ItemsDropEvent dropped{ items, *target, zone };
            emitEvent(dropped);
        }
    }

    void StackViewBase::nestedKeyDown(KeyDownEvent& event)
    {
        if (m_zoneDrag && m_zoneDrag->pressedControl && event.key == Keys::Escape)
        {
            clearDropTarget();
            hideDropTooltip();
            m_zoneDrag->canceled = true;
            Platform::setCursor(CursorShape::Arrow);
            event.handled = true;
            return;
        }
        StackPanelBase::nestedKeyDown(event);
    }

    void StackViewBase::paintOverlay(PaintEvent& event)
    {
        if (!m_zoneDrag || !m_zoneDrag->target)
        {
            return;
        }
        const Control& target = *m_zoneDrag->target;
        const FloatRect row = target.boundsInForm();
        const Color accent = event.accentRgb(InkGrade::Strong);
        const float stroke = std::max(1.0f, event.scale(1.0f));
        if (m_zoneDrag->zone == DropZone::Inside)
        {
            event.canvas().fillRectangle(row, accent.withOpacity(0.18f));
            event.canvas().drawRectangle(row, accent, stroke);
        }
        else
        {
            const float radius = event.scale(2.5f);
            const float y = m_zoneDrag->zone == DropZone::Before
                ? row.top + stroke : row.bottom - stroke;
            const FloatPoint start = { row.left + std::max(radius, dropLineInset(target)), y };
            event.canvas().drawLine(start, { row.right, y }, accent, stroke * 2.0f);
            event.canvas().fillCircle(start, radius, accent);
        }
    }

    void StackViewBase::nestedAdjustTooltip(AdjustTooltipEvent& event)
    {
        if (!m_zoneDrag || !m_zoneDrag->pressedControl || m_zoneDrag->canceled || !Input::isMouseDown())
        {
            StackPanelBase::nestedAdjustTooltip(event);
            return;
        }
        static const Text rejected{ InTextIcon{ Icons::RowIcons::Size, Icons::ErrorIcon::draw } };
        if (m_zoneDrag->target)
        {
            static const Text moveHere{ L"Move here" };
            event.text = Text{
                InTextIcon{ Icons::RowIcons::Size, drawDropAction, Tag{ m_zoneDrag->zone } },
                Space{ Icons::RowIcons::Gap }
            };
            event.text << (m_zoneDrag->hint.empty() ? moveHere : m_zoneDrag->hint);
        }
        else
        {
            event.text = rejected;
        }
        event.placement = FormPlacement::Mouse;
        event.anchorRect = FloatRect::FromDimensions(form().mousePos(), {});
        event.hideOnUserInput = false;
    }

    Control* StackViewBase::groupEndDropTarget(Control& row, std::span<Control* const>)
    {
        return nextVisibleSiblingOf(row) ? nullptr : &row;
    }

    void StackViewBase::enableZoneDrop()
    {
        m_zoneDrag = std::make_unique<ZoneDrag>();
        m_zoneDrag->hoverTimer.onTick([this](TimerEvent&) {
            onDropHoverTimer();
        });
    }

    Control* StackViewBase::dropItemAt(Control* hit)
    {
        if (!hit || !containsNested(hit, CheckSelf::No))
        {
            return nullptr;
        }
        if (focusScope() == FocusScope::Yes)
        {
            return itemAt(hit);
        }
        while (hit->parent() != this)
        {
            hit = hit->parent();
        }
        return hit;
    }

    void StackViewBase::updateDropTarget()
    {
        ZoneDrag& drag = *m_zoneDrag;
        const PointInForm point = form().mousePos();
        Control* target = dropItemAt(form().controlUnderPointer());
        if (!target || !target->visibleRectInForm().contains(point)
            || std::ranges::find(drag.items, target) != drag.items.end())
        {
            clearDropTarget();
            return;
        }
        const FloatRect row = target->boundsInForm();
        DropZone zone = DropZone::Inside;
        if (point.y < row.top + row.height() * 0.30f)
        {
            zone = DropZone::Before;
        }
        else if (point.y >= row.bottom - row.height() * 0.15f)
        {
            if (Control* end = groupEndDropTarget(*target, drag.items))
            {
                if (std::ranges::find(drag.items, end) != drag.items.end())
                {
                    clearDropTarget();
                    return;
                }
                zone = end == target ? DropZone::After : DropZone::Before;
                target = end;
            }
        }
        CanDropItemsEvent query{ drag.items, *target, zone };
        emitEvent(query);
        if (!query.allowed)
        {
            clearDropTarget();
            return;
        }
        drag.hint = std::move(query.hint);
        if (target == drag.target && zone == drag.zone)
        {
            return;
        }
        drag.hoverTimer.stop();
        if (zone == DropZone::Inside)
        {
            drag.hoverTimer.start(MilliSeconds{ 500u });
        }
        drag.target = target;
        drag.zone = zone;
        invalidate();
    }

    void StackViewBase::clearDropTarget()
    {
        m_zoneDrag->hoverTimer.stop();
        if (m_zoneDrag->target)
        {
            m_zoneDrag->target = nullptr;
            m_zoneDrag->hint.clear();
            invalidate();
        }
    }

    void StackViewBase::hideDropTooltip()
    {
        if (Tooltip::control() == this)
        {
            Tooltip::stopAndHide();
        }
    }

    void StackViewBase::onDropHoverTimer()
    {
        if (m_zoneDrag && Input::isMouseDown())
        {
            Control* target = m_zoneDrag->target;
            updateDropTarget();
            if (target && m_zoneDrag->target == target && m_zoneDrag->zone == DropZone::Inside)
            {
                expandDropTarget(*target);
            }
            form().mouseTick();
        }
    }
}
