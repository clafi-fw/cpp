module ClaFi.Core.TextEngine.StyleSheet;

import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // Whether two styles state the same fields and script levels. Names and bases aside.
        [[nodiscard]] bool sameFields(const TextStyle& style, const TextStyle& other)
        {
            return style.fontFamily == other.fontFamily
                && style.fontSize == other.fontSize
                && style.fontWeight == other.fontWeight
                && style.fontStyle == other.fontStyle
                && style.ink == other.ink
                && style.scripts == other.scripts;
        }

        // Whether a style states any field or script level of its own.
        [[nodiscard]] bool statesAField(const TextStyle& style)
        {
            return !sameFields(style, TextStyle{});
        }

        // Whether a style standing in for its namesake draws otherwise than the namesake does.
        [[nodiscard]] bool drawsOtherwise(const TextStyle& style, const TextStyle* namesake)
        {
            return statesAField(style) || style.base != namesake;
        }

        // A framework style: a generic family, a size, a weight and a slant, and no ink.
        [[nodiscard]] TextStyle frameworkStyle(std::wstring_view name, const wchar_t* family,
            float size, FontWeight weight, FontStyle slant)
        {
            return {
                .name = std::wstring{ name },
                .fontFamily = std::wstring{ family },
                .fontSize = size,
                .fontWeight = weight,
                .fontStyle = slant,
            };
        }
    }

    // --- UnknownStyleEvent ---

    UnknownStyleEvent::UnknownStyleEvent(std::wstring_view name)
        :
        m_name{ name }
    {
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

    StyleSheet::StyleSheet()
        :
        m_parent{ &framework() }
    {
    }

    StyleSheet::StyleSheet(const StyleSheet* parent)
        :
        m_parent{ parent }
    {
    }

    // A base among the other's own is the copy at its position; one up the parents is the same.
    StyleSheet::StyleSheet(const StyleSheet& other)
        :
        m_parent{ other.m_parent }
    {
        m_entries.reserve(other.m_entries.size());
        for (const EntryPtr& entry : other.m_entries)
            m_entries.push_back(std::make_unique<Entry>(*entry));
        for (EntryPtr& entry : m_entries)
        {
            const TextStyle* base = entry->style.base;
            if (!base || !other.holds(base))
                continue;
            for (std::size_t i = 0; i != other.m_entries.size(); ++i)
            {
                if (&other.m_entries[i]->style == base)
                {
                    entry->style.base = &m_entries[i]->style;
                    break;
                }
            }
        }
    }

    // Built on first use and never changed, so a text based on one of its styles holds it for the
    // process.
    const StyleSheet& StyleSheet::framework()
    {
        static const StyleSheet sheet = []() {
            StyleSheet built{ nullptr };
            const wchar_t* sans = GenericFamily::SansSerif;
            built.set(frameworkStyle(TextStyles::Body, sans, 12.0f, FontWeight::Normal,
                FontStyle::Normal));
            built.set(frameworkStyle(TextStyles::Title, sans, 24.0f, FontWeight::Bold,
                FontStyle::Normal));
            built.set(frameworkStyle(TextStyles::SubTitle, sans, 17.0f, FontWeight::Normal,
                FontStyle::Normal));
            built.set(frameworkStyle(TextStyles::Heading, sans, 16.0f, FontWeight::SemiBold,
                FontStyle::Normal));
            built.set(frameworkStyle(TextStyles::SubHeading, sans, 14.0f, FontWeight::Normal,
                FontStyle::Normal));
            built.set(frameworkStyle(TextStyles::Section, sans, 12.0f, FontWeight::Bold,
                FontStyle::Normal));
            built.set(frameworkStyle(TextStyles::SubSection, sans, 12.0f, FontWeight::Normal,
                FontStyle::Italic));
            built.set(frameworkStyle(TextStyles::SubBody, sans, 10.0f, FontWeight::Normal,
                FontStyle::Normal));
            built.set(frameworkStyle(TextStyles::Code, GenericFamily::Monospace, 12.0f,
                FontWeight::Normal, FontStyle::Normal));
            return built;
        }();
        return sheet;
    }

    const StyleSheet& StyleSheet::application()
    {
        return s_applicationSheet ? *s_applicationSheet : framework();
    }

    void StyleSheet::setApplicationSheet(const StyleSheet* sheet)
    {
        s_applicationSheet = sheet;
    }

    const StyleSheet* StyleSheet::reading()
    {
        return s_scopeSheet;
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
        if (const TextStyle* own = findOwn(name))
            return own;
        return m_parent ? m_parent->find(name) : nullptr;
    }

    const TextStyle* StyleSheet::findOwn(std::wstring_view name) const
    {
        if (name.empty())
            return nullptr;
        const Entry* entry = liveEntry(name);
        return entry ? &entry->style : nullptr;
    }

    // A sheet based on none of the framework's may hold no Body, and the framework's stands in.
    const TextStyle& StyleSheet::body() const
    {
        if (const TextStyle* found = find(TextStyles::Body))
            return *found;
        return *framework().findOwn(TextStyles::Body);
    }

    bool StyleSheet::holds(const TextStyle* style) const
    {
        return std::ranges::any_of(m_entries, [style](const EntryPtr& entry) {
            return &entry->style == style;
        });
    }

    bool StyleSheet::reaches(const TextStyle* style) const
    {
        const bool listed = std::ranges::any_of(m_entries, [style](const EntryPtr& entry) {
            return !entry->removed && !entry->style.name.empty() && &entry->style == style;
        });
        if (listed)
            return true;
        return m_parent && m_parent->reaches(style);
    }

    StyleSheet::StyleList StyleSheet::styles() const
    {
        StyleList result;
        for (const EntryPtr& entry : m_entries)
        {
            if (!entry->removed && !entry->style.name.empty())
                result.push_back(&entry->style);
        }
        return result;
    }

    bool StyleSheet::empty() const
    {
        return std::ranges::none_of(m_entries, [](const EntryPtr& entry) {
            return !entry->removed && !entry->style.name.empty();
        });
    }

    // Two sheets are equal when they list the same named styles in one order, each with the same
    // fields and a base alike: one of their own of one name, or one style up their parents.
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
            if (style.name != otherStyle.name || !sameFields(style, otherStyle))
                return false;
            if (!style.base || !otherStyle.base)
            {
                if (style.base != otherStyle.base)
                    return false;
                continue;
            }
            const bool own = holds(style.base);
            if (own != other.holds(otherStyle.base))
                return false;
            if (own ? style.base->name != otherStyle.base->name : style.base != otherStyle.base)
                return false;
        }
        return true;
    }

    // **An unnamed style is known by its values**, a name given with them aside, so two runs
    // stating the same over one base share an entry. Adding one is no edit: nothing names it.
    std::size_t StyleSheet::cache(const TextStyle& values)
    {
        TextStyle unnamed = values;
        unnamed.name.clear();
        for (std::size_t i = 0; i != m_entries.size(); ++i)
        {
            const Entry& entry = *m_entries[i];
            if (!entry.removed && entry.style == unnamed)
                return i;
        }
        EntryPtr added = std::make_unique<Entry>();
        added->style = std::move(unnamed);
        m_entries.push_back(std::move(added));
        return m_entries.size() - 1;
    }

    std::size_t StyleSheet::count() const
    {
        return m_entries.size();
    }

    const TextStyle& StyleSheet::at(std::size_t position) const
    {
        return m_entries[position]->style;
    }

    const TextStyle* StyleSheet::set(const TextStyle& source)
    {
        return state(source, false);
    }

    const TextStyle* StyleSheet::carry(const TextStyle& source)
    {
        return state(source, true);
    }

    // **A name the sheet holds is the sheet's style**, in a base as in a file: a base named as
    // one of the sheet's own is that one, and a style named as one up the parents stands in for it
    // in every base here. Such a style based on none is based on the one it restates. A base is
    // refused unless the sheet reaches it, and so is one leading back to the style being stated.
    // An edit is a change to a style held, or a style standing in for one it draws otherwise than.
    const TextStyle* StyleSheet::state(const TextStyle& source, bool quiet)
    {
        if (source.name.empty())
            return nullptr;

        const TextStyle* namesake = namesakeOf(source.name);
        const TextStyle* base = source.base ? source.base : namesake;
        if (base && !reaches(base))
            return nullptr;
        if (base && base != namesake && !holds(base))
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
            added->style.name = source.name;
            stateFields(*added, source);
            added->style.base = base;
            m_entries.push_back(std::move(added));
            Entry& stated = *m_entries.back();
            standInFor(stated, namesake);
            if (!quiet && namesake && drawsOtherwise(stated.style, namesake))
                edited();
            return &stated.style;
        }

        if (sameFields(entry->style, source) && entry->style.base == base)
            return &entry->style;
        stateFields(*entry, source);
        entry->style.base = base;
        if (!quiet)
            edited();
        return &entry->style;
    }

    // **A rename is an edit**: a format naming either name finds another style from then on. A
    // style renamed as one up the parents stands in for it, as set says, and one based on none is
    // based on it from then on.
    bool StyleSheet::rename(std::wstring_view name, std::wstring_view newName)
    {
        Entry* entry = liveEntry(name);
        if (!entry || newName.empty())
            return false;
        if (name == newName)
            return true;
        if (liveEntry(newName))
            return false;
        const TextStyle* namesake = namesakeOf(newName);
        if (leadsBack(entry->style.base, nullptr, namesake))
            return false;

        entry->style.name = newName;
        if (!entry->style.base && namesake)
            entry->style.base = namesake;
        standInFor(*entry, namesake);
        edited();
        return true;
    }

    // **A removed style stays where it stands**, so a text based on it draws as it did. The styles
    // based on it are based on its base instead - one restating a style up the parents on that one
    // where its base is none. A removal is an edit: a format naming the style finds another.
    bool StyleSheet::remove(std::wstring_view name)
    {
        Entry* entry = liveEntry(name);
        if (!entry)
            return false;

        entry->removed = true;
        for (const EntryPtr& other : m_entries)
        {
            if (other->removed || other->style.base != &entry->style)
                continue;
            const TextStyle* base = entry->style.base;
            if (!base)
                base = namesakeOf(other->style.name);
            other->style.base = base;
        }
        edited();
        return true;
    }

    // The order is the one a file is written in and a reader lists, and no text draws from it.
    bool StyleSheet::moveTo(std::wstring_view name, std::size_t position)
    {
        const Entries::iterator found = std::ranges::find_if(m_entries,
            [name](const EntryPtr& entry) {
                return !entry->removed && !entry->style.name.empty() && entry->style.name == name;
            });
        if (found == m_entries.end())
            return false;

        EntryPtr moved = std::move(*found);
        m_entries.erase(found);
        std::size_t listed = 0;
        Entries::iterator place = m_entries.begin();
        for (; place != m_entries.end(); ++place)
        {
            if ((*place)->removed || (*place)->style.name.empty())
                continue;
            if (listed == position)
                break;
            ++listed;
        }
        m_entries.insert(place, std::move(moved));
        return true;
    }

    // **An edit where a style already here changes, or goes**, as remove says, or where a style
    // added stands in for one up the parents that it draws otherwise than.
    void StyleSheet::takeStyles(const StyleSheet& other)
    {
        if (mergeFrom(other))
            edited();
    }

    void StyleSheet::swap(StyleSheet& other) noexcept
    {
        std::swap(m_parent, other.m_parent);
        m_entries.swap(other.m_entries);
    }

    StyleSheet::Entry* StyleSheet::liveEntry(std::wstring_view name)
    {
        return const_cast<Entry*>(std::as_const(*this).liveEntry(name));
    }

    const StyleSheet::Entry* StyleSheet::liveEntry(std::wstring_view name) const
    {
        for (const EntryPtr& entry : m_entries)
        {
            if (!entry->removed && !entry->style.name.empty() && entry->style.name == name)
                return entry.get();
        }
        return nullptr;
    }

    const TextStyle* StyleSheet::namesakeOf(std::wstring_view name) const
    {
        return m_parent ? m_parent->find(name) : nullptr;
    }

    // A chain from the base that reaches the style, or reaches its namesake past the base itself,
    // closes a loop once the style stands in for that namesake.
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

    // **A style named alike in both keeps its entry**, so a text based on it here keeps its style.
    // The named ones are listed in the other's order, what it names nowhere is removed and kept,
    // and the unnamed ones stay after them.
    bool StyleSheet::mergeFrom(const StyleSheet& other)
    {
        if (this == &other)
            return false;

        // One of the other's named styles, and whether an entry here named alike stands for it.
        struct Match
        {
            const Entry* source{ nullptr };
            bool kept{ false };
        };

        std::vector<Match> matches;
        Entries listed;
        for (const EntryPtr& source : other.m_entries)
        {
            if (source->removed || source->style.name.empty())
                continue;
            const Entries::iterator kept = std::ranges::find_if(m_entries,
                [&source](const EntryPtr& entry) {
                    return entry && !entry->removed && entry->style.name == source->style.name;
                });
            const bool found = kept != m_entries.end();
            EntryPtr entry = found ? std::move(*kept) : std::make_unique<Entry>();
            if (!found)
                entry->style.name = source->style.name;
            matches.push_back({ source.get(), found });
            listed.push_back(std::move(entry));
        }
        bool changed = false;
        for (EntryPtr& entry : m_entries)
        {
            if (!entry)
                continue;
            if (!entry->style.name.empty() && !entry->removed)
            {
                entry->removed = true;
                changed = true;
            }
            listed.push_back(std::move(entry));
        }
        m_entries = std::move(listed);

        // A base is found here by its name: one of the sheet's own first, and the namesake up the
        // parents for a style based on the one it restates.
        for (std::size_t i = 0; i != matches.size(); ++i)
        {
            const TextStyle& source = matches[i].source->style;
            Entry& entry = *m_entries[i];
            const TextStyle* base = nullptr;
            if (source.base)
            {
                const Entry* own = source.base->name == source.name
                    ? nullptr
                    : liveEntry(source.base->name);
                base = own ? &own->style : namesakeOf(source.base->name);
            }
            const bool same = sameFields(entry.style, source) && entry.style.base == base;
            stateFields(entry, source);
            entry.style.base = base;
            if (matches[i].kept)
            {
                changed = changed || !same;
                continue;
            }
            const TextStyle* namesake = namesakeOf(source.name);
            changed = changed || (namesake && drawsOtherwise(entry.style, namesake));
        }
        return changed;
    }

    void StyleSheet::stateFields(Entry& entry, const TextStyle& source)
    {
        entry.style.fontFamily = source.fontFamily;
        entry.style.fontSize = source.fontSize;
        entry.style.fontWeight = source.fontWeight;
        entry.style.fontStyle = source.fontStyle;
        entry.style.ink = source.ink;
        entry.style.scripts = source.scripts;
    }

    void StyleSheet::edited()
    {
        ++s_revision;
        events().emit<StylesEditedEvent>();
    }
}
