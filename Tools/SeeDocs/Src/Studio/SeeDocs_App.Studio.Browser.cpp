module SeeDocs_App.Studio.Browser;

import SeeDocs_App.Studio.Editing;
import SeeDocs_App.Studio.Icons;
import SeeDocs_App.Studio.PageView;
import SeeDocs_App.Studio.SurfaceTree;
import SeeDocs_App.Database;
import SeeDocs_App.Edits;
import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.Browser.Consts;
import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
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
        constexpr float k_wordsIndent = 12.0f;   // of a path quoted in the studio's words
        constexpr std::wstring_view k_noteExtension = L".md";
        constexpr std::wstring_view k_sourceJoin = L" \u203A ";
        constexpr std::wstring_view k_separator = L" \u00B7 ";
        constexpr std::wstring_view k_noProject = L"No project is open.";

        // The names down a path, home's empty one left out: the chapter's first.
        [[nodiscard]] std::vector<std::wstring_view> segmentsOf(const PageData& data)
        {
            std::vector<std::wstring_view> result;
            for (const PageData* walk = &data; walk && walk->parent; walk = walk->parent)
                result.push_back(walk->name);
            std::ranges::reverse(result);
            return result;
        }

        // A folder's category with one more folder under it; the root's is the folder alone.
        [[nodiscard]] std::wstring joinFolder(const std::wstring_view folder,
            const std::wstring_view name)
        {
            if (folder.empty())
                return std::wstring{ name };
            std::wstring result{ folder };
            result.append(1, ConfigNames::pathSeparator).append(name);
            return result;
        }
    }

    // SurfacePage

    SurfaceBrowser& SurfacePage::browser() const
    {
        return static_cast<SurfaceBrowser&>(tab().browserControl());
    }

    // The link a menu is up for on the page, as the path of the surface it names - so Open and
    // Open in new tab are about that link, and greyed for one the surface does not carry.
    std::wstring SurfacePage::pathToOpen()
    {
        const std::wstring_view target = m_view.menuLink();
        if (target.empty())
            return {};
        const std::optional<Url> url = browser().urlOf(target);
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
        if (const std::optional<Url> url = browser().urlOf(target))
            browser().goToLater(*url);
    }

    // SurfaceBrowser

    bool SurfaceBrowser::Target::isGone() const
    {
        std::size_t resolved = folderDepth;
        if (module)
            ++resolved;
        if (type)
            ++resolved;
        if (!method.empty())
            ++resolved;
        return depth > resolved;
    }

    bool SurfaceBrowser::bind(Surface& surface, Notes& notes, const Project& project,
        const OpenTabs openTabs)
    {
        m_surface = &surface;
        m_notes = &notes;
        m_projectName = project.name;
        m_sources = project.folder;
        m_database = project.databasePath();
        m_words = {};
        m_contents = contentsOf(surface);
        m_targets.clear();
        m_folders.clear();
        for (const ContentsChapter& chapter : m_contents)
        {
            m_targets[chapter.name] = { .chapter = &chapter };
            // Every folder on the way to the chapter's is a folder, whether or not a chapter
            // of its own; the chapter's own folder is this chapter.
            std::wstring folder;
            std::wstring_view rest = chapter.category;
            while (!rest.empty())
            {
                const std::size_t slash = rest.find(ConfigNames::pathSeparator);
                folder = joinFolder(folder, rest.substr(0, slash));
                m_folders.try_emplace(folder, nullptr);
                rest = slash == std::wstring_view::npos ? std::wstring_view{} : rest.substr(slash + 1);
            }
            m_folders[chapter.category] = &chapter;
            for (const ContentsModule& module : chapter.modules)
            {
                m_targets[module.name] = { .chapter = &chapter, .module = &module };
                for (const Type* type : module.types)
                {
                    m_targets[type->qualifiedName] = {
                        .chapter = &chapter,
                        .module = &module,
                        .type = type
                    };
                }
            }
        }
        m_tree.build(m_contents);

        if (openTabs == OpenTabs::Close)
            resetToHome();
        else
        {
            refreshPageData(homePageData());
            rebuildPages();
        }
        return !m_contents.empty();
    }

    void SurfaceBrowser::showWords(const Text& words)
    {
        m_surface = nullptr;
        m_notes = nullptr;
        m_projectName.clear();
        m_sources.clear();
        m_database.clear();
        m_words = words;
        m_contents.clear();
        m_targets.clear();
        m_folders.clear();
        m_tree.clear();
        resetToHome();
    }

    std::optional<Url> SurfaceBrowser::urlOf(const std::wstring_view target) const
    {
        const Targets::const_iterator found = m_targets.find(std::wstring{ target });
        if (found != m_targets.end())
        {
            const Target& named = found->second;
            return Url{ pathOf(named.chapter, named.module, named.type) };
        }

        const std::optional<MemberLink> link = readMemberLink(target);
        if (!link.has_value())
            return std::nullopt;
        const Targets::const_iterator owner = m_targets.find(std::wstring{ link->type });
        if (owner == m_targets.end() || !owner->second.type
            || !hasMethod(*owner->second.type, link->member))
            return std::nullopt;
        const Target& named = owner->second;
        return Url{ pathOf(named.chapter, named.module, named.type, link->member) };
    }

    std::wstring SurfaceBrowser::writeHint(const CommentSite& site, const std::wstring_view hint)
    {
        // Unchanged words are no edit, so the database and the page stay as they are.
        if (!changesHint(site, hint))
            return {};
        if (!m_surface)
            return std::wstring{ k_noProject };
        const Refusal refusal = SeeDocs_App::writeHint(*m_surface, m_sources, site, hint);
        if (!refusal.empty())
            return refusal;
        writeDatabase(*m_surface, m_database);
        refreshPages(std::nullopt);
        return {};
    }

    // The editor's bar names the note and the section, and how many comments reach it - or that
    // none does, the section being the type's by its name.
    FootnoteDraft SurfaceBrowser::draftFootnote(const Reference& reference)
    {
        FootnoteDraft draft;
        draft.reference = reference;
        const NoteSection* section = m_notes
            ? m_notes->section(reference.note, reference.anchor)
            : nullptr;
        if (!m_surface || !section)
        {
            draft.refusal = reference.note + std::wstring{ k_noteExtension }
                + L" no longer holds this section.";
            return draft;
        }
        draft.text = flowedText(section->lines);
        draft.place = reference.note + std::wstring{ k_noteExtension }
            + std::wstring{ k_sourceJoin } + section->heading;
        const std::size_t uses = referencesTo(*m_surface, reference);
        draft.place += k_separator;
        if (uses == 0)
            draft.place += L"matched by name";
        else if (uses == 1)
            draft.place += L"reached from 1 declaration";
        else
            draft.place += L"reached from " + std::to_wstring(uses) + L" declarations";
        return draft;
    }

    FootnoteDraft SurfaceBrowser::draftFootnote(const CommentSite& site, const FootnoteHome& home)
    {
        FootnoteDraft draft;
        draft.site = site;
        if (!m_notes)
        {
            draft.refusal = std::wstring{ k_noProject };
            return draft;
        }
        std::optional<NewFootnote> placed = placeFootnote(*m_notes, site, home.note, home.after,
            home.owner, draft.refusal);
        if (!placed)
            return draft;
        draft.reference = placed->reference;
        draft.place = placed->reference.note + std::wstring{ k_noteExtension }
            + std::wstring{ k_sourceJoin } + placed->heading + std::wstring{ k_separator }
            + L"new, with a See in the comment";
        draft.placed = std::move(placed);
        return draft;
    }

    std::wstring SurfaceBrowser::writeFootnote(const FootnoteDraft& draft,
        const std::wstring_view text)
    {
        if (!m_surface || !m_notes)
            return std::wstring{ k_noProject };
        if (draft.placed && draft.site)
        {
            const Refusal refusal = addFootnote(*m_surface, *m_notes, m_sources, *draft.site,
                *draft.placed, text);
            if (!refusal.empty())
                return refusal;
            writeDatabase(*m_surface, m_database);
            refreshPages(draft.placed->reference);
            return {};
        }
        const Refusal refusal = SeeDocs_App::writeFootnote(*m_notes, draft.reference, text);
        if (!refusal.empty())
            return refusal;
        refreshPages(std::nullopt);
        return {};
    }

    // Said for every page as it is built, which is before anything asks for its list: a page
    // with a list carries a strip from the start. A folder holds folders or modules by
    // construction; a type holds methods where it has any.
    void SurfaceBrowser::initPageData(PageData& data)
    {
        const Target target = resolve(data);
        data.title = titleOf(data, target);
        const bool hasList = target.isHome()
            || (!target.isGone() && target.method.empty()
                && (!target.type || !methodNames(*target.type).empty()));
        if (hasList)
            data.fetchState = FetchState::HasChildren;
    }

    // The set under a page stands while the surface does, so the page is marked Fetched; another
    // surface puts the mark back - see refreshPageData.
    void SurfaceBrowser::fetchSubItems(PageData& data)
    {
        const Target target = resolve(data);
        if (target.isGone())
            return;
        if (target.isFolder())
        {
            for (const std::wstring& name : foldersUnder(target.folder))
                addSubItem(data, name);
            if (target.chapter)
            {
                for (const ContentsModule& module : target.chapter->modules)
                    addSubItem(data, module.name);
            }
        }
        else if (!target.type)
        {
            for (const Type* type : target.module->types)
                addSubItem(data, type->qualifiedName);
        }
        else if (target.method.empty())
        {
            for (const std::wstring_view name : methodNames(*target.type))
                addSubItem(data, name);
        }
        data.fetchState = FetchState::Fetched;
    }

    // A PAGE IS BUILT FOR ONE PAGE DATA, so a tab that has moved to another needs a new page even
    // where the kind of page is the same. The tree follows the tab wherever it has gone.
    void SurfaceBrowser::showPage(BrowserTab& tab)
    {
        PageData& data = *tab.pageData();
        const Target target = resolve(data);
        data.title = titleOf(data, target);

        SurfacePage* page = static_cast<SurfacePage*>(tab.page());
        if (page && &page->pageData() != &data)
        {
            page->deleteSelf();
            tab.setPage(nullptr);
            page = nullptr;
        }
        if (!page)
        {
            page = &tab.createPage<SurfacePage>();
            page->view().setWriter(*this);
            fill(*page, target, data);
            tab.setPage(page);
            // The page is what the icon comes from once there is one, and the tab last asked
            // before it existed.
            tab.updateIconMode();
            page->show();
        }
        if (target.isGone())
            m_tree.follow(nullptr, nullptr, nullptr);
        else
            m_tree.follow(target.chapter, target.module, target.type);
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

    // The folders are taken as far as the segments spell folders the surface has, and what
    // follows is looked up by its spelling and held to the one above it, so a module's name
    // under the wrong folder resolves to nothing, as a path the surface does not carry does.
    SurfaceBrowser::Target SurfaceBrowser::resolve(const PageData& data) const
    {
        const std::vector<std::wstring_view> segments = segmentsOf(data);
        Target result{ .depth = segments.size() };
        std::size_t next = 0;
        while (next < segments.size())
        {
            const std::wstring candidate = joinFolder(result.folder, segments[next]);
            if (!m_folders.contains(candidate))
                break;
            result.folder = candidate;
            ++next;
        }
        result.folderDepth = next;
        const Folders::const_iterator folder = m_folders.find(result.folder);
        if (folder != m_folders.end())
            result.chapter = folder->second;
        if (next == segments.size() || !result.chapter)
            return result;

        Targets::const_iterator found = m_targets.find(std::wstring{ segments[next] });
        if (found == m_targets.end() || !found->second.module || found->second.type
            || found->second.chapter != result.chapter)
            return result;
        result.module = found->second.module;
        if (++next == segments.size())
            return result;

        found = m_targets.find(std::wstring{ segments[next] });
        if (found == m_targets.end() || !found->second.type
            || found->second.module != result.module)
            return result;
        result.type = found->second.type;
        if (++next == segments.size())
            return result;

        if (hasMethod(*result.type, segments[next]))
            result.method = segments[next];
        return result;
    }

    // A chapter's folders are its category, which spells them with the path's own separator.
    std::wstring SurfaceBrowser::pathOf(const ContentsChapter* chapter,
        const ContentsModule* module, const Type* type, const std::wstring_view method)
    {
        std::wstring result;
        if (!chapter)
            return result;
        if (!chapter->category.empty())
            result.append(1, ConfigNames::pathSeparator).append(chapter->category);
        if (!module)
            return result;
        result.append(1, ConfigNames::pathSeparator).append(module->name);
        if (!type)
            return result;
        result.append(1, ConfigNames::pathSeparator).append(type->qualifiedName);
        if (!method.empty())
            result.append(1, ConfigNames::pathSeparator).append(method);
        return result;
    }

    // The folders directly under one are read off every chapter's category, in the order the
    // chapters read, each once.
    std::vector<std::wstring> SurfaceBrowser::foldersUnder(const std::wstring_view folder) const
    {
        std::vector<std::wstring> result;
        for (const ContentsChapter& chapter : m_contents)
        {
            std::wstring_view rest = chapter.category;
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

    // A page the surface does not carry is titled off its own segment still, so a crumb or a
    // history line names what was there.
    std::wstring SurfaceBrowser::titleOf(const PageData& data, const Target& target) const
    {
        if (target.isHome())
        {
            if (m_surface)
                return m_projectName;
            return appContext().appName().plainText();
        }
        if (target.depth <= target.folderDepth)
            return folderNameOf(data.name);
        if (target.module && target.depth == target.folderDepth + 1)
            return target.module->shortName;
        if (target.type && target.depth == target.folderDepth + 2)
            return target.type->name;
        return data.name;
    }

    std::optional<RowIcon> SurfaceBrowser::iconOf(const Target& target)
    {
        if (target.isGone() || target.isHome() || !target.method.empty())
            return std::nullopt;
        if (target.type)
            return rowIconOf(*target.type);
        if (target.module)
            return RowIcon::Module;
        return RowIcon::Chapter;
    }

    std::optional<Page> SurfaceBrowser::pageOf(const Target& target) const
    {
        if (!m_surface || target.isGone())
            return std::nullopt;
        if (target.isHome())
            return contentsPage(m_contents, m_projectName);
        if (target.isFolder() && !target.chapter)
        {
            return contentsPage(chaptersUnder(m_contents, target.folder),
                folderNameOf(target.folder));
        }
        if (target.isFolder())
            return chapterPage(*m_surface, *m_notes, m_contents, *target.chapter);
        if (!target.type)
            return modulePage(*m_surface, *m_notes, *target.chapter, *target.module);
        if (target.method.empty())
            return typePage(*m_surface, *m_notes, *target.type);
        return methodPage(*m_surface, *m_notes, *target.type, target.method);
    }

    void SurfaceBrowser::fill(SurfacePage& page, const Target& target, const PageData& data) const
    {
        PageView& view = page.view();
        if (!m_surface)
        {
            view.showText(m_words);
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
    // their titles were worked out against whatever surface stood then, and their lists are the
    // old surface's.
    void SurfaceBrowser::refreshPageData(PageData& data)
    {
        data.title.clear();
        data.fetchState = FetchState::Unfetched;
        initPageData(data);
        for (const PageDataPtr& item : data.items)
            refreshPageData(*item);
    }

    // The selected tab is sent to the url it stands on, which builds its page; any other tab's
    // page is built when the tab is selected.
    void SurfaceBrowser::rebuildPages()
    {
        BrowserTab* selected = nullptr;
        for (BrowserTab& tab : tabs())
        {
            if (tab.page())
            {
                tab.page()->deleteSelf();
                tab.setPage(nullptr);
            }
            if (tab.selected())
                selected = &tab;
        }
        if (selected)
            goTo(*selected->pageData(), selected->anchor(), *selected);
    }

    // THE EDIT THAT ASKED STANDS ON THE SELECTED TAB'S PAGE, so that page takes the new words in
    // place - nothing under the edit is deleted - and is built again only once the edit has
    // unwound, where the words changed what the page is made of: a footnote added. Any other
    // tab's page goes, and is built when its tab is selected.
    void SurfaceBrowser::refreshPages(std::optional<Reference> show)
    {
        for (BrowserTab& tab : tabs())
        {
            SurfacePage* page = static_cast<SurfacePage*>(tab.page());
            if (!page)
                continue;
            if (!tab.selected())
            {
                page->deleteSelf();
                tab.setPage(nullptr);
                continue;
            }
            std::optional<Page> built = pageOf(resolve(page->pageData()));
            if (built && page->view().refresh(std::move(*built)))
                continue;
            m_rebuildFootnote = std::move(show);
            m_rebuildTimer.start(MilliSeconds{ 0u });
        }
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
        const SurfacePage* page = static_cast<const SurfacePage*>(tab->page());
        if (!show || !page)
            return;
        const std::wstring anchor = page->view().anchorOf(*show);
        if (!anchor.empty())
            goTo(Url{ tab->pageData()->path(), anchor }, HistoryEntry::Replace);
    }

    Text SurfaceBrowser::goneWords(const PageData& data) const
    {
        Text text;
        text << TextStyleId::Title << L"No such page" << PopTextStyle{};
        text << k_endLine << k_endLine;
        text << m_projectName << L" has no page at this path:";
        text << k_endLine << ParaIndent{ k_wordsIndent };
        text << PushThemeColor{ InkWell::spotInk() } << data.path() << PopColor{};
        text << ParaIndent{ 0.0f } << k_endLine << k_endLine;
        text << L"The tree may have been scanned again since the page was last open.";
        return text;
    }

    // Any function of the name, an accessor included: a crumb's line is painted every frame, and
    // the lists a type gives out are the ones that leave the accessors out.
    bool SurfaceBrowser::hasMethod(const Type& type, const std::wstring_view name)
    {
        return std::ranges::any_of(type.functions, [name](const Function& function) {
            return function.name == name;
        });
    }
}
