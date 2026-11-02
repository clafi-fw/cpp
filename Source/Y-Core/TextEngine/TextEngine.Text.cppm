export module ClaFi.Core.TextEngine.Text;

import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    // **Which** text, and **which** state of it. See TextEngine-Types
    export struct TextStamp
    {
        std::size_t id{ 0 };
        std::size_t revision{ 0 };
        [[nodiscard]] bool named() const { return id != 0; }
        bool operator==(const TextStamp&) const = default;
    };

    // Words between a run's opening marker and its closer. See TextEngine-Types#scopedtext
    export template<typename... Content>
    class ScopedText
    {
    public:
        using ContentTuple = std::tuple<Content...>;

    public:
        explicit ScopedText(FormatItem opening, Content&&...);

    public:
        [[nodiscard]] const FormatItem& opening() const { return m_opening; }
        [[nodiscard]] const ContentTuple& content() const { return m_content; }

    private:
        FormatItem m_opening;
        ContentTuple m_content;
    };

    // Words set in one of TextStyles.
    export template<typename... Content>
    class StyledText : public ScopedText<Content...>
    {
    public:
        explicit StyledText(const TextStyleProps& style, Content&&...);

    private:
        [[nodiscard]] static TextStyle styleOf(const TextStyleProps& style);
    };

    template<typename... Content>
    StyledText(const TextStyleProps&, Content&&...) -> StyledText<Content...>;

    // Words drawn in an ink or a colour of their own.
    export template<typename... Content>
    class ColoredText : public ScopedText<Content...>
    {
    public:
        explicit ColoredText(Ink, Content&&...);
        explicit ColoredText(InkGrade grade, Content&&...); // the text ink at that step
        explicit ColoredText(InkColor color, Content&&...); // that colour at Strongest
        explicit ColoredText(Color, Content&&...);

    private:
        [[nodiscard]] static constexpr Ink inkOf(InkColor color);
    };

    template<typename... Content>
    ColoredText(Ink, Content&&...) -> ColoredText<Content...>;
    template<typename... Content>
    ColoredText(InkGrade, Content&&...) -> ColoredText<Content...>;
    template<typename... Content>
    ColoredText(InkColor, Content&&...) -> ColoredText<Content...>;
    template<typename... Content>
    ColoredText(Color, Content&&...) -> ColoredText<Content...>;

    // Words set bold.
    export template<typename... Content>
    class BoldText : public ScopedText<Content...>
    {
    public:
        explicit BoldText(Content&&...);
    };

    template<typename... Content>
    BoldText(Content&&...) -> BoldText<Content...>;

    // Words set italic.
    export template<typename... Content>
    class ItalicText : public ScopedText<Content...>
    {
    public:
        explicit ItalicText(Content&&...);
    };

    template<typename... Content>
    ItalicText(Content&&...) -> ItalicText<Content...>;

    // Words set at a size of their own, in design units.
    export template<typename... Content>
    class SizedText : public ScopedText<Content...>
    {
    public:
        explicit SizedText(float size, Content&&...);
    };

    template<typename... Content>
    SizedText(float, Content&&...) -> SizedText<Content...>;

    // Words set in a font family of their own, named as the platform knows it.
    export template<typename... Content>
    class FamilyText : public ScopedText<Content...>
    {
    public:
        explicit FamilyText(std::wstring family, Content&&...);
    };

    template<typename... Content>
    FamilyText(std::wstring, Content&&...) -> FamilyText<Content...>;

    // Words raised off the baseline and set smaller.
    export template<typename... Content>
    class SuperscriptText : public ScopedText<Content...>
    {
    public:
        explicit SuperscriptText(Content&&...);
    };

    template<typename... Content>
    SuperscriptText(Content&&...) -> SuperscriptText<Content...>;

    // Words lowered off the baseline and set smaller.
    export template<typename... Content>
    class SubscriptText : public ScopedText<Content...>
    {
    public:
        explicit SubscriptText(Content&&...);
    };

    template<typename... Content>
    SubscriptText(Content&&...) -> SubscriptText<Content...>;

    // Words that are a link to the target they name. See TextEngine-Types#links
    export template<typename... Content>
    class LinkText : public ScopedText<Content...>
    {
    public:
        explicit LinkText(std::wstring target, Content&&...);
    };

    template<typename... Content>
    LinkText(std::wstring, Content&&...) -> LinkText<Content...>;

    // Words a link reaches with # and the name. See TextEngine-Types#anchors
    export template<typename... Content>
    class AnchorText : public ScopedText<Content...>
    {
    public:
        explicit AnchorText(std::wstring name, Content&&...);
    };

    template<typename... Content>
    AnchorText(std::wstring, Content&&...) -> AnchorText<Content...>;

    // Words on lines of their own, each taking the property. See TextEngine-Types#paragraphtext
    export template<typename... Content>
    class ParagraphText
    {
    public:
        using ContentTuple = std::tuple<Content...>;

    public:
        explicit ParagraphText(TextAlign, Content&&...);
        explicit ParagraphText(ParaIndent, Content&&...);
        explicit ParagraphText(ParaLineSpacing, Content&&...);

    public:
        [[nodiscard]] const FormatItem& property() const { return m_property; }
        [[nodiscard]] const ContentTuple& content() const { return m_content; }

    private:
        FormatItem m_property;
        ContentTuple m_content;
    };

    template<typename... Content>
    ParagraphText(TextAlign, Content&&...) -> ParagraphText<Content...>;
    template<typename... Content>
    ParagraphText(ParaIndent, Content&&...) -> ParagraphText<Content...>;
    template<typename... Content>
    ParagraphText(ParaLineSpacing, Content&&...) -> ParagraphText<Content...>;

    // A run of text with its styles, as the framework holds it.
    export class Text
    {
    public:
        // A marker and the index in the plain text it stands at.
        using Marker = std::pair<std::size_t, FormatItem>;
        using Markers = std::vector<Marker>;

        Text() = default;
        template<typename... Args> explicit Text(Args&&... args) : Text{} { (*this << ... << std::forward<Args>(args)); }
        // A text stated whole, which is how a document hands one back: the plain text and the
        // markers standing in it, taken as they are. The markers are in text order with every
        // index inside the text, since that is what every route through a Text keeps, and a
        // marker that puts a character in - a space, an icon - stands at that character.
        explicit Text(std::wstring plainText, Markers markers);

        // Anything the stream below takes, written as an assignment. What is assigned **replaces**
        // what the text holds, which is the one thing that separates this from operator<<: the
        // stream adds to a text, this states the whole of it. One argument, because an
        // assignment operator takes exactly one - several parts are a Text of their own:
        //     caption = Text{ ColoredText{ Ink::accent, name }, L" saved" };
        //
        // Constrained off Text so that copying and moving a Text stay the assignments the
        // compiler writes. Unconstrained, this is an exact match for a non-const Text lvalue
        // while the copy assignment needs a qualification conversion on top, so it would win and
        // rebuild the text through the stream - and a Text handed over as an rvalue would be
        // copied where it could have been taken. Derived rather than same, because a
        // PlaceHolderText assigned to a Text asks the same question.
        //
        // An assignment operator template is never the copy or the move assignment operator, so
        // both of those are still implicitly declared and this stands beside them.
        template<typename Arg>
            requires (!std::derived_from<std::remove_cvref_t<Arg>, Text>)
        Text& operator=(Arg&& arg)
        {
            clear();
            *this << std::forward<Arg>(arg);
            return *this;
        }

        Text& operator<<(const std::wstring_view);
        Text& operator<<(const std::wstring&);
        Text& operator<<(const wchar_t*);
        Text& operator<<(wchar_t);
        Text& operator<<(int);
        // **What the framework counts with**, and so what a caller has in hand to write: a line, a
        // column, a length. Stated on its own rather than left to the int above, because an
        // unsigned reaches int and wchar_t by the same rank of conversion and neither wins - a
        // caller writing a count would be told the two are ambiguous.
        Text& operator<<(std::size_t);
        Text& operator<<(const InTextIcon&);
        Text& operator<<(const Space&);
        Text& operator<<(const VSpace&);
        Text& operator<<(const FlexSpace&);
        Text& operator<<(const TabTo&);
        Text& operator<<(const Text&);
        // A run that holds no character writes nothing.
        template<typename... Content>
        Text& operator<<(const ScopedText<Content...>&);
        // Lines that hold no character write nothing. The words written next start a line.
        template<typename... Content>
        Text& operator<<(const ParagraphText<Content...>&);

        bool empty() const { return m_plainText.empty(); }
        // Nothing written into it at all, markers included. A text carrying only markers says
        // nothing and is not empty, which is the difference a caller collecting contributions has
        // to see - see AdjustTextEventBase::contribute.
        [[nodiscard]] bool blank() const { return m_plainText.empty() && m_markers.empty(); }
        void clear();
        std::size_t hash() const;
        // Whether a flex space is in here, which is the one thing a paint lays out differently
        // from a calculate: it fills the box it is drawn in and contributes nothing to a size.
        // Swept when it is asked for, so a Text carries no field for it.
        [[nodiscard]] bool hasFlexSpace() const;

        const std::wstring& plainText() const { return m_plainText; }
        // The words and markers, so two texts that draw alike are equal whatever was last written.
        bool operator==(const Text&) const;
        [[nodiscard]] const Markers& markers() const { return m_markers; }
        // The text the first anchor of that name holds, or nothing where no anchor has it. A walk
        // over the markers, since an anchor is asked for when a link to it is followed and not
        // before.
        [[nodiscard]] std::optional<TextRange> anchorRange(std::wstring_view name) const;
        // The link a caret at that position stands in, its ends included; nothing off every link.
        [[nodiscard]] std::optional<LinkSpan> linkAtCaret(std::size_t caretPos) const;

        // Edit support
        void replaceText(const TextRange&, const std::wstring_view = {});
        void replaceText(const TextRange&, const Text&);
        Text selectedText(const TextRange&) const;

    private:
        // How the text stood when a wrapper started writing into it.
        struct WriteStart
        {
            std::size_t plainLength{ 0 };
            std::size_t markerCount{ 0 };
            bool lineBroken{ false }; // the wrapper ended the line it found open
            bool startsOwnLine{ false };
            bool endsOwnLine{ false };
        };

    private:
        [[nodiscard]] WriteStart openScope(const FormatItem& opening);
        void closeScope(const WriteStart&);
        [[nodiscard]] WriteStart openParagraph(const FormatItem& property);
        void closeParagraph(const WriteStart&);
        void breakOwnLine();

    private:
        std::wstring m_plainText;
        Markers m_markers;
        bool m_startsOwnLine{ false }; // put after words, the first of these start a line
        bool m_endsOwnLine{ false }; // the next words written start a line of their own
    };

