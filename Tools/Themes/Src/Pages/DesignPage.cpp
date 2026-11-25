module Themes_App.DesignPage;

import Themes_App.ApplyToControl;
import Themes_App.Consts;
import Themes_App.ElementPage;
import Themes_App.PageKinds;
import Themes_App.PreviewPage;
import Themes_App.RuleSlider;

import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Tree;

import ClaFi.Icons.PlusMark;
import ClaFi.Icons.RowIcons;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace RowIcons = ::ClaFi::Icons::RowIcons;

    namespace
    {
        constexpr std::wstring_view k_elementsAnchor{ L"Elements" };
        constexpr std::wstring_view k_newStyleName{ L"New element" };
        constexpr std::array k_selectedTextOutputs{ PaintChannel::Surface, PaintChannel::Text };
        constexpr std::array k_foundTextOutputs{ PaintChannel::Surface };

        struct StylePreview
        {
            std::wstring_view name{};
            PreviewPage page{};
        };

        constexpr StylePreview k_stylePreviews[]{
            { L"AnyWindow", PreviewPage::Windows },
            { L"Dialog", PreviewPage::Windows },
            { L"DialogTitle", PreviewPage::Windows },
            { L"Menu", PreviewPage::Windows },
            { L"Tooltip", PreviewPage::Windows },
            { L"Dropdown", PreviewPage::Windows },
            { L"Page", PreviewPage::Surfaces },
            { L"Section", PreviewPage::Surfaces },
            { L"SectionHeader", PreviewPage::Surfaces },
            { L"Expander", PreviewPage::Surfaces },
            { L"Divider", PreviewPage::Surfaces },
            { L"ToolBar", PreviewPage::Surfaces },
            { L"Field", PreviewPage::Inputs },
            { L"AnyButton", PreviewPage::Inputs },
            { L"Button", PreviewPage::Inputs },
            { L"ToolButton", PreviewPage::Inputs },
            { L"TextBox", PreviewPage::Inputs },
            { L"Slider", PreviewPage::Inputs },
            { L"Grid", PreviewPage::Grid },
            { L"GridHeader", PreviewPage::Grid },
            { L"GridRow", PreviewPage::Grid },
            { L"GridCell", PreviewPage::Grid },
            { L"AnyIndicator", PreviewPage::Selection },
            { L"SelectionIndicator", PreviewPage::Selection },
            { L"HoverIndicator", PreviewPage::Selection },
            { L"ListItem", PreviewPage::Selection },
            { L"SelectedText", PreviewPage::Selection },
            { L"FoundText", PreviewPage::Selection },
            { L"Tab", PreviewPage::Tabs },
            { L"ScrollBar", PreviewPage::Scrolling },
            { L"ScrollButton", PreviewPage::Scrolling },
            { L"ScrollThumb", PreviewPage::Scrolling },
            { L"AnyCallout", PreviewPage::Callouts },
            { L"InfoCallout", PreviewPage::Callouts },
            { L"QuestionCallout", PreviewPage::Callouts },
            { L"WarningCallout", PreviewPage::Callouts },
            { L"ErrorCallout", PreviewPage::Callouts },
            { L"OkCallout", PreviewPage::Callouts },
            { L"DoDont", PreviewPage::DoDont },
            { L"Do", PreviewPage::DoDont },
            { L"Dont", PreviewPage::DoDont },
            { L"CodeBlock", PreviewPage::CodeBlock },
            { k_testeeName, PreviewPage::TestSubjects },
            { k_besteeName, PreviewPage::TestSubjects }
        };

        template<typename Row>
        class EditableElementRow : public WithInPlaceEdit<Row>
        {
        public:
            using WithInPlaceEdit<Row>::WithInPlaceEdit;
        public:
            void setStyleName(std::wstring_view name) { m_styleName = name; }
        protected:
            void getEditorText(Text&) const override;
        private:
            std::wstring m_styleName{};
        };

        template<typename Row>
        void EditableElementRow<Row>::getEditorText(Text& text) const
        {
            text << m_styleName;
        }

        using ElementItem = EditableElementRow<TreeItem>;
        using ElementHeader = EditableElementRow<TreeNodeHeader>;

        [[nodiscard]] Text rowText(const PageKind kind, const std::wstring_view name)
        {
            return Text{
                pageIcon(kind, RowIcons::Size),
                Space{ RowIcons::Gap },
                name
            };
        }

        [[nodiscard]] PaintChannels outputsOf(const std::wstring_view style)
        {
            if (style == L"SelectedText")
                return k_selectedTextOutputs;
            if (style == L"FoundText")
                return k_foundTextOutputs;
            return {};
        }
    }

    std::wstring_view DesignPage::pickedPage() const
    {
        if (const TreeEntry* entry = pickedEntry())
            return entry->anchor;
        return {};
    }

    NodePreview DesignPage::pickedPreview() const
    {
        const TreeEntry* entry = pickedEntry();
        if (!entry || !entry->elementPage)
            return {};

        const auto preview = std::ranges::find(k_stylePreviews, entry->name, &StylePreview::name);
        if (preview == std::end(k_stylePreviews))
        {
            return { .page = PreviewPage::StyleStates, .styleName = entry->name };
        }
        return { .page = preview->page, .showSizesNote = true };
    }

    void DesignPage::pickPage(const std::wstring_view anchor)
    {
        auto found = std::ranges::find(m_entries, anchor, &TreeEntry::anchor);
        if (found == m_entries.end() && !anchor.empty() && !anchor.starts_with(k_stylePrefix))
            found = std::ranges::find(m_entries, styleAnchor(anchor), &TreeEntry::anchor);
        if (found == m_entries.end())
            found = m_entries.begin() + (anchor.empty() ? 0ull : k_rootIndex);

        for (const TreeEntry& entry : m_entries | std::views::drop(k_rootIndex))
        {
            if (entry.node && entry.node->rows().containsNested(found->row))
                entry.node->header().setExpanded(true);
        }
        if (m_tree.currentItem() != found->row)
            m_tree.setCurrentItem(found->row);
        showPickedPage();
    }

    void DesignPage::showPlace(const DesignPlace& place)
    {
        const TreeEntry* before = pickedEntry();
        pickPage(place.page);
        const TreeEntry* entry = pickedEntry();
        if (!entry)
            return;
        const TakeFocus takeFocus = visible() ? TakeFocus::Yes : TakeFocus::No;
        if (entry != before && takeFocus == TakeFocus::Yes)
            entry->row->setFocus();
        if (entry->elementPage)
            entry->elementPage->select(place.selection, takeFocus);
    }

    void DesignPage::bind(AppTheme& theme, const OnGetElementRuleBase& ruleBase,
        const OnRulesChanged& onRulesChanged, const OnFollowElementLink& onFollowElementLink)
    {
        m_moveTimer.stop();
        m_colors = &theme.colors;
        m_styles = &theme.styles;
        m_ruleBase = ruleBase;
        m_onRulesChanged = onRulesChanged;
        m_onFollowElementLink = onFollowElementLink;
        rebuildTree(std::wstring{ pickedPage() });
    }

    void DesignPage::rebuildRules()
    {
        const bool sameTree = std::ranges::equal(m_links, m_styles->list,
            [](const ElementLink& link, const ControlStyle& style) {
                return link.name == style.name && link.base == style.base;
            });
        if (!sameTree)
        {
            rebuildTree(std::wstring{ pickedPage() });
            return;
        }
        for (const TreeEntry& entry : m_entries)
        {
            if (entry.elementPage)
                entry.elementPage->rebuild();
        }
    }

    void DesignPage::visibilityChanged()
    {
        if (visible() && m_tree.currentItem())
            m_tree.currentItem()->scrollIntoViewOnAlign();
    }

    void DesignPage::buildTree()
    {
        constexpr std::wstring_view k_paletteTitle{ L"Palette" };
        writePageTitle(m_paletteTitle.text(), PageKind::Palette, k_paletteTitle);

        TreeItem& palette = m_tree.addItem(rowText(PageKind::Palette, k_paletteTitle));
        addEntry({
            .anchor = std::wstring{ k_paletteToken },
            .row = &palette,
            .page = &m_palettePage
        });

        HeaderText rootText{};
        rootText << rowText(PageKind::Folder, k_elementsAnchor);
        TreeNode& root = m_tree.addNode(std::move(rootText));
        addEntry({
            .anchor = std::wstring{ k_elementsAnchor },
            .row = &root.header(),
            .page = &m_elementsPage,
            .node = &root
        });

        Panel& topBar = m_elementsPage.createTopBar<Panel>(Padding{ 12.0f, 8.0f });
        Label& title = topBar.createBody<Label>(VerticalTextAnchor::Center);
        writePageTitle(title.text(), PageKind::Folder, k_elementsAnchor);
        ToolButton& addButton = topBar.createRightBar<ToolButton>(
            IconSize{ 18.0f },
            ButtonViewMode::IconOnly,
            OnDrawIcon{ Icons::PlusMark::draw },
            L"Add root element"
        );
        addButton.onClick([this](ClickEvent&) {
            addStyle({});
        });
        m_elementsPage.createBody<Label>(
            VerticalAlign::Top,
            Padding{ 12.0f, 8.0f },
            Text{ L"Drop on a highlighted row to make it the Base. "
                L"Drop at a line to insert the element at that level." }
        );
        pickPage(k_paletteToken);
    }

    void DesignPage::rebuildTree(std::wstring anchor, const TakeFocus takeFocus)
    {
        m_rebuildingTree = true;
        if (const TreeEntry* entry = pickedEntry(); entry && entry->elementPage)
        {
            m_tree.setCurrentItem(nullptr);
            m_pages.setCurrentItem(m_elementsPage);
        }

        std::set<std::wstring> collapsed{};
        for (const TreeEntry& entry : m_entries | std::views::drop(k_rootIndex + 1ull))
        {
            if (entry.node && !entry.node->header().expanded())
                collapsed.insert(entry.name);
            entry.page->deleteSelf();
        }
        TreeNode& root = *m_entries[k_rootIndex].node;
        while (!root.rows().controls().empty())
            root.rows().controls().back()->deleteSelf();
        m_entries.resize(k_rootIndex + 1ull);

        m_links.clear();
        Children children{};
        ShownElements names{};
        for (const ControlStyle& style : m_styles->list)
        {
            m_links.push_back({ .name = style.name, .base = style.base });
            if (!style.name.empty())
                names.insert(style.name);
        }
        for (const ControlStyle& style : m_styles->list)
        {
            if (style.name.empty())
                continue;
            const std::wstring_view parent = names.contains(style.base) ? style.base : std::wstring_view{};
            children[parent].push_back(&style);
        }

        ShownElements shown{};
        for (const ControlStyle* style : children[{}])
            addElementRow(*style, root, children, shown, collapsed);
        // A cycle in a loaded file has no root. Show it once, starting at its first element.
        for (const ControlStyle& style : m_styles->list)
        {
            if (!style.name.empty())
                addElementRow(style, root, children, shown, collapsed);
        }

        for (const TreeEntry& entry : m_entries | std::views::drop(k_rootIndex + 1ull))
            bindPage(entry);
        m_rebuildingTree = false;
        pickPage(anchor);
        if (takeFocus == TakeFocus::Yes)
        {
            m_tree.currentItem()->setFocus();
        }
        form().invalidateAlign();
    }

    void DesignPage::addElementRow(const ControlStyle& style, TreeNode& parent,
        const Children& children, ShownElements& shown, const std::set<std::wstring>& collapsed)
    {
        if (!shown.insert(style.name).second)
            return;

        const auto descendants = children.find(style.name);
        const bool hasChildren = descendants != children.end()
            && !std::ranges::all_of(descendants->second, [&shown](const ControlStyle* child) {
                return shown.contains(child->name);
            });
        RichControl* row = nullptr;
        TreeNode* node = nullptr;
        const auto rename = [this, name = style.name](AcceptEditEvent& event) {
            renameStyle(name, event);
        };
        if (hasChildren)
        {
            HeaderText headerText{};
            headerText << rowText(PageKind::Style, style.name);
            node = &parent.addNode(std::type_identity<ElementHeader>{}, std::move(headerText));
            auto& header = static_cast<ElementHeader&>(node->header());
            header.setStyleName(style.name);
            header.onAcceptEdit(rename);
            row = &header;
        }
        else
        {
            ElementItem& item = parent.addItem(std::type_identity<ElementItem>{},
                rowText(PageKind::Style, style.name));
            item.setStyleName(style.name);
            item.onAcceptEdit(rename);
            row = &item;
        }

        ElementPage& page = m_pages.add<ElementPage>(style.name);
        addEntry({
            .anchor = styleAnchor(style.name),
            .name = style.name,
            .row = row,
            .page = &page,
            .node = node,
            .elementPage = &page
        });
        if (node)
        {
            for (const ControlStyle* child : descendants->second)
                addElementRow(*child, *node, children, shown, collapsed);
            if (collapsed.contains(style.name))
                node->header().setExpanded(false);
        }
    }

    void DesignPage::addEntry(TreeEntry entry)
    {
        m_entries.push_back(std::move(entry));
    }

    void DesignPage::bindPage(const TreeEntry& entry)
    {
        const std::wstring& name = entry.name;
        entry.elementPage->bind(PageBinding{
            .styles = m_styles,
            .style = name,
            .defaults = m_defaultStyles.of(name),
            .outputs = outputsOf(name),
            .colors = m_colors,
            .ruleBase = [this, name](const ColorRule& rule, const RuleChannel channel) {
                return m_ruleBase(name, rule, channel);
            },
            .styleNames = [this]() {
                return styleNames();
            },
            .onRulesChanged = [this, name](const EditPhase phase, const Text& what,
                const RuleSelection& at) {
                if (phase == EditPhase::Settled)
                    rebuildPagesBasedOn(name);
                if (m_onRulesChanged)
                    m_onRulesChanged(phase, what, at);
            },
            .onAddChild = [this, name]() {
                addStyle(name);
            },
            .onDeleteStyle = [this, name]() {
                deleteStyle(name);
            },
            .onFollowElementLink = m_onFollowElementLink
        });
    }

    void DesignPage::rebuildPagesBasedOn(const std::wstring_view style)
    {
        for (const TreeEntry& entry : m_entries)
        {
            if (!entry.elementPage)
                continue;
            const StyleChain chain = m_styles->chainOf(entry.name);
            const bool inheritsStyle = std::ranges::any_of(chain | std::views::drop(1),
                [style](const ControlStyle* base) {
                    return base->name == style;
                });
            if (inheritsStyle)
            {
                const RuleSelection selection = entry.elementPage->selection();
                entry.elementPage->rebuild();
                entry.elementPage->select(selection, TakeFocus::No);
            }
        }
    }

    SetNames DesignPage::styleNames() const
    {
        SetNames names{};
        for (const TreeEntry& entry : m_entries)
        {
            if (entry.elementPage)
                names.push_back({ .token = entry.name, .name = entry.name });
        }
        return names;
    }

    const DesignPage::TreeEntry* DesignPage::pickedEntry() const
    {
        const auto found = std::ranges::find(m_entries, m_tree.currentItem(), &TreeEntry::row);
        return found == m_entries.end() ? nullptr : &*found;
    }

    const DesignPage::TreeEntry* DesignPage::entryAt(const Control* control) const
    {
        while (control && control != &m_tree)
        {
            const auto found = std::ranges::find(m_entries, control, &TreeEntry::row);
            if (found != m_entries.end())
                return &*found;
            control = control->parent();
        }
        return nullptr;
    }

    void DesignPage::showPickedPage()
    {
        if (const TreeEntry* entry = pickedEntry())
            m_pages.setCurrentItem(entry->page);
    }

    void DesignPage::addStyle(const std::wstring_view base)
    {
        m_newStyleBase = base;
        m_nameTimer.start(MilliSeconds{ 0u });
    }

    void DesignPage::nameAddedStyle()
    {
        std::wstring base = std::exchange(m_newStyleBase, {});
        if (!base.empty() && !m_styles->of(base))
            return;

        std::wstring name{ k_newStyleName };
        for (std::size_t count = 2ull; m_styles->of(name); ++count)
            name = std::format(L"{} {}", k_newStyleName, count);
        m_styles->add(ControlStyle{ .name = name, .base = std::move(base) });
        rebuildTree(styleAnchor(name), TakeFocus::Yes);
        setsChanged(Text{ L"Add element ", ColoredText{ InkWell::accentInk(), name } });
        if (const TreeEntry* entry = pickedEntry())
        {
            if (entry->node)
                static_cast<ElementHeader*>(entry->row)->openEditor();
            else if (entry->elementPage)
                static_cast<ElementItem*>(entry->row)->openEditor();
        }
    }

    void DesignPage::renameStyle(const std::wstring_view name, AcceptEditEvent& event)
    {
        std::wstring_view typed = event.text.plainText();
        trimLeft(typed);
        trimRight(typed);
        if (typed == name)
            return;
        if (typed.empty())
        {
            event.refuse(L"An element needs a name.");
            return;
        }
        if (m_styles->of(typed))
        {
            event.refuse(L"The theme already has an element with that name.");
            return;
        }
        m_styleToRename = name;
        m_newName = typed;
        m_renameTimer.start(MilliSeconds{ 0u });
    }

    void DesignPage::renamePendingStyle()
    {
        if (InPlaceEdit::isRunning())
        {
            m_renameTimer.start(MilliSeconds{ 0u });
            return;
        }
        const std::wstring before = std::exchange(m_styleToRename, {});
        const std::wstring after = std::exchange(m_newName, {});
        if (!m_styles->of(before) || m_styles->of(after))
            return;
        for (ControlStyle& style : m_styles->list)
        {
            if (style.name == before)
                style.name = after;
            if (style.base == before)
                style.base = after;
            if (style.standsOn == before)
                style.standsOn = after;
        }
        rebuildTree(styleAnchor(after), TakeFocus::Yes);
        setsChanged(Text{
            L"Rename element ", ColoredText{ InkWell::accentInk(), before },
            L" to ", ColoredText{ InkWell::accentInk(), after }
        });
    }

    void DesignPage::deleteStyle(const std::wstring_view name)
    {
        m_styleToDelete = name;
        m_deleteTimer.start(MilliSeconds{ 0u });
    }

    void DesignPage::deletePendingStyle()
    {
        const std::wstring name = std::exchange(m_styleToDelete, {});
        const ControlStyle* deleted = m_styles->of(name);
        if (!deleted)
            return;

        const std::wstring parent = canMoveStyle(name, deleted->base) ? deleted->base : std::wstring{};
        for (ControlStyle& style : m_styles->list)
        {
            if (style.base == name)
                style.base = parent;
            if (style.standsOn == name)
                style.standsOn.clear();
        }
        m_styles->remove(name);
        rebuildTree(parent.empty() ? std::wstring{ k_elementsAnchor } : styleAnchor(parent), TakeFocus::Yes);
        setsChanged(Text{ L"Delete element ", ColoredText{ InkWell::accentInk(), name } });
    }

    void DesignPage::queryDrop(CanDropItemsEvent& event) const
    {
        const TreeEntry* source = entryAt(event.items.front());
        const TreeEntry* target = entryAt(&event.target);
        event.allowed = source && target && acceptsDrop(source->name, *target, event.zone);
        if (event.allowed && event.zone == DropZone::Inside)
        {
            if (target->elementPage)
            {
                event.hint << L"Move inside " << target->name;
            }
            else
            {
                event.hint << L"Move to root";
            }
        }
    }

    void DesignPage::dropElements(const ItemsDropEvent& event)
    {
        const TreeEntry* source = entryAt(event.items.front());
        const TreeEntry* target = entryAt(&event.target);
        if (source && target)
        {
            m_styleToMove = source->name;
            m_targetAnchor = target->anchor;
            m_dropZone = event.zone;
            m_moveTimer.start(MilliSeconds{ 0u });
        }
    }

    bool DesignPage::acceptsDrop(const std::wstring_view source, const TreeEntry& target,
        const DropZone zone) const
    {
        if (source == k_rootStyle || !m_styles->of(source))
        {
            return false;
        }
        if (!target.elementPage)
        {
            return target.anchor == k_elementsAnchor && zone == DropZone::Inside && !m_styles->of(k_rootStyle);
        }
        const ControlStyle* style = m_styles->of(target.name);
        if (!style || !canMoveStyle(source, target.name))
        {
            return false;
        }
        if (zone == DropZone::Inside)
        {
            return true;
        }
        return target.name != k_rootStyle && (m_styles->of(style->base) || !m_styles->of(k_rootStyle));
    }

    bool DesignPage::canMoveStyle(const std::wstring_view name, const std::wstring_view base) const
    {
        if (base.empty())
            return true;
        if (!m_styles->of(base))
            return false;
        const StyleChain chain = m_styles->chainOf(base);
        return std::ranges::none_of(chain, [name](const ControlStyle* style) {
            return style->name == name;
        });
    }

    void DesignPage::movePendingStyle()
    {
        const std::wstring name = std::exchange(m_styleToMove, {});
        const std::wstring anchor = std::exchange(m_targetAnchor, {});
        const auto target = std::ranges::find(m_entries, anchor, &TreeEntry::anchor);
        if (target == m_entries.end() || !acceptsDrop(name, *target, m_dropZone))
        {
            return;
        }
        ControlStyleList& styles = m_styles->list;
        const auto source = std::ranges::find(styles, name, &ControlStyle::name);
        const auto neighbor = std::ranges::find(styles, target->name, &ControlStyle::name);
        const bool inside = m_dropZone == DropZone::Inside;
        const std::wstring base = inside ? target->name : neighbor->base;
        auto position = inside ? styles.end() : neighbor + (m_dropZone == DropZone::After ? 1 : 0);
        if (source->base == base)
        {
            const auto first = source < position ? source + 1 : position;
            const auto last = source < position ? position : source;
            if (std::none_of(first, last, [&base](const ControlStyle& sibling) {
                return sibling.base == base;
            }))
            {
                return;
            }
        }
        source->base = base;
        if (source < position)
        {
            std::rotate(source, source + 1, position);
        }
        else
        {
            std::rotate(position, source, source + 1);
        }
        Text what{ L"Move element ", ColoredText{ InkWell::accentInk(), name } };
        if (target->name.empty())
        {
            what << L" to the root";
        }
        else
        {
            const std::wstring_view relation = inside ? L" under "
                : m_dropZone == DropZone::Before ? L" before " : L" after ";
            what << relation << ColoredText{ InkWell::accentInk(), target->name };
        }
        rebuildTree(styleAnchor(name), TakeFocus::Yes);
        setsChanged(what);
    }

    void DesignPage::setsChanged(const Text& what)
    {
        if (m_onRulesChanged)
            m_onRulesChanged(EditPhase::Settled, what, RuleSelection{});
    }
}
