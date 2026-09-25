module ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // =========================================================================
    namespace {
        // Helper to categorize markers for pair matching
        enum class MarkerCat
        {
            None,
            Color,
            TextStyle,
            FontSize,
            FontFamily,
            Bold,
            Italic,
            Script,
            Link,
            Anchor,
            // How many there are, which is what a table indexed by one is sized from.
            Count
        };

        MarkerCat getPushCategory(const FormatItem& item)
        {
            return std::visit([](const auto& arg) -> MarkerCat {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, PushThemeColor> || std::is_same_v<T, PushCustomColor>)
                    return MarkerCat::Color;
                else if constexpr (std::is_same_v<T, PushTextStyle>)
                    return MarkerCat::TextStyle;
                else if constexpr (std::is_same_v<T, PushFontSize>)
                    return MarkerCat::FontSize;
                else if constexpr (std::is_same_v<T, PushFontFamily>)
                    return MarkerCat::FontFamily;
                else if constexpr (std::is_same_v<T, PushLink>)
                    return MarkerCat::Link;
                else if constexpr (std::is_same_v<T, PushAnchor>)
                    return MarkerCat::Anchor;
                else if constexpr (std::is_same_v<T, TextOp>)
                {
                    if (arg == TextOp::PushBold)
                        return MarkerCat::Bold;
                    if (arg == TextOp::PushItalic)
                        return MarkerCat::Italic;
                    if (arg == TextOp::PushSuperscript || arg == TextOp::PushSubscript)
                        return MarkerCat::Script;
                    return MarkerCat::None;
                }
                else
                    return MarkerCat::None;
                }, item);
        }

        MarkerCat getPopCategory(const FormatItem& item)
        {
            return std::visit([](const auto& arg) -> MarkerCat {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, PopColor>)
                    return MarkerCat::Color;
                else if constexpr (std::is_same_v<T, PopTextStyle>)
                    return MarkerCat::TextStyle;
                else if constexpr (std::is_same_v<T, PopFontSize>)
                    return MarkerCat::FontSize;
                else if constexpr (std::is_same_v<T, PopFontFamily>)
                    return MarkerCat::FontFamily;
                else if constexpr (std::is_same_v<T, PopLink>)
                    return MarkerCat::Link;
                else if constexpr (std::is_same_v<T, PopAnchor>)
                    return MarkerCat::Anchor;
                else if constexpr (std::is_same_v<T, TextOp>)
                {
                    if (arg == TextOp::PopBold)
                        return MarkerCat::Bold;
                    if (arg == TextOp::PopItalic)
                        return MarkerCat::Italic;
                    if (arg == TextOp::PopScript)
                        return MarkerCat::Script;
                    return MarkerCat::None;
                }
                else
                    return MarkerCat::None;
                }, item);
        }

        bool isPreservableMarker(const FormatItem& item) {
            return getPushCategory(item) != MarkerCat::None || getPopCategory(item) != MarkerCat::None;
        }

        // One flag per MarkerCat: which of them a text states for itself.
        using StatedCategories = std::array<bool, static_cast<std::size_t>(MarkerCat::Count)>;

        // The categories a text opens at its very start. Text::selectedText writes there every
        // run the selection stood inside, so a text taken out of a document and put back names
        // each of them, and a text that carries only what was typed names none.
        StatedCategories categoriesStatedAtStart(
            const std::vector<std::pair<std::size_t, FormatItem>>& markers)
        {
            StatedCategories stated{};
            for (const std::pair<std::size_t, FormatItem>& marker : markers)
            {
                if (marker.first != 0)
                    break;

                const MarkerCat category = getPushCategory(marker.second);
                if (category != MarkerCat::None)
                    stated[static_cast<std::size_t>(category)] = true;
            }
            return stated;
        }

        // Whether a marker standing where a text is put in closes a run that text is no part of.
        // A Pop there closes a run that opened before it, and what goes in JOINS that run - which
        // is how a character typed at the end of a bold word comes out bold - unless it states
        // that category for itself, and then the run closes in front of it.
        bool closesBeforeInsertion(const FormatItem& item, const StatedCategories& statedAtStart)
        {
            const MarkerCat category = getPopCategory(item);
            return category != MarkerCat::None && statedAtStart[static_cast<std::size_t>(category)];
        }

        // The target of the link the PopLink at position closes: the innermost push still open
        // before it. Walked back from the marker, so it costs the markers before it - and it is
        // asked only where one link closes and another opens at an insertion point.
        const std::wstring* closedLinkTarget(const Text::Markers& markers, std::size_t position)
        {
            std::size_t depth = 0;
            for (std::size_t i = position; i-- != 0;)
            {
                const FormatItem& item = markers[i].second;
                if (std::holds_alternative<PopLink>(item))
                {
                    ++depth;
                    continue;
                }
                const PushLink* push = std::get_if<PushLink>(&item);
                if (!push)
                    continue;
                if (depth == 0)
                    return &push->target;
                --depth;
            }
            return nullptr;
        }

        // Whether a link closing where a text is put in, with nothing replaced, stays in front of
        // it. A link is the one run that does not take in what is typed at its end - unless the
        // next link marker at that index opens a link naming the target this one closes. The two
        // read as one link, and what is typed where they meet goes inside it. An undo that puts a
        // link's last characters back is what leaves such a pair. See TextEngine-Types#links
        bool linkEndsBeforeInsertion(const Text::Markers& markers, std::size_t position)
        {
            if (!std::holds_alternative<PopLink>(markers[position].second))
                return false;

            // A link opened at this same index is empty and goes in the cleanup; it has no end to
            // keep the text out of, and moving its close in front of its open would change which
            // link each of them names.
            const std::size_t index = markers[position].first;
            std::size_t depth = 0;
            for (std::size_t before = position; before-- != 0 && markers[before].first == index;)
            {
                if (std::holds_alternative<PopLink>(markers[before].second))
                    ++depth;
                else if (std::holds_alternative<PushLink>(markers[before].second))
                {
                    if (depth == 0)
                        return false;
                    --depth;
                }
            }

            for (std::size_t next = position + 1;
                next != markers.size() && markers[next].first == index; ++next)
            {
                const FormatItem& item = markers[next].second;
                if (std::holds_alternative<PopLink>(item))
                    return true;
                if (const PushLink* push = std::get_if<PushLink>(&item))
                {
                    const std::wstring* closed = closedLinkTarget(markers, position);
                    return !closed || *closed != push->target;
                }
            }
            return true;
        }

        // The markers standing where a text is put in, once the ones that stay in front of it are
        // moved ahead of the rest there: [begin, begin + staying) are written in front of what goes
        // in, and the rest of that index behind it. Moved rather than tested in place, because the
        // vector has to stay in text order, and each part keeps the order it was in.
        struct Boundary
        {
            std::size_t begin{ 0 };
            std::size_t staying{ 0 };
            [[nodiscard]] bool stays(std::size_t position) const
            {
                return position >= begin && position < begin + staying;
            }
        };

        // Decides each marker at index while the markers stand as they were written, since
        // whether one stays can turn on what stands beside it.
        template<typename Stays>
        Boundary settleBoundary(Text::Markers& markers, std::size_t index, Stays stays)
        {
            Boundary result{};
            result.begin = static_cast<std::size_t>(std::lower_bound(markers.begin(), markers.end(),
                index, [](const Text::Marker& marker, std::size_t position){
                    return marker.first < position;
                }) - markers.begin());
            std::size_t end = result.begin;
            while (end != markers.size() && markers[end].first == index)
                ++end;

            // Empty for the common case, a character typed where no marker stands.
            std::vector<bool> staysHere{};
            bool ordered = true;
            for (std::size_t i = result.begin; i != end; ++i)
            {
                staysHere.push_back(stays(markers, i));
                if (staysHere.back())
                {
                    ordered = ordered && result.staying == i - result.begin;
                    ++result.staying;
                }
            }
            if (ordered)
                return result;

            Text::Markers moved{};
            moved.reserve(end - result.begin);
            for (const bool first : { true, false })
            {
                for (std::size_t i = result.begin; i != end; ++i)
                {
                    if (staysHere[i - result.begin] == first)
                        moved.push_back(std::move(markers[i]));
                }
            }
            std::move(moved.begin(), moved.end(),
                markers.begin() + static_cast<std::ptrdiff_t>(result.begin));
            return result;
        }

        struct OptionalPop { bool hasValue{ false }; FormatItem value; };

        OptionalPop createMatchingPop(const FormatItem& pushItem) {
            OptionalPop result;
            std::visit([&result](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, PushThemeColor> || std::is_same_v<T, PushCustomColor>) { result.hasValue = true; result.value = PopColor{}; }
                else if constexpr (std::is_same_v<T, PushTextStyle>) { result.hasValue = true; result.value = PopTextStyle{}; }
                else if constexpr (std::is_same_v<T, PushFontSize>) { result.hasValue = true; result.value = PopFontSize{}; }
                else if constexpr (std::is_same_v<T, PushFontFamily>) { result.hasValue = true; result.value = PopFontFamily{}; }
                else if constexpr (std::is_same_v<T, PushLink>)
                {
                    result.hasValue = true;
                    result.value = PopLink{};
                }
                else if constexpr (std::is_same_v<T, PushAnchor>)
                {
                    result.hasValue = true;
                    result.value = PopAnchor{};
                }
                else if constexpr (std::is_same_v<T, TextOp>) {
                    if (arg == TextOp::PushBold) { result.hasValue = true; result.value = TextOp::PopBold; }
                    else if (arg == TextOp::PushItalic) { result.hasValue = true; result.value = TextOp::PopItalic; }
                    else if (arg == TextOp::PushSuperscript || arg == TextOp::PushSubscript) { result.hasValue = true; result.value = TextOp::PopScript; }
                }
                }, pushItem);
            return result;
        }

        // The working state the annihilation pass reads and writes.
        struct AnnihilateScratch
        {
            std::array<std::vector<std::size_t>, static_cast<std::size_t>(MarkerCat::Count)> openPushes;
            std::vector<bool> annihilated;
        };

        // Held across calls rather than built per edit, the way the rasterizer's masks are: a text
        // of a hundred thousand markers is a hundred thousand flags, and a key press would
        // otherwise take them from the allocator and give them back.
        AnnihilateScratch g_annihilateScratch;

        // Takes out the Push/Pop pairs that both open and close at one text index: nothing
        // stands between them, so they change nothing. A Pop closes the NEAREST unclosed Push
        // of its category - the stack BakedText::rebuild walks - so Push(red), Push(blue), Pop
        // at one index leaves red in force.
        //
        // One pass with one stack per category finds every pair. Nothing is moved here - the
        // pairs are only MARKED, and the caller compacts the vector once for all of them, because
        // a group can hold a whole text's markers: Text::replaceText collapses every preservable
        // marker inside the replaced range onto its start, so a Select All followed by a Delete
        // puts all of them at one index.
        //
        // Answers how many entries it marked.
        std::size_t annihilateGroup(const std::vector<std::pair<std::size_t, FormatItem>>& markers,
            std::size_t from, std::size_t to)
        {
            for (std::vector<std::size_t>& openPushes : g_annihilateScratch.openPushes)
                openPushes.clear();

            std::size_t annihilatedCount = 0;
            for (std::size_t i = from; i != to; ++i)
            {
                const MarkerCat pushCat = getPushCategory(markers[i].second);
                if (pushCat != MarkerCat::None)
                {
                    g_annihilateScratch.openPushes[static_cast<std::size_t>(pushCat)].push_back(i);
                    continue;
                }

                const MarkerCat popCat = getPopCategory(markers[i].second);
                if (popCat == MarkerCat::None)
                    continue;

                std::vector<std::size_t>& openPushes =
                    g_annihilateScratch.openPushes[static_cast<std::size_t>(popCat)];
                // A Pop with nothing open closes a Push that stands earlier in the text, and
                // that is what it goes on saying.
                if (openPushes.empty())
                    continue;

                g_annihilateScratch.annihilated[openPushes.back()] = true;
                g_annihilateScratch.annihilated[i] = true;
                openPushes.pop_back();
                annihilatedCount += 2;
            }
            return annihilatedCount;
        }

        // MARKERS ARRIVE IN TEXT ORDER, and this does not sort them. Every route that builds a
        // marker vector writes it that way: both replaceText overloads write what stands before
        // the replaced range, then that range's own boundary, then the tail, and selectedText
        // writes the inherited state at 0, the selection's markers in order, and the closers at
        // the end. Sorting a whole text's markers to arrive back where they started is what an
        // edit used to pay on every key press - and the group walk below checks the property for
        // nothing, since it compares neighbouring indexes anyway.
        //
        // Nothing is moved unless a pair is actually taken out: a text where nothing annihilates
        // leaves this having read the vector and written none of it.
        void cleanupMarkers(std::vector<std::pair<std::size_t, FormatItem>>& markers)
        {
            if (markers.size() < 2)
                return;

            g_annihilateScratch.annihilated.assign(markers.size(), false);
            std::size_t annihilatedCount = 0;

            std::size_t groupStart = 0;
            while (groupStart != markers.size())
            {
                std::size_t groupEnd = groupStart + 1;
                while (groupEnd != markers.size() && markers[groupEnd].first == markers[groupStart].first)
                    ++groupEnd;

                if (groupEnd != markers.size() && markers[groupEnd].first < markers[groupStart].first)
                    unreachable("A marker vector reached cleanupMarkers out of text order");

                if (groupEnd - groupStart >= 2)
                    annihilatedCount += annihilateGroup(markers, groupStart, groupEnd);

                groupStart = groupEnd;
            }

            if (!annihilatedCount)
                return;

            std::size_t write = 0;
            for (std::size_t read = 0; read != markers.size(); ++read)
            {
                if (g_annihilateScratch.annihilated[read])
                    continue;

                if (write != read)
                    markers[write] = std::move(markers[read]);

                ++write;
            }
            markers.resize(write);
        }
    }
    // Implementations
    // =========================================================================

    Text::Text(std::wstring plainText, Markers markers)
        :
        m_plainText{ std::move(plainText) },
        m_markers{ std::move(markers) }
    {
    }

    Text& Text::operator<<(const std::wstring_view str) {
        if (!str.empty()) m_plainText.append(str);
        return *this;
    }
    Text& Text::operator<<(const std::wstring& str) {
        return operator<<(std::wstring_view{ str });
    }
    Text& Text::operator<<(const wchar_t* str) {
        return operator<<(std::wstring_view{ str });
    }
    Text& Text::operator<<(wchar_t chr) {
        m_plainText.append(&chr, 1); return *this;
    }
    Text& Text::operator<<(int value) {
        return operator<<(std::to_wstring(value));
    }
    Text& Text::operator<<(std::size_t value)
    {
        return operator<<(std::to_wstring(value));
    }
    Text& Text::operator<<(Ink ink) {
        m_markers.emplace_back(m_plainText.size(), PushThemeColor{ ink });
        return *this;
    }
    Text& Text::operator<<(InkGrade grade)
    {
        return *this << InkWell::textInk(grade);
    }
    Text& Text::operator<<(InkColor color)
    {
        Ink ink;
        ink.color = color;
        return *this << ink;
    }
    Text& Text::operator<<(const PushThemeColor& op) {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const PushCustomColor& op) {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(Color color)
    {
        return *this << PushCustomColor{ color };
    }
    Text& Text::operator<<(const PopColor& op) {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(TextStyleId op) {
        m_markers.emplace_back(m_plainText.size(), PushTextStyle{ op });
        return *this;
    }
    Text& Text::operator<<(const PopTextStyle& op) {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(TextAlign align) {
        m_markers.emplace_back(m_plainText.size(), align);
        return *this;
    }
    Text& Text::operator<<(const InTextIcon& obj) {
        m_markers.emplace_back(m_plainText.size(), obj);
        m_plainText.push_back(L'\uFFFC');
        return *this;
    }
    Text& Text::operator<<(const SetIndent& obj)
    {
        m_markers.emplace_back(m_plainText.size(), obj);
        //m_text.push_back(L'\uFFFC');
        return *this;
    }
    Text& Text::operator<<(const Space& obj) {
        m_markers.emplace_back(m_plainText.size(), obj);
        m_plainText.push_back(L'\uFFFC');
        return *this;
    }
    Text& Text::operator<<(const VSpace& obj) {
        m_markers.emplace_back(m_plainText.size(), obj);
        m_plainText.push_back(L'\uFFFC');
        return *this;
    }
    Text& Text::operator<<(const FlexSpace& obj) {
        m_markers.emplace_back(m_plainText.size(), obj);
        m_plainText.push_back(L'\uFFFC');
        return *this;
    }
    Text& Text::operator<<(const TabTo& obj) {
        m_markers.emplace_back(m_plainText.size(), obj);
        m_plainText.push_back(L'\uFFFC');
        return *this;
    }
    Text& Text::operator<<(TextOp op) {
        m_markers.emplace_back(m_plainText.size(), op);
        if (op == TextOp::EndLine)
            m_plainText.push_back(L'\n');
        return *this;
    }
    Text& Text::operator<<(const PushFontSize& op) {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const PopFontSize& op) {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const PushFontFamily& op) {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const PopFontFamily& op) {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const PushLink& op)
    {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const PopLink& op)
    {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const PushAnchor& op)
    {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const PopAnchor& op)
    {
        m_markers.emplace_back(m_plainText.size(), op);
        return *this;
    }
    Text& Text::operator<<(const Text& other) {
        if (other.m_plainText.empty() && other.m_markers.empty())
            return *this;
        std::size_t offset = m_plainText.size();
        m_plainText.append(other.m_plainText);
        m_markers.reserve(m_markers.size() + other.m_markers.size());
        for (const auto& marker : other.m_markers) m_markers.emplace_back(marker.first + offset, marker.second);
        return *this;
    }
    Text& Text::setIndent(float indent) {
        m_markers.emplace_back(m_plainText.size(), SetIndent{ indent });
        return *this;
    }
    Text& Text::setLineSpacing(float spacing) {
        m_markers.emplace_back(m_plainText.size(), SetLineSpacing{ spacing });
        return *this;
    }

    void Text::clear() {
        m_plainText.clear();
        m_markers.clear();
    }

    std::size_t Text::hash() const {
        // FNV-1a 64-bit hash
        std::size_t hash = 14695981039346656037ull;

        auto combine = [&hash](std::size_t val) {
            hash ^= val;
            hash *= 1099511628211ull;
        };

        auto combineFloat = [&combine](float f) {
            std::uint32_t bits;
            std::memcpy(&bits, &f, sizeof(float));
            combine(bits);
        };

        // 1. Hash the raw string
        for (wchar_t c : m_plainText) combine(c);

        // 2. Hash the formatting markers
        for (const auto& marker : m_markers) {
            combine(marker.first);             // The index position
            combine(marker.second.index());    // The std::variant type index

            std::visit([&](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, PushThemeColor>) {
                    // Every field an Ink carries. A hit is confirmed against the whole Text, so a
                    // field left out here costs a rebuild rather than a wrong colour: the two inks
                    // land on one key, and each paint evicts the other.
                    combine(static_cast<std::size_t>(arg.ink.color));
                    combineFloat(arg.ink.grade);
                }
                else if constexpr (std::is_same_v<T, PushCustomColor>) {
                    // The four bytes operator== compares.
                    combine(arg.color.asUint());
                }
                else if constexpr (std::is_same_v<T, PushTextStyle>) combine(static_cast<std::size_t>(arg.style));
                else if constexpr (std::is_same_v<T, TextAlign>) combine(static_cast<std::size_t>(arg));
                else if constexpr (std::is_same_v<T, SetIndent>) combineFloat(arg.indent);
                else if constexpr (std::is_same_v<T, SetLineSpacing>) combineFloat(arg.spacing);
                else if constexpr (std::is_same_v<T, InTextIcon>) {
                    combineFloat(arg.designWidth);
                    combineFloat(arg.designHeight);
                    combineFloat(arg.designBaseline);
                    combine(static_cast<std::size_t>(arg.tag.value));
                }
                else if constexpr (std::is_same_v<T, Space>) combineFloat(arg.width);
                else if constexpr (std::is_same_v<T, VSpace>) combineFloat(arg.height);
                else if constexpr (std::is_same_v<T, FlexSpace>) combine(1);
                else if constexpr (std::is_same_v<T, TabTo>) combineFloat(arg.targetX);
                else if constexpr (std::is_same_v<T, TextOp>) combine(static_cast<std::size_t>(arg));
                else if constexpr (std::is_same_v<T, PushFontSize>) combineFloat(arg.size);
                else if constexpr (std::is_same_v<T, PushFontFamily>) {
                    for (wchar_t c : arg.family) combine(c);
                }
                else if constexpr (std::is_same_v<T, PushLink>)
                {
                    for (const wchar_t c : arg.target)
                        combine(c);
                }
                else if constexpr (std::is_same_v<T, PushAnchor>)
                {
                    for (const wchar_t c : arg.name)
                        combine(c);
                }
            }, marker.second);
        }
        return hash;
    }
    bool Text::hasFlexSpace() const
    {
        for (const auto& marker : m_markers)
        {
            if (std::holds_alternative<FlexSpace>(marker.second))
                return true;
        }
        return false;
    }
    std::optional<TextRange> Text::anchorRange(std::wstring_view name) const
    {
        // The anchor ends at the first close that finds no anchor opened after it still open.
        std::size_t start = k_maxSize;
        std::size_t depth = 0;
        for (const Marker& marker : m_markers)
        {
            if (start == k_maxSize)
            {
                const PushAnchor* push = std::get_if<PushAnchor>(&marker.second);
                if (push && push->name == name)
                    start = marker.first;
                continue;
            }

            if (std::holds_alternative<PushAnchor>(marker.second))
            {
                ++depth;
                continue;
            }
            if (!std::holds_alternative<PopAnchor>(marker.second))
                continue;
            if (depth == 0)
                return TextRange{ start, marker.first - start };
            --depth;
        }

        if (start == k_maxSize)
            return std::nullopt;
        // An anchor left open holds the rest of the text.
        return TextRange{ start, m_plainText.size() - start };
    }
    void Text::replaceText(const TextRange& range, const std::wstring_view str) {
        std::size_t start = std::min(range.start, m_plainText.size());
        std::size_t length = std::min(range.length, m_plainText.size() - start);
        std::size_t end = start + length;

        const std::ptrdiff_t delta = static_cast<std::ptrdiff_t>(str.size()) - static_cast<std::ptrdiff_t>(length);

        // A link closing where the text goes in, with nothing replaced, stays in front of it.
        Boundary boundary{};
        if (length == 0)
            boundary = settleBoundary(m_markers, end, linkEndsBeforeInsertion);

        // In place. A marker before the replaced range does not move, one inside it either
        // collapses onto the range's start or goes, and one after it shifts by what the text
        // gained or lost - and all three keep the order they were in, so the vector stays sorted.
        // Building a second vector of the whole text to say that costs an allocation and a copy of
        // every marker, for an edit that usually touches none of them.
        std::size_t write = 0;
        for (std::size_t read = 0; read != m_markers.size(); ++read)
        {
            std::pair<std::size_t, FormatItem>& marker = m_markers[read];
            if (marker.first >= end && !boundary.stays(read))
            {
                marker.first = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(marker.first) + delta);
            }
            else if (marker.first >= start)
            {
                // Inside what went. A marker that opens or closes a run is kept and moved onto the
                // boundary, so the cleanup below can pair it off; anything else has nothing left
                // to apply to.
                if (!isPreservableMarker(marker.second))
                    continue;

                marker.first = start;
            }

            if (write != read)
                m_markers[write] = std::move(marker);

            ++write;
        }
        m_markers.resize(write);

        m_plainText.replace(start, length, str);
        cleanupMarkers(m_markers);
    }

    void Text::replaceText(const TextRange& range, const Text& other) {
        std::size_t start = std::min(range.start, m_plainText.size());
        std::size_t length = std::min(range.length, m_plainText.size() - start);
        std::size_t end = start + length;

        const std::ptrdiff_t delta = static_cast<std::ptrdiff_t>(other.m_plainText.size())
            - static_cast<std::ptrdiff_t>(length);

        const StatedCategories statedAtStart = categoriesStatedAtStart(other.m_markers);

        // The closers that stay put: one whose run the text going in states for itself, and a
        // link's where nothing is replaced. A text whose first marker is past its start states
        // nothing to be in front of, and a replaced range keeps its link, so with neither the
        // boundary is not read.
        const bool insertion = length == 0;
        const bool statesAtStart = !other.m_markers.empty() && other.m_markers.front().first == 0;
        const auto staysInFront = [&statedAtStart, insertion](const Markers& markers,
            std::size_t position){
            return closesBeforeInsertion(markers[position].second, statedAtStart)
                || (insertion && linkEndsBeforeInsertion(markers, position));
            };
        Boundary boundary{};
        if (statesAtStart || insertion)
            boundary = settleBoundary(m_markers, end, staysInFront);

        // In place, three parts in this order - what stands before the replaced range, that
        // range's own boundary, then the tail - the first two made by moving what is already here
        // rather than by copying a whole text's markers into a second vector. insertAt is where
        // the tail begins once that is done, which is where the inserted text's own markers
        // belong.
        std::size_t write = 0;
        std::size_t insertAt = k_maxSize;
        for (std::size_t read = 0; read != m_markers.size(); ++read)
        {
            std::pair<std::size_t, FormatItem>& marker = m_markers[read];
            // A closer that stays put is collapsed onto the range's start with the markers from
            // inside it: it closes a run the text going in is no part of, so it belongs in front
            // of that text rather than behind it.
            if (marker.first >= end && !boundary.stays(read))
            {
                if (insertAt == k_maxSize)
                    insertAt = write;

                marker.first = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(marker.first) + delta);
            }
            else if (marker.first >= start)
            {
                if (!isPreservableMarker(marker.second))
                    continue;

                marker.first = start;
            }

            if (write != read)
                m_markers[write] = std::move(marker);

            ++write;
        }
        m_markers.resize(write);
        if (insertAt == k_maxSize)
            insertAt = write;

        // A typed character carries none of these, which is what makes the common edit a walk and
        // nothing else.
        if (!other.m_markers.empty())
        {
            m_markers.insert(m_markers.begin() + insertAt,
                other.m_markers.begin(), other.m_markers.end());
            for (std::size_t i = insertAt; i != insertAt + other.m_markers.size(); ++i)
                m_markers[i].first += start;
        }

        m_plainText.replace(start, length, other.m_plainText);
        cleanupMarkers(m_markers);
    }

    Text Text::selectedText(const TextRange& range) const {
        Text result;
        std::size_t start = std::min(range.start, m_plainText.size());
        std::size_t length = std::min(range.length, m_plainText.size() - start);
        std::size_t end = start + length;

        if (length == 0) return result;

        result.m_plainText = m_plainText.substr(start, length);

        std::vector<FormatItem> activeStack;

        // For non-stackable "sticky" paragraph block states
        bool hasAlign = false; TextAlign lastAlign;
        bool hasIndent = false; SetIndent lastIndent;
        bool hasSpacing = false; SetLineSpacing lastSpacing;

        // 1. Process markers before 'start' to build the initial active state
        for (const auto& marker : m_markers) {
            if (marker.first >= start) break; // Stop at selection boundary

            const auto& item = marker.second;
            MarkerCat pushCat = getPushCategory(item);

            if (pushCat != MarkerCat::None) {
                activeStack.push_back(item);
            }
            else {
                MarkerCat popCat = getPopCategory(item);
                if (popCat != MarkerCat::None) {
                    // Find matching Push and remove it
                    for (auto it = activeStack.rbegin(); it != activeStack.rend(); ++it) {
                        if (getPushCategory(*it) == popCat) {
                            activeStack.erase(std::next(it).base());
                            break;
                        }
                    }
                }
                else {
                    // Check for sticky block states
                    std::visit([&](const auto& arg) {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, TextAlign>) { hasAlign = true; lastAlign = arg; }
                        else if constexpr (std::is_same_v<T, SetIndent>) { hasIndent = true; lastIndent = arg; }
                        else if constexpr (std::is_same_v<T, SetLineSpacing>) { hasSpacing = true; lastSpacing = arg; }
                        }, item);
                }
            }
        }

        // 2. Inject inherited active states at index 0 of the copied text
        if (hasAlign) result.m_markers.emplace_back(0, lastAlign);
        if (hasIndent) result.m_markers.emplace_back(0, lastIndent);
        if (hasSpacing) result.m_markers.emplace_back(0, lastSpacing);

        for (const auto& activeItem : activeStack) {
            result.m_markers.emplace_back(0, activeItem);
        }

        // 3. Process markers strictly inside the selection
        for (const auto& marker : m_markers) {
            if (marker.first >= end) break;
            if (marker.first >= start) {
                // Copy tag adjusting its index relative to the selection start
                result.m_markers.emplace_back(marker.first - start, marker.second);

                // Maintain stack to know what remains unclosed at the end
                const auto& item = marker.second;
                MarkerCat pushCat = getPushCategory(item);
                if (pushCat != MarkerCat::None) {
                    activeStack.push_back(item);
                }
                else {
                    MarkerCat popCat = getPopCategory(item);
                    if (popCat != MarkerCat::None) {
                        for (auto it = activeStack.rbegin(); it != activeStack.rend(); ++it) {
                            if (getPushCategory(*it) == popCat) {
                                activeStack.erase(std::next(it).base());
                                break;
                            }
                        }
                    }
                }
            }
        }

        // 4. Safely close all tags left unpopped (in reverse LIFO order)
        for (auto it = activeStack.rbegin(); it != activeStack.rend(); ++it) {
            auto popResult = createMatchingPop(*it);
            if (popResult.hasValue) {
                result.m_markers.emplace_back(length, popResult.value);
            }
        }

        // WHAT STANDS AT INDEX 0 IS THE STATE THE SELECTION INHERITED, and it stays there even
        // where the selection's own first marker closes it again. Annihilating that pair would
        // leave the text saying nothing about a run it stood inside, and that is what replaceText
        // reads to tell a run the text belongs to from one that closes where the text lands. The
        // pair is two markers and goes in the cleanup replaceText runs over the text it is put
        // into.

        return result;
    }
}
