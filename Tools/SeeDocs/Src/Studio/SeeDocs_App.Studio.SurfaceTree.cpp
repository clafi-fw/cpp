module SeeDocs_App.Studio.SurfaceTree;

import SeeDocs_App.Studio.Icons;
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

    TreePickEvent::TreePickEvent(const ContentsChapter& pickedChapter,
        const ContentsModule* pickedModule, const Type* pickedType)
        :
        ::ClaFi::Event{},
        chapter{ pickedChapter },
        module{ pickedModule },
        type{ pickedType }
    {
    }

    // SurfaceTree

    // A ROW IS PICKED BY A CLICK, NOT BY BECOMING CURRENT: the arrow keys walk the rows without
    // opening a page for each one passed, and Enter opens the row they stop on.
    void SurfaceTree::build(const ContentsChapters& contents)
    {
        clear();
        for (const ContentsChapter& chapter : contents)
        {
            TreeNode& chapterNode = addNode(
                HeaderText{ rowIcon(RowIcon::Chapter), Space{ k_iconGap }, chapter.name });
            const std::size_t chapterIndex = m_entries.size();
            chapterNode.header().onClick([this, chapterIndex](ClickEvent&) {
                pick(chapterIndex);
            });
            m_entries.push_back({ .chapter = &chapter, .node = &chapterNode });
            m_entryOf[&chapter] = chapterIndex;
            for (const ContentsModule& module : chapter.modules)
            {
                TreeNode& moduleNode = chapterNode.addNode(
                    HeaderText{ rowIcon(RowIcon::Module), Space{ k_iconGap }, module.shortName });
                const std::size_t moduleIndex = m_entries.size();
                moduleNode.header().onClick([this, moduleIndex](ClickEvent&) {
                    pick(moduleIndex);
                });
                m_entries.push_back({
                    .chapter = &chapter,
                    .module = &module,
                    .node = &moduleNode,
                    .parent = chapterIndex
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
                        .chapter = &chapter,
                        .module = &module,
                        .type = type,
                        .item = &item,
                        .parent = moduleIndex
                    });
                    m_entryOf[type] = typeIndex;
                }
            }
        }
        if (!m_entries.empty())
            m_entries.front().node->header().setExpanded(true);
    }

    // The current row is let go before any row is deleted, so it never names a dead row; a
    // chapter's node takes its modules and their types down with it.
    void SurfaceTree::clear()
    {
        setCurrentItem(nullptr);
        for (const Entry& entry : m_entries)
        {
            if (entry.parent == k_root)
                entry.node->deleteSelf();
        }
        m_entries.clear();
        m_entryOf.clear();
    }

    void SurfaceTree::follow(const ContentsChapter* chapter, const ContentsModule* module,
        const Type* type)
    {
        const void* named = type;
        if (!named)
            named = module;
        if (!named)
            named = chapter;
        const EntryIndexes::const_iterator found = m_entryOf.find(named);
        if (!named || found == m_entryOf.end())
        {
            setCurrentItem(nullptr);
            return;
        }

        const Entry& entry = m_entries[found->second];
        // Opened from the chapter down, so the row stands in the tree before it is made current.
        for (std::size_t at = entry.parent; at != k_root; at = m_entries[at].parent)
            m_entries[at].node->header().setExpanded(true);
        Control& row = rowOf(entry);
        setCurrentItem(row);
        row.scrollIntoViewOnAlign();
    }

    void SurfaceTree::pick(const std::size_t index)
    {
        const Entry& entry = m_entries[index];
        TreePickEvent event{ *entry.chapter, entry.module, entry.type };
        emitEvent(event);
    }

    // The row an entry is picked by: an item's is the item, a node's is its header.
    Control& SurfaceTree::rowOf(const Entry& entry)
    {
        if (entry.item)
            return *entry.item;
        return entry.node->header();
    }

    // A row leads with the mark of its kind and ends with its kind word.
    Text SurfaceTree::rowText(const Type& type)
    {
        Text text{ rowIcon(rowIconOf(type)), Space{ k_iconGap }, type.name };
        writeRowKind(text, type);
        return text;
    }
}
