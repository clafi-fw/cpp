export module SeeDocs_App.SidebarEditor;

import SeeDocs_App.Icons;
import SeeDocs_App.Project;

import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.SplitButton;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Tree;
import ClaFi.Controls.Base.StackBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The sidebar was edited on its page: what it is to be. The surface has an Event of its own.
    export class SidebarEditEvent : public ::ClaFi::Event
    {
    public:
        explicit SidebarEditEvent(SidebarItems);
    public:
        SidebarItems items;
    };

    // **A project's sidebar as its editor**: the items as a tree under the project's own node,
    // which always stands; Add, dropping the kinds, and Delete over the selected item, in the bar
    // the editor is given; the selected item's kind picked at the right; a name typed where it
    // stands; and the items under one node carried into another order. Every edit says what the
    // sidebar is to be, and the editor shows the items as they read once it is written. See the
    // README beside the project.
    export class SidebarEditor : public Panel
    {
    public:
        template<typename... Args>
        explicit SidebarEditor(const CreateParams&, std::wstring_view projectName,
            const SidebarItems&, Args&&...);
    public:
        // Connects a handler raised when an edit says what the sidebar is to be.
        template<typename F>
        EventConnection onEdit(F&& callback);
        // Puts Add and Delete into the bar, which the editor reads the selected item for.
        void addCommands(StackPanel& bar);
        // Shows the items as they now read, once whatever asked for it has unwound.
        void reload();
    protected:
        void nestedKeyDown(KeyDownEvent&) override;   // Delete takes the selected item out
    private:
        // Where an item stands: its index in each list down from the top.
        using IndexPath = std::vector<std::size_t>;
        // A row of the editor's tree, and the item it stands for.
        struct Row
        {
            IndexPath path{};
            TreeNode* node{ nullptr };   // a category's
            TreeItem* row{ nullptr };    // a page's, an All the APIs item's, or a divider's
        };
        using Rows = std::vector<Row>;
        class CategoryHeader;
        class ItemRow;
        class DividerRow;
        class AddButton;
    private:
        // Adds the project's own node to the tree: its mark, its name, and the carry for its rows.
        [[nodiscard]] TreeNode& addRoot(std::wstring_view projectName);
        void connectKinds();   // the kind set, each kind an answer of it
        void addKind(SidebarKind, std::wstring_view caption);
        // Makes the rows under the node for the items, each knowing where it stands.
        void buildRows(const SidebarItems&, TreeNode& holder, const IndexPath& under);
        void rebuildRows();   // over the items as they now read, the selection kept
        // The row the control is or stands in; null for the project's own node, or none.
        [[nodiscard]] const Row* rowOf(const Control*) const;
        // The row a child of a node's rows is: a category's node, or any other item's row.
        [[nodiscard]] const Row* rowOfChild(const Control*) const;
        [[nodiscard]] const Row* selectedRow() const;
        [[nodiscard]] const Row* rowAt(const IndexPath&) const;
        [[nodiscard]] const SidebarItem* selectedItem() const;
        // The list the path's last index counts in, and the item it names - null where the items
        // no longer reach that far, the rows standing until the tick that builds them again.
        [[nodiscard]] static SidebarItems& listAt(SidebarItems&, const IndexPath&);
        [[nodiscard]] static const SidebarItem* itemAt(const SidebarItems&, const IndexPath&);
        // A name no item of the list goes by: the kind's, numbered where that is taken.
        [[nodiscard]] static std::wstring newName(SidebarKind, const SidebarItems&);
        // Why the name cannot be an item's here; empty where it can.
        [[nodiscard]] static std::wstring refusalOf(std::wstring_view name,
            const SidebarItems& siblings, const SidebarItem* self);
        void addItem(SidebarKind);   // under the selected category, else after the selected item
        void deleteSelected();
        [[nodiscard]] bool canDelete() const;
        // Whether the item may be of the kind: a category holding items keeps its kind.
        [[nodiscard]] bool canBeKind(SidebarKind) const;
        void setKind(SidebarKind);
        // Takes the name typed for the item, or refuses it with the reason.
        void acceptName(const IndexPath&, AcceptEditEvent&);
        // The items under one node were carried into another order.
        void itemsMoved(const IndexPath& under, StackPanel& rows);
        // Says what the sidebar is to be, and shows it read back where the shape changed.
        void edit(SidebarItems, bool rebuild);
        void selectionChanged();
        [[nodiscard]] static std::wstring_view kindCaption(SidebarKind);
    private:
        static constexpr float k_treeWidth = 340.0f;    // the bar the tree stands in
        static constexpr float k_treePadding = 4.0f;
        static constexpr float k_kindsPadding = 12.0f;  // between the tree and the kind set
        static constexpr float k_choiceIndent = 20.0f;  // the kinds in from their caption

        const SidebarItems& m_items;   // the project's, read whenever the rows are built
        Rows m_rows{};
        std::optional<IndexPath> m_select{};   // the row to select once the rows are built
        bool m_renameNext{ false };   // the row selected then opens its editor
        UiTimer m_rebuildTimer{};
        Button* m_deleteButton{ nullptr };
        ScrollBox& m_treeBox{ createLeftBar<ScrollBox>(
            PreferredSize{ k_treeWidth, 0.0f },
            ScrollBars::Vertical,
            UiElement::Section
        ) };
        TreeView& m_tree{ m_treeBox.createBody<TreeView>(Padding{ k_treePadding }) };
        TreeNode& m_root;   // the project's own node, which the items stand under
        StackPanel& m_kinds{ createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ k_kindsPadding, 0.0f }
        ) };
        StackPanel& m_kindSet{ m_kinds.add<StackPanel>(
            Orientation::Vertical,
            Padding{ k_choiceIndent, 0.0f }
        ) };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    SidebarEditor::SidebarEditor(const CreateParams& params, const std::wstring_view projectName,
        const SidebarItems& items, Args&&... args)
        :
        Panel{
            params,
            Padding{ 0.0f },
            std::forward<Args>(args)...
        },
        m_items{ items },
        m_root{ addRoot(projectName) }
    {
        connectKinds();
        m_rebuildTimer.onTick([this](TimerEvent&) {
            rebuildRows();
        });
        m_tree.onCurrentItemChange([this](CurrentItemChangeEvent&) {
            selectionChanged();
        });
        m_root.rows().onItemsMove([this](ItemsMoveEvent&) {
            itemsMoved({}, m_root.rows());
        });
        rebuildRows();
    }

    template<typename F>
    EventConnection SidebarEditor::onEdit(F&& callback)
    {
        return connectEvent<SidebarEditEvent>(std::forward<F>(callback));
    }
}
