module ClaFi.Controls.Tree;

import ClaFi.Controls.Menu;
import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Stack;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // TreeItem

    ScaledSize TreeItem::calculateContent(AlignEvent& event)
    {
        ScaledSize result = ToolButton::calculateContent(event);
        result.x += textLead(event.scaler());
        return result;
    }

    // The text is measured in the width the lead leaves, the box it is painted in; measured in
    // the whole width, a wrapping text that fits that and not the box comes out a line short.
    void TreeItem::alignContent(AlignEvent& event, ScaledPosition position,
        ScaledSize& contentSize)
    {
        const float lead = textLead(event.scaler());
        contentSize.x = std::max(0.0f, contentSize.x - lead);
        position.x += lead;
        ToolButton::alignContent(event, position, contentSize);
        contentSize.x += lead;
    }

    // A row stating a format of its own is measured whole: the tree's line is one of body text.
    ScaledSize TreeItem::measureText(AlignEvent& event, ScaledSize asked,
        const Text& text)
    {
        if (m_tree.m_rowLine.has_value() && !textFormat())
            return { 0.0f, *m_tree.m_rowLine };
        return ToolButton::measureText(event, asked, text);
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
        return scaler.scale(static_cast<float>(m_level) * TreeRows::Step)
            + scaler.scale(TreeRows::MarkSize)
            + scaler.scale(TreeRows::MarkPadding) * 2.0f
            + scaler.scale(TreeRows::Gap);
    }

    // TreeDivider

    // Counted in the width as a row's lead is, so the line never ends before it starts.
    ScaledSize TreeDivider::calculateContent(AlignEvent& event)
    {
        ScaledSize result = Divider::calculateContent(event);
        result.x += event.scale(lead());
        return result;
    }

    // The lead is on the left alone: the line starts where the rows of its level do.
    void TreeDivider::paintSurface(PaintEvent& event)
    {
        event.defaultPaintSurface(leadingInset());
    }

    void TreeDivider::paintFrame(PaintEvent& event)
    {
        event.defaultPaintFrame(leadingInset());
    }

    float TreeDivider::lead() const
    {
        return static_cast<float>(m_level) * TreeRows::Step;
    }

    FloatInset TreeDivider::leadingInset() const
    {
        const Padding room = padding();
        return {
            .left = room.x + lead(),
            .top = room.y,
            .right = room.x,
            .bottom = room.y
        };
    }

    // TreeNodeHeader

    // A tree node builds its header and holds it, so the parent is that node.
    ScaledSize TreeNodeHeader::measureText(AlignEvent& event, ScaledSize asked,
        const Text& text)
    {
        const TreeView& tree = static_cast<const TreeNode*>(parent())->m_tree;
        if (tree.m_rowLine.has_value() && !textFormat())
            return { 0.0f, *tree.m_rowLine };
        return ExpanderHeader::measureText(event, asked, text);
    }

    // TreeNode

    // **Left from a row inside the node goes to its header.** The header opens on Right and closes
    // on Left by itself, and leaves Left on a closed node to the node holding this one, so a run
    // of presses climbs the tree a level at a time.
    void TreeNode::nestedKeyDown(KeyDownEvent& event)
    {
        ExpanderHeader& strip = header();
        if (event.key == Keys::Left && event.modifiers.empty() && !strip.isFocused())
        {
            strip.setFocus();
            strip.scrollIntoView();
            event.handled = true;
            return;
        }
        Expander::nestedKeyDown(event);
    }

    // The header leaves Right on an open node to the tree, and the rows are where it goes - the way
    // Left from a row comes back to the header. A shut node's rows are hidden and hold no stop.
    Control* TreeNode::navigationNeighbour(const Control& child, const NavigationStep& step)
    {
        const bool intoRows = &child == &header() && step.direction == NavigationDirection::Right;
        if (intoRows && m_rows.holdsNavigationStop())
            return &m_rows;
        return Expander::navigationNeighbour(child, step);
    }

    void TreeNode::layOutHeader(const ControlMetrics& buttonMetrics)
    {
        ExpanderHeader& strip = header();
        strip.setPadding(TreeRows::Inset, buttonMetrics.padding.y);
        strip.setSpacing(TreeRows::Gap);
        strip.setMinSize(0.0f, buttonMetrics.minSize.y);
        strip.setLead(static_cast<float>(m_level) * TreeRows::Step);
        strip.setWordWrap(WordWrap::No);
        ExpanderButton& mark = strip.button();
        mark.setIconSize(IconSize{ TreeRows::MarkSize });
        // No padding across, so the mark never makes the row taller than its text does.
        mark.setPadding(TreeRows::MarkPadding, 0.0f);
        mark.setMinSize(0.0f);
    }

    // Tree

    // By index: a node opening may fetch nodes, and they join the end of the list.
    void TreeView::expandAll()
    {
        for (std::size_t i = 0; i != m_nodes.size(); ++i)
            m_nodes[i]->header().setExpanded(true);
        showCurrentRow();
    }

    void TreeView::collapseAll()
    {
        for (TreeNode* node : m_nodes)
            node->header().setExpanded(false);
        showCurrentRow();
    }

    // By index, as expandAll: the current row's own node may fetch nodes as it opens.
    void TreeView::collapseOthers()
    {
        for (std::size_t i = 0; i != m_nodes.size(); ++i)
        {
            TreeNode& node = *m_nodes[i];
            node.header().setExpanded(holdsCurrentRow(node));
        }
        showCurrentRow();
    }

    // **Every row is one line**, and where the tree's width is given nothing reads a row's own,
    // so the line is measured once for all of them. A tree scrolled across is as wide as its
    // widest row. See Controls#tree
    void TreeView::calculateChildren(FormBase& form)
    {
        m_rowLine.reset();
        if (isWidthGivenFromOutside() && !isScrolledByParent(ScrollAxis::Horizontal))
        {
            const Text probe{ k_lineProbe };
            m_rowLine = textEngine().calculateText(form.context(), probe,
                { k_maxFloat, k_maxFloat }, false, false).y;
        }
        TreeViewBase::calculateChildren(form);
    }

    // A node's mark opens the node rather than standing as a row, so it is no item.
    bool TreeView::defaultCanFocusItem(Control& value)
    {
        return TreeViewBase::defaultCanFocusItem(value) && isRow(value);
    }

    // The application has first refusal, and a handler that stops the event has replaced the
    // menu outright; one on AdjustContextPopupEvent adjusts the tree's commands instead, and a
    // tree with no node has none of its own. Stopped here before a menu runs, so nothing above
    // raises a second menu behind this one. The menu stands on the row it was asked for - the one
    // under the pointer, or the current row from the keyboard - rather than on the tree, whose
    // bounds run past the window the moment it scrolls.
    void TreeView::nestedContextPopup(ContextPopupEvent& event)
    {
        TreeViewBase::nestedContextPopup(event);
        if (event.propagationStopped())
            return;
        ActionList items;
        if (!m_nodes.empty())
            items = { &m_expandAll, &m_collapseAll, &m_collapseOthers };
        AdjustContextPopupEvent adjustEvent{ *event.control, event.form, items, event.mousePos };
        adjustContextPopup(adjustEvent);
        if (adjustEvent.propagationStopped() || items.empty())
            return;
        event.stopPropagation();
        Menu menu{ *event.control };
        menu.addActions(items);
        menu.execute();
    }

    void TreeView::nestedControlDeleted(Control* item)
    {
        TreeViewBase::nestedControlDeleted(item);
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

    Control* TreeView::currentRow()
    {
        Control* item = containerCurrentItem(CheckSelf::Yes);
        return item && containsNested(item, CheckSelf::No) ? item : nullptr;
    }

    bool TreeView::holdsCurrentRow(const TreeNode& node)
    {
        const Control* row = currentRow();
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

    void TreeView::showCurrentRow()
    {
        if (Control* row = currentRow())
            row->scrollIntoViewOnAlign();
    }

}
