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

    BaseCrumb::BaseCrumb(const CreateParams& params, const BaseNode& node)
        :
        DropdownControlBase{
            params,
            Interactivity::Focusable,
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
        m_node{ node }
    {
    }

    // The strip stands at the crumb's edge, inside the child inset alone.
    float BaseCrumb::markCentreX(const FormContext& formContext) const
    {
        const Control& strip = *secondaryPart();
        return childInset(formContext, scaledPadding()).x + strip.scaledWidth() / 2.0f;
    }

    void BaseCrumb::getMainText(GetTextEvent& event) const
    {
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
            line.setFocus();
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
        addCrumbs(m_bases);
    }

    void BasesTree::clear()
    {
        m_bases = {};
        m_crumbsMap.clear();
        m_links.clear();
        m_branches.clear();
        m_trunks.clear();
        while (!controls().empty())
            controls().back()->deleteSelf();
    }

    ScaledSize BasesTree::calculateContent(AlignEvent& event)
    {
        return placeCrumbs(event, k_maxFloat);
    }

    // The measuring pass placed the crumbs against no wall. They stand where they are when that
    // fits the room given, and are placed again against its end when it does not.
    void BasesTree::alignContent(AlignEvent& event, ScaledPosition, ScaledSize& newSize)
    {
        const ScaledSize measuredSize = scaledSize() - event.scaledPadding() * 2.0f;
        if (measuredSize.x <= newSize.x)
            return;
        newSize = placeCrumbs(event, newSize.x);
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

    ScaledSize BasesTree::placeCrumbs(AlignEvent& event, const float wallX)
    {
        m_links.clear();
        m_branches.clear();
        m_trunks.clear();
        if (m_bases.empty())
            return {};
        Placing placing = {
            .wallX = wallX,
            .spacing = event.scaledSpacing(),
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
            BaseCrumb& crumb = crumbOf(node);
            const int row = freeRow(placing, crumb, 0.0f, trunkX, previousRow);
            put(placing, crumb, row, 0.0f);
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
            BaseCrumb& crumb = crumbOf(base);
            if (first && fitsOnRow(placing, parentCrumb, base))
            {
                put(placing, crumb, parentRow, parentRightX + placing.spacing.x);
                addLink(placing, parentRightX, crumb.topLeft().x, parentRow);
            }
            else
            {
                const int row = freeRow(placing, crumb, x, trunkX, previousRow);
                put(placing, crumb, row, x);
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
        const float rightX = x + crumbOf(base).scaledWidth() + placing.clearance;
        return rowFree(placing, rowOf(placing, parent), { parentRightX, rightX });
    }

    // The first row below the previous one where the crumb, its clearance and the stub reaching
    // it from the trunk meet nothing.
    int BasesTree::freeRow(const Placing& placing, const BaseCrumb& crumb, const float x,
        const float trunkX, const int previousRow) const
    {
        const Span span = {
            std::min(trunkX, x - placing.clearance),
            x + crumb.scaledWidth() + placing.clearance,
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
        const float width = crumbOf(base).scaledWidth();
        if (!innerWidth)
            return width;
        return std::max(width, placing.indent + innerWidth);
    }

    void BasesTree::put(Placing& placing, BaseCrumb& crumb, const int row, const float x)
    {
        setScaledPosition(crumb, { x, static_cast<float>(row) * placing.rowPitch });
        const float rightX = x + crumb.scaledWidth();
        takeSpan(placing, row, { x, rightX });
        placing.rightX = std::max(placing.rightX, rightX);
        placing.lastRow = std::max(placing.lastRow, row);
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

    void BasesTree::addCrumbs(BaseSpan bases)
    {
        for (const BaseNode& base : bases)
        {
            BaseCrumb& newCrumb = add<BaseCrumb>(base);
            m_crumbsMap.emplace(&base, &newCrumb);

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
            addCrumbs(base.bases);
        }
    }

}
