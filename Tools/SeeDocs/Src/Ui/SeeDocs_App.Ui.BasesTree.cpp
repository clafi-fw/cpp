module SeeDocs_App.Ui.BasesTree;

import SeeDocs_App.Ui.Icons;
import SeeDocs_App.Ui.PageText;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Base.Container;
import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.Menu;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App::Ui
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        // What a base's list says while no type the surface has stands on the base.
        constexpr std::wstring_view k_noDerived = L"No derived types";
        // After a crumb whose bases are left out.
        constexpr std::wstring_view k_ellipsis = L"\u2026";

        // The ellipsis as a collapsed expander draws its own.
        [[nodiscard]] Text ellipsisText()
        {
            return Text{ InkGrade::Muted, k_ellipsis };
        }

        // The page a crumb names: the link its words carry; empty for a base with no page.
        [[nodiscard]] std::wstring_view targetOf(const BaseNode& node)
        {
            for (const Run& run : node.text)
            {
                if (!run.link.empty())
                    return run.link;
            }
            return {};
        }

        // A line's centre where a stroke that wide covers whole pixels.
        [[nodiscard]] float pixelCentre(const float value, const float strokeWidth)
        {
            const bool oddWidth = static_cast<int>(strokeWidth) % 2 == 1;
            return std::floor(value) + (oddWidth ? 0.5f : 0.0f);
        }

        // From a radius above the corner, the turn right into a line ending at endX.
        void turnInto(Graphics::PixelPath& path, const FloatPoint corner, const float endX,
            const float radius)
        {
            path.quadTo(corner, { corner.x + radius, corner.y });
            path.hLineTo(endX);
        }
    }

    // BaseLinkEvent

    BaseLinkEvent::BaseLinkEvent(std::wstring linkTarget)
        :
        ::ClaFi::Event{},
        target{ std::move(linkTarget) }
    {
    }

    // BaseLinkContextPopupEvent

    BaseLinkContextPopupEvent::BaseLinkContextPopupEvent(Control& owner, std::wstring_view linkTarget,
        ContextPopupEvent& request)
        :
        ::ClaFi::Event{},
        crumb{ owner },
        target{ std::move(linkTarget) },
        popup{ request }
    {
    }

    // BaseCrumb

    BaseCrumb::BaseCrumb(const CreateParams& params, const BaseNode& node,
        const std::optional<Color> nameColor)
        :
        DropdownControlBase{
            params,
            NavigationReach::Yes,
            MouseFocus::Self,
            ClickTarget::Self,
            SecondaryPartPlacement::Left,
            UiElement::Button,
            ShowSelectionOnSurface::Yes,
            params.themeMetrics().button,
            //Padding{ 4.0f },
            // The bar narrows its crumbs itself, so only the height is held: a button's.
            MinSize{ 0.0f, params.themeMetrics().button.minSize.y },
            // One line, cut short where the bar narrows it.
            WordWrap::No,
            // Level with the middle of the run it stands in, a column of bases beside it included.
            VerticalAlign::Center
        },
        m_node{ node },
        m_nameColor{ nameColor }
    {
    }

    // The strip stands at the crumb's edge, inside the child inset alone.
    float BaseCrumb::markCentreX(const FormContext& formContext) const
    {
        const Control& strip = *secondaryPart();
        return childInset(formContext, scaledPadding()).x + strip.scaledWidth() / 2.0f;
    }

    void BaseCrumb::adjustMainText(AdjustTextEvent& event) const
    {
        if (m_nameColor)
            writeRuns(event.text, m_node.text, *m_nameColor);
        else
            writeRuns(event.text, m_node.text);
    }

    // The types standing directly on the base, the one the way up came through marked and entered
    // on, so the keys start from where the bar is - as a path crumb's list marks the page the path
    // goes on to.
    void BaseCrumb::showDropdown(Control& initiator)
    {
        Menu menu{ initiator };
        menu.setPlaceHolderText(Text{ k_noDerived });
        for (const Type* type : m_node.derived)
        {
            const RowIcon icon = rowIconOf(*type);
            MenuItem& line = menu.add(
                Text{ type->name },
                OnDrawIcon{ [icon](DrawIconEvent& event) {
                    drawRowIcon(event, icon);
                } },
                OnClick{ [this, type](ClickEvent&) {
                    BaseLinkEvent linkEvent{ typeLink(*type) };
                    emitEvent(linkEvent);
                } }
            );
            if (type != m_node.from)
                continue;
            line.setShowSelectionOnSurface(ShowSelectionOnSurface::Yes);
            line.onGetState([](GetStateEvent& event) {
                event.state.selected = true;
            });
            line.makeCurrentItem();
            // Once the list has its window: the pass that measures it has no scroll range to spend
            // a scroll on - see BreadCrumbBarItem::showDropdown.
            menu.onAligned([&line, scrolled = false](FormAlignedEvent&) mutable {
                if (scrolled)
                    return;
                scrolled = true;
                line.scrollIntoView();
            });
        }
        // The list hangs from the mark that drops it, ending where the mark does - a path crumb's
        // list mirrored.
        menu.executeUnder(*secondaryPart(), FormPlacement::BottomRight);
    }

    void BaseCrumb::adjustPaint(AdjustPaintEvent& event)
    {
        DropdownControlBase::adjustPaint(event);
    }

    // A click on the crumb's face opens the page of the base it names. A click on the strip is a
    // nested one, and the base drops the list from it.
    void BaseCrumb::click(ClickEvent& event)
    {
        DropdownControlBase::click(event);
        const std::wstring_view target = targetOf(m_node);
        if (target.empty())
            return;
        BaseLinkEvent linkEvent{ std::wstring{ target } };
        emitEvent(linkEvent);
    }

    DropdownMarkTurn BaseCrumb::dropdownMarkTurn() const
    {
        return { ChevronTurn::Left, ChevronTurn::Down };
    }


    // BasesTree

    void BasesTree::setBases(const BaseNodes& value)
    {
        clear();
        m_bases = value;
        addCrumbs(m_bases, nullptr);
    }

    void BasesTree::clear()
    {
        m_bases = {};
        m_crumbsMap.clear();
        m_parentCrumbs.clear();
        m_links.clear();
        m_branches.clear();
        m_trunks.clear();
        m_ellipses.clear();
        m_colorCodes.clear();
        while (!controls().empty())
            controls().back()->deleteSelf();
    }

    ScaledSize BasesTree::calculateContent(AlignEvent& event)
    {
        return placeCrumbs(event, k_maxFloat);
    }

    // The measuring pass placed the crumbs against no wall. They stand where they are when that
    // fits the room given, and are placed again against its end when it does not. Either way
    // every crumb is then aligned where it stands - the pass that places its strip.
    void BasesTree::alignContent(AlignEvent& event, ScaledPosition, ScaledSize& newSize)
    {
        const ScaledSize measuredSize = scaledSize() - event.scaledPadding() * 2.0f;
        if (measuredSize.x > newSize.x)
            newSize = placeCrumbs(event, newSize.x);
        for (ControlPtr& crumb : controls())
            alignControl(crumb.get(), event, crumb->topLeft(), crumb->scaledSize());
    }

    // The lines go under the crumbs, which are painted after the surface.
    void BasesTree::paintSurface(PaintEvent& event)
    {
        ContainerBase::paintSurface(event);
        if (m_trunks.empty())
            return;
        const FloatPoint origin = event.contentPosition();
        const Color ink = event.textRgb(InkGrade::Subtle);
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const auto snapped = [origin, strokeWidth](const FloatPoint point) {
            return FloatPoint{
                pixelCentre(origin.x + point.x, strokeWidth),
                pixelCentre(origin.y + point.y, strokeWidth),
            };
        };
        Graphics::Canvas& canvas = event.canvas();
        for (const Line& link : m_links)
            canvas.drawLine(snapped(link.from), snapped(link.to), ink, strokeWidth);
        for (const Line& branch : m_branches)
        {
            const FloatPoint corner = snapped(branch.from);
            Graphics::PixelPath path;
            path.moveTo({ corner.x, corner.y - m_cornerRadius });
            turnInto(path, corner, snapped(branch.to).x, m_cornerRadius);
            canvas.drawPath(path, ink, strokeWidth);
        }
        for (const Trunk& trunk : m_trunks)
        {
            const FloatPoint top = snapped(trunk.top);
            const FloatPoint end = snapped({ trunk.endX, trunk.bottomY });
            Graphics::PixelPath path;
            path.moveTo(top);
            path.vLineTo(end.y - m_cornerRadius);
            turnInto(path, { top.x, end.y }, end.x, m_cornerRadius);
            canvas.drawPath(path, ink, strokeWidth);
        }
    }

    void BasesTree::paintOwnText(PaintEvent& event)
    {
        ContainerBase::paintOwnText(event);
        if (m_ellipses.empty())
            return;
        const FloatPoint origin = event.contentPosition();
        const Text ellipsis = ellipsisText();
        for (const FloatRect& place : m_ellipses)
        {
            FloatRect bounds = place;
            bounds.offset(origin);
            drawText(event, bounds, ellipsis);
        }
    }

    // The lines are what Right and Left follow, as a TreeView's do, so a crumb the rows packed
    // beside another is reached from it by Up and Down only.
    Control* BasesTree::navigationNeighbour(const Control& child, const NavigationStep& step)
    {
        const BaseCrumb& crumb = static_cast<const BaseCrumb&>(child);
        if (step.direction == NavigationDirection::Right)
        {
            const BaseNodes& bases = crumb.node().bases;
            return bases.empty() ? nullptr : &crumbOf(bases.front());
        }
        if (step.direction == NavigationDirection::Left)
        {
            const ParentCrumbsMap::const_iterator found = m_parentCrumbs.find(&crumb);
            return found == m_parentCrumbs.end() ? nullptr : found->second;
        }
        return crumbNearLine(crumb.topLeft().y, step.forward(), lineAmongCrumbs(step).x);
    }

    Control* BasesTree::navigationEntry(const NavigationStep& step)
    {
        if (!step.vertical())
            return ContainerBase::navigationEntry(step);
        const bool downward = step.forward();
        const float beforeNearRowY = downward ? -k_maxFloat : k_maxFloat;
        return crumbNearLine(beforeNearRowY, downward, lineAmongCrumbs(step).x);
    }

    ScaledSize BasesTree::placeCrumbs(AlignEvent& event, const float wallX)
    {
        m_links.clear();
        m_branches.clear();
        m_trunks.clear();
        m_ellipses.clear();
        if (m_bases.empty())
            return {};
        Placing placing = {
            .wallX = wallX,
            .spacing = event.scaledSpacing(),
            .ellipsisSize = measureText(event, { k_maxFloat, k_maxFloat }, ellipsisText()),
        };
        for (const CrumbsMap::value_type& entry : m_crumbsMap)
            placing.rowHeight = std::max(placing.rowHeight, entry.second->scaledHeight());
        placing.rowPitch = placing.rowHeight + placing.spacing.y;
        // Every crumb wears the same strip, so one says where a base stacked under a crumb starts.
        const BaseCrumb& firstCrumb = crumbOf(m_bases.front());
        placing.indent = firstCrumb.markCentreX(event.formContext()) + placing.spacing.x;
        placing.clearance = placing.spacing.x / 2.0f;
        m_cornerRadius = placing.spacing.x / 2.0f;
        placeDirectBases(event, placing);
        return {
            placing.rightX,
            static_cast<float>(placing.lastRow) * placing.rowPitch + placing.rowHeight,
        };
    }

    // The direct bases hang from the title's icon: their trunk runs under its centre, in the
    // padding the head gives the tree, from the tree's top edge.
    void BasesTree::placeDirectBases(AlignEvent& event, Placing& placing)
    {
        const float trunkX = event.scale(k_titleIconSize / 2.0f) - event.scaledPadding().x;
        const float topY = -event.scaledPadding().y;
        Lines stubs;
        int previousRow = -1;
        for (const BaseNode& node : m_bases)
        {
            const int row = freeRow(placing, crumbRoom(placing, node), 0.0f, trunkX, previousRow);
            put(placing, node, row, 0.0f);
            addStub(placing, stubs, trunkX, row, 0.0f);
            previousRow = row;
            placeBases(event, placing, node);
        }
        addTrunk(placing, stubs, trunkX, topY, 0, previousRow);
    }

    // A crumb's first base continues its row where the way up fits before the wall and nothing
    // stands there. Every other base stands under the crumb, on the first row with room for it
    // below the base before it, so the bases keep their order while the rows pack.
    void BasesTree::placeBases(AlignEvent& event, Placing& placing, const BaseNode& parent)
    {
        const BaseCrumb& parentCrumb = crumbOf(parent);
        const int parentRow = rowOf(placing, parentCrumb);
        const float parentRightX = parentCrumb.topLeft().x + parentCrumb.scaledWidth();
        const float trunkX = parentCrumb.topLeft().x + parentCrumb.markCentreX(event.formContext());
        const float x = trunkX + placing.spacing.x;
        Lines stubs;
        int previousRow = parentRow;
        bool first = true;
        for (const BaseNode& base : parent.bases)
        {
            if (first && fitsOnRow(placing, parentCrumb, base))
            {
                put(placing, base, parentRow, parentRightX + placing.spacing.x);
                addLink(placing, parentRightX, crumbOf(base).topLeft().x, parentRow);
            }
            else
            {
                const int row = freeRow(placing, crumbRoom(placing, base), x, trunkX, previousRow);
                put(placing, base, row, x);
                addStub(placing, stubs, trunkX, row, x);
                previousRow = row;
            }
            first = false;
            placeBases(event, placing, base);
        }
        if (stubs.empty())
            return;
        const float parentBottomY = parentCrumb.topLeft().y + parentCrumb.scaledHeight();
        addTrunk(placing, stubs, trunkX, parentBottomY, parentRow + 1, previousRow);
    }

    bool BasesTree::fitsOnRow(const Placing& placing, const BaseCrumb& parent,
        const BaseNode& base) const
    {
        const float parentRightX = parent.topLeft().x + parent.scaledWidth();
        const float x = parentRightX + placing.spacing.x;
        if (x + requiredWidth(placing, base) > placing.wallX)
            return false;
        const float rightX = x + crumbRoom(placing, base) + placing.clearance;
        return rowFree(placing, rowOf(placing, parent), { parentRightX, rightX });
    }

    // The first row below the previous one where the crumb, its clearance and the stub reaching
    // it from the trunk meet nothing.
    int BasesTree::freeRow(const Placing& placing, const float width, const float x,
        const float trunkX, const int previousRow) const
    {
        const Span span = {
            std::min(trunkX, x - placing.clearance),
            x + width + placing.clearance,
        };
        int row = previousRow + 1;
        while (!rowFree(placing, row, span))
            ++row;
        return row;
    }

    // The least room a base and everything over it take: every base stacked under its crumb, a
    // row continued being wider. A crumb placed with this much room before the wall can place
    // everything over it, since each base stacked under it has this much less to place in.
    float BasesTree::requiredWidth(const Placing& placing, const BaseNode& base) const
    {
        float innerWidth = 0.0f;
        for (const BaseNode& node : base.bases)
            innerWidth = std::max(innerWidth, requiredWidth(placing, node));
        const float width = crumbRoom(placing, base);
        if (!innerWidth)
            return width;
        return std::max(width, placing.indent + innerWidth);
    }

    float BasesTree::crumbRoom(const Placing& placing, const BaseNode& base) const
    {
        const float width = crumbOf(base).scaledWidth();
        if (!base.basesLeftOut)
            return width;
        return width + placing.spacing.x + placing.ellipsisSize.x;
    }

    // The ellipsis stands a spacing after the crumb, level with the middle of its row.
    void BasesTree::put(Placing& placing, const BaseNode& base, const int row, const float x)
    {
        const float y = static_cast<float>(row) * placing.rowPitch;
        setControlScaledPosition(crumbOf(base), { x, y });
        const float rightX = x + crumbRoom(placing, base);
        takeSpan(placing, row, { x, rightX });
        placing.rightX = std::max(placing.rightX, rightX);
        placing.lastRow = std::max(placing.lastRow, row);
        if (!base.basesLeftOut)
            return;
        const ScaledSize size = placing.ellipsisSize;
        m_ellipses.push_back(FloatRect::FromDimensions(
            { rightX - size.x, y + (placing.rowHeight - size.y) / 2.0f }, size));
    }

    void BasesTree::addLink(Placing& placing, const float fromX, const float toX, const int row)
    {
        const float y = rowCentreY(placing, row);
        m_links.push_back({ { fromX, y }, { toX, y } });
        takeSpan(placing, row, { fromX, toX });
    }

    void BasesTree::addStub(Placing& placing, Lines& stubs, const float trunkX, const int row,
        const float x)
    {
        const float y = rowCentreY(placing, row);
        stubs.push_back({ { trunkX, y }, { x, y } });
        takeSpan(placing, row, { trunkX, x });
    }

    // The stubs but the last branch off the trunk; the last is where it turns.
    void BasesTree::addTrunk(Placing& placing, const Lines& stubs, const float trunkX,
        const float topY, const int fromRow, const int toRow)
    {
        for (int row = fromRow; row <= toRow; ++row)
            takeSpan(placing, row, { trunkX, trunkX });
        const Line& last = stubs.back();
        m_trunks.push_back({ .top = { trunkX, topY }, .bottomY = last.from.y, .endX = last.to.x });
        m_branches.insert(m_branches.end(), stubs.begin(), stubs.end() - 1);
    }

    // Touching is not meeting: a link starts where its crumb ends, and a crumb's clearance may
    // end on a line.
    bool BasesTree::rowFree(const Placing& placing, const int row, const Span span)
    {
        const std::size_t index = static_cast<std::size_t>(row);
        if (index >= placing.rows.size())
            return true;
        for (const Span& taken : placing.rows[index])
        {
            if (span.right > taken.left && span.left < taken.right)
                return false;
        }
        return true;
    }

    void BasesTree::takeSpan(Placing& placing, const int row, const Span span)
    {
        const std::size_t index = static_cast<std::size_t>(row);
        if (index >= placing.rows.size())
            placing.rows.resize(index + 1);
        placing.rows[index].push_back(span);
    }

    int BasesTree::rowOf(const Placing& placing, const BaseCrumb& crumb)
    {
        return static_cast<int>(std::lround(crumb.topLeft().y / placing.rowPitch));
    }

    float BasesTree::rowCentreY(const Placing& placing, const int row)
    {
        return static_cast<float>(row) * placing.rowPitch + placing.rowHeight / 2.0f;
    }

    BaseCrumb& BasesTree::crumbOf(const BaseNode& node)
    {
        return *m_crumbsMap.at(&node);
    }

    const BaseCrumb& BasesTree::crumbOf(const BaseNode& node) const
    {
        return *m_crumbsMap.at(&node);
    }

    void BasesTree::addCrumbs(BaseSpan bases, BaseCrumb* parentCrumb)
    {
        for (const BaseNode& base : bases)
        {
            std::optional<Color> nameColor;
            if (base.repeated)
                nameColor = m_colorCodes.colorOf(base.spelling);
            BaseCrumb& newCrumb = add<BaseCrumb>(base, nameColor);
            m_crumbsMap.emplace(&base, &newCrumb);
            if (parentCrumb)
                m_parentCrumbs.emplace(&newCrumb, parentCrumb);

            newCrumb.onContextPopup([this, &newCrumb](ContextPopupEvent& event) {
                // event.control could also be the dropdown button - it's "onNestedContextPopup" actually
                //BaseCrumb* crumb = static_cast<BaseCrumb*>(event.control);
                std::wstring_view target = targetOf(newCrumb.node());
                if (target.empty())
                    return;
                BaseLinkContextPopupEvent linkEvent{ newCrumb, target, event };
                emitEvent(linkEvent);
                });
            newCrumb.onBaseLink([this](BaseLinkEvent& event) {
                // re-emitting the crumb's event
                emitEvent(event);
                });
            addCrumbs(base.bases, &newCrumb);
        }
    }

    FloatPoint BasesTree::lineAmongCrumbs(const NavigationStep& step) const
    {
        return step.line - form().contentOriginOfControl(this);
    }

    // A row is the crumbs standing at one top: put places every crumb of a row at the same y.
    BaseCrumb* BasesTree::crumbNearLine(const float fromTopY, const bool downward,
        const float lineX)
    {
        BaseCrumb* result = nullptr;
        float rowTopY = 0.0f;
        float nearest = k_maxFloat;
        for (BaseCrumb& crumb : controlsAs<BaseCrumb>())
        {
            const float topY = crumb.topLeft().y;
            const bool past = downward ? topY > fromTopY : topY < fromTopY;
            if (!past)
                continue;
            const bool nearerRow = !result || (downward ? topY < rowTopY : topY > rowTopY);
            if (!nearerRow && topY != rowTopY)
                continue;
            const float leftX = crumb.topLeft().x;
            const AxisSpan span = { leftX, leftX + crumb.scaledWidth() };
            const float distance = span.distanceTo(lineX);
            if (nearerRow || distance < nearest)
            {
                result = &crumb;
                rowTopY = topY;
                nearest = distance;
            }
        }
        return result;
    }

}
