module SeeDocs_App.Studio.PageView;

import SeeDocs_App.Studio.Icons;
import SeeDocs_App.Studio.PageText;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Expander;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.TreeView;
import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // OpenNameEvent

    OpenNameEvent::OpenNameEvent(std::wstring linkName)
        :
        ::ClaFi::Event{},
        name{ std::move(linkName) }
    {
    }

    // PageView

    void PageView::show(Page page)
    {
        clear();
        m_page = std::move(page);
        addHead();
        addLead();
        addFacts();
        for (const Section& section : m_page.sections)
        {
            Expander& expander = addSection(section);
            switch (section.kind)
            {
                case SectionKind::Note:
                    addProse(expander, section.excerpt);
                    break;
                case SectionKind::Tree:
                    addTree(expander, section.branches);
                    break;
                case SectionKind::Table:
                    addTable(expander, section.table);
                    break;
            }
        }
        invalidateFormAlign();
        m_pageBox.scrollToBegin();
    }

    void PageView::showText(const Text& text)
    {
        clear();
        m_page = {};
        m_head.text().clear();
        m_head.setVisible(false);
        TextBox& box = m_pageBox.body().add<TextBox>(ReadOnly::Yes);
        box.text() = text;
        m_parts.push_back(&box);
        invalidateFormAlign();
        m_pageBox.scrollToBegin();
    }

    void PageView::clear()
    {
        for (Control* part : m_parts)
            part->deleteSelf();
        m_parts.clear();
    }

    // A link in the head names a base to open.
    void PageView::connectHeadLinks()
    {
        m_head.onLinkClick([this](LinkClickEvent& event) {
            open(event.target);
        });
    }

    // The title with what it is beside it and the ways up through its bases under it, in the bar
    // that stays.
    void PageView::addHead()
    {
        Text head;
        head << TextStyleId::Title << m_page.title << PopTextStyle{};
        if (!m_page.badges.empty())
        {
            head << L"   " << TextStyleId::SubBody;
            writeRuns(head, m_page.badges);
            head << PopTextStyle{};
        }
        for (const Runs& chain : m_page.bases)
        {
            head << k_endLine << ParaIndent{ k_basesIndent };
            writeRuns(head, chain);
        }
        m_head.text() = head;
        m_head.setVisible(true);
    }

    void PageView::addLead()
    {
        if (m_page.lead.empty())
            return;
        Text lead;
        lead << TextStyleId::SubTitle;
        writeRuns(lead, m_page.lead);
        lead << PopTextStyle{};
        TextBox& box = m_pageBox.body().add<TextBox>(ReadOnly::Yes);
        box.text() = lead;
        m_parts.push_back(&box);
    }

    // The facts as a grid of two columns, the label muted.
    void PageView::addFacts()
    {
        if (m_page.facts.empty())
            return;
        Grids::Grid& grid = m_pageBox.body().add<Grids::Grid>(
            themeMetrics().page,
            UiElement::Section,
            Grids::GridLines::Horizontal
        );
        grid.columns().add(Tag{ k_factLabel }, TextFormat{ InkGrade::Muted });
        grid.columns().add(Tag{ k_factValue }, Grids::ColumnWidthMode::Fill);
        for (const Fact& fact : m_page.facts)
        {
            Grids::Row& row = grid.addRow();
            row.onGetCellText([&fact](Grids::GetCellTextEvent& event) {
                if (event.column().tag().value == k_factLabel)
                    event.text() << fact.label;
                else
                    writeRuns(event.text(), fact.value);
            });
        }
        connectLinks(grid, nullptr);
        m_parts.push_back(&grid);
    }

    // An expander headed by a labelled divider - the section's heading and count as the label -
    // holding its body without room round it.
    Expander& PageView::addSection(const Section& section)
    {
        HeaderText header;
        header << TextStyleId::Heading;
        writeRuns(header, section.heading);
        if (!section.count.empty())
            header << L"  " << InkGrade::Muted << section.count << PopColor{};
        header << PopTextStyle{};
        Expander& expander = m_pageBox.body().add<Expander>(
            VerticalAlign::Top,
            ExpanderViewMode::Divider,
            Padding{ 0.0f },
            Spacing{ k_headerSpacing },
            std::move(header)
        );
        m_parts.push_back(&expander);
        return expander;
    }

    void PageView::addProse(Expander& expander, const Excerpt& excerpt)
    {
        TextBox& box = expander.createBody<TextBox>(ReadOnly::Yes, Padding{ k_prosePadding });
        box.text() = textOf(excerpt.blocks);
    }

    void PageView::addTree(Expander& expander, const Branches& branches)
    {
        TreeView& tree = expander.createBody<TreeView>(
            Padding{ k_treePadding },
            HorizontalAlign::Left
        );
        addTreeRows(tree, branches);
    }

    // A row leads with the mark of the type's kind where it names one, and ends with its kind word.
    Text PageView::treeRowText(const Branch& branch) const
    {
        Text text;
        if (branch.type)
            text << rowIcon(rowIconOf(*branch.type)) << Space{ k_iconGap };
        writeRuns(text, branch.text);
        if (branch.type)
            writeRowKind(text, *branch.type);
        return text;
    }

    HeaderText PageView::treeHeaderText(const Branch& branch) const
    {
        HeaderText header;
        header << treeRowText(branch);
        return header;
    }

    // The grid: the group column where the table names one, its own columns with a mark column
    // after the first where any row has a footnote, a header where the table asks for one, and
    // the footnotes under it in a box of their own.
    void PageView::addTable(Expander& expander, const Table& table)
    {
        Stack& body = expander.createBody<Stack>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ 0.0f }
        );
        Grids::Grid& grid = body.add<Grids::Grid>(
            themeMetrics().page,
            UiElement::Section,
            Grids::GridLines::Horizontal
        );
        if (!table.groupColumn.empty())
            grid.columns().add(Tag{ k_groupColumn }, Text{ table.groupColumn });
        const bool footnoted = hasFootnotes(table);
        for (std::size_t i = 0; i != table.columns.size(); ++i)
        {
            const TableColumn& column = table.columns[i];
            grid.columns().add(
                Tag{ i },
                Text{ column.name },
                column.fills ? Grids::ColumnWidthMode::Fill : Grids::ColumnWidthMode::FitContent
            );
            if (i == 0 && footnoted)
                grid.columns().add(Tag{ k_markColumn }, Text{});
        }
        if (table.header)
            grid.addHeader();

        Footnotes footnotes;
        for (const TableGroup& group : table.groups)
            addTableRows(grid, table, group, footnotes);
        TextBox* box = nullptr;
        if (footnotes.count != 0)
        {
            box = &body.add<TextBox>(ReadOnly::Yes, Padding{ k_prosePadding });
            box->text() = footnotes.text;
        }
        connectLinks(grid, box);
    }

    // A labelled group stands beside its label in the group column, folding under it, or under
    // a held expander row of the label where the table has no such column.
    void PageView::addTableRows(Grids::GridBase& grid, const Table& table,
        const TableGroup& group, Footnotes& footnotes)
    {
        const bool beside = !table.groupColumn.empty();
        Grids::GridBase* target = &grid;
        if (!group.label.empty() && beside)
        {
            Grids::RowGroup& groupRow = grid.addGroup(Grids::Collapsible::Expanded);
            Grids::RowGroupSpan& span = groupRow.span();
            span.onCellContent([](Grids::CellContentEvent& event) {
                event.hasContent = event.column().tag().value == k_groupColumn;
                event.stopPropagation();
            });
            span.onGetCellText([&group](Grids::GetCellTextEvent& event) {
                event.text() << rowIcon(RowIcon::Module) << Space{ k_iconGap };
                writeRuns(event.text(), group.label);
            });
            if (!group.hint.empty())
            {
                span.onGetCellHint([&group](Grids::GetCellHintEvent& event) {
                    writeRuns(event.text(), group.hint);
                });
            }
            target = &groupRow.body();
        }
        else if (!group.label.empty())
        {
            Grids::RowExpander& expander = grid.addExpander();
            expander.header().setHeaderText(textOf(group.label));
            target = &expander.body();
        }
        for (const TableRow& row : group.rows)
        {
            std::optional<std::wstring> anchor;
            if (row.excerpt.has_value())
            {
                anchor = footnoteAnchor(++footnotes.count);
                writeFootnote(footnotes.text, *anchor, k_mark, row.cells.front(), *row.excerpt);
            }
            Grids::Row& gridRow = target->addRow();
            if (beside)
            {
                gridRow.onCellContent([](Grids::CellContentEvent& event) {
                    event.hasContent = event.column().tag().value != k_groupColumn;
                    event.stopPropagation();
                });
            }
            gridRow.onGetCellText([&row, anchor](Grids::GetCellTextEvent& event) {
                const TagValue column = event.column().tag().value;
                if (column == k_markColumn)
                {
                    if (anchor.has_value())
                    {
                        event.text() << PushLink{ std::wstring{ k_anchorPrefix } + *anchor }
                            << k_mark << PopLink{};
                    }
                    return;
                }
                if (column >= row.cells.size())
                    return;
                if (column == 0 && row.type)
                    event.text() << rowIcon(rowIconOf(*row.type)) << Space{ k_iconGap };
                writeRuns(event.text(), row.cells[column]);
            });
        }
    }

    // A link to a footnote goes to its anchor in the box under the grid; any other names a page.
    void PageView::connectLinks(Grids::Grid& grid, TextBox* footnotes)
    {
        grid.onCellLinkClick([this, footnotes](Grids::CellLinkClickEvent& event) {
            if (!event.target.starts_with(k_anchorPrefix))
            {
                open(event.target);
                return;
            }
            if (footnotes)
                footnotes->goToAnchor(event.target.substr(k_anchorPrefix.size()));
        });
    }

    void PageView::open(std::wstring name)
    {
        OpenNameEvent event{ std::move(name) };
        emitEvent(event);
    }

    bool PageView::hasFootnotes(const Table& table)
    {
        for (const TableGroup& group : table.groups)
        {
            for (const TableRow& row : group.rows)
            {
                if (row.excerpt.has_value())
                    return true;
            }
        }
        return false;
    }

    std::wstring PageView::footnoteAnchor(const std::size_t index)
    {
        return L"n" + std::to_wstring(index);
    }
}