#define DERIVED_TEXT_CLASS public Text\
    {\
        public: using Text::Text;\
        using Text::operator=;\
        using Text::operator<<;\
    };

    export class PlaceHolderText : DERIVED_TEXT_CLASS
    // The text a control offers as its tooltip.
    export class TooltipText : DERIVED_TEXT_CLASS
    export class HeaderText : DERIVED_TEXT_CLASS

#undef DERIVED_TEXT_CLASS

    // A text a control keeps, asked about on every measurement and paint. See TextEngine-Types
    export class ControlText : public Text
    {
    public:
        using Text::Text;

    public:
        ControlText& operator=(const ControlText& other)
        {
            Text::operator=(other);
            return changed();
        }

        ControlText& operator=(ControlText&& other) noexcept
        {
            Text::operator=(std::move(other));
            other.changed();
            return changed();
        }

        // A Text assigned over a live one: the base states the whole of what this says, and the
        // stamp moves on. Stated because the operators here hide the base's, and a plain Text is
        // what a caller usually has.
        ControlText& operator=(const Text& other)
        {
            Text::operator=(other);
            return changed();
        }

        ControlText& operator=(Text&& other) noexcept
        {
            Text::operator=(std::move(other));
            return changed();
        }

        // One template for the whole of what Text takes, rather than a wrapper per overload.
        // Constrained the way Text's own is, so copying and moving a ControlText stay the two above.
        template<typename Arg>
            requires (!std::derived_from<std::remove_cvref_t<Arg>, Text>)
        ControlText& operator=(Arg&& arg)
        {
            Text::operator=(std::forward<Arg>(arg));
            return changed();
        }

    public:
        ControlText() = default;
        // A copy and a move both produce a **new** text, so neither takes the other's id - and the
        // one a move empties has changed as much as anything here ever changes.
        ControlText(const ControlText& other) : Text{ other } {}
        ControlText(ControlText&& other) noexcept : Text{ std::move(other) } { other.changed(); }

        template<typename Arg>
        ControlText& operator<<(Arg&& arg)
        {
            Text::operator<<(std::forward<Arg>(arg));
            return changed();
        }

        void clear()
        {
            Text::clear();
            changed();
        }

        void replaceText(const TextRange& range, const std::wstring_view str = {})
        {
            Text::replaceText(range, str);
            changed();
        }

        void replaceText(const TextRange& range, const Text& other)
        {
            Text::replaceText(range, other);
            changed();
        }

        [[nodiscard]] TextStamp stamp() const { return m_stamp; }

    private:
        ControlText& changed()
        {
            ++m_stamp.revision;
            return *this;
        }

    private:
        // Never reused, so a stamp taken on a text that has since died cannot be matched by
        // whatever comes to stand where it stood.
        static inline std::size_t s_lastId{ 0 };
        TextStamp m_stamp{ ++s_lastId, 0 };
    };

    // The markers a format takes as they are - each a value the text runs on from.
    export template<typename Item>
    concept IsFormatState = std::same_as<Item, PushThemeColor>
        || std::same_as<Item, PushCustomColor>
        || std::same_as<Item, TextAlign>
        || std::same_as<Item, ParaIndent>
        || std::same_as<Item, ParaLineSpacing>
        || std::same_as<Item, PushFontSize>
        || std::same_as<Item, PushFontFamily>;

    // Markers alone, laid out ahead of a text's own and under them. See TextEngine-Types#textformat
    export class TextFormat
    {
    public:
        TextFormat() = default;
        template<typename... Args>
        explicit TextFormat(Args&&...);
    public:
        // Each replaces what the format already holds of its kind.
        template<IsFormatState Item>
        TextFormat& operator<<(const Item&);
        TextFormat& operator<<(Ink);
        TextFormat& operator<<(InkGrade); // the framework's own ink at that step
        TextFormat& operator<<(InkColor); // that colour at Strongest
        TextFormat& operator<<(Color);
        TextFormat& operator<<(TextStyle);
        TextFormat& operator<<(TextOp); // bold or italic
        TextFormat& operator<<(const TextFormat&);
        // Refused, a value that only converts to one of the above included - a wchar_t is no Color.
        template<typename Other>
        TextFormat& operator<<(const Other&) = delete;
        [[nodiscard]] bool empty() const { return m_markers.empty(); }
        [[nodiscard]] const Text::Markers& markers() const { return m_markers; }
        [[nodiscard]] std::size_t hash() const;
        // Whether this says what that one says, where none says nothing.
        [[nodiscard]] bool matches(const TextFormat*) const;
        bool operator==(const TextFormat&) const = default;
    private:
        void place(FormatItem);
    private:
        Text::Markers m_markers{}; // every one standing at 0, one of each kind in kind order
    };


