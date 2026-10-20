export module SeeDocs_App.Browser;

import SeeDocs_App.AppIcon;
import SeeDocs_App.Editing;
import SeeDocs_App.ExternalEditors;
import SeeDocs_App.Icons;
import SeeDocs_App.PageView;
import SeeDocs_App.SidebarEditor;
import SeeDocs_App.SurfaceTree;
import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Project;
import SeeDocs_App.Search;
import SeeDocs_App.Surface;

import ClaFi.App.Notices;
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

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Searcher;
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
        NoticeHandle unwritten{};   // the database the pages show could not be written
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

    // The root page's rows were carried into another order. The surface has an Event of its own.
    export class OrderProjectsEvent : public ::ClaFi::Event
    {
    public:
        explicit OrderProjectsEvent(std::vector<OpenProject*>);
    public:
        const std::vector<OpenProject*> projects;   // in the order the rows stand
    };

    // Rescan asked for a project to be scanned again. The surface has an Event of its own.
    export class RescanProjectEvent : public ::ClaFi::Event
    {
    public:
        explicit RescanProjectEvent(OpenProject&);
    public:
        OpenProject& project;
    };

    // A page's edit changed the project's surface, for its database to hold. The surface has an
    // Event of its own.
    export class SurfaceEditedEvent : public ::ClaFi::Event
    {
    public:
        explicit SurfaceEditedEvent(OpenProject&);
    public:
        OpenProject& project;
    };

    // The sidebar page said what the project's sidebar is to be, for its properties to hold. The
    // surface has an Event of its own.
    export class SidebarEditedEvent : public ::ClaFi::Event
    {
    public:
        SidebarEditedEvent(OpenProject&, SidebarItems);
    public:
        OpenProject& project;
        SidebarItems items;
    };

    // The commands the browser answers about the projects it shows.
    namespace Actions
    {
        export extern Action rescan;   // scans the shown tab's project, or every one from the root
    }

    export class SurfaceBrowser;

    // One page of the browser: a PageView over what the tab's url names.
    export class SurfacePage : public BrowserPage
    {
    public:
        // The page bands the words of the browser's search.
        template<typename... Args>
        explicit SurfacePage(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] PageView& view() const { return m_view; }
        float viewPosition() const override { return m_view.viewPosition(); }
    protected:
        std::wstring pathToOpen() override;
    private:
        // The browser the page is shown in. Structure rather than state, so const stops here.
        [[nodiscard]] SurfaceBrowser& browser() const;
        // Takes the tab where a link on the page points: an anchor of the page stays on it,
        // anything else names a page of the surface - or several, picked from over the source.
        void followLink(std::wstring_view target, Control& source);
        // Claims Open and Open in new tab for a link menu over a link naming several pages.
        void connectLinkActions();
        void runCommand(std::wstring_view target);   // hands a command's click to the browser
        void orderRows(std::span<const std::wstring> targets);   // hands a carry to the browser
        void openSource(const SourcePlace&);   // hands a source button's click to the browser
        // Shows the anchor the tab arrived at, the page at the position where one is given, and
        // tells the browser where the page opens.
        void arrive(const Url&, std::optional<float> position);
    private:
        PageView& m_view;
    };

    // A project's options as a page of the browser: the head every page wears, and under it the
    // groups the application fills the page with.
    export class ProjectPage : public BrowserPage
    {
    public:
        template<typename... Args>
        explicit ProjectPage(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] PageHead& head() { return m_head; }
        [[nodiscard]] AppMenuPage& options() { return m_optionsBox.body(); }
    private:
        PageHead& m_head{ createTopBar<PageHead>() };
        ScrollBoxWith<AppMenuPage>& m_optionsBox{ createBody<ScrollBoxWith<AppMenuPage>>(
            HostProps{
                ScrollBars::Vertical
            },
            BodyProps<>{}
        ) };
    };

    // A project's sidebar as a page of the browser: the head, with Add and Delete in a row under
    // the title, and under it the editor of the sidebar's items.
    export class SidebarPage : public BrowserPage
    {
    public:
        // The tab is the page's, as every page's; the name and the items are the editor's.
        template<typename... Args>
        SidebarPage(const CreateParams&, BrowserTab*, std::wstring_view projectName,
            const SidebarItems&, Args&&...);
    public:
        [[nodiscard]] PageHead& head() { return m_head; }
        [[nodiscard]] SidebarEditor& editor() { return m_editor; }
    private:
        PageHead& m_head{ createTopBar<PageHead>() };
        SidebarEditor& m_editor;
    };

    // What a project's options page is filled with.
    export using ProjectPageBuilder = std::function<void(AppMenuPage&, OpenProject&)>;

    // The browser over the open projects: down the left the tab's own tree, a node per project
    // holding its sidebar's items, and a page per url on the right - /project/item/.../folder/
    // .../module/type/method, the items as the sidebar lists them, the folders as the sources
    // stand on disk under its All the APIs item, and the root listing the projects - with
    // SeeDocs' words while none is open. What a project's pages edit is written through the
    // browser into that project, and a project's closing, which its home and the root offer, is
    // asked of whoever owns it. A tab's tree is made the first time the tab is shown, and the
    // nodes it has open are kept with the tab. A search is typed at the crumb bar's end, and
    // every page shown bands its words. See the README beside the project.
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
        // Connects a handler raised when the root page's rows were carried into another order.
        template<typename F>
        EventConnection onOrderProjects(F&& callback);
        // Connects a handler raised when Rescan asks for a project to be scanned again.
        template<typename F>
        EventConnection onRescanProject(F&& callback);
        // Connects a handler raised when a page's edit changed a project's surface.
        template<typename F>
        EventConnection onSurfaceEdited(F&& callback);
        // Connects a handler raised when the sidebar page said what a project's sidebar is to be.
        template<typename F>
        EventConnection onSidebarEdited(F&& callback);
        // Shows a project opened: a node of its own at the end of the tree, and its pages under a
        // segment of its name. The project is held by reference until it is removed, and what its
        // pages edit is written into it - its sources, its footnotes and its database - as its
        // options allow.
        void addProject(OpenProject&, HomeTab);
        void openHome(const OpenProject&);   // a new tab on the project's home
        // Shows the project again over its surface as it now reads, the tabs standing.
        void rebindProject(OpenProject&);
        // Shows the project again over its sidebar as it now reads, the surface standing: every
        // tree's node and the pages the sidebar is read into.
        void rebindSidebar(OpenProject&);
        // Calls the project by its name as it now reads: its node, its segment with every url
        // under it, and the words on a page that say it.
        void renameProject(OpenProject&);
        // Takes the project out, its node and every tab standing in it. A tab is left, on the root
        // where no other stands.
        void removeProject(OpenProject&);
        // Shows the projects in this order: the nodes of every tree, the root's rows and lists.
        void orderProjects(std::span<OpenProject* const>);
        void showWords(const Text&);   // what the root shows while no project is open
        // What a project's options page is filled with, each time one is built.
        void connectProjectPage(ProjectPageBuilder);
        // The url of what a link on the page names - a type, a module, a folder or a member of
        // the page's own project, or a path of the browser - or nothing where it names none, or
        // several.
        [[nodiscard]] std::optional<Url> urlOf(std::wstring_view target,
            const PageData& from) const;
        // Whether a link on the page names namesakes in several folders. See README#namesakes
        [[nodiscard]] bool namesSeveral(std::wstring_view target, const PageData& from) const;
        // The type a link on the page names; null where it names none, several or no type.
        [[nodiscard]] const Type* typeOf(std::wstring_view target, const PageData& from) const;
        // Opens what a link on the page names, or drops the pages it names over the source to
        // pick one from. See README#namesakes
        void followLink(const PageData& from, std::wstring_view target, Control& source);
        // Drops the pages a link names over the owner once the press that asked has unwound -
        // the menu it was asked from gone - the one picked opening as the select says.
        void pickLater(const PageData& from, std::wstring_view target, Control& owner,
            SelectTab);
        // A page's command was clicked: the project whose home the target names is asked to be
        // closed, the one command a page asks of the browser.
        void answerCommand(std::wstring_view target);
        // The root's rows were carried about: the projects their home paths name are asked to be
        // put in that order.
        void answerOrder(std::span<const std::wstring> targets);
        // Opens a source a page names in the editor Settings keeps for its language. See README
        void openSource(const PageData& from, const SourcePlace&);
        [[nodiscard]] const Searcher& searcher() const { return m_searcher; }   // the box's words
        // Opens what a link on the page names; a link on a search page takes the search on there.
        void openLinked(const PageData& from, const Url&);
        // A page has shown the anchor its tab arrived at: one the search opened goes on with it
        // from where the page opens, and the box counts what the page holds. See README
        void pageArrived(SurfacePage&, const Control& opening);
        // What a tab's own file keeps of its tree: the nodes open, by project.
        [[nodiscard]] static Dom::Dt::Section createTabConfigSchema();
    protected:
        void initPageData(PageData&) override;
        void fetchSubItems(PageData&) override;
        void showPage(BrowserTab&) override;
        void saveViewState(BrowserTab&) override;   // the tab is closing: its tree goes
        IconSize pageIconSize(PageData&) override;
        void paintPageIcon(PageData&, PaintIconEvent&) override;
        ButtonViewMode rootCrumbViewMode() const override { return ButtonViewMode::IconOnly; }
        IconSize tabIconSize(BrowserTab&) override;
        void paintTabIcon(BrowserTab&, PaintIconEvent&) override;
        // Notes the control the focus comes into the search box from - Escape hands it back.
        void nestedControlFocusing(FocusEvent&) override;
        // Forgets a focus origin being deleted, and a pending pick's owner.
        void nestedControlDeleted(Control*) override;
    private:
        struct ShownProject;
        // A page of the project itself, named by a dot segment after the project's.
        enum class OwnPage
        {
            None,
            Options,
            Sidebar
        };
        // What a path names, read off its segments: the root for none, else a project by its
        // segment, and in it its own page, or its sidebar's items down from the top as far as they
        // go - its home for none - then, under an All the APIs item, the folders down from the
        // root as far as they go, a list of the last folder, or a module of it, a type of that or
        // a function or a constant of the module, and a method of the type. A segment nothing
        // answers for makes the path one no project open carries.
        struct Target
        {
            ShownProject* project{ nullptr };   // null for the root, and for no project open
            const SidebarItem* item{ nullptr }; // the item the path names, or stands under
            std::wstring itemPath{};            // the item's names from the top, a slash between
            std::size_t itemDepth{ 0 };         // how many segments past the project's are items
            std::wstring folder{};              // the path the folder segments spell
            std::size_t folderDepth{ 0 };       // how many segments past the items are folders
            const ContentsFolder* contents{ nullptr };   // the folder's modules, where it has any
            const ContentsModule* module{ nullptr };
            const Type* type{ nullptr };
            std::wstring_view exportName{};     // a function or a constant of the module
            std::wstring_view method{};
            std::wstring search{};              // what a search page searches for
            std::optional<ListKind> list{};     // a list of the folder's
            std::size_t depth{ 0 };   // segments of the path
            OwnPage ownPage{ OwnPage::None };
            bool searching{ false };   // a search page - a folder's, a project's, or the root's
            [[nodiscard]] bool isOwnPage() const { return ownPage != OwnPage::None; }
            [[nodiscard]] bool isRoot() const { return depth == 0; }
            [[nodiscard]] bool isHome() const { return project && depth == 1; }
            // The item's own page: a category's, a page's, or the All the APIs item's.
            [[nodiscard]] bool isItem() const { return item && depth == itemDepth + 1; }
            // A folder's page, the root's being the All the APIs item's or the home.
            [[nodiscard]] bool isFolder() const
                { return project && folderDepth > 0 && depth == itemDepth + folderDepth + 1; }
            // Whether the path reaches the folders: it stands under an All the APIs item, or
            // under the home of a project whose sidebar has none.
            [[nodiscard]] bool holdsFolders() const;
            [[nodiscard]] bool isGone() const;
        };
        // The folder, module and type a link names, keyed by the link - a folder by its path,
        // whether or not it holds modules, a module by moduleLink and a type by typeLink.
        using Targets = std::unordered_map<std::wstring, Target>;
        // The types a qualified name alone names, as a text spells one, in reading order - several
        // where namesakes stand in several folders.
        using Namesakes = std::unordered_map<std::wstring, std::vector<Target>>;
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
            // The path of the sidebar's first All the APIs item, which the folders stand under;
            // empty where it has none, the folders then standing under the home, unlisted.
            std::wstring apisPath{};
            ContentsFolders contents{};
            ContentsTree contentsTree{};   // over the contents, which every tab's tree reads
            SearchIndex index{};   // over the contents, which a search reads
            Targets targets{};
            Namesakes namesakes{};
            Folders folders{};
            NoticeHandle goneSource{};   // a source a page names that is not where it was read
            // Reads the contents, the targets and the folders off the surface as it now reads.
            void read();
            void readSidebar();   // where the folders stand, off the sidebar as it now reads
            // Puts up a notice that the source is gone, Rescan its answer. See README
            void noteGone(const SourceFile&, bool rootRead);
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
        // A line of the search box's list: a name and the project it stands in, or a search.
        struct ListedLine
        {
            const ShownProject* project{ nullptr };   // null for a search
            const NamedEntry* entry{ nullptr };
            std::wstring searchPath{};   // the page a search's line opens
        };
        using ListedLines = std::vector<ListedLine>;
        // A place the search box's list offers to search: a folder, a project, or the root's.
        struct SearchPlace
        {
            const ShownProject* project{ nullptr };   // null for the root's
            std::wstring folder{};   // empty for the whole project
            std::wstring name{};   // what the search's line calls it
        };
        using SearchPlaces = std::vector<SearchPlace>;
        using ShownProjects = std::vector<const ShownProject*>;
        // A page a link names: its url, and the target it is where it is a module's or a type's.
        struct LinkedPlace
        {
            Url url{};
            const Target* target{ nullptr };   // null for a list's, or a url the link spells
            std::wstring_view member{};   // a member's or an export's name, the target its owner
        };
        using LinkedPlaces = std::vector<LinkedPlace>;
        using TargetList = std::vector<const Target*>;
        // A pick of a link's pages asked for from a link menu, waiting for that menu to go.
        struct PendingPick
        {
            const PageData* from{ nullptr };
            std::wstring target{};
            Control* owner{ nullptr };
            SelectTab select{ SelectTab::Yes };
        };
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
        [[nodiscard]] static std::wstring pathOf(const ShownProject&, OwnPage);
        // The path of a sidebar item, by its names from the top; the home's for none.
        [[nodiscard]] static std::wstring itemPathOf(const ShownProject&,
            std::wstring_view item);
        // The path of the nearest thing named in the project, under its All the APIs item, down
        // to the member where one is given - a method of the type, or a function or a constant
        // of the module where there is no type.
        [[nodiscard]] static std::wstring pathOf(const ShownProject&, std::wstring_view folder,
            const ContentsModule*, const Type*, std::wstring_view member = {});
        // The same under the item the path names, which another All the APIs item may be.
        [[nodiscard]] static std::wstring pathUnder(const ShownProject&, std::wstring_view item,
            std::wstring_view folder, const ContentsModule*, const Type*,
            std::wstring_view member = {});
        // The tab's tree follows the page: the item's row, the folder's, the module's or the
        // type's, or the project's node for a page none of those is.
        void followPage(SurfaceTree&, const Target&);
        // Puts Add and Delete into the page's head and hands what the editor says on to whoever
        // owns the project.
        void connectSidebarPage(SidebarPage&, ShownProject&);
        // The names of the folders directly under this one, in reading order.
        [[nodiscard]] static std::vector<std::wstring> subfolderNames(const ShownProject&,
            std::wstring_view folder);
        [[nodiscard]] std::wstring titleOf(const PageData&, const Target&) const;
        [[nodiscard]] static std::optional<RowIcon> iconOf(const Target&);
        // The page a target names, built over its project's surface; nothing where it names
        // words - no project open, a path no project carries - or a page of the project itself.
        [[nodiscard]] std::optional<Page> pageOf(const Target&) const;
        // The project's home: its sidebar's items, Options and Sidebar in a row under its title,
        // Close project beside it.
        [[nodiscard]] static Page homePageOf(const ShownProject&);
        // The items under the category, or the sidebar's top ones for none, each linked to its
        // page, a divider parting the tables.
        [[nodiscard]] static Page itemsPageOf(const ShownProject&, const SidebarItem* category,
            std::wstring_view itemPath);
        // The All the APIs item's page: what the home said of the folders before the sidebar.
        [[nodiscard]] static Page apisPageOf(const ShownProject&, const SidebarItem&);
        [[nodiscard]] static Page contentPageOf(const SidebarItem&);   // its head alone, so far
        [[nodiscard]] Page projectsPage() const;   // the root's, while a project is open
        [[nodiscard]] static Command closeCommand(const ShownProject&, std::wstring text);
        // Fills a page with what the target names: the projects or SeeDocs' words on the root,
        // the words of a page no project carries, and else the page the surface gives.
        void fill(SurfacePage&, const Target&, const PageData&) const;
        // Puts the title and the list of every page data, the icon of every tab and Rescan's
        // tooltip right for the projects shown.
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
        // What a home with nothing to document says instead: why, and the folders it reads.
        static void explainEmpty(Page&, const Project&);
        [[nodiscard]] static bool hasMethod(const Type&, std::wstring_view name);
        // Whether the module exports a function or a constant of the name.
        [[nodiscard]] static bool exports(const ContentsModule&, std::wstring_view name);
        // Whether a row of the type's page is named so: a property, an event, a field or a member.
        [[nodiscard]] static bool hasRow(const Type&, std::wstring_view name);
        // The url a link of the project's pages names; nothing where it names none, or several.
        [[nodiscard]] std::optional<Url> urlIn(const ShownProject&,
            std::wstring_view target) const;
        // The pages a link on the page names: none, the one, or namesakes to pick from.
        [[nodiscard]] LinkedPlaces placesOf(std::wstring_view target, const PageData& from) const;
        // The pages a link of the project's pages names, read from the page the target stands
        // for. See README#namesakes
        [[nodiscard]] LinkedPlaces placesIn(const ShownProject&, std::wstring_view target,
            const Target& from) const;
        // The namesakes a page names its link by: the one in its own module, else in its folder,
        // else all of them.
        [[nodiscard]] static TargetList narrowed(const std::vector<Target>&, const Target& from);
        // A menu line per page over the owner, the one picked opening as the select says.
        void showPicks(const PageData& from, const LinkedPlaces&, Control& owner, SelectTab);
        void showPendingPick();   // where its owner still stands
        // What a search or Rescan reaches: the shown tab's project, or all of them from the root.
        [[nodiscard]] ShownProjects searchedProjects() const;
        // The projects a search page searches: its own, or all from the root.
        [[nodiscard]] ShownProjects searchedProjects(const Target&) const;
        // Search "row" in ClaFi - a search's list line, and the name of its page.
        [[nodiscard]] static std::wstring searchTitle(std::wstring_view typed,
            std::wstring_view place);
        // What the root's search and Rescan reach: the one project open, all projects, or none.
        [[nodiscard]] std::wstring rootSearchPlace() const;
        // Where a search page searches: its folder, its project, or the root's place.
        [[nodiscard]] std::wstring searchPlaceOf(const Target&) const;
        // A search page's path: the query after its folder's path, its project's, or the root.
        [[nodiscard]] static std::wstring searchPath(const ShownProject*, std::wstring_view folder,
            std::wstring_view query);
        [[nodiscard]] Page searchPageOf(const Target&) const;
        // The box's requests, Ctrl+F putting the focus in it, and F3 and Shift+F3 stepping.
        void connectSearch();
        void searchRequested(const SearchEvent&);
        // The box's words are the search's: the pages band them, and the list drops under the box
        // while it is typed in.
        void takeWords();
        // The selected tab's page; null where it shows a page of a project itself, or none.
        [[nodiscard]] SurfacePage* surfacePage();
        // Steps through the shown page's matches, the focus going with them from the page.
        void stepFound(FindTarget);
        void writeFound();   // the box's count, of the shown page
        // The focus onto the shown page's current match, else back where it came into the box
        // from while that is shown, else onto the page.
        void handBackFocus();
        // Drops the searches and the names holding what the box holds under it. See README
        void listMatches();
        // The places the list offers to search, the narrowest first. See README
        [[nodiscard]] SearchPlaces searchPlaces();
        [[nodiscard]] DropListLine lineOf(const ListedLine&, const NameHit&,
            bool namesProject) const;
        // A search's line, naming where it searches.
        [[nodiscard]] static DropListLine searchLine(std::wstring_view typed,
            std::wstring_view place);
        // Opens what a line of the list stands for, in a new tab where Alt was held.
        void openListed(std::size_t line, KeyModifiers);
        // Opens a search's page, in a new tab where Alt was held.
        void openSearch(const Url&, KeyModifiers);
        void dropListed();   // the list down and its lines forgotten, a surface being read again
        void connectRescan();   // claims Rescan, with its button after Up
        void rescanShown();   // the shown tab's project scanned again, or every one from the root
        void writeRescanTooltip();   // names what Rescan reaches from the page shown
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
        Searcher m_searcher{};   // the box's words, which every page shown bands
        std::optional<Url> m_searchOpens{};   // what the search opened, which goes on with it
        std::optional<PendingPick> m_pendingPick{};
        UiTimer m_pickTimer{};
        UiTimer m_rescanTimer{};
        EditorRuns m_editors{ appContext() };

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
            FormPlacement::BottomRight,
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
        },
        m_view{ createBody<PageView>(browser().searcher()) }
    {
        m_view.onPageLink([this](PageLinkEvent& event) {
            followLink(event.target, event.source);
        });
        m_view.onPageLinkType([this](PageLinkTypeEvent& event) {
            event.type = browser().typeOf(event.target, pageData());
        });
        m_view.onPageCommand([this](PageCommandEvent& event) {
            runCommand(event.target);
        });
        m_view.onPageOrder([this](PageOrderEvent& event) {
            orderRows(event.targets);
        });
        m_view.onPageSource([this](PageSourceEvent& event) {
            openSource(event.source);
        });
        m_view.onPageAnchor([this](PageAnchorEvent& event) {
            setAnchor(event.anchor);
        });
        onShowAnchor([this](ShowAnchorEvent& event) {
            arrive(event.url, event.position);
        });
        connectLinkActions();
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
    }

    // SidebarPage

    template<typename... Args>
    SidebarPage::SidebarPage(const CreateParams& params, BrowserTab* tab,
        const std::wstring_view projectName, const SidebarItems& items, Args&&... args)
        :
        BrowserPage{
            params,
            tab,
            Padding{ 0.0f },
            Spacing{ 0.0f },
            UiElement::Page,
            std::forward<Args>(args)...
        },
        m_editor{ createBody<SidebarEditor>(projectName, items) }
    {
    }

    // SurfaceBrowser

    template<typename... Args>
    SurfaceBrowser::SurfaceBrowser(const CreateParams& params, Args&&... args)
        :
        BrowserControl{ params, std::forward<Args>(args)... }
    {
        appButton().onPaintIcon(AppIcon::paintIcon);
        createSplitter(PanelSlot::Left);
        onBarResize([this](BarResizeEvent& event) {
            storeTreeWidth(event.extent);
        });
        m_rebuildTimer.onTick([this](TimerEvent&) {
            rebuildCurrentPage();
        });
        m_pickTimer.onTick([this](TimerEvent&) {
            showPendingPick();
        });
        connectSearch();
        connectRescan();
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

    template<typename F>
    EventConnection SurfaceBrowser::onOrderProjects(F&& callback)
    {
        return connectEvent<OrderProjectsEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection SurfaceBrowser::onRescanProject(F&& callback)
    {
        return connectEvent<RescanProjectEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection SurfaceBrowser::onSurfaceEdited(F&& callback)
    {
        return connectEvent<SurfaceEditedEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection SurfaceBrowser::onSidebarEdited(F&& callback)
    {
        return connectEvent<SidebarEditedEvent>(std::forward<F>(callback));
    }
}
