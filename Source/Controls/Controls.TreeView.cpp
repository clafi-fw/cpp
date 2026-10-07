module ClaFi.Controls.TreeView;

import ClaFi.Controls.Menu;
import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.StackView;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // TreeItem

    ScaledDimensions TreeItem::calculateContent(AlignEvent& event)
    {
        ScaledDimensions result = ToolButton::calculateContent(event);
        result.x += textLead(event.scaler());
        return result;
    }

    FloatRect TreeItem::iconRect(const PaintEvent& event) const
    {
        FloatRect result = ToolButton::iconRect(event);
        result.offset(textLead(event.scaler()), 0.0f);
        return result;
    }

    void TreeItem::adjustTextRect(AdjustTextRectEvent& event) const
    {
        event.textBounds.left += textLead(event.scaler());
        ToolButton::adjustTextRect(event);
    }

    // Each part is scaled on its own, the way the header scales its lead, its mark and its
    // spacing, so the rounding comes out the same in both.
    float TreeItem::textLead(const Scaler& scaler) const
    {
        return scaler.scale(static_cast<float>(m_level) * TreeRows::step)
            + scaler.scale(TreeRows::markSize)
            + scaler.scale(TreeRows::markPadding) * 2.0f
            + scaler.scale(TreeRows::gap);
    }

    // TreeNode

    void TreeNode::nestedKeyDown(KeyDownEvent& event)
    {
        if (event.modifiers.empty() && answerTreeKey(event.key))
        {
            event.handled = true;
            return;
        }
        Expander::nestedKeyDown(event);
    }

    void TreeNode::layOutHeader(const ControlMetrics& buttonMetrics)
    {
        ExpanderHeader& strip = header();
        strip.setPadding(TreeRows::inset, buttonMetrics.padding.y);
        strip.setSpacing(TreeRows::gap);
        strip.setMinSize(0.0f, buttonMetrics.minSize.y);
        strip.setLead(static_cast<float>(m_level) * TreeRows::step);
        ExpanderButton& mark = strip.button();
        mark.setIconSize(IconSize{ TreeRows::markSize });
        // No padding across, so the mark never makes the row taller than its text does.
        mark.setPadding(TreeRows::markPadding, 0.0f);
        mark.setMinSize(0.0f);
    }

    // Right opens a closed node from its header and Left closes an open one. Left from a row
    // inside the node goes to the header, and from a closed header it is left to the node
    // holding this one, so a run of presses climbs the tree a level at a time.
    bool TreeNode::answerTreeKey(const KeyCode key)
    {
        ExpanderHeader& strip = header();
        const bool onHeader = strip.isFocused();
        switch (key)
        {
            case Keys::Right:
                if (!onHeader || strip.expanded())
                    return false;
                strip.setExpanded(true);
                return true;
            case Keys::Left:
                if (!onHeader)
                {
                    strip.setFocus();
                    strip.scrollIntoView();
                    return true;
                }
                if (!strip.expanded())
                    return false;
                strip.setExpanded(false);
                return true;
        }
        return false;
    }

    // TreeView

    void TreeView::expandAll()
    {
        for (TreeNode* node : m_nodes)
            node->header().setExpanded(true);
        showCurrentRow();
    }

    void TreeView::collapseAll()
    {
        for (TreeNode* node : m_nodes)
            node->header().setExpanded(false);
        showCurrentRow();
    }

    void TreeView::collapseOthers()
    {
        for (TreeNode* node : m_nodes)
        {
            if (!holdsCurrentRow(*node))
                node->header().setExpanded(false);
        }
        showCurrentRow();
    }

    // A node's mark opens the node rather than standing as a row, so it is no item.
    bool TreeView::defaultCanFocusItem(Control& value)
    {
        return StackView::defaultCanFocusItem(value) && isRow(value);
    }

    // The application has first refusal, and a handler that stops the event has replaced the
    // menu outright. Stopped here before the menu runs, so nothing above raises a second menu
    // behind this one.
    void TreeView::nestedContextPopup(ContextPopupEvent& event)
    {
        StackView::nestedContextPopup(event);
        if (event.propagationStopped() || m_nodes.empty())
            return;
        event.stopPropagation();
        Menu menu{ *this };
        menu.add(m_expandAll);
        menu.add(m_collapseAll);
        menu.add(m_collapseOthers);
        menu.execute();
    }

    void TreeView::nestedControlDeleted(Control* item)
    {
        StackView::nestedControlDeleted(item);
        std::erase_if(m_nodes, [item](const TreeNode* node) {
            return node == item;
        });
    }

    // Claiming says this tree is what the command acts on; what it claims says whether there is
    // anything left for it to move.
    void TreeView::connectActions()
    {
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &m_expandAll)
                event.claim({ .enabled = hasNode(false) });
            else if (&event.action == &m_collapseAll)
                event.claim({ .enabled = hasNode(true) });
            else if (&event.action == &m_collapseOthers)
                event.claim({ .enabled = hasOtherNodeOpen() });
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

    // A node's header stands in the node, outside its rows, and the node stands in the rows of
    // another or in the tree. What stands inside the header - its mark - has the header for a
    // parent, and a header is in no rows, which is what keeps it out.
    bool TreeView::isRow(const Control& control) const
    {
        const Control* parent = control.parent();
        if (!parent)
            return false;
        if (parent == this)
            return true;
        if (parent->isHostedAsBody())
            return isRow(*parent->parent());
        const Control* nodeParent = parent->parent();
        const bool nodeInRows = nodeParent && (nodeParent == this || nodeParent->isHostedAsBody());
        return nodeInRows && !control.isHostedAsBody() && isRow(*parent);
    }

    bool TreeView::holdsCurrentRow(const TreeNode& node) const
    {
        const Control* row = currentItem();
        if (!row)
            return false;
        return row == &node.header() || node.rows().containsNested(row);
    }

    bool TreeView::hasNode(const bool expanded) const
    {
        return std::ranges::any_of(m_nodes, [expanded](const TreeNode* node) {
            return node->header().expanded() == expanded;
        });
    }

    bool TreeView::hasOtherNodeOpen() const
    {
        return std::ranges::any_of(m_nodes, [this](const TreeNode* node) {
            return node->header().expanded() && !holdsCurrentRow(*node);
        });
    }

    // The nodes are listed with every node after the one holding it, so the first closed node
    // over the row is the outermost one.
    void TreeView::showCurrentRow()
    {
        Control* row = currentItem();
        if (!row)
            return;
        for (TreeNode* node : m_nodes)
        {
            if (!node->header().expanded() && node->rows().containsNested(row))
            {
                node->header().scrollIntoViewOnAlign();
                return;
            }
        }
        row->scrollIntoViewOnAlign();
    }

}