//-----------------------------------------------------------------------------


    template<typename... Content>
    ScopedText<Content...>::ScopedText(FormatItem opening, Content&&... content)
        :
        m_opening{ std::move(opening) },
        m_content{ std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    StyledText<Content...>::StyledText(const TextStyleProps& style, Content&&... content)
        :
        ScopedText<Content...>{ PushTextStyle{ styleOf(style) }, std::forward<Content>(content)... }
    {
    }

    // A style is written by the TextStyle it is one of.
    template<typename... Content>
    TextStyle StyledText<Content...>::styleOf(const TextStyleProps& style)
    {
        for (std::size_t index = 0; index != static_cast<std::size_t>(TextStyle::Count); ++index)
        {
            const TextStyle candidate = static_cast<TextStyle>(index);
            if (&TextStyles::Of(candidate) == &style)
                return candidate;
        }
        noReach("A StyledText takes one of TextStyles");
    }

    template<typename... Content>
    ColoredText<Content...>::ColoredText(Ink ink, Content&&... content)
        :
        ScopedText<Content...>{ PushThemeColor{ ink }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ColoredText<Content...>::ColoredText(InkGrade grade, Content&&... content)
        :
        ScopedText<Content...>{
            PushThemeColor{ InkWell::textInk(grade) },
            std::forward<Content>(content)...
        }
    {
    }

    template<typename... Content>
    ColoredText<Content...>::ColoredText(InkColor color, Content&&... content)
        :
        ScopedText<Content...>{ PushThemeColor{ inkOf(color) }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ColoredText<Content...>::ColoredText(Color color, Content&&... content)
        :
        ScopedText<Content...>{ PushCustomColor{ color }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    constexpr Ink ColoredText<Content...>::inkOf(InkColor color)
    {
        Ink ink = InkWell::textInk();
        ink.color = color;
        return ink;
    }

    template<typename... Content>
    BoldText<Content...>::BoldText(Content&&... content)
        :
        ScopedText<Content...>{ TextOp::PushBold, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ItalicText<Content...>::ItalicText(Content&&... content)
        :
        ScopedText<Content...>{ TextOp::PushItalic, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    SizedText<Content...>::SizedText(float size, Content&&... content)
        :
        ScopedText<Content...>{ PushFontSize{ size }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    FamilyText<Content...>::FamilyText(std::wstring family, Content&&... content)
        :
        ScopedText<Content...>{
            PushFontFamily{ std::move(family) },
            std::forward<Content>(content)...
        }
    {
    }

    template<typename... Content>
    SuperscriptText<Content...>::SuperscriptText(Content&&... content)
        :
        ScopedText<Content...>{ TextOp::PushSuperscript, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    SubscriptText<Content...>::SubscriptText(Content&&... content)
        :
        ScopedText<Content...>{ TextOp::PushSubscript, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    LinkText<Content...>::LinkText(std::wstring target, Content&&... content)
        :
        ScopedText<Content...>{ PushLink{ std::move(target) }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    AnchorText<Content...>::AnchorText(std::wstring name, Content&&... content)
        :
        ScopedText<Content...>{ PushAnchor{ std::move(name) }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ParagraphText<Content...>::ParagraphText(TextAlign align, Content&&... content)
        :
        m_property{ align },
        m_content{ std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ParagraphText<Content...>::ParagraphText(ParaIndent indent, Content&&... content)
        :
        m_property{ indent },
        m_content{ std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ParagraphText<Content...>::ParagraphText(ParaLineSpacing spacing, Content&&... content)
        :
        m_property{ spacing },
        m_content{ std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    Text& Text::operator<<(const ScopedText<Content...>& scope)
    {
        const WriteStart start = openScope(scope.opening());
        std::apply([this](const auto&... content){
            ((*this << content), ...);
        }, scope.content());
        closeScope(start);
        return *this;
    }

    template<typename... Content>
    Text& Text::operator<<(const ParagraphText<Content...>& paragraph)
    {
        const WriteStart start = openParagraph(paragraph.property());
        std::apply([this](const auto&... content){
            ((*this << content), ...);
        }, paragraph.content());
        closeParagraph(start);
        return *this;
    }

    template<typename... Args>
    TextFormat::TextFormat(Args&&... args)
        :
        TextFormat{}
    {
        (*this << ... << std::forward<Args>(args));
    }

    template<IsFormatState Item>
    TextFormat& TextFormat::operator<<(const Item& item)
    {
        place(item);
        return *this;
    }

}
