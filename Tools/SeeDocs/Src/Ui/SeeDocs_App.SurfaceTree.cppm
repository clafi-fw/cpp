export module SeeDocs_App.SurfaceTree;

import SeeDocs_App.Pages;
import SeeDocs_App.Project;
import SeeDocs_App.SidebarEdits;
import SeeDocs_App.Surface;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Tree;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What tells the projects of the tree apart, given by whoever adds them.
    export using ProjectId = std::size_t;

    // What a row of the surface tree opens.
    export enum class RowKind
    {
        Home,       // the project's own node: its home page
        Item,       // an item of its sidebar: a category, an All the APIs item or a page
        Contents    // a folder, a module or a type of its surface, under an All the APIs item
    };

    // Which tab the page a picked row names opens in.
    export enum class PickTab
    {
        Shown,   // the tab shown goes to it
        New      // a new tab, behind the one shown
    };

    // A row of the surface tree was picked. The surface has an Event of its own.
    export class TreePickEvent : public ::ClaFi::Event
    {
    public:
        TreePickEvent(ProjectId, RowKind, std::wstring_view item, std::wstring_view folder,
            const ContentsModule*, const Type*, PickTab);
    public:
        ProjectId project;              // the project the row stands in
        RowKind kind;
        // The path of the sidebar item the row is, or stands under: its names from the top, a
        // slash between; empty for the home.
        std::wstring_view item;
        std::wstring_view folder;       // the path of the folder the row stands in
        const ContentsModule* module;   // null for a folder's row
        const Type* type;               // null for a folder's or a module's row
        PickTab tab;
    };

    // A node of the surface tree was opened or closed, by the user or by a follow.
    export class TreeToggleEvent : public ::ClaFi::Event
    {
    public:
        explicit TreeToggleEvent(ProjectId);
    public:
        ProjectId project;
    };

    // A project's sidebar was edited in the tree: what it is to be.
    export class TreeEditEvent : public ::ClaFi::Event
    {
    public:
        TreeEditEvent(ProjectId, SidebarItems);
    public:
        ProjectId project;
        SidebarItems items;
    };

    // The commands a sidebar row's menu holds. Constructed before main, so nothing here touches
    // the platform, the theme or the text engine. See README
    namespace Actions
    {
        export extern Action addItem;      // opens the kinds an item is added as
        export extern Action addCategory;
        export extern Action addDivider;
        export extern Action addApis;
        export extern Action addPage;
        export extern Action deleteItem;
        export extern Action setKind;      // opens the kinds the item can be made
        export extern Action beCategory;
        export extern Action beDivider;
        export extern Action beApis;
        export extern Action bePage;
        // Puts the kinds under Add and under Kind, once.
        export void prepare();
    }

    // The nodes of a project open in a tree: its own, its sidebar's items by path, and its
    // folders by path and its modules by moduleLink, each under the path of the item it hangs from.
    export struct OpenRows
    {
        bool home{ false };
        std::vector<std::wstring> items{};
        std::vector<std::wstring> folders{};   // the item's path, a slash, the folder's
        std::vector<std::wstring> modules{};   // the item's path, a slash, the module's link
    };

    // The kind an Add line and a Kind line stand for.
    struct KindAction
    {
        SidebarKind kind;
        Action* add;
        Action* be;
    };

    // **The open projects as a tree**, a node each holding its sidebar's items - a category the
    // items under it, an All the APIs item the folders, modules and types read off the project's
    // contents tree, which every tree shares. A node's rows are made the first time it opens, so a
    // tree costs the rows it has shown. A click on a row picks what it names, and Open in new tab
    // on the row's menu picks it for a new tab. **The sidebar is edited where it stands**: the
    // items under one node carried into another order, a row's menu adding, renaming and deleting
    // items and making one another kind, a name typed in place. Every edit says what the sidebar
    // is to be, and the tree shows it once it is written. See README
    export class SurfaceTree : public TreeView
    {
    public:
        template<typename... Args>
        explicit SurfaceTree(const CreateParams&, Args&&...);
    public:
        // Connects a handler raised when a row is clicked, or opened in a new tab from its menu.
        template<typename F>
        EventConnection onPick(F&& callback);
        // Connects a handler raised when a node opens or closes, other than by restoreRows.
        template<typename F>
        EventConnection onToggle(F&& callback);
        // Connects a handler raised when an edit says what a project's sidebar is to be.
        template<typename F>
        EventConnection onEdit(F&& callback);
        // Adds the project's node at the end of the tree, closed. The sidebar and the contents are
        // held by reference for as long as the rows stand.
        void addProject(ProjectId, std::wstring_view name, const SidebarItems&,
            const ContentsTree&);
        // Builds the project's rows again over the sidebar and the contents, the nodes named open -
        // asked of the tree before they changed - open again.
        void rebuildProject(ProjectId, const SidebarItems&, const ContentsTree&, const OpenRows&);
        void renameProject(ProjectId, std::wstring_view name);
        // Takes the project's node out, with every row under it.
        void removeProject(ProjectId);
        // Puts the projects' nodes in this order, one not named after those that are.
        void orderProjects(std::span<const ProjectId>);
        // Makes the row of the folder, the module or the type - the nearest one named - under the
        // item the path names the current row, the nodes over it opened, and scrolls it into view;
        // with none named, the item's own row. Nothing named makes the project's own node current.
        // Raises no pick.
        void follow(ProjectId, std::wstring_view item, std::wstring_view folder,
            const ContentsModule*, const Type*);
        void followHome(ProjectId);   // the project's own node, the same way
        // The project's nodes standing open, a node folded away under a closed one included.
        [[nodiscard]] OpenRows openRows(ProjectId) const;
        // Opens the project's nodes named, each that names a row; raises no toggle.
        void restoreRows(ProjectId, const OpenRows&);
    protected:
        // Holds the row the menu is asked for while it runs, so the commands are about that row.
        void nestedContextPopup(ContextPopupEvent&) override;
        // Open in new tab leads the tree's own commands on a row a pick opens; a sidebar row's
        // edits follow - Rename is the standard command, which the row answers - and the
        // project's node's Add.
        void adjustContextPopup(AdjustContextPopupEvent&) override;
    private:
        // The controls a row of the contents is shown by, none until its holder's rows are made.
        struct MadeRow
        {
            TreeNode* node{ nullptr };   // a folder's, or a module's that exports a type
            TreeItem* item{ nullptr };   // a type's, or a module's that exports none
            bool filled{ false };        // a node's rows are made
        };
        using MadeRows = std::vector<MadeRow>;
        // One item of the sidebar: the item, its path, the item it stands in, the controls it is
        // shown by once its holder's rows are made, and under an All the APIs item a made row per
        // row of the contents.
        struct ItemRows
        {
            const SidebarItem* item{ nullptr };
            std::wstring path{};   // its names from the top, a slash between
            std::optional<std::size_t> parent{};   // the category it stands in
            TreeNode* node{ nullptr };     // a category's, or an All the APIs item's
            TreeItem* row{ nullptr };      // a page's, or a divider's
            bool filled{ false };          // the node's rows are made
            MadeRows made{};
        };
        using ItemsRows = std::vector<ItemRows>;
        // One project's rows: the heavy line over its node where a project stands above it, the
        // node, and its sidebar's items in reading order, a category's items after it.
        struct ProjectRows
        {
            ProjectId id{};
            const SidebarItems* sidebar{ nullptr };
            const ContentsTree* contents{ nullptr };
            TreeDivider* above{ nullptr };   // none for the first project
            TreeNode* node{ nullptr };
            bool filled{ false };   // the node's rows are made
            ItemsRows items{};
        };
        // Held by pointer: a row's click handler holds its project's rows.
        using Projects = std::vector<std::unique_ptr<ProjectRows>>;
        // What a node fetches as it opens: the project's items, an item's, or a contents row's.
        struct Fetch
        {
            std::optional<std::size_t> item{};
            std::optional<std::size_t> contents{};
        };
        // The row a menu is up for, as a pick of it names it.
        struct MenuRow
        {
            const ProjectRows* rows{ nullptr };
            RowKind kind{ RowKind::Home };
            std::size_t item{ 0 };       // the sidebar item, for Item and Contents
            std::size_t contents{ 0 };   // the row of the contents, for Contents
        };
        // The row of a project to open for its name once the tree stands over the edit that
        // made it.
        struct RenameNext
        {
            ProjectId project{};
            IndexPath path{};
        };
    private:
        void connectActions();   // claims the commands while a menu is up on a row
        [[nodiscard]] ProjectRows* rowsOf(ProjectId) const;
        // The sidebar's items in reading order, each knowing the category it stands in.
        [[nodiscard]] static ItemsRows flattenItems(const SidebarItems&);
        [[nodiscard]] static std::optional<std::size_t> itemOf(const ProjectRows&,
            std::wstring_view path);
        // Where the item stands: its index in each list down from the top.
        [[nodiscard]] static IndexPath indexPathOf(const ProjectRows&, std::size_t item);
        [[nodiscard]] static std::optional<std::size_t> itemAtPath(const ProjectRows&,
            const IndexPath&);
        void fillProject(ProjectRows&);   // makes the rows under the project's node
        // Makes the rows under an item's node: a category's items, an All the APIs item's folders.
        void fillItem(ProjectRows&, std::size_t item);
        // Makes the rows under a node of the contents.
        void fillContentsRow(ProjectRows&, std::size_t item, std::size_t contents);
        void makeItemRow(ProjectRows&, std::size_t item, TreeNode& holder);
        void makeContentsRow(ProjectRows&, std::size_t item, std::size_t contents,
            TreeNode& holder);
        // Makes the controls of the row and of every row over it, opening none of them.
        void ensureItemRow(ProjectRows&, std::size_t item);
        void ensureContentsRow(ProjectRows&, std::size_t item, std::size_t contents);
        void clearRows(ProjectRows&);
        // A node the user opens or closes says so, a node opening with no rows asks for them.
        void connectNode(ProjectRows&, TreeNode&, Fetch);
        // The rows under the node take the carry, and say when they were carried.
        void connectCarry(ProjectRows&, TreeNode&, std::optional<std::size_t> item);
        void pickItem(const ProjectRows&, std::size_t item, PickTab);
        void pickContents(const ProjectRows&, std::size_t item, std::size_t contents, PickTab);
        void pickHome(ProjectId, PickTab);   // the project's own node
        // The row the control is or stands in, where it is one a pick opens.
        [[nodiscard]] std::optional<MenuRow> menuRowOf(const Control&) const;
        // Makes the row current, the nodes over it opened, and scrolls it into view.
        void showRow(ProjectRows&, std::size_t item, Control& row);
        [[nodiscard]] bool holdsCurrentRow(const ProjectRows&) const;
        [[nodiscard]] static Control* rowOf(const ItemRows&);
        [[nodiscard]] static Control& rowOf(const MadeRow&);
        [[nodiscard]] static HeaderText projectText(std::wstring_view name);
        [[nodiscard]] static HeaderText folderText(std::wstring_view folder);
        [[nodiscard]] static Text rowText(const Type&);
        // The sidebar item the menu is up for; null for any other row.
        [[nodiscard]] const SidebarItem* menuItem() const;
        [[nodiscard]] static const KindAction* kindOf(const Action&);
        // Runs the command on the row the menu is up for, once the menu has unwound.
        void runCommand(const Action&);
        void addItem(ProjectId, std::optional<std::size_t> item, SidebarKind);
        void deleteItem(ProjectId, std::size_t item);   // asks first for a category with items
        void setKind(ProjectId, std::size_t item, SidebarKind);
        // Takes the name typed for the item, or refuses it with the reason.
        void acceptName(ProjectId, std::size_t item, AcceptEditEvent&);
        // The items under the node were carried into another order.
        void itemsMoved(ProjectId, std::optional<std::size_t> item, StackPanel& rows);
        void openEditor(ProjectRows&, std::size_t item);   // the row's, for its name
        // Says what the sidebar is to be, once whatever asked has unwound.
        void edit(ProjectId, SidebarItems);
        void runPending();   // the edit or the command waiting on the tick
        void renameNext();   // opens the editor on the row an edit made
    private:
        static constexpr float k_iconGap = 5.0f;   // between a row's icon and its text

        Projects m_projects{};
        bool m_restoring{ false };   // opening nodes the user did not open
        std::optional<MenuRow> m_menuRow{};   // the row the menu that is up is about
        std::function<void()> m_pending{};    // what runs on the tick
        UiTimer m_pendingTimer{};
        std::optional<RenameNext> m_renameNext{};
        UiTimer m_renameTimer{};
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    SurfaceTree::SurfaceTree(const CreateParams& params, Args&&... args)
        :
        TreeView{ params, std::forward<Args>(args)... }
    {
        Actions::prepare();
        connectActions();
        m_pendingTimer.onTick([this](TimerEvent&) {
            runPending();
        });
        m_renameTimer.onTick([this](TimerEvent&) {
            renameNext();
        });
    }

    template<typename F>
    EventConnection SurfaceTree::onPick(F&& callback)
    {
        return connectEvent<TreePickEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection SurfaceTree::onToggle(F&& callback)
    {
        return connectEvent<TreeToggleEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection SurfaceTree::onEdit(F&& callback)
    {
        return connectEvent<TreeEditEvent>(std::forward<F>(callback));
    }
}
