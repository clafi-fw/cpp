module ClaFi.Controls.Grids;

import ClaFi.Controls.Menu;
import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    // Grid

    // **The application has first refusal**, and a handler that stops the event - a row's own
    // menu, a link's - has replaced the menu outright. Stopped here before the menu runs, so
    // nothing above raises a second menu behind it. The menu stands on the control it was asked
    // on, as a tree's stands on its row, and that control is what Collapse others opens.
    void Grid::nestedContextPopup(ContextPopupEvent& event)
    {
        GridBase::nestedContextPopup(event);
        if (event.propagationStopped() || !holdsExpander())
            return;
        event.stopPropagation();
        m_menuControl = event.control;
        Menu menu{ *event.control };
        menu.add(m_expandAll);
        menu.add(m_collapseAll);
        menu.add(m_collapseOthers);
        menu.execute();
        m_menuControl = nullptr;
    }

    // Claiming says this grid is what the command acts on; what it claims says whether there is
    // anything left for it to move.
    void Grid::connectActions()
    {
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &m_expandAll)
                event.claim({ .enabled = hasExpander(false) });
            else if (&event.action == &m_collapseAll)
                event.claim({ .enabled = hasExpander(true) });
            else if (&event.action == &m_collapseOthers)
                event.claim({});
        });
        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &m_expandAll)
                expandAll();
            else if (&event.action == &m_collapseAll)
                collapseAll();
            else if (&event.action == &m_collapseOthers)
                collapseOthers();
        });
    }

    void Grid::expandAll()
    {
        forEachExpander([](RowExpander& expander) {
            expander.header().setExpanded(true);
        });
        showMenuControl();
    }

    void Grid::collapseAll()
    {
        forEachExpander([](RowExpander& expander) {
            expander.header().setExpanded(false);
        });
        showMenuControl();
    }

    void Grid::collapseOthers()
    {
        forEachExpander([this](RowExpander& expander) {
            expander.header().setExpanded(expander.containsNested(m_menuControl));
        });
        showMenuControl();
    }

    bool Grid::holdsExpander()
    {
        bool found = false;
        forEachExpander([&found](RowExpander&) {
            found = true;
        });
        return found;
    }

    bool Grid::hasExpander(const bool expanded)
    {
        bool found = false;
        forEachExpander([expanded, &found](RowExpander& expander) {
            if (expander.header().expanded() == expanded)
                found = true;
        });
        return found;
    }

    // A control folded away is brought back as the expander row it went under, the outermost.
    void Grid::showMenuControl()
    {
        if (!m_menuControl)
            return;
        Control* shown = m_menuControl;
        for (Control* control = m_menuControl; control && control != this;
            control = control->parent())
        {
            if (!control->visible())
                shown = control->parent();
        }
        shown->scrollIntoViewOnAlign();
    }
}
