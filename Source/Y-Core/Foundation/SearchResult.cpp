module ClaFi.Core.Foundation;

import :SearchResult;
import :Control;

import ClaFi.Core.TextEngine.Searcher;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // The place holding the range, ends included, so a caret against either end of a match
        // stands on it. A whole-text selection ends past every place.
        [[nodiscard]] std::optional<std::size_t> placeHolding(const std::vector<TextRange>& places,
            const TextRange& range)
        {
            const std::size_t after = static_cast<std::size_t>(
                std::ranges::upper_bound(places, range.start, {}, &TextRange::start)
                - places.begin());
            if (after == 0 || range.end() > places[after - 1].end())
                return std::nullopt;
            return after - 1;
        }
    }

    // SearchResult

    SearchResult::SearchResult(const Searcher& searcher, Control& container)
        :
        m_searcher{ &searcher },
        m_container{ &container },
        m_containerGone{ container.onDestroy([this](DestroyEvent&) {
            m_container = nullptr;
        }) }
    {
        container.setSearchResult(this);
        container.invalidate();
    }

    // **Either may go first.** A container gone takes its pointer back here through the
    // connection above, and a result going first takes itself back from the container.
    SearchResult::~SearchResult()
    {
        if (m_container)
            m_container->setSearchResult(nullptr);
    }

    void SearchResult::setSearcher(const Searcher& value)
    {
        m_searcher = &value;
        m_found.stale = true;
        if (m_container)
            m_container->invalidate();
    }

    std::size_t SearchResult::count() const
    {
        refresh();
        return m_found.count;
    }

    std::optional<std::size_t> SearchResult::currentIndex() const
    {
        refresh();
        if (!m_found.current)
            return std::nullopt;
        std::size_t result = m_found.current->place;
        for (std::size_t i = 0; i != m_found.current->text; ++i)
            result += m_found.texts[i].count;
        return result;
    }

    // **Asked by every text painted under the search**, so a text the reader does not stand in
    // answers before anything is read again.
    TextRange SearchResult::currentIn(const Control& holder, const std::size_t part) const
    {
        const std::optional<ReaderPlace>& reader = m_found.reader;
        if (!reader || reader->holder != &holder || reader->part != part)
            return {};
        refresh();
        return m_found.current ? m_found.current->range : TextRange{};
    }

    bool SearchResult::step(const FindTarget target)
    {
        refresh();
        if (m_found.count == 0)
            return false;

        // The holder the mark leaves, repainted once it has moved. A current match is one the
        // walk just taken met, so the holder stands.
        const Control* left = m_found.current ? m_found.reader->holder : nullptr;
        CurrentMatch next = targetOf(target);
        const FoundText& text = m_found.texts[next.text];
        findPlaces(text);
        next.range = m_places[next.place];
        Control& holder = *text.holder;
        const std::size_t part = text.part;
        m_found.reader = ReaderPlace{ &holder, part, next.range };
        m_found.readerOrdinal = text.ordinal;
        m_found.current = next;

        // **Shown last**: a holder keeping a selection moves it, and what hears of the move may ask
        // this result again, which reads everything anew.
        const bool revealed = reveal(holder);
        holder.showFound(part, next.range);
        // What was hidden has not been laid out yet, so it is scrolled to once it has.
        if (revealed)
            holder.scrollIntoViewOnAlign();
        if (left && left != &holder)
            left->invalidate();
        holder.invalidate();
        return true;
    }

    void SearchResult::invalidate()
    {
        m_found.stale = true;
    }

    // **The reader is looked for again.** Met, it follows the selection its text keeps, and the
    // current match is the place it stands on. Not met, its text is gone, and the first selection
    // the walk met stands in for it.
    void SearchResult::refresh() const
    {
        Found& found = m_found;
        if (!found.stale && found.revision == m_searcher->revision())
            return;
        found.stale = false;
        found.revision = m_searcher->revision();
        found.texts.clear();
        found.count = 0;
        found.current.reset();
        const std::optional<ReaderPlace> previous = std::exchange(found.reader, std::nullopt);
        // A search for nothing finds nothing, and the reader stays where it was.
        if (!m_container || m_searcher->empty())
        {
            found.reader = previous;
            return;
        }

        std::optional<ReaderPlace> firstSelection;
        std::size_t firstSelectionOrdinal = 0;
        std::size_t ordinal = 0;
        const auto read = [&](Control& holder, const std::size_t part,
            const std::wstring_view text, const TextRange* selection) {
            const bool isReader = previous && previous->holder == &holder
                && previous->part == part;
            if (isReader)
            {
                const TextRange range = selection ? *selection : previous->range;
                found.reader = ReaderPlace{ &holder, part, range };
                found.readerOrdinal = ordinal;
            }
            else if (selection && !firstSelection)
            {
                firstSelection = ReaderPlace{ &holder, part, *selection };
                firstSelectionOrdinal = ordinal;
            }
            m_searcher->findIn(text, m_places);
            if (m_places.empty())
            {
                ++ordinal;
                return;
            }
            const std::optional<std::size_t> place = isReader
                ? placeHolding(m_places, found.reader->range)
                : std::nullopt;
            if (place)
                found.current = CurrentMatch{ found.texts.size(), *place, m_places[*place] };
            found.texts.push_back({ &holder, part, m_places.size(), ordinal });
            found.count += m_places.size();
            ++ordinal;
        };
        walk(*m_container, read);
        if (!found.reader && firstSelection)
        {
            found.reader = firstSelection;
            found.readerOrdinal = firstSelectionOrdinal;
        }
    }

    // **What the search reads**: the control's own texts, then its children in the order it
    // holds them - a hidden one only where the control opens it, and none the control is made of.
    // A container keeping a search of its own is read by that one.
    void SearchResult::walk(Control& control, const TextVisitor& visit) const
    {
        control.searchTexts([&](const std::size_t part, const std::wstring_view text,
            const TextRange* selection) {
            visit(control, part, text, selection);
        });
        for (ControlPtr& child : control.controls())
        {
            const ChildSearch search = control.childSearch(*child);
            if (search == ChildSearch::Skipped)
                continue;
            if (search == ChildSearch::Searched && !child->visible())
                continue;
            const SearchResult* own = child->searchResult();
            if (own && own != this)
                continue;
            walk(*child, visit);
        }
    }

    void SearchResult::findPlaces(const FoundText& text) const
    {
        m_places.clear();
        text.holder->searchTexts([&](const std::size_t part, const std::wstring_view shown,
            const TextRange*) {
            if (part == text.part)
                m_searcher->findIn(shown, m_places);
        });
    }

    // **Measured from the reader**: from the place it stands on in its text, or from the text it
    // stands in where that holds no match - the next text holding one, or the last one before.
    // Past either end is the other end. With no reader, the first match, or the last going back.
    SearchResult::CurrentMatch SearchResult::targetOf(const FindTarget target) const
    {
        const Found& found = m_found;
        const std::size_t last = found.texts.size() - 1;
        const auto lastOf = [&found](const std::size_t text) {
            return CurrentMatch{ text, found.texts[text].count - 1 };
        };
        if (!found.reader)
            return target == FindTarget::Previous ? lastOf(last) : CurrentMatch{};

        const std::size_t split = static_cast<std::size_t>(
            std::ranges::partition_point(found.texts, [&found](const FoundText& text) {
                return text.ordinal < found.readerOrdinal;
            }) - found.texts.begin());
        const bool readerHolds = split != found.texts.size()
            && found.texts[split].ordinal == found.readerOrdinal;
        if (!readerHolds)
        {
            if (target == FindTarget::Previous)
                return lastOf(split == 0 ? last : split - 1);
            return CurrentMatch{ split == found.texts.size() ? 0 : split };
        }

        findPlaces(found.texts[split]);
        const TextRange& reader = found.reader->range;
        const std::size_t atOrAfter = static_cast<std::size_t>(
            std::ranges::lower_bound(m_places, reader.start, {}, &TextRange::start)
            - m_places.begin());
        std::size_t place = atOrAfter;
        switch (target)
        {
            case FindTarget::AtSelection:
                break;
            case FindTarget::Next:
                place = static_cast<std::size_t>(
                    std::ranges::upper_bound(m_places, reader.start, {}, &TextRange::start)
                    - m_places.begin());
                break;
            case FindTarget::Previous:
            {
                // Back from the place the reader stands on, or from the first after it.
                const std::size_t from = placeHolding(m_places, reader).value_or(atOrAfter);
                if (from == 0)
                    return lastOf(split == 0 ? last : split - 1);
                return CurrentMatch{ split, from - 1 };
            }
        }
        if (place == m_places.size())
            return CurrentMatch{ split == last ? 0 : split + 1 };
        return CurrentMatch{ split, place };
    }

    // From the container down, so that an outer body is opened ahead of one inside it.
    bool SearchResult::reveal(Control& control)
    {
        Control* parent = control.parent();
        if (&control == m_container || !parent)
            return false;
        bool revealed = reveal(*parent);
        if (!control.visible() && parent->childSearch(control) == ChildSearch::Revealed)
        {
            parent->revealChild(control);
            revealed = true;
        }
        return revealed;
    }
}
