module SeeDocs_App.SidebarEdits;

import SeeDocs_App.Project;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    namespace
    {
        constexpr std::wstring_view k_categoryName = L"New category";
        constexpr std::wstring_view k_contentName = L"New page";
        constexpr std::wstring_view k_apisName = L"All the APIs";
        // What no Windows file name holds, the slash that parts a url's names among it.
        constexpr std::wstring_view k_unsafeCharacters = L"/\\:*?\"<>|";
        constexpr std::wstring_view k_unsafeRefusal =
            L"A name cannot hold any of / \\ : * ? \" < > |.";

        // The device names Windows keeps, which no file takes, with an extension after them or not.
        constexpr std::array<std::wstring_view, 30> k_deviceNames{
            L"con", L"prn", L"aux", L"nul",
            L"com0", L"com1", L"com2", L"com3", L"com4",
            L"com5", L"com6", L"com7", L"com8", L"com9",
            L"com\u00B9", L"com\u00B2", L"com\u00B3",
            L"lpt0", L"lpt1", L"lpt2", L"lpt3", L"lpt4",
            L"lpt5", L"lpt6", L"lpt7", L"lpt8", L"lpt9",
            L"lpt\u00B9", L"lpt\u00B2", L"lpt\u00B3"
        };

        [[nodiscard]] bool holdsControlCharacter(const std::wstring_view name)
        {
            for (const wchar_t character : name)
            {
                if (character < L' ')
                    return true;
            }
            return false;
        }

        [[nodiscard]] std::wstring folded(const std::wstring_view name)
        {
            std::wstring result{ name };
            ::ClaFi::foldCase(result);
            return result;
        }

        // A name no item of the list goes by: the kind's, numbered where that is taken.
        [[nodiscard]] std::wstring newName(const SidebarKind kind, const SidebarItems& siblings)
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
    }

    // SidebarEditEvent

    SidebarEditEvent::SidebarEditEvent(SidebarItems edited, std::optional<IndexPath> renamedAt)
        :
        ::ClaFi::Event{},
        items{ std::move(edited) },
        renamedItem{ std::move(renamedAt) }
    {
    }

    std::wstring_view kindCaption(const SidebarKind kind)
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

    SidebarItems& listAt(SidebarItems& items, const IndexPath& path)
    {
        SidebarItems* list = &items;
        for (std::size_t i = 0; i + 1ull < path.size(); ++i)
            list = &(*list)[path[i]].items;
        return *list;
    }

    const SidebarItem* itemAt(const SidebarItems& items, const IndexPath& path)
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

    SidebarItem* itemAt(SidebarItems& items, const IndexPath& path)
    {
        return const_cast<SidebarItem*>(itemAt(std::as_const(items), path));
    }

    SidebarNames namesTo(const SidebarItems& items, const IndexPath& path)
    {
        SidebarNames names;
        const SidebarItems* list = &items;
        for (const std::size_t index : path)
        {
            const SidebarItem& item = (*list)[index];
            names.push_back(item.name);
            list = &item.items;
        }
        return names;
    }

    IndexPath placeToAdd(const SidebarItems& items, const IndexPath* picked)
    {
        const SidebarItem* item = picked ? itemAt(items, *picked) : nullptr;
        if (!item)
            return { items.size() };
        IndexPath at = *picked;
        if (item->kind == SidebarKind::Category)
            at.push_back(item->items.size());
        else
            ++at.back();
        return at;
    }

    void addItem(SidebarItems& items, const IndexPath& at, const SidebarKind kind)
    {
        SidebarItems& list = listAt(items, at);
        list.insert(list.begin() + static_cast<std::ptrdiff_t>(at.back()), SidebarItem{
            .kind = kind,
            .name = newName(kind, list)
        });
    }

    IndexPath deleteItem(SidebarItems& items, const IndexPath& path)
    {
        SidebarItems& list = listAt(items, path);
        list.erase(list.begin() + static_cast<std::ptrdiff_t>(path.back()));
        IndexPath landing = path;
        if (landing.back() >= list.size() && landing.back() > 0ull)
            --landing.back();
        else if (list.empty())
            landing.pop_back();
        return landing;
    }

    bool canBeKind(const SidebarItem& item, const SidebarKind kind)
    {
        return item.kind == kind || item.items.empty();
    }

    void setKind(SidebarItems& items, const IndexPath& path, const SidebarKind kind)
    {
        SidebarItems& list = listAt(items, path);
        SidebarItem& item = list[path.back()];
        const bool wasDivider = item.kind == SidebarKind::Divider;
        item.kind = kind;
        if (kind == SidebarKind::Divider)
            item.name.clear();
        else if (wasDivider)
            item.name = newName(kind, list);
    }

    std::wstring refusalOf(const std::wstring_view name, const SidebarItems& siblings,
        const SidebarItem* self)
    {
        if (name.empty())
            return L"The name is empty.";
        if (name.find_first_of(k_unsafeCharacters) != std::wstring_view::npos)
            return std::wstring{ k_unsafeRefusal };
        if (holdsControlCharacter(name))
            return L"A name cannot hold a tab or another control character.";
        if (name.back() == L'.' || name.back() == L' ')
            return L"A name cannot end in a dot or a space.";
        const std::wstring_view stem = name.substr(0, name.find(L'.'));
        if (std::ranges::find(k_deviceNames, folded(stem)) != k_deviceNames.end())
            return std::format(L"Windows keeps the name {} for a device.", stem);
        const std::wstring foldedName = folded(name);
        for (const SidebarItem& sibling : siblings)
        {
            if (&sibling == self || sibling.kind == SidebarKind::Divider)
                continue;
            if (sibling.name == name)
                return L"Another item here is named so.";
            if (folded(sibling.name) == foldedName)
                return L"Another item here has this name in other letter case.";
        }
        return {};
    }

    bool reorder(SidebarItems& items, const IndexPath& under,
        const std::span<const std::size_t> order)
    {
        IndexPath first = under;
        first.push_back(0ull);
        if (!under.empty() && !itemAt(items, under))
            return false;
        SidebarItems& list = listAt(items, first);
        std::vector<bool> named(list.size(), false);
        for (const std::size_t index : order)
        {
            if (index >= list.size() || named[index])
                return false;
            named[index] = true;
        }
        if (order.size() != list.size())
            return false;
        SidebarItems ordered;
        for (const std::size_t index : order)
            ordered.push_back(std::move(list[index]));
        list = std::move(ordered);
        return true;
    }

    IndexPath afterReorder(IndexPath path, const IndexPath& under,
        const std::span<const std::size_t> order)
    {
        const bool inList = path.size() > under.size()
            && std::equal(under.begin(), under.end(), path.begin());
        if (!inList)
            return path;
        const std::span<const std::size_t>::iterator place = std::ranges::find(order,
            path[under.size()]);
        if (place != order.end())
            path[under.size()] = static_cast<std::size_t>(place - order.begin());
        return path;
    }
}
