module ClaFi.Core.TextEngine.StyleSheet;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // Whether two styles state the same fields. Their names and bases are not compared.
        [[nodiscard]] bool sameFields(const TextStyle& style, const TextStyle& other)
        {
            return style.fontFamily == other.fontFamily
                && style.fontSize == other.fontSize
                && style.fontWeight == other.fontWeight
                && style.fontStyle == other.fontStyle
                && style.ink == other.ink;
        }

        // Whether a style states any field of its own.
        [[nodiscard]] bool statesAField(const TextStyle& style)
        {
            return !sameFields(style, TextStyle{});
        }

        // The fields the other states that the style leaves unstated, copied in.
        void foldFields(TextStyle& style, const TextStyle& other)
        {
            if (!style.fontFamily)
                style.fontFamily = other.fontFamily;
            if (!style.fontSize)
                style.fontSize = other.fontSize;
            if (!style.fontWeight)
                style.fontWeight = other.fontWeight;
            if (!style.fontStyle)
                style.fontStyle = other.fontStyle;
            if (!style.ink)
                style.ink = other.ink;
        }

        // Whether two styles of two sheets are based alike: on one framework style, on styles of
        // one name in their own sheets, or on none.
        [[nodiscard]] bool sameBase(const TextStyle& style, const TextStyle& other)
        {
            if (!style.base || !other.base)
                return style.base == other.base;
            if (TextStyles::contains(style.base) || TextStyles::contains(other.base))
                return style.base == other.base;
            return style.base->name == other.base->name;
        }
    }

    // --- StyleSheet::ReadingScope ---

    StyleSheet::ReadingScope::ReadingScope(const StyleSheet* sheet)
        :
        m_outerSheet{ s_scopeSheet }
    {
        s_scopeSheet = sheet;
    }

    StyleSheet::ReadingScope::~ReadingScope()
    {
        s_scopeSheet = m_outerSheet;
    }

    // --- StyleSheet ---

    std::uint64_t StyleSheet::s_revision{ 0 };
    const StyleSheet* StyleSheet::s_applicationSheet{ nullptr };
    thread_local const StyleSheet* StyleSheet::s_scopeSheet{ nullptr };

    StyleSheet::StyleSheet(const StyleSheet& other)
    {
        takeStylesOf(other);
    }

    // **An assignment is an edit only where a style already here changes.** A style added here
    // is named by no text yet, and one the other sheet lacks is removed and kept, as remove does.
    StyleSheet& StyleSheet::operator=(const StyleSheet& other)
    {
        if (this == &other)
            return *this;
        if (takeStylesOf(other))
            edited();
        return *this;
    }

    StyleSheet& StyleSheet::operator=(StyleSheet&& other)
    {
        return *this = std::as_const(other);
    }

    // **A carried style draws as it did**, listed after the styles it is based on. A base a file
    // cannot name is stated into the style field by field instead: a style with no name, and one
    // of the framework's that a carried style of its name would stand in for. So every style is
    // added and none changed, and a copy is no edit.
    StyleSheet StyleSheet::NamedIn(const Text& text)
    {
        StyleList carried;
        text.forEachRun([&carried](const TextRunView& run) {
            StyleList chain;
            for (const TextStyle* style = run.look.style; style; style = style->base)
            {
                if (!TextStyles::contains(style) && !style->name.empty())
                    chain.push_back(style);
            }
            for (const TextStyle* style : std::views::reverse(chain))
            {
                if (std::ranges::find(carried, style->name, &TextStyle::name) == carried.end())
                    carried.push_back(style);
            }
        });

        StyleSheet result;
        for (const TextStyle* style : carried)
        {
            TextStyle stated = *style;
            const TextStyle* base = style->base;
            while (base && !TextStyles::contains(base) && base->name.empty())
            {
                foldFields(stated, *base);
                base = base->base;
            }
            const bool namesakeCarried = base && TextStyles::contains(base)
                && base->name != style->name
                && std::ranges::find(carried, base->name, &TextStyle::name) != carried.end();
            if (namesakeCarried)
            {
                foldFields(stated, *base);
                base = nullptr;
            }
            else if (base && !TextStyles::contains(base))
            {
                base = result.findOwn(base->name);
            }
            stated.base = base;
            result.set(stated);
        }
        return result;
    }

    const StyleSheet* StyleSheet::reading()
    {
        return s_scopeSheet ? s_scopeSheet : s_applicationSheet;
    }

    const TextStyle* StyleSheet::findReading(std::wstring_view name)
    {
        const StyleSheet* sheet = reading();
        return sheet ? sheet->find(name) : TextStyles::named(name);
    }

    void StyleSheet::setApplicationSheet(const StyleSheet* sheet)
    {
        s_applicationSheet = sheet;
    }

    std::uint64_t StyleSheet::revision()
    {
        return s_revision;
    }

    // A dispatcher of the process, which every window connects to and none outlives.
    EventDispatcher& StyleSheet::events()
    {
        static EventDispatcher dispatcher;
        return dispatcher;
    }

    const TextStyle* StyleSheet::find(std::wstring_view name) const
    {
        const TextStyle* own = findOwn(name);
        return own ? own : TextStyles::named(name);
    }

    const TextStyle* StyleSheet::findOwn(std::wstring_view name) const
    {
        const Entry* entry = liveEntry(name);
        return entry ? &entry->style : nullptr;
    }

    const TextStyle& StyleSheet::styleFor(const TextStyle& frameworkStyle) const
    {
        const TextStyle* own = findOwn(frameworkStyle.name);
        return own ? *own : frameworkStyle;
    }

    bool StyleSheet::holds(const TextStyle* style) const
    {
        return std::ranges::any_of(m_entries, [style](const EntryPtr& entry) {
            return &entry->style == style;
        });
    }

    StyleSheet::StyleList StyleSheet::styles() const
    {
        StyleList result;
        for (const EntryPtr& entry : m_entries)
        {
            if (!entry->removed)
                result.push_back(&entry->style);
        }
        return result;
    }

    bool StyleSheet::empty() const
    {
        return std::ranges::all_of(m_entries, [](const EntryPtr& entry) {
            return entry->removed;
        });
    }

    // Two sheets are equal when they list the same styles in one order, each with the same name,
    // the same fields and a base alike.
    bool StyleSheet::operator==(const StyleSheet& other) const
    {
        const StyleList listed = styles();
        const StyleList otherListed = other.styles();
        if (listed.size() != otherListed.size())
            return false;
        for (std::size_t i = 0; i != listed.size(); ++i)
        {
            const TextStyle& style = *listed[i];
            const TextStyle& otherStyle = *otherListed[i];
            if (style.name != otherStyle.name)
                return false;
            if (!sameFields(style, otherStyle) || !sameBase(style, otherStyle))
                return false;
        }
        return true;
    }

    // **A name the sheet holds is the sheet's style**, in a base as in a file: a base named as
    // one of the sheet's own is that one, and a style named as one of the framework's stands in
    // for it in every base here, which is an edit where one was based on it. Such a style based
    // on none is based on the framework's. A base is refused unless it is the framework's or one
    // the sheet lists, and so is one leading back to the style being stated.
    const TextStyle* StyleSheet::set(const TextStyle& source)
    {
        if (source.name.empty())
            return nullptr;

        const TextStyle* namesake = TextStyles::named(source.name);
        const TextStyle* base = source.base ? source.base : namesake;
        if (base && !takesAsBase(base))
            return nullptr;
        if (base && base != namesake && TextStyles::contains(base))
        {
            if (const Entry* own = liveEntry(base->name))
                base = &own->style;
        }

        Entry* entry = liveEntry(source.name);
        if (leadsBack(base, entry ? &entry->style : nullptr, namesake))
            return nullptr;

        if (!entry)
        {
            EntryPtr added = std::make_unique<Entry>();
            added->name = std::wstring{ source.name };
            added->style.name = added->name;
            stateFields(*added, source);
            added->style.base = base;
            m_entries.push_back(std::move(added));
            Entry& stated = *m_entries.back();
            const bool drawsOtherwise = statesAField(stated.style) || stated.style.base != namesake;
            if (standInFor(stated, namesake) && drawsOtherwise)
                edited();
            return &stated.style;
        }

        if (sameFields(entry->style, source) && entry->style.base == base)
            return &entry->style;
        stateFields(*entry, source);
        entry->style.base = base;
        edited();
        return &entry->style;
    }

    // A style renamed as one of the framework's stands in for it, as set says, and one based on
    // none is based on it from then on.
    bool StyleSheet::rename(std::wstring_view name, std::wstring_view newName)
    {
        Entry* entry = liveEntry(name);
        if (!entry || newName.empty())
            return false;
        const Entry* holder = liveEntry(newName);
        if (holder && holder != entry)
            return false;
        const TextStyle* namesake = TextStyles::named(newName);
        if (leadsBack(entry->style.base, nullptr, namesake))
            return false;

        std::wstring renamed{ newName };
        entry->name = std::move(renamed);
        entry->style.name = entry->name;
        const bool basedAnew = !entry->style.base && namesake;
        if (basedAnew)
            entry->style.base = namesake;
        const bool drawsOtherwise = statesAField(entry->style) || entry->style.base != namesake;
        const bool rebased = standInFor(*entry, namesake) && drawsOtherwise;
        if (basedAnew || rebased)
            edited();
        return true;
    }

    // **A removed style stays where it stands**, so a text naming it draws as it did. The styles
    // based on it are based on its base instead - one named as one of the framework's on that one
    // where its base is none - and that is an edit where it changes what they draw.
    bool StyleSheet::remove(std::wstring_view name)
    {
        Entry* entry = liveEntry(name);
        if (!entry)
            return false;

        entry->removed = true;
        bool redrawn = false;
        for (const EntryPtr& other : m_entries)
        {
            if (other->removed || other->style.base != &entry->style)
                continue;
            const TextStyle* base = entry->style.base;
            if (!base)
                base = TextStyles::named(other->name);
            other->style.base = base;
            if (statesAField(entry->style) || base != entry->style.base)
                redrawn = true;
        }
        if (redrawn)
            edited();
        return true;
    }

    // The order is the one a file is written in and a reader lists, and no text draws from it.
    bool StyleSheet::moveTo(std::wstring_view name, std::size_t position)
    {
        const Entries::iterator found = std::ranges::find_if(m_entries,
            [name](const EntryPtr& entry) {
                return !entry->removed && entry->name == name;
            });
        if (found == m_entries.end())
            return false;

        EntryPtr moved = std::move(*found);
        m_entries.erase(found);
        std::size_t listed = 0;
        Entries::iterator place = m_entries.begin();
        for (; place != m_entries.end(); ++place)
        {
            if ((*place)->removed)
                continue;
            if (listed == position)
                break;
            ++listed;
        }
        m_entries.insert(place, std::move(moved));
        return true;
    }

    StyleSheet::Entry* StyleSheet::liveEntry(std::wstring_view name)
    {
        return const_cast<Entry*>(std::as_const(*this).liveEntry(name));
    }

    const StyleSheet::Entry* StyleSheet::liveEntry(std::wstring_view name) const
    {
        for (const EntryPtr& entry : m_entries)
        {
            if (!entry->removed && entry->name == name)
                return entry.get();
        }
        return nullptr;
    }

    bool StyleSheet::takesAsBase(const TextStyle* style) const
    {
        if (TextStyles::contains(style))
            return true;
        return std::ranges::any_of(m_entries, [style](const EntryPtr& entry) {
            return !entry->removed && &entry->style == style;
        });
    }

    // A chain from the base that reaches the style, or reaches its framework namesake past the
    // base itself, closes a loop once the style stands in for that namesake.
    bool StyleSheet::leadsBack(const TextStyle* base, const TextStyle* style,
        const TextStyle* namesake)
    {
        for (const TextStyle* ancestor = base; ancestor; ancestor = ancestor->base)
        {
            if (ancestor == style)
                return true;
            if (namesake && ancestor == namesake && ancestor != base)
                return true;
        }
        return false;
    }

    bool StyleSheet::standInFor(const Entry& entry, const TextStyle* namesake)
    {
        if (!namesake)
            return false;
        bool rebased = false;
        for (const EntryPtr& other : m_entries)
        {
            if (other->removed || other.get() == &entry || other->style.base != namesake)
                continue;
            other->style.base = &entry.style;
            rebased = true;
        }
        return rebased;
    }

    // **A style named alike in both keeps its entry**, so a text naming it here keeps its style.
    // The rest is listed in the other's order, and what it names nowhere is removed and kept.
    bool StyleSheet::takeStylesOf(const StyleSheet& other)
    {
        if (this == &other)
            return false;

        // One of the other's styles, and whether an entry here named alike stands for it.
        struct Match
        {
            const Entry* source{ nullptr };
            bool kept{ false };
        };

        std::vector<Match> matches;
        Entries listed;
        for (const EntryPtr& source : other.m_entries)
        {
            if (source->removed)
                continue;
            const Entries::iterator kept = std::ranges::find_if(m_entries,
                [&source](const EntryPtr& entry) {
                    return entry && !entry->removed && entry->name == source->name;
                });
            const bool found = kept != m_entries.end();
            EntryPtr entry = found ? std::move(*kept) : std::make_unique<Entry>();
            if (!found)
            {
                entry->name = source->name;
                entry->style.name = entry->name;
            }
            matches.push_back({ source.get(), found });
            listed.push_back(std::move(entry));
        }
        for (EntryPtr& entry : m_entries)
        {
            if (!entry)
                continue;
            entry->removed = true;
            listed.push_back(std::move(entry));
        }
        m_entries = std::move(listed);

        // Every listed style is based on one of the framework's or one listed, so a base among
        // the other's own is found here by its name.
        bool changed = false;
        for (std::size_t i = 0; i != matches.size(); ++i)
        {
            const TextStyle& source = matches[i].source->style;
            Entry& entry = *m_entries[i];
            const TextStyle* base = source.base;
            if (base && !TextStyles::contains(base))
                base = &liveEntry(base->name)->style;
            if (sameFields(entry.style, source) && entry.style.base == base)
                continue;
            stateFields(entry, source);
            entry.style.base = base;
            if (matches[i].kept)
                changed = true;
        }
        return changed;
    }

    void StyleSheet::stateFields(Entry& entry, const TextStyle& source)
    {
        std::optional<std::wstring> family;
        if (source.fontFamily)
            family = std::wstring{ *source.fontFamily };
        entry.fontFamily = std::move(family);
        entry.style.fontFamily = entry.fontFamily
            ? std::optional<std::wstring_view>{ *entry.fontFamily }
            : std::nullopt;
        entry.style.fontSize = source.fontSize;
        entry.style.fontWeight = source.fontWeight;
        entry.style.fontStyle = source.fontStyle;
        entry.style.ink = source.ink;
    }

    void StyleSheet::edited()
    {
        ++s_revision;
        events().emit<StylesEditedEvent>();
    }
}
