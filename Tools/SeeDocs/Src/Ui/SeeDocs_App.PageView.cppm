export module SeeDocs_App.PageView;

import SeeDocs_App.BasesBar;
import SeeDocs_App.Editing;
import SeeDocs_App.Icons;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Tree;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Searcher;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // What a link to an anchor of the page opens with, ahead of the anchor's name.
    export constexpr std::wstring_view k_anchorPrefix = L"#";

    // Whether the control and every holder up to the host are shown.
    export [[nodiscard]] bool shownWithin(const Control&, const Control& host);

    // A link on the page was clicked. The surface has an Event of its own.
    export class PageLinkEvent : public ::ClaFi::Event
    {
    public:
        explicit PageLinkEvent(std::wstring target);
    public:
        // A type's qualified name, a module's name, a folder's path, a member link, or # and an
        // anchor of the page.
        const std::wstring target;
    };

    // A command's button on the page was clicked. The surface has an Event of its own.
    export class PageCommandEvent : public ::ClaFi::Event
    {
    public:
        explicit PageCommandEvent(std::wstring target);
    public:
        const std::wstring target;   // what the command is about, as the page was given it
    };

    // The rows of a rearrangeable table were carried to another order. The surface has an Event
    // of its own.
    export class PageOrderEvent : public ::ClaFi::Event
    {
    public:
        explicit PageOrderEvent(std::vector<std::wstring> targets);
    public:
        const std::vector<std::wstring> targets;   // the rows' first links, as the rows stand
    };

    // A fact's Open in editor button was clicked. The surface has an Event of its own.
    export class PageSourceEvent : public ::ClaFi::Event
    {
    public:
        explicit PageSourceEvent(const SourcePlace&);
    public:
        const SourcePlace source;   // the file and line the fact names
    };

    constexpr float k_pagePadding = 24.0f;   // at the sides of a page's parts
    constexpr float k_headPadding = 12.0f;   // above and below the title, and over the body

    // The bar a page's title stands in, which stays while the page scrolls: the mark of what
    // the page is, large, then the title with its scope before it and its badges after it, the
    // title's lines under it and a class's bases under those as crumbs, with the page's commands
    // at its right end. Every page of the browser wears one, whatever it is made of.
    export class PageHead : public Panel
    {
    public:
        template<typename... Args>
        explicit PageHead(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] TextBox& titleBox() { return m_title; }
        [[nodiscard]] BasesBar& basesBar() { return m_bases; }
        [[nodiscard]] StackPanel& commandBar() { return m_commands; }   // hidden at first
        void write(const Page&);
        void write(std::optional<RowIcon>, std::wstring_view title);
        void clear();
    private:
        static void writeIcon(Text&, std::optional<RowIcon>);
    private:
        static constexpr float k_iconGap = 8.0f;      // between the icon and the title
        static constexpr float k_basesIndent = 24.0f;   // of what stands under the title
        static constexpr float k_commandGap = 10.0f;    // before and between the buttons

        StackPanel& m_lines{ createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 0.0f },
            Spacing{ 0.0f }
        ) };
        TextBox& m_title{ m_lines.add<TextBox>(ReadOnly::Yes, Padding{ 0.0f }) };
        BasesBar& m_bases{ m_lines.add<BasesBar>(Padding{ k_basesIndent, 0.0f }) };
        StackPanel& m_commands{ createRightBar<StackPanel>(
            Orientation::Horizontal,
            Padding{ 0.0f },
            Spacing{ k_commandGap },
            VerticalAlign::Center
        ) };
    };

    // A page as controls: its head, with the page's commands as buttons at its right end, and
    // under it, scrolling, its lead, its facts, its own links and an expander per section holding
    // the section's prose, tree or grid - or a method page's declarations under their access
    // labels, as code. Its hints and footnotes are edited where they stand. See the README beside
    // the project.
    export class PageView : public Panel
    {
    public:
        // The searcher is the window's: every page shown bands its words.
        template<typename... Args>
        explicit PageView(const CreateParams&, const Searcher&, Args&&...);
    public:
        // Connects a handler raised when a link on the page is clicked.
        template<typename F>
        EventConnection onPageLink(F&& callback);
        // Connects a handler raised when a command's button on the page is clicked.
        template<typename F>
        EventConnection onPageCommand(F&& callback);
        // Connects a handler raised when a rearrangeable table's rows were carried about.
        template<typename F>
        EventConnection onPageOrder(F&& callback);
        // Connects a handler raised when a fact's Open in editor button is clicked.
        template<typename F>
        EventConnection onPageSource(F&& callback);
        // What the page writes edited words back through. A page with none shows them only.
        void setWriter(IPageWriter&);
        // Builds the page's controls, the page kept for as long as they stand.
        void show(Page);
        // Takes the page built again over the surface as it now reads. Where it has the shape of
        // the page shown - the same sections, rows and footnotes - its words go into the controls
        // standing, which keep their place; false where it has not, and nothing changes.
        [[nodiscard]] bool refresh(Page);
        // Shows SeeDocs' own words in place of a page.
        void showText(const Text&);
        // Scrolls to the footnote the anchor names, or else to the row of a member of that name,
        // its section opened and the row selected. Answers where the page opens: what the anchor
        // names, else the page itself.
        Control& showAnchor(std::wstring_view anchor);
        // The anchor of the footnote that section's words stand in; empty where none on the page.
        [[nodiscard]] std::wstring anchorOf(const Reference&) const;
        // Focuses the current match's control, where it takes the focus; false where none does.
        bool focusFound();
        // Focuses the current match's control, else the row the anchor selected, else the first.
        void focusContent();
        // What the page holds of the window's search.
        [[nodiscard]] SearchResult& found() { return m_found; }
        // The link a menu is up for, as the text states it; empty while no link menu is up.
        [[nodiscard]] std::wstring_view menuLink() const { return m_menuLink; }
    private:
        // A footnote standing on the page: its slot, the words it shows, the anchor a link reaches
        // it by - none for the type's own note - and the row it belongs to where it is a member's.
        struct PageFootnote
        {
            FootnoteSlot* slot{ nullptr };
            const Excerpt* excerpt{ nullptr };
            std::wstring anchor{};
            const TableRow* row{ nullptr };
        };
        // A row an anchor names: the section's expander, the grid the row stands in, and the row.
        struct RowAnchor
        {
            Expander* expander{ nullptr };
            Grids::GridBase* grid{ nullptr };
            Grids::Row* row{ nullptr };
        };
        // Writes one box's words out of the page again.
        using Render = std::function<void()>;
        using PageFootnotes = std::vector<PageFootnote>;
        // The first row of each name on the page, by the anchor that names it.
        using RowAnchors = std::map<std::wstring, RowAnchor, std::less<>>;
    private:
        void clear();
        void addHead();
        void addLead();
        void addFacts();
        void addLinks();
        [[nodiscard]] Expander& addSection(const Section&);
        void addProse(Expander&, const Section&);
        void addTree(Expander&, const Branches&, bool nodesOpen);
        void addTable(Expander&, const Table&);
        void addSignatures(const Section&);
        void addCommands();
        // Under the tree or a node of it: a branch with nothing under it is an item, any other
        // a node whose rows are built as it opens; a click on either opens the type it names.
        template<typename Holder>
        void addTreeRows(Holder&, const Branches&, bool nodesOpen);
        [[nodiscard]] Text treeRowText(const Branch&) const;
        [[nodiscard]] HeaderText treeHeaderText(const Branch&) const;
        // The column at the end holding the rows' buttons; null where no row has a command.
        void addTableRows(Expander&, Grids::GridBase&, const Table&, const TableGroup&,
            StackPanel& notes, TagValue hintColumn, Grids::Column* commandColumn, PageFootnotes&);
        // The button's caption and hint are written out of the command, and its click raises it.
        void connectCommand(Button&, const Command&);
        // Raises PageOrderEvent with the rows' first links, in the order the rows stand in now.
        void reportOrder(Grids::Grid&);
        // The button opens the fact's source, greyed with a hint while no editor is set for it.
        void connectSource(Button&, const Fact&);
        void connectFolder(Button&, const Fact&);   // its click opens the fact's folder
        // The row's hint cell opens on the comment's words and writes them back.
        void connectHintCell(Grids::Row&, const TableRow&, TagValue hintColumn);
        // The row's menu edits its footnote, or adds one where it has none.
        void connectRowMenu(Grids::Row&, const TableRow&, StackPanel& notes);
        // The box takes edits where the page has a writer and the hint a comment to write into.
        void connectHintBox(HintBox&, const std::optional<CommentSite>&);
        FootnoteSlot& addFootnoteSlot(StackPanel& holder, PageFootnote, Padding);
        void editFootnote(const PageFootnote&) const;
        void addFootnote(const TableRow&, StackPanel& notes);
        [[nodiscard]] const PageFootnote* footnoteOf(const TableRow&) const;
        // The anchor of the footnote showing the section in full; empty where none has one.
        [[nodiscard]] std::wstring shownAnchorOf(const Reference&) const;
        [[nodiscard]] bool canAddFootnote(const TableRow&) const;
        // Runs the edit once the command that asked for it has unwound: a menu is still closing
        // over the control the edit takes the place of.
        void later(std::function<void()>);
        void render();
        void connectLinks(Grids::Grid&);
        void connectLinks(TextBox&);
        // Writes the declaration a link's type shows in a hint; false for a link naming none.
        bool writePreview(Text&, std::wstring_view link) const;
        void connectHead();
        void open(std::wstring target);
        void ask(std::wstring target);   // raises the command the target is about
        // Runs the menu for a link - Open and Open in new tab - on the control it was asked on.
        void showLinkMenu(Control& owner, std::wstring target, ContextPopupEvent&);
        [[nodiscard]] static bool hasFootnotes(const Table&);
        [[nodiscard]] static bool hasFootnotes(const TableGroup&);
        [[nodiscard]] static bool hasCommands(const Table&);
        [[nodiscard]] static bool hasCommands(const TableGroup&);
        // Whether a row of the group has a hint the writer writes.
        [[nodiscard]] bool writesHints(const TableGroup&) const;
        [[nodiscard]] bool writesHint(const std::optional<CommentSite>&) const;
        [[nodiscard]] static std::wstring footnoteAnchor(std::size_t index);
    private:
        // The look of every section's expander at the root of a page.
        static constexpr ExpanderViewMode k_expanderViewMode = ExpanderViewMode::Ellipsis;
        static constexpr float k_sectionSpacing = 18.0f;  // between the parts down the page
        static constexpr float k_headerSpacing = 6.0f;    // between a section's header and body
        static constexpr float k_prosePadding = 12.0f;
        static constexpr float k_footnotePadding = 3.0f;   // above and below a member's footnote
        static constexpr float k_footnoteGap = 6.0f;       // between a table's footnotes
        static constexpr float k_signatureGap = 12.0f;     // between declarations
        static constexpr float k_leadGap = 6.0f;           // between a declaration's parts
        static constexpr float k_signatureIndent = 18.0f;  // of the declarations under their label
        static constexpr float k_treePadding = 4.0f;
        static constexpr float k_kindGap = 12.0f;   // the least room before a tree row's kind word
        static constexpr float k_iconGap = 5.0f;    // between a tree row's icon and its text
        static constexpr TagValue k_factLabel = 0;
        static constexpr TagValue k_factValue = 1;
        static constexpr TagValue k_factButton = 2;
        static constexpr TagValue k_factAcross = 3;   // the value and the button's column as one
        static constexpr TagValue k_markColumn = std::numeric_limits<TagValue>::max();
        static constexpr TagValue k_groupColumn = k_markColumn - 1;
        static constexpr TagValue k_noColumn = k_markColumn - 2;
        static constexpr TagValue k_markParentColumn = k_markColumn - 3;
        static constexpr TagValue k_commandColumn = k_markColumn - 4;
        static constexpr std::wstring_view k_mark = L"*";
        static constexpr std::wstring_view k_openFolder = L"Open folder";

        Page m_page{};
        IPageWriter* m_writer{ nullptr };
        std::vector<Control*> m_parts{};   // what the page built, under the head and in it
        std::vector<Render> m_renders{};   // every box the page writes its words into
        PageFootnotes m_footnotes{};
        RowAnchors m_rowAnchors{};
        Grids::Row* m_firstRow{ nullptr };      // the first row a table of the page holds
        Grids::Row* m_anchoredRow{ nullptr };   // the row the anchor shown selected
        // A footnote not written yet, by the row it is for, while its editor stands.
        std::map<const TableRow*, FootnoteSlot*> m_newFootnotes{};
        std::size_t m_footnoteCount{ 0 };   // numbered through the page, so an anchor names one
        std::wstring m_menuLink{};   // the link the menu that is up is about
        std::function<void()> m_pending{};
        UiTimer m_later{};
        std::vector<ScopedEventConnection> m_editorWatches{};   // on the editors' command lines

        PageHead& m_head{ createTopBar<PageHead>() };

        ScrollBoxWith<StackPanel>& m_pageBox{ createBody<ScrollBoxWith<StackPanel>>(
            HostProps{
                ScrollBars::Vertical
            },
            BodyProps{
                Orientation::Vertical,
                Padding{ k_pagePadding, k_headPadding },
                Spacing{ k_sectionSpacing }
            }
        ) };
        SearchResult m_found;   // what the page holds of the window's search
    };


    //-------------------------------------------------------------------------


    // PageHead

    template<typename... Args>
    PageHead::PageHead(const CreateParams& params, Args&&... args)
        :
        Panel{
            params,
            Padding{ k_pagePadding, k_headPadding },
            Spacing{ k_commandGap },
            std::forward<Args>(args)...
        }
    {
        m_commands.setVisible(false);
    }

    // PageView

    template<typename... Args>
    PageView::PageView(const CreateParams& params, const Searcher& searcher, Args&&... args)
        :
        Panel{ params, UiElement::Page, std::forward<Args>(args)... },
        m_found{ searcher, *this }
    {
        Actions::registerAll();
        connectHead();
        m_later.onTick([this](TimerEvent&) {
            const std::function<void()> pending = std::exchange(m_pending, {});
            if (pending)
                pending();
        });
    }

    template<typename F>
    EventConnection PageView::onPageLink(F&& callback)
    {
        return connectEvent<PageLinkEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection PageView::onPageCommand(F&& callback)
    {
        return connectEvent<PageCommandEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection PageView::onPageOrder(F&& callback)
    {
        return connectEvent<PageOrderEvent>(std::forward<F>(callback));
    }

    template<typename F>
    EventConnection PageView::onPageSource(F&& callback)
    {
        return connectEvent<PageSourceEvent>(std::forward<F>(callback));
    }

    template<typename Holder>
    void PageView::addTreeRows(Holder& holder, const Branches& branches, const bool nodesOpen)
    {
        for (const Branch& branch : branches)
        {
            const Type* type = branch.type;
            if (branch.children.empty())
            {
                TreeItem& item = holder.addItem(treeRowText(branch));
                item.onClick([this, type](ClickEvent&) {
                    if (type)
                        open(type->qualifiedName);
                });
                continue;
            }
            Controls::TreeNode& node = holder.addNode(treeHeaderText(branch));
            node.header().onClick([this, type](ClickEvent&) {
                if (type)
                    open(type->qualifiedName);
            });
            if (nodesOpen)
            {
                addTreeRows(node, branch.children, true);
                continue;
            }
            node.header().setExpanded(false);
            node.onFetchRows([this, &branch](FetchRowsEvent& event) {
                addTreeRows(event.node, branch.children, false);
            });
        }
    }
}
