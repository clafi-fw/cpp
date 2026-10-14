module SeeDocs_App.Browser;

import SeeDocs_App.Editing;
import SeeDocs_App.ExternalEditors;
import SeeDocs_App.Icons;
import SeeDocs_App.PageView;
import SeeDocs_App.SurfaceTree;
import SeeDocs_App.Database;
import SeeDocs_App.Edits;
import SeeDocs_App.Notes;
import SeeDocs_App.PageText;
import SeeDocs_App.Pages;
import SeeDocs_App.Project;
import SeeDocs_App.Search;
import SeeDocs_App.Surface;

import ClaFi.App.Notices;
import ClaFi.App.Settings;

import ClaFi.Browser.Consts;
import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Controls.DropList;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.SearchBox;
import ClaFi.Controls.Tree;

import ClaFi.StdActions;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer, which is what makes the tab's nodes readable as strings.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Searcher;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Url;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Browser;

    namespace
    {
        constexpr float k_wordsIndent = 12.0f;   // of a path quoted in SeeDocs' words
        constexpr std::wstring_view k_noteExtension = L".md";
        constexpr std::wstring_view k_sourceJoin = L" › ";
        constexpr std::wstring_view k_separator = L" · ";
        // The options page's segment, under its project's. The scanner reads no folder whose name
        // starts with a dot, and no module or header is named so, so no page of a surface can
        // take it.
        constexpr std::wstring_view k_optionsSegment = L".project";
        // What a search page's segment opens with, the query after it - a dot for the same reason.
        constexpr std::wstring_view k_searchPrefix = L".search=";
        constexpr std::wstring_view k_scope = L"::";
        constexpr float k_iconGap = 5.0f;   // between a list line's icon and its text
        constexpr float k_kindGap = 12.0f;  // the least room before a list line's kind word
        constexpr std::wstring_view k_unnamedSegment = L"Project";   // for a name that gives none
        constexpr std::wstring_view k_closeProjectCaption = L"Close project";   // on a home
        constexpr std::wstring_view k_closeCaption = L"Close";   // on a row of the root
        constexpr std::wstring_view k_closeHint = L"Closes the project and every tab on it";
        // What a tab's file keeps its tree's open nodes under, an entry per project.
        constexpr std::wstring_view k_treeNodeName = L"Tree";
        // What a tab's file keeps the trees' bar's width under; zero is a bar never resized.
        constexpr std::wstring_view k_treeWidthName = L"TreeWidth";

        namespace TreeKeys
        {
            constexpr std::wstring_view project = L"Project";   // the project's folder
            constexpr std::wstring_view home = L"Home";         // its own node open
            constexpr std::wstring_view folders = L"Folders";   // by path
            constexpr std::wstring_view modules = L"Modules";   // by name
        }

        using TreeEntries = Dom::Sequence<Dom::Section>;
        using NodeNames = Dom::Sequence<std::wstring>;
        constexpr wchar_t k_segmentSlash = L'-';   // a slash in a project's name, in its segment

        // The names down a path, the root's empty one left out: the project's first.
        [[nodiscard]] std::vector<std::wstring_view> segmentsOf(const PageData& data)
        {
            std::vector<std::wstring_view> result;
            for (const PageData* walk = &data; walk && walk->parent; walk = walk->parent)
                result.push_back(walk->name);
            std::ranges::reverse(result);
            return result;
        }

        // The segment a module stands under in a path: its name, or a header's file name - a
        // header being named by its path, whose folders are segments of their own.
        [[nodiscard]] std::wstring_view segmentOf(const ContentsModule& module)
        {
            return module.header ? module.shownName : module.name;
        }

        // A folder's path with one more folder under it; the root's is the folder alone.
        [[nodiscard]] std::wstring joinFolder(const std::wstring_view folder,
            const std::wstring_view name)
        {
            if (folder.empty())
                return std::wstring{ name };
            std::wstring result{ folder };
            result.append(1, ConfigNames::pathSeparator).append(name);
            return result;
        }

        // **A query is one segment of a url**, so a slash in it is written as %2F, and a % that
        // would read as the start of one as %25.
        [[nodiscard]] std::wstring escapedQuery(const std::wstring_view query)
        {
            std::wstring result;
            for (const wchar_t current : query)
            {
                if (current == L'%')
                    result += L"%25";
                else if (current == ConfigNames::pathSeparator)
                    result += L"%2F";
                else
                    result += current;
            }
            return result;
        }

        [[nodiscard]] std::wstring unescapedQuery(const std::wstring_view segment)
        {
            std::wstring result;
            for (std::size_t i = 0; i < segment.size(); ++i)
            {
                const std::wstring_view rest = segment.substr(i);
                if (rest.starts_with(L"%2F") || rest.starts_with(L"%2f"))
                {
                    result += ConfigNames::pathSeparator;
                    i += 2;
                }
                else if (rest.starts_with(L"%25"))
                {
                    result += L'%';
                    i += 2;
                }
                else
                {
                    result += segment[i];
                }
            }
            return result;
        }

        // A path as a paragraph of its own, in spot ink, the way SeeDocs' words quote one.
        void writePath(Text& text, const std::wstring_view path)
        {
            text << k_endLine << ParaIndent{ k_wordsIndent };
            text << PushThemeColor{ InkWell::spotInk() } << path << PopColor{};
            text << ParaIndent{ 0.0f } << k_endLine;
        }
    }

    // OpenProject

    OpenProject::OpenProject(Project opened, Surface read)
        :
        project{ std::move(opened) },
        surface{ std::move(read) },
        notes{ project.footnotesFolder() }
    {
    }

    // ProjectShownEvent

    ProjectShownEvent::ProjectShownEvent(const OpenProject* shown)
        :
        ::ClaFi::Event{},
        project{ shown }
    {
    }

    // CloseProjectEvent

    CloseProjectEvent::CloseProjectEvent(OpenProject& closing)
        :
        ::ClaFi::Event{},
        project{ closing }
    {
    }

    // RescanProjectEvent

    RescanProjectEvent::RescanProjectEvent(OpenProject& rescanned)
        :
        ::ClaFi::Event{},
        project{ rescanned }
    {
    }

    // SurfacePage

    SurfaceBrowser& SurfacePage::browser() const
    {
        return static_cast<SurfaceBrowser&>(tab().browserControl());
    }

    // The link a menu is up for on the page, as the path it names - so Open and Open in new tab
    // are about that link, and greyed for one no project open carries.
    std::wstring SurfacePage::pathToOpen()
    {
        const std::wstring_view target = m_view.menuLink();
        if (target.empty())
            return {};
        const std::optional<Url> url = browser().urlOf(target, pageData());
        return url.has_value() ? url->path() : std::wstring{};
    }

    // An anchor is a move on the page standing, so nothing goes down and the move runs at once;
    // it replaces, so a walk through the footnotes leaves no trail. A page named is a move that
    // takes this page down with it, and waits for the click to unwind. See Browser
    void SurfacePage::followLink(const std::wstring_view target)
    {
        if (target.starts_with(k_anchorPrefix))
        {
            const Url url{ pageData().path(), target.substr(k_anchorPrefix.size()) };
            browser().goTo(url, HistoryEntry::Replace);
            return;
        }
        if (const std::optional<Url> url = browser().urlOf(target, pageData()))
            browser().openLinked(pageData(), *url);
    }

    void SurfacePage::runCommand(const std::wstring_view target)
    {
        browser().answerCommand(target);
    }

    void SurfacePage::openSource(const SourcePlace& source)
    {
        browser().openSource(pageData(), source);
    }

    void SurfacePage::arrive(const Url& url)
    {
        const Control& opening = m_view.showAnchor(url.anchor());
        browser().pageArrived(*this, opening);
    }

    // SurfaceBrowser::Target

    bool SurfaceBrowser::Target::isGone() const
    {
        std::size_t resolved = folderDepth;
        if (project)
            ++resolved;
        if (projectOptions)
            ++resolved;
        if (module)
            ++resolved;
        if (type || !exportName.empty())
            ++resolved;
        if (!method.empty())
            ++resolved;
        if (searching)
            ++resolved;
        return depth > resolved;
    }

    // SurfaceBrowser::ShownProject

    SurfaceBrowser::ShownProject::ShownProject(SurfaceBrowser& shownBy, OpenProject& opened,
        const ProjectId given)
        :
        browser{ shownBy },
        open{ opened },
        id{ given }
    {
    }

    void SurfaceBrowser::ShownProject::read()
    {
        contents = contentsOf(open.surface);
        contentsTree = ContentsTree{ contents };
        index = SearchIndex{ open.surface, contents };
        targets.clear();
        folders.clear();
        for (const ContentsFolder& folder : contents)
        {
            targets[folder.path] = { .folder = folder.path, .contents = &folder };
            // Every folder on the way to this one is a folder, whether or not it holds modules
            // of its own, and a link names it by its path.
            std::wstring way;
            std::wstring_view rest = folder.path;
            while (!rest.empty())
            {
                const std::size_t slash = rest.find(ConfigNames::pathSeparator);
                way = joinFolder(way, rest.substr(0, slash));
                folders.try_emplace(way, nullptr);
                targets.try_emplace(way, Target{ .folder = way });
                rest = slash == std::wstring_view::npos ? std::wstring_view{} : rest.substr(slash + 1);
            }
            folders[folder.path] = &folder;
            for (const ContentsModule& module : folder.modules)
            {
                targets[module.name] = {
                    .folder = folder.path,
                    .contents = &folder,
                    .module = &module
                };
                for (const Type* type : module.types)
                {
                    targets[type->qualifiedName] = {
                        .folder = folder.path,
                        .contents = &folder,
                        .module = &module,
                        .type = type
                    };
                }
            }
        }
    }

    // **A scan is how the pages catch up with the disk**, so Rescan is the answer. The notice goes
    // with the project, which is all the answer could act on.
    void SurfaceBrowser::ShownProject::noteGone(const SourceFile& file, const bool folderRead)
    {
        Text message{};
        if (folderRead)
            message << L"The file is no longer on disk.";
        else
            message << L"The file stands in a folder the project no longer reads.";
        message << L" Rescan the project to bring its pages up to date.";
        goneSource = notices().post(Notice{
            .icon = MessageIcon::Warning,
            .title = openInEditorTitle(file.path),
            .message = std::move(message),
            .answers = {
                NoticeAnswer{
                    .caption = L"Rescan",
                    .run = [this](ClickEvent&) {
                        RescanProjectEvent event{ open };
                        browser.emitEvent(event);
                    }
                }
            }
        });
    }

    // The writer puts words where a strict read takes them from, in a C++ comment; a surface read
    // otherwise than the project now asks is written into by nothing until it is scanned again.
    bool SurfaceBrowser::ShownProject::writesHint(const CommentSite& site) const
    {
        const Surface& surface = open.surface;
        if (!open.project.writesHints() || surface.hintRules() != HintRules::Strict)
            return false;
        if (site.place.file >= surface.files().size())
            return false;
        const std::wstring& module = surface.files()[site.place.file].module;
        return surface.languageOf(module) == SourceLanguage::Cpp;
    }

    std::wstring SurfaceBrowser::ShownProject::writeHint(const CommentSite& site,
        const std::wstring_view hint)
    {
        // Unchanged words are no edit, so the database and the page stay as they are.
        if (!changesHint(site, hint))
            return {};
        if (!writesHint(site))
            return L"The project does not write hints into its sources.";
        const Refusal refusal = SeeDocs_App::writeHint(open.surface, open.project.sourceRoots(),
            site, hint);
        if (!refusal.empty())
            return refusal;
        writeDatabase(open.surface, open.project.databasePath());
        browser.refreshPages(*this, std::nullopt);
        return {};
    }

    // The editor's bar names the note and the section, and how many comments reach it - or that
    // none does, the section being the type's by its name.
    FootnoteDraft SurfaceBrowser::ShownProject::draftFootnote(const Reference& reference)
    {
        FootnoteDraft draft;
        draft.reference = reference;
        const NoteSection* section = open.notes.section(reference.note, reference.anchor);
        if (!section)
        {
            draft.refusal = reference.note + std::wstring{ k_noteExtension }
                + L" no longer holds this section.";
            return draft;
        }
        draft.text = flowedText(section->lines);
        draft.place = reference.note + std::wstring{ k_noteExtension }
            + std::wstring{ k_sourceJoin } + section->heading;
        const std::size_t uses = referencesTo(open.surface, reference);
        draft.place += k_separator;
        if (uses == 0)
            draft.place += L"matched by name";
        else if (uses == 1)
            draft.place += L"reached from 1 declaration";
        else
            draft.place += L"reached from " + std::to_wstring(uses) + L" declarations";
        return draft;
    }

    FootnoteDraft SurfaceBrowser::ShownProject::draftFootnote(const CommentSite& site,
        const FootnoteHome& home)
    {
        FootnoteDraft draft;
        std::optional<NewFootnote> placed = placeFootnote(open.notes, site, home.note, home.after,
            home.owner, draft.refusal);
        if (!placed)
            return draft;
        draft.reference = placed->reference;
        draft.place = placed->reference.note + std::wstring{ k_noteExtension }
            + std::wstring{ k_sourceJoin } + placed->heading + std::wstring{ k_separator }
            + L"new, matched by name";
        draft.placed = std::move(placed);
        return draft;
    }

    std::wstring SurfaceBrowser::ShownProject::writeFootnote(const FootnoteDraft& draft,
        const std::wstring_view text)
    {
        if (draft.placed)
        {
            const Refusal refusal = addFootnote(open.notes, *draft.placed, text);
            if (!refusal.empty())
                return refusal;
            browser.refreshPages(*this, draft.placed->reference);
            return {};
        }
        const Refusal refusal = SeeDocs_App::writeFootnote(open.notes, draft.reference, text);
        if (!refusal.empty())
            return refusal;
        browser.refreshPages(*this, std::nullopt);
        return {};
    }

    // SurfaceBrowser

    // The pages a project reaches go, whatever was built on its paths while it was not open.
    void SurfaceBrowser::addProject(OpenProject& open, const HomeTab homeTab)
    {
        const ProjectId id = m_nextId;
        ++m_nextId;
        std::unique_ptr<ShownProject> made = std::make_unique<ShownProject>(*this, open, id);
        ShownProject& shown = *made;
        shown.segment = segmentFor(open.project.name, nullptr);
        shown.read();
        m_projects.push_back(std::move(made));
        for (const TabTree& entry : m_tabTrees)
            addToTree(entry, shown);
        refreshPageTree();
        rebuildPages(&shown, nullptr);
        if (homeTab == HomeTab::Open)
            openHome(open);
        else if (homeTab == HomeTab::Options)
            addTab(Url{ optionsPath(shown) });
    }

    void SurfaceBrowser::openHome(const OpenProject& open)
    {
        if (const ShownProject* shown = projectOf(open))
            addTab(Url{ homePath(*shown) });
    }

    // **The selected tab's options page stands**, where it is this project's: its own buttons are
    // what read the sources again, and it shows no surface. Each tree's open nodes are asked for
    // before the contents they were made over go.
    void SurfaceBrowser::rebindProject(OpenProject& open)
    {
        ShownProject* shown = projectOf(open);
        if (!shown)
            return;
        std::vector<OpenRows> opened;
        for (const TabTree& entry : m_tabTrees)
            opened.push_back(entry.tree->openRows(shown->id));
        dropListed();
        shown->read();
        for (std::size_t i = 0; i != m_tabTrees.size(); ++i)
            m_tabTrees[i].tree->rebuildProject(shown->id, shown->contentsTree, opened[i]);
        refreshPageTree();
        const BrowserPage* keep = nullptr;
        for (BrowserTab& tab : tabs())
        {
            if (!tab.selected() || !tab.page())
                continue;
            const BrowserPage& page = static_cast<const BrowserPage&>(*tab.page());
            const Target target = resolve(page.pageData());
            if (target.project == shown && target.projectOptions)
                keep = &page;
        }
        rebuildPages(shown, keep);
    }

    // The segment follows the name, and the browser takes every tab and history entry under it to
    // the new path - see BrowserControl::renamePageData - so the pages standing keep their place.
    void SurfaceBrowser::renameProject(OpenProject& open)
    {
        ShownProject* shown = projectOf(open);
        if (!shown)
            return;
        const std::wstring segment = segmentFor(open.project.name, shown);
        if (segment != shown->segment)
        {
            PageData* page = homePageData().findSubItem(shown->segment);
            shown->segment = segment;
            if (page)
                renamePageData(*page, segment);
        }
        for (const TabTree& entry : m_tabTrees)
            entry.tree->renameProject(shown->id, open.project.name);
        refreshPageTree();
        BrowserTab* selected = nullptr;
        for (BrowserTab& tab : tabs())
        {
            if (tab.selected())
                selected = &tab;
            if (!tab.pageData())
                continue;
            const Target target = resolve(*tab.pageData());
            if (target.project != shown || !target.isHome())
                continue;
            // A tab on the project's home is captioned with its name.
            storeTabSettings(tab);
            tab.invalidate();
        }
        refreshPages(*shown, std::nullopt);
        // Sent to the url it stands on, the selected tab builds its crumbs again.
        if (selected && selected->pageData())
            goTo(*selected->pageData(), selected->anchor(), *selected);
    }

    // **No tab closes while it is shown**: the selected one gives way first, to a tab standing
    // elsewhere or to a new one on the root, and only then do the project's tabs go.
    void SurfaceBrowser::removeProject(OpenProject& open)
    {
        const Projects::iterator found = std::ranges::find_if(m_projects,
            [&open](const std::unique_ptr<ShownProject>& shown) {
                return &shown->open == &open;
            });
        if (found == m_projects.end())
            return;
        const ShownProject* removed = found->get();
        std::vector<BrowserTab*> closing;
        BrowserTab* survivor = nullptr;
        bool selectedCloses = false;
        for (BrowserTab& tab : tabs())
        {
            if (!tab.pageData() || resolve(*tab.pageData()).project != removed)
            {
                if (!survivor)
                    survivor = &tab;
                continue;
            }
            closing.push_back(&tab);
            if (tab.selected())
                selectedCloses = true;
        }
        if (selectedCloses && survivor)
            survivor->select();
        else if (selectedCloses)
            addTab();
        for (BrowserTab* tab : closing)
        {
            deleteTabSettings(*tab);
            tab->deleteSelf();
        }
        for (const TabTree& entry : m_tabTrees)
            entry.tree->removeProject(removed->id);
        dropListed();
        m_projects.erase(found);
        refreshPageTree();
        rebuildPages(nullptr, nullptr);
    }

    void SurfaceBrowser::showWords(const Text& words)
    {
        m_words = words;
        rebuildPages(nullptr, nullptr);
    }

    void SurfaceBrowser::connectProjectPage(ProjectPageBuilder builder)
    {
        m_projectPageBuilder = std::move(builder);
    }

    // A target starting with the path separator is a url already, an anchor and all - the root's
    // links are; anything else is looked up in the page's own project.
    std::optional<Url> SurfaceBrowser::urlOf(const std::wstring_view target,
        const PageData& from) const
    {
        if (target.starts_with(ConfigNames::pathSeparator))
            return Url::parse(target);
        const ShownProject* project = resolve(from).project;
        if (!project)
            return std::nullopt;
        return urlIn(*project, target);
    }

    // **The click is still unwinding on the page that asked**, and the answer may take that page
    // down, so the owner is told and does the closing when it sees fit. The target is a home path,
    // the project's segment after the separator - see closeCommand.
    void SurfaceBrowser::answerCommand(const std::wstring_view target)
    {
        if (!target.starts_with(ConfigNames::pathSeparator))
            return;
        ShownProject* shown = projectOf(target.substr(1));
        if (!shown)
            return;
        CloseProjectEvent event{ shown->open };
        emitEvent(event);
    }

    // **The database keeps a file by its path under its source's folder**, so the place is read
    // against the folders the project reads now, as a hint written into the file is. A page shows
    // what the last scan read, so a file gone since is said to be gone before anything runs.
    void SurfaceBrowser::openSource(const PageData& from, const SourcePlace& source)
    {
        ShownProject* shown = resolve(from).project;
        if (!shown)
            return;
        const SourceFile& file = shown->open.surface.files()[source.place.file];
        const SourceRoots roots = shown->open.project.sourceRoots();
        if (file.root >= roots.size())
        {
            shown->noteGone(file, false);
            return;
        }
        const std::filesystem::path path = (roots[file.root] / file.path).lexically_normal();
        std::error_code error{};
        if (!std::filesystem::is_regular_file(path, error))
        {
            shown->noteGone(file, true);
            return;
        }
        m_editors.open(source.language, path, source.place.line);
    }

    // Said for every page as it is built, which is before anything asks for its list: a page
    // with a list carries a strip from the start. The root holds the projects, a folder holds
    // folders or modules and a module what it exports, by construction; a type holds methods
    // where it has any.
    void SurfaceBrowser::initPageData(PageData& data)
    {
        const Target target = resolve(data);
        data.title = titleOf(data, target);
        const bool hasList = target.isRoot()
            || (target.project && !target.isGone() && !target.projectOptions && !target.searching
                && target.method.empty() && target.exportName.empty()
                && (!target.type || !methodNames(*target.type).empty()));
        if (hasList)
            data.fetchState = FetchState::HasChildren;
    }

    // The set under a page stands while the projects do, so the page is marked Fetched; a change
    // to them puts the mark back - see refreshPageData. A home's list starts as its node in the
    // tree does, with the project's options page.
    void SurfaceBrowser::fetchSubItems(PageData& data)
    {
        const Target target = resolve(data);
        if (target.isRoot())
        {
            for (const std::unique_ptr<ShownProject>& shown : m_projects)
                addSubItem(data, shown->segment);
            data.fetchState = FetchState::Fetched;
            return;
        }
        if (target.isGone() || target.projectOptions || target.searching)
            return;
        const ShownProject& project = *target.project;
        if (target.isFolder())
        {
            if (target.isHome())
                addSubItem(data, k_optionsSegment);
            for (const std::wstring& name : subfolderNames(project, target.folder))
                addSubItem(data, name);
            if (target.contents)
            {
                for (const ContentsModule& module : target.contents->modules)
                    addSubItem(data, segmentOf(module));
            }
        }
        else if (!target.type)
        {
            for (const Type* type : target.module->types)
                addSubItem(data, type->qualifiedName);
            for (const std::wstring_view name : exportNames(*target.module))
                addSubItem(data, name);
        }
        else if (target.method.empty())
        {
            for (const std::wstring_view name : methodNames(*target.type))
                addSubItem(data, name);
        }
        data.fetchState = FetchState::Fetched;
    }

    // **A page is built for one page data**, so a tab that has moved to another needs a new page
    // even where the kind of page is the same. The tab's tree is shown and follows the tab wherever
    // it has gone, and the project it stands in is told when it is another than before. A search
    // page puts its words back in the box, for every page to band from then on - unless the box
    // holds them already, spaced as typed.
    void SurfaceBrowser::showPage(BrowserTab& tab)
    {
        PageData& data = *tab.pageData();
        const Target target = resolve(data);
        data.title = titleOf(data, target);

        BrowserPage* page = static_cast<BrowserPage*>(tab.page());
        if (page && &page->pageData() != &data)
        {
            page->deleteSelf();
            tab.setPage(nullptr);
            page = nullptr;
        }
        if (!page)
        {
            if (target.projectOptions)
            {
                ProjectPage& projectPage = tab.createPage<ProjectPage>(
                    MinSize{ k_leastPageWidth, 0.0f });
                if (m_projectPageBuilder)
                    m_projectPageBuilder(projectPage.options(), target.project->open);
                page = &projectPage;
            }
            else
            {
                SurfacePage& surfacePage = tab.createPage<SurfacePage>(
                    MinSize{ k_leastPageWidth, 0.0f });
                if (target.project)
                    surfacePage.view().setWriter(*target.project);
                fill(surfacePage, target, data);
                page = &surfacePage;
            }
            tab.setPage(page);
            // The page is what the icon comes from once there is one, and the tab last asked
            // before it existed.
            tab.updateIconMode();
            page->show();
        }

        TabTree& tabTree = treeOf(tab);
        m_trees.setCurrentItem(tabTree.box);
        showTreeWidth(tab);
        SurfaceTree& tree = *tabTree.tree;
        if (!target.project)
            tree.setCurrentItem(nullptr);
        else if (target.projectOptions)
            tree.followOptions(target.project->id);
        else if (target.isHome() || target.isGone() || (target.searching && target.folder.empty()))
            tree.followHome(target.project->id);
        else
            tree.follow(target.project->id, target.folder, target.module, target.type);

        const OpenProject* shown = target.project ? &target.project->open : nullptr;
        if (shown != m_shownProject)
        {
            m_shownProject = shown;
            ProjectShownEvent event{ shown };
            emitEvent(event);
        }
        if (target.searching && trimmed(m_search.searchText()) != target.search)
            m_search.setSearchText(target.search);
        writeFound();
    }

    // **Asked as a tab's page goes with the tab**, which is the one time it is: the nodes the tree
    // has open were written into the tab's file as they opened, and the file goes too.
    void SurfaceBrowser::saveViewState(BrowserTab& tab)
    {
        const TabTrees::iterator found = std::ranges::find_if(m_tabTrees,
            [&tab](const TabTree& entry) {
                return entry.tab == &tab;
            });
        if (found == m_tabTrees.end())
            return;
        found->box->deleteSelf();
        m_tabTrees.erase(found);
    }

    IconSize SurfaceBrowser::pageIconSize(PageData& data)
    {
        return iconOf(resolve(data)).has_value() ? k_pageIconSize : IconSize{ 0.0f };
    }

    void SurfaceBrowser::paintPageIcon(PageData& data, PaintIconEvent& event)
    {
        if (const std::optional<RowIcon> icon = iconOf(resolve(data)))
            paintRowIcon(event, *icon);
    }

    IconSize SurfaceBrowser::tabIconSize(BrowserTab& tab)
    {
        if (!tab.pageData())
            return IconSize{ 0.0f };
        return pageIconSize(*tab.pageData());
    }

    void SurfaceBrowser::paintTabIcon(BrowserTab& tab, PaintIconEvent& event)
    {
        if (tab.pageData())
            paintPageIcon(*tab.pageData(), event);
    }

    // The project is taken by its segment, the folders as far as the segments after it spell
    // folders its surface has, and what follows is a search's segment or is looked up by its
    // spelling and held to the one above it, so a module's name under the wrong folder resolves to
    // nothing, as a path no project carries does.
    SurfaceBrowser::Target SurfaceBrowser::resolve(const PageData& data) const
    {
        const std::vector<std::wstring_view> segments = segmentsOf(data);
        Target result{ .depth = segments.size() };
        if (segments.empty())
            return result;
        if (segments.size() == 1 && segments.front().starts_with(k_searchPrefix))
        {
            result.searching = true;
            result.search = unescapedQuery(segments.front().substr(k_searchPrefix.size()));
            return result;
        }
        ShownProject* project = projectOf(segments.front());
        if (!project)
            return result;
        result.project = project;
        if (segments.size() == 2 && segments[1] == k_optionsSegment)
        {
            result.projectOptions = true;
            return result;
        }

        std::size_t next = 1;
        while (next < segments.size())
        {
            const std::wstring candidate = joinFolder(result.folder, segments[next]);
            if (!project->folders.contains(candidate))
                break;
            result.folder = candidate;
            ++next;
        }
        result.folderDepth = next - 1;
        if (next + 1 == segments.size() && segments[next].starts_with(k_searchPrefix))
        {
            result.searching = true;
            result.search = unescapedQuery(segments[next].substr(k_searchPrefix.size()));
            return result;
        }
        const Folders::const_iterator folder = project->folders.find(result.folder);
        if (folder != project->folders.end())
            result.contents = folder->second;
        if (next == segments.size() || !result.contents)
            return result;

        // A header's segment is its file name, and the folders before it make its name.
        const Targets& targets = project->targets;
        Targets::const_iterator found = targets.find(joinFolder(result.folder, segments[next]));
        if (found == targets.end() || !found->second.module || !found->second.module->header)
            found = targets.find(std::wstring{ segments[next] });
        if (found == targets.end() || !found->second.module || found->second.type
            || found->second.contents != result.contents)
            return result;
        result.module = found->second.module;
        if (++next == segments.size())
            return result;

        found = targets.find(std::wstring{ segments[next] });
        if (found == targets.end() || !found->second.type
            || found->second.module != result.module)
        {
            if (exports(*result.module, segments[next]))
                result.exportName = segments[next];
            return result;
        }
        result.type = found->second.type;
        if (++next == segments.size())
            return result;

        if (hasMethod(*result.type, segments[next]))
            result.method = segments[next];
        return result;
    }

    SurfaceBrowser::ShownProject* SurfaceBrowser::projectOf(const std::wstring_view segment) const
    {
        for (const std::unique_ptr<ShownProject>& shown : m_projects)
        {
            if (shown->segment == segment)
                return shown.get();
        }
        return nullptr;
    }

    SurfaceBrowser::ShownProject* SurfaceBrowser::projectOf(const ProjectId id) const
    {
        for (const std::unique_ptr<ShownProject>& shown : m_projects)
        {
            if (shown->id == id)
                return shown.get();
        }
        return nullptr;
    }

    SurfaceBrowser::ShownProject* SurfaceBrowser::projectOf(const OpenProject& open) const
    {
        for (const std::unique_ptr<ShownProject>& shown : m_projects)
        {
            if (&shown->open == &open)
                return shown.get();
        }
        return nullptr;
    }

    // A slash would split the segment in two. A project whose own segment is asked for again
    // keeps it where its name still gives it, so a project is never renumbered against itself.
    std::wstring SurfaceBrowser::segmentFor(const std::wstring_view name,
        const ShownProject* self) const
    {
        std::wstring base{ name };
        std::ranges::replace(base, ConfigNames::pathSeparator, k_segmentSlash);
        if (base.empty())
            base = k_unnamedSegment;
        std::wstring result = base;
        for (std::size_t number = 2; ; ++number)
        {
            const bool taken = std::ranges::any_of(m_projects,
                [self, &result](const std::unique_ptr<ShownProject>& shown) {
                    return shown.get() != self && shown->segment == result;
                });
            if (!taken)
                return result;
            result = base + L" " + std::to_wstring(number);
        }
    }

    void SurfaceBrowser::openPicked(const TreePickEvent& event)
    {
        const ShownProject* project = projectOf(event.project);
        if (!project)
            return;
        switch (event.kind)
        {
            case RowKind::Home:
                goTo(Url{ homePath(*project) });
                return;
            case RowKind::Options:
                goTo(Url{ optionsPath(*project) });
                return;
            case RowKind::Contents:
                goTo(Url{ pathOf(*project, event.folder, event.module, event.type) });
                return;
        }
    }

    // The tree stands apart from the page a pick's move takes down, so the move needs no wait.
    SurfaceBrowser::TabTree& SurfaceBrowser::treeOf(const BrowserTab& tab)
    {
        for (TabTree& entry : m_tabTrees)
        {
            if (entry.tab == &tab)
                return entry;
        }
        ScrollBox& box = m_trees.add<ScrollBox>(
            ScrollBars::Vertical,
            UiElement::Section
        );
        SurfaceTree& tree = box.createBody<SurfaceTree>(Padding{ 4.0f });
        m_tabTrees.push_back({ .tab = &tab, .box = &box, .tree = &tree });
        const std::size_t index = m_tabTrees.size() - 1;
        tree.onPick([this](TreePickEvent& event) {
            openPicked(event);
        });
        const BrowserTab* owner = &tab;
        tree.onToggle([this, owner](TreeToggleEvent& event) {
            for (const TabTree& entry : m_tabTrees)
            {
                if (entry.tab == owner)
                    storeTreeState(entry, event.project);
            }
        });
        for (const std::unique_ptr<ShownProject>& shown : m_projects)
            addToTree(m_tabTrees[index], *shown);
        return m_tabTrees[index];
    }

    // A project the tab's file says nothing of comes in closed.
    void SurfaceBrowser::addToTree(const TabTree& entry, const ShownProject& shown)
    {
        entry.tree->addProject(shown.id, shown.open.project.name, shown.contentsTree);
        const Dom::Section& config = settings().tabConfig(entry.tab->id());
        const TreeEntries& states = (config / k_treeNodeName).as<TreeEntries>();
        const std::wstring folder = shown.open.project.folder.wstring();
        for (const Dom::Section& state : states)
        {
            if ((state / TreeKeys::project).get<std::wstring>() != folder)
                continue;
            OpenRows open{ .home = (state / TreeKeys::home).get<bool>() };
            for (const std::wstring& name : (state / TreeKeys::folders).as<NodeNames>().get())
                open.folders.push_back(name);
            for (const std::wstring& name : (state / TreeKeys::modules).as<NodeNames>().get())
                open.modules.push_back(name);
            entry.tree->restoreRows(shown.id, open);
            return;
        }
    }

    // Kept by the project's folder, which a rename leaves where it was, so a project opened again
    // comes back as the tab left it.
    void SurfaceBrowser::storeTreeState(const TabTree& entry, const ProjectId id)
    {
        const ShownProject* shown = projectOf(id);
        if (!shown)
            return;
        const OpenRows open = entry.tree->openRows(id);
        const std::wstring folder = shown->open.project.folder.wstring();
        Dom::Section& config = settings().tabConfig(entry.tab->id());
        TreeEntries& states = (config / k_treeNodeName).as<TreeEntries>();
        Dom::Section* found = states.childWhere(TreeKeys::project, folder);
        Dom::Section& state = found ? *found : states.add();
        (state / TreeKeys::project).set(folder);
        (state / TreeKeys::home).set(open.home);
        NodeNames& folders = (state / TreeKeys::folders).as<NodeNames>();
        folders.clear();
        for (const std::wstring& name : open.folders)
            folders.add().set(name);
        NodeNames& modules = (state / TreeKeys::modules).as<NodeNames>();
        modules.clear();
        for (const std::wstring& name : open.modules)
            modules.add().set(name);
    }

    void SurfaceBrowser::showTreeWidth(const BrowserTab& tab)
    {
        const float width = (settings().tabConfig(tab.id()) / k_treeWidthName).get<float>();
        if (width > 0.0f)
            setBarExtent(PanelSlot::Left, width);
        else
            resetBarExtent(PanelSlot::Left);
    }

    void SurfaceBrowser::storeTreeWidth(const float width)
    {
        const BrowserPage* page = currentPage();
        if (!page)
            return;
        (settings().tabConfig(page->tabId()) / k_treeWidthName).set(width);
    }

    Dom::Dt::Section SurfaceBrowser::createTabConfigSchema()
    {
        return Dom::Dt::Section{
            Dom::Dt::Value{ k_treeWidthName, 0.0f },
            Dom::Dt::Sequence{
                k_treeNodeName,
                Dom::Dt::Section{
                    Dom::Dt::Value{ TreeKeys::project, std::wstring{} },
                    Dom::Dt::Value{ TreeKeys::home, false },
                    Dom::Dt::Sequence{ TreeKeys::folders, std::wstring{} },
                    Dom::Dt::Sequence{ TreeKeys::modules, std::wstring{} }
                }
            }
        };
    }

    std::wstring SurfaceBrowser::homePath(const ShownProject& project)
    {
        std::wstring result;
        result.append(1, ConfigNames::pathSeparator).append(project.segment);
        return result;
    }

    std::wstring SurfaceBrowser::optionsPath(const ShownProject& project)
    {
        std::wstring result = homePath(project);
        result.append(1, ConfigNames::pathSeparator).append(k_optionsSegment);
        return result;
    }

    // A folder's path spells its folders with the url's own separator.
    std::wstring SurfaceBrowser::pathOf(const ShownProject& project,
        const std::wstring_view folder, const ContentsModule* module, const Type* type,
        const std::wstring_view member)
    {
        std::wstring result = homePath(project);
        if (!folder.empty())
            result.append(1, ConfigNames::pathSeparator).append(folder);
        if (!module)
            return result;
        result.append(1, ConfigNames::pathSeparator).append(segmentOf(*module));
        if (type)
            result.append(1, ConfigNames::pathSeparator).append(type->qualifiedName);
        if (!member.empty())
            result.append(1, ConfigNames::pathSeparator).append(member);
        return result;
    }

    // The folders directly under one are read off the path of every folder with modules, in
    // the order the folders read, each once.
    std::vector<std::wstring> SurfaceBrowser::subfolderNames(const ShownProject& project,
        const std::wstring_view folder)
    {
        std::vector<std::wstring> result;
        for (const ContentsFolder& contents : project.contents)
        {
            std::wstring_view rest = contents.path;
            if (!folder.empty())
            {
                const bool under = rest.starts_with(folder) && rest.size() > folder.size()
                    && rest[folder.size()] == ConfigNames::pathSeparator;
                if (!under)
                    continue;
                rest.remove_prefix(folder.size() + 1);
            }
            if (rest.empty())
                continue;
            const std::wstring name{ rest.substr(0, rest.find(ConfigNames::pathSeparator)) };
            if (std::ranges::find(result, name) == result.end())
                result.push_back(name);
        }
        return result;
    }

    // A page no project carries is titled off its own segment still, so a crumb or a history
    // line names what was there.
    std::wstring SurfaceBrowser::titleOf(const PageData& data, const Target& target) const
    {
        if (target.isRoot())
            return appContext().appName().plainText();
        if (target.searching)
            return searchTitle(target.search, searchPlaceOf(target));
        if (!target.project)
            return data.name;
        if (target.isHome())
            return target.project->open.project.name;
        if (target.projectOptions)
            return std::wstring{ k_optionsTitle };
        const std::size_t folders = target.folderDepth + 1;
        if (target.depth <= folders)
            return folderNameOf(data.name);
        if (target.module && target.depth == folders + 1)
            return target.module->shownName;
        if (target.type && target.depth == folders + 2)
            return target.type->name;
        return data.name;
    }

    std::optional<RowIcon> SurfaceBrowser::iconOf(const Target& target)
    {
        if (target.isRoot())
            return RowIcon::Home;
        if (target.searching)
            return RowIcon::Search;
        if (!target.project || target.isGone() || !target.method.empty()
            || !target.exportName.empty())
        {
            return std::nullopt;
        }
        if (target.projectOptions)
            return RowIcon::Options;
        if (target.isHome())
            return RowIcon::Project;
        if (target.type)
            return rowIconOf(*target.type);
        if (target.module)
            return RowIcon::Module;
        return RowIcon::Folder;
    }

    std::optional<Page> SurfaceBrowser::pageOf(const Target& target) const
    {
        if (target.isRoot())
        {
            if (m_projects.empty())
                return std::nullopt;
            return projectsPage();
        }
        if (target.searching)
        {
            if (m_projects.empty())
                return std::nullopt;
            return searchPageOf(target);
        }
        if (target.isGone() || target.projectOptions)
            return std::nullopt;
        const ShownProject& project = *target.project;
        OpenProject& open = project.open;
        if (target.isHome())
            return homePageOf(project);
        if (target.isFolder())
        {
            return folderPage(open.surface, open.notes, project.contents, target.folder,
                open.project.sourceRoots());
        }
        if (!target.exportName.empty())
        {
            return exportPage(open.surface, open.notes, *target.contents, *target.module,
                target.exportName);
        }
        if (!target.type)
            return modulePage(open.surface, open.notes, *target.contents, *target.module);
        if (target.method.empty())
            return typePage(open.surface, open.notes, *target.type);
        return methodPage(open.surface, open.notes, *target.type, target.method);
    }

    // The home links to the project's options page by path, ahead of its folders.
    Page SurfaceBrowser::homePageOf(const ShownProject& project)
    {
        OpenProject& open = project.open;
        Page page = homePage(open.surface, open.notes, project.contents, open.project.name);
        if (project.contents.empty())
            explainEmpty(page, open.project);
        page.links = {
            Run{ .text = std::wstring{ k_optionsTitle }, .link = optionsPath(project) }
        };
        page.commands.push_back(closeCommand(project, std::wstring{ k_closeProjectCaption }));
        return page;
    }

    // Each project links to its home by path, beside the folder it reads, and closes from its row.
    Page SurfaceBrowser::projectsPage() const
    {
        TableGroup projects;
        for (const std::unique_ptr<ShownProject>& shown : m_projects)
        {
            TableRow row;
            row.pageKind = PageKind::Project;
            row.cells.push_back({
                Run{ .text = shown->open.project.name, .link = homePath(*shown) }
            });
            row.cells.push_back({
                Run{ .text = shown->open.project.folder.wstring(), .style = RunStyle::Muted }
            });
            row.command = closeCommand(*shown, std::wstring{ k_closeCaption });
            projects.rows.push_back(std::move(row));
        }
        const std::size_t count = projects.rows.size();
        Section section;
        section.kind = SectionKind::Table;
        section.heading = { Run{ .text = L"Open projects" } };
        section.hint = std::to_wstring(count) + (count == 1 ? L" element" : L" elements");
        section.table.columns = {
            TableColumn{ .name = L"Project" },
            TableColumn{ .name = L"Folder", .fills = true }
        };
        section.table.header = false;
        section.table.groups.push_back(std::move(projects));

        Page page;
        page.kind = PageKind::Projects;
        page.title = appContext().appName().plainText();
        page.sections.push_back(std::move(section));
        return page;
    }

    // The command is about the project's home path, which answerCommand reads the project off.
    Command SurfaceBrowser::closeCommand(const ShownProject& project, std::wstring text)
    {
        return {
            .text = std::move(text),
            .hint = std::wstring{ k_closeHint },
            .target = homePath(project)
        };
    }

    void SurfaceBrowser::fill(SurfacePage& page, const Target& target, const PageData& data) const
    {
        PageView& view = page.view();
        if (target.isRoot() || (target.searching && m_projects.empty()))
        {
            if (m_projects.empty())
                view.showText(m_words);
            else
                view.show(projectsPage());
            return;
        }
        if (target.searching)
        {
            view.show(searchPageOf(target));
            return;
        }
        if (target.isGone())
        {
            view.showText(goneWords(data));
            return;
        }
        if (std::optional<Page> built = pageOf(target))
            view.show(std::move(*built));
    }

    // The page datas standing are the ones restored from the settings and the ones walked since;
    // their titles were worked out against whatever projects stood then, and their lists are
    // those projects'. So were the tabs' icons, which a tab asks for again only when its page is
    // shown; a tab standing on a restored page is asked here, so it wears its mark before it is
    // selected.
    void SurfaceBrowser::refreshPageTree()
    {
        refreshPageData(homePageData());
        for (BrowserTab& tab : tabs())
            tab.updateIconMode();
    }

    void SurfaceBrowser::refreshPageData(PageData& data)
    {
        data.title.clear();
        data.fetchState = FetchState::Unfetched;
        initPageData(data);
        for (const PageDataPtr& item : data.items)
            refreshPageData(*item);
    }

    // The root's search reads every project, as the root lists them.
    bool SurfaceBrowser::reaches(const Target& target, const ShownProject* project)
    {
        if (target.isRoot() || (target.searching && !target.project))
            return true;
        return project && target.project == project;
    }

    // The selected tab is sent to the url it stands on, which builds its page again where it
    // went; any other tab's page is built when the tab is selected.
    //
    // **A page a project reaches was built since the project was added** - adding one drops every
    // page on its paths - so the target a standing page resolves to says which kind of page it is.
    void SurfaceBrowser::rebuildPages(const ShownProject* project, const BrowserPage* keep)
    {
        BrowserTab* selected = nullptr;
        for (BrowserTab& tab : tabs())
        {
            if (tab.selected())
                selected = &tab;
            const BrowserPage* page = static_cast<const BrowserPage*>(tab.page());
            if (!page || page == keep || !reaches(resolve(page->pageData()), project))
                continue;
            tab.page()->deleteSelf();
            tab.setPage(nullptr);
        }
        if (selected && selected->pageData())
            goTo(*selected->pageData(), selected->anchor(), *selected);
    }

    // **The edit that asked stands on the selected tab's page**, so that page takes the new words
    // in place - nothing under the edit is deleted - and is built again only once the edit has
    // unwound, where the words changed what the page is made of: a footnote added. Any other
    // tab's page the project reaches goes, and is built when its tab is selected. An options
    // page takes no words from the files, and stands as it is.
    void SurfaceBrowser::refreshPages(const ShownProject& project,
        std::optional<Reference> show)
    {
        for (BrowserTab& tab : tabs())
        {
            BrowserPage* standing = static_cast<BrowserPage*>(tab.page());
            if (!standing)
                continue;
            const Target target = resolve(standing->pageData());
            if (!reaches(target, &project))
                continue;
            if (!tab.selected())
            {
                standing->deleteSelf();
                tab.setPage(nullptr);
                continue;
            }
            if (target.projectOptions)
                continue;
            SurfacePage& page = static_cast<SurfacePage&>(*standing);
            std::optional<Page> built = pageOf(target);
            if (built && page.view().refresh(std::move(*built)))
                continue;
            m_rebuildFootnote = std::move(show);
            m_rebuildTimer.start(MilliSeconds{ 0u });
        }
        writeFound();
    }

    // A footnote waiting to be shown is reached the way a click on its mark reaches it, once the
    // page stands again.
    void SurfaceBrowser::rebuildCurrentPage()
    {
        const std::optional<Reference> show = std::exchange(m_rebuildFootnote, std::nullopt);
        BrowserTab* tab = nullptr;
        for (BrowserTab& each : tabs())
        {
            if (each.selected())
                tab = &each;
        }
        if (!tab || !tab->pageData())
            return;
        if (tab->page())
        {
            tab->page()->deleteSelf();
            tab->setPage(nullptr);
        }
        goTo(*tab->pageData(), tab->anchor(), *tab);
        if (!show || !tab->page() || resolve(*tab->pageData()).projectOptions)
            return;
        const SurfacePage& page = static_cast<const SurfacePage&>(*tab->page());
        const std::wstring anchor = page.view().anchorOf(*show);
        if (!anchor.empty())
            goTo(Url{ tab->pageData()->path(), anchor }, HistoryEntry::Replace);
    }

    Text SurfaceBrowser::goneWords(const PageData& data) const
    {
        const ShownProject* project = resolve(data).project;
        Text text;
        text << TextStyleId::Title << L"No such page" << PopTextStyle{};
        text << k_endLine << k_endLine;
        if (project)
            text << project->open.project.name << L" has no page at this path:";
        else
            text << L"No project open has a page at this path:";
        writePath(text, data.path());
        text << k_endLine;
        if (project)
            text << L"The tree may have been scanned again since the page was last open.";
        else
            text << L"The project may have been closed or renamed since the page was last open.";
        return text;
    }

    // The folders read are facts, the first one labelled, so that each path has a row of its own.
    void SurfaceBrowser::explainEmpty(Page& page, const Project& project)
    {
        if (project.sources.empty())
        {
            page.lead = {
                Run{ .text = L"There is nothing to document yet: the project reads no folder of "
                    L"sources. Its options page adds them." }
            };
            return;
        }
        if (project.mode == ScanMode::Full)
        {
            page.lead = {
                Run{ .text = L"There is nothing to document: no source in its folders exports a "
                    L"type. A third-party library is read as any C++ and Delphi, which the "
                    L"project's options page sets." }
            };
        }
        else
        {
            page.lead = {
                Run{ .text = L"There is nothing to document: no source in its folders declares a "
                    L"type." }
            };
        }
        std::wstring_view label = L"reads";
        for (const SourceFolder& source : project.sourceFolders())
        {
            page.facts.push_back({
                .label = std::wstring{ label },
                .value = { Run{ .text = source.path.wstring(), .style = RunStyle::Code } }
            });
            label = {};
        }
    }

    // Any function of the name, an accessor included: a crumb's line is painted every frame, and
    // the lists a type gives out are the ones that leave the accessors out.
    bool SurfaceBrowser::hasMethod(const Type& type, const std::wstring_view name)
    {
        return std::ranges::any_of(type.functions, [name](const Function& function) {
            return function.name == name;
        });
    }

    bool SurfaceBrowser::exports(const ContentsModule& module, const std::wstring_view name)
    {
        const bool isFunction = std::ranges::any_of(module.functions,
            [name](const FreeFunction* free) {
                return free->function.name == name;
            });
        const bool isConstant = std::ranges::any_of(module.constants,
            [name](const Variable* variable) {
                return variable->field.name == name;
            });
        return isFunction || isConstant;
    }

    bool SurfaceBrowser::hasRow(const Type& type, const std::wstring_view name)
    {
        const bool property = std::ranges::any_of(type.properties,
            [name](const Property& value) {
                return value.name == name;
            });
        const bool event = std::ranges::any_of(type.events, [name](const Event& value) {
            return value.alias == name;
        });
        const bool field = std::ranges::any_of(type.fields, [name](const Field& value) {
            return value.name == name;
        });
        const bool member = std::ranges::any_of(type.members, [name](const EnumMember& value) {
            return value.name == name;
        });
        return property || event || field || member;
    }

    // A member that has no page of its own is a row of its type's page, which the anchor names.
    std::optional<Url> SurfaceBrowser::urlIn(const ShownProject& project,
        const std::wstring_view target) const
    {
        const Targets::const_iterator found = project.targets.find(std::wstring{ target });
        if (found != project.targets.end())
        {
            const Target& named = found->second;
            return Url{ pathOf(project, named.folder, named.module, named.type) };
        }

        const std::optional<MemberLink> link = readMemberLink(target);
        if (!link.has_value())
            return std::nullopt;
        const Targets::const_iterator owner = project.targets.find(std::wstring{ link->type });
        if (owner == project.targets.end() || !owner->second.module)
            return std::nullopt;
        const Target& named = owner->second;
        if (named.type && !hasMethod(*named.type, link->member))
        {
            if (!hasRow(*named.type, link->member))
                return std::nullopt;
            return Url{ pathOf(project, named.folder, named.module, named.type), link->member };
        }
        if (!named.type && !exports(*named.module, link->member))
            return std::nullopt;
        return Url{ pathOf(project, named.folder, named.module, named.type, link->member) };
    }

    SurfaceBrowser::ShownProjects SurfaceBrowser::searchedProjects() const
    {
        ShownProjects result;
        if (m_shownProject)
        {
            if (const ShownProject* shown = projectOf(*m_shownProject))
            {
                result.push_back(shown);
                return result;
            }
        }
        for (const std::unique_ptr<ShownProject>& shown : m_projects)
            result.push_back(shown.get());
        return result;
    }

    std::wstring SurfaceBrowser::searchPath(const ShownProject* project,
        const std::wstring_view folder, const std::wstring_view query)
    {
        std::wstring result = project
            ? pathOf(*project, folder, nullptr, nullptr)
            : std::wstring{};
        result.append(1, ConfigNames::pathSeparator);
        result.append(k_searchPrefix);
        result.append(escapedQuery(query));
        return result;
    }

    SurfaceBrowser::ShownProjects SurfaceBrowser::searchedProjects(const Target& target) const
    {
        ShownProjects result;
        if (target.project)
        {
            result.push_back(target.project);
            return result;
        }
        for (const std::unique_ptr<ShownProject>& shown : m_projects)
            result.push_back(shown.get());
        return result;
    }

    std::wstring SurfaceBrowser::searchTitle(const std::wstring_view typed,
        const std::wstring_view place)
    {
        std::wstring result = L"Search \"";
        result.append(typed);
        result.append(L"\"");
        if (!place.empty())
            result.append(L" in ").append(place);
        return result;
    }

    std::wstring SurfaceBrowser::rootSearchPlace() const
    {
        if (m_projects.size() == 1)
            return m_projects.front()->open.project.name;
        if (m_projects.empty())
            return {};
        return L"all projects";
    }

    // A folder is named as its own crumb is, by its last segment.
    std::wstring SurfaceBrowser::searchPlaceOf(const Target& target) const
    {
        if (!target.project)
            return rootSearchPlace();
        if (!target.folder.empty())
            return folderNameOf(target.folder);
        return target.project->open.project.name;
    }

    // A project's search page links in the project, as its other pages do; the root's by url,
    // which names the project each row stands in.
    Page SurfaceBrowser::searchPageOf(const Target& target) const
    {
        const ShownProjects projects = searchedProjects(target);
        SearchedProjects searched;
        for (const ShownProject* project : projects)
        {
            searched.push_back({
                .name = project->open.project.name,
                .surface = &project->open.surface,
                .notes = &project->open.notes,
                .index = &project->index,
                .folder = target.folder
            });
        }
        const bool rooted = !target.project;
        const LinkSpeller spell = [this, &projects, rooted](const std::size_t at,
            const std::wstring_view link) {
            if (!rooted)
                return std::wstring{ link };
            const std::optional<Url> url = urlIn(*projects[at], link);
            return url.has_value() ? url->str() : std::wstring{};
        };
        return searchPage(searched, target.search, spell);
    }

    // **Ctrl+F, F3 and Shift+F3 are the search's from anywhere in the window**: the browser
    // stands over every control in it, and an action asks each control from the focused one
    // upwards.
    void SurfaceBrowser::connectSearch()
    {
        m_search.setCountPlace(L"on page");
        m_search.onSearch([this](SearchEvent& event) {
            searchRequested(event);
        });
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::find)
                event.claim({});
            else if (&event.action == &StdActions::findNext
                || &event.action == &StdActions::findPrevious)
            {
                const SurfacePage* page = surfacePage();
                event.claim({ .enabled = page && page->view().found().count() != 0 });
            }
        });
        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::find)
                m_search.startSearch();
            else if (&event.action == &StdActions::findNext)
                stepFound(FindTarget::Next);
            else if (&event.action == &StdActions::findPrevious)
                stepFound(FindTarget::Previous);
        });
    }

    // Enter, the list taken down, steps through the page's matches; the search's own page is
    // opened from its line of the list.
    void SurfaceBrowser::searchRequested(const SearchEvent& event)
    {
        switch (event.request)
        {
            case SearchRequest::Search:
                takeWords();
                return;
            case SearchRequest::Next:
                stepFound(FindTarget::Next);
                return;
            case SearchRequest::Previous:
                stepFound(FindTarget::Previous);
                return;
            case SearchRequest::Pick:
                openListed(event.line, event.modifiers);
                return;
            case SearchRequest::End:
                handBackFocus();
                return;
        }
    }

    // **One box for both searches**: the names of the projects drop under it while it is typed
    // in, and the pages band its words however they came there - typed, or put back by a search
    // page shown.
    void SurfaceBrowser::takeWords()
    {
        // The searcher knows nothing of the pages, so the bands are repainted from here. A page
        // not shown is painted afresh when it is.
        const std::wstring words = searchedWords(m_search.searchText());
        if (m_searcher.setText(words, SearchSplit::Words))
        {
            if (BrowserPage* page = currentPage())
                page->invalidate();
        }
        if (m_search.containsNested(Input::focusedControl()))
            listMatches();
        else
            dropListed();
        writeFound();
    }

    SurfacePage* SurfaceBrowser::surfacePage()
    {
        BrowserPage* page = currentPage();
        if (!page || resolve(page->pageData()).projectOptions)
            return nullptr;
        return static_cast<SurfacePage*>(page);
    }

    // **The focus goes with the match from the page**, so the keys that follow act where the
    // match stands; in the box it stays, for the next step.
    void SurfaceBrowser::stepFound(const FindTarget target)
    {
        SurfacePage* page = surfacePage();
        if (!page)
            return;
        const bool fromPage = page->containsNested(Input::focusedControl());
        page->view().found().step(target);
        if (fromPage)
            page->view().focusFound();
        writeFound();
    }

    // An options page counts as a page holding none.
    void SurfaceBrowser::writeFound()
    {
        if (const SurfacePage* page = surfacePage())
        {
            const SearchResult& found = page->view().found();
            m_search.setResult(found.currentIndex(), found.count());
        }
        else
            m_search.setResult(std::nullopt, 0);
        // A presenter keeps the answer it was last given until it is asked again.
        StdActions::findNext.invalidateState();
        StdActions::findPrevious.invalidateState();
    }

    void SurfaceBrowser::openLinked(const PageData& from, const Url& url)
    {
        m_searchOpens.reset();
        if (resolve(from).searching)
            m_searchOpens = url;
        goToLater(url);
    }

    // **Asked of the url the tab arrived at**, so an opening refused, or one that moved nothing,
    // is not taken up by the next page shown.
    void SurfaceBrowser::pageArrived(SurfacePage& page, const Control& opening)
    {
        const std::optional<Url> opened = std::exchange(m_searchOpens, std::nullopt);
        if (opened == page.tab().url())
            page.view().found().stepFrom(opening);
        writeFound();
    }

    // **What was searched for takes the focus first**: the page's current match. A tab not
    // selected keeps its tree and its page, hidden, so a control there is passed over, and a page
    // with nothing to take the focus leaves it in the box.
    void SurfaceBrowser::handBackFocus()
    {
        SurfacePage* page = surfacePage();
        if (page && page->view().focusFound())
            return;
        if (m_focusOrigin && shownWithin(*m_focusOrigin, *this) && m_focusOrigin->canTakeFocus())
        {
            m_focusOrigin->setFocus();
            return;
        }
        if (page)
            page->view().focusContent();
    }

    // Read before the walk ends, while the focus is still where it comes from - and only inside the
    // browser, which is told of its controls' deletion.
    void SurfaceBrowser::nestedControlFocusing(FocusEvent& event)
    {
        Control* from = Input::focusedControl();
        if (m_search.containsNested(event.control)
            && containsNested(from)
            && !m_search.containsNested(from))
        {
            m_focusOrigin = from;
        }
        BrowserControl::nestedControlFocusing(event);
    }

    void SurfaceBrowser::nestedControlDeleted(Control* value)
    {
        if (value == m_focusOrigin)
            m_focusOrigin = nullptr;
        BrowserControl::nestedControlDeleted(value);
    }

    // **The searches lead**, a divider under the last, and the names follow. No line is current
    // until the keys go down the list, so Enter steps through the page's matches. The best names of
    // each project searched are merged by how they hold the words, and a line names its project
    // where several are searched.
    void SurfaceBrowser::listMatches()
    {
        m_listed.clear();
        const std::wstring_view typed = trimmed(m_search.searchText());
        if (typed.empty())
        {
            m_search.hideList();
            return;
        }

        // A name found, and the project it was found in.
        struct Found
        {
            NameHit hit;
            const ShownProject* project{ nullptr };
        };

        const ShownProjects projects = searchedProjects();
        std::vector<Found> found;
        for (const ShownProject* project : projects)
        {
            for (const NameHit& hit : project->index.find(typed, k_listedNames))
                found.push_back({ .hit = hit, .project = project });
        }
        std::ranges::stable_sort(found, [](const Found& left, const Found& right) {
            return left.hit.rank < right.hit.rank;
        });
        if (found.size() > k_listedNames)
            found.resize(k_listedNames);

        const bool namesProject = projects.size() > 1;
        DropListLines lines;
        for (const SearchPlace& place : searchPlaces())
        {
            m_listed.push_back({
                .project = nullptr,
                .entry = nullptr,
                .searchPath = searchPath(place.project, place.folder, typed)
            });
            lines.push_back(searchLine(typed, place.name));
        }
        lines.back().dividerBelow = true;
        for (const Found& each : found)
        {
            m_listed.push_back({
                .project = each.project,
                .entry = each.hit.entry,
                .searchPath = {}
            });
            lines.push_back(lineOf(m_listed.back(), each.hit, namesProject));
        }
        m_search.showList(std::move(lines), std::nullopt);
    }

    // **The narrowest place first**: the folder the shown page stands in, its project, then all
    // projects. A place that searches what the one before it does is left out - the home's folder
    // is its project, and one project open is all of them - and the root offers its own search.
    SurfaceBrowser::SearchPlaces SurfaceBrowser::searchPlaces()
    {
        const BrowserPage* page = currentPage();
        const Target target = page ? resolve(page->pageData()) : Target{};
        SearchPlaces result;
        if (target.project)
        {
            if (!target.folder.empty())
            {
                result.push_back({
                    .project = target.project,
                    .folder = target.folder,
                    .name = folderNameOf(target.folder)
                });
            }
            result.push_back({
                .project = target.project,
                .folder = {},
                .name = target.project->open.project.name
            });
        }
        if (!target.project || m_projects.size() > 1)
            result.push_back({ .project = nullptr, .folder = {}, .name = rootSearchPlace() });
        return result;
    }

    // The mark of what the name is - a member's line keeps the room blank - a member's type muted
    // before it, the typed words in the accent and the kind at the line's end; the hint is the
    // declaration's words and where it stands.
    DropListLine SurfaceBrowser::lineOf(const ListedLine& listed, const NameHit& hit,
        const bool namesProject) const
    {
        const NamedEntry& entry = *listed.entry;
        DropListLine line;
        Text& text = line.text;
        if (entry.kind == NameKind::Type)
            text << rowIcon(rowIconOf(*entry.type)) << Space{ k_iconGap };
        else if (entry.kind == NameKind::Folder)
            text << rowIcon(RowIcon::Folder) << Space{ k_iconGap };
        else if (entry.kind == NameKind::Module)
            text << rowIcon(RowIcon::Module) << Space{ k_iconGap };
        else
            text << Space{ k_rowIconSize + k_iconGap };
        if (entry.type && entry.kind != NameKind::Type)
            text << InkGrade::Muted << entry.type->name << k_scope << PopColor{};
        const std::wstring_view name = entry.name;
        if (hit.start.has_value() && *hit.start + hit.length <= name.size())
        {
            text << name.substr(0, *hit.start);
            text << InkWell::accentInk() << name.substr(*hit.start, hit.length) << PopColor{};
            text << name.substr(*hit.start + hit.length);
        }
        else
        {
            text << name;
        }
        text << FlexSpace{ k_kindGap } << TextStyleId::SubBody << InkGrade::Muted;
        text << entry.kindWord;
        if (namesProject)
            text << k_separator << listed.project->open.project.name;
        text << PopColor{} << PopTextStyle{};

        if (entry.comment && !entry.comment->text.empty())
        {
            writeRuns(line.hint, hintRunsOf(listed.project->open.surface, *entry.comment,
                entry.nameSpace));
            line.hint << k_endLine;
        }
        std::wstring place = entry.folder;
        if (entry.module && entry.kind != NameKind::Module)
        {
            if (!place.empty())
                place += k_sourceJoin;
            place += entry.module->shownName;
        }
        if (place.empty())
            place = listed.project->open.project.name;
        line.hint << InkGrade::Muted << place << PopColor{};
        return line;
    }

    DropListLine SurfaceBrowser::searchLine(const std::wstring_view typed,
        const std::wstring_view place)
    {
        DropListLine line;
        line.text << rowIcon(RowIcon::Search) << Space{ k_iconGap } << searchTitle(typed, place);
        return line;
    }

    void SurfaceBrowser::openListed(const std::size_t line, const KeyModifiers modifiers)
    {
        if (line >= m_listed.size())
            return;
        const ListedLine& listed = m_listed[line];
        if (!listed.entry)
        {
            openSearch(Url{ listed.searchPath }, modifiers);
            return;
        }
        const std::optional<Url> url = urlIn(*listed.project, listed.entry->link);
        if (!url.has_value())
            return;
        if (modifiers.alt)
        {
            addTab(*url, SelectTab::No);
            return;
        }
        m_searchOpens = *url;
        goTo(*url);
    }

    void SurfaceBrowser::openSearch(const Url& url, const KeyModifiers modifiers)
    {
        if (modifiers.alt)
            addTab(url, SelectTab::No);
        else
            goTo(url);
    }

    void SurfaceBrowser::dropListed()
    {
        m_search.hideList();
        m_listed.clear();
    }
}
