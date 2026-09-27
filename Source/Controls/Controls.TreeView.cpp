module ClaFi.Controls.TreeView;

import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.StackView;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;

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

    // Right opens a closed node from its mark and Left closes an open one. Left from inside a
    // node goes to its mark, and from a closed mark it is left to the node holding this one, so
    // a run of presses climbs the tree a level at a time.
    bool TreeNode::answerTreeKey(const KeyCode key)
    {
        ExpanderHeader& strip = header();
        ExpanderButton& mark = strip.button();
        const bool onMark = mark.isFocused();
        switch (key)
        {
            case Keys::Right:
                if (!onMark || strip.expanded())
                    return false;
                strip.setExpanded(true);
                return true;
            case Keys::Left:
                if (!onMark)
                {
                    mark.setFocus();
                    mark.scrollIntoView();
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

    // A node's mark opens the node rather than standing as a row, so it is no item.
    bool TreeView::defaultCanFocusItem(Control& value)
    {
        return StackView::defaultCanFocusItem(value) && isRow(value);
    }

    // A mark stands in its node's header, and a header is no body.
    bool TreeView::isRow(const Control& control) const
    {
        const Control* rows = control.parent();
        while (rows && rows != this)
        {
            if (!rows->isHostedAsBody())
                return false;
            rows = rows->parent()->parent();
        }
        return rows == this;
    }

}
