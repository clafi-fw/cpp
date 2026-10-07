module SeeDocs_App.Studio.SurfaceView;

import SeeDocs_App.Studio.Icons;
import SeeDocs_App.Studio.PageView;
import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.TreeView;
import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    bool SurfaceView::bind(const Surface& surface, Notes& notes)
    {
        clearTree();
        m_surface = &surface;
        m_notes = &notes;
        m_contents = contentsOf(surface);
        buildTree();
        if (m_entries.empty())
            return false;
        ExpanderHeader& first = m_entries.front().node->header();
        first.setExpanded(true);
        m_tree.setCurrentItem(first);
        return true;
    }

    bool SurfaceView::showNamed(const std::wstring_view name)
    {
        const auto found = m_entryByName.find(std::wstring{ name });
        if (found == m_entryByName.end())
            return false;
        m_pending = found->second;
        m_showTimer.start(MilliSeconds{ 0u });
        return true;
    }

    void SurfaceView::showText(const Text& text)
    {
        clearTree();
        m_surface = nullptr;
        m_notes = nullptr;
        m_contents.clear();
        m_page.showText(text);
    }

    // The current item is let go before any row is deleted, so the pick never names a dead row;
    // a chapter's node takes its modules and their types down with it.
    void SurfaceView::clearTree()
    {
        m_pending = k_root;
        m_tree.setCurrentItem(nullptr);
        for (const Entry& entry : m_entries)
        {
            if (entry.parent == k_root)
                entry.node->deleteSelf();
        }
        m_entries.clear();
        m_entryByName.clear();
    }

    void SurfaceView::showPending()
    {
        const std::size_t index = std::exchange(m_pending, k_root);
        if (index == k_root)
            return;
        const Entry& entry = m_entries[index];

        // Opened from the chapter down, so the row stands in the tree before it is picked. A
        // node named is opened as well, so what it holds stands under it.
        for (std::size_t at = entry.parent; at != k_root; at = m_entries[at].parent)
            m_entries[at].node->header().setExpanded(true);
        if (entry.node)
            entry.node->header().setExpanded(true);

        // Picking the row shows the page, except where the row is picked already.
        Control& row = rowOf(entry);
        if (m_tree.currentItem() == &row)
        {
            show(index);
            row.scrollIntoViewOnAlign();
        }
        else
        {
            m_tree.setCurrentItem(row);
        }
    }

    void SurfaceView::buildTree()
    {
        for (const ContentsChapter& chapter : m_contents)
        {
            TreeNode& chapterNode = m_tree.addNode(
                HeaderText{ rowIcon(RowIcon::Chapter), Space{ k_iconGap }, chapter.name });
            const std::size_t chapterIndex = m_entries.size();
            chapterNode.header().setTag(Tag{ chapterIndex });
            m_entries.push_back({ .chapter = &chapter, .node = &chapterNode });
            for (const ContentsModule& module : chapter.modules)
            {
                TreeNode& moduleNode = chapterNode.addNode(
                    HeaderText{ rowIcon(RowIcon::Module), Space{ k_iconGap }, module.shortName });
                const std::size_t moduleIndex = m_entries.size();
                moduleNode.header().setTag(Tag{ moduleIndex });
                m_entries.push_back({
                    .chapter = &chapter,
                    .module = &module,
                    .node = &moduleNode,
                    .parent = chapterIndex
                });
                m_entryByName[module.name] = moduleIndex;
                for (const Type* type : module.types)
                {
                    TreeItem& item = moduleNode.addItem(rowText(*type));
                    item.setTag(Tag{ m_entries.size() });
                    m_entries.push_back({
                        .chapter = &chapter,
                        .module = &module,
                        .type = type,
                        .item = &item,
                        .parent = moduleIndex
                    });
                    m_entryByName[type->qualifiedName] = m_entries.size() - 1;
                }
            }
        }
    }

    void SurfaceView::show(const std::size_t index)
    {
        const Entry& entry = m_entries[index];
        Page page;
        if (entry.type)
            page = typePage(*m_surface, *m_notes, *entry.type);
        else if (entry.module)
            page = modulePage(*m_surface, *m_notes, *entry.chapter, *entry.module);
        else
            page = chapterPage(*m_surface, *m_notes, *entry.chapter);
        m_page.show(std::move(page));
    }

    // The row an entry is picked by: an item's is the item, a node's is its header.
    Control& SurfaceView::rowOf(const Entry& entry)
    {
        if (entry.item)
            return *entry.item;
        return entry.node->header();
    }

    // A row leads with the mark of its kind and ends with its kind word.
    Text SurfaceView::rowText(const Type& type)
    {
        Text text{ rowIcon(rowIconOf(type)), Space{ k_iconGap }, type.name };
        writeRowKind(text, type);
        return text;
    }
}
