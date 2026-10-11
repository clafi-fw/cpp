export module SeeDocs_App.Browser;

import SeeDocs_App.Editing;
import SeeDocs_App.Icons;
import SeeDocs_App.PageView;
import SeeDocs_App.SurfaceTree;
import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Project;
import SeeDocs_App.Search;
import SeeDocs_App.Surface;

import ClaFi.App.Settings;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.DropList;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.SearchBox;
import ClaFi.Controls.Tree;
import ClaFi.Icons.SideBar;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Url;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Browser;

    // A project open in the window: what it is, the surface read out of its database, and the
    // notes its comments reach. The workspace owns it, and the browser shows it.
    export struct OpenProject
    {
        OpenProject(Project, Surface);
        Project project;
        Surface surface;
        Notes notes;   // the project's footnotes folder
    };

    // Whether showing a project opens a tab on it.
    export enum class HomeTab
    {
        Open,      // a new tab on the project's home, selected
        Options,   // a new tab on the project's options page, selected
        None       // the tabs stand as they are, a restored one reaching the project by its url
    };

    // The tab shown stands in another project, or in none. The surface has an Event of its own.
    export class ProjectShownEvent : public ::ClaFi::Event
    {
    public:
        explicit ProjectShownEvent(const OpenProject*);
    public:
        const OpenProject* project;   // null where the tab stands in no project
    };

    // A page asked for a project to be closed - its own, or one the root lists. The browser
    // shows the project until whoever owns it takes it out. The surface has an Event of its own.
    export class CloseProjectEvent : public ::ClaFi::Event
    {
    public:
        explicit CloseProjectEvent(OpenProject&);
    public:
        OpenProject& project;
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
    protected:
        std::wstring pathToOpen() override;
    private:
        // The browser the page is shown in. Structure rather than state, so const stops here.
        [[nodiscard]] SurfaceBrowser& browser() const;
        // Takes the tab where a link on the page points: an anchor of the page stays on it,
        // anything else names a page of the surface.
        void followLink(std::wstring_view target);
        void runCommand(std::wstring_view target);   // hands a command's click to the browser
    private:
        PageView& m_view{ createBody<PageView>() };
    };

    // A project's options as a page of the browser: the head every page wears, and under it the
    // options, filled by the application.
    export class ProjectPage : public BrowserPage
    {
    public:
        template<typename... Args>
        explicit ProjectPage(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] OptionsPage& options() { return m_optionsBox.body(); }
    private:
        PageHead& m_head{ createTopBar<PageHead>() };
        ScrollBoxWith<OptionsPage>& m_optionsBox{ createBody<ScrollBoxWith<OptionsPage>>(
            HostProps{
                ScrollBars::Vertical
            },
            BodyProps<>{}
        ) };
    };

    // What a project's options page is filled with.
    export using ProjectPageBuilder = std::function<void(OptionsPage&, OpenProject&)>;

    // The browser over the open projects: down the left the tab's own tree, a node per project,
    // and a page per url on the right - /project/folder/.../module/type/method, the folders as
    // the sources stand on disk, and the root listing the projects - with SeeDocs' words while
    // none is open. What a project's pages edit is written through the browser into that project,
    // and a project's closing, which its home and the root offer, is asked of whoever owns it. A
    // tab's tree is made the first time the tab is shown, and the nodes it has open are kept
    // with the tab. A search is typed at the crumb bar's end. See the README beside the project.
    export class SurfaceBrowser : public BrowserControl
    {
    public:
        template<typename... Args>
        explicit SurfaceBrowser(const CreateParams&, Args&&...);
    public:
        // Connects a handler raised when the tab shown stands in another project than before.
        template<typename F>
        EventConnection onProjectShown(F&& callback);
        // Connects a handler raised when a page asks for a project to be closed.
        template<typename F>
        EventConnection onCloseProject(F&& callback);
        // Shows a project opened: a node of its own at the end of the tree, and its pages under a
        // segment of its name. The project is held by reference until it is removed, and what its
        // pages edit is written into it - its tree and its database - unless it is read in
        // limited mode.
        void addProject(OpenProject&, HomeTab);
        void openHome(const OpenProject&);   // a new tab on the project's home
        // Shows the project again over its surface as it now reads, the tabs standing.
        void rebindProject(OpenProject&);
        // Calls the project by its name as it now reads: its node, its segment with every url
        // under it, and the words on a page that say it.
        void renameProject(OpenProject&);
        // Takes the project out, its node and every tab standing in it. A tab is left, on the root
        // where no other stands.
        void removeProject(OpenProject&);
        void showWords(const Text&);   // what the root shows while no project is open
        // What a project's options page is filled with, each time one is built.
        void connectProjectPage(ProjectPageBuilder);
        // The url of what a link on the page names - a type, a module, a folder or a member of
        // the page's own project, or a path of the browser - or nothing where it names none.
        [[nodiscard]] std::optional<Url> urlOf(std::wstring_view target,
            const PageData& from) const;
        // A page's command was clicked: the project whose home the target names is asked to be
        // closed, the one command a page has.
        void answerCommand(std::wstring_view target);
        // What a tab's own file keeps of its tree: the nodes open, by project.
        [[nodiscard]] static Dom::Dt::Section createTabConfigSchema();
    protected:
        void initPageData(PageData&) override;
        void fetchSubItems(PageData&) override;
        void showPage(BrowserTab&) override;
        void saveViewState(BrowserTab&) override;   // the tab is closing: its tree goes
        IconSize pageIconSize(PageData&) override;
        void paintPageIcon(PageData&, PaintIconEvent&) override;
        IconSize tabIconSize(BrowserTab&) override;
        void paintTabIcon(BrowserTab&, PaintIconEvent&) override;
        // Notes the control the focus comes into the search box from - Escape hands it back.
        void nestedControlFocusing(FocusEvent&) override;
        void nestedControlDeleted(Control*) override;   // forgets a focus origin being deleted
    private:
        struct ShownProject;
        // What a path names, read off its segments: the root for none, else a project by its
        // segment, and in it its options page, or the folders down from its root as far as they
        // go - its home for none - then a module of the last folder, a type of
        // that or a function or a constant of the module, and a method of the type. A segment
        // nothing answers for makes the path one no project open carries.
        struct Target
        {
            ShownProject* project{ nullptr };   // null for the root, and for no project open
            std::wstring folder{};              // the path the folder segments spell
            std::size_t folderDepth{ 0 };       // how many segments past the project's are folders
            const ContentsFolder* contents{ nullptr };   // the folder's modules, where it has any
            const ContentsModule* module{ nullptr };
            const Type* type{ nullptr };
            std::wstring_view exportName{};     // a function or a constant of the module
            std::wstring_view method{};
            std::wstring search{};              // what a search page searches for
            std::size_t depth{ 0 };   // segments of the path
            bool projectOptions{ false };   // the project's options page
            bool searching{ false };   // a search page - the project's, or the root's for none
            [[nodiscard]] bool isRoot() const { return depth == 0; }
            [[nodiscard]] bool isHome() const { return project && depth == 1; }
            [[nodiscard]] bool isFolder() const { return project && depth == folderDepth + 1; }
            [[nodiscard]] bool isGone() const;
        };
        // The folder, module and type a link's spelling names, keyed by that spelling - a
        // folder by its path, whether or not it holds modules.
        using Targets = std::unordered_map<std::wstring, Target>;
        // Every folder on the way to one with modules, keyed by its path, with the modules
        // the folder holds - null for one that only holds other folders.
        using Folders = std::unordered_map<std::wstring, const ContentsFolder*>;
        // A project shown: what the workspace opened, the segment its urls start with, and what
        // its surface lists, read for the trees and the paths. Its pages' edits go through it.
        struct ShownProject : public IPageWriter
        {
            ShownProject(SurfaceBrowser&, OpenProject&, ProjectId);
            SurfaceBrowser& browser;
            OpenProject& open;
            ProjectId id;
            std::wstring segment{};
            ContentsFolders contents{};
            ContentsTree contentsTree{};   // over the contents, which every tab's tree reads
            SearchIndex index{};   // over the contents, which a search reads
            Targets targets{};
            Folders folders{};
            // Reads the contents, the targets and the folders off the surface as it now reads.
            void read();
            // A hint read by the strict rules from C++, where the project edits hints directly.
            [[nodiscard]] bool writesHint(const CommentSite&) const override;
            [[nodiscard]] std::wstring writeHint(const CommentSite&,
                std::wstring_view hint) override;
            [[nodiscard]] FootnoteDraft draftFootnote(const Reference&) override;
            [[nodiscard]] FootnoteDraft draftFootnote(const CommentSite&,
                const FootnoteHome&) override;
            [[nodiscard]] std::wstring writeFootnote(const FootnoteDraft&,
                std::wstring_view text) override;
        };
        // Held by pointer: a page writes through its project, and the tree's picks name it.
        using Projects = std::vector<std::unique_ptr<ShownProject>>;
        // A tab's tree, in the box that scrolls it.
        struct TabTree
        {
            const BrowserTab* tab{ nullptr };
            ScrollBox* box{ nullptr };
            SurfaceTree* tree{ nullptr };
        };
        using TabTrees = std::vector<TabTree>;
        // A line of the search box's list: a name and the project it stands in, or the search.
        struct ListedLine
        {
            const ShownProject* project{ nullptr };   // null for the search
            const NamedEntry* entry{ nullptr };
        };
        using ListedLines = std::vector<ListedLine>;
        using ShownProjects = std::vector<const ShownProject*>;
    private:
        [[nodiscard]] Target resolve(const PageData&) const;
        [[nodiscard]] ShownProject* projectOf(std::wstring_view segment) const;
        [[nodiscard]] ShownProject* projectOf(ProjectId) const;
        [[nodiscard]] ShownProject* projectOf(const OpenProject&) const;
        // The segment a project's urls start with: its name, a slash in it written as a hyphen,
        // and a number after it where another project open goes by the same.
        [[nodiscard]] std::wstring segmentFor(std::wstring_view name,
            const ShownProject* self) const;
        void openPicked(const TreePickEvent&);
        // The tab's tree, made with a node per project where the tab has none yet.
        [[nodiscard]] TabTree& treeOf(const BrowserTab&);
        // Adds the project's node to the tab's tree, open as the tab last left it.
        void addToTree(const TabTree&, const ShownProject&);
        // Writes the project's nodes open in the tab's tree into the tab's file.
        void storeTreeState(const TabTree&, ProjectId);
        // Shows the trees' bar at the width the tab's file keeps, or where it starts without one.
        void showTreeWidth(const BrowserTab&);
        // Writes the trees' bar's width into the shown tab's file.
        void storeTreeWidth(float width);
        [[nodiscard]] static std::wstring homePath(const ShownProject&);
        [[nodiscard]] static std::wstring optionsPath(const ShownProject&);
        // The path of the nearest thing named in the project, down to the member where one is
        // given - a method of the type, or a function or a constant of the module where there is
        // no type.
        [[nodiscard]] static std::wstring pathOf(const ShownProject&, std::wstring_view folder,
            const ContentsModule*, const Type*, std::wstring_view member = {});
        // The names of the folders directly under this one, in reading order.
        [[nodiscard]] static std::vector<std::wstring> subfolderNames(const ShownProject&,
            std::wstring_view folder);
        [[nodiscard]] std::wstring titleOf(const PageData&, const Target&) const;
        [[nodiscard]] static std::optional<RowIcon> iconOf(const Target&);
        // The page a target names, built over its project's surface; nothing where it names
        // words - no project open, a surface that lists nothing, a path no project carries - or
        // the options page.
        [[nodiscard]] std::optional<Page> pageOf(const Target&) const;
        // The project's home: its options page leads the contents, and Close project ends the
        // page.
        [[nodiscard]] static Page homePageOf(const ShownProject&);
        [[nodiscard]] Page projectsPage() const;   // the root's, while a project is open
        [[nodiscard]] static Command closeCommand(const ShownProject&, std::wstring text);
        // Fills a page with what the target names: the projects or SeeDocs' words on the root,
        // the words of a page no project carries or of a surface that lists nothing, and else the
        // page the surface gives.
        void fill(SurfacePage&, const Target&, const PageData&) const;
        // Puts the title and the list of every page data, and the icon of every tab, right for
        // the projects shown.
        void refreshPageTree();
        void refreshPageData(PageData&);   // this one and every one under it
        // Whether a change to the project reaches the page: one in the project, or the root,
        // which lists the projects. Null reaches the root alone.
        [[nodiscard]] static bool reaches(const Target&, const ShownProject*);
        // Drops every page the project reaches but the one kept, and shows the selected tab's
        // again.
        void rebuildPages(const ShownProject*, const BrowserPage* keep);
        // Gives every page the project reaches the words as the files now hold them, the
        // footnote named shown once its page is built again.
        void refreshPages(const ShownProject&, std::optional<Reference> show);
        // Builds the selected tab's page again, then shows the footnote waiting to be shown.
        void rebuildCurrentPage();
        [[nodiscard]] Text goneWords(const PageData&) const;
        // A project whose surface lists nothing is open all the same; only its pages are words.
        [[nodiscard]] static Text emptyWords(const ShownProject&);
        [[nodiscard]] static bool hasMethod(const Type&, std::wstring_view name);
        // Whether the module exports a function or a constant of the name.
        [[nodiscard]] static bool exports(const ContentsModule&, std::wstring_view name);
        // Whether a row of the type's page is named so: a property, an event, a field or a member.
        [[nodiscard]] static bool hasRow(const Type&, std::wstring_view name);
        // The url a link of the project's pages names; nothing where it names none.
        [[nodiscard]] std::optional<Url> urlIn(const ShownProject&,
            std::wstring_view target) const;
        // The projects a search reaches: the one the shown tab stands in, or all from the root.
        [[nodiscard]] ShownProjects searchedProjects() const;
        // The path of a search's page: the query after the project's segment, or after the root.
        [[nodiscard]] static std::wstring searchPath(const ShownProject*, std::wstring_view query);
        [[nodiscard]] Page searchPageOf(const Target&) const;
        void connectSearch();   // the box's requests, and Ctrl+F putting the focus in it
        void searchRequested(const SearchEvent&);
        // The focus back where it came into the box from, while that is shown, else onto the page.
        void handBackFocus();
        // Drops the names holding what the box holds under it, the search among them. See README
        void listMatches();
        [[nodiscard]] DropListLine lineOf(const ListedLine&, const NameHit&,
            bool namesProject) const;
        [[nodiscard]] static DropListLine searchLine(std::wstring_view typed);
        // Opens what a line of the list stands for, in a new tab where Alt was held.
        void openListed(std::size_t line, KeyModifiers);
        // Opens the page of what the box holds, in a new tab where Alt was held.
        void openSearch(KeyModifiers);
        void dropListed();   // the list down and its lines forgotten, a surface being read again
    private:
        static constexpr float k_treeWidth = 300.0f;   // where the trees' bar starts
        static constexpr float k_leastTreeWidth = 120.0f;   // the least the bar is dragged to
        static constexpr float k_leastPageWidth = 240.0f;   // what a page keeps against the bar
        static constexpr IconSize k_pageIconSize{ 16.0f };
        static constexpr float k_searchWidth = 300.0f;
        static constexpr std::size_t k_listedNames = 10;   // what the search box's list holds

        Projects m_projects{};
        ProjectId m_nextId{ 0 };
        const OpenProject* m_shownProject{ nullptr };   // the selected tab's, as last told
        UiTimer m_rebuildTimer{};
        std::optional<Reference> m_rebuildFootnote{};   // to show once the page stands again
        Text m_words{};   // what the root shows while no project is open
        ProjectPageBuilder m_projectPageBuilder{};
        ListedLines m_listed{};   // what each line of the search box's list stands for
        Control* m_focusOrigin{ nullptr };   // what held the focus before the search box took it

        TabTrees m_tabTrees{};
        // The tabs' trees, the selected tab's shown.
        PageControl& m_trees{ createLeftBar<PageControl>(
            PreferredSize{ k_treeWidth, 0.0f },
            MinSize{ k_leastTreeWidth, 0.0f }
        ) };
        SearchBox& m_search{ createCrumbBarEnd<SearchBox>(
            MinSize{ k_searchWidth, 0.0f },
            MaxSize{ k_searchWidth, k_maxFloat },
            // The box stands at the window's right end, so its list drops from the right edge.
            FormPlacement::BottomEnd,
            // A line taken with Alt opens in a tab behind, and the list stays for the next.
            KeyModifiers{ .alt = true }
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
        m_view.onPageCommand([this](PageCommandEvent& event) {
            runCommand(event.target);
        });
        onShowAnchor([this](ShowAnchorEvent& event) {
            m_view.showAnchor(event.url.anchor());
        });
    }

    // ProjectPage

    template<typename... Args>
    ProjectPage::ProjectPage(const CreateParams& params, Args&&... args)
        :
        BrowserPage{
            params,
            Padding{ 0.0f },
            Spacing{ 0.0f },
            UiElement::Page,
            std::forward<Args>(args)...
        }
    {
        m_head.write(RowIcon::Options, k_optionsTitle);
    }

    // SurfaceBrowser

    template<typename... Args>
    SurfaceBrowser::SurfaceBrowser(const CreateParams& params, Args&&... args)
        :
        BrowserControl{ params, std::forward<Args>(args)... }
    {
        appButton().onPaintIcon(Icons::SideBar::paintPanelIcon);
        createSplitter(PanelSlot::Left);
        onBarResize([this](BarResizeEvent& event) {
            storeTreeWidth(event.extent);
        });
        m_rebuildTimer.onTick([this](TimerEvent&) {
            rebuildCurrentPage();
        });
        connectSearch();
    }

    template<typename F>
    EventConnection SurfaceBrowser::onProjectShown(F&& callback)
    {
        return connectEvent<ProjectShownEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection SurfaceBrowser::onCloseProject(F&& callback)
    {
        return connectEvent<CloseProjectEvent>(std::forward<F>(callback));
    }
}
