export module SeeDocs_App.Studio.Browser;

import SeeDocs_App.Studio.Icons;
import SeeDocs_App.Studio.PageView;
import SeeDocs_App.Studio.SurfaceTree;
import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Icons.SideBar;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Url;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Browser;

    // What becomes of the open tabs when another surface is shown.
    export enum class OpenTabs
    {
        Keep,    // the tabs stand, their pages built again over the surface shown
        Close    // every tab goes, and one opens on the home page
    };

    export class SurfaceBrowser;

    // One page of the browser: a PageView over what the tab's url names.
    export class SurfacePage : public BrowserPage
    {
    public:
        template<typename... Args>
        explicit SurfacePage(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] PageView& view() const { return m_view; }
    private:
        // The browser the page is shown in. Structure rather than state, so const stops here.
        [[nodiscard]] SurfaceBrowser& browser() const;
        // Takes the tab where a link on the page points: an anchor of the page stays on it,
        // anything else names a page of the surface.
        void followLink(std::wstring_view target);
    private:
        PageView& m_view{ createBody<PageView>() };
    };

    // The browser over the surface: the tree down the left, a page per url on the right -
    // /folder/.../module/type/method, the folders as the tree stands on disk - and the studio's
    // words where there is no surface.
    export class SurfaceBrowser : public BrowserControl
    {
    public:
        template<typename... Args>
        explicit SurfaceBrowser(const CreateParams&, Args&&...);
    public:
        // Shows the surface: the tree over it, the home page named after the project. Both are
        // held by reference for as long as they are shown. False where the surface lists nothing:
        // the tree stands empty, and the home page shows the words given since.
        [[nodiscard]] bool bind(const Surface&, Notes&, std::wstring_view projectName, OpenTabs);
        // Shows words of the studio's own on the home page in place of a surface, the tree
        // emptied. Every tab goes, and one opens on home.
        void showWords(const Text&);
        // The url of what a link names - a type, a module, a chapter or a member - or nothing
        // where the surface does not carry it.
        [[nodiscard]] std::optional<Url> urlOf(std::wstring_view target) const;
    protected:
        void initPageData(PageData&) override;
        void fetchSubItems(PageData&) override;
        void showPage(BrowserTab&) override;
        IconSize pageIconSize(PageData&) override;
        void paintPageIcon(PageData&, PaintIconEvent&) override;
        IconSize tabIconSize(BrowserTab&) override;
        void paintTabIcon(BrowserTab&, PaintIconEvent&) override;
    private:
        // What a path names, read off its segments: the folders down from the root as far as
        // they go - home for none - then a module of the chapter the last folder is, a type of
        // that, and a method of the type. A segment nothing answers for makes the path one the
        // surface does not carry.
        struct Target
        {
            std::wstring folder{};              // the category the folder segments spell
            std::size_t folderDepth{ 0 };       // how many segments are folders
            const ContentsChapter* chapter{ nullptr };   // the folder's own chapter, where it is one
            const ContentsModule* module{ nullptr };
            const Type* type{ nullptr };
            std::wstring_view method{};
            std::size_t depth{ 0 };   // segments of the path
            [[nodiscard]] bool isHome() const { return depth == 0; }
            [[nodiscard]] bool isFolder() const { return depth == folderDepth; }
            [[nodiscard]] bool isGone() const;
        };
        // The chapter, module and type a link's spelling names, keyed by that spelling.
        using Targets = std::unordered_map<std::wstring, Target>;
        // Every folder on the way to a chapter, keyed by its category, with the chapter the
        // folder is - null for one that only holds other folders.
        using Folders = std::unordered_map<std::wstring, const ContentsChapter*>;
    private:
        [[nodiscard]] Target resolve(const PageData&) const;
        // The path of the nearest thing named, down to the method where one is given.
        [[nodiscard]] static std::wstring pathOf(const ContentsChapter*, const ContentsModule*,
            const Type*, std::wstring_view method = {});
        // The folders directly under this one, in reading order.
        [[nodiscard]] std::vector<std::wstring> foldersUnder(std::wstring_view folder) const;
        [[nodiscard]] std::wstring titleOf(const PageData&, const Target&) const;
        [[nodiscard]] static std::optional<RowIcon> iconOf(const Target&);
        // Fills a page with what the target names: the words while there is no surface, the
        // contents on home, and the words of a page the surface does not carry.
        void fill(SurfacePage&, const Target&, const PageData&) const;
        // Puts the title and the list of this page data and every one under it right for the
        // surface shown.
        void refreshPageData(PageData&);
        // Drops every tab's page and shows the selected tab's again, over the surface shown.
        void rebuildPages();
        [[nodiscard]] Text goneWords(const PageData&) const;
        [[nodiscard]] static bool hasMethod(const Type&, std::wstring_view name);
    private:
        static constexpr float k_treeWidth = 300.0f;
        static constexpr IconSize k_pageIconSize{ 16.0f };

        const Surface* m_surface{ nullptr };
        Notes* m_notes{ nullptr };
        ContentsChapters m_contents{};
        Targets m_targets{};
        Folders m_folders{};
        std::wstring m_projectName{};
        Text m_words{};   // what every page shows while there is no surface

        SurfaceTree& m_tree{ createLeftBar<ScrollBox>(
            ScrollBars::Vertical,
            UiElement::Section,
            MinSize{ k_treeWidth, 0.0f },
            MaxSize{ k_treeWidth, k_maxFloat }
        ).createBody<SurfaceTree>(
            Padding{ 4.0f }
        ) };
    };


    //-------------------------------------------------------------------------


    // SurfacePage

    template<typename... Args>
    SurfacePage::SurfacePage(const CreateParams& params, Args&&... args)
        :
        BrowserPage{
            params,
            Padding{ 0.0f },
            Spacing{ 0.0f },
            UiElement::Page,
            std::forward<Args>(args)...
        }
    {
        m_view.onPageLink([this](PageLinkEvent& event) {
            followLink(event.target);
        });
        onShowAnchor([this](ShowAnchorEvent& event) {
            m_view.showAnchor(event.url.anchor());
        });
    }

    // SurfaceBrowser

    template<typename... Args>
    SurfaceBrowser::SurfaceBrowser(const CreateParams& params, Args&&... args)
        :
        BrowserControl{ params, std::forward<Args>(args)... }
    {
        appButton().onPaintIcon(Icons::SideBar::paintPanelIcon);
        // The tree stands apart from the page a move takes down, so the move needs no wait.
        m_tree.onPick([this](TreePickEvent& event) {
            goTo(Url{ pathOf(&event.chapter, event.module, event.type) });
        });
    }
}
