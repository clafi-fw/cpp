export module SeeDocs_App.SurfaceTree;

import SeeDocs_App.Pages;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.Controls.Tree;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
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

    // The nodes of a project open in a tree: its own, its sidebar's items by path, and its
    // folders by path and its modules by moduleLink, each under the path of the item it hangs from.
    export struct OpenRows
    {
        bool home{ false };
        std::vector<std::wstring> items{};
        std::vector<std::wstring> folders{};   // the item's path, a slash, the folder's
        std::vector<std::wstring> modules{};   // the item's path, a slash, the module's link
    };

    // **The open projects as a tree**, a node each holding its sidebar's items - a category the
    // items under it, an All the APIs item the folders, modules and types read off the project's
    // contents tree, which every tree shares. A node's rows are made the first time it opens, so a
    // tree costs the rows it has shown. A click on a row picks what it names, and Open in new tab
    // on the row's menu picks it for a new tab.
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
        // Holds the row the menu is asked for while it runs, so Open in new tab is about that row.
        void nestedContextPopup(ContextPopupEvent&) override;
        // Open in new tab leads the tree's own commands on a row a pick opens.
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
            TreeItem* row{ nullptr };      // a page's
            TreeDivider* line{ nullptr };  // a divider's
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
    private:
        void connectActions();   // claims Open in new tab while a menu is up on a row
        [[nodiscard]] ProjectRows* rowsOf(ProjectId) const;
        // The sidebar's items in reading order, each knowing the category it stands in.
        [[nodiscard]] static ItemsRows flattenItems(const SidebarItems&);
        [[nodiscard]] static std::optional<std::size_t> itemOf(const ProjectRows&,
            std::wstring_view path);
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
        [[nodiscard]] static Text itemText(const SidebarItem&);
    private:
        static constexpr float k_iconGap = 5.0f;   // between a row's icon and its text

        Projects m_projects{};
        bool m_restoring{ false };   // opening nodes the user did not open
        std::optional<MenuRow> m_menuRow{};   // the row the menu that is up is about
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    SurfaceTree::SurfaceTree(const CreateParams& params, Args&&... args)
        :
        TreeView{ params, std::forward<Args>(args)... }
    {
        connectActions();
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
}
