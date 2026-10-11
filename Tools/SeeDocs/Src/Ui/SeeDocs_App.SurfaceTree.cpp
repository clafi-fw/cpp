module SeeDocs_App.SurfaceTree;

import SeeDocs_App.Icons;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Tree;
import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // TreePickEvent

    TreePickEvent::TreePickEvent(const ProjectId pickedProject, const RowKind pickedKind,
        const std::wstring_view pickedFolder, const ContentsModule* pickedModule,
        const Type* pickedType)
        :
        ::ClaFi::Event{},
        project{ pickedProject },
        kind{ pickedKind },
        folder{ pickedFolder },
        module{ pickedModule },
        type{ pickedType }
    {
    }

    // TreeToggleEvent

    TreeToggleEvent::TreeToggleEvent(const ProjectId toggledProject)
        :
        ::ClaFi::Event{},
        project{ toggledProject }
    {
    }

    // SurfaceTree

    // **A row is picked by a click, not by becoming current**: the arrow keys walk the rows without
    // opening a page for each one passed, and Enter opens the row they stop on.
    void SurfaceTree::addProject(const ProjectId id, const std::wstring_view name,
        const ContentsTree& contents)
    {
        std::unique_ptr<ProjectRows> made = std::make_unique<ProjectRows>();
        ProjectRows& rows = *made;
        rows.id = id;
        rows.contents = &contents;
        rows.made.resize(contents.rows().size());
        if (!m_projects.empty())
            rows.above = &addDivider(Thickness::Heavy);
        rows.node = &addNode(projectText(name));
        rows.node->header().setExpanded(false);
        rows.node->header().onClick([this, id](ClickEvent&) {
            pickRow(id, RowKind::Home);
        });
        m_projects.push_back(std::move(made));
        connectNode(rows, *rows.node, std::nullopt);
    }

    // **The rows standing were made over the contents before**, which the project has let go of by
    // now, so they are taken down by what was made and not by what the contents say.
    void SurfaceTree::rebuildProject(const ProjectId id, const ContentsTree& contents,
        const OpenRows& open)
    {
        ProjectRows* rows = rowsOf(id);
        if (!rows)
            return;
        clearRows(*rows);
        rows->contents = &contents;
        rows->made.assign(contents.rows().size(), MadeRow{});
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

    // Opened from the row up, so the row stands in the tree before it is made current.
    void SurfaceTree::follow(const ProjectId id, const std::wstring_view folder,
        const ContentsModule* module, const Type* type)
    {
        ProjectRows* rows = rowsOf(id);
        const std::optional<std::size_t> index = rows
            ? rows->contents->rowOf(folder, module, type)
            : std::nullopt;
        if (!index.has_value())
        {
            setCurrentItem(nullptr);
            return;
        }

        ensureRow(*rows, *index);
        const ContentsRows& contents = rows->contents->rows();
        std::optional<std::size_t> at = contents[*index].parent;
        while (at.has_value())
        {
            rows->made[*at].node->header().setExpanded(true);
            at = contents[*at].parent;
        }
        showRow(*rows, rowOf(rows->made[*index]));
    }

    void SurfaceTree::followHome(const ProjectId id)
    {
        if (ProjectRows* rows = rowsOf(id))
            showRow(*rows, rows->node->header());
        else
            setCurrentItem(nullptr);
    }

    void SurfaceTree::followOptions(const ProjectId id)
    {
        ProjectRows* rows = rowsOf(id);
        if (!rows)
        {
            setCurrentItem(nullptr);
            return;
        }
        fillProject(*rows);
        showRow(*rows, *rows->options);
    }

    OpenRows SurfaceTree::openRows(const ProjectId id) const
    {
        OpenRows open;
        const ProjectRows* rows = rowsOf(id);
        if (!rows)
            return open;
        open.home = rows->node->header().expanded();
        const ContentsRows& contents = rows->contents->rows();
        for (std::size_t i = 0; i != rows->made.size(); ++i)
        {
            TreeNode* node = rows->made[i].node;
            if (!node || !node->header().expanded())
                continue;
            if (contents[i].kind == ContentsRowKind::Folder)
                open.folders.emplace_back(contents[i].folder);
            else
                open.modules.push_back(contents[i].module->name);
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
        const auto openRow = [this, rows](const std::optional<std::size_t> index) {
            if (!index.has_value())
                return;
            ensureRow(*rows, *index);
            if (TreeNode* node = rows->made[*index].node)
                node->header().setExpanded(true);
        };
        for (const std::wstring& folder : open.folders)
            openRow(rows->contents->folderRow(folder));
        for (const std::wstring& module : open.modules)
            openRow(rows->contents->moduleRow(module));
        if (open.home)
            fillProject(*rows);
        rows->node->header().setExpanded(open.home);
        m_restoring = false;
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

    // The line under the options row parts it from the contents, so it stands while there are
    // any.
    void SurfaceTree::fillProject(ProjectRows& rows)
    {
        if (rows.options)
            return;
        TreeNode& node = *rows.node;
        const ProjectId id = rows.id;
        rows.options = &node.addItem(
            Text{ rowIcon(RowIcon::Options), Space{ k_iconGap }, k_optionsTitle });
        rows.options->onClick([this, id](ClickEvent&) {
            pickRow(id, RowKind::Options);
        });
        rows.divider = &node.addDivider();
        rows.divider->setVisible(!rows.contents->empty());
        for (const std::size_t index : rows.contents->top())
        {
            makeRow(rows, index, node);
            const MadeRow& made = rows.made[index];
            if (made.node)
                rows.topRows.push_back(made.node);
            else
                rows.topRows.push_back(made.item);
        }
    }

    void SurfaceTree::fillRow(ProjectRows& rows, const std::size_t index)
    {
        MadeRow& made = rows.made[index];
        if (made.filled || !made.node)
            return;
        made.filled = true;
        for (const std::size_t child : rows.contents->rows()[index].children)
            makeRow(rows, child, *made.node);
    }

    // A node is made closed: its rows are made as it opens.
    void SurfaceTree::makeRow(ProjectRows& rows, const std::size_t index, TreeNode& holder)
    {
        const ContentsRow& row = rows.contents->rows()[index];
        MadeRow& made = rows.made[index];
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
                Text{ rowIcon(RowIcon::Module), Space{ k_iconGap }, row.module->shortName });
        }
        else
        {
            made.node = &holder.addNode(
                HeaderText{ rowIcon(RowIcon::Module), Space{ k_iconGap }, row.module->shortName });
        }

        ProjectRows* holderRows = &rows;
        const auto picked = [this, holderRows, index](ClickEvent&) {
            pick(*holderRows, index);
        };
        if (made.item)
        {
            made.item->onClick(picked);
            return;
        }
        made.node->header().setExpanded(false);
        made.node->header().onClick(picked);
        connectNode(rows, *made.node, index);
    }

    void SurfaceTree::ensureRow(ProjectRows& rows, const std::size_t index)
    {
        const MadeRow& made = rows.made[index];
        if (made.node || made.item)
            return;
        const std::optional<std::size_t> parent = rows.contents->rows()[index].parent;
        if (!parent.has_value())
        {
            fillProject(rows);
            return;
        }
        ensureRow(rows, *parent);
        fillRow(rows, *parent);
    }

    // The current row is let go before any row is deleted where it is one of them, so it never
    // names a dead row; a node takes everything under it down with it.
    void SurfaceTree::clearRows(ProjectRows& rows)
    {
        const Control* current = currentItem();
        if (current && rows.node->rows().containsNested(current))
            setCurrentItem(nullptr);
        for (Control* row : rows.topRows)
            row->deleteSelf();
        rows.topRows.clear();
        if (rows.options)
            rows.options->deleteSelf();
        if (rows.divider)
            rows.divider->deleteSelf();
        rows.options = nullptr;
        rows.divider = nullptr;
        rows.made.clear();
    }

    // Connected after the node is made closed, so only the user's opening and closing - and a
    // follow's - is told.
    void SurfaceTree::connectNode(ProjectRows& rows, TreeNode& node,
        const std::optional<std::size_t> index)
    {
        ProjectRows* holder = &rows;
        node.onFetchRows([this, holder, index](FetchRowsEvent&) {
            if (index.has_value())
                fillRow(*holder, *index);
            else
                fillProject(*holder);
        });
        node.header().connectEvent<ToggleExpandedEvent>([this, holder](ToggleExpandedEvent&) {
            if (!m_restoring)
                emitEvent<TreeToggleEvent>(holder->id);
        });
    }

    void SurfaceTree::pick(const ProjectRows& rows, const std::size_t index)
    {
        const ContentsRow& row = rows.contents->rows()[index];
        TreePickEvent event{ rows.id, RowKind::Contents, row.folder, row.module, row.type };
        emitEvent(event);
    }

    void SurfaceTree::pickRow(const ProjectId id, const RowKind kind)
    {
        TreePickEvent event{ id, kind, {}, nullptr, nullptr };
        emitEvent(event);
    }

    void SurfaceTree::showRow(ProjectRows& rows, Control& row)
    {
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
}
