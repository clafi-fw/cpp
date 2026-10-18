module SeeDocs_App.PageView;

import SeeDocs_App.BasesBar;
import SeeDocs_App.Editing;
import SeeDocs_App.ExternalEditors;
import SeeDocs_App.Icons;
import SeeDocs_App.PageText;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Browser.Actions;

import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Menu;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBar;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Tree;
import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Base.StackBase;
import ClaFi.Controls.Base.WithEditMode;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Searcher;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Scaler;
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

        // A hint running over the empty cells before it was given its cells as the row was built.
        [[nodiscard]] bool sameCellsEmpty(const TableRow& left, const TableRow& right)
        {
            for (std::size_t i = 0; i != left.cells.size(); ++i)
            {
                if (left.cells[i].empty() != right.cells[i].empty())
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool sameShape(const TableRow& left, const TableRow& right)
        {
            return left.cells.size() == right.cells.size()
                && sameCellsEmpty(left, right)
                && left.type == right.type
                && left.pageKind == right.pageKind
                && left.tooltip.empty() == right.tooltip.empty()
                && left.excerpt.has_value() == right.excerpt.has_value()
                && left.site.has_value() == right.site.has_value()
                && left.command.has_value() == right.command.has_value();
        }

        [[nodiscard]] bool sameShape(const TableGroup& left, const TableGroup& right)
        {
            const bool counts = left.rows.size() == right.rows.size()
                && left.groups.size() == right.groups.size()
                && left.label.empty() == right.label.empty()
                && left.placement == right.placement;
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
                && leftTable.hintTakesEmptyCells == rightTable.hintTakesEmptyCells
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
                if (leftSignature.lead.empty() != rightSignature.lead.empty())
                    return false;
                if (leftSignature.excerpt.has_value() != rightSignature.excerpt.has_value())
                    return false;
                if (leftSignature.site.has_value() != rightSignature.site.has_value())
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool sameShape(const Facts& left, const Facts& right)
        {
            if (left.size() != right.size())
                return false;
            for (std::size_t i = 0; i != left.size(); ++i)
            {
                if (left[i].source.has_value() != right[i].source.has_value())
                    return false;
                if (left[i].folder.has_value() != right[i].folder.has_value())
                    return false;
                if (left[i].fullValue.empty() != right[i].fullValue.empty())
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool takesButton(const Fact& fact)
        {
            return fact.source.has_value() || fact.folder.has_value();
        }

        [[nodiscard]] bool sameShape(const Page& left, const Page& right)
        {
            const bool counts = left.lead.empty() == right.lead.empty()
                && left.leadSite.has_value() == right.leadSite.has_value()
                && sameShape(left.facts, right.facts)
                && left.links.empty() == right.links.empty()
                && left.sections.size() == right.sections.size()
                && left.commands.size() == right.commands.size();
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

        void moveWords(Branches& to, Branches& from)
        {
            for (std::size_t i = 0; i != to.size(); ++i)
            {
                to[i].text = std::move(from[i].text);
                moveWords(to[i].children, from[i].children);
            }
        }

        void moveWords(TableGroup& to, TableGroup& from)
        {
            to.label = std::move(from.label);
            to.tooltip = std::move(from.tooltip);
            for (std::size_t i = 0; i != to.rows.size(); ++i)
                to.rows[i] = std::move(from.rows[i]);
            for (std::size_t i = 0; i != to.groups.size(); ++i)
                moveWords(to.groups[i], from.groups[i]);
        }

        void moveWords(Section& to, Section& from)
        {
            to.heading = std::move(from.heading);
            to.tooltip = std::move(from.tooltip);
            to.excerpt = std::move(from.excerpt);
            moveWords(to.branches, from.branches);
            to.treeOpen = from.treeOpen;
            to.table.columns = std::move(from.table.columns);
            for (std::size_t i = 0; i != to.table.groups.size(); ++i)
                moveWords(to.table.groups[i], from.table.groups[i]);
            for (std::size_t i = 0; i != to.signatures.size(); ++i)
                to.signatures[i] = std::move(from.signatures[i]);
        }

        void moveWords(Page& to, Page& from)
        {
            to.kind = from.kind;
            to.surface = from.surface;
            to.type = from.type;
            to.scope = std::move(from.scope);
            to.title = std::move(from.title);
            to.badges = std::move(from.badges);
            to.titleLines = std::move(from.titleLines);
            to.bases = std::move(from.bases);
            to.lead = std::move(from.lead);
            to.leadSite = std::move(from.leadSite);
            for (std::size_t i = 0; i != to.facts.size(); ++i)
                to.facts[i] = std::move(from.facts[i]);
            to.links = std::move(from.links);
            for (std::size_t i = 0; i != to.sections.size(); ++i)
                moveWords(to.sections[i], from.sections[i]);
            to.footnoteHome = std::move(from.footnoteHome);
            for (std::size_t i = 0; i != to.commands.size(); ++i)
                to.commands[i] = std::move(from.commands[i]);
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

        // A page wears the mark its tree row does - a folder's the folder, a header's the
        // module's - and the root wears home's; a member's page has none, as its crumb has none.
        [[nodiscard]] std::optional<RowIcon> iconOf(const PageKind kind, const Type* type)
        {
            switch (kind)
            {
                case PageKind::Projects:
                    return RowIcon::Home;
                case PageKind::Project:
                    return RowIcon::Project;
                case PageKind::Folder:
                    return RowIcon::Folder;
                case PageKind::Module:
                    return RowIcon::Module;
                case PageKind::Type:
                    if (type)
                        return rowIconOf(*type);
                    return std::nullopt;
                case PageKind::Search:
                    return RowIcon::Search;
                case PageKind::Member:
                    break;
            }
            return std::nullopt;
        }

        // A row wears its type's mark, or the mark of the page its first cell opens.
        [[nodiscard]] std::optional<RowIcon> iconOf(const TableRow& row)
        {
            if (row.type)
                return rowIconOf(*row.type);
            if (row.pageKind.has_value())
                return iconOf(*row.pageKind, nullptr);
            return std::nullopt;
        }

        // The runs' words as one, which anchors an item that has no name of its own.
        [[nodiscard]] std::wstring wordsOf(const Runs& runs)
        {
            std::wstring result;
            for (const Run& run : runs)
                result.append(run.text);
            return result;
        }

        // The name a row is anchored by: its own, else its first cell's words.
        [[nodiscard]] std::wstring anchorNameOf(const TableRow& row)
        {
            if (!row.anchor.empty())
                return row.anchor;
            return row.cells.empty() ? std::wstring{} : wordsOf(row.cells.front());
        }

        // An access label as an anchor is the word without its colon.
        [[nodiscard]] std::wstring labelNameOf(const Runs& heading)
        {
            std::wstring result = wordsOf(heading);
            if (result.ends_with(L':'))
                result.pop_back();
            return result;
        }
    }

    bool shownWithin(const Control& control, const Control& host)
    {
        for (const Control* item = &control; item && item != &host; item = item->parent())
        {
            if (!item->visible())
                return false;
        }
        return true;
    }

    // PageLinkEvent

    PageLinkEvent::PageLinkEvent(std::wstring linkTarget)
        :
        ::ClaFi::Event{},
        target{ std::move(linkTarget) }
    {
    }

    // PageCommandEvent

    PageCommandEvent::PageCommandEvent(std::wstring commandTarget)
        :
        ::ClaFi::Event{},
        target{ std::move(commandTarget) }
    {
    }

    // PageOrderEvent

    PageOrderEvent::PageOrderEvent(std::vector<std::wstring> rowTargets)
        :
        ::ClaFi::Event{},
        targets{ std::move(rowTargets) }
    {
    }

    // PageSourceEvent

    PageSourceEvent::PageSourceEvent(const SourcePlace& sourcePlace)
        :
        ::ClaFi::Event{},
        source{ sourcePlace }
    {
    }

    // PageAnchorEvent

    PageAnchorEvent::PageAnchorEvent(std::wstring itemAnchor)
        :
        ::ClaFi::Event{},
        anchor{ std::move(itemAnchor) }
    {
    }

    // PageHead

    // **The crumbs are built again on every write**, because each holds the base it names by
    // reference: a page refreshed in place takes a new set of bases, and the crumbs of the old set
    // must not outlive it.
    void PageHead::write(const Page& page)
    {
        Text head;
        head << TextStyleId::Title;
        writeIcon(head, iconOf(page.kind, page.type));
        writeRuns(head, page.scope);
        head << page.title << PopTextStyle{};
        if (!page.badges.empty())
        {
            head << L"   " << TextStyleId::SubBody;
            writeRuns(head, page.badges);
            head << PopTextStyle{};
        }
        for (const Runs& line : page.titleLines)
        {
            head << k_endLine << ParaIndent{ k_basesIndent };
            writeRuns(head, line);
        }
        m_title.text() = head;
        m_bases.setBases(page.bases);
    }

    void PageHead::write(const std::optional<RowIcon> icon, const std::wstring_view title)
    {
        Text head;
        head << TextStyleId::Title;
        writeIcon(head, icon);
        head << title << PopTextStyle{};
        m_title.text() = head;
        m_bases.clearBases();
    }

    void PageHead::clear()
    {
        m_title.text().clear();
        m_bases.clearBases();
    }

    void PageHead::writeIcon(Text& head, const std::optional<RowIcon> icon)
    {
        if (icon.has_value())
            head << titleIcon(*icon) << Space{ k_iconGap };
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
        addLinks();
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
        addCommands();
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
        m_head.clear();
        m_page = {};
        m_head.setVisible(false);
        TextBox& box = m_pageBox.body().add<TextBox>(ReadOnly::Yes, EditModeSwitching::Yes);
        box.text() = text;
        m_parts.push_back(&box);
        invalidateFormAlign();
        m_pageBox.scrollToBegin();
    }

    // The footnote the anchor names stands in one of the slots; each is asked until one has it.
    // The position waits for the layout: a page just built has no height to be scrolled through,
    // and a row selected as the page is built would be scrolled to by the pass otherwise. It is
    // measured from the item where the anchor names one, so a page laid out at another width
    // shows the item where it stood.
    Control& PageView::showAnchor(const std::wstring_view anchor,
        const std::optional<float> position)
    {
        m_pendingPosition = position;
        m_shownAnchor = m_anchors.contains(anchor) ? std::wstring{ anchor } : std::wstring{};
        if (anchor.empty())
            return *this;
        for (const PageFootnote& footnote : m_footnotes)
        {
            TextBox& shown = footnote.slot->shown();
            if (shown.goToAnchor(anchor))
                return shown;
        }
        const Anchors::const_iterator found = m_anchors.find(anchor);
        if (found == m_anchors.end())
            return *this;
        const Anchored& named = found->second;
        if (!named.control && named.branch)
            buildBranchRow(*named.branch);
        if (!named.control)
            return *this;
        Control& item = *named.control;
        item.reveal(m_pageBox.body());
        if (named.grid)
        {
            named.grid->selectCell(*named.row, named.grid->findColumnByTag(Tag{ 0 }));
            m_anchoredRow = named.row;
        }
        else
        {
            m_pageBox.body().setCurrentItem(item);
        }
        if (!position.has_value())
            item.scrollIntoViewOnAlign();
        return item;
    }

    float PageView::viewPosition() const
    {
        const float scrolled = m_pageBox.vScrollBar().positionTarget();
        const Anchors::const_iterator found = m_anchors.find(m_shownAnchor);
        if (found == m_anchors.end() || !found->second.control)
            return scaler().unScale(scrolled);
        return scaler().unScale(topInBody(*found->second.control) - scrolled);
    }

    // A match whose section was shut since stands hidden, and is passed over.
    bool PageView::focusFound()
    {
        Control* holder = m_found.currentHolder();
        if (!holder || !shownWithin(*holder, *this) || !holder->canTakeFocus())
            return false;
        holder->scrollIntoView();
        holder->setFocus();
        return true;
    }

    // A row in a section shut since, or in a group folded, stands hidden and is passed over.
    void PageView::focusContent()
    {
        if (focusFound())
            return;
        for (Grids::Row* row : { m_anchoredRow, m_firstRow })
        {
            if (row && shownWithin(*row, *this))
            {
                row->scrollIntoView();
                row->setFocus();
                return;
            }
        }
    }

    // The footnote showing the section in full comes first, then one naming where it stands.
    std::wstring PageView::anchorOf(const Reference& reference) const
    {
        const std::wstring shown = shownAnchorOf(reference);
        if (!shown.empty())
            return shown;
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
        m_editorWatches.clear();
        for (Control* part : m_parts)
            part->deleteSelf();
        m_parts.clear();
        m_renders.clear();
        m_footnotes.clear();
        m_anchors.clear();
        m_anchorsOf.clear();
        m_branchRows.clear();
        m_firstRow = nullptr;
        m_anchoredRow = nullptr;
        m_pendingPosition.reset();
        m_shownAnchor.clear();
        m_newFootnotes.clear();
        m_footnoteCount = 0;
        m_pending = {};
    }

    void PageView::addHead()
    {
        m_head.setVisible(true);
        m_renders.push_back([this]() {
            m_head.write(m_page);
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
        anchorItem(std::wstring{ k_leadAnchor }, {}, box);
        m_renders.push_back([this, &box]() {
            box.showWords(hintWords(m_page.lead));
        });
    }

    // The facts as a grid, the label muted. A fact naming a source or a folder has a button
    // opening it at its row's end; the words of a fact with none stand across the button's column.
    void PageView::addFacts()
    {
        if (m_page.facts.empty())
            return;
        Grids::Grid& grid = m_pageBox.body().add<Grids::Grid>(
            themeMetrics().page,
            UiElement::Section,
            Grids::GridLines::Horizontal,
            Interactivity::None // the page is the container; the rows are its items
        );
        grid.columns().add(Tag{ k_factLabel }, TextFormat{ InkGrade::Muted });
        const bool opens = std::ranges::any_of(m_page.facts, takesButton);
        Grids::Column* buttonColumn = nullptr;
        if (opens)
        {
            Grids::Column& across = grid.columns().add(Tag{ k_factAcross },
                Grids::ColumnWidthMode::Fill);
            Grids::ColumnCollection& under = across.createSubColumns();
            under.add(Tag{ k_factValue }, Grids::ColumnWidthMode::Fill);
            buttonColumn = &under.add(Tag{ k_factButton });
        }
        else
        {
            grid.columns().add(Tag{ k_factValue }, Grids::ColumnWidthMode::Fill);
        }
        for (const Fact& fact : m_page.facts)
        {
            const bool hasButton = takesButton(fact);
            Grids::Row* made;
            if (hasButton)
            {
                Grids::RowContainer& container = grid.addControlRow();
                Button& button = container.addControl<Button>(*buttonColumn);
                if (fact.source.has_value())
                    connectSource(button, fact);
                else
                    connectFolder(button, fact);
                made = &container;
            }
            else
            {
                made = &grid.addRow();
            }
            Grids::Row& row = *made;
            if (!m_firstRow)
                m_firstRow = &row;
            anchorRow(fact.label, {}, grid, row);
            if (opens)
            {
                row.onCellContent([hasButton](Grids::CellContentEvent& event) {
                    const TagValue column = event.column().tag().value;
                    if (column == k_factAcross)
                        event.hasContent = !hasButton;
                    else if (column != k_factLabel)
                        event.hasContent = hasButton;
                });
            }
            row.onGetCellText([&fact](Grids::GetCellTextEvent& event) {
                const TagValue column = event.column().tag().value;
                if (column == k_factLabel)
                    event.text() << fact.label;
                else if (column == k_factValue || column == k_factAcross)
                    writeRuns(event.text(), fact.value);
            });
            if (!fact.fullValue.empty())
                connectFullValue(row, fact);
        }
        connectLinks(grid);
        m_parts.push_back(&grid);
    }

    // The page's own links on a line of their own, ahead of the sections.
    void PageView::addLinks()
    {
        if (m_page.links.empty())
            return;
        TextBox& box = m_pageBox.body().add<TextBox>(
            ReadOnly::Yes,
            EditModeSwitching::Yes,
            Padding{ 0.0f }
        );
        connectLinks(box);
        m_parts.push_back(&box);
        anchorItem(std::wstring{ k_linksAnchor }, {}, box);
        m_renders.push_back([this, &box]() {
            box.text() = textOf(m_page.links);
        });
    }

    // An expander in the page's look - the section's heading as its label, its count in the
    // label's tooltip - holding its body without room round it.
    Expander& PageView::addSection(const Section& section)
    {
        HeaderText header;
        header << TextStyleId::Heading;
        writeRuns(header, section.heading);
        header << PopTextStyle{};
        Expander& expander = m_pageBox.body().add<Expander>(
            VerticalAlign::Top,
            k_expanderViewMode,
            Padding{ 0.0f },
            Spacing{ k_headerSpacing },
            std::move(header)
        );
        if (!section.tooltip.empty())
            expander.header().tooltipText() << section.tooltip;
        m_parts.push_back(&expander);
        anchorItem(wordsOf(section.heading), {}, expander.header());
        return expander;
    }

    void PageView::addProse(Expander& expander, const Section& section)
    {
        StackPanel& body = expander.createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ 0.0f }
        );
        FootnoteSlot& slot = addFootnoteSlot(body, { .excerpt = &section.excerpt },
            Padding{ k_prosePadding });
        anchorItem(wordsOf(section.heading), {}, slot.shown());
    }

    void PageView::addTree(Expander& expander, const Branches& branches, const bool nodesOpen)
    {
        TreeView& tree = expander.createBody<TreeView>(
            Padding{ k_treePadding },
            HorizontalAlign::Left,
            Interactivity::None // the page is the container; the rows are its items
        );
        anchorBranches(branches, nullptr);
        addTreeRows(tree, branches, nodesOpen);
    }

    // A branch is anchored by its type's name, or by its words where it names no type.
    void PageView::anchorBranches(const Branches& branches, const Branch* parent)
    {
        for (const Branch& branch : branches)
        {
            std::wstring name = branch.type ? branch.type->name : std::wstring{};
            if (name.empty())
                name = wordsOf(branch.text);
            const std::wstring anchor = freeAnchor(std::move(name), {});
            m_anchors.insert_or_assign(anchor, Anchored{
                .control = nullptr,
                .grid = nullptr,
                .row = nullptr,
                .branch = &branch
            });
            m_branchRows.insert_or_assign(&branch, BranchRow{
                .anchor = anchor,
                .parent = parent,
                .control = nullptr,
                .node = nullptr
            });
            anchorBranches(branch.children, &branch);
        }
    }

    void PageView::rowBuilt(const Branch& branch, Control& row, Controls::TreeNode* node)
    {
        const BranchRows::iterator found = m_branchRows.find(&branch);
        if (found == m_branchRows.end())
            return;
        BranchRow& branchRow = found->second;
        branchRow.control = &row;
        branchRow.node = node;
        m_anchorsOf.insert_or_assign(&row, branchRow.anchor);
        const Anchors::iterator anchored = m_anchors.find(branchRow.anchor);
        if (anchored != m_anchors.end())
            anchored->second.control = &row;
    }

    // Opening a node builds the rows under it at once, so the branch's row stands by the time
    // the node over it has been opened; a branch at the top of its tree stands with the tree.
    Control* PageView::buildBranchRow(const Branch& branch)
    {
        const BranchRows::iterator found = m_branchRows.find(&branch);
        if (found == m_branchRows.end())
            return nullptr;
        if (found->second.control || !found->second.parent)
            return found->second.control;
        buildBranchRow(*found->second.parent);
        const BranchRows::iterator parentFound = m_branchRows.find(found->second.parent);
        if (parentFound == m_branchRows.end() || !parentFound->second.node)
            return nullptr;
        parentFound->second.node->header().setExpanded(true);
        return found->second.control;
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

    // The grid: the group column where the table names one, its own columns, the marks where
    // any row has a footnote and the buttons where any has a command, a header where the table
    // asks for one, and under it a slot per footnote. The hints are typed over where the page
    // writes its words back.
    void PageView::addTable(Expander& expander, const Table& table)
    {
        StackPanel& body = expander.createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ 0.0f }
        );
        Grids::Grid& grid = body.add<Grids::Grid>(
            themeMetrics().page,
            UiElement::Section,
            Grids::GridLines::Horizontal,
            Interactivity::None, // the page is the container; the rows are its items
            table.rearrangeable ? ItemsOrder::Rearrangeable : ItemsOrder::Fixed
        );
        if (table.rearrangeable)
        {
            grid.onItemsMove([this, &grid](ItemsMoveEvent&) {
                reportOrder(grid);
            });
        }
        StackPanel& notes = body.add<StackPanel>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ k_footnoteGap }
        );
        if (!table.groupColumn.empty())
            grid.columns().add(Tag{ k_groupColumn }, Text{ table.groupColumn });
        const bool footnoted = hasFootnotes(table);
        const bool editable = std::ranges::any_of(table.groups, [this](const TableGroup& group) {
            return writesHints(group);
        });
        const bool spans = table.hintTakesEmptyCells;
        // A hint that runs over cells is written in a parent that would cover the marks too.
        const bool marksUnderHint = footnoted && !spans;
        const std::size_t lastColumn = table.columns.size() - 1;
        Grids::ColumnCollection* into = &grid.columns();
        TagValue hintColumn = k_noColumn;
        for (std::size_t i = 0; i != table.columns.size(); ++i)
        {
            const TableColumn& column = table.columns[i];
            const Grids::ColumnWidthMode width = column.fills
                ? Grids::ColumnWidthMode::Fill
                : Grids::ColumnWidthMode::FitContent;
            const Grids::MaxLines maxLines{ column.maxLines };
            Grids::ShowInHeader named = Grids::ShowInHeader::Yes;
            // A parent over this column and the ones after it, which a hint running over the
            // empty cells from here on is written in.
            if (spans && i != 0 && i != lastColumn)
            {
                Grids::Column& across = into->add(
                    Tag{ acrossColumn(i) },
                    Grids::ColumnWidthMode::Fill,
                    Grids::ShowInHeader::No,
                    editable ? EditorMode::Editable : EditorMode::ReadOnly
                );
                into = &across.createSubColumns();
            }
            // The last column's name stands over its cells and the marks.
            if (marksUnderHint && i == lastColumn)
            {
                Grids::Column& parent = into->add(Tag{ k_markParentColumn }, Text{ column.name },
                    width);
                into = &parent.createSubColumns();
                named = Grids::ShowInHeader::No;
            }
            EditorMode editor = EditorMode::ReadOnly;
            if (column.hint && editable)
            {
                hintColumn = i;
                editor = EditorMode::Editable;
            }
            into->add(Tag{ i }, Text{ column.name }, width, maxLines, named, editor);
        }
        if (footnoted)
        {
            Grids::ColumnCollection& marks = marksUnderHint ? *into : grid.columns();
            marks.add(Tag{ k_markColumn }, Grids::ShowInHeader::No);
        }
        Grids::Column* commandColumn = nullptr;
        if (hasCommands(table))
            commandColumn = &grid.columns().add(Tag{ k_commandColumn }, Grids::ShowInHeader::No);
        // A row fills neither the group column nor the parent over the marks, and a parent a hint
        // runs over only where it says so.
        grid.onCellContent([](Grids::CellContentEvent& event) {
            const TagValue column = event.column().tag().value;
            event.hasContent = column != k_groupColumn && column != k_markParentColumn
                && !isAcrossColumn(column);
        });
        if (table.header)
            grid.addHeader();

        PageFootnotes footnotes;
        for (const TableGroup& group : table.groups)
            addTableRows(grid, table, group, {}, notes, hintColumn, commandColumn, footnotes);
        const Padding footnotePadding{ k_prosePadding, k_footnotePadding };
        for (PageFootnote& footnote : footnotes)
            addFootnoteSlot(notes, std::move(footnote), footnotePadding);
        connectLinks(grid);
    }

    // The access label as code at the margin and, set in under it, each declaration in its inks
    // with its links, the hint under it, and the note the comment references after that. No
    // expander: the page is the declarations.
    void PageView::addSignatures(const Section& section)
    {
        StackPanel& block = m_pageBox.body().add<StackPanel>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ k_signatureGap }
        );
        const bool labelled = !section.heading.empty();
        // Every box of the block is anchored by the label, numbered down the block.
        const std::wstring name = labelled
            ? labelNameOf(section.heading)
            : std::wstring{ k_signatureAnchor };
        if (labelled)
        {
            TextBox& label = block.add<TextBox>(
                ReadOnly::Yes,
                EditModeSwitching::Yes,
                Padding{ 0.0f }
            );
            anchorItem(name, {}, label);
            m_renders.push_back([&label, &section]() {
                Text text;
                writeSignature(text, section.heading, false, SourceLanguage::Cpp);
                label.text() = text;
            });
        }
        const float indentWidth = labelled ? k_signatureIndent : 0.0f;
        const Padding indent{ indentWidth, 0.0f };
        for (const Signature& signature : section.signatures)
        {
            StackPanel& entry = block.add<StackPanel>(
                Orientation::Vertical,
                Padding{ 0.0f },
                Spacing{ k_leadGap }
            );
            TextBox& code = entry.add<TextBox>(ReadOnly::Yes, EditModeSwitching::Yes, indent);
            connectLinks(code);
            anchorItem(name, {}, code);
            m_renders.push_back([&code, &signature]() {
                Text text;
                writeSignature(text, signature.code, true, signature.language);
                code.text() = text;
            });
            if (!signature.lead.empty())
            {
                HintBox& lead = entry.add<HintBox>(ReadOnly::Yes, noHintText(), indent);
                connectLinks(lead);
                connectHintBox(lead, signature.site);
                anchorItem(name, {}, lead);
                m_renders.push_back([&lead, &signature]() {
                    lead.showWords(hintWords(signature.lead));
                });
            }
            if (signature.excerpt.has_value())
            {
                FootnoteSlot& slot = addFootnoteSlot(entry, { .excerpt = &*signature.excerpt },
                    indent);
                anchorItem(name, {}, slot.shown());
            }
        }
        m_parts.push_back(&block);
    }

    void PageView::addCommands()
    {
        StackPanel& bar = m_head.commandBar();
        bar.setVisible(!m_page.commands.empty());
        for (const Command& command : m_page.commands)
        {
            Button& button = bar.add<Button>();
            connectCommand(button, command);
            m_parts.push_back(&button);
        }
    }

    // A labelled group stands beside its label in the group column, folding under it, or under
    // a held expander row of the label, where its links are followed as a cell's are; the groups
    // it holds follow its rows inside it, each under a label of its own. A row with a command is
    // one that holds a control, its button, in the column at the end.
    void PageView::addTableRows(Grids::GridBase& grid, const Table& table,
        const TableGroup& group, const std::wstring_view owner, StackPanel& notes,
        const TagValue hintColumn, Grids::Column* commandColumn, PageFootnotes& footnotes)
    {
        const bool beside = group.placement == LabelPlacement::Beside
            && !table.groupColumn.empty();
        const std::wstring_view rowsOwner = group.owner.empty() ? owner : group.owner;
        Grids::GridBase* target = &grid;
        if (!group.label.empty() && beside)
        {
            const Grids::Collapsible collapsible = group.folded
                ? Grids::Collapsible::Collapsed
                : Grids::Collapsible::Expanded;
            Grids::RowGroup& groupRow = grid.addGroup(collapsible);
            Grids::RowGroupSpan& span = groupRow.span();
            span.onCellContent([](Grids::CellContentEvent& event) {
                event.hasContent = event.column().tag().value == k_groupColumn;
                event.stopPropagation();
            });
            span.onGetCellText([&group](Grids::GetCellTextEvent& event) {
                writeLabel(event.text(), group);
            });
            if (!group.tooltip.empty())
            {
                span.onGetCellTooltip([&group](Grids::GetCellTooltipEvent& event) {
                    writeRuns(event.text(), group.tooltip);
                });
            }
            target = &groupRow.body();
        }
        else if (!group.label.empty())
        {
            Grids::RowExpander& groupExpander = grid.addExpander();
            ExpanderHeader& header = groupExpander.header();
            Text label;
            writeLabel(label, group);
            header.setHeaderText(label);
            if (!group.tooltip.empty())
            {
                header.onGetTooltip([&group](GetTooltipEvent& event) {
                    writeRuns(event.text, group.tooltip);
                });
            }
            connectLinks(header);
            if (group.folded)
                header.setExpanded(false);
            anchorItem(wordsOf(group.label), {}, header);
            target = &groupExpander.body();
        }
        for (const TableRow& row : group.rows)
        {
            std::optional<std::wstring> anchor;
            if (row.excerpt.has_value())
            {
                anchor = footnoteAnchor(++m_footnoteCount);
                footnotes.push_back({ .excerpt = &*row.excerpt, .anchor = *anchor, .row = &row });
            }
            Grids::Row* made;
            if (row.command.has_value() && commandColumn)
            {
                Grids::RowContainer& container = target->addControlRow();
                connectCommand(container.addControl<Button>(*commandColumn), *row.command);
                made = &container;
            }
            else
            {
                made = &target->addRow();
            }
            Grids::Row& gridRow = *made;
            // A carried row is told apart by the table row it was built from.
            if (table.rearrangeable)
                gridRow.setTag(Tag{ &row });
            if (!m_firstRow)
                m_firstRow = &gridRow;
            anchorRow(anchorNameOf(row), rowsOwner, *target, gridRow);
            if (!row.tooltip.empty())
            {
                gridRow.onGetCellTooltip([&row](Grids::GetCellTooltipEvent& event) {
                    if (event.column().tag().value == 0)
                        writeSignature(event.text(), row.tooltip, false, row.language);
                });
            }
            // The mark previews its footnote over the row, as a link previews its anchor.
            if (anchor.has_value())
            {
                gridRow.onGetCellTooltip([&row](Grids::GetCellTooltipEvent& event) {
                    if (event.column().tag().value != k_markColumn)
                        return;
                    GetTooltipEvent& tooltip = event.tooltip();
                    tooltip.placement = FormPlacement::Top;
                    tooltip.writePreview(textOf(row.excerpt->blocks));
                });
            }
            // **A hint running over the empty cells before it** is written in the parent over
            // them, and the cells under that parent are its.
            const TagValue spanColumn = hintSpanColumn(row, table.hintTakesEmptyCells);
            if (spanColumn != k_noColumn)
            {
                const std::size_t hintFrom = spanColumn - k_acrossColumn;
                gridRow.onCellContent([spanColumn, hintFrom](Grids::CellContentEvent& event) {
                    const TagValue column = event.column().tag().value;
                    if (isAcrossColumn(column))
                        event.hasContent = column == spanColumn;
                    else if (column >= hintFrom && column < k_acrossColumn)
                        event.hasContent = false;
                    else
                        return;
                    event.stopPropagation();
                });
            }
            gridRow.onGetCellText([&row, anchor, spanColumn](Grids::GetCellTextEvent& event) {
                TagValue column = event.column().tag().value;
                if (column == k_markColumn)
                {
                    if (anchor.has_value())
                    {
                        event.text() << PushLink{ std::wstring{ k_anchorPrefix } + *anchor }
                            << InkColor::Spot << k_mark << PopColor{} << PopLink{};
                    }
                    return;
                }
                if (column == spanColumn)
                    column = row.cells.size() - 1;
                if (column >= row.cells.size())
                    return;
                if (column == 0)
                {
                    const std::optional<RowIcon> icon = iconOf(row);
                    if (icon.has_value())
                        event.text() << rowIcon(*icon) << k_cellIconGap;
                }
                writeRuns(event.text(), row.cells[column]);
                // A qualifier stands at its cell's end, as a tree row's kind word does.
                if (column < row.qualifiers.size() && !row.qualifiers[column].empty())
                {
                    event.text() << FlexSpace{ k_kindGap } << TextStyleId::SubBody;
                    writeRuns(event.text(), row.qualifiers[column]);
                    event.text() << PopTextStyle{};
                }
            });
            if (row.site.has_value() && m_writer)
            {
                if (hintColumn != k_noColumn && writesHint(row.site))
                {
                    const TagValue editedColumn = spanColumn == k_noColumn
                        ? hintColumn
                        : spanColumn;
                    connectHintCell(gridRow, row, editedColumn);
                }
                if (table.takesFootnotes)
                    connectRowMenu(gridRow, row, notes);
            }
        }
        for (const TableGroup& nested : group.groups)
        {
            addTableRows(*target, table, nested, rowsOwner, notes, hintColumn, commandColumn,
                footnotes);
        }
    }

    // Every anchor is spelled apart from the others on the page, so a url names one item. A
    // footnote's anchor is the one its mark links to, and a footnote is made after the rows of
    // its table, so no row takes it.
    void PageView::anchorItem(std::wstring name, const std::wstring_view owner, Control& item)
    {
        anchorAs(std::move(name), owner,
            { .control = &item, .grid = nullptr, .row = nullptr, .branch = nullptr });
    }

    void PageView::anchorRow(std::wstring name, const std::wstring_view owner,
        Grids::GridBase& grid, Grids::Row& row)
    {
        anchorAs(std::move(name), owner,
            { .control = &row, .grid = &grid, .row = &row, .branch = nullptr });
    }

    void PageView::anchorAs(std::wstring name, const std::wstring_view owner, const Anchored item)
    {
        if (name.empty())
            return;
        const std::wstring anchor = freeAnchor(std::move(name), owner);
        m_anchorsOf.insert_or_assign(item.control, anchor);
        m_anchors.insert_or_assign(anchor, item);
    }

    std::wstring PageView::freeAnchor(std::wstring name, const std::wstring_view owner) const
    {
        if (!m_anchors.contains(name))
            return name;
        if (!owner.empty())
        {
            std::wstring owned{ owner };
            owned.append(k_ownerJoin);
            owned.append(name);
            if (!m_anchors.contains(owned))
                return owned;
        }
        for (std::size_t ordinal = 2;; ++ordinal)
        {
            std::wstring numbered = name;
            numbered.push_back(k_ordinalMark);
            numbered.append(std::to_wstring(ordinal));
            if (!m_anchors.contains(numbered))
                return numbered;
        }
    }

    const Control* PageView::selectedAnchored() const
    {
        const StackPanel& host = m_pageBox.body();
        for (const Control* item = host.currentItem(); item && item != &host;
            item = item->parent())
        {
            if (m_anchorsOf.contains(item))
                return item;
        }
        return nullptr;
    }

    // The body stands in the box at its slot's top less the scroll, so the difference of the two
    // tops is the item's place in the content, however far the page is scrolled.
    float PageView::topInBody(const Control& item) const
    {
        return item.boundsInForm().top - m_pageBox.body().boundsInForm().top;
    }

    void PageView::reportAnchor()
    {
        m_shownAnchor.clear();
        if (const Control* item = selectedAnchored())
            m_shownAnchor = m_anchorsOf.find(item)->second;
        PageAnchorEvent event{ m_shownAnchor };
        emitEvent(event);
    }

    void PageView::reportOrder(Grids::Grid& grid)
    {
        std::vector<std::wstring> targets{};
        for (ControlPtr& control : grid.controls())
        {
            const TableRow* row = control->tag().get<const TableRow*>();
            if (!row)
                continue;
            std::wstring target{};
            if (!row->cells.empty())
            {
                for (const Run& run : row->cells.front())
                {
                    if (run.link.empty())
                        continue;
                    target = run.link;
                    break;
                }
            }
            targets.push_back(std::move(target));
        }
        PageOrderEvent event{ std::move(targets) };
        emitEvent(event);
    }

    void PageView::connectCommand(Button& button, const Command& command)
    {
        button.onClick([this, &command](ClickEvent&) {
            ask(command.target);
        });
        m_renders.push_back([&button, &command]() {
            button.text() = Text{ command.text };
            button.tooltipText() = TooltipText{ command.tooltip };
        });
    }

    // **The setting is read whenever the button is asked**, and a change to it asks again, so a
    // command line typed on the Settings page takes the grey off a page standing behind it.
    void PageView::connectSource(Button& button, const Fact& fact)
    {
        button.text() = Text{ k_openInEditorCaption };
        button.onGetState([this, &fact](GetStateEvent& event) {
            event.state.enabled = hasEditorCommand(appContext(), fact.source->language);
        });
        button.onGetTooltip([this, &fact](GetTooltipEvent& event) {
            const SourceLanguage language = fact.source->language;
            if (hasEditorCommand(appContext(), language))
                return;
            event.text << L"Set a " << editorLanguageName(language) << L" editor in Settings";
        });
        button.onClick([this, &fact](ClickEvent&) {
            PageSourceEvent event{ *fact.source };
            emitEvent(event);
        });
        m_editorWatches.push_back(onEditorCommandChange(appContext(), fact.source->language,
            [&button]() {
                button.invalidateState();
            }));
    }

    void PageView::connectFolder(Button& button, const Fact& fact)
    {
        button.text() = Text{ k_openFolder };
        button.onClick([this, &fact](ClickEvent&) {
            Platform::shellExecute(form(), fact.folder->wstring());
        });
    }

    void PageView::connectFullValue(Grids::Row& row, const Fact& fact)
    {
        row.onGetCellTooltip([&fact](Grids::GetCellTooltipEvent& event) {
            if (event.column().tag().value == k_factValue)
                event.text() << fact.fullValue;
        });
        row.onGetCellEditorText([&fact](Grids::GetCellEditorTextEvent& event) {
            if (event.column().tag().value == k_factValue)
                event.text = Text{ fact.fullValue };
        });
    }

    // **The cell shows more than the comment holds** - its types linked, a missing hint marked, an
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
    void PageView::connectRowMenu(Grids::Row& gridRow, const TableRow& row, StackPanel& notes)
    {
        gridRow.onGetActionState([this, &row](GetActionStateEvent& event) {
            const bool written = footnoteOf(row) != nullptr;
            if (&event.action == &Actions::editFootnote && written)
                event.claim({});
            else if (&event.action == &Actions::addFootnote && !written)
                event.claim({ .enabled = canAddFootnote(row) });
        });
        gridRow.onActionClick([this, &row, &notes](ActionClickEvent& event) {
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
                later([this, &row, &notes]() {
                    addFootnote(row, notes);
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
        if (writesHint(site))
            box.connectSite(*m_writer, *site);
    }

    // Every slot claims Edit footnote for the words it shows, and offers it on the box's menu
    // beside the box's own commands.
    FootnoteSlot& PageView::addFootnoteSlot(StackPanel& holder, PageFootnote footnote,
        const Padding padding)
    {
        FootnoteSlot& slot = holder.add<FootnoteSlot>(padding);
        footnote.slot = &slot;
        const std::size_t index = m_footnotes.size();
        const std::wstring anchor = footnote.anchor;
        m_footnotes.push_back(std::move(footnote));
        TextBox& shown = slot.shown();
        connectLinks(shown);
        if (!anchor.empty())
            anchorItem(anchor, {}, shown);
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
        shown.onAdjustContextPopup([](AdjustContextPopupEvent& event) {
            event.actions.push_back(nullptr);
            event.actions.push_back(&Actions::editFootnote);
        });
        m_renders.push_back([this, index]() {
            const PageFootnote& entry = m_footnotes[index];
            const Excerpt& excerpt = *entry.excerpt;
            Text text;
            if (excerpt.shownAt.has_value())
            {
                const Runs name = entry.row ? entry.row->cells.front() : Runs{};
                const std::wstring holder = shownAnchorOf(excerpt.from);
                const std::wstring link = holder.empty()
                    ? std::wstring{}
                    : std::wstring{ k_anchorPrefix } + holder;
                writeShownAt(text, entry.anchor, k_mark, name, *excerpt.shownAt, link);
            }
            else if (entry.row)
            {
                writeFootnote(text, entry.anchor, k_mark, entry.row->cells.front(), excerpt);
            }
            else
            {
                writeBlocks(text, excerpt.blocks);
            }
            entry.slot->shown().text() = text;
        });
        return slot;
    }

    void PageView::editFootnote(const PageFootnote& footnote) const
    {
        if (!m_writer || footnote.slot->editing())
            return;
        footnote.slot->edit(*m_writer, m_writer->draftFootnote(footnote.excerpt->from));
    }

    // **A footnote not written yet stands under the table** only while its editor does: written,
    // the page is built again with it; left, its slot goes.
    void PageView::addFootnote(const TableRow& row, StackPanel& notes)
    {
        if (!canAddFootnote(row))
            return;
        FootnoteSlot* slot;
        if (const auto found = m_newFootnotes.find(&row); found != m_newFootnotes.end())
        {
            slot = found->second;
        }
        else
        {
            slot = &notes.add<FootnoteSlot>(Padding{ k_prosePadding, k_footnotePadding });
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

    std::wstring PageView::shownAnchorOf(const Reference& reference) const
    {
        for (const PageFootnote& footnote : m_footnotes)
        {
            const Reference& from = footnote.excerpt->from;
            const bool reaches = from.note == reference.note && from.anchor == reference.anchor;
            if (reaches && !footnote.excerpt->shownAt.has_value() && !footnote.anchor.empty())
                return footnote.anchor;
        }
        return {};
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

    // **A link is answered by whoever shows the page**, a footnote's as much as a page's: a
    // footnote is an anchor of the page, and where a tab stands is the browser's to record.
    // See Browser
    void PageView::connectLinks(Grids::Grid& grid)
    {
        grid.onCellLinkClick([this](Grids::CellLinkClickEvent& event) {
            open(event.target);
        });
        // The row, which is where the request was raised.
        grid.onCellLinkContextPopup([this](Grids::CellLinkContextPopupEvent& event) {
            showLinkMenu(*event.popup.control, event.target, event.popup);
        });
        // Over the cell, as a link's tooltip stands over its line - where the row has not answered
        // for the cell already, as a method's row answers with its signatures.
        grid.onGetCellTooltip([this](Grids::GetCellTooltipEvent& event) {
            if (event.link().empty() || !event.text().empty())
                return;
            if (writePreview(event.text(), event.link()))
                event.tooltip().placement = FormPlacement::Top;
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
        box.onGetLinkTooltip([this](GetLinkTooltipEvent& event) {
            writePreview(event.tooltip.text, event.target);
        });
    }

    void PageView::connectLinks(ExpanderHeader& header)
    {
        header.onLinkClick([this](HeaderLinkClickEvent& event) {
            open(event.target);
        });
        header.onLinkContextPopup([this](HeaderLinkContextPopupEvent& event) {
            showLinkMenu(event.sender(), event.target, event.popup);
        });
    }

    bool PageView::writePreview(Text& text, const std::wstring_view link) const
    {
        if (!m_page.surface)
            return false;
        const std::optional<Preview> preview = previewOf(*m_page.surface, link);
        if (!preview.has_value())
            return false;
        writeSignature(text, preview->code, false, preview->language);
        return true;
    }

    // **A crumb's page opens once the press has unwound**: the page shown next builds the crumbs
    // again, and the one pressed - or the one whose list is still closing - goes with them.
    void PageView::connectHead()
    {
        connectLinks(m_head.titleBox());
        BasesBar& bases = m_head.basesBar();
        bases.onBaseLink([this](BaseLinkEvent& event) {
            later([this, target = event.target]() {
                open(target);
            });
        });
        bases.onBaseLinkContextPopup([this](BaseLinkContextPopupEvent& event) {
            showLinkMenu(event.crumb, event.target, event.popup);
        });
    }

    void PageView::open(std::wstring target)
    {
        PageLinkEvent event{ std::move(target) };
        emitEvent(event);
    }

    // **A command is answered by whoever shows the page**, as a link is: what it is about was
    // spelled by whoever built the page, and the page may be one of the things its answer takes
    // down.
    void PageView::ask(std::wstring target)
    {
        PageCommandEvent event{ std::move(target) };
        emitEvent(event);
    }

    // **The menu names two commands and settles neither**: Open and Open in new tab are the
    // browser's, answered against the path this page names - the link the menu is up for, held for
    // exactly that long, so a target the surface does not carry arrives greyed. An anchor of the
    // page is a move on the page standing, and no menu is made for one. Stopped before the menu
    // runs, so nothing above raises a second menu behind it. See Browser
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

    bool PageView::hasCommands(const Table& table)
    {
        return std::ranges::any_of(table.groups, [](const TableGroup& group) {
            return hasCommands(group);
        });
    }

    bool PageView::hasCommands(const TableGroup& group)
    {
        const bool inRows = std::ranges::any_of(group.rows, [](const TableRow& row) {
            return row.command.has_value();
        });
        return inRows || std::ranges::any_of(group.groups, [](const TableGroup& nested) {
            return hasCommands(nested);
        });
    }

    bool PageView::writesHints(const TableGroup& group) const
    {
        const bool inRows = std::ranges::any_of(group.rows, [this](const TableRow& row) {
            return writesHint(row.site);
        });
        return inRows || std::ranges::any_of(group.groups, [this](const TableGroup& nested) {
            return writesHints(nested);
        });
    }

    bool PageView::writesHint(const std::optional<CommentSite>& site) const
    {
        return m_writer && site.has_value() && m_writer->writesHint(*site);
    }

    std::wstring PageView::footnoteAnchor(const std::size_t index)
    {
        return L"n" + std::to_wstring(index);
    }

    void PageView::writeLabel(Text& text, const TableGroup& group)
    {
        if (group.pageKind.has_value())
        {
            const std::optional<RowIcon> icon = iconOf(*group.pageKind, nullptr);
            if (icon.has_value())
                text << rowIcon(*icon) << k_cellIconGap;
        }
        writeRuns(text, group.label);
    }

    // The empty cells a hint takes are the ones right before it, and never the first column,
    // which names the row.
    TagValue PageView::hintSpanColumn(const TableRow& row, const bool takesEmptyCells)
    {
        if (!takesEmptyCells || row.cells.size() < 3)
            return k_noColumn;
        const std::size_t lastCell = row.cells.size() - 1;
        std::size_t from = lastCell;
        while (from > 1 && row.cells[from - 1].empty())
            --from;
        if (from == lastCell)
            return k_noColumn;
        return acrossColumn(from);
    }

    TagValue PageView::acrossColumn(const std::size_t from)
    {
        return k_acrossColumn + from;
    }

    bool PageView::isAcrossColumn(const TagValue column)
    {
        return column > k_acrossColumn && column < k_commandColumn;
    }
}
