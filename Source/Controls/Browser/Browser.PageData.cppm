export module ClaFi.Browser.PageData;

import ClaFi.Browser.Consts;
import ClaFi.Core.TextEngine.Text;
import ClaFi.StdLib;

namespace ClaFi::Browser
{
    // How much is known about what lies under a page. See Browser
    export enum class FetchState
    {
        Unfetched,
        HasChildren,
        Fetched
    };

    export struct PageData;
    export using AddPageDataCallback = std::function<void(PageData&)>;
    /// Whether a page must stay in the tree, asked of each one its parent is about to drop.
    export using KeepPageDataCallback = std::function<bool(const PageData&)>;

    export using PageDataPtr = std::unique_ptr<PageData>;
    export using PageDataList = std::vector<PageDataPtr>;

    export struct PageData
    {
    public:
        PageData(PageData* parent, std::wstring_view name);
        PageData& addSubItem(std::wstring_view, const AddPageDataCallback&);
        PageData& addSubPath(std::wstring_view, const AddPageDataCallback&);
        std::size_t level();
        void paintText(Text&) const;
        std::wstring_view displayTitle() const;
        std::wstring path() const;
        PageData* findOrAdd(std::wstring_view, const AddPageDataCallback&);
        /// The sub-item of this page carrying this name, or nothing when no such page has been
        /// reached. This is what lets a fetch over a partly walked page add only what is missing.
        [[nodiscard]] PageData* findSubItem(std::wstring_view subItemName);
        /// Whether anything stands under this page. Sub-items already reached answer for
        /// themselves; a page the browser marked HasChildren answers before any of them has been
        /// named. Anything else is a leaf as far as the tree can tell, and is drawn as one.
        [[nodiscard]] bool hasSubItems() const;
        /// Where a sub-item stands in the list, or items.size() when it is not one of them.
        [[nodiscard]] std::size_t subItemIndex(const PageData& subItem) const;
        /// Moves a sub-item to this place in the list, carrying the ones it passes along with it.
        /// An index past the end, or a page that is not a sub-item of this one, moves nothing.
        void moveSubItemTo(const PageData& subItem, std::size_t index);
        /// Frees the sub-items standing at this index and after, other than the ones the callback
        /// keeps. What is kept holds the order it stands in.
        ///
        /// @note A PAGE IS FREED WITH EVERYTHING UNDER IT, so a page reached through is kept by
        /// answering for the page itself - the callback is asked only about this page's own
        /// sub-items, and never about what stands beneath them.
        void dropSubItemsFrom(std::size_t index, const KeepPageDataCallback&);
        /// Takes the name this page is known by where it stands - a file name, a key, whatever the
        /// browser walks a path out of.
        ///
        /// @note THE PAGE ITSELF STAYS PUT, so every `PageData*` held elsewhere - a tab, an open
        /// page, the browser's selection - goes on naming it and follows the new name. What does
        /// change is the path of this page and of every page under it, which a browser storing
        /// paths writes out again.
        ///
        /// @note THE TITLE GOES WITH THE NAME. A title is worked out from the name - the browser
        /// does that as a page is built - so one worked out from the name being replaced is not
        /// this page's title any more, and the browser is asked for it again.
        void rename(std::wstring_view newName);
    public:
        PageData* parent{};
        std::wstring name;
        std::wstring title{};
        FetchState fetchState{};
        PageDataList items{};
    };

    PageData::PageData(PageData* parent, std::wstring_view name)
        :
        parent{ parent },
        name{ name }
    {
    }

    PageData& PageData::addSubItem(std::wstring_view subItemName, const AddPageDataCallback& callback)
    {
        items.push_back(std::make_unique<PageData>(this, subItemName));
        PageData& result = *items.back();
        callback(result);
        return result;
    }

    PageData& PageData::addSubPath(std::wstring_view path, const AddPageDataCallback& callback)
    {
        std::size_t i = path.find(ConfigNames::pathSeparator);
        if (i == path.npos)
            return addSubItem(path, callback);
        const std::wstring_view subItemName = path.substr(0, i);
        PageData& newSubItem = addSubItem(subItemName, callback);
        const std::wstring_view remainPath = path.substr(i + 1ull);
        return newSubItem.addSubPath(remainPath, callback);
    }

    std::size_t PageData::level()
    {
        return parent ? parent->level() + 1ull : 0;
    }

    void PageData::paintText(Text& tt) const
    {
        tt << displayTitle();
    }

    std::wstring_view PageData::displayTitle() const
    {
        return title.empty() ? (name.empty() ? ConfigNames::untitledPage : name) : title;
    }

    std::wstring PageData::path() const
    {
        std::wstring result{};
        if (parent)
        {
            result = parent->path();
            result.append(L"/");
        }
        result.append(name);
        return result;
    }

    PageData* PageData::findOrAdd(std::wstring_view path, const AddPageDataCallback& callback)
    {
        std::size_t i = path.find(ConfigNames::pathSeparator, 0);
        if (i == path.npos)
        {
            if (name == path)
                return this;
        }
        else
            if (name == path.substr(0ull, i))
            {
                const std::wstring_view subPath = path.substr(i + 1ull);
                for (PageDataPtr& subItem : items)
                    if (PageData* result = subItem->findOrAdd(subPath, callback))
                        return result;
                return &addSubPath(subPath, callback);
            }
        return nullptr;
    }

    PageData* PageData::findSubItem(const std::wstring_view subItemName)
    {
        for (const PageDataPtr& subItem : items)
            if (subItem->name == subItemName)
                return subItem.get();

        return nullptr;
    }

    bool PageData::hasSubItems() const
    {
        return !items.empty() || fetchState == FetchState::HasChildren;
    }

    std::size_t PageData::subItemIndex(const PageData& subItem) const
    {
        for (std::size_t i = 0; i != items.size(); ++i)
            if (items[i].get() == &subItem)
                return i;

        return items.size();
    }

    void PageData::moveSubItemTo(const PageData& subItem, const std::size_t index)
    {
        if (index >= items.size())
            return;

        const std::size_t current = subItemIndex(subItem);
        if (current == items.size() || current == index)
            return;

        const PageDataList::iterator found = items.begin() + current;

        // THE POINTERS IN THE LIST MOVE, THE PAGES THEY POINT AT DO NOT. That is what lets a tab go
        // on holding a PageData* across a reorder of the list it stands in - and every tab does.
        if (current > index)
            std::rotate(items.begin() + index, found, found + 1);
        else
            std::rotate(found, found + 1, items.begin() + index + 1);
    }

    void PageData::dropSubItemsFrom(const std::size_t index, const KeepPageDataCallback& keep)
    {
        if (index >= items.size())
            return;

        const PageDataList::iterator first = items.begin() + index;
        items.erase(
            std::remove_if(first, items.end(), [&keep](const PageDataPtr& subItem){
                return !keep(*subItem);
            }),
            items.end()
        );
    }

    void PageData::rename(const std::wstring_view newName)
    {
        name = newName;
        title.clear();
    }
}
