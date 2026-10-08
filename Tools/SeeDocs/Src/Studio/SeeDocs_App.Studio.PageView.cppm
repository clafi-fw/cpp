export module SeeDocs_App.Studio.PageView;

import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.Expander;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.TreeView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A link on the page named a type or a module to open. The surface has an Event of its own.
    export class OpenNameEvent : public ::ClaFi::Event
    {
    public:
        explicit OpenNameEvent(std::wstring name);
    public:
        const std::wstring name;   // a type's qualified name or a module's name
    };

    // A page as controls: its title and bases in a bar that stays, and under it, scrolling, its
    // lead, its facts and an expander per section holding the section's prose, tree or grid. See
    // the README beside the project.
    export class PageView : public Panel
    {
    public:
        template<typename... Args>
        explicit PageView(const CreateParams&, Args&&...);
    public:
        // Connects a handler raised when a link on the page names a type or a module.
        template<typename F>
        EventConnection onOpenName(F&& callback);
        // Builds the page's controls, the page kept for as long as they stand.
        void show(Page);
        // Shows words of the studio's own in place of a page.
        void showText(const Text&);
    private:
        // Where a row's words go: the footnotes box under the grid, once there is one.
        struct Footnotes
        {
            Text text;
            std::size_t count{ 0 };
        };
    private:
        void clear();
        void connectHeadLinks();
        void addHead();
        void addLead();
        void addFacts();
        [[nodiscard]] Expander& addSection(const Section&);
        void addProse(Expander&, const Excerpt&);
        void addTree(Expander&, const Branches&);
        void addTable(Expander&, const Table&);
        // Under the tree or a node of it: a branch with nothing under it is an item, any other
        // a node, open; a click on either opens the type the branch names.
        template<typename Holder>
        void addTreeRows(Holder&, const Branches&);
        [[nodiscard]] Text treeRowText(const Branch&) const;
        [[nodiscard]] HeaderText treeHeaderText(const Branch&) const;
        void addTableRows(Grids::GridBase&, const Table&, const TableGroup&, Footnotes&);
        void connectLinks(Grids::Grid&, TextBox* footnotes);
        void open(std::wstring name);
        [[nodiscard]] static bool hasFootnotes(const Table&);
        [[nodiscard]] static std::wstring footnoteAnchor(std::size_t index);
    private:
        static constexpr float k_pagePadding = 24.0f;
        static constexpr float k_headPadding = 12.0f;   // above and below the title
        static constexpr float k_basesIndent = 24.0f;   // of the lines of bases under the title
        static constexpr float k_sectionSpacing = 18.0f;  // between the parts down the page
        static constexpr float k_headerSpacing = 6.0f;    // between a section's header and body
        static constexpr float k_prosePadding = 12.0f;
        static constexpr float k_treePadding = 4.0f;
        static constexpr float k_kindGap = 12.0f;   // the least room before a tree row's kind word
        static constexpr float k_iconGap = 5.0f;    // between a tree row's icon and its text
        static constexpr TagValue k_factLabel = 0;
        static constexpr TagValue k_factValue = 1;
        static constexpr TagValue k_markColumn = std::numeric_limits<TagValue>::max();
        static constexpr TagValue k_groupColumn = k_markColumn - 1;
        static constexpr std::wstring_view k_mark = L"*";
        static constexpr std::wstring_view k_anchorPrefix = L"#";

        Page m_page{};
        std::vector<Control*> m_parts{};   // what the page stands as under the title, in order

        TextBox& m_head{ createTopBar<TextBox>(
            ReadOnly::Yes,
            Padding{ k_pagePadding, k_headPadding }
        ) };

        ScrollBoxWith<Stack>& m_pageBox{ createBody<ScrollBoxWith<Stack>>(
            HostProps{
                ScrollBars::Vertical
            },
            BodyProps{
                Orientation::Vertical,
                Padding{ k_pagePadding, k_headPadding },
                Spacing{ k_sectionSpacing }
            }
        ) };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    PageView::PageView(const CreateParams& params, Args&&... args)
        :
        Panel{ params, UiElement::Page, std::forward<Args>(args)... }
    {
        connectHeadLinks();
    }

    template<typename F>
    EventConnection PageView::onOpenName(F&& callback)
    {
        return connectEvent<OpenNameEvent>(std::forward<F>(callback));
    }

    template<typename Holder>
    void PageView::addTreeRows(Holder& holder, const Branches& branches)
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
            addTreeRows(node, branch.children);
            node.header().setExpanded(true);
            node.header().onClick([this, type](ClickEvent&) {
                if (type)
                    open(type->qualifiedName);
            });
        }
    }
}
