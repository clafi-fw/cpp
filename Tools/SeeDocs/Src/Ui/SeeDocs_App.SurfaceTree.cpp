module SeeDocs_App.SurfaceTree;

import SeeDocs_App.Icons;
import SeeDocs_App.Pages;
import SeeDocs_App.Project;
import SeeDocs_App.SidebarEdits;
import SeeDocs_App.Surface;

import ClaFi.Browser.Actions;

import ClaFi.Controls.Tree;
import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        constexpr wchar_t k_pathSeparator = L'/';   // between an item's names, and after them

        // An item's path with one more name under it; the top's is the name alone.
        [[nodiscard]] std::wstring joinPath(const std::wstring_view path,
            const std::wstring_view name)
        {
            if (path.empty())
                return std::wstring{ name };
            std::wstring result{ path };
            result.append(1, k_pathSeparator).append(name);
            return result;
        }

        // What follows the item's path and its slash in a key; nothing where the key is not
        // the item's.
        [[nodiscard]] std::optional<std::wstring_view> underItem(const std::wstring_view key,
            const std::wstring_view item)
        {
            if (!key.starts_with(item) || key.size() <= item.size()
                || key[item.size()] != k_pathSeparator)
            {
                return std::nullopt;
            }
            return key.substr(item.size() + 1);
        }

        void paintSidebarMark(PaintIconEvent& event)
        {
            paintRowIcon(event, RowIcon::Sidebar);
        }
    }

    namespace Actions
    {
        Action editSidebar{ Text{ L"Edit sidebar" }, Action::OnPaintIcon{ paintSidebarMark } };
    }

    // TreePickEvent

    TreePickEvent::TreePickEvent(const ProjectId pickedProject, const RowKind pickedKind,
        const std::wstring_view pickedItem, const std::wstring_view pickedFolder,
        const ContentsModule* pickedModule, const Type* pickedType, const PickTab pickedTab)
        :
        ::ClaFi::Event{},
        project{ pickedProject },
        kind{ pickedKind },
        item{ pickedItem },
        folder{ pickedFolder },
        module{ pickedModule },
        type{ pickedType },
        tab{ pickedTab }
    {
    }

    // TreeToggleEvent

    TreeToggleEvent::TreeToggleEvent(const ProjectId toggledProject)
        :
        ::ClaFi::Event{},
        project{ toggledProject }
    {
    }

    // TreeEditSidebarEvent

    TreeEditSidebarEvent::TreeEditSidebarEvent(const ProjectId editedProject, IndexPath rowItem)
        :
        ::ClaFi::Event{},
        project{ editedProject },
        item{ std::move(rowItem) }
    {
    }

    // SurfaceTree

    // **A row is picked by a click, not by becoming current**: the arrow keys walk the rows without
    // opening a page for each one passed, and Enter opens the row they stop on.
    void SurfaceTree::addProject(const ProjectId id, const std::wstring_view name,
        const SidebarItems& sidebar, const ContentsTree& contents)
    {
        std::unique_ptr<ProjectRows> made = std::make_unique<ProjectRows>();
        ProjectRows& rows = *made;
        rows.id = id;
        rows.sidebar = &sidebar;
        rows.contents = &contents;
        rows.items = flattenItems(sidebar);
        if (!m_projects.empty())
            rows.above = &addDivider(Thickness::Heavy);
        rows.node = &addNode(projectText(name));
        rows.node->header().setExpanded(false);
        rows.node->header().onClick([this, id](ClickEvent&) {
            pickHome(id, PickTab::Shown);
        });
        m_projects.push_back(std::move(made));
        connectNode(rows, *rows.node, {});
    }

    // **The rows standing were made over the sidebar and the contents before**, which the project
    // has let go of by now, so they are taken down by what was made and not by what those say.
    void SurfaceTree::rebuildProject(const ProjectId id, const SidebarItems& sidebar,
        const ContentsTree& contents, const OpenRows& open)
    {
        ProjectRows* rows = rowsOf(id);
        if (!rows)
            return;
        clearRows(*rows);
        rows->sidebar = &sidebar;
        rows->contents = &contents;
        rows->items = flattenItems(sidebar);
        restoreRows(id, open);
    }

    void SurfaceTree::renameProject(const ProjectId id, const std::wstring_view name)
    {
        if (ProjectRows* rows = rowsOf(id))
        {
            ExpanderHeader& header = rows->node->header();
            header.text() = projectText(name);
            header.invalidateFormAlign();
        }
    }

    // The current row is let go before the node goes, so it never names a dead row. The line
    // over the node goes with it, and the project that is first afterwards loses its own.
    void SurfaceTree::removeProject(const ProjectId id)
    {
        const Projects::iterator found = std::ranges::find_if(m_projects,
            [id](const std::unique_ptr<ProjectRows>& rows) {
                return rows->id == id;
            });
        if (found == m_projects.end())
            return;
        if (holdsCurrentRow(**found))
            setCurrentItem(nullptr);
        if ((*found)->above)
            (*found)->above->deleteSelf();
        (*found)->node->deleteSelf();
        m_projects.erase(found);
        if (!m_projects.empty() && m_projects.front()->above)
        {
            m_projects.front()->above->deleteSelf();
            m_projects.front()->above = nullptr;
        }
    }

    // The heavy line stands over every node but the first, so the lines follow the nodes about.
    void SurfaceTree::orderProjects(const std::span<const ProjectId> ids)
    {
        Projects ordered{};
        ordered.reserve(m_projects.size());
        for (const ProjectId id : ids)
        {
            const Projects::iterator found = std::ranges::find_if(m_projects,
                [id](const std::unique_ptr<ProjectRows>& rows) {
                    return rows->id == id;
                });
            if (found == m_projects.end())
                continue;
            ordered.push_back(std::move(*found));
            m_projects.erase(found);
        }
        for (std::unique_ptr<ProjectRows>& rows : m_projects)
            ordered.push_back(std::move(rows));
        m_projects = std::move(ordered);
        std::size_t place = 0ull;
        for (std::size_t i = 0ull; i != m_projects.size(); ++i)
        {
            ProjectRows& rows = *m_projects[i];
            if (i == 0ull && rows.above)
            {
                rows.above->deleteSelf();
                rows.above = nullptr;
            }
            if (i != 0ull && !rows.above)
                rows.above = &addDivider(Thickness::Heavy);
            if (rows.above)
                moveControl(*rows.above, place++);
            moveControl(*rows.node, place++);
        }
    }

    // Opened from the row up, so the row stands in the tree before it is made current. A divider
    // has no row to show, and the item's category stands in for it.
    void SurfaceTree::follow(const ProjectId id, const std::wstring_view item,
        const std::wstring_view folder, const ContentsModule* module, const Type* type)
    {
        ProjectRows* rows = rowsOf(id);
        if (!rows)
        {
            followHome(id);
            return;
        }
        const std::optional<std::size_t> apis = foldersAt(*rows, item);
        const std::optional<std::size_t> contents = apis.has_value()
            ? rows->contents->rowOf(folder, module, type)
            : std::nullopt;
        if (contents.has_value())
        {
            ensureContentsRow(*rows, *apis, *contents);
            const ItemRows& apisRows = rows->items[*apis];
            const ContentsRows& contentsRows = rows->contents->rows();
            std::optional<std::size_t> at = contentsRows[*contents].parent;
            while (at.has_value())
            {
                apisRows.made[*at].node->header().setExpanded(true);
                at = contentsRows[*at].parent;
            }
            if (apisRows.node)
                apisRows.node->header().setExpanded(true);
            showRow(*rows, *apis, rowOf(apisRows.made[*contents]));
            return;
        }
        const std::optional<std::size_t> index = itemOf(*rows, item);
        if (!index.has_value())
        {
            followHome(id);
            return;
        }
        const ItemRows& itemRows = rows->items[*index];
        ensureItemRow(*rows, *index);
        if (Control* row = rowOf(itemRows))
            showRow(*rows, *index, *row);
        else if (itemRows.parent.has_value())
            showRow(*rows, *itemRows.parent, *rowOf(rows->items[*itemRows.parent]));
        else
            followHome(id);
    }

    void SurfaceTree::followHome(const ProjectId id)
    {
        if (ProjectRows* rows = rowsOf(id))
        {
            rows->node->header().setExpanded(true);
            setCurrentItem(rows->node->header());
            rows->node->header().scrollIntoViewOnAlign();
        }
        else
        {
            setCurrentItem(nullptr);
        }
    }

    OpenRows SurfaceTree::openRows(const ProjectId id) const
    {
        OpenRows open;
        const ProjectRows* rows = rowsOf(id);
        if (!rows)
            return open;
        open.home = rows->node->header().expanded();
        const ContentsRows& contents = rows->contents->rows();
        for (const ItemRows& item : rows->items)
        {
            if (item.node)
            {
                if (!item.node->header().expanded())
                    continue;
                open.items.push_back(item.path);
            }
            for (std::size_t i = 0; i != item.made.size(); ++i)
            {
                TreeNode* node = item.made[i].node;
                if (!node || !node->header().expanded())
                    continue;
                if (contents[i].kind == ContentsRowKind::Folder)
                    open.folders.push_back(joinPath(item.path, contents[i].folder));
                else
                {
                    open.modules.push_back(joinPath(item.path,
                        moduleLink(contents[i].folder, contents[i].module->name)));
                }
            }
        }
        return open;
    }

    // A node open under a closed one is made and opened all the same, so the tree stands as it
    // was left when the closed one opens again.
    void SurfaceTree::restoreRows(const ProjectId id, const OpenRows& open)
    {
        ProjectRows* rows = rowsOf(id);
        if (!rows)
            return;
        m_restoring = true;
        for (const std::wstring& path : open.items)
        {
            const std::optional<std::size_t> index = itemOf(*rows, path);
            if (!index.has_value())
                continue;
            ensureItemRow(*rows, *index);
            if (TreeNode* node = rows->items[*index].node)
                node->header().setExpanded(true);
        }
        const auto openContents = [this, rows](const std::wstring& key, const bool isFolder) {
            for (std::size_t i = 0; i != rows->items.size(); ++i)
            {
                const ItemRows& item = rows->items[i];
                if (item.item->kind != SidebarKind::AllTheApis)
                    continue;
                const std::optional<std::wstring_view> rest = underItem(key, item.path);
                if (!rest.has_value())
                    continue;
                const std::optional<std::size_t> contents = isFolder
                    ? rows->contents->folderRow(*rest)
                    : rows->contents->moduleRow(*rest);
                if (!contents.has_value())
                    return;
                ensureContentsRow(*rows, i, *contents);
                if (TreeNode* node = rows->items[i].made[*contents].node)
                    node->header().setExpanded(true);
                return;
            }
        };
        for (const std::wstring& key : open.folders)
            openContents(key, true);
        for (const std::wstring& key : open.modules)
            openContents(key, false);
        if (open.home)
            fillProject(*rows);
        rows->node->header().setExpanded(open.home);
        m_restoring = false;
    }

    // A command taken from the menu runs before the menu returns, so the row is held for as long
    // as anything can ask about it.
    void SurfaceTree::nestedContextPopup(ContextPopupEvent& event)
    {
        m_menuRow = menuRowOf(*event.control);
        TreeView::nestedContextPopup(event);
        m_menuRow.reset();
    }

    void SurfaceTree::adjustContextPopup(AdjustContextPopupEvent& event)
    {
        if (m_menuRow.has_value())
        {
            ActionList leading = { &Browser::Actions::openInNewTab, nullptr };
            if (m_menuRow->kind != RowKind::Contents)
                leading.insert(leading.end(), { &Actions::editSidebar, nullptr });
            event.actions.insert(event.actions.begin(), leading.begin(), leading.end());
        }
        TreeView::adjustContextPopup(event);
    }

    // Claimed only while a menu is up on a row, so the click always finds the row held, and
    // anywhere else the browser answers the command for its page.
    void SurfaceTree::connectActions()
    {
        onGetActionState([this](GetActionStateEvent& event) {
            if (!m_menuRow.has_value())
                return;
            if (&event.action == &Browser::Actions::openInNewTab)
                event.claim({});
            else if (&event.action == &Actions::editSidebar && m_menuRow->kind != RowKind::Contents)
                event.claim({});
        });
        onActionClick([this](ActionClickEvent& event) {
            if (!m_menuRow.has_value())
                return;
            const MenuRow& row = *m_menuRow;
            if (&event.action == &Actions::editSidebar)
            {
                editSidebar(row);
                return;
            }
            if (&event.action != &Browser::Actions::openInNewTab)
                return;
            switch (row.kind)
            {
                case RowKind::Home:
                    pickHome(row.rows->id, PickTab::New);
                    break;
                case RowKind::Item:
                    pickItem(*row.rows, row.item, PickTab::New);
                    break;
                case RowKind::Contents:
                    pickContents(*row.rows, row.item, row.contents, PickTab::New);
                    break;
            }
        });
    }

    SurfaceTree::ProjectRows* SurfaceTree::rowsOf(const ProjectId id) const
    {
        for (const std::unique_ptr<ProjectRows>& rows : m_projects)
        {
            if (rows->id == id)
                return rows.get();
        }
        return nullptr;
    }

    SurfaceTree::ItemsRows SurfaceTree::flattenItems(const SidebarItems& sidebar)
    {
        ItemsRows result;
        const auto add = [&result](const auto& self, const SidebarItems& items,
            const std::optional<std::size_t> parent, const std::wstring_view path) -> void {
            for (const SidebarItem& item : items)
            {
                const std::size_t index = result.size();
                // Held here, since the rows added under it move the vector's strings.
                const std::wstring itemPath = joinPath(path, item.name);
                result.push_back({
                    .item = &item,
                    .path = itemPath,
                    .parent = parent
                });
                if (item.kind == SidebarKind::Category)
                    self(self, item.items, index, itemPath);
            }
        };
        add(add, sidebar, std::nullopt, {});
        return result;
    }

    std::optional<std::size_t> SurfaceTree::itemOf(const ProjectRows& rows,
        const std::wstring_view path)
    {
        if (path.empty())
            return std::nullopt;
        for (std::size_t i = 0; i != rows.items.size(); ++i)
        {
            if (rows.items[i].path == path && hasOwnPage(*rows.items[i].item))
                return i;
        }
        return std::nullopt;
    }

    std::wstring_view SurfaceTree::foldersPathOf(const ProjectRows& rows, const std::size_t item)
    {
        const ItemRows& itemRows = rows.items[item];
        if (itemRows.item->subgroup)
            return itemRows.path;
        if (itemRows.parent.has_value())
            return rows.items[*itemRows.parent].path;
        return {};
    }

    std::optional<std::size_t> SurfaceTree::foldersAt(const ProjectRows& rows,
        const std::wstring_view path)
    {
        for (std::size_t i = 0; i != rows.items.size(); ++i)
        {
            if (rows.items[i].item->kind == SidebarKind::AllTheApis
                && foldersPathOf(rows, i) == path)
            {
                return i;
            }
        }
        return std::nullopt;
    }

    // The items stand in reading order, so an item's place in its list is how many stand in it
    // ahead of the item.
    IndexPath SurfaceTree::indexPathOf(const ProjectRows& rows, const std::size_t item)
    {
        IndexPath path;
        for (std::optional<std::size_t> at = item; at.has_value();
            at = rows.items[*at].parent)
        {
            std::size_t place = 0;
            for (std::size_t i = 0; i != *at; ++i)
            {
                if (rows.items[i].parent == rows.items[*at].parent)
                    ++place;
            }
            path.insert(path.begin(), place);
        }
        return path;
    }

    void SurfaceTree::fillProject(ProjectRows& rows)
    {
        if (rows.filled)
            return;
        rows.filled = true;
        for (std::size_t i = 0; i != rows.items.size(); ++i)
        {
            if (!rows.items[i].parent.has_value())
                makeItemRow(rows, i, *rows.node);
        }
    }

    void SurfaceTree::fillItem(ProjectRows& rows, const std::size_t item)
    {
        ItemRows& itemRows = rows.items[item];
        if (itemRows.filled || !itemRows.node)
            return;
        itemRows.filled = true;
        if (itemRows.item->kind == SidebarKind::Category)
        {
            for (std::size_t i = 0; i != rows.items.size(); ++i)
            {
                if (rows.items[i].parent == item)
                    makeItemRow(rows, i, *itemRows.node);
            }
            return;
        }
        itemRows.made.assign(rows.contents->rows().size(), MadeRow{});
        for (const std::size_t index : rows.contents->top())
            makeContentsRow(rows, item, index, *itemRows.node);
    }

    void SurfaceTree::fillContentsRow(ProjectRows& rows, const std::size_t item,
        const std::size_t contents)
    {
        MadeRow& made = rows.items[item].made[contents];
        if (made.filled || !made.node)
            return;
        made.filled = true;
        for (const std::size_t child : rows.contents->rows()[contents].children)
            makeContentsRow(rows, item, child, *made.node);
    }

    // A node is made closed: its rows are made as it opens. A divider is a line, which no pick
    // lands on. An All the APIs item without a subgroup makes the folders and the root's modules
    // in its place at once, as the rows of its holder.
    void SurfaceTree::makeItemRow(ProjectRows& rows, const std::size_t item, TreeNode& holder)
    {
        ItemRows& itemRows = rows.items[item];
        const SidebarItem& sidebarItem = *itemRows.item;
        ProjectRows* holderRows = &rows;
        const auto picked = [this, holderRows, item](ClickEvent&) {
            pickItem(*holderRows, item, PickTab::Shown);
        };
        switch (sidebarItem.kind)
        {
            case SidebarKind::Divider:
                itemRows.line = &holder.addDivider();
                return;
            case SidebarKind::Content:
                itemRows.row = &holder.addItem(itemText(sidebarItem));
                itemRows.row->onClick(picked);
                return;
            case SidebarKind::AllTheApis:
                if (sidebarItem.subgroup)
                    break;
                itemRows.filled = true;
                itemRows.made.assign(rows.contents->rows().size(), MadeRow{});
                for (const std::size_t index : rows.contents->top())
                {
                    makeContentsRow(rows, item, index, holder);
                    const MadeRow& made = itemRows.made[index];
                    itemRows.placed.push_back(made.node
                        ? static_cast<Control*>(made.node)
                        : static_cast<Control*>(made.item));
                }
                return;
            case SidebarKind::Category:
                break;
        }
        HeaderText header;
        header << itemText(sidebarItem);
        itemRows.node = &holder.addNode(std::move(header));
        itemRows.node->header().setExpanded(false);
        itemRows.node->header().onClick(picked);
        connectNode(rows, *itemRows.node, { .item = item });
    }

    void SurfaceTree::makeContentsRow(ProjectRows& rows, const std::size_t item,
        const std::size_t contents, TreeNode& holder)
    {
        const ContentsRow& row = rows.contents->rows()[contents];
        MadeRow& made = rows.items[item].made[contents];
        if (row.kind == ContentsRowKind::Folder)
        {
            made.node = &holder.addNode(folderText(row.folder));
        }
        else if (row.kind == ContentsRowKind::Type)
        {
            made.item = &holder.addItem(rowText(*row.type));
        }
        else if (row.children.empty())
        {
            made.item = &holder.addItem(
                Text{ rowIcon(RowIcon::Module), Space{ k_iconGap }, row.module->shownName });
        }
        else
        {
            made.node = &holder.addNode(
                HeaderText{ rowIcon(RowIcon::Module), Space{ k_iconGap }, row.module->shownName });
        }

        ProjectRows* holderRows = &rows;
        const auto picked = [this, holderRows, item, contents](ClickEvent&) {
            pickContents(*holderRows, item, contents, PickTab::Shown);
        };
        if (made.item)
        {
            made.item->onClick(picked);
            return;
        }
        made.node->header().setExpanded(false);
        made.node->header().onClick(picked);
        connectNode(rows, *made.node, { .item = item, .contents = contents });
    }

    void SurfaceTree::ensureItemRow(ProjectRows& rows, const std::size_t item)
    {
        const ItemRows& itemRows = rows.items[item];
        if (itemRows.node || itemRows.row || itemRows.line || itemRows.filled)
            return;
        if (!itemRows.parent.has_value())
        {
            fillProject(rows);
            return;
        }
        ensureItemRow(rows, *itemRows.parent);
        fillItem(rows, *itemRows.parent);
    }

    void SurfaceTree::ensureContentsRow(ProjectRows& rows, const std::size_t item,
        const std::size_t contents)
    {
        ensureItemRow(rows, item);
        fillItem(rows, item);
        const MadeRow& made = rows.items[item].made[contents];
        if (made.node || made.item)
            return;
        const std::optional<std::size_t> parent = rows.contents->rows()[contents].parent;
        if (!parent.has_value())
            return;
        ensureContentsRow(rows, item, *parent);
        fillContentsRow(rows, item, *parent);
    }

    // The current row is let go before any row is deleted where it is one of them, so it never
    // names a dead row; a node takes everything under it down with it.
    void SurfaceTree::clearRows(ProjectRows& rows)
    {
        const Control* current = currentItem();
        if (current && rows.node->rows().containsNested(current))
            setCurrentItem(nullptr);
        for (const ItemRows& item : rows.items)
        {
            if (item.parent.has_value())
                continue;
            if (item.node)
                item.node->deleteSelf();
            if (item.row)
                item.row->deleteSelf();
            if (item.line)
                item.line->deleteSelf();
            for (Control* placed : item.placed)
                placed->deleteSelf();
        }
        rows.filled = false;
        rows.items.clear();
    }

    // Connected after the node is made closed, so only the user's opening and closing - and a
    // follow's - is told.
    void SurfaceTree::connectNode(ProjectRows& rows, TreeNode& node, const Fetch fetch)
    {
        ProjectRows* holder = &rows;
        node.onFetchRows([this, holder, fetch](FetchRowsEvent&) {
            if (fetch.contents.has_value())
                fillContentsRow(*holder, *fetch.item, *fetch.contents);
            else if (fetch.item.has_value())
                fillItem(*holder, *fetch.item);
            else
                fillProject(*holder);
        });
        node.header().connectEvent<ToggleExpandedEvent>([this, holder](ToggleExpandedEvent&) {
            if (!m_restoring)
                emitEvent<TreeToggleEvent>(holder->id);
        });
    }

    void SurfaceTree::pickItem(const ProjectRows& rows, const std::size_t item, const PickTab tab)
    {
        TreePickEvent event{ rows.id, RowKind::Item, rows.items[item].path, {}, nullptr, nullptr,
            tab };
        emitEvent(event);
    }

    void SurfaceTree::pickContents(const ProjectRows& rows, const std::size_t item,
        const std::size_t contents, const PickTab tab)
    {
        const ContentsRow& row = rows.contents->rows()[contents];
        TreePickEvent event{ rows.id, RowKind::Contents, foldersPathOf(rows, item), row.folder,
            row.module, row.type, tab };
        emitEvent(event);
    }

    void SurfaceTree::pickHome(const ProjectId id, const PickTab tab)
    {
        TreePickEvent event{ id, RowKind::Home, {}, {}, nullptr, nullptr, tab };
        emitEvent(event);
    }

    void SurfaceTree::editSidebar(const MenuRow& row)
    {
        IndexPath item;
        if (row.kind == RowKind::Item)
            item = indexPathOf(*row.rows, row.item);
        TreeEditSidebarEvent event{ row.rows->id, std::move(item) };
        emitEvent(event);
    }

    // A node's header holds its mark and its text but none of the rows under it, so one row at
    // most holds the control.
    std::optional<SurfaceTree::MenuRow> SurfaceTree::menuRowOf(const Control& control) const
    {
        for (const std::unique_ptr<ProjectRows>& rows : m_projects)
        {
            if (rows->node->header().containsNested(control))
                return MenuRow{ .rows = rows.get(), .kind = RowKind::Home };
            for (std::size_t i = 0; i != rows->items.size(); ++i)
            {
                const ItemRows& item = rows->items[i];
                const Control* row = rowOf(item);
                if (row && row->containsNested(control))
                    return MenuRow{ .rows = rows.get(), .kind = RowKind::Item, .item = i };
                for (std::size_t j = 0; j != item.made.size(); ++j)
                {
                    const MadeRow& made = item.made[j];
                    if ((made.item || made.node) && rowOf(made).containsNested(control))
                    {
                        return MenuRow{
                            .rows = rows.get(),
                            .kind = RowKind::Contents,
                            .item = i,
                            .contents = j
                        };
                    }
                }
            }
        }
        return std::nullopt;
    }

    // Every category over the item is opened, and the project's node over them.
    void SurfaceTree::showRow(ProjectRows& rows, const std::size_t item, Control& row)
    {
        std::optional<std::size_t> at = rows.items[item].parent;
        while (at.has_value())
        {
            rows.items[*at].node->header().setExpanded(true);
            at = rows.items[*at].parent;
        }
        rows.node->header().setExpanded(true);
        setCurrentItem(row);
        row.scrollIntoViewOnAlign();
    }

    bool SurfaceTree::holdsCurrentRow(const ProjectRows& rows) const
    {
        const Control* current = currentItem();
        if (!current)
            return false;
        return current == &rows.node->header() || rows.node->rows().containsNested(current);
    }

    // The row an item is picked by: a page's is its row, a node's is its header; a divider has
    // none.
    Control* SurfaceTree::rowOf(const ItemRows& item)
    {
        if (item.row)
            return item.row;
        if (item.node)
            return &item.node->header();
        return nullptr;
    }

    // The row a made row is picked by: an item's is the item, a node's is its header.
    Control& SurfaceTree::rowOf(const MadeRow& made)
    {
        if (made.item)
            return *made.item;
        return made.node->header();
    }

    HeaderText SurfaceTree::projectText(const std::wstring_view name)
    {
        return HeaderText{ rowIcon(RowIcon::Project), Space{ k_iconGap }, name };
    }

    HeaderText SurfaceTree::folderText(const std::wstring_view folder)
    {
        return HeaderText{ rowIcon(RowIcon::Folder), Space{ k_iconGap }, folderNameOf(folder) };
    }

    // A row leads with the mark of its kind and ends with its kind word.
    Text SurfaceTree::rowText(const Type& type)
    {
        Text text{ rowIcon(rowIconOf(type)), Space{ k_iconGap }, type.name };
        writeRowKind(text, type);
        return text;
    }

    Text SurfaceTree::itemText(const SidebarItem& item)
    {
        Text text;
        if (const std::optional<RowIcon> icon = rowIconOf(item.kind))
            text << rowIcon(*icon) << Space{ k_iconGap };
        text << item.name;
        return text;
    }
}
