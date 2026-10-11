module SeeDocs_App.BasesBar;

import SeeDocs_App.Icons;
import SeeDocs_App.PageText;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.Menu;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
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
            // **A crumb is as narrow as its own spelling**, so bases wider than the page narrow
            // their crumbs, and the over-text hint shows a spelling cut short. Only the width is
            // freed: the height stays a button's.
            MinSize{ 0.0f, params.themeMetrics().button.minSize.y },
            // Level with the middle of a column of bases standing beside it on the row.
            VerticalAlign::Center
        },
        m_bar{ bar },
        m_node{ node }
    {
        // The strip is what says there is a list behind the crumb, so a base with no type standing
        // on it - one the surface does not have - has none.
        secondaryPart()->setVisible(!m_node.derived.empty());
        onContextPopup([this](ContextPopupEvent& event) {
            const std::wstring_view target = targetOf(m_node);
            if (target.empty())
                return;
            m_bar.askMenu(*this, std::wstring{ target }, event);
        });
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
        for (const Type* type : m_node.derived)
        {
            const RowIcon icon = rowIconOf(*type);
            MenuItem& line = menu.add(
                type->name,
                [icon](PaintIconEvent& event) {
                    paintRowIcon(event, icon);
                },
                [this, type](Control&) {
                    m_bar.choose(type->qualifiedName);
                }
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
        menu.executeUnder(*this);
    }

    void BaseCrumb::adjustPaint(AdjustPaintEvent& event)
    {
        DropdownControlBase::adjustPaint(event);
        event.setColorRules(UiElement::ToolButton);
    }

    // A press on the strip is the base's, which drops the list; what is left is a press on the
    // crumb's face, which opens the page of the base it names.
    void BaseCrumb::nestedClick(ClickEvent& event)
    {
        const bool pressedStrip = pressedSecondary(event);
        DropdownControlBase::nestedClick(event);
        const std::wstring_view target = targetOf(m_node);
        if (pressedStrip || target.empty())
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
        addRun(*this, bases);
    }

    void BasesBar::clearBases()
    {
        while (!controls().empty())
            controls().back()->deleteSelf();
    }

    void BasesBar::addRun(Stack& row, const BaseNodes& bases)
    {
        const BaseNodes* step = &bases;
        while (step->size() == 1)
        {
            row.add<BaseCrumb>(*this, step->front());
            step = &step->front().bases;
        }
        if (step->empty())
            return;
        Stack& column = row.add<Stack>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ 0.0f },
            VerticalAlign::Center
        );
        for (const BaseNode& base : *step)
        {
            Stack& branch = column.add<Stack>(
                Orientation::Horizontal,
                Padding{ 0.0f },
                Spacing{ 0.0f }
            );
            branch.add<BaseCrumb>(*this, base);
            addRun(branch, base.bases);
        }
    }

    void BasesBar::choose(std::wstring target)
    {
        BaseLinkEvent event{ std::move(target) };
        emitEvent(event);
    }

    void BasesBar::askMenu(Control& crumb, std::wstring target, ContextPopupEvent& popup)
    {
        BaseLinkContextPopupEvent event{ crumb, std::move(target), popup };
        emitEvent(event);
    }
}
