module SeeDocs_App.BasesBar;

import SeeDocs_App.Icons;
import SeeDocs_App.PageText;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Base.Container;
import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.Menu;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
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
    }

    // BaseLinkEvent

    BaseLinkEvent::BaseLinkEvent(std::wstring linkTarget)
        :
        ::ClaFi::Event{},
        target{ std::move(linkTarget) }
    {
    }

    // BaseLinkContextPopupEvent

    BaseLinkContextPopupEvent::BaseLinkContextPopupEvent(Control& owner, std::wstring linkTarget,
        ContextPopupEvent& request)
        :
        ::ClaFi::Event{},
        crumb{ owner },
        target{ std::move(linkTarget) },
        popup{ request }
    {
    }

    // BaseCrumb

    BaseCrumb::BaseCrumb(const CreateParams& params, BasesBar& bar, const BaseNode& node)
        :
        DropdownControlBase{
            params,
            Interactivity::Focusable,
            SecondaryPartPlacement::Left,
            params.themeMetrics().toolButton,
            Padding{ 4.0f },
            // The bar narrows its crumbs itself, so only the height is held: a button's.
            MinSize{ 0.0f, params.themeMetrics().button.minSize.y },
            // One line, cut short where the bar narrows it.
            WordWrap::No,
            // Level with the middle of the run it stands in, a column of bases beside it included.
            VerticalAlign::Center
        },
        m_bar{ bar },
        m_node{ node }
    {
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
                MenuItem::OnPaintIcon{ [icon](PaintIconEvent& event) {
                    paintRowIcon(event, icon);
                } },
                MenuItem::OnClick{ [this, type](ClickEvent&) {
                    m_bar.choose(type->qualifiedName);
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
        event.setColorRules(UiElement::ToolButton);
    }

    // A click on the crumb's face opens the page of the base it names. A click on the strip is a
    // nested one, and the base drops the list from it.
    void BaseCrumb::click(ClickEvent& event)
    {
        DropdownControlBase::click(event);
        const std::wstring_view target = targetOf(m_node);
        if (target.empty())
            return;
        m_bar.choose(std::wstring{ target });
    }

    DropdownMarkTurn BaseCrumb::dropdownMarkTurn() const
    {
        return { ChevronTurn::left, ChevronTurn::down };
    }

    // BasesBar

    void BasesBar::setBases(const BaseNodes& bases)
    {
        clearBases();
        m_bases = bases;
        addCrumbs(m_bases);
        m_fullWidths.assign(m_crumbs.size(), 0.0f);
    }

    void BasesBar::clearBases()
    {
        m_bases = {};
        m_crumbs.clear();
        m_fullWidths.clear();
        while (!controls().empty())
            controls().back()->deleteSelf();
    }

    // **What a crumb measured is kept here**, where it is still its measurement: the align pass
    // writes the width a crumb is given over it.
    ScaledSize BasesBar::calculateContent(AlignEvent&)
    {
        for (std::size_t i = 0; i != m_crumbs.size(); ++i)
            m_fullWidths[i] = m_crumbs[i]->scaledWidth();
        std::size_t crumb = 0;
        const RunExtent whole = runExtent(m_bases, crumb, k_maxFloat);
        return { whole.width, whole.height };
    }

    void BasesBar::alignContent(AlignEvent& event, ScaledPosition position,
        ScaledSize& contentDimensions)
    {
        const float cap = capFor(contentDimensions.x, event.scale(k_leastCrumb));
        std::size_t crumb = 0;
        const RunExtent whole = runExtent(m_bases, crumb, cap);
        crumb = 0;
        placeRun(event, m_bases, crumb, cap, position, whole.height);
        contentDimensions = { whole.width, whole.height };
    }

    void BasesBar::addCrumbs(const BaseSpan bases)
    {
        for (const BaseNode& base : bases)
        {
            BaseCrumb& newCrumb = add<BaseCrumb>(*this, base);
            m_crumbs.push_back(&newCrumb);
            newCrumb.onContextPopup([this, &newCrumb](ContextPopupEvent& event) {
                // event.control could also be the dropdown button - it's "onNestedContextPopup" actually
                //BaseCrumb* crumb = static_cast<BaseCrumb*>(event.control);
                const std::wstring_view target = targetOf(newCrumb.node());
                if (target.empty())
                    return;
                askMenu(newCrumb, std::wstring{ target }, event);
            });
            addCrumbs(base.bases);
        }
    }

    // Along a way the crumbs add up, and a column is as wide as its widest row and as tall as its
    // rows together.
    BasesBar::RunExtent BasesBar::runExtent(BaseSpan bases, std::size_t& crumb,
        const float cap) const
    {
        RunExtent run = {};
        while (bases.size() == 1)
        {
            run.width += crumbWidth(crumb, cap);
            run.height = std::max(run.height, m_crumbs[crumb]->scaledHeight());
            ++crumb;
            bases = bases.front().bases;
        }
        if (bases.empty())
            return run;
        RunExtent column = {};
        for (std::size_t i = 0; i != bases.size(); ++i)
        {
            const RunExtent row = runExtent(bases.subspan(i, 1), crumb, cap);
            column.width = std::max(column.width, row.width);
            column.height += row.height;
        }
        run.width += column.width;
        run.height = std::max(run.height, column.height);
        return run;
    }

    void BasesBar::placeRun(AlignEvent& event, BaseSpan bases, std::size_t& crumb,
        const float cap, ScaledPosition position, const float height)
    {
        while (bases.size() == 1)
        {
            const float width = crumbWidth(crumb, cap);
            alignControl(m_crumbs[crumb], event, position, { width, height });
            ++crumb;
            position.x += width;
            bases = bases.front().bases;
        }
        if (bases.empty())
            return;
        std::vector<float> rowHeights;
        float columnHeight = 0.0f;
        std::size_t ahead = crumb;
        for (std::size_t i = 0; i != bases.size(); ++i)
        {
            rowHeights.push_back(runExtent(bases.subspan(i, 1), ahead, cap).height);
            columnHeight += rowHeights.back();
        }
        position.y += (height - columnHeight) / 2.0f;
        for (std::size_t i = 0; i != bases.size(); ++i)
        {
            placeRun(event, bases.subspan(i, 1), crumb, cap, position, rowHeights[i]);
            position.y += rowHeights[i];
        }
    }

    float BasesBar::crumbWidth(const std::size_t crumb, const float cap) const
    {
        return std::min(m_fullWidths[crumb], cap);
    }

    // **The widest crumbs come down to one common width**, the way a fit levels a row of tabs, so
    // a short name stays whole while a long template spelling gives way. A column makes the bar's
    // width the widest of its rows rather than their sum, so the cap is found by halving the range
    // it can stand in rather than solved for. A bar that overruns at the least a crumb keeps is
    // left to overrun. A bar with no crumbs takes no cap, even at a width under zero.
    float BasesBar::capFor(const float width, const float leastCrumb) const
    {
        if (m_crumbs.empty())
            return k_maxFloat;
        std::size_t crumb = 0;
        if (runExtent(m_bases, crumb, k_maxFloat).width <= width)
            return k_maxFloat;
        float fits = leastCrumb;
        float overruns = std::ranges::max(m_fullWidths);
        for (int step = 0; step != k_capSteps; ++step)
        {
            const float cap = (fits + overruns) / 2.0f;
            crumb = 0;
            if (runExtent(m_bases, crumb, cap).width <= width)
                fits = cap;
            else
                overruns = cap;
        }
        // Whole pixels: a crumb's align ceils the width it is given, and a fraction would push it
        // over the next crumb.
        return std::floor(fits);
    }

    void BasesBar::choose(std::wstring target)
    {
        BaseLinkEvent event{ std::move(target) };
        emitEvent(event);
    }

    void BasesBar::askMenu(Control& crumb, std::wstring target, ContextPopupEvent& popup) const
    {
        BaseLinkContextPopupEvent event{ crumb, std::move(target), popup };
        emitEvent(event);
    }
}
