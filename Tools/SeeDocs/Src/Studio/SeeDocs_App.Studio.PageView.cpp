module SeeDocs_App.Studio.PageView;

import SeeDocs_App.Studio.Editing;
import SeeDocs_App.Studio.Icons;
import SeeDocs_App.Studio.PageText;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Browser.Actions;

import ClaFi.Controls.Expander;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Menu;
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
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        // Whether a page built again can take the controls of the one shown: the same sections,
        // the same rows in the same groups, and a footnote and a hint's comment where there were.

        [[nodiscard]] bool sameShape(const Branches& left, const Branches& right)
        {
            if (left.size() != right.size())
                return false;
            for (std::size_t i = 0; i != left.size(); ++i)
            {
                if (left[i].type != right[i].type)
                    return false;
                if (!sameShape(left[i].children, right[i].children))
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool sameShape(const TableRow& left, const TableRow& right)
        {
            return left.cells.size() == right.cells.size()
                && left.type == right.type
                && left.hint.empty() == right.hint.empty()
                && left.excerpt.has_value() == right.excerpt.has_value()
                && left.site.has_value() == right.site.has_value();
        }

        [[nodiscard]] bool sameShape(const TableGroup& left, const TableGroup& right)
        {
            const bool counts = left.rows.size() == right.rows.size()
                && left.groups.size() == right.groups.size()
                && left.label.empty() == right.label.empty();
            if (!counts)
                return false;
            for (std::size_t i = 0; i != left.rows.size(); ++i)
            {
                if (!sameShape(left.rows[i], right.rows[i]))
                    return false;
            }
            for (std::size_t i = 0; i != left.groups.size(); ++i)
            {
                if (!sameShape(left.groups[i], right.groups[i]))
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool sameShape(const Section& left, const Section& right)
        {
            const Table& leftTable = left.table;
            const Table& rightTable = right.table;
            const bool counts = left.kind == right.kind
                && leftTable.columns.size() == rightTable.columns.size()
                && leftTable.groupColumn == rightTable.groupColumn
                && leftTable.header == rightTable.header
                && leftTable.groups.size() == rightTable.groups.size()
                && left.signatures.size() == right.signatures.size();
            if (!counts || !sameShape(left.branches, right.branches))
                return false;
            for (std::size_t i = 0; i != leftTable.groups.size(); ++i)
            {
                if (!sameShape(leftTable.groups[i], rightTable.groups[i]))
                    return false;
            }
            for (std::size_t i = 0; i != left.signatures.size(); ++i)
            {
                const Signature& leftSignature = left.signatures[i];
                const Signature& rightSignature = right.signatures[i];
                if (leftSignature.excerpt.has_value() != rightSignature.excerpt.has_value())
                    return false;
                if (leftSignature.site.has_value() != rightSignature.site.has_value())
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool sameShape(const Page& left, const Page& right)
        {
            const bool counts = left.lead.empty() == right.lead.empty()
                && left.leadSite.has_value() == right.leadSite.has_value()
                && left.facts.size() == right.facts.size()
                && left.sections.size() == right.sections.size();
            if (!counts)
                return false;
            for (std::size_t i = 0; i != left.sections.size(); ++i)
            {
                if (!sameShape(left.sections[i], right.sections[i]))
                    return false;
            }
            return true;
        }

        // A page of the same shape moved into the one shown object by object, so a control
        // holding a reference into the page shown reads the new words through it.

        void moveWords(TableGroup& to, TableGroup& from)
        {
            to.label = std::move(from.label);
            to.hint = std::move(from.hint);
            for (std::size_t i = 0; i != to.rows.size(); ++i)
                to.rows[i] = std::move(from.rows[i]);
            for (std::size_t i = 0; i != to.groups.size(); ++i)
                moveWords(to.groups[i], from.groups[i]);
        }

        void moveWords(Section& to, Section& from)
        {
            to.heading = std::move(from.heading);
            to.hint = std::move(from.hint);
            to.excerpt = std::move(from.excerpt);
            to.branches = std::move(from.branches);
            to.treeOpen = from.treeOpen;
            to.table.columns = std::move(from.table.columns);
            for (std::size_t i = 0; i != to.table.groups.size(); ++i)
                moveWords(to.table.groups[i], from.table.groups[i]);
            for (std::size_t i = 0; i != to.signatures.size(); ++i)
                to.signatures[i] = std::move(from.signatures[i]);
        }

        void moveWords(Page& to, Page& from)
        {
            to.scope = std::move(from.scope);
            to.title = std::move(from.title);
            to.badges = std::move(from.badges);
            to.bases = std::move(from.bases);
            to.lead = std::move(from.lead);
            to.leadSite = std::move(from.leadSite);
            for (std::size_t i = 0; i != to.facts.size(); ++i)
                to.facts[i] = std::move(from.facts[i]);
            for (std::size_t i = 0; i != to.sections.size(); ++i)
                moveWords(to.sections[i], from.sections[i]);
            to.footnoteHome = std::move(from.footnoteHome);
        }

        // A hint the comment does not state is the box's placeholder, so typing starts on an
        // empty box.
        [[nodiscard]] PlaceHolderText noHintText()
        {
            PlaceHolderText text;
            const Run missing{ .text = std::wstring{ k_noHint }, .style = RunStyle::Missing };
            writeRuns(text, { missing });
            return text;
        }

        // The words a hint box holds: none for a missing hint, which the placeholder shows.
        [[nodiscard]] Text hintWords(const Runs& runs)
        {
            const bool missing = runs.size() == 1 && runs.front().style == RunStyle::Missing;
            if (missing)
                return {};
            return textOf(runs);
        }
    }

    // PageLinkEvent

    PageLinkEvent::PageLinkEvent(std::wstring linkTarget)
        :
        ::ClaFi::Event{},
        target{ std::move(linkTarget) }
    {
    }

    // PageView

    void PageView::setWriter(IPageWriter& writer)
    {
        m_writer = &writer;
    }

    void PageView::show(Page page)
    {
        clear();
        m_page = std::move(page);
        addHead();
        addLead();
        addFacts();
        for (const Section& section : m_page.sections)
        {
            if (section.kind == SectionKind::Signature)
            {
                addSignatures(section);
                continue;
            }
            Expander& expander = addSection(section);
            switch (section.kind)
            {
                case SectionKind::Note:
                    addProse(expander, section);
                    break;
                case SectionKind::Tree:
                    addTree(expander, section.branches, section.treeOpen);
                    break;
                case SectionKind::Table:
                    addTable(expander, section.table);
                    break;
                case SectionKind::Signature:
                    break;
            }
        }
        render();
        invalidateFormAlign();
        m_pageBox.scrollToBegin();
    }

    // Nothing is deleted and nothing scrolls: every control stands where it stood, reading the
    // page it holds a reference into, and the boxes are written again.
    bool PageView::refresh(Page page)
    {
        if (!sameShape(m_page, page))
            return false;
        moveWords(m_page, page);
        render();
        invalidateFormAlign();
        return true;
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

    // The footnote the anchor names stands in one of the slots; each is asked until one has it.
    void PageView::showAnchor(const std::wstring_view anchor)
    {
        if (anchor.empty())
            return;
        for (const PageFootnote& footnote : m_footnotes)
        {
            if (footnote.slot->shown().goToAnchor(anchor))
                return;
        }
    }

    std::wstring PageView::anchorOf(const Reference& reference) const
    {
        for (const PageFootnote& footnote : m_footnotes)
        {
            const Reference& from = footnote.excerpt->from;
            const bool reaches = from.note == reference.note && from.anchor == reference.anchor;
            if (reaches && !footnote.anchor.empty())
                return footnote.anchor;
        }
        return {};
    }

    void PageView::clear()
    {
        for (Control* part : m_parts)
            part->deleteSelf();
        m_parts.clear();
        m_renders.clear();
        m_footnotes.clear();
        m_newFootnotes.clear();
        m_footnoteCount = 0;
        m_pending = {};
    }

    // The title with its scope before it and what it is beside it, and the ways up through its
    // bases under it, in the bar that stays.
    void PageView::addHead()
    {
        m_head.setVisible(true);
        m_renders.push_back([this]() {
            Text head;
            head << TextStyleId::Title;
            writeRuns(head, m_page.scope);
            head << m_page.title << PopTextStyle{};
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
        });
    }

    void PageView::addLead()
    {
        if (m_page.lead.empty())
            return;
        HintBox& box = m_pageBox.body().add<HintBox>(
            ReadOnly::Yes,
            TextFormat{ TextStyleId::SubTitle },
            noHintText()
        );
        connectLinks(box);
        connectHintBox(box, m_page.leadSite);
        m_parts.push_back(&box);
        m_renders.push_back([this, &box]() {
            box.showHint(hintWords(m_page.lead));
        });
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
        connectLinks(grid);
        m_parts.push_back(&grid);
    }

    // An expander headed by a labelled divider - the section's heading as the label, its count in
    // the label's hint - holding its body without room round it.
    Expander& PageView::addSection(const Section& section)
    {
        HeaderText header;
        header << TextStyleId::Heading;
        writeRuns(header, section.heading);
        header << PopTextStyle{};
        Expander& expander = m_pageBox.body().add<Expander>(
            VerticalAlign::Top,
            ExpanderViewMode::Divider,
            Padding{ 0.0f },
            Spacing{ k_headerSpacing },
            std::move(header)
        );
        if (!section.hint.empty())
            expander.header().hintText() << section.hint;
        m_parts.push_back(&expander);
        return expander;
    }

    void PageView::addProse(Expander& expander, const Section& section)
    {
        Stack& body = expander.createBody<Stack>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ 0.0f }
        );
        addFootnoteSlot(body, { .excerpt = &section.excerpt }, Padding{ k_prosePadding });
    }

    void PageView::addTree(Expander& expander, const Branches& branches, const bool nodesOpen)
    {
        TreeView& tree = expander.createBody<TreeView>(
            Padding{ k_treePadding },
            HorizontalAlign::Left
        );
        addTreeRows(tree, branches, nodesOpen);
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
    // under it a slot per footnote. The hints are typed over where the page writes its words back.
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
        const bool editable = m_writer && std::ranges::any_of(table.groups, hasSites);
        TagValue hintColumn = k_noColumn;
        for (std::size_t i = 0; i != table.columns.size(); ++i)
        {
            const TableColumn& column = table.columns[i];
            const Grids::ColumnWidthMode width = column.fills
                ? Grids::ColumnWidthMode::Fill
                : Grids::ColumnWidthMode::FitContent;
            const Grids::MaxLines maxLines{ column.maxLines };
            if (column.hint && editable)
            {
                hintColumn = i;
                grid.columns().add(Tag{ i }, Text{ column.name }, width, maxLines,
                    EditorMode::Editable);
            }
            else
            {
                grid.columns().add(Tag{ i }, Text{ column.name }, width, maxLines);
            }
            if (i == 0 && footnoted)
                grid.columns().add(Tag{ k_markColumn }, Text{});
        }
        if (table.header)
            grid.addHeader();

        PageFootnotes footnotes;
        for (const TableGroup& group : table.groups)
            addTableRows(grid, table, group, body, hintColumn, footnotes);
        const Padding footnotePadding{ k_prosePadding, k_footnotePadding };
        for (PageFootnote& footnote : footnotes)
            addFootnoteSlot(body, std::move(footnote), footnotePadding);
        connectLinks(grid);
    }

    // The access label as code at the margin and, set in under it, each declaration in its inks
    // with its links, the hint under it, and the note the comment references after that. No
    // expander: the page is the declarations.
    void PageView::addSignatures(const Section& section)
    {
        Stack& block = m_pageBox.body().add<Stack>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ k_signatureGap }
        );
        TextBox& label = block.add<TextBox>(ReadOnly::Yes, Padding{ 0.0f });
        m_renders.push_back([&label, &section]() {
            Text text;
            writeSignature(text, section.heading, false);
            label.text() = text;
        });
        const Padding indent{ k_signatureIndent, 0.0f };
        for (const Signature& signature : section.signatures)
        {
            Stack& entry = block.add<Stack>(
                Orientation::Vertical,
                Padding{ 0.0f },
                Spacing{ 0.0f }
            );
            TextBox& code = entry.add<TextBox>(ReadOnly::Yes, indent);
            connectLinks(code);
            HintBox& lead = entry.add<HintBox>(ReadOnly::Yes, noHintText(), indent);
            connectLinks(lead);
            connectHintBox(lead, signature.site);
            m_renders.push_back([&code, &lead, &signature]() {
                Text text;
                writeSignature(text, signature.code, true);
                code.text() = text;
                lead.showHint(hintWords(signature.lead));
            });
            if (signature.excerpt.has_value())
            {
                addFootnoteSlot(entry, { .excerpt = &*signature.excerpt },
                    Padding{ k_signatureIndent, k_signatureGap });
            }
        }
        m_parts.push_back(&block);
    }

    // A labelled group stands beside its label in the group column, folding under it, or under
    // a held expander row of the label where the table has no such column; the groups it holds
    // follow its rows inside it, each under a header of its own.
    void PageView::addTableRows(Grids::GridBase& grid, const Table& table,
        const TableGroup& group, Stack& body, const TagValue hintColumn, PageFootnotes& footnotes)
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
                anchor = footnoteAnchor(++m_footnoteCount);
                footnotes.push_back({ .excerpt = &*row.excerpt, .anchor = *anchor, .row = &row });
            }
            Grids::Row& gridRow = target->addRow();
            if (beside)
            {
                gridRow.onCellContent([](Grids::CellContentEvent& event) {
                    event.hasContent = event.column().tag().value != k_groupColumn;
                    event.stopPropagation();
                });
            }
            if (!row.hint.empty())
            {
                gridRow.onGetCellHint([&row](Grids::GetCellHintEvent& event) {
                    if (event.column().tag().value == 0)
                        writeSignature(event.text(), row.hint, false);
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
            if (row.site.has_value() && m_writer)
            {
                if (hintColumn != k_noColumn)
                    connectHintCell(gridRow, row, hintColumn);
                if (table.takesFootnotes)
                    connectRowMenu(gridRow, row, body);
            }
        }
        for (const TableGroup& nested : group.groups)
            addTableRows(*target, table, nested, body, hintColumn, footnotes);
    }

    // THE CELL SHOWS MORE THAN THE COMMENT HOLDS - its types linked, a missing hint marked, an
    // event's payload named after it - so the editor opens on the comment's own words.
    void PageView::connectHintCell(Grids::Row& gridRow, const TableRow& row,
        const TagValue hintColumn)
    {
        gridRow.onGetCellEditorText([&row, hintColumn](Grids::GetCellEditorTextEvent& event) {
            if (event.column().tag().value == hintColumn && row.site.has_value())
                event.text = Text{ row.site->comment->text };
        });
        gridRow.onAcceptCellText([this, &row, hintColumn](Grids::AcceptCellTextEvent& event) {
            if (event.column().tag().value != hintColumn)
                return;
            // A copy: the write refreshes the page, which moves new words into the row.
            const CommentSite site = *row.site;
            const std::wstring refusal = m_writer->writeHint(site, event.text().plainText());
            if (!refusal.empty())
                event.refuse(refusal);
        });
    }

    // The row is the subject of its menu, so the commands are claimed and answered here; each
    // edit waits for the menu to close over the row.
    void PageView::connectRowMenu(Grids::Row& gridRow, const TableRow& row, Stack& body)
    {
        gridRow.onGetActionState([this, &row](GetActionStateEvent& event) {
            const bool written = footnoteOf(row) != nullptr;
            if (&event.action == &Actions::editFootnote && written)
                event.claim({});
            else if (&event.action == &Actions::addFootnote && !written)
                event.claim({ .enabled = canAddFootnote(row) });
        });
        gridRow.onActionClick([this, &row, &body](ActionClickEvent& event) {
            if (&event.action == &Actions::editFootnote)
            {
                const PageFootnote* footnote = footnoteOf(row);
                if (!footnote)
                    return;
                const std::size_t index = static_cast<std::size_t>(footnote - m_footnotes.data());
                later([this, index]() {
                    editFootnote(m_footnotes[index]);
                });
            }
            else if (&event.action == &Actions::addFootnote)
            {
                later([this, &row, &body]() {
                    addFootnote(row, body);
                });
            }
        });
        gridRow.onContextPopup([this, &row, &gridRow](ContextPopupEvent& event) {
            Menu menu{ gridRow };
            menu.add(footnoteOf(row) ? Actions::editFootnote : Actions::addFootnote);
            event.stopPropagation();
            menu.execute();
        });
    }

    void PageView::connectHintBox(HintBox& box, const std::optional<CommentSite>& site)
    {
        if (m_writer && site.has_value())
            box.connectSite(*m_writer, *site);
    }

    // Every slot claims Edit footnote for the words it shows, and offers it on the box's menu
    // beside the box's own commands.
    FootnoteSlot& PageView::addFootnoteSlot(Stack& holder, PageFootnote footnote,
        const Padding padding)
    {
        FootnoteSlot& slot = holder.add<FootnoteSlot>(padding);
        footnote.slot = &slot;
        const std::size_t index = m_footnotes.size();
        m_footnotes.push_back(std::move(footnote));
        TextBox& shown = slot.shown();
        connectLinks(shown);
        shown.onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &Actions::editFootnote)
                event.claim({ .enabled = m_writer != nullptr });
        });
        shown.onActionClick([this, index](ActionClickEvent& event) {
            if (&event.action != &Actions::editFootnote)
                return;
            later([this, index]() {
                editFootnote(m_footnotes[index]);
            });
        });
        shown.onEditContextPopup([](EditContextPopupEvent& event) {
            event.actions.push_back(nullptr);
            event.actions.push_back(&Actions::editFootnote);
        });
        m_renders.push_back([this, index]() {
            const PageFootnote& entry = m_footnotes[index];
            Text text;
            if (entry.row)
                writeFootnote(text, entry.anchor, k_mark, entry.row->cells.front(), *entry.excerpt);
            else
                writeBlocks(text, entry.excerpt->blocks);
            entry.slot->shown().text() = text;
        });
        return slot;
    }

    void PageView::editFootnote(const PageFootnote& footnote)
    {
        if (!m_writer || footnote.slot->editing())
            return;
        footnote.slot->edit(*m_writer, m_writer->draftFootnote(footnote.excerpt->from));
    }

    // A FOOTNOTE NOT WRITTEN YET STANDS UNDER THE TABLE only while its editor does: written, the
    // page is built again with it; left, its slot goes.
    void PageView::addFootnote(const TableRow& row, Stack& body)
    {
        if (!canAddFootnote(row))
            return;
        FootnoteSlot* slot = nullptr;
        if (const auto found = m_newFootnotes.find(&row); found != m_newFootnotes.end())
        {
            slot = found->second;
        }
        else
        {
            slot = &body.add<FootnoteSlot>(Padding{ k_prosePadding, k_footnotePadding });
            connectLinks(slot->shown());
            m_newFootnotes.emplace(&row, slot);
        }
        if (slot->editing())
            return;
        slot->setVisible(true);
        slot->edit(*m_writer, m_writer->draftFootnote(*row.site, *m_page.footnoteHome));
    }

    auto PageView::footnoteOf(const TableRow& row) const -> const PageFootnote*
    {
        for (const PageFootnote& footnote : m_footnotes)
        {
            if (footnote.row == &row)
                return &footnote;
        }
        return nullptr;
    }

    // A comment whose See reaches no section is a reference to mend by hand, not one to add to.
    bool PageView::canAddFootnote(const TableRow& row) const
    {
        return m_writer && row.site.has_value() && !row.site->comment->reference
            && m_page.footnoteHome.has_value();
    }

    void PageView::later(std::function<void()> edit)
    {
        m_pending = std::move(edit);
        m_later.start(MilliSeconds{ 0u });
    }

    void PageView::render()
    {
        for (const Render& each : m_renders)
            each();
    }

    // A LINK IS ANSWERED BY WHOEVER SHOWS THE PAGE, a footnote's as much as a page's: a footnote
    // is an anchor of the page, and where a tab stands is the browser's to record. See Browser
    void PageView::connectLinks(Grids::Grid& grid)
    {
        grid.onCellLinkClick([this](Grids::CellLinkClickEvent& event) {
            open(event.target);
        });
        // The row, which is where the request was raised.
        grid.onCellLinkContextPopup([this](Grids::CellLinkContextPopupEvent& event) {
            showLinkMenu(*event.popup.control, event.target, event.popup);
        });
    }

    void PageView::connectLinks(TextBox& box)
    {
        box.onLinkClick([this](LinkClickEvent& event) {
            open(event.target);
        });
        box.onLinkContextPopup([this](LinkContextPopupEvent& event) {
            showLinkMenu(event.sender(), event.target, event.popup);
        });
    }

    void PageView::open(std::wstring target)
    {
        PageLinkEvent event{ std::move(target) };
        emitEvent(event);
    }

    // THE MENU NAMES TWO COMMANDS AND SETTLES NEITHER: Open and Open in new tab are the browser's,
    // answered against the path this page names - the link the menu is up for, held for exactly
    // that long, so a target the surface does not carry arrives greyed. An anchor of the page is
    // a move on the page standing, and no menu is made for one. Stopped before the menu runs, so
    // nothing above raises a second menu behind it. See Browser
    void PageView::showLinkMenu(Control& owner, std::wstring target, ContextPopupEvent& event)
    {
        if (target.starts_with(k_anchorPrefix))
            return;
        m_menuLink = std::move(target);
        Menu menu{ owner };
        menu.add(Browser::Actions::open);
        menu.add(Browser::Actions::openInNewTab);
        event.stopPropagation();
        menu.execute();
        m_menuLink.clear();
    }

    bool PageView::hasFootnotes(const Table& table)
    {
        return std::ranges::any_of(table.groups, [](const TableGroup& group) {
            return hasFootnotes(group);
        });
    }

    bool PageView::hasFootnotes(const TableGroup& group)
    {
        const bool inRows = std::ranges::any_of(group.rows, [](const TableRow& row) {
            return row.excerpt.has_value();
        });
        return inRows || std::ranges::any_of(group.groups, [](const TableGroup& nested) {
            return hasFootnotes(nested);
        });
    }

    bool PageView::hasSites(const TableGroup& group)
    {
        const bool inRows = std::ranges::any_of(group.rows, [](const TableRow& row) {
            return row.site.has_value();
        });
        return inRows || std::ranges::any_of(group.groups, [](const TableGroup& nested) {
            return hasSites(nested);
        });
    }

    std::wstring PageView::footnoteAnchor(const std::size_t index)
    {
        return L"n" + std::to_wstring(index);
    }
}
