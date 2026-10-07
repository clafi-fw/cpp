module SeeDocs_App.Studio.SurfaceView;

import SeeDocs_App.Studio.Icons;
import SeeDocs_App.Studio.PageText;
import SeeDocs_App.Database;
import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.TextBox;
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

    void SurfaceView::bind(const Surface& surface, Notes& notes)
    {
        m_surface = &surface;
        m_notes = &notes;
        m_contents = contentsOf(surface);
        buildTree();
        if (m_entries.empty())
            return;
        ExpanderHeader& first = m_entries.front().node->header();
        first.setExpanded(true);
        m_tree.setCurrentItem(first);
    }

    bool SurfaceView::showNamed(const std::wstring_view name)
    {
        const auto found = m_entryByName.find(std::wstring{ name });
        if (found == m_entryByName.end())
            return false;
        const std::size_t index = found->second;
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
        return true;
    }

    void SurfaceView::showText(const Text& text)
    {
        TextBox& body = m_page.body();
        body.text() = text;
        body.setCaretPos(0);
        // The page is what the box measures, so a new one is a new size for the scroll box
        // around it.
        body.invalidateFormAlign();
        m_page.scrollToBegin();
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
        if (entry.type)
            showPage(typePage(*m_surface, *m_notes, *entry.type));
        else if (entry.module)
            showPage(modulePage(*m_surface, *m_notes, *entry.chapter, *entry.module));
        else
            showPage(chapterPage(*m_surface, *m_notes, *entry.chapter));
    }

    void SurfaceView::showPage(const Page& page)
    {
        showText(textOf(page));
    }

    // The row an entry is picked by: an item's is the item, a node's is its header.
    Control& SurfaceView::rowOf(const Entry& entry)
    {
        if (entry.item)
            return *entry.item;
        return entry.node->header();
    }

    // A row leads with the mark of its kind. A control's row is its name alone; any other
    // type's says what kind it is, at the row's far end.
    Text SurfaceView::rowText(const Type& type)
    {
        Text text{ rowIcon(rowIconOf(type)), Space{ k_iconGap }, type.name };
        if (!type.isControl)
        {
            text << FlexSpace{ k_kindGap } << TextStyleId::SubBody << InkGrade::Muted
                << kindWord(type.kind) << PopColor{} << PopTextStyle{};
        }
        return text;
    }
}
