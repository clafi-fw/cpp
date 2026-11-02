module ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // The style of a run that states nothing, which every style index 0 stands for.
        const TextStyle k_noStyle{};

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

        // The field a base states, where the style states none of its own.
        template<typename Value>
        void lendField(TextStyle& style, const TextStyle& base,
            std::optional<Value> TextStyle::* field)
        {
            if (!(style.*field).has_value())
                style.*field = base.*field;
        }

        // The style's base lends it its fields and script levels, and the style is based on the
        // base's own base, drawing as it did.
        void foldBase(TextStyle& style)
        {
            const TextStyle& base = *style.base;
            lendField(style, base, &TextStyle::fontFamily);
            lendField(style, base, &TextStyle::fontSize);
            lendField(style, base, &TextStyle::fontWeight);
            lendField(style, base, &TextStyle::fontStyle);
            lendField(style, base, &TextStyle::ink);
            style.scripts.insert(style.scripts.begin(), base.scripts.begin(), base.scripts.end());
            style.base = base.base;
        }

        // **A base its sheet does not find by its name is folded in**: a style removed, or one
        // standing behind another of its name, is carried by what it states rather than by a name
        // that means another. See TextEngine-Types#text-styles
        [[nodiscard]] TextStyle foldedIn(const TextStyle& style, const StyleSheet* from)
        {
            TextStyle result = style;
            while (from && result.base && from->find(result.base->name) != result.base)
                foldBase(result);
            return result;
        }

        // One field of an inner style under an outer one. The inner keeps what it states, itself
        // or through its base. The outer fills the rest from its own fields, and from its base
        // where the inner keeps a base of its own - otherwise the inner takes the outer's base
        // and that fills those fields itself.
        template<typename Value>
        void fillField(TextStyle& result, const TextStyle& inner, const TextStyle& outer,
            std::optional<Value> TextStyle::* field, bool takesOuterBase)
        {
            if (inner.stated(field))
                return;
            if ((outer.*field).has_value())
            {
                result.*field = outer.*field;
                return;
            }
            if (takesOuterBase || !outer.base)
                return;
            if (const Value* value = outer.base->stated(field))
                result.*field = *value;
        }

        // An inner style laid under an outer one: the inner wins every field it states, and the
        // outer fills the rest. See TextEngine-Types#textlayer
        [[nodiscard]] TextStyle laidUnder(const TextStyle& inner, const TextStyle& outer,
            bool innerLinked)
        {
            if (outer.empty())
                return inner;
            if (inner.empty() && !innerLinked)
                return outer;

            TextStyle result = inner;
            const bool takesOuterBase = !inner.base;
            if (takesOuterBase)
                result.base = outer.base;
            fillField(result, inner, outer, &TextStyle::fontFamily, takesOuterBase);
            fillField(result, inner, outer, &TextStyle::fontSize, takesOuterBase);
            fillField(result, inner, outer, &TextStyle::fontWeight, takesOuterBase);
            fillField(result, inner, outer, &TextStyle::fontStyle, takesOuterBase);

            // A link inside states its ink, so an outer one does not reach the words. A base
            // taken would, and the link's ink is stated against it.
            if (!innerLinked)
                fillField(result, inner, outer, &TextStyle::ink, takesOuterBase);
            else if (takesOuterBase && !result.ink && outer.base
                && outer.base->stated(&TextStyle::ink))
                result.ink = k_linkInk;

            // The outer's levels stand outside every level of the inner's, so an inner base stating
            // levels is folded in where the outer adds any; a base taken carries its own outermost.
            std::vector<TextScript> levels = takesOuterBase || !outer.base
                ? std::vector<TextScript>{}
                : outer.base->levels();
            levels.insert(levels.end(), outer.scripts.begin(), outer.scripts.end());
            while (!takesOuterBase && !levels.empty() && result.base
                && !result.base->levels().empty())
            {
                foldBase(result);
            }
            result.scripts.insert(result.scripts.begin(), levels.begin(), levels.end());
            return result;
        }

        // Whether two styles state the field alike, themselves or through their bases.
        template<typename Value>
        [[nodiscard]] bool statesAlike(const TextStyle& style, const TextStyle& other,
            std::optional<Value> TextStyle::* field)
        {
            const Value* value = style.stated(field);
            const Value* otherValue = other.stated(field);
            return value && otherValue ? *value == *otherValue : value == otherValue;
        }

        // Whether two run styles draw alike over one Body: every field and script level the same.
        [[nodiscard]] bool drawnAlike(const TextStyle& style, const TextStyle& other)
        {
            return statesAlike(style, other, &TextStyle::fontFamily)
                && statesAlike(style, other, &TextStyle::fontSize)
                && statesAlike(style, other, &TextStyle::fontWeight)
                && statesAlike(style, other, &TextStyle::fontStyle)
                && statesAlike(style, other, &TextStyle::ink)
                && style.levels() == other.levels();
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

        // An unstated field and a stated one hash apart, whatever the value.
        template<typename Value>
        void combineStated(std::size_t& hash, const TextStyle& style,
            std::optional<Value> TextStyle::* field)
        {
            const Value* value = style.stated(field);
            combine(hash, static_cast<std::size_t>(value != nullptr));
            if (value)
                combine(hash, *value);
        }

        // What a style draws, so two styles drawing alike hash alike, whatever their bases.
        void combineStyle(std::size_t& hash, const TextStyle& style)
        {
            combineStated(hash, style, &TextStyle::fontFamily);
            combineStated(hash, style, &TextStyle::fontSize);
            combineStated(hash, style, &TextStyle::fontWeight);
            combineStated(hash, style, &TextStyle::fontStyle);
            const ColorDef* ink = style.stated(&TextStyle::ink);
            combine(hash, static_cast<std::size_t>(ink != nullptr));
            if (ink)
                combineInk(hash, *ink);
            for (const TextScript script : style.levels())
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

    Text::Text()
        :
        m_styles{ &StyleSheet::application() }
    {
    }

    Text::Text(const StyleSheet& sheet)
        :
        m_styles{ &sheet }
    {
    }

    Text::Text(TextPieces pieces)
        :
        Text{ std::move(pieces), StyleSheet{ &StyleSheet::application() } }
    {
    }

    // **A paragraph's piece states the whole look of the paragraph the text has reached**, and the
    // paragraphs started after it take that look until another piece states one. A base the styles
    // do not reach is found among them by its name, as a style put in is.
    Text::Text(TextPieces pieces, StyleSheet styles)
        :
        m_styles{ std::move(styles) }
    {
        for (const TextPiece& piece : pieces)
        {
            if (piece.paragraph)
                stateParagraph(paragraphStartOf(m_plainText.size()), *piece.paragraph);

            const Run run = {
                .style = adoptedIndexOf(piece.style, nullptr),
                .link = nameIndexOf(piece.link),
                .anchor = nameIndexOf(piece.anchor),
            };
            if (piece.object)
            {
                appendObject(*piece.object);
                m_runs.back().style = run.style;
                m_runs.back().link = run.link;
                m_runs.back().anchor = run.anchor;
                continue;
            }
            if (piece.words.empty())
                continue;
            const std::size_t from = m_plainText.size();
            m_plainText.append(piece.words);
            Run words = run;
            words.length = piece.words.size();
            appendRun(words);
            carryLook(from);
        }
    }

    Text& Text::operator<<(const std::wstring_view words)
    {
        if (words.empty())
            return *this;
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
        appendObject(icon);
        return *this;
    }

    Text& Text::operator<<(const Space& space)
    {
        appendObject(space);
        return *this;
    }

    Text& Text::operator<<(const VSpace& space)
    {
        appendObject(space);
        return *this;
    }

    Text& Text::operator<<(const FlexSpace& space)
    {
        appendObject(space);
        return *this;
    }

    Text& Text::operator<<(const TabTo& tab)
    {
        appendObject(tab);
        return *this;
    }

    Text& Text::operator<<(TextAlign align)
    {
        layLastLook(ParagraphLook{ .align = align });
        return *this;
    }

    Text& Text::operator<<(TextIndent indent)
    {
        layLastLook(ParagraphLook{ .indent = indent.indent });
        return *this;
    }

    Text& Text::operator<<(TextLineSpacing spacing)
    {
        layLastLook(ParagraphLook{ .lineSpacing = spacing.spacing });
        return *this;
    }

    Text& Text::operator=(const Text& other)
    {
        if (this != &other)
        {
            Text copy{ other };
            swap(copy);
        }
        return *this;
    }

    Text& Text::operator=(Text&& other) noexcept
    {
        Text taken{ std::move(other) };
        swap(taken);
        return *this;
    }

    // **Written as its words and tags would be**: its first paragraph joins the one the text has
    // reached, and each of its paragraphs takes the look it states over the look in effect there.
    Text& Text::operator<<(const Text& other)
    {
        if (&other == this)
        {
            const Text copy{ other };
            return *this << copy;
        }

        const std::size_t offset = m_plainText.size();
        const std::size_t joined = paragraphStartOf(offset);
        const ParagraphLook inEffect = paragraphLookAt(joined);
        const Runs added = runsOf(other, 0, other.m_plainText.size());
        m_plainText.append(other.m_plainText);
        for (const Run& run : added)
            appendRun(run);
        stateParagraph(joined, laidUnder(other.paragraphLookAt(0), inEffect));
        const std::wstring& words = other.m_plainText;
        for (std::size_t lineEnd = words.find(k_endLine); lineEnd != std::wstring::npos;
            lineEnd = words.find(k_endLine, lineEnd + 1))
        {
            const ParagraphLook own = other.paragraphLookAt(lineEnd + 1);
            stateParagraph(offset + lineEnd + 1, laidUnder(own, inEffect));
        }
        return *this;
    }

    // The text stays over the sheet it is over, and drops the styles of its own.
    void Text::clear()
    {
        StyleSheet styles{ m_styles.parent() };
        m_styles.swap(styles);
        m_plainText.clear();
        m_runs.clear();
        m_names.clear();
        m_objects.clear();
        m_paragraphs.clear();
    }

    std::size_t Text::hash() const
    {
        std::size_t hash = k_fnvOffset;
        combine(hash, std::wstring_view{ m_plainText });
        for (const Run& run : m_runs)
        {
            combine(hash, run.length);
            combineStyle(hash, styleAt(run.style));
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

    // **By what each run draws**, since two texts hold their styles in the order they were
    // written, and one style may be the other's own copy. Over one sheet and with the same named
    // styles of their own, which state what the runs leave to Body and what a format names.
    bool Text::operator==(const Text& other) const
    {
        if (m_styles.parent() != other.m_styles.parent() || m_plainText != other.m_plainText
            || m_runs.size() != other.m_runs.size() || m_paragraphs != other.m_paragraphs
            || !(m_styles == other.m_styles))
        {
            return false;
        }
        for (std::size_t i = 0; i != m_runs.size(); ++i)
        {
            const Run& run = m_runs[i];
            const Run& otherRun = other.m_runs[i];
            if (run.length != otherRun.length || (run.object == 0) != (otherRun.object == 0))
                return false;
            if (!drawnAlike(styleAt(run.style), other.styleAt(otherRun.style)))
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
    // words begin. A base the text does not find by its name is folded into the piece.
    TextPieces Text::pieces() const
    {
        // A paragraph's piece stands where the look changes, the first look stated included, and
        // one stating nothing where a paragraph has no look after one that had.
        StatedParagraphs changes;
        ParagraphLook previous{};
        for (std::size_t paragraphStart = 0; paragraphStart != std::wstring::npos;)
        {
            const ParagraphLook look = paragraphLookAt(paragraphStart);
            if (look != previous)
                changes.push_back({ paragraphStart, look });
            previous = look;
            const std::size_t lineEnd = m_plainText.find(k_endLine, paragraphStart);
            paragraphStart = lineEnd == std::wstring::npos ? lineEnd : lineEnd + 1;
        }

        TextPieces result;
        std::size_t nextParagraph = 0;
        const auto stateParagraphsUpTo = [&changes, &result, &nextParagraph](std::size_t pos){
            while (nextParagraph != changes.size() && changes[nextParagraph].start <= pos)
            {
                result.push_back(TextPiece{ .paragraph = changes[nextParagraph].look });
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
                const std::size_t to = nextParagraph != changes.size()
                    && changes[nextParagraph].start < end
                    ? changes[nextParagraph].start
                    : end;
                TextPiece piece{
                    .words = m_plainText.substr(from, to - from),
                    .style = foldedIn(styleAt(run.style), &m_styles),
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
        Text result = blank();
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
        return !before.object && !after.object && before.style == after.style
            && before.link == after.link && before.anchor == after.anchor;
    }

    const TextStyle& Text::styleAt(std::uint32_t style) const
    {
        return style ? m_styles.at(style - 1) : k_noStyle;
    }

    std::wstring_view Text::nameAt(std::uint32_t name) const
    {
        return name ? std::wstring_view{ m_names[name - 1] } : std::wstring_view{};
    }

    // A text holds a style once, so two runs in one style carry one index.
    std::uint32_t Text::styleIndexOf(const TextStyle& style)
    {
        if (style.empty())
            return 0;
        return static_cast<std::uint32_t>(m_styles.cache(style)) + 1;
    }

    // **A style goes by its name.** The one of that name the text reaches draws the words; where
    // it reaches none, a copy of the style becomes one of the text's own, based on its base as
    // found here. See TextEngine-Types#text-styles
    const TextStyle* Text::adopted(const TextStyle* style, const StyleSheet* from)
    {
        if (!style)
            return nullptr;
        TextStyle copy = foldedIn(*style, from);
        if (const TextStyle* named = m_styles.find(copy.name))
            return named;
        copy.base = adopted(copy.base, from);
        return m_styles.carry(copy);
    }

    std::uint32_t Text::adoptedIndexOf(const TextStyle& style, const StyleSheet* from)
    {
        TextStyle own = foldedIn(style, from);
        own.name.clear();
        own.base = adopted(own.base, from);
        return styleIndexOf(own);
    }

    // **A name the text reaches no style of is reported** and lays nothing, so the words are set
    // as they would be without it.
    TextStyle Text::styleOf(const TextLayer& layer) const
    {
        TextStyle result = layer.fields;
        result.name.clear();
        result.base = nullptr;
        if (layer.style.empty())
            return result;
        result.base = m_styles.find(layer.style);
        if (!result.base)
            StyleSheet::events().emit<UnknownStyleEvent>(std::wstring_view{ layer.style });
        return result;
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
    // style, so the runs stay the fewest that say what the text says.
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
        const std::size_t from = m_plainText.size();
        m_plainText.append(words);
        appendRun(Run{ .length = words.size() });
        carryLook(from);
    }

    void Text::appendObject(InlineObject object)
    {
        m_objects.push_back(std::move(object));
        m_plainText.push_back(k_objectChar);
        m_runs.push_back(Run{ .length = 1, .object = static_cast<std::uint32_t>(m_objects.size()) });
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

    // **A paragraph takes the look of the one before it**, so a look stays in effect until a tag
    // replaces it. See TextEngine-Types#paragraphlook
    void Text::carryLook(std::size_t from)
    {
        const ParagraphLook look = paragraphLookAt(paragraphStartOf(from));
        if (look.empty())
            return;
        for (std::size_t lineEnd = m_plainText.find(k_endLine, from); lineEnd != std::wstring::npos;
            lineEnd = m_plainText.find(k_endLine, lineEnd + 1))
        {
            stateParagraph(lineEnd + 1, look);
        }
    }

    // A tag states its field for the whole paragraph it is written in, a field written there
    // before included.
    void Text::layLastLook(const ParagraphLook& fields)
    {
        const std::size_t start = paragraphStartOf(m_plainText.size());
        stateParagraph(start, laidUnder(fields, paragraphLookAt(start)));
    }

    // Another text's runs from one position to another, cut to that range, each index moved into
    // this text's tables. Every style and name is looked up once, however many runs carry it.
    Text::Runs Text::runsOf(const Text& other, std::size_t from, std::size_t to)
    {
        IndexMap styles(other.m_styles.count() + 1, k_unmapped);
        IndexMap names(other.m_names.size() + 1, k_unmapped);
        styles[0] = 0;
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
                if (styles[run.style] == k_unmapped)
                    styles[run.style] = adoptedIndexOf(other.styleAt(run.style), &other.m_styles);
                Run copy = {
                    .length = std::min(end, to) - std::max(start, from),
                    .style = styles[run.style],
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

    // Read before the edit. Words replacing others take the style, link and anchor of the first
    // they replace. Words put in at a caret take the style and anchor of the character before it,
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
                .style = replaced.style,
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
            .style = sourceRun.style,
            .link = link,
            .anchor = sourceRun.anchor,
        };
    }

    // What follows a line end put in stands at a paragraph's start, where words take the style of
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
            .style = after.style,
            .link = before.link,
            .anchor = after.anchor,
        };
    }

    void Text::nestRuns(std::span<Run> runs, const Surroundings& around)
    {
        for (Run& run : runs)
        {
            run.style = styleIndexOf(laidUnder(styleAt(run.style), styleAt(around.style),
                run.link != 0));
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

        const std::size_t start = std::min(range.start, m_plainText.size());
        const std::size_t end = start + std::min(range.length, m_plainText.size() - start);
        const std::size_t insertedLength = inserted.m_plainText.size();

        // Nested, the words after the first line end put in take the style after the edit.
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
        const TextStyle layered = styleOf(layer);
        const std::size_t first = splitAt(from);
        const std::size_t last = splitAt(to);
        const std::uint32_t link = nameIndexOf(layer.link);
        const std::uint32_t anchor = nameIndexOf(layer.anchor);
        for (std::size_t i = first; i != last; ++i)
        {
            Run& run = m_runs[i];
            if (!layered.empty())
                run.style = styleIndexOf(laidUnder(styleAt(run.style), layered, run.link != 0));
            if (!run.link)
                run.link = link;
            if (!run.anchor)
                run.anchor = anchor;
        }
        joinRuns(first, last);
    }

    void Text::closeScope(std::size_t start, const TextLayer& layer)
    {
        if (m_plainText.size() != start)
            applyLayer(start, m_plainText.size(), layer);
    }

    Text Text::blank() const
    {
        Text result;
        StyleSheet styles{ m_styles.parent() };
        result.m_styles.swap(styles);
        return result;
    }

    void Text::swap(Text& other) noexcept
    {
        m_plainText.swap(other.m_plainText);
        m_runs.swap(other.m_runs);
        m_styles.swap(other.m_styles);
        m_names.swap(other.m_names);
        m_objects.swap(other.m_objects);
        m_paragraphs.swap(other.m_paragraphs);
    }

    // TextFormat

    TextFormat& TextFormat::operator<<(std::wstring_view style)
    {
        m_style = style;
        return *this;
    }

    TextFormat& TextFormat::operator<<(Ink ink)
    {
        m_fields.ink = ink;
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
        m_fields.ink = color;
        return *this;
    }

    TextFormat& TextFormat::operator<<(TextAlign align)
    {
        m_paragraph.align = align;
        return *this;
    }

    TextFormat& TextFormat::operator<<(TextIndent indent)
    {
        m_paragraph.indent = indent.indent;
        return *this;
    }

    TextFormat& TextFormat::operator<<(TextLineSpacing spacing)
    {
        m_paragraph.lineSpacing = spacing.spacing;
        return *this;
    }

    TextFormat& TextFormat::operator<<(const TextFormat& other)
    {
        if (!other.m_style.empty())
            m_style = other.m_style;
        const TextStyle& fields = other.m_fields;
        if (fields.fontFamily)
            m_fields.fontFamily = fields.fontFamily;
        if (fields.fontSize)
            m_fields.fontSize = fields.fontSize;
        if (fields.fontWeight)
            m_fields.fontWeight = fields.fontWeight;
        if (fields.fontStyle)
            m_fields.fontStyle = fields.fontStyle;
        if (fields.ink)
            m_fields.ink = fields.ink;
        m_paragraph = laidUnder(other.m_paragraph, m_paragraph);
        return *this;
    }

    bool TextFormat::empty() const
    {
        return m_style.empty() && m_fields.empty() && m_paragraph.empty();
    }

    std::size_t TextFormat::hash() const
    {
        std::size_t hash = k_fnvOffset;
        combine(hash, std::wstring_view{ m_style });
        combineStyle(hash, m_fields);
        combineParagraph(hash, m_paragraph);
        return hash;
    }

    bool TextFormat::matches(const TextFormat* other) const
    {
        return other ? *this == *other : empty();
    }
}
