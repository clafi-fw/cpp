export module ClaFi.Controls.TreeView;

import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
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
    }

    export class TreeView;

    // A row of a tree a pick can land on, its text past the mark slot every level keeps clear.
    export class TreeItem : public ToolButton
    {
    public:
        template<typename... Args>
        explicit TreeItem(const CreateParams&, std::size_t level, Args&&...);
    protected:
        ScaledDimensions calculateContent(AlignEvent&) override;
        FloatRect iconRect(const PaintEvent&) const override;
        void adjustTextRect(AdjustTextRectEvent&) const override;
    private:
        [[nodiscard]] float textLead(const Scaler&) const; // where the text begins past the padding
    private:
        std::size_t m_level;
    };

    // A branch of a tree: a header that is a row of its own, and under it the rows its mark opens.
    export class TreeNode : public Expander
    {
    public:
        template<typename... Args>
        explicit TreeNode(const CreateParams&, TreeView& tree, std::size_t level, Args&&...);
    public:
        // The rows under the header.
        [[nodiscard]] Stack& rows() { return m_rows; }
        [[nodiscard]] const Stack& rows() const { return m_rows; }
        // Adds a row a pick can land on, one level in.
        template<typename... Args>
        TreeItem& addItem(Args&&...);
        // Adds a branch one level in.
        template<typename... Args>
        TreeNode& addNode(Args&&...);
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
        Stack& m_rows;
    };

    // Rows nested in branches, each level stepped in one mark slot; a node's header is a row too.
    export class TreeView : public StackView
    {
        friend TreeNode;
    public:
        template<typename... Args>
        explicit TreeView(const CreateParams&, Args&&...);
    public:
        std::wstring_view diagnosticText() const override { return L"TreeView"; }
        // Adds a row a pick can land on, at the top level.
        template<typename... Args>
        TreeItem& addItem(Args&&...);
        // Adds a branch at the top level.
        template<typename... Args>
        TreeNode& addNode(Args&&...);
        // Opens every node.
        void expandAll();
        // Closes every node.
        void collapseAll();
        // Closes every node but the current row's own and the ones it stands under.
        void collapseOthers();
    protected:
        bool defaultCanFocusItem(Control&) override;
        void nestedContextPopup(ContextPopupEvent&) override;
        void nestedControlDeleted(Control*) override;
    private:
        using Nodes = std::vector<TreeNode*>;
    private:
        // Builds a node into the rows given and keeps it on the list every bulk operation walks.
        template<typename... Args>
        TreeNode& createNode(Stack& rows, std::size_t level, Args&&...);
        void connectActions();
        // Whether the control stands in this tree, in the rows of a node that does, or is the
        // header of such a node.
        [[nodiscard]] bool isRow(const Control&) const;
        // Whether the node is the current row's own, or holds the current row in its rows.
        [[nodiscard]] bool holdsCurrentRow(const TreeNode&) const;
        [[nodiscard]] bool hasNode(bool expanded) const;
        [[nodiscard]] bool hasOtherNodeOpen() const;
        // Brings the current row back after the nodes have moved - or, where it is folded away,
        // the header of the outermost closed node over it.
        void showCurrentRow();
    private:
        Nodes m_nodes{}; // every node of the tree, each after the one holding it
        Action m_expandAll{ Text{ L"Expand all" } };
        Action m_collapseAll{ Text{ L"Collapse all" } };
        Action m_collapseOthers{ Text{ L"Collapse others" } };
    };


    //----------------------------------------------------------------------------


    // TreeItem

    template<typename... Args>
    TreeItem::TreeItem(const CreateParams& params, const std::size_t level, Args&&... args)
        :
        ToolButton{
            params,
            HorizontalTextAnchor::Left,
            ShowSelectionOnSurface::Yes,
            Padding{ TreeRows::inset, params.themeMetrics().button.padding.y },
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
            ExpanderViewMode::TreeNode,
            VerticalAlign::Top, // a panel filling its slot does not grow to hold its body
            Padding{ 0.0f },    // the page's padding would add up level after level
            std::forward<Args>(args)...
        },
        m_tree{ tree },
        m_level{ level },
        m_rows{ createBody<Stack>(Orientation::Vertical) }
    {
        layOutHeader(params.themeMetrics().button);
    }

    template<typename... Args>
    TreeItem& TreeNode::addItem(Args&&... args)
    {
        return m_rows.add<TreeItem>(m_level + 1, std::forward<Args>(args)...);
    }

    template<typename... Args>
    TreeNode& TreeNode::addNode(Args&&... args)
    {
        return m_tree.createNode(m_rows, m_level + 1, std::forward<Args>(args)...);
    }

    // TreeView

    template<typename... Args>
    TreeView::TreeView(const CreateParams& params, Args&&... args)
        :
        StackView{ params, Orientation::Vertical, std::forward<Args>(args)... }
    {
        connectActions();
    }

    template<typename... Args>
    TreeItem& TreeView::addItem(Args&&... args)
    {
        return add<TreeItem>(std::size_t{ 0 }, std::forward<Args>(args)...);
    }

    template<typename... Args>
    TreeNode& TreeView::addNode(Args&&... args)
    {
        return createNode(*this, std::size_t{ 0 }, std::forward<Args>(args)...);
    }

    template<typename... Args>
    TreeNode& TreeView::createNode(Stack& rows, const std::size_t level, Args&&... args)
    {
        TreeNode& node = rows.add<TreeNode>(*this, level, std::forward<Args>(args)...);
        m_nodes.push_back(&node);
        return node;
    }

}
