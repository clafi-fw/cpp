export module SeeDocs_App.SidebarEdits;

import SeeDocs_App.Project;

import ClaFi.Core.System.Events;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    // Where an item of a sidebar stands: its index in each list down from the top.
    export using IndexPath = std::vector<std::size_t>;

    // The kinds in the order a menu and the kind set list them.
    export constexpr std::array<SidebarKind, 4ull> k_sidebarKinds{
        SidebarKind::Category,
        SidebarKind::Divider,
        SidebarKind::AllTheApis,
        SidebarKind::Content
    };

    // A sidebar was edited: what it is to be. The surface has an Event of its own.
    export class SidebarEditEvent : public ::ClaFi::Event
    {
    public:
        explicit SidebarEditEvent(SidebarItems);
    public:
        SidebarItems items;
    };

    export [[nodiscard]] std::wstring_view kindCaption(SidebarKind);
    // The list the path's last index counts in. The path's other indexes name items that are
    // there.
    export [[nodiscard]] SidebarItems& listAt(SidebarItems&, const IndexPath&);
    // The item the path names; null where the items do not reach that far.
    export [[nodiscard]] const SidebarItem* itemAt(const SidebarItems&, const IndexPath&);
    export [[nodiscard]] SidebarItem* itemAt(SidebarItems&, const IndexPath&);
    // Where an item added with this one picked goes: the end of a category, after any other
    // item, and the end of the top level with none picked. See README
    export [[nodiscard]] IndexPath placeToAdd(const SidebarItems&, const IndexPath* picked);
    // Puts an item of the kind at the path, named by its kind - numbered where that is taken,
    // and a divider named nothing.
    export void addItem(SidebarItems&, const IndexPath& at, SidebarKind);
    // Takes the item out with everything under it, and answers where a pick lands: on the item
    // taking its place, else the one before it, else the category it stood in.
    export [[nodiscard]] IndexPath deleteItem(SidebarItems&, const IndexPath&);
    // Whether the item may be of the kind: a category holding items keeps its kind.
    export [[nodiscard]] bool canBeKind(const SidebarItem&, SidebarKind);
    // Makes the item of the kind. One made a divider loses its name, and a divider made anything
    // else is named by its new kind.
    export void setKind(SidebarItems&, const IndexPath&, SidebarKind);
    // Why the name cannot be the item's; empty where it can. A name is a segment of the item's
    // url, so it holds no slash, and it tells the item from the others beside it.
    export [[nodiscard]] std::wstring refusalOf(std::wstring_view name,
        const SidebarItems& siblings, const SidebarItem* self);
    // Puts the items under the path in the order of their indexes as they stand - each named
    // once - and answers whether the order did so.
    export [[nodiscard]] bool reorder(SidebarItems&, const IndexPath& under,
        std::span<const std::size_t> order);
    // Where the path leads once the items under the other path stand in the order.
    export [[nodiscard]] IndexPath afterReorder(IndexPath path, const IndexPath& under,
        std::span<const std::size_t> order);
}
