export module SeeDocs_App.SurfaceTree;

import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.TreeView;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A row of the surface tree was clicked. The surface has an Event of its own.
    export class TreePickEvent : public ::ClaFi::Event
    {
    public:
        TreePickEvent(std::wstring_view, const ContentsModule*, const Type*);
    public:
        std::wstring_view folder;       // the category of the folder the row stands in
        const ContentsModule* module;   // null for a folder's row
        const Type* type;               // null for a folder's or a module's row
    };

    // The surface as a tree of folders, modules and types; a click on a row picks what it names.
    export class SurfaceTree : public TreeView
    {
    public:
        using TreeView::TreeView;
    public:
        // Connects a handler raised when a row is clicked.
        template<typename F>
        EventConnection onPick(F&& callback);
        // Builds the rows over the contents, in place of whatever stood there, every node closed.
        // The contents are held by reference for as long as the rows stand.
        void build(const ContentsChapters&);
        void clear();
        // Makes the row of the folder, the module or the type - the nearest one named - the
        // current row, its ancestors opened, and scrolls it into view. Nothing named clears the
        // current row. Raises no pick.
        void follow(std::wstring_view folder, const ContentsModule*, const Type*);
    private:
        // A row and what it names: a folder, a module of one, or a type of that.
        struct Entry
        {
            std::wstring_view folder{};     // the category of the folder the row stands in
            const ContentsModule* module{ nullptr };
            const Type* type{ nullptr };
            TreeNode* node{ nullptr };      // a folder's, or a module's that exports a type
            TreeItem* item{ nullptr };      // a type's, or a module's that exports none
            std::size_t parent{ k_root };   // the entry whose node holds this row
        };
        using Entries = std::vector<Entry>;
        // The entry of a module or a type, by the address of what it names.
        using EntryIndexes = std::unordered_map<const void*, std::size_t>;
        // The entry of a folder, by its category.
        using FolderIndexes = std::unordered_map<std::wstring, std::size_t>;
    private:
        // The folder's entry, made with every folder above it that is not there yet.
        std::size_t addFolder(std::wstring_view category);
        void addModules(const ContentsChapter&);
        [[nodiscard]] std::optional<std::size_t> entryOf(std::wstring_view folder,
            const ContentsModule*, const Type*) const;
        void pick(std::size_t index);
        [[nodiscard]] static Control& rowOf(const Entry&);
        [[nodiscard]] static HeaderText folderText(std::wstring_view category);
        [[nodiscard]] static Text rowText(const Type&);
    private:
        static constexpr float k_iconGap = 5.0f;   // between a row's icon and its text
        static constexpr wchar_t k_folderSeparator = L'/';   // between a category's folders
        static constexpr std::size_t k_root = std::numeric_limits<std::size_t>::max();

        Entries m_entries{};
        EntryIndexes m_entryOf{};
        FolderIndexes m_folderOf{};   // its keys are what the folders' entries view
    };


    //-------------------------------------------------------------------------


    template<typename F>
    EventConnection SurfaceTree::onPick(F&& callback)
    {
        return connectEvent<TreePickEvent>(std::forward<F>(callback));
    }
}
