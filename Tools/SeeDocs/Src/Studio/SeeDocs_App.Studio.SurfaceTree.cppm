export module SeeDocs_App.Studio.SurfaceTree;

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
        TreePickEvent(const ContentsChapter&, const ContentsModule*, const Type*);
    public:
        const ContentsChapter& chapter;
        const ContentsModule* module;   // null for a chapter's row
        const Type* type;               // null for a chapter's or a module's row
    };

    // The surface as a tree of chapters, modules and types; a click on a row picks what it names.
    export class SurfaceTree : public TreeView
    {
    public:
        using TreeView::TreeView;
    public:
        // Connects a handler raised when a row is clicked.
        template<typename F>
        EventConnection onPick(F&& callback);
        // Builds the rows over the contents, in place of whatever stood there, the first chapter
        // open. The contents are held by reference for as long as the rows stand.
        void build(const ContentsChapters&);
        void clear();
        // Makes the row of the chapter, the module or the type - the nearest one named - the
        // current row, its ancestors opened, and scrolls it into view. Nothing named clears the
        // current row. Raises no pick.
        void follow(const ContentsChapter*, const ContentsModule*, const Type*);
    private:
        // A row and what it names: a chapter, a module of one, or a type of that.
        struct Entry
        {
            const ContentsChapter* chapter{ nullptr };
            const ContentsModule* module{ nullptr };
            const Type* type{ nullptr };
            TreeNode* node{ nullptr };      // a chapter's or a module's
            TreeItem* item{ nullptr };      // a type's
            std::size_t parent{ k_root };   // the entry whose node holds this row
        };
        using Entries = std::vector<Entry>;
        // The entry of a chapter, a module or a type, by the address of what it names.
        using EntryIndexes = std::unordered_map<const void*, std::size_t>;
    private:
        void pick(std::size_t index);
        [[nodiscard]] static Control& rowOf(const Entry&);
        [[nodiscard]] static Text rowText(const Type&);
    private:
        static constexpr float k_iconGap = 5.0f;   // between a row's icon and its text
        static constexpr std::size_t k_root = std::numeric_limits<std::size_t>::max();

        Entries m_entries{};
        EntryIndexes m_entryOf{};
    };


    //-------------------------------------------------------------------------


    template<typename F>
    EventConnection SurfaceTree::onPick(F&& callback)
    {
        return connectEvent<TreePickEvent>(std::forward<F>(callback));
    }
}
