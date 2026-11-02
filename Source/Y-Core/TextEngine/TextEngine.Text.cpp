module ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // The look of a run that states nothing, which every look index 0 stands for.
        const TextLook k_noLook{};

        // An index of another text's table not yet looked up in this one's.
        constexpr std::uint32_t k_unmapped = std::numeric_limits<std::uint32_t>::max();

        constexpr std::size_t k_fnvOffset = 14695981039346656037ull;
        constexpr std::size_t k_fnvPrime = 1099511628211ull;

        // The ink an InkColor stands for: that colour at Strongest.
        [[nodiscard]] Ink inkOf(InkColor color)
        {
            Ink ink;
            ink.color = color;
            return ink;
        }

        // One field of an inner look under an outer one. The inner keeps what it states, itself
        // or through its style. The outer fills the rest from its own fields, and from its style
        // where the inner keeps a style of its own - otherwise the inner takes the outer's style
        // whole and it fills those fields itself.
        template<typename LookValue, typename StyleValue>
        void fillField(TextLook& result, const TextLook& inner, const TextLook& outer,
            std::optional<LookValue> TextLook::* own, std::optional<StyleValue> TextStyle::* styled,
            bool takesOuterStyle)
        {
            if ((result.*own).has_value())
                return;
            if (inner.style && inner.style->stated(styled).has_value())
                return;
            if ((outer.*own).has_value())
            {
                result.*own = outer.*own;
                return;
            }
            if (takesOuterStyle || !outer.style)
                return;
            if (const std::optional<StyleValue> value = outer.style->stated(styled))
                result.*own = LookValue{ *value };
        }

        // An inner look laid under an outer one: the inner wins every field it states, and the
        // outer fills the rest. See TextEngine-Types#textlook
        [[nodiscard]] TextLook laidUnder(const TextLook& inner, const TextLook& outer,
            bool innerLinked)
        {
            if (outer.empty())
                return inner;
            if (inner.empty() && !innerLinked)
                return outer;

            TextLook result = inner;
            const bool takesOuterStyle = !inner.style;
            if (takesOuterStyle)
                result.style = outer.style;
            fillField(result, inner, outer, &TextLook::fontFamily, &TextStyle::fontFamily,
                takesOuterStyle);
            fillField(result, inner, outer, &TextLook::fontSize, &TextStyle::fontSize,
                takesOuterStyle);
            fillField(result, inner, outer, &TextLook::fontWeight, &TextStyle::fontWeight,
                takesOuterStyle);
            fillField(result, inner, outer, &TextLook::fontStyle, &TextStyle::fontStyle,
                takesOuterStyle);

            // A link inside states its ink, so an outer one does not reach the words. A style
            // taken whole would, and the link's ink is stated against it.
            if (!innerLinked)
                fillField(result, inner, outer, &TextLook::ink, &TextStyle::ink, takesOuterStyle);
            else if (takesOuterStyle && !result.ink && outer.style
                && outer.style->stated(&TextStyle::ink).has_value())
                result.ink = k_linkInk;

            result.scripts.insert(result.scripts.begin(), outer.scripts.begin(),
                outer.scripts.end());
            return result;
        }

        // An inner paragraph look under an outer one, the inner winning every field it states.
        [[nodiscard]] ParagraphLook laidUnder(const ParagraphLook& inner, const ParagraphLook& outer)
        {
            return {
                .align = inner.align ? inner.align : outer.align,
                .indent = inner.indent ? inner.indent : outer.indent,
                .lineSpacing = inner.lineSpacing ? inner.lineSpacing : outer.lineSpacing,
            };
        }

        void combine(std::size_t& hash, std::size_t value)
        {
            hash ^= value;
            hash *= k_fnvPrime;
        }

        void combine(std::size_t& hash, float value)
        {
            std::uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(float));
            combine(hash, static_cast<std::size_t>(bits));
        }

        void combine(std::size_t& hash, std::wstring_view words)
        {
            for (const wchar_t character : words)
                combine(hash, static_cast<std::size_t>(character));
        }

        void combine(std::size_t& hash, FontWeight weight)
        {
            combine(hash, static_cast<std::size_t>(weight));
        }

        void combine(std::size_t& hash, FontStyle style)
        {
            combine(hash, static_cast<std::size_t>(style));
        }

        void combine(std::size_t& hash, TextAlign align)
        {
            combine(hash, static_cast<std::size_t>(align));
        }

        // Every field an Ink carries. A hit is confirmed against the whole Text, so a field left
        // out here costs a rebuild rather than a wrong colour: the two inks land on one key, and
        // each paint evicts the other. Named apart from combine, whose size_t calls would
        // otherwise be offered ColorDef's converting constructor.
        void combineInk(std::size_t& hash, const ColorDef& ink)
        {
            combine(hash, ink.index());
            if (const Ink* themed = std::get_if<Ink>(&ink))
            {
                combine(hash, static_cast<std::size_t>(themed->color));
                combine(hash, themed->grade);
            }
            else
            {
                combine(hash, static_cast<std::size_t>(std::get<Color>(ink).asUint()));
            }
        }

        // An absent value and a present one hash apart, whatever the value.
        template<typename Value>
        void combine(std::size_t& hash, const std::optional<Value>& value)
        {
            combine(hash, static_cast<std::size_t>(value.has_value()));
            if (value)
                combine(hash, *value);
        }

        void combineLook(std::size_t& hash, const TextLook& look)
        {
            combine(hash, reinterpret_cast<std::size_t>(look.style));
            combine(hash, look.fontFamily);
            combine(hash, look.fontSize);
            combine(hash, look.fontWeight);
            combine(hash, look.fontStyle);
            combine(hash, static_cast<std::size_t>(look.ink.has_value()));
            if (look.ink)
                combineInk(hash, *look.ink);
            for (const TextScript script : look.scripts)
                combine(hash, static_cast<std::size_t>(script) + 1);
        }

        void combineParagraph(std::size_t& hash, const ParagraphLook& look)
        {
            combine(hash, look.align);
            combine(hash, look.indent);
            combine(hash, look.lineSpacing);
        }

        void combineObject(std::size_t& hash, const InlineObject& object)
        {
            combine(hash, object.index());
            std::visit([&hash](const auto& held){
                using Held = std::decay_t<decltype(held)>;
                if constexpr (std::is_same_v<Held, InTextIcon>)
                {
                    combine(hash, held.designWidth);
                    combine(hash, held.designHeight);
                    combine(hash, held.designBaseline);
                    combine(hash, static_cast<std::size_t>(held.tag.value));
                }
                else if constexpr (std::is_same_v<Held, Space>)
                    combine(hash, held.width);
                else if constexpr (std::is_same_v<Held, VSpace>)
                    combine(hash, held.height);
                else if constexpr (std::is_same_v<Held, FlexSpace>)
                    combine(hash, held.minWidth);
                else if constexpr (std::is_same_v<Held, TabTo>)
                    combine(hash, held.targetX);
            }, object);
        }
    }

    // Text

    // A paragraph's piece states the paragraph the text has reached, ahead of its words.
    Text::Text(TextPieces pieces)
    {
        for (const TextPiece& piece : pieces)
        {
            if (piece.paragraph)
                stateParagraph(paragraphStartOf(m_plainText.size()), *piece.paragraph);

            const Run run = {
                .look = lookIndexOf(piece.look),
                .link = nameIndexOf(piece.link),
                .anchor = nameIndexOf(piece.anchor),
            };
            if (piece.object)
            {
                appendObject(*piece.object);
                m_runs.back().look = run.look;
                m_runs.back().link = run.link;
                m_runs.back().anchor = run.anchor;
                continue;
            }
            if (piece.words.empty())
                continue;
            m_plainText.append(piece.words);
            Run words = run;
            words.length = piece.words.size();
            appendRun(words);
        }
    }

    Text& Text::operator<<(const std::wstring_view words)
    {
        if (words.empty())
            return *this;
        breakOwnLine();
        appendWords(words);
        return *this;
    }

    Text& Text::operator<<(const std::wstring& words)
    {
        return operator<<(std::wstring_view{ words });
    }

    Text& Text::operator<<(const wchar_t* words)
    {
        return operator<<(std::wstring_view{ words });
    }

    Text& Text::operator<<(wchar_t character)
    {
        breakOwnLine();
        appendWords(std::wstring_view{ &character, 1 });
        return *this;
    }

    Text& Text::operator<<(int value)
    {
        return operator<<(std::to_wstring(value));
    }

    Text& Text::operator<<(std::size_t value)
    {
        return operator<<(std::to_wstring(value));
    }

    Text& Text::operator<<(const InTextIcon& icon)
    {
        breakOwnLine();
        appendObject(icon);
        return *this;
    }

    Text& Text::operator<<(const Space& space)
    {
        breakOwnLine();
        appendObject(space);
        return *this;
    }

    Text& Text::operator<<(const VSpace& space)
    {
        breakOwnLine();
        appendObject(space);
        return *this;
    }

    Text& Text::operator<<(const FlexSpace& space)
    {
        breakOwnLine();
        appendObject(space);
        return *this;
    }

    Text& Text::operator<<(const TabTo& tab)
    {
        breakOwnLine();
        appendObject(tab);
        return *this;
    }

    // A text whose words own their first or last line keeps them on it wherever it is put. Its
    // first paragraph's look goes where it starts a paragraph, and a line it joins keeps its own.
    Text& Text::operator<<(const Text& other)
    {
        if (&other == this)
        {
            const Text copy{ other };
            return *this << copy;
        }
        if (other.empty())
            return *this;

        breakOwnLine();
        if (m_plainText.empty())
            m_startsOwnLine = other.m_startsOwnLine;
        else if (other.m_startsOwnLine && m_plainText.back() != k_endLine)
            appendWords(std::wstring_view{ &k_endLine, 1 });

        const std::size_t offset = m_plainText.size();
        const bool startsAParagraph = startsParagraph(offset);
        const Runs added = runsOf(other, 0, other.m_plainText.size());
        m_plainText.append(other.m_plainText);
        for (const Run& run : added)
            appendRun(run);
        for (const StatedParagraph& stated : other.m_paragraphs)
        {
            if (stated.start == 0 && !startsAParagraph)
                continue;
            stateParagraph(offset + stated.start, stated.look);
        }
        m_endsOwnLine = other.m_endsOwnLine;
        return *this;
    }

    void Text::clear()
    {
        m_plainText.clear();
        m_runs.clear();
        m_looks.clear();
        m_names.clear();
        m_objects.clear();
        m_paragraphs.clear();
        m_startsOwnLine = false;
        m_endsOwnLine = false;
    }

    std::size_t Text::hash() const
    {
        std::size_t hash = k_fnvOffset;
        combine(hash, std::wstring_view{ m_plainText });
        for (const Run& run : m_runs)
        {
            combine(hash, run.length);
            combineLook(hash, lookAt(run.look));
            combine(hash, nameAt(run.link));
            combine(hash, static_cast<std::size_t>(run.link != 0));
            combine(hash, nameAt(run.anchor));
            combine(hash, static_cast<std::size_t>(run.anchor != 0));
            if (run.object)
                combineObject(hash, m_objects[run.object - 1]);
        }
        for (const StatedParagraph& stated : m_paragraphs)
        {
            combine(hash, stated.start);
            combineParagraph(hash, stated.look);
        }
        return hash;
    }

    bool Text::hasFlexSpace() const
    {
        for (const Run& run : m_runs)
        {
            if (run.object && std::holds_alternative<FlexSpace>(m_objects[run.object - 1]))
                return true;
        }
        return false;
    }

    // By what each run says, since two texts hold their tables in the order they were written.
    bool Text::operator==(const Text& other) const
    {
        if (m_plainText != other.m_plainText || m_runs.size() != other.m_runs.size()
            || m_paragraphs != other.m_paragraphs)
        {
            return false;
        }
        for (std::size_t i = 0; i != m_runs.size(); ++i)
        {
            const Run& run = m_runs[i];
            const Run& otherRun = other.m_runs[i];
            if (run.length != otherRun.length || (run.object == 0) != (otherRun.object == 0))
                return false;
            if (lookAt(run.look) != other.lookAt(otherRun.look))
                return false;
            if (nameAt(run.link) != other.nameAt(otherRun.link)
                || nameAt(run.anchor) != other.nameAt(otherRun.anchor))
            {
                return false;
            }
            if (run.object && m_objects[run.object - 1] != other.m_objects[otherRun.object - 1])
                return false;
        }
        return true;
    }

    ParagraphLook Text::paragraphLookAt(std::size_t paragraphStart) const
    {
        const StatedParagraph* stated = statedAt(paragraphStart);
        return stated ? stated->look : ParagraphLook{};
    }

    // A run a paragraph starts inside is cut there, so each paragraph's piece stands where its
    // words begin.
    TextPieces Text::pieces() const
    {
        TextPieces result;
        std::size_t nextParagraph = 0;
        const auto stateParagraphsUpTo = [this, &result, &nextParagraph](std::size_t pos){
            while (nextParagraph != m_paragraphs.size() && m_paragraphs[nextParagraph].start <= pos)
            {
                result.push_back(TextPiece{ .paragraph = m_paragraphs[nextParagraph].look });
                ++nextParagraph;
            }
        };

        std::size_t start = 0;
        for (const Run& run : m_runs)
        {
            const std::size_t end = start + run.length;
            std::size_t from = start;
            while (from != end)
            {
                stateParagraphsUpTo(from);
                const std::size_t to = nextParagraph != m_paragraphs.size()
                    && m_paragraphs[nextParagraph].start < end
                    ? m_paragraphs[nextParagraph].start
                    : end;
                TextPiece piece{
                    .words = m_plainText.substr(from, to - from),
                    .look = lookAt(run.look),
                    .link = std::wstring{ nameAt(run.link) },
                    .anchor = std::wstring{ nameAt(run.anchor) },
                };
                if (run.object)
                    piece.object = m_objects[run.object - 1];
                result.push_back(std::move(piece));
                from = to;
            }
            start = end;
        }
        stateParagraphsUpTo(m_plainText.size());
        return result;
    }

    // An anchor runs on through the runs that carry it, as one wrapper's words do.
    std::optional<TextRange> Text::anchorRange(std::wstring_view name) const
    {
        std::size_t start = 0;
        for (std::size_t i = 0; i != m_runs.size(); ++i)
        {
            if (!m_runs[i].anchor || nameAt(m_runs[i].anchor) != name)
            {
                start += m_runs[i].length;
                continue;
            }
            std::size_t length = 0;
            for (std::size_t j = i; j != m_runs.size() && m_runs[j].anchor == m_runs[i].anchor; ++j)
                length += m_runs[j].length;
            return TextRange{ start, length };
        }
        return std::nullopt;
    }

    // The character before the caret is asked first, so where two links meet at the caret it is
    // the ending one.
    std::optional<LinkSpan> Text::linkAtCaret(const std::size_t caretPos) const
    {
        for (const std::size_t pos : { caretPos - 1, caretPos })
        {
            if (pos >= m_plainText.size())
                continue;
            const RunPlace place = placeOf(pos);
            const std::uint32_t link = m_runs[place.index].link;
            if (!link)
                continue;

            std::size_t first = place.index;
            std::size_t start = place.start;
            while (first != 0 && m_runs[first - 1].link == link)
            {
                --first;
                start -= m_runs[first].length;
            }
            std::size_t length = 0;
            for (std::size_t i = first; i != m_runs.size() && m_runs[i].link == link; ++i)
                length += m_runs[i].length;
            return LinkSpan{ { start, length }, nameAt(link) };
        }
        return std::nullopt;
    }

    std::optional<std::wstring_view> Text::firstLink() const
    {
        for (const Run& run : m_runs)
        {
            if (run.link)
                return nameAt(run.link);
        }
        return std::nullopt;
    }

    void Text::replaceText(const TextRange& range, const std::wstring_view words)
    {
        edit(range, Text{ words }, Insertion::Nested);
    }

    void Text::replaceText(const TextRange& range, const Text& inserted)
    {
        edit(range, inserted, Insertion::Nested);
    }

    // The look of the paragraph the range starts in goes back where it is given - the one an edit
    // that emptied that paragraph replaced.
    void Text::restoreText(const TextRange& range, const Text& taken,
        const std::optional<ParagraphLook>& startParagraphLook)
    {
        const std::size_t start = std::min(range.start, m_plainText.size());
        edit(range, taken, Insertion::Restored);
        if (startParagraphLook.has_value())
            stateParagraph(paragraphStartOf(start), startParagraphLook.value());
    }

    // A range starting where a paragraph starts carries that paragraph's look, so a copy of whole
    // paragraphs reads as they look.
    Text Text::selectedText(const TextRange& range) const
    {
        Text result;
        const std::size_t start = std::min(range.start, m_plainText.size());
        const std::size_t end = start + std::min(range.length, m_plainText.size() - start);

        result.m_plainText = m_plainText.substr(start, end - start);
        result.m_runs = result.runsOf(*this, start, end);
        for (const StatedParagraph& stated : m_paragraphs)
        {
            if (stated.start >= start && stated.start <= end)
                result.m_paragraphs.push_back({ stated.start - start, stated.look });
        }
        return result;
    }

    // **The paragraph the range starts in keeps its look through the edit**, so what restoreText
    // puts back holds none for it. A look at the start would also be misread once two removals
    // are merged into one undo step: appended at a paragraph start, it would stand for the
    // paragraph after the join.
    Text Text::restorableText(const TextRange& range) const
    {
        Text result = selectedText(range);
        if (!result.m_paragraphs.empty() && result.m_paragraphs.front().start == 0)
            result.m_paragraphs.erase(result.m_paragraphs.begin());
        return result;
    }

    bool Text::joins(const Run& before, const Run& after)
    {
        return !before.object && !after.object && before.look == after.look
            && before.link == after.link && before.anchor == after.anchor;
    }

    const TextLook& Text::lookAt(std::uint32_t look) const
    {
        return look ? m_looks[look - 1] : k_noLook;
    }

    std::wstring_view Text::nameAt(std::uint32_t name) const
    {
        return name ? std::wstring_view{ m_names[name - 1] } : std::wstring_view{};
    }

    // A text holds a look once, so two runs in one look carry one index.
    std::uint32_t Text::lookIndexOf(const TextLook& look)
    {
        if (look.empty())
            return 0;
        const Looks::const_iterator found = std::ranges::find(m_looks, look);
        if (found != m_looks.end())
            return static_cast<std::uint32_t>(found - m_looks.begin()) + 1;
        m_looks.push_back(look);
        return static_cast<std::uint32_t>(m_looks.size());
    }

    std::uint32_t Text::nameIndexOf(std::wstring_view name)
    {
        if (name.empty())
            return 0;
        const Names::const_iterator found = std::ranges::find(m_names, name);
        if (found != m_names.end())
            return static_cast<std::uint32_t>(found - m_names.begin()) + 1;
        m_names.emplace_back(name);
        return static_cast<std::uint32_t>(m_names.size());
    }

    const Text::StatedParagraph* Text::statedAt(std::size_t paragraphStart) const
    {
        const StatedParagraphs::const_iterator found = std::ranges::lower_bound(m_paragraphs,
            paragraphStart, std::less<>{}, &StatedParagraph::start);
        if (found == m_paragraphs.end() || found->start != paragraphStart)
            return nullptr;
        return &*found;
    }

    std::size_t Text::paragraphStartOf(std::size_t pos) const
    {
        if (pos == 0)
            return 0;
        const std::size_t lineEnd = m_plainText.rfind(k_endLine, pos - 1);
        return lineEnd == std::wstring::npos ? 0 : lineEnd + 1;
    }

    bool Text::startsParagraph(std::size_t pos) const
    {
        return pos == 0 || m_plainText[pos - 1] == k_endLine;
    }

    // Walked from whichever end is nearer: a wrapper reaches back from the end of the text it is
    // writing, and a caret is anywhere. Past the last character is the end of the runs.
    Text::RunPlace Text::placeOf(std::size_t pos) const
    {
        if (pos >= m_plainText.size())
            return { m_runs.size(), m_plainText.size() };

        if (pos < m_plainText.size() / 2)
        {
            std::size_t start = 0;
            for (std::size_t i = 0; i != m_runs.size(); ++i)
            {
                if (pos < start + m_runs[i].length)
                    return { i, start };
                start += m_runs[i].length;
            }
        }
        else
        {
            std::size_t start = m_plainText.size();
            for (std::size_t i = m_runs.size(); i-- != 0;)
            {
                start -= m_runs[i].length;
                if (start <= pos)
                    return { i, start };
            }
        }
        noReach("A text's runs do not cover its characters");
    }

    // The index of the run that starts at pos, cutting the run that holds it in two.
    std::size_t Text::splitAt(std::size_t pos)
    {
        const RunPlace place = placeOf(pos);
        if (place.index == m_runs.size() || place.start == pos)
            return place.index;

        Run tail = m_runs[place.index];
        tail.length = place.start + tail.length - pos;
        m_runs[place.index].length = pos - place.start;
        m_runs.insert(m_runs.begin() + static_cast<std::ptrdiff_t>(place.index) + 1, tail);
        return place.index + 1;
    }

    // Joins the runs from the one before first to the one at last wherever two in a row are one
    // look, so the runs stay the fewest that say what the text says.
    void Text::joinRuns(std::size_t first, std::size_t last)
    {
        const std::size_t from = first == 0 ? 0 : first - 1;
        const std::size_t to = std::min(last + 1, m_runs.size());
        if (to - from < 2)
            return;

        std::size_t write = from;
        for (std::size_t read = from + 1; read != to; ++read)
        {
            if (joins(m_runs[write], m_runs[read]))
            {
                m_runs[write].length += m_runs[read].length;
                continue;
            }
            ++write;
            m_runs[write] = m_runs[read];
        }
        m_runs.erase(m_runs.begin() + static_cast<std::ptrdiff_t>(write) + 1,
            m_runs.begin() + static_cast<std::ptrdiff_t>(to));
    }

    void Text::appendRun(const Run& run)
    {
        if (!m_runs.empty() && joins(m_runs.back(), run))
            m_runs.back().length += run.length;
        else
            m_runs.push_back(run);
    }

    void Text::appendWords(std::wstring_view words)
    {
        m_plainText.append(words);
        appendRun(Run{ .length = words.size() });
    }

    void Text::appendObject(InlineObject object)
    {
        m_objects.push_back(std::move(object));
        m_plainText.push_back(k_objectChar);
        m_runs.push_back(Run{ .length = 1, .object = static_cast<std::uint32_t>(m_objects.size()) });
    }

    void Text::dropLastCharacter()
    {
        m_plainText.pop_back();
        if (--m_runs.back().length == 0)
            m_runs.pop_back();
    }

    // A look that states nothing is no entry, which is how a paragraph's look is taken off.
    void Text::stateParagraph(std::size_t paragraphStart, const ParagraphLook& look)
    {
        const StatedParagraphs::iterator found = std::ranges::lower_bound(m_paragraphs,
            paragraphStart, std::less<>{}, &StatedParagraph::start);
        const bool stated = found != m_paragraphs.end() && found->start == paragraphStart;
        if (look.empty())
        {
            if (stated)
                m_paragraphs.erase(found);
            return;
        }
        if (stated)
            found->look = look;
        else
            m_paragraphs.insert(found, StatedParagraph{ paragraphStart, look });
    }

    void Text::layParagraph(std::size_t paragraphStart, const ParagraphLook& outer)
    {
        stateParagraph(paragraphStart, laidUnder(paragraphLookAt(paragraphStart), outer));
    }

    // Another text's runs from one position to another, cut to that range, each index moved into
    // this text's tables. Every look and name is looked up once, however many runs carry it.
    Text::Runs Text::runsOf(const Text& other, std::size_t from, std::size_t to)
    {
        IndexMap looks(other.m_looks.size() + 1, k_unmapped);
        IndexMap names(other.m_names.size() + 1, k_unmapped);
        looks[0] = 0;
        names[0] = 0;
        const auto mapName = [this, &other, &names](std::uint32_t name){
            if (names[name] == k_unmapped)
                names[name] = nameIndexOf(other.nameAt(name));
            return names[name];
        };

        Runs result;
        std::size_t start = 0;
        for (const Run& run : other.m_runs)
        {
            if (start >= to)
                break;
            const std::size_t end = start + run.length;
            if (std::min(end, to) > std::max(start, from))
            {
                if (looks[run.look] == k_unmapped)
                    looks[run.look] = lookIndexOf(other.lookAt(run.look));
                Run copy = {
                    .length = std::min(end, to) - std::max(start, from),
                    .look = looks[run.look],
                    .link = mapName(run.link),
                    .anchor = mapName(run.anchor),
                };
                if (run.object)
                {
                    m_objects.push_back(other.m_objects[run.object - 1]);
                    copy.object = static_cast<std::uint32_t>(m_objects.size());
                }
                result.push_back(copy);
            }
            start = end;
        }
        return result;
    }

    // Read before the edit. Words replacing others take the look, link and anchor of the first
    // they replace. Words put in at a caret take the look and anchor of the character before it,
    // or of the one after at a paragraph's start, and a link only from inside it, where the
    // characters on both sides carry it.
    Text::Surroundings Text::surroundingsOf(std::size_t start, std::size_t end) const
    {
        const std::size_t size = m_plainText.size();
        if (size == 0)
            return {};

        if (end != start)
        {
            const Run& replaced = m_runs[placeOf(start).index];
            return {
                .look = replaced.look,
                .link = replaced.link,
                .anchor = replaced.anchor,
            };
        }

        const bool takesAfter = start != size && (start == 0 || m_plainText[start - 1] == k_endLine);
        const Run& sourceRun = m_runs[placeOf(takesAfter ? start : start - 1).index];
        std::uint32_t link = 0;
        if (start != 0 && start != size)
        {
            const std::uint32_t before = m_runs[placeOf(start - 1).index].link;
            if (before && before == m_runs[placeOf(start).index].link)
                link = before;
        }
        return {
            .look = sourceRun.look,
            .link = link,
            .anchor = sourceRun.anchor,
        };
    }

    // What follows a line end put in stands at a paragraph's start, where words take the look of
    // the character after them: the one the edit's end reaches, or the line end itself where the
    // edit reaches the end of the text. So words put in at once come out as they would typed one
    // by one.
    Text::Surroundings Text::surroundingsAfterLineEnd(std::size_t end,
        const Surroundings& before) const
    {
        if (end == m_plainText.size())
            return before;
        const Run& after = m_runs[placeOf(end).index];
        return {
            .look = after.look,
            .link = before.link,
            .anchor = after.anchor,
        };
    }

    void Text::nestRuns(std::span<Run> runs, const Surroundings& around)
    {
        for (Run& run : runs)
        {
            run.look = lookIndexOf(laidUnder(lookAt(run.look), lookAt(around.look), run.link != 0));
            if (!run.link)
                run.link = around.link;
            if (!run.anchor)
                run.anchor = around.anchor;
        }
    }

    // An object a run no longer stands at is dropped with the run, so a table an edit grew stays
    // the size of what the text holds.
    void Text::dropUnusedObjects()
    {
        Objects kept;
        for (Run& run : m_runs)
        {
            if (!run.object)
                continue;
            kept.push_back(std::move(m_objects[run.object - 1]));
            run.object = static_cast<std::uint32_t>(kept.size());
        }
        m_objects = std::move(kept);
    }

    void Text::edit(const TextRange& range, const Text& inserted, Insertion insertion)
    {
        if (&inserted == this)
        {
            const Text copy{ inserted };
            edit(range, copy, insertion);
            return;
        }

        m_startsOwnLine = false;
        m_endsOwnLine = false;
        const std::size_t start = std::min(range.start, m_plainText.size());
        const std::size_t end = start + std::min(range.length, m_plainText.size() - start);
        const std::size_t insertedLength = inserted.m_plainText.size();

        // Nested, the words after the first line end put in take the look after the edit.
        const std::size_t lineEnd = inserted.m_plainText.find(k_endLine);
        const std::size_t firstLineLength = insertion == Insertion::Nested
            && lineEnd != std::wstring::npos
            ? lineEnd + 1
            : insertedLength;
        Runs added = runsOf(inserted, 0, firstLineLength);
        Runs following = runsOf(inserted, firstLineLength, insertedLength);
        if (insertion == Insertion::Nested)
        {
            const Surroundings around = surroundingsOf(start, end);
            nestRuns(added, around);
            nestRuns(following, surroundingsAfterLineEnd(end, around));
        }
        added.insert(added.end(), following.begin(), following.end());
        editParagraphs(start, end, inserted, insertion);

        const std::size_t first = splitAt(start);
        const std::size_t last = splitAt(end);
        bool objectsGone = false;
        for (std::size_t i = first; i != last; ++i)
            objectsGone = objectsGone || m_runs[i].object != 0;
        m_runs.erase(m_runs.begin() + static_cast<std::ptrdiff_t>(first),
            m_runs.begin() + static_cast<std::ptrdiff_t>(last));
        m_runs.insert(m_runs.begin() + static_cast<std::ptrdiff_t>(first), added.begin(),
            added.end());
        m_plainText.replace(start, end - start, inserted.m_plainText);
        joinRuns(first, first + added.size());
        if (objectsGone || !inserted.m_objects.empty())
            dropUnusedObjects();
    }

    // **A paragraph keeps its look through every edit made inside it**, and one joined to it takes
    // that look, so the paragraphs starting inside what goes are dropped. The paragraphs a text
    // put in starts take their own looks back where it is restored. Put in any other way, each
    // takes the look of the paragraph the edit starts in under its own, and the one holding the
    // words that followed the edit keeps that look. A paragraph the edit empties has nothing of
    // its own left, and takes the first look of what is put in over its own.
    // See TextEngine-Types#edits
    void Text::editParagraphs(std::size_t start, std::size_t end, const Text& inserted,
        Insertion insertion)
    {
        const ParagraphLook upper = paragraphLookAt(paragraphStartOf(start));
        const bool emptied = insertion == Insertion::Nested && startsParagraph(start)
            && (end == m_plainText.size() || m_plainText[end] == k_endLine);
        const std::size_t insertedLength = inserted.m_plainText.size();

        StatedParagraphs kept;
        kept.reserve(m_paragraphs.size());
        for (const StatedParagraph& stated : m_paragraphs)
        {
            if (stated.start <= start)
                kept.push_back(stated);
            else if (stated.start > end)
                kept.push_back({ stated.start - (end - start) + insertedLength, stated.look });
        }
        m_paragraphs = std::move(kept);

        if (insertion == Insertion::Restored)
        {
            for (const StatedParagraph& stated : inserted.m_paragraphs)
            {
                if (stated.start != 0)
                    stateParagraph(start + stated.start, stated.look);
            }
            return;
        }

        if (emptied)
            stateParagraph(start, laidUnder(inserted.paragraphLookAt(0), upper));

        const std::wstring& words = inserted.m_plainText;
        for (std::size_t lineEnd = words.find(k_endLine); lineEnd != std::wstring::npos;
            lineEnd = words.find(k_endLine, lineEnd + 1))
        {
            const std::size_t next = lineEnd + 1;
            const ParagraphLook own = next == insertedLength
                ? ParagraphLook{}
                : inserted.paragraphLookAt(next);
            stateParagraph(start + next, laidUnder(own, upper));
        }
    }

    void Text::applyLayer(std::size_t from, std::size_t to, const TextLayer& layer)
    {
        const std::size_t first = splitAt(from);
        const std::size_t last = splitAt(to);
        const std::uint32_t link = nameIndexOf(layer.link);
        const std::uint32_t anchor = nameIndexOf(layer.anchor);
        for (std::size_t i = first; i != last; ++i)
        {
            Run& run = m_runs[i];
            if (!layer.look.empty())
                run.look = lookIndexOf(laidUnder(lookAt(run.look), layer.look, run.link != 0));
            if (!run.link)
                run.link = link;
            if (!run.anchor)
                run.anchor = anchor;
        }
        joinRuns(first, last);
    }

    // The line the last words own is ended first, so the run starts where its words do.
    Text::WriteStart Text::openScope()
    {
        const bool endsOwnLine = m_endsOwnLine;
        breakOwnLine();
        return {
            .plainLength = m_plainText.size(),
            .lineBroken = endsOwnLine,
            .startsOwnLine = m_startsOwnLine,
            .endsOwnLine = endsOwnLine,
        };
    }

    void Text::closeScope(const WriteStart& start, const TextLayer& layer)
    {
        if (m_plainText.size() == start.plainLength)
        {
            undoOpening(start);
            return;
        }
        applyLayer(start.plainLength, m_plainText.size(), layer);
    }

    Text::WriteStart Text::openParagraph()
    {
        const bool startsOwnLine = m_startsOwnLine;
        const bool endsOwnLine = m_endsOwnLine;
        const bool lineBroken = !m_plainText.empty() && m_plainText.back() != k_endLine;
        if (lineBroken)
            appendWords(std::wstring_view{ &k_endLine, 1 });
        if (m_plainText.empty())
            m_startsOwnLine = true;
        m_endsOwnLine = false;
        return {
            .plainLength = m_plainText.size(),
            .lineBroken = lineBroken,
            .startsOwnLine = startsOwnLine,
            .endsOwnLine = endsOwnLine,
        };
    }

    // Every line the words start takes the look, under whatever the words state there themselves.
    void Text::closeParagraph(const WriteStart& start, const ParagraphLook& look)
    {
        if (m_plainText.size() == start.plainLength)
        {
            undoOpening(start);
            return;
        }

        layParagraph(start.plainLength, look);
        for (std::size_t at = start.plainLength; at + 1 < m_plainText.size(); ++at)
        {
            if (m_plainText[at] == k_endLine)
                layParagraph(at + 1, look);
        }
        m_endsOwnLine = m_plainText.back() != k_endLine;
    }

    // A wrapper that wrote nothing leaves the text as it found it.
    void Text::undoOpening(const WriteStart& start)
    {
        if (start.lineBroken)
            dropLastCharacter();
        m_startsOwnLine = start.startsOwnLine;
        m_endsOwnLine = start.endsOwnLine;
    }

    // The line end a paragraph wrapper leaves to whatever is written after it, so a text that
    // ends in one ends with no line end.
    void Text::breakOwnLine()
    {
        if (!m_endsOwnLine)
            return;
        appendWords(std::wstring_view{ &k_endLine, 1 });
        m_endsOwnLine = false;
    }

    // TextFormat

    TextFormat& TextFormat::operator<<(const TextStyle& style)
    {
        m_look.style = &style;
        return *this;
    }

    TextFormat& TextFormat::operator<<(Ink ink)
    {
        m_look.ink = ink;
        return *this;
    }

    TextFormat& TextFormat::operator<<(InkGrade grade)
    {
        return *this << InkWell::textInk(grade);
    }

    TextFormat& TextFormat::operator<<(InkColor color)
    {
        return *this << inkOf(color);
    }

    TextFormat& TextFormat::operator<<(Color color)
    {
        m_look.ink = color;
        return *this;
    }

    TextFormat& TextFormat::operator<<(TextAlign align)
    {
        m_paragraph.align = align;
        return *this;
    }

    TextFormat& TextFormat::operator<<(ParaIndent indent)
    {
        m_paragraph.indent = indent.indent;
        return *this;
    }

    TextFormat& TextFormat::operator<<(ParaLineSpacing spacing)
    {
        m_paragraph.lineSpacing = spacing.spacing;
        return *this;
    }

    TextFormat& TextFormat::operator<<(const TextFormat& other)
    {
        const TextLook& look = other.m_look;
        if (look.style)
            m_look.style = look.style;
        if (look.fontFamily)
            m_look.fontFamily = look.fontFamily;
        if (look.fontSize)
            m_look.fontSize = look.fontSize;
        if (look.fontWeight)
            m_look.fontWeight = look.fontWeight;
        if (look.fontStyle)
            m_look.fontStyle = look.fontStyle;
        if (look.ink)
            m_look.ink = look.ink;
        m_paragraph = laidUnder(other.m_paragraph, m_paragraph);
        return *this;
    }

    std::size_t TextFormat::hash() const
    {
        std::size_t hash = k_fnvOffset;
        combineLook(hash, m_look);
        combineParagraph(hash, m_paragraph);
        return hash;
    }

    bool TextFormat::matches(const TextFormat* other) const
    {
        return other ? *this == *other : empty();
    }
}
