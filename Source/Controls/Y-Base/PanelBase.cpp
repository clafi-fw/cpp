module ClaFi.Controls.Base.PanelBase;

import ClaFi.Controls.Base.Container;
import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Splitter

    Splitter::Splitter(const CreateParams& params, const PanelSlot bar)
        :
        Control{ params },
        m_bar{ bar }
    {
    }

    CursorShape Splitter::cursor() const
    {
        return upright() ? CursorShape::SizeWE : CursorShape::SizeNS;
    }

    bool Splitter::upright() const
    {
        return PanelBase::isSideBar(m_bar);
    }

    void Splitter::adjustPaint(AdjustPaintEvent& event)
    {
        event.setColorRules(UiElement::Divider);
    }

    // The line is drawn in the middle of the band, which is laid out as the line plus the same
    // lead either side - so the lead is read back off the box rather than computed again.
    void Splitter::paintSurface(PaintEvent& event)
    {
        const float line = event.scaledStrokeWidth(k_thickness);
        const float lead = upright()
            ? event.unScale((width() - line) / 2.0f)
            : event.unScale((height() - line) / 2.0f);
        event.defaultPaintSurface(upright() ? FloatPoint{ lead, 0.0f } : FloatPoint{ 0.0f, lead });
    }

    ScaledDimensions Splitter::calculateContent(AlignEvent& event)
    {
        const float line = event.scaledStrokeWidth(k_thickness);
        return { line, line };
    }

    void Splitter::nestedPressDown(PressDownEvent& event)
    {
        event.stopPropagation();
        m_startExtent = panel().barExtent(m_bar);
    }

    // The bar follows the pointer by what it moved since the press, held inside what the bar's
    // floor and the body's leave it.
    void Splitter::nestedDrag(DragEvent& event)
    {
        const PointInForm startPos = event.startPos();
        const PointInForm currentPos = event.currentPos();
        float delta = upright() ? currentPos.x - startPos.x : currentPos.y - startPos.y;
        delta = scaler().unScale(delta);
        if (m_bar == PanelSlot::Right || m_bar == PanelSlot::Bottom)
            delta = -delta;
        const PanelBase::ExtentRange range = panel().barExtentRange(m_bar);
        panel().dragBarTo(m_bar, std::clamp(m_startExtent + delta, range.least, range.most));
        event.lockHoveredControl();
        event.stopPropagation();
    }

    // Stopping the event keeps the second press from starting a drag over the fit.
    void Splitter::nestedDoubleClick(DoubleClickEvent& event)
    {
        event.stopPropagation();
        panel().fitBarToContent(m_bar);
    }

    PanelBase& Splitter::panel() const
    {
        return *static_cast<PanelBase*>(parent());
    }

    // BarResizeEvent

    BarResizeEvent::BarResizeEvent(PanelBase& panel, const PanelSlot slot, const float extent)
        :
        panel{ panel },
        slot{ slot },
        extent{ extent }
    {
    }

    // PanelBase

    // The splitter is an overlay of its own panel: the band it is taken by reaches over the bar
    // and the body, so it has to be met before them and drawn after them.
    Splitter& PanelBase::createSplitter(const PanelSlot slotId)
    {
        if (slotId == PanelSlot::Body)
            throw std::logic_error{ "A splitter stands beside a bar, not the body" };
        if (!slot(slotId))
            throw std::logic_error{ "A splitter is created after the bar it stands beside" };
        BarSizing& sizing = barSizing(slotId);
        if (sizing.splitter)
            throw std::logic_error{ "The bar already has a splitter" };
        Splitter& result = add<Splitter>(slotId);
        sizing.splitter = &result;
        addOverlayControl(result, ClippingMode::Standard);
        result.setVisible(slot(slotId)->visible());
        return result;
    }

    float PanelBase::barExtent(const PanelSlot slotId) const
    {
        const Control* bar = slot(slotId);
        if (!bar)
            return 0.0f;
        return scaler().unScale(isSideBar(slotId) ? bar->width() : bar->height());
    }

    void PanelBase::setBarExtent(const PanelSlot slotId, const float extent)
    {
        BarSizing& sizing = barSizing(slotId);
        if (sizing.extent == extent && !sizing.fitAsked)
            return;
        sizing.extent = extent;
        sizing.fitAsked = false;
        invalidateFormAlign();
    }

    void PanelBase::resetBarExtent(const PanelSlot slotId)
    {
        BarSizing& sizing = barSizing(slotId);
        if (!sizing.extent && !sizing.fitAsked)
            return;
        sizing.extent.reset();
        sizing.fitAsked = false;
        invalidateFormAlign();
    }

    // The measurement is the next pass's to make - see keepFittedExtents, which reads it.
    void PanelBase::fitBarToContent(const PanelSlot slotId)
    {
        BarSizing& sizing = barSizing(slotId);
        sizing.extent.reset();
        sizing.fitAsked = true;
        invalidateFormAlign();
    }

    ScaledDimensions PanelBase::calculateContent(AlignEvent& event)
    {
        keepFittedExtents();
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
                lrcWidth += splitterGap(PanelSlot::Left, event.spacing.x);
            lrcWidth += m_bodyRect.right;
        }
        if (m_bodyRect.bottom)
            lrcHeight = std::max(lrcHeight, m_bodyRect.bottom);

        if (isNotNullAndVisible(rightBar()))
        {
            if (lrcWidth)
                lrcWidth += splitterGap(PanelSlot::Right, event.spacing.x);
            lrcWidth += rightBar()->width();
            lrcHeight = std::max(lrcHeight, rightBar()->height());
        }

        if (lrcHeight)
        {
            if (result.y)
                result.y += splitterGap(PanelSlot::Top, event.spacing.y);
            result.y += lrcHeight;
        }
        result.x = std::max(result.x, lrcWidth);

        if (isNotNullAndVisible(bottomBar()))
        {
            if (result.y)
                result.y += splitterGap(PanelSlot::Bottom, event.spacing.y);
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
        {
            const float gap = splitterGap(PanelSlot::Top, event.spacing.y);
            placeSplitter(PanelSlot::Top, event, m_bodyRect.top, gap);
            m_bodyRect.top += gap;
        }
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
            {
                const float gap = splitterGap(PanelSlot::Bottom, event.spacing.y);
                m_bodyRect.bottom -= gap;
                placeSplitter(PanelSlot::Bottom, event, m_bodyRect.bottom, gap);
            }
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
            {
                const float gap = splitterGap(PanelSlot::Left, event.spacing.x);
                placeSplitter(PanelSlot::Left, event, m_bodyRect.left, gap);
                m_bodyRect.left += gap;
            }
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
                const float gap = splitterGap(PanelSlot::Right, event.spacing.x);
                m_bodyRect.right -= gap;
                placeSplitter(PanelSlot::Right, event, m_bodyRect.right, gap);
            }
        }

        // The slot is settled here whether or not anything stands in it: its width takes no more
        // changes below, and the only one its height takes is the grow a body asks for.
        bodySlotSettled(event);
        if (m_textPlacement == TextPlacement::Body && wordWrap() && m_textSize.y > 0.0f)
            rememberTextWidth(event, m_bodyRect.width());

        if (isNotNullAndVisible(body()))
        {
            // A body coming out taller than the slot it was offered grows this panel to hold it -
            // unless the panel's own height is dictated from outside, in which case growing fights
            // whoever set it. Two separate things dictate a height, and either one is enough:
            //
            //   Fill            - a parent stretches this panel to the space it was given, and
            //                     the form's root control is stretched to the window this way.
            //                     The root has no parent to ask about a body slot at all.
            //   isHostedAsBody  - a host handed this panel exact dimensions for its body slot,
            //                     whatever this panel's own alignment says, and has already laid
            //                     the rest of itself out around the height it gave.
            //
            // So only a dictated panel lays its body out past the slot: a growing one would grow
            // by whatever bodyExtent states.
            bool sizeComesFromOutside = verticalAlign() == VerticalAlign::Fill || isHostedAsBody();
            float delta = body()->height();
            alignControl(body(), event, m_bodyRect.topLeft(),
                sizeComesFromOutside ? bodyExtent() : m_bodyRect.dimensions());
            delta = body()->height() - delta;

            bool needExpand = (delta > 0.0f) && !sizeComesFromOutside;
            if (needExpand)
            {
                m_contentRect.bottom += delta;
                m_bodyRect.bottom += delta;
                contentSize.y += delta;
                if (isNotNullAndVisible(bottomBar()))
                    offsetControl(bottomBar(), { 0.0f, delta });
                if (Splitter* splitter = shownSplitter(PanelSlot::Bottom))
                    offsetControl(splitter, { 0.0f, delta });
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

    // A stated extent is the bar's preference for this pass, and a fit asked for takes the
    // preference away so the content decides. The bar's own metrics are read and never written -
    // a setting is the caller's, and a preference is what the pointer may move.
    void PanelBase::adjustChildMetrics(AdjustMetricsEvent& event) const
    {
        Container::adjustChildMetrics(event);
        const std::optional<PanelSlot> slotId = slotOf(event.control);
        if (!slotId)
            return;
        const BarSizing& sizing = barSizing(*slotId);
        float& preference = isSideBar(*slotId)
            ? event.metrics.preferredSize.x
            : event.metrics.preferredSize.y;
        if (sizing.fitAsked)
            preference = 0.0f;
        else if (sizing.extent)
            preference = *sizing.extent;
    }

    // A splitter shows with its bar.
    void PanelBase::childVisibilityChanged(Control& child)
    {
        Container::childVisibilityChanged(child);
        const std::optional<PanelSlot> slotId = slotOf(child);
        if (!slotId)
            return;
        if (Splitter* splitter = barSizing(*slotId).splitter)
            splitter->setVisible(child.visible());
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

    std::optional<PanelSlot> PanelBase::slotOf(const Control& child) const
    {
        for (std::size_t i = 0; i < m_slots.size(); ++i)
        {
            if (m_slots[i] == &child)
                return static_cast<PanelSlot>(i);
        }
        return {};
    }

    Splitter* PanelBase::shownSplitter(const PanelSlot slotId)
    {
        Splitter* splitter = barSizing(slotId).splitter;
        if (!splitter || !isNotNullAndVisible(slot(slotId)))
            return nullptr;
        return splitter;
    }

    float PanelBase::splitterGap(const PanelSlot slotId, const float spacing)
    {
        const Splitter* splitter = shownSplitter(slotId);
        if (!splitter)
            return spacing;
        const float line = isSideBar(slotId) ? splitter->width() : splitter->height();
        return std::max(spacing, line);
    }

    // The line stands in the middle of the gap, and the band the pointer takes it by is centred
    // on the line, reaching over the bar on one side and the body on the other. The splitter is
    // read before it is laid out, so its box is still the line it measured.
    void PanelBase::placeSplitter(const PanelSlot slotId, AlignEvent& event, const float gapStart,
        const float gap)
    {
        Splitter* splitter = shownSplitter(slotId);
        if (!splitter)
            return;
        const bool upright = isSideBar(slotId);
        const float line = upright ? splitter->width() : splitter->height();
        const float band = std::max(event.scale(Splitter::k_grabWidth), line);
        const float lead = std::floor((band - line) / 2.0f);
        const float bandStart = gapStart + std::floor((gap - line) / 2.0f) - lead;
        const float bandWidth = line + lead * 2.0f;
        if (upright)
        {
            alignControl(splitter, event,
                { bandStart, m_bodyRect.top }, { bandWidth, m_bodyRect.height() });
        }
        else
        {
            alignControl(splitter, event,
                { m_contentRect.left, bandStart }, { m_contentRect.width(), bandWidth });
        }
    }

    // A bar is dragged no smaller than its floor, and no larger than leaves the body its own.
    PanelBase::ExtentRange PanelBase::barExtentRange(const PanelSlot slotId) const
    {
        const Control* bar = slot(slotId);
        if (!bar)
            return {};
        const bool upright = isSideBar(slotId);
        const ScaledDimensions barMin = bar->calculatedMinSize();
        const ScaledDimensions bodyMin = bodyMinSize();
        const float shown = upright ? bar->width() : bar->height();
        const float bodyRoom = upright
            ? m_bodyRect.width() - bodyMin.x
            : m_bodyRect.height() - bodyMin.y;
        ExtentRange result = {
            scaler().unScale(upright ? barMin.x : barMin.y),
            scaler().unScale(shown + std::max(bodyRoom, 0.0f)),
        };
        result.most = std::max(result.most, result.least);
        return result;
    }

    void PanelBase::dragBarTo(const PanelSlot slotId, const float extent)
    {
        if (barSizing(slotId).extent == extent)
            return;
        setBarExtent(slotId, extent);
        BarResizeEvent event{ *this, slotId, extent };
        emitEvent(event);
    }

    // Run as the children stand measured and before anything is laid out, so the measurement
    // read is the one this pass goes on to use.
    void PanelBase::keepFittedExtents()
    {
        for (std::size_t i = 0; i < m_barSizing.size(); ++i)
        {
            BarSizing& sizing = m_barSizing[i];
            if (!sizing.fitAsked)
                continue;
            sizing.fitAsked = false;
            const PanelSlot slotId = static_cast<PanelSlot>(i);
            sizing.extent = barExtent(slotId);
            BarResizeEvent event{ *this, slotId, *sizing.extent };
            emitEvent(event);
        }
    }

}
