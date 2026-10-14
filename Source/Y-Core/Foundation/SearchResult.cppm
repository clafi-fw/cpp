export module ClaFi.Core.Foundation :SearchResult;

import :Control;

import ClaFi.Core.TextEngine.Searcher;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // Which match a step makes current, measured from where the reader stands.
    export enum class FindTarget
    {
        AtSelection,    // the first at or after it - what typing a search does
        Next,           // the first after it
        Previous        // the last before it
    };

    // What a searcher finds in the texts one container shows. See Control-Foundation#searchresult
    export class SearchResult
    {
    public:
        // Hands itself to the container, which keeps it for the paint.
        SearchResult(const Searcher&, Control& container);
        ~SearchResult();
        SearchResult(const SearchResult&) = delete;
        SearchResult(SearchResult&&) = delete;
        SearchResult& operator=(const SearchResult&) = delete;
        SearchResult& operator=(SearchResult&&) = delete;
    public:
        void setSearcher(const Searcher&);
        [[nodiscard]] const Searcher& searcher() const { return *m_searcher; }
        [[nodiscard]] std::size_t count() const;   // the matches the container's texts hold
        // Which match is current, counted from zero in reading order; none while none is.
        [[nodiscard]] std::optional<std::size_t> currentIndex() const;
        // The current match where it stands in that text of the holder; empty where it does not.
        [[nodiscard]] TextRange currentIn(const Control& holder, std::size_t part) const;
        [[nodiscard]] Control* currentHolder() const;   // the control the current match is in
        // Makes the match the target names current and shows it; false where there is none.
        bool step(FindTarget);
        // Makes the first match at or after where the control is read from current, and shows it.
        bool stepFrom(const Control&);
        void invalidate();   // a text it reads moved: read again when next asked
        // A control under the container is deleted: read again, the reader not looked for in it.
        void controlDeleted(const Control&);
    private:
        // A text holding matches: whose, which of its texts, how many, and where the walk met it.
        struct FoundText
        {
            Control* holder{ nullptr };
            std::size_t part{ 0 };
            std::size_t count{ 0 };
            std::size_t ordinal{ 0 };
        };
        // Where the reader stands: one text, and a range in it.
        struct ReaderPlace
        {
            const Control* holder{ nullptr };
            std::size_t part{ 0 };
            TextRange range{};
        };
        // The current match: its text among the found ones, and its place in that text.
        struct CurrentMatch
        {
            std::size_t text{ 0 };
            std::size_t place{ 0 };
            TextRange range{};
        };
        // The match a step last showed: the holder, which of its texts, and the range.
        struct ShownPlace
        {
            Control* holder{ nullptr };
            std::size_t part{ 0 };
            TextRange range{};
        };
        // What the last walk found, read when asked so that the questions stay const.
        struct Found
        {
            std::vector<FoundText> texts{};
            std::size_t count{ 0 };
            std::optional<ReaderPlace> reader{};
            std::size_t readerOrdinal{ 0 };   // where the walk met the reader's text
            std::optional<CurrentMatch> current{};
            std::size_t revision{ 0 };   // the searcher's, as the walk read it
            bool stale{ true };
        };
        using TextVisitor = std::function<void(Control& holder, std::size_t part,
            std::wstring_view, const TextRange* selection)>;
    private:
        void refresh() const;
        void walk(Control&, const TextVisitor&) const;
        void findPlaces(const FoundText&) const;   // into m_places
        // The text among the found ones a step lands in, and the place in it.
        [[nodiscard]] CurrentMatch targetOf(FindTarget) const;
        bool makeCurrent(CurrentMatch);   // and shows it
        // Opens what hides the control below the container; true where anything was opened.
        bool reveal(Control&);
    private:
        const Searcher* m_searcher;
        Control* m_container;   // null once it is gone
        mutable Found m_found{};
        std::optional<ShownPlace> m_shown{};   // kept apart from the reader, which a refresh moves
        mutable std::vector<TextRange> m_places{};   // what one text holds, the buffer reused
        ScopedEventConnection m_containerGone;
    };
}
