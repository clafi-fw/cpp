export module SeeDocs_App.SurfaceTree;

import SeeDocs_App.Pages;
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

    // A project's options page, as its row and its crumb name it.
    export constexpr std::wstring_view k_optionsTitle = L"Project options";

    // What a row of the surface tree opens.
    export enum class RowKind
    {
        Home,       // the project's own node: its home page
        Options,    // the project's options page
        Contents    // a folder, a module or a type of its surface
    };

    // A row of the surface tree was clicked. The surface has an Event of its own.
    export class TreePickEvent : public ::ClaFi::Event
    {
    public:
        TreePickEvent(ProjectId, RowKind, std::wstring_view, const ContentsModule*, const Type*);
    public:
        ProjectId project;              // the project the row stands in
        RowKind kind;
        std::wstring_view folder;       // the path of the folder the row stands in
        const ContentsModule* module;   // null for a folder's row
        const Type* type;               // null for a folder's or a module's row
    };

    // A node of the surface tree was opened or closed, by the user or by a follow.
    export class TreeToggleEvent : public ::ClaFi::Event
    {
    public:
        explicit TreeToggleEvent(ProjectId);
    public:
        ProjectId project;
    };

    // The nodes of a project open in a tree: its own, its folders by path and its modules by
    // name.
    export struct OpenRows
    {
        bool home{ false };
        std::vector<std::wstring> folders{};
        std::vector<std::wstring> modules{};
    };

    // THE OPEN PROJECTS AS A TREE, a node each: its options row and a line, then its folders,
    // modules and types, read off the project's contents tree, which every tree shares. A node's
    // rows are made the first time it opens, so a tree costs the rows it has shown. A click on a
    // row picks what it names.
    export class SurfaceTree : public Tree
    {
    public:
        using Tree::Tree;
    public:
        // Connects a handler raised when a row is clicked.
        template<typename F>
        EventConnection onPick(F&& callback);
        // Connects a handler raised when a node opens or closes, other than by restoreRows.
        template<typename F>
        EventConnection onToggle(F&& callback);
        // Adds the project's node at the end of the tree, closed. The contents are held by
        // reference for as long as the rows stand.
        void addProject(ProjectId, std::wstring_view name, const ContentsTree&);
        // Builds the project's rows again over the contents, the nodes named open - asked of the
        // tree before the contents changed - open again.
        void rebuildProject(ProjectId, const ContentsTree&, const OpenRows&);
        void renameProject(ProjectId, std::wstring_view name);
        // Takes the project's node out, with every row under it.
        void removeProject(ProjectId);
        // Makes the row of the folder, the module or the type - the nearest one named - the
        // current row, the nodes over it opened, and scrolls it into view. Nothing named clears
        // the current row. Raises no pick.
        void follow(ProjectId, std::wstring_view folder, const ContentsModule*, const Type*);
        void followHome(ProjectId);      // the project's own node, the same way
        void followOptions(ProjectId);   // the project's options row, the same way
        // The project's nodes standing open, a node folded away under a closed one included.
        [[nodiscard]] OpenRows openRows(ProjectId) const;
        // Opens the project's nodes named, each that names a row; raises no toggle.
        void restoreRows(ProjectId, const OpenRows&);
    private:
        // The controls a row of the contents is shown by, none until its holder's rows are made.
        struct MadeRow
        {
            TreeNode* node{ nullptr };   // a folder's, or a module's that exports a type
            TreeItem* item{ nullptr };   // a type's, or a module's that exports none
            bool filled{ false };        // a node's rows are made
        };
        using MadeRows = std::vector<MadeRow>;
        // One project's rows: the heavy line over its node where a project stands above it, the
        // node, its options row and the line under it, and a made row per row of its contents.
        struct ProjectRows
        {
            ProjectId id{};
            const ContentsTree* contents{ nullptr };
            TreeDivider* above{ nullptr };   // none for the first project
            TreeNode* node{ nullptr };
            TreeItem* options{ nullptr };   // made with the rows of the node
            TreeDivider* divider{ nullptr };
            std::vector<Control*> topRows{};   // the rows of the contents under the node
            MadeRows made{};
        };
        // Held by pointer: a row's click handler holds its project's rows.
        using Projects = std::vector<std::unique_ptr<ProjectRows>>;
    private:
        [[nodiscard]] ProjectRows* rowsOf(ProjectId) const;
        // Makes the rows under the project's node: the options row, the line and the top rows.
        void fillProject(ProjectRows&);
        // Makes the rows under a node of the contents.
        void fillRow(ProjectRows&, std::size_t index);
        void makeRow(ProjectRows&, std::size_t index, TreeNode& holder);
        // Makes the controls of the row and of every row over it, opening none of them.
        void ensureRow(ProjectRows&, std::size_t index);
        void clearRows(ProjectRows&);
        // A node the user opens or closes says so, a node opening with no rows asks for them.
        void connectNode(ProjectRows&, TreeNode&, std::optional<std::size_t> index);
        void pick(const ProjectRows&, std::size_t index);
        void pickRow(ProjectId, RowKind);   // the project's own node or its options row
        // Makes the row current, the project's node opened over it, and scrolls it into view.
        void showRow(ProjectRows&, Control& row);
        [[nodiscard]] bool holdsCurrentRow(const ProjectRows&) const;
        [[nodiscard]] static Control& rowOf(const MadeRow&);
        [[nodiscard]] static HeaderText projectText(std::wstring_view name);
        [[nodiscard]] static HeaderText folderText(std::wstring_view folder);
        [[nodiscard]] static Text rowText(const Type&);
    private:
        static constexpr float k_iconGap = 5.0f;   // between a row's icon and its text

        Projects m_projects{};
        bool m_restoring{ false };   // opening nodes the user did not open
    };


    //-------------------------------------------------------------------------


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
