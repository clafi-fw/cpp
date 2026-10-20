module SeeDocs_App.SidebarEditor;

import SeeDocs_App.Icons;
import SeeDocs_App.Project;
import SeeDocs_App.Surface;

import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.Menu;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.Panel;
import ClaFi.Controls.RadioButton;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.SplitButton;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Tree;
import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    namespace
    {
        constexpr float k_iconGap = 5.0f;   // between a row's mark and its text
        constexpr float k_markRoom = k_rowIconSize + k_iconGap;   // what a row's mark takes
        constexpr std::wstring_view k_addCaption = L"Add";
        constexpr std::wstring_view k_deleteCaption = L"Delete";
        constexpr std::wstring_view k_addTooltip = L"Adds an item under the selected category, "
            L"or after the selected item";
        constexpr std::wstring_view k_deleteTooltip = L"Takes the selected item out, with "
            L"everything under it";
        constexpr std::wstring_view k_deleteTitle = L"Delete category";
        constexpr std::wstring_view k_categoryName = L"New category";
        constexpr std::wstring_view k_contentName = L"New page";
        constexpr std::wstring_view k_apisName = L"All the APIs";
        constexpr wchar_t k_pathSeparator = L'/';   // what a url parts an item's names with

        // The kinds as the Add menu and the kind set list them.
        constexpr std::array<SidebarKind, 4ull> k_kinds{
            SidebarKind::Category,
            SidebarKind::Divider,
            SidebarKind::AllTheApis,
            SidebarKind::Content
        };
    }

    // The mark of the row's kind, painted ahead of the text in room the text leaves it, so an
    // editor opened over the text covers the words alone.
    template<typename Host>
    class WithMark : public Host
    {
    public:
        using Host::Host;
    public:
        void setMark(const RowIcon mark) { m_mark = mark; }
    protected:
        ScaledSize calculateContent(AlignEvent& event) override
        {
            ScaledSize result = Host::calculateContent(event);
            const float room = event.scale(k_markRoom);
            result.x += room;
            if (event.composedMinSize.x > 0.0f)
                event.composedMinSize.x += room;
            return result;
        }

        void adjustTextRect(AdjustTextRectEvent& event) const override
        {
            Host::adjustTextRect(event);
            event.textBounds.left += event.scale(k_markRoom);
        }

        void paintText(PaintEvent& event) override
        {
            if (m_mark.has_value())
            {
                const FloatRect bounds = this->textBounds(event);
                const float size = event.scaler().scale(k_rowIconSize);
                const float left = bounds.left - event.scaler().scale(k_markRoom);
                const float top = bounds.top + (bounds.height() - size) / 2.0f;
                PaintIconEvent icon{
                    event.controlContext(),
                    FloatRect{ left, top, left + size, top + size },
                    event.control().tag(),
                    event.disabledAmount()
                };
                paintRowIcon(icon, *m_mark);
            }
            Host::paintText(event);
        }
    private:
        std::optional<RowIcon> m_mark{};
    };

    // The header of a category's node: its name typed where it stands, its mark ahead of it.
    class SidebarEditor::CategoryHeader : public WithMark<WithInPlaceEdit<TreeNodeHeader>>
    {
    public:
        using WithMark::WithMark;
    };

    // The row of a page or an All the APIs item: its name typed where it stands.
    class SidebarEditor::ItemRow : public WithMark<WithInPlaceEdit<TreeItem>>
    {
    public:
        using WithMark::WithMark;
    };

    // A divider's row: a line across the text slot, which a pick lands on like any row.
    class SidebarEditor::DividerRow : public TreeItem
    {
    public:
        using TreeItem::TreeItem;
    protected:
        void paintText(PaintEvent&) override;
    };

    // Add: a press anywhere on it drops the kinds.
    class SidebarEditor::AddButton : public SplitButton
    {
    public:
        template<typename... Args>
        explicit AddButton(const CreateParams&, SidebarEditor&, Args&&...);
    protected:
        [[nodiscard]] bool dropOnPrimaryPress() const override { return true; }
        void dropdown(DropdownEvent&) override;
    private:
        SidebarEditor& m_editor;
    };

    // SidebarEditEvent

    SidebarEditEvent::SidebarEditEvent(SidebarItems edited)
        :
        ::ClaFi::Event{},
        items{ std::move(edited) }
    {
    }

    // DividerRow

    void SidebarEditor::DividerRow::paintText(PaintEvent& event)
    {
        const FloatRect bounds = textBounds(event);
        const float y = std::round(bounds.center().y);
        event.canvas().drawLine({ bounds.left, y }, { bounds.right, y },
            event.textRgb(InkGrade::Faint), event.scaledStrokeWidth(Thickness::Thin));
    }

    // AddButton

    // A button's metrics and look, as Delete beside it has them, over the tool button's a split
    // button takes.
    template<typename... Args>
    SidebarEditor::AddButton::AddButton(const CreateParams& params, SidebarEditor& editor,
        Args&&... args)
        :
        SplitButton{
            params,
            params.themeMetrics().button,
            UiElement::Button,
            std::forward<Args>(args)...
        },
        m_editor{ editor }
    {
    }

    // A line per kind, its mark ahead of its caption; the one picked is added.
    void SidebarEditor::AddButton::dropdown(DropdownEvent&)
    {
        Menu menu{ *this };
        for (const SidebarKind kind : k_kinds)
        {
            const std::optional<RowIcon> icon = rowIconOf(kind);
            menu.add(
                Text{ kindCaption(kind) },
                MenuItem::OnPaintIcon{ [icon](PaintIconEvent& paint) {
                    if (icon.has_value())
                        paintRowIcon(paint, *icon);
                } },
                MenuItem::OnClick{ [this, kind](ClickEvent&) {
                    m_editor.addItem(kind);
                } }
            );
        }
        menu.executeUnder(*this);
    }

    // SidebarEditor

    TreeNode& SidebarEditor::addRoot(const std::wstring_view projectName)
    {
        return m_tree.addNode(
            ItemsOrder::Rearrangeable,
            HeaderText{ rowIcon(RowIcon::Project), Space{ k_iconGap }, projectName }
        );
    }

    StackPanel& SidebarEditor::addKindSet()
    {
        m_kinds.add<Label>(Text{ k_kindCaption });
        return m_kinds.add<StackPanel>(Orientation::Vertical, Padding{ k_choiceIndent, 0.0f });
    }

    void SidebarEditor::addCommands(StackPanel& bar)
    {
        bar.add<AddButton>(
            *this,
            k_addCaption,
            TooltipText{ k_addTooltip }
        );
        Button& remove = bar.add<Button>(
            k_deleteCaption,
            TooltipText{ k_deleteTooltip }
        );
        remove.onGetState([this](GetStateEvent& event) {
            event.state.enabled = canDelete();
        });
        remove.onClick([this](ClickEvent&) {
            deleteSelected();
        });
        m_deleteButton = &remove;
        bar.setVisible(true);
    }

    // **The rows stand until whatever asked has unwound**: a carry is still unwinding on the row
    // it moved, and a click on a row's menu line on the menu. A tick comes after both.
    void SidebarEditor::reload()
    {
        m_rebuildTimer.start(MilliSeconds{ 0u });
    }

    void SidebarEditor::nestedKeyDown(KeyDownEvent& event)
    {
        if (event.key == Keys::Delete && event.modifiers.empty() && m_tree.isFocused()
            && canDelete())
        {
            deleteSelected();
            event.handled = true;
            return;
        }
        Panel::nestedKeyDown(event);
    }

    void SidebarEditor::connectKinds()
    {
        for (const SidebarKind kind : k_kinds)
            addKind(kind, kindCaption(kind));
        m_kinds.setVisible(false);
    }

    // An answer of the set, chosen while the selected item is of its kind and greyed while the
    // item cannot be; a click makes the item so and redraws the set.
    void SidebarEditor::addKind(const SidebarKind kind, const std::wstring_view caption)
    {
        m_kindSet.add<RadioButton>(
            caption,
            Padding{ 0.0f, m_kindSet.themeMetrics().listItem.padding.y },
            HorizontalAlign::Left,
            RadioButton::OnGetState{ [this, kind](GetStateEvent& event) {
                const SidebarItem* item = selectedItem();
                event.state.selected = item && item->kind == kind;
                event.state.enabled = canBeKind(kind);
            } },
            RadioButton::OnClick{ [this, kind](ClickEvent& event) {
                const SidebarItem* item = selectedItem();
                if (item && item->kind != kind)
                    setKind(kind);
                event.control->invalidateSiblingStates();
            } }
        );
    }

    // A category's node takes the carry, and its header the name; a divider is a row with no
    // name to type.
    void SidebarEditor::buildRows(const SidebarItems& items, TreeNode& holder,
        const IndexPath& under)
    {
        for (std::size_t i = 0; i != items.size(); ++i)
        {
            const SidebarItem& item = items[i];
            IndexPath path = under;
            path.push_back(i);
            Row row{ .path = path };
            const std::optional<RowIcon> mark = rowIconOf(item.kind);
            switch (item.kind)
            {
                case SidebarKind::Category:
                {
                    TreeNode& node = holder.addNode(
                        std::type_identity<CategoryHeader>{},
                        ItemsOrder::Rearrangeable,
                        HeaderText{ item.name }
                    );
                    CategoryHeader& header = static_cast<CategoryHeader&>(node.header());
                    header.setMark(*mark);
                    header.onAcceptEdit([this, path](AcceptEditEvent& event) {
                        acceptName(path, event);
                    });
                    node.rows().onItemsMove([this, path, &node](ItemsMoveEvent&) {
                        itemsMoved(path, node.rows());
                    });
                    row.node = &node;
                    buildRows(item.items, node, path);
                    break;
                }
                case SidebarKind::Divider:
                {
                    row.row = &holder.addItem(std::type_identity<DividerRow>{});
                    break;
                }
                case SidebarKind::AllTheApis:
                case SidebarKind::Content:
                {
                    ItemRow& made = holder.addItem(std::type_identity<ItemRow>{},
                        Text{ item.name });
                    made.setMark(*mark);
                    made.onAcceptEdit([this, path](AcceptEditEvent& event) {
                        acceptName(path, event);
                    });
                    row.row = &made;
                    break;
                }
            }
            m_rows.push_back(std::move(row));
        }
    }

    // The current row is let go before the rows go, so it never names a dead one. The row asked
    // for is selected afterwards, else the one that was, else the project's node; every node
    // stands open, the sidebar being small.
    void SidebarEditor::rebuildRows()
    {
        IndexPath select = m_select.value_or(IndexPath{});
        if (!m_select.has_value())
        {
            if (const Row* selected = selectedRow())
                select = selected->path;
        }
        m_select.reset();
        m_tree.setCurrentItem(nullptr);
        for (const Row& row : m_rows)
        {
            if (row.path.size() != 1ull)
                continue;
            if (row.node)
                row.node->deleteSelf();
            else
                row.row->deleteSelf();
        }
        m_rows.clear();
        buildRows(m_items, m_root, {});
        tagRows();
        m_root.header().setExpanded(true);
        for (const Row& row : m_rows)
        {
            if (row.node)
                row.node->header().setExpanded(true);
        }

        // The nearest row standing on the way to the one asked for, where it has gone.
        const Row* found = nullptr;
        while (!select.empty() && !found)
        {
            found = rowAt(select);
            if (!found)
                select.pop_back();
        }
        Control& current = found
            ? (found->node ? static_cast<Control&>(found->node->header()) : *found->row)
            : m_root.header();
        m_tree.setCurrentItem(current);
        current.scrollIntoViewOnAlign();
        selectionChanged();

        const SidebarItem* item = found ? itemAt(m_items, found->path) : nullptr;
        if (std::exchange(m_renameNext, false) && item)
        {
            if (found->node)
                static_cast<CategoryHeader&>(found->node->header()).openEditor();
            else if (item->kind != SidebarKind::Divider)
                static_cast<ItemRow&>(*found->row).openEditor();
        }
    }

    // **The rows are tagged once they all stand**: a row is where its vector put it, and the
    // vector settles only then. A node and its header carry the row alike, the header being
    // what the tree makes current and the node what its rows stack carries.
    void SidebarEditor::tagRows()
    {
        for (Row& row : m_rows)
        {
            const Tag tag{ &row };
            if (row.node)
            {
                row.node->setTag(tag);
                row.node->header().setTag(tag);
            }
            else
            {
                row.row->setTag(tag);
            }
        }
    }

    const SidebarEditor::Row* SidebarEditor::rowOf(const Control* control)
    {
        return control ? control->tag().get<const Row*>() : nullptr;
    }

    const SidebarEditor::Row* SidebarEditor::selectedRow() const
    {
        return rowOf(m_tree.currentItem());
    }

    const SidebarEditor::Row* SidebarEditor::rowAt(const IndexPath& path) const
    {
        for (const Row& row : m_rows)
        {
            if (row.path == path)
                return &row;
        }
        return nullptr;
    }

    const SidebarItem* SidebarEditor::selectedItem() const
    {
        const Row* row = selectedRow();
        return row ? itemAt(m_items, row->path) : nullptr;
    }

    SidebarItems& SidebarEditor::listAt(SidebarItems& items, const IndexPath& path)
    {
        SidebarItems* list = &items;
        for (std::size_t i = 0; i + 1ull < path.size(); ++i)
            list = &(*list)[path[i]].items;
        return *list;
    }

    const SidebarItem* SidebarEditor::itemAt(const SidebarItems& items, const IndexPath& path)
    {
        const SidebarItems* list = &items;
        for (std::size_t i = 0; i + 1ull < path.size(); ++i)
        {
            if (path[i] >= list->size())
                return nullptr;
            list = &(*list)[path[i]].items;
        }
        if (path.empty() || path.back() >= list->size())
            return nullptr;
        return &(*list)[path.back()];
    }

    std::wstring SidebarEditor::newName(const SidebarKind kind, const SidebarItems& siblings)
    {
        std::wstring_view base;
        switch (kind)
        {
            case SidebarKind::Category:
                base = k_categoryName;
                break;
            case SidebarKind::Content:
                base = k_contentName;
                break;
            case SidebarKind::AllTheApis:
                base = k_apisName;
                break;
            case SidebarKind::Divider:
                return {};
        }
        std::wstring result{ base };
        for (std::size_t number = 2; !refusalOf(result, siblings, nullptr).empty(); ++number)
            result = std::wstring{ base } + L" " + std::to_wstring(number);
        return result;
    }

    // A name is a segment of the item's url, so it holds no slash; and it is what tells the
    // item from the others beside it.
    std::wstring SidebarEditor::refusalOf(const std::wstring_view name,
        const SidebarItems& siblings, const SidebarItem* self)
    {
        if (name.empty())
            return L"The name is empty.";
        if (name.find(k_pathSeparator) != std::wstring_view::npos)
            return L"A name cannot hold a slash.";
        for (const SidebarItem& sibling : siblings)
        {
            if (&sibling != self && sibling.kind != SidebarKind::Divider && sibling.name == name)
                return L"Another item here is named so.";
        }
        return {};
    }

    // **A new item goes where the selection says**: at the end of the selected category, after
    // the selected item, or at the end of the top level with nothing but the project's node
    // selected. It is named by its kind and, a divider aside, opens for its name at once.
    void SidebarEditor::addItem(const SidebarKind kind)
    {
        SidebarItems items = m_items;
        IndexPath at{};
        const Row* selected = selectedRow();
        const SidebarItem* item = selected ? itemAt(items, selected->path) : nullptr;
        if (item)
        {
            at = selected->path;
            if (item->kind == SidebarKind::Category)
                at.push_back(item->items.size());
            else
                ++at.back();
        }
        else
        {
            at.push_back(items.size());
        }
        SidebarItems& list = listAt(items, at);
        list.insert(list.begin() + static_cast<std::ptrdiff_t>(at.back()), SidebarItem{
            .kind = kind,
            .name = newName(kind, list)
        });
        m_select = at;
        m_renameNext = kind != SidebarKind::Divider;
        edit(std::move(items), true);
    }

    // The selection lands on the row that takes the item's place, else on the one before it,
    // else on the category it stood in.
    void SidebarEditor::deleteSelected()
    {
        const Row* selected = selectedRow();
        const SidebarItem* item = selected ? itemAt(m_items, selected->path) : nullptr;
        if (!item)
            return;
        const IndexPath path = selected->path;
        if (!item->items.empty())
        {
            Text message;
            message << L"The category " << item->name << L" holds "
                << std::to_wstring(item->items.size())
                << (item->items.size() == 1ull ? L" item" : L" items")
                << L". Delete it with everything under it?";
            MessageDialog dialog{ *m_tree.currentItem(), k_deleteTitle, message,
                MessageIcon::Question };
            dialog.add(DialogAnswer::Yes);
            dialog.add(DialogAnswer::No);
            if (dialog.execute() != DialogAnswer::Yes)
                return;
        }
        SidebarItems items = m_items;
        SidebarItems& list = listAt(items, path);
        list.erase(list.begin() + static_cast<std::ptrdiff_t>(path.back()));
        IndexPath select = path;
        if (select.back() >= list.size() && select.back() > 0ull)
            --select.back();
        else if (list.empty())
            select.pop_back();
        m_select = select;
        edit(std::move(items), true);
    }

    bool SidebarEditor::canDelete() const
    {
        return selectedItem() != nullptr;
    }

    bool SidebarEditor::canBeKind(const SidebarKind kind) const
    {
        const SidebarItem* item = selectedItem();
        if (!item)
            return false;
        return item->kind == kind || item->items.empty();
    }

    // An item made a divider loses its name, and a divider made anything else is named by its
    // new kind.
    void SidebarEditor::setKind(const SidebarKind kind)
    {
        const Row* selected = selectedRow();
        if (!selected || !itemAt(m_items, selected->path))
            return;
        SidebarItems items = m_items;
        SidebarItems& list = listAt(items, selected->path);
        SidebarItem& item = list[selected->path.back()];
        const bool wasDivider = item.kind == SidebarKind::Divider;
        item.kind = kind;
        if (kind == SidebarKind::Divider)
            item.name.clear();
        else if (wasDivider)
            item.name = newName(kind, list);
        m_select = selected->path;
        edit(std::move(items), true);
    }

    // The row keeps its place, so its words are put right where it stands and no row is built
    // again under the editor.
    void SidebarEditor::acceptName(const IndexPath& path, AcceptEditEvent& event)
    {
        const std::wstring name{ trimmed(event.text.plainText()) };
        if (!itemAt(m_items, path))
            return;
        SidebarItems items = m_items;
        SidebarItems& list = listAt(items, path);
        SidebarItem& item = list[path.back()];
        const std::wstring refusal = refusalOf(name, list, &item);
        if (!refusal.empty())
        {
            event.refuse(refusal);
            return;
        }
        if (name == item.name)
            return;
        item.name = name;
        if (const Row* row = rowAt(path))
        {
            if (row->node)
            {
                ExpanderHeader& header = row->node->header();
                header.text() = HeaderText{ name };
                header.invalidateFormAlign();
            }
            else
            {
                row->row->text() = Text{ name };
                row->row->invalidateFormAlign();
            }
        }
        edit(std::move(items), false);
    }

    // The rows stand in the order carried, so the items are put in the order the rows name, and
    // the selection follows its item to where it stands now.
    void SidebarEditor::itemsMoved(const IndexPath& under, StackPanel& rows)
    {
        SidebarItems items = m_items;
        IndexPath first = under;
        first.push_back(0ull);
        if (!under.empty() && !itemAt(items, under))
            return;
        SidebarItems& list = listAt(items, first);
        std::vector<std::size_t> order;   // the items' old indexes, in the rows' order
        for (const ControlPtr& child : rows.controls())
        {
            const Row* row = rowOf(child.get());
            if (row && row->path.size() == under.size() + 1ull)
                order.push_back(row->path.back());
        }
        const bool whole = order.size() == list.size()
            && std::ranges::all_of(order, [&list](const std::size_t index) {
                return index < list.size();
            });
        if (!whole)
            return;
        SidebarItems ordered;
        for (const std::size_t index : order)
            ordered.push_back(std::move(list[index]));
        list = std::move(ordered);
        if (const Row* selected = selectedRow())
        {
            IndexPath select = selected->path;
            const bool inList = select.size() > under.size()
                && std::equal(under.begin(), under.end(), select.begin());
            if (inList)
            {
                const std::vector<std::size_t>::iterator place = std::ranges::find(order,
                    select[under.size()]);
                select[under.size()] = static_cast<std::size_t>(place - order.begin());
            }
            m_select = select;
        }
        edit(std::move(items), true);
    }

    void SidebarEditor::edit(SidebarItems items, const bool rebuild)
    {
        SidebarEditEvent event{ std::move(items) };
        emitEvent(event);
        if (rebuild)
            reload();
    }

    // The kind set shows for an item, and Delete reads the selection again.
    void SidebarEditor::selectionChanged()
    {
        m_kinds.setVisible(selectedItem() != nullptr);
        for (const ControlPtr& answer : m_kindSet.controls())
            answer->invalidateState();
        if (m_deleteButton)
            m_deleteButton->invalidateState();
    }

    std::wstring_view SidebarEditor::kindCaption(const SidebarKind kind)
    {
        switch (kind)
        {
            case SidebarKind::Category:
                return L"Category";
            case SidebarKind::Divider:
                return L"Divider";
            case SidebarKind::AllTheApis:
                return L"All the APIs";
            case SidebarKind::Content:
                return L"Content";
        }
        return {};
    }
}
