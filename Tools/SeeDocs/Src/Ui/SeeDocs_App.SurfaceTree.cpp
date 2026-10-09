module SeeDocs_App.SurfaceTree;

import SeeDocs_App.Icons;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.TreeView;
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

    TreePickEvent::TreePickEvent(const std::wstring_view pickedFolder,
        const ContentsModule* pickedModule, const Type* pickedType)
        :
        ::ClaFi::Event{},
        folder{ pickedFolder },
        module{ pickedModule },
        type{ pickedType }
    {
    }

    // SurfaceTree

    // A ROW IS PICKED BY A CLICK, NOT BY BECOMING CURRENT: the arrow keys walk the rows without
    // opening a page for each one passed, and Enter opens the row they stop on. Every folder is
    // made before any module, so a folder's subfolders stand above its modules - the tree's
    // own folder is home, and its modules stand at the top level under its folders.
    void SurfaceTree::build(const ContentsChapters& contents)
    {
        clear();
        for (const ContentsChapter& chapter : contents)
        {
            if (!chapter.category.empty())
                addFolder(chapter.category);
        }
        for (const ContentsChapter& chapter : contents)
            addModules(chapter);
        collapseAll();
    }

    std::size_t SurfaceTree::addFolder(const std::wstring_view category)
    {
        const FolderIndexes::const_iterator found = m_folderOf.find(std::wstring{ category });
        if (found != m_folderOf.end())
            return found->second;
        const std::size_t slash = category.rfind(k_folderSeparator);
        const std::size_t parent = slash == std::wstring_view::npos
            ? k_root
            : addFolder(category.substr(0, slash));
        TreeNode& node = parent == k_root
            ? addNode(folderText(category))
            : m_entries[parent].node->addNode(folderText(category));
        const std::size_t index = m_entries.size();
        node.header().onClick([this, index](ClickEvent&) {
            pick(index);
        });
        const FolderIndexes::const_iterator added =
            m_folderOf.emplace(std::wstring{ category }, index).first;
        m_entries.push_back({ .folder = added->first, .node = &node, .parent = parent });
        return index;
    }

    void SurfaceTree::addModules(const ContentsChapter& chapter)
    {
        const bool atRoot = chapter.category.empty();
        const std::size_t folderIndex = atRoot ? k_root : m_folderOf.at(chapter.category);
        TreeNode* folderNode = atRoot ? nullptr : m_entries[folderIndex].node;
        for (const ContentsModule& module : chapter.modules)
        {
            const std::size_t moduleIndex = m_entries.size();
            if (module.types.empty())
            {
                const Text text{ rowIcon(RowIcon::Module), Space{ k_iconGap }, module.shortName };
                TreeItem& item = folderNode ? folderNode->addItem(text) : addItem(text);
                item.onClick([this, moduleIndex](ClickEvent&) {
                    pick(moduleIndex);
                });
                m_entries.push_back({
                    .folder = chapter.category,
                    .module = &module,
                    .item = &item,
                    .parent = folderIndex
                });
                m_entryOf[&module] = moduleIndex;
                continue;
            }
            const HeaderText header{
                rowIcon(RowIcon::Module),
                Space{ k_iconGap },
                module.shortName
            };
            TreeNode& moduleNode = folderNode ? folderNode->addNode(header) : addNode(header);
            moduleNode.header().onClick([this, moduleIndex](ClickEvent&) {
                pick(moduleIndex);
            });
            m_entries.push_back({
                .folder = chapter.category,
                .module = &module,
                .node = &moduleNode,
                .parent = folderIndex
            });
            m_entryOf[&module] = moduleIndex;
            for (const Type* type : module.types)
            {
                TreeItem& item = moduleNode.addItem(rowText(*type));
                const std::size_t typeIndex = m_entries.size();
                item.onClick([this, typeIndex](ClickEvent&) {
                    pick(typeIndex);
                });
                m_entries.push_back({
                    .folder = chapter.category,
                    .module = &module,
                    .type = type,
                    .item = &item,
                    .parent = moduleIndex
                });
                m_entryOf[type] = typeIndex;
            }
        }
    }

    // The current row is let go before any row is deleted, so it never names a dead row; a
    // folder's node takes everything under it down with it.
    void SurfaceTree::clear()
    {
        setCurrentItem(nullptr);
        for (const Entry& entry : m_entries)
        {
            if (entry.parent != k_root)
                continue;
            if (entry.node)
                entry.node->deleteSelf();
            else
                entry.item->deleteSelf();
        }
        m_entries.clear();
        m_entryOf.clear();
        m_folderOf.clear();
    }

    void SurfaceTree::follow(const std::wstring_view folder, const ContentsModule* module,
        const Type* type)
    {
        const std::optional<std::size_t> index = entryOf(folder, module, type);
        if (!index.has_value())
        {
            setCurrentItem(nullptr);
            return;
        }

        const Entry& entry = m_entries[*index];
        // Opened from the row up, so the row stands in the tree before it is made current.
        for (std::size_t at = entry.parent; at != k_root; at = m_entries[at].parent)
            m_entries[at].node->header().setExpanded(true);
        Control& row = rowOf(entry);
        setCurrentItem(row);
        row.scrollIntoViewOnAlign();
    }

    // The nearest one named: the type, else the module, else the folder - home's empty one names
    // no row.
    std::optional<std::size_t> SurfaceTree::entryOf(const std::wstring_view folder,
        const ContentsModule* module, const Type* type) const
    {
        const void* named = type;
        if (!named)
            named = module;
        if (named)
        {
            const EntryIndexes::const_iterator found = m_entryOf.find(named);
            if (found == m_entryOf.end())
                return std::nullopt;
            return found->second;
        }
        if (folder.empty())
            return std::nullopt;
        const FolderIndexes::const_iterator found = m_folderOf.find(std::wstring{ folder });
        if (found == m_folderOf.end())
            return std::nullopt;
        return found->second;
    }

    void SurfaceTree::pick(const std::size_t index)
    {
        const Entry& entry = m_entries[index];
        TreePickEvent event{ entry.folder, entry.module, entry.type };
        emitEvent(event);
    }

    // The row an entry is picked by: an item's is the item, a node's is its header.
    Control& SurfaceTree::rowOf(const Entry& entry)
    {
        if (entry.item)
            return *entry.item;
        return entry.node->header();
    }

    HeaderText SurfaceTree::folderText(const std::wstring_view category)
    {
        return HeaderText{ rowIcon(RowIcon::Chapter), Space{ k_iconGap }, folderNameOf(category) };
    }

    // A row leads with the mark of its kind and ends with its kind word.
    Text SurfaceTree::rowText(const Type& type)
    {
        Text text{ rowIcon(rowIconOf(type)), Space{ k_iconGap }, type.name };
        writeRowKind(text, type);
        return text;
    }
}
