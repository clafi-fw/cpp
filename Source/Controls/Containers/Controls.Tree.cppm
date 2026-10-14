module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Tree;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // The design geometry every row of a tree is laid out by.
    namespace TreeRows
    {
        constexpr float inset = 6.0f;       // between a row's edges and what it holds
        constexpr float markSize = 12.0f;   // the mark a node opens by
        constexpr float markPadding = 3.0f; // either side of the mark
        constexpr float markSlot = markSize + markPadding * 2.0f;
        constexpr float gap = 4.0f;         // between the mark slot and the text
        constexpr float step = markSlot;    // how far a level stands in from the one holding it
        constexpr float dividerRoom = 4.0f; // above and below a divider's line
    }

    export class TreeView;
    export class TreeNode;

    // A node is opening with no rows under it, and asks for them.
    export class FetchRowsEvent : public Event
    {
    public:
        explicit FetchRowsEvent(TreeNode& node) : node{ node } {}
        TreeNode& node;
    };

    // A row of a tree a pick can land on, its text past the mark slot every level keeps clear.
    export class TreeItem : public ToolButton
    {
    public:
        template<typename... Args>
        explicit TreeItem(const CreateParams&, TreeView& tree, std::size_t level, Args&&...);
    protected:
        ScaledDimensions calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        CalculatedDimensions measureText(AlignEvent&, ScaledDimensions asked, const Text&) override;
        FloatRect iconRect(const PaintEvent&) const override;
        void adjustTextRect(AdjustTextRectEvent&) const override;
    private:
        [[nodiscard]] float textLead(const Scaler&) const; // where the text begins past the padding
    private:
        TreeView& m_tree;
        std::size_t m_level;
    };

    // A line between two groups of rows, standing in as far as the rows of its level do.
    export class TreeDivider : public Divider
    {
    public:
        template<typename... Args>
        explicit TreeDivider(const CreateParams&, std::size_t level, Args&&...);
    protected:
        ScaledDimensions calculateContent(AlignEvent&) override;
        void paintSurface(PaintEvent&) override;
    private:
        [[nodiscard]] float lead() const; // how far the line stands in past the padding
    private:
        std::size_t m_level;
    };

    // The header of a tree's node, a row of the tree measured the way its items are.
    export class TreeNodeHeader : public ExpanderHeader
    {
    public:
        using ExpanderHeader::ExpanderHeader;
    protected:
        CalculatedDimensions measureText(AlignEvent&, ScaledDimensions asked, const Text&) override;
    };

    // A branch of a tree: a header that is a row of its own, and under it the rows its mark opens.
    export class TreeNode : public Expander
    {
        friend TreeNodeHeader;
    public:
        template<typename... Args>
        explicit TreeNode(const CreateParams&, TreeView& tree, std::size_t level, Args&&...);
    public:
        // The node is opening with no rows under it, and asks for them.
        DECLARE_EVENT(FetchRowsEvent, OnFetchRows, onFetchRows)
    public:
        // The rows under the header.
        [[nodiscard]] StackPanel& rows() { return m_rows; }
        [[nodiscard]] const StackPanel& rows() const { return m_rows; }
        // Adds a row a pick can land on, one level in.
        template<typename... Args>
        TreeItem& addItem(Args&&...);
        // Adds a branch one level in.
        template<typename... Args>
        TreeNode& addNode(Args&&...);
        // Adds a line between two groups of rows, one level in.
        template<typename... Args>
        TreeDivider& addDivider(Args&&...);
    protected:
        void nestedKeyDown(KeyDownEvent&) override;
    private:
        // Lays the header out the way a TreeItem is laid out, so the two texts share a column.
        void layOutHeader(const ControlMetrics& buttonMetrics);
        // Takes a key that opens, closes or leaves this node; false leaves it to what holds it.
        [[nodiscard]] bool answerTreeKey(KeyCode);
    private:
        TreeView& m_tree;
        std::size_t m_level;
        StackPanel& m_rows;
    };

    // Rows nested in branches, each level stepped in one mark slot; a node's header is a row too.
    using TreeViewBase = SingleSelectionView;
    export class TreeView : public TreeViewBase
    {
        friend TreeItem;
        friend TreeNodeHeader;
        friend TreeNode;
    public:
        template<typename... Args>
        explicit TreeView(const CreateParams&, Args&&...);
    public:
        std::wstring_view diagnosticText() const override { return L"Tree"; }
        // Adds a row a pick can land on, at the top level.
        template<typename... Args>
        TreeItem& addItem(Args&&...);
        // Adds a branch at the top level.
        template<typename... Args>
        TreeNode& addNode(Args&&...);
        // Adds a line between two groups of rows, at the top level.
        template<typename... Args>
        TreeDivider& addDivider(Args&&...);
        // Opens every node, and every node a node fetches as it opens.
        void expandAll();
        // Closes every node; a current row folded away moves to the outermost header over it.
        void collapseAll();
        // Closes every node but the current row's own and the ones it stands under.
        void collapseOthers();
    protected:
        void calculateChildren(FormBase&) override;
        bool defaultCanFocusItem(Control&) override;
        void nestedContextPopup(ContextPopupEvent&) override;
        void nestedControlDeleted(Control*) override;
    private:
        using Nodes = std::vector<TreeNode*>;
    private:
        // Builds a node into the rows given and keeps it on the list every bulk operation walks.
        template<typename NodesContainer, typename... Args>
        TreeNode& createNode(NodesContainer&, std::size_t level, Args&&...);
        void connectActions();
        // Whether the control stands in this tree, in the rows of a node that does, or is the
        // header of such a node.
        [[nodiscard]] bool isRow(const Control&) const;
        // Whether the node is the current row's own, or holds the current row in its rows.
        [[nodiscard]] bool holdsCurrentRow(const TreeNode&) const;
        [[nodiscard]] bool hasNode(bool expanded) const;
        [[nodiscard]] bool hasOtherNodeOpen() const;
        // The node has closed; a current row folded away under it moves to its header.
        void rowsFolded(TreeNode&);
        // Brings the current row back after the nodes have moved.
        void showCurrentRow();
    private:
        static constexpr std::wstring_view k_lineProbe = L"0"; // what a row's line is measured on

        Nodes m_nodes{}; // every node of the tree, each after the one holding it
        std::optional<float> m_rowLine{}; // the height of every row's text while the width is given
        Action m_expandAll{ Text{ L"Expand all" } };
        Action m_collapseAll{ Text{ L"Collapse all" } };
        Action m_collapseOthers{ Text{ L"Collapse others" } };
    };


    //----------------------------------------------------------------------------


    // TreeItem

    template<typename... Args>
    TreeItem::TreeItem(const CreateParams& params, TreeView& tree, const std::size_t level,
        Args&&... args)
        :
        ToolButton{
            params,
            HorizontalTextAnchor::Left,
            ShowSelectionOnSurface::Yes,
            Padding{ TreeRows::inset, params.themeMetrics().button.padding.y },
            WordWrap::No, // a row is one line; before the caller's arguments, so a stated one wins
            std::forward<Args>(args)...
        },
        m_tree{ tree },
        m_level{ level }
    {
    }

    // TreeDivider

    template<typename... Args>
    TreeDivider::TreeDivider(const CreateParams& params, const std::size_t level, Args&&... args)
        :
        Divider{
            params,
            Padding{ TreeRows::inset, TreeRows::dividerRoom },
            std::forward<Args>(args)...
        },
        m_level{ level }
    {
    }

    // TreeNode

    template<typename... Args>
    TreeNode::TreeNode(const CreateParams& params, TreeView& tree, const std::size_t level,
        Args&&... args)
        :
        Expander{
            params,
            std::type_identity<TreeNodeHeader>{},
            ExpanderViewMode::TreeNode,
            VerticalAlign::Top, // a panel filling its slot does not grow to hold its body
            Padding{ 0.0f },    // the page's padding would add up level after level
            std::forward<Args>(args)...
        },
        m_tree{ tree },
        m_level{ level },
        m_rows{ createBody<StackPanel>(Orientation::Vertical) }
    {
        layOutHeader(params.themeMetrics().button);
        // Closing over the current row folds it away, and the header is where it went. Opening
        // with no rows asks for them.
        header().connectEvent<ToggleExpandedEvent>([this](ToggleExpandedEvent& event) {
            if (!event.expanded())
                m_tree.rowsFolded(*this);
            else if (m_rows.controls().empty())
                emitEvent<FetchRowsEvent>(*this);
        });
    }

    template<typename... Args>
    TreeItem& TreeNode::addItem(Args&&... args)
    {
        return m_rows.add<TreeItem>(m_tree, m_level + 1, std::forward<Args>(args)...);
    }

    template<typename... Args>
    TreeNode& TreeNode::addNode(Args&&... args)
    {
        return m_tree.createNode(m_rows, m_level + 1, std::forward<Args>(args)...);
    }

    template<typename... Args>
    TreeDivider& TreeNode::addDivider(Args&&... args)
    {
        return m_rows.add<TreeDivider>(m_level + 1, std::forward<Args>(args)...);
    }

    // Tree

    template<typename... Args>
    TreeView::TreeView(const CreateParams& params, Args&&... args)
        :
        TreeViewBase{
            params,
            Orientation::Vertical,
            WordWrap::No, // no row breaks at the width it is given; before the caller's arguments
            std::forward<Args>(args)...
        }
    {
        connectActions();
    }

    template<typename... Args>
    TreeItem& TreeView::addItem(Args&&... args)
    {
        return add<TreeItem>(*this, std::size_t{ 0 }, std::forward<Args>(args)...);
    }

    template<typename... Args>
    TreeNode& TreeView::addNode(Args&&... args)
    {
        return createNode(*this, std::size_t{ 0 }, std::forward<Args>(args)...);
    }

    template<typename... Args>
    TreeDivider& TreeView::addDivider(Args&&... args)
    {
        return add<TreeDivider>(std::size_t{ 0 }, std::forward<Args>(args)...);
    }

    template<typename NodesContainer, typename... Args>
    TreeNode& TreeView::createNode(NodesContainer& container, const std::size_t level, Args&&... args)
    {
        TreeNode& node = container.template add<TreeNode>(*this, level, std::forward<Args>(args)...);
        m_nodes.push_back(&node);
        return node;
    }

}
