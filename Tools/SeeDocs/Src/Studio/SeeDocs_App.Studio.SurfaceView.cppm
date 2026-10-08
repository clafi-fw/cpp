export module SeeDocs_App.Studio.SurfaceView;

import SeeDocs_App.Studio.PageView;
import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.TreeView;
import ClaFi.Controls.Base.StackBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The surface down the left as a tree of chapters, modules and types, and on the right the
    // page of whichever row is picked - a chapter's, a module's or a type's.
    export class SurfaceView : public Panel
    {
    public:
        template<typename... Args>
        explicit SurfaceView(const CreateParams&, Args&&...);
    public:
        // Builds the tree over the surface, in place of whatever stood there, and picks its first
        // chapter, open. Both are held by reference for as long as they are shown. False where the
        // surface lists nothing: the tree stands empty and the page is the caller's to fill.
        [[nodiscard]] bool bind(const Surface&, Notes&);
        // Shows the page of the type or module named, or of the method a member link names,
        // and picks the type or module in the tree, on the next tick - the request may come from
        // the page about to go. False for a name the surface does not carry.
        bool showNamed(std::wstring_view name);
        // Shows words of the studio's own in place of a page, the tree emptied.
        void showText(const Text&);
    private:
        // A row of the tree and what it opens: a chapter, a module of one, or a type of that.
        struct Entry
        {
            const ContentsChapter* chapter{ nullptr };
            const ContentsModule* module{ nullptr };
            const Type* type{ nullptr };
            TreeNode* node{ nullptr };   // a chapter's or a module's
            TreeItem* item{ nullptr };   // a type's
            std::size_t parent{ k_root };   // the entry whose node holds this row
        };
        using Entries = std::vector<Entry>;
        using EntryIndexes = std::unordered_map<std::wstring, std::size_t>;
    private:
        void clearTree();
        void buildTree();
        void show(std::size_t index);
        void showPending();
        [[nodiscard]] static Control& rowOf(const Entry&);
        [[nodiscard]] static Text rowText(const Type&);
        [[nodiscard]] static bool hasMethod(const Entry&, std::wstring_view name);
    private:
        static constexpr float k_treeWidth = 300.0f;
        static constexpr float k_iconGap = 5.0f; // between a row's icon and its text
        static constexpr std::size_t k_root = std::numeric_limits<std::size_t>::max();

        const Surface* m_surface{ nullptr };
        Notes* m_notes{ nullptr };
        ContentsChapters m_contents{};
        Entries m_entries{};
        EntryIndexes m_entryByName{};   // types by qualified name, modules by name
        std::size_t m_pending{ k_root }; // the entry showNamed is to show
        std::wstring m_member{};         // the method of its type to show; empty for the type
        UiTimer m_showTimer{};

        TreeView& m_tree{ createLeftBar<ScrollBox>(
            ScrollBars::Vertical,
            UiElement::Section,
            MinSize{ k_treeWidth, 0.0f },
            MaxSize{ k_treeWidth, k_maxFloat }
        ).createBody<TreeView>(
            Padding{ 4.0f }
        ) };

        PageView& m_page{ createBody<PageView>() };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    SurfaceView::SurfaceView(const CreateParams& params, Args&&... args)
        :
        Panel{ params, std::forward<Args>(args)... }
    {
        m_tree.onCurrentItemChange([this](CurrentItemChangeEvent&) {
            if (const Control* item = m_tree.currentItem())
                show(item->tag<std::size_t>());
        });
        m_page.onOpenName([this](OpenNameEvent& event) {
            showNamed(event.name);
        });
        m_showTimer.onTick([this](TimerEvent&) {
            showPending();
        });
    }
}
