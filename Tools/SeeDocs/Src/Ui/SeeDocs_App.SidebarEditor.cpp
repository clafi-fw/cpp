module SeeDocs_App.SidebarEditor;

import SeeDocs_App.Icons;
import SeeDocs_App.Project;
import SeeDocs_App.SidebarEdits;
import SeeDocs_App.SidebarRows;
import SeeDocs_App.Surface;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
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
        constexpr std::wstring_view k_addCaption = L"Add";
        constexpr std::wstring_view k_deleteCaption = L"Delete";
        constexpr std::wstring_view k_addTooltip = L"Adds an item under the selected category, "
            L"or after the selected item";
        constexpr std::wstring_view k_deleteTooltip = L"Takes the selected item out, with "
            L"everything under it";
        constexpr std::wstring_view k_deleteTitle = L"Delete category";
    }

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
        for (const SidebarKind kind : k_sidebarKinds)
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
            HeaderText{ rowIcon(RowIcon::Project), Space{ k_rowIconGap }, projectName }
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

    void SidebarEditor::select(IndexPath path)
    {
        const Row* found = nullptr;
        while (!path.empty() && !found)
        {
            found = rowAt(path);
            if (!found)
                path.pop_back();
        }
        Control& current = found
            ? (found->node ? static_cast<Control&>(found->node->header()) : *found->row)
            : m_root.header();
        m_tree.setCurrentItem(current);
        current.scrollIntoViewOnAlign();
        selectionChanged();
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
        for (const SidebarKind kind : k_sidebarKinds)
            addKind(kind, kindCaption(kind));
        addSubgroupOption();
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

    // Set in as the kinds are, its box under their marks.
    void SidebarEditor::addSubgroupOption()
    {
        m_subgroupOption = &m_kinds.add<StackPanel>(
            Orientation::Vertical,
            Padding{ k_choiceIndent, k_optionGap }
        );
        m_subgroupOption->add<CheckBox>(
            k_subgroupCaption,
            Padding{ 0.0f, m_kinds.themeMetrics().listItem.padding.y },
            HorizontalAlign::Left,
            CheckBox::OnGetState{ [this](GetStateEvent& event) {
                const SidebarItem* item = selectedItem();
                event.state.selected = item && item->subgroup;
            } },
            CheckBox::OnClick{ [this](ClickEvent& event) {
                const SidebarItem* item = selectedItem();
                if (item)
                    setSubgroup(!item->subgroup);
                event.control->invalidateState();
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
                        std::type_identity<SidebarNodeHeader>{},
                        ItemsOrder::Rearrangeable,
                        HeaderText{ item.name }
                    );
                    SidebarNodeHeader& header = static_cast<SidebarNodeHeader&>(node.header());
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
                    row.row = &holder.addItem(std::type_identity<SidebarDividerRow>{});
                    break;
                }
                case SidebarKind::AllTheApis:
                case SidebarKind::Content:
                {
                    SidebarItemRow& made = holder.addItem(std::type_identity<SidebarItemRow>{},
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
        IndexPath wanted = m_select.value_or(IndexPath{});
        if (!m_select.has_value())
        {
            if (const Row* selected = selectedRow())
                wanted = selected->path;
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

        select(std::move(wanted));
        const Row* found = selectedRow();
        const SidebarItem* item = found ? itemAt(m_items, found->path) : nullptr;
        if (std::exchange(m_renameNext, false) && item)
        {
            if (found->node)
                static_cast<SidebarNodeHeader&>(found->node->header()).openEditor();
            else if (item->kind != SidebarKind::Divider)
                static_cast<SidebarItemRow&>(*found->row).openEditor();
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

    // The new item is named by its kind and, a divider aside, opens for its name at once.
    void SidebarEditor::addItem(const SidebarKind kind)
    {
        SidebarItems items = m_items;
        const Row* selected = selectedRow();
        const IndexPath at = placeToAdd(items, selected ? &selected->path : nullptr);
        SeeDocs_App::addItem(items, at, kind);
        m_select = at;
        m_renameNext = kind != SidebarKind::Divider;
        edit(std::move(items), true);
    }

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
        m_select = deleteItem(items, path);
        edit(std::move(items), true);
    }

    bool SidebarEditor::canDelete() const
    {
        return selectedItem() != nullptr;
    }

    bool SidebarEditor::canBeKind(const SidebarKind kind) const
    {
        const SidebarItem* item = selectedItem();
        return item && SeeDocs_App::canBeKind(*item, kind);
    }

    void SidebarEditor::setKind(const SidebarKind kind)
    {
        const Row* selected = selectedRow();
        if (!selected || !itemAt(m_items, selected->path))
            return;
        SidebarItems items = m_items;
        SeeDocs_App::setKind(items, selected->path, kind);
        m_select = selected->path;
        edit(std::move(items), true);
    }

    // The rows stand as they are: only the folders move, in the trees and on the pages.
    void SidebarEditor::setSubgroup(const bool subgroup)
    {
        const Row* selected = selectedRow();
        if (!selected)
            return;
        SidebarItems items = m_items;
        SidebarItem* item = itemAt(items, selected->path);
        if (!item || item->subgroup == subgroup)
            return;
        item->subgroup = subgroup;
        edit(std::move(items), false);
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
        std::vector<std::size_t> order;   // the items' old indexes, in the rows' order
        for (const ControlPtr& child : rows.controls())
        {
            const Row* row = rowOf(child.get());
            if (row && row->path.size() == under.size() + 1ull)
                order.push_back(row->path.back());
        }
        SidebarItems items = m_items;
        if (!reorder(items, under, order))
            return;
        if (const Row* selected = selectedRow())
            m_select = afterReorder(selected->path, under, order);
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
        const SidebarItem* item = selectedItem();
        m_kinds.setVisible(item != nullptr);
        m_subgroupOption->setVisible(item && item->kind == SidebarKind::AllTheApis);
        for (const ControlPtr& answer : m_kindSet.controls())
            answer->invalidateState();
        for (const ControlPtr& answer : m_subgroupOption->controls())
            answer->invalidateState();
        if (m_deleteButton)
            m_deleteButton->invalidateState();
    }
}
