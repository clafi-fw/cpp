export module ClaFi.Core.TextEngine.Text;

import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
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

    // The ink a link is drawn in where its words state none. See TextEngine-Types#links
    export constexpr Ink k_linkInk = InkWell::accentInk();

    // What a wrapper lays over the words it holds. See TextEngine-Types#textlayer
    export struct TextLayer
    {
        std::wstring style{}; // a name, found where the words are written
        TextStyle fields{}; // stated over that style, itself named none and based on none
        std::wstring link{};
        std::wstring anchor{};
    };

    // Words a wrapper lays a style, a link or an anchor over. See TextEngine-Types#scopedtext
    export template<typename... Content>
    class ScopedText
    {
    public:
        using ContentTuple = std::tuple<Content...>;

    public:
        explicit ScopedText(TextLayer, Content&&...);

    public:
        [[nodiscard]] const TextLayer& layer() const { return m_layer; }
        [[nodiscard]] const ContentTuple& content() const { return m_content; }

    private:
        TextLayer m_layer;
        ContentTuple m_content;
    };

    // Words set in the style of that name, found as written. See TextEngine-Types#styledtext
    export template<typename... Content>
    class StyledText : public ScopedText<Content...>
    {
    public:
        explicit StyledText(std::wstring_view style, Content&&...);
    };

    template<typename... Content>
    StyledText(std::wstring_view, Content&&...) -> StyledText<Content...>;

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
        [[nodiscard]] static TextLayer layerOf(ColorDef ink);
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
        [[nodiscard]] const ParagraphLook& look() const { return m_look; }
        [[nodiscard]] const ContentTuple& content() const { return m_content; }

    private:
        ParagraphLook m_look;
        ContentTuple m_content;
    };

    template<typename... Content>
    ParagraphText(TextAlign, Content&&...) -> ParagraphText<Content...>;
    template<typename... Content>
    ParagraphText(ParaIndent, Content&&...) -> ParagraphText<Content...>;
    template<typename... Content>
    ParagraphText(ParaLineSpacing, Content&&...) -> ParagraphText<Content...>;

    // A run of a text as a document holds it, or a paragraph's look. See TextEngine-Types#textpiece
    export struct TextPiece
    {
        std::wstring words{};
        TextStyle style{}; // named none: its own fields over a style the text reaches
        std::wstring link{};
        std::wstring anchor{};
        std::optional<InlineObject> object{}; // the words then hold its one character
        std::optional<ParagraphLook> paragraph{}; // stated by a piece holding no words
    };

    export using TextPieces = std::vector<TextPiece>;

    // One run of a text as a reader walks it. See TextEngine-Types#textrunview
    export struct TextRunView
    {
        TextRange range{};
        const TextStyle& style; // the text's own, named none
        std::wstring_view link{};
        std::wstring_view anchor{};
        const InlineObject* object{ nullptr };
    };

    // Whether the first of the arguments is a sheet, which a text is built over.
    template<typename... Args>
    [[nodiscard]] consteval bool startsWithSheet()
    {
        if constexpr (sizeof...(Args) == 0)
            return false;
        else
            return std::same_as<std::remove_cvref_t<std::tuple_element_t<0, std::tuple<Args...>>>,
                StyleSheet>;
    }

    // Words in runs, each in a style of the text's own, over a sheet. See TextEngine-Types#text
    export class Text
    {
    public:
        // A text over the application's sheet.
        Text();
        // A text over that sheet, which outlives it. See TextEngine-Types#text
        explicit Text(const StyleSheet& sheet);
        // Refused: a sheet made for the call is gone before the text is drawn.
        explicit Text(const StyleSheet&&) = delete;
        // Constrained off a lone Text, which is copied or moved, and off a leading sheet.
        template<typename... Args>
            requires (sizeof...(Args) != 1 || !(std::derived_from<std::remove_cvref_t<Args>, Text> && ...))
                && (!startsWithSheet<Args...>())
        explicit Text(Args&&... args) : Text{} { (*this << ... << std::forward<Args>(args)); }
        // Words written over that sheet.
        template<typename... Args>
            requires (sizeof...(Args) != 0)
        explicit Text(const StyleSheet& sheet, Args&&... args) : Text{ sheet }
        {
            (*this << ... << std::forward<Args>(args));
        }
        // Refused: a sheet made for the call is gone before the text is drawn.
        template<typename... Args>
        explicit Text(const StyleSheet&&, Args&&...) = delete;
        // A text stated whole, the way a document hands one back. See TextEngine-Types#textpiece
        explicit Text(TextPieces);
        // The same over its own styles, given whole. See TextEngine-Types#textpiece
        Text(TextPieces, StyleSheet styles);
        Text(const Text&) = default;
        Text(Text&&) noexcept = default;
        // Every style of the other's own comes along, and the sheet it is over.
        Text& operator=(const Text&);
        Text& operator=(Text&&) noexcept;
        ~Text() = default;

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
        // this stands beside the two declared above.
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
        // Every run as it is, under whatever wrapper the text is written inside.
        Text& operator<<(const Text&);
        // A run that holds no character writes nothing.
        template<typename... Content>
        Text& operator<<(const ScopedText<Content...>&);
        // Lines that hold no character write nothing. The words written next start a line.
        template<typename... Content>
        Text& operator<<(const ParagraphText<Content...>&);

        [[nodiscard]] bool empty() const { return m_plainText.empty(); }
        void clear();
        [[nodiscard]] std::size_t hash() const;
        // Whether a flex space is in here, which is the one thing a paint lays out differently
        // from a calculate: it fills the box it is drawn in and contributes nothing to a size.
        // Swept when it is asked for, so a Text carries no field for it.
        [[nodiscard]] bool hasFlexSpace() const;

        [[nodiscard]] const std::wstring& plainText() const { return m_plainText; }
        // The run styles and the named ones the text carries. See TextEngine-Types#text-styles
        [[nodiscard]] const StyleSheet& styles() const { return m_styles; }
        // Over one sheet, words drawn alike and the same paragraph looks. See TextEngine-Types#text
        bool operator==(const Text&) const;
        // Each run in order, handed to visit as a TextRunView.
        template<typename Visit>
        void forEachRun(Visit visit) const;
        // The look stated for the paragraph starting there, empty where it states none.
        [[nodiscard]] ParagraphLook paragraphLookAt(std::size_t paragraphStart) const;
        // The runs and the paragraphs' looks in text order, a paragraph's look ahead of its runs.
        [[nodiscard]] TextPieces pieces() const;
        // The text the first anchor of that name holds, or nothing where no anchor has it.
        [[nodiscard]] std::optional<TextRange> anchorRange(std::wstring_view name) const;
        // The link a caret at that position stands in, its ends included; nothing off every link.
        [[nodiscard]] std::optional<LinkSpan> linkAtCaret(std::size_t caretPos) const;
        // The target of the first link in the text, or nothing where it holds none.
        [[nodiscard]] std::optional<std::wstring_view> firstLink() const;

        // Words put in where they go, in the style there. See TextEngine-Types#edits
        void replaceText(const TextRange&, const std::wstring_view = {});
        // A text put in where it goes, its runs inside the style there. See TextEngine-Types#edits
        void replaceText(const TextRange&, const Text&);
        // A text restorableText took out put back as it was. See TextEngine-Types#edits
        void restoreText(const TextRange&, const Text&,
            const std::optional<ParagraphLook>& startParagraphLook = std::nullopt);
        // The runs of a range and the looks of the paragraphs starting in it. See TextEngine-Types
        [[nodiscard]] Text selectedText(const TextRange&) const;
        // The same, less a look for a paragraph starting at its start. See TextEngine-Types#edits
        [[nodiscard]] Text restorableText(const TextRange&) const;

    private:
        // A stretch of characters in one style: its length, and an index into a table for the rest.
        struct Run
        {
            std::size_t length{ 0 };
            std::uint32_t style{ 0 };
            std::uint32_t link{ 0 };
            std::uint32_t anchor{ 0 };
            std::uint32_t object{ 0 };
        };

        // The look stated for the paragraph that starts at a position.
        struct StatedParagraph
        {
            std::size_t start{ 0 };
            ParagraphLook look{};
            bool operator==(const StatedParagraph&) const = default;
        };

        // How the text stood when a wrapper started writing into it.
        struct WriteStart
        {
            std::size_t plainLength{ 0 };
            bool lineBroken{ false }; // the wrapper ended the line it found open
            bool startsOwnLine{ false };
            bool endsOwnLine{ false };
        };

        // What words put into the text take on where they go, as indexes into its tables.
        struct Surroundings
        {
            std::uint32_t style{ 0 };
            std::uint32_t link{ 0 };
            std::uint32_t anchor{ 0 };
        };

        // The run holding a character, and where the run starts.
        struct RunPlace
        {
            std::size_t index{ 0 };
            std::size_t start{ 0 };
        };

        // How a text put in meets the runs and paragraphs around it.
        enum class Insertion
        {
            Nested,
            Restored
        };

        using Runs = std::vector<Run>;
        using Names = std::vector<std::wstring>;
        using Objects = std::vector<InlineObject>;
        using StatedParagraphs = std::vector<StatedParagraph>;
        // Where each entry of another text's table landed in this one's.
        using IndexMap = std::vector<std::uint32_t>;

    private:
        [[nodiscard]] static bool joins(const Run& before, const Run& after);
        [[nodiscard]] const TextStyle& styleAt(std::uint32_t style) const;
        [[nodiscard]] std::wstring_view nameAt(std::uint32_t name) const;
        [[nodiscard]] std::uint32_t styleIndexOf(const TextStyle&);
        // A style from that sheet, or one unknown, found here. See TextEngine-Types#text-styles
        [[nodiscard]] const TextStyle* adopted(const TextStyle*, const StyleSheet* from);
        // A run style from elsewhere, its base found here.
        [[nodiscard]] std::uint32_t adoptedIndexOf(const TextStyle&, const StyleSheet* from);
        // The style a layer lays, its own found here by name. See TextEngine-Types#styledtext
        [[nodiscard]] TextStyle styleOf(const TextLayer&) const;
        [[nodiscard]] std::uint32_t nameIndexOf(std::wstring_view);
        [[nodiscard]] const StatedParagraph* statedAt(std::size_t paragraphStart) const;
        [[nodiscard]] std::size_t paragraphStartOf(std::size_t pos) const;
        [[nodiscard]] bool startsParagraph(std::size_t pos) const;
        [[nodiscard]] RunPlace placeOf(std::size_t pos) const;
        [[nodiscard]] std::size_t splitAt(std::size_t pos);
        void joinRuns(std::size_t first, std::size_t last);
        void appendRun(const Run&);
        void appendWords(std::wstring_view words);
        void appendObject(InlineObject);
        void dropLastCharacter();
        void stateParagraph(std::size_t paragraphStart, const ParagraphLook&);
        void layParagraph(std::size_t paragraphStart, const ParagraphLook& outer);
        [[nodiscard]] Runs runsOf(const Text& other, std::size_t from, std::size_t to);
        [[nodiscard]] Surroundings surroundingsOf(std::size_t start, std::size_t end) const;
        [[nodiscard]] Surroundings surroundingsAfterLineEnd(std::size_t end,
            const Surroundings& before) const;
        void nestRuns(std::span<Run> runs, const Surroundings&);
        void dropUnusedObjects();
        void edit(const TextRange&, const Text& inserted, Insertion);
        void editParagraphs(std::size_t start, std::size_t end, const Text& inserted, Insertion);
        void applyLayer(std::size_t from, std::size_t to, const TextLayer&);
        [[nodiscard]] WriteStart openScope();
        void closeScope(const WriteStart&, const TextLayer&);
        [[nodiscard]] WriteStart openParagraph();
        void closeParagraph(const WriteStart&, const ParagraphLook&);
        void undoOpening(const WriteStart&);
        void breakOwnLine();
        // A text holding nothing, over the sheet this one is over.
        [[nodiscard]] Text blank() const;
        void swap(Text&) noexcept;

    private:
        std::wstring m_plainText;
        Runs m_runs;
        StyleSheet m_styles; // run.style - 1, over the sheet the text is built over
        Names m_names; // the links and anchors, by run.link - 1 and run.anchor - 1
        Objects m_objects; // run.object - 1
        StatedParagraphs m_paragraphs; // in text order, only those stating a look
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

        void restoreText(const TextRange& range, const Text& taken,
            const std::optional<ParagraphLook>& startParagraphLook = std::nullopt)
        {
            Text::restoreText(range, taken, startParagraphLook);
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

    // The style a text is laid out over, and its paragraphs' look. See TextEngine-Types#textformat
    export class TextFormat
    {
    public:
        TextFormat() = default;
        template<typename... Args>
        explicit TextFormat(Args&&...);
    public:
        // Each replaces what the format already states of its kind.
        TextFormat& operator<<(std::wstring_view style); // a name, found where the text is laid out
        TextFormat& operator<<(Ink);
        TextFormat& operator<<(InkGrade); // the framework's own ink at that step
        TextFormat& operator<<(InkColor); // that colour at Strongest
        TextFormat& operator<<(Color);
        TextFormat& operator<<(TextAlign);
        TextFormat& operator<<(ParaIndent);
        TextFormat& operator<<(ParaLineSpacing);
        // What that one states, each field over this one's.
        TextFormat& operator<<(const TextFormat&);
        // Refused, a value that only converts to one of the above included - a wchar_t is no Color.
        template<typename Other>
            requires (!std::convertible_to<const Other&, std::wstring_view>)
        TextFormat& operator<<(const Other&) = delete;
        [[nodiscard]] bool empty() const;
        // The name of the style, empty where the format states none.
        [[nodiscard]] const std::wstring& style() const { return m_style; }
        // The fields stated over the style, named none and based on none.
        [[nodiscard]] const TextStyle& fields() const { return m_fields; }
        [[nodiscard]] const ParagraphLook& paragraph() const { return m_paragraph; }
        [[nodiscard]] std::size_t hash() const;
        // Whether this says what that one says, where none says nothing.
        [[nodiscard]] bool matches(const TextFormat*) const;
        bool operator==(const TextFormat&) const = default;
    private:
        std::wstring m_style{};
        TextStyle m_fields{};
        ParagraphLook m_paragraph{};
    };

//-----------------------------------------------------------------------------


    template<typename... Content>
    ScopedText<Content...>::ScopedText(TextLayer layer, Content&&... content)
        :
        m_layer{ std::move(layer) },
        m_content{ std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    StyledText<Content...>::StyledText(std::wstring_view style, Content&&... content)
        :
        ScopedText<Content...>{
            TextLayer{ .style = std::wstring{ style } },
            std::forward<Content>(content)...
        }
    {
    }

    template<typename... Content>
    ColoredText<Content...>::ColoredText(Ink ink, Content&&... content)
        :
        ScopedText<Content...>{ layerOf(ink), std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ColoredText<Content...>::ColoredText(InkGrade grade, Content&&... content)
        :
        ScopedText<Content...>{ layerOf(InkWell::textInk(grade)), std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ColoredText<Content...>::ColoredText(InkColor color, Content&&... content)
        :
        ScopedText<Content...>{ layerOf(inkOf(color)), std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ColoredText<Content...>::ColoredText(Color color, Content&&... content)
        :
        ScopedText<Content...>{ layerOf(color), std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    TextLayer ColoredText<Content...>::layerOf(ColorDef ink)
    {
        return TextLayer{ .fields{ .ink = ink } };
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
        ScopedText<Content...>{
            TextLayer{ .fields{ .fontWeight = FontWeight::Bold } },
            std::forward<Content>(content)...
        }
    {
    }

    template<typename... Content>
    ItalicText<Content...>::ItalicText(Content&&... content)
        :
        ScopedText<Content...>{
            TextLayer{ .fields{ .fontStyle = FontStyle::Italic } },
            std::forward<Content>(content)...
        }
    {
    }

    template<typename... Content>
    SizedText<Content...>::SizedText(float size, Content&&... content)
        :
        ScopedText<Content...>{ TextLayer{ .fields{ .fontSize = size } }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    FamilyText<Content...>::FamilyText(std::wstring family, Content&&... content)
        :
        ScopedText<Content...>{
            TextLayer{ .fields{ .fontFamily = std::move(family) } },
            std::forward<Content>(content)...
        }
    {
    }

    template<typename... Content>
    SuperscriptText<Content...>::SuperscriptText(Content&&... content)
        :
        ScopedText<Content...>{
            TextLayer{ .fields{ .scripts = { TextScript::Super } } },
            std::forward<Content>(content)...
        }
    {
    }

    template<typename... Content>
    SubscriptText<Content...>::SubscriptText(Content&&... content)
        :
        ScopedText<Content...>{
            TextLayer{ .fields{ .scripts = { TextScript::Sub } } },
            std::forward<Content>(content)...
        }
    {
    }

    template<typename... Content>
    LinkText<Content...>::LinkText(std::wstring target, Content&&... content)
        :
        ScopedText<Content...>{ TextLayer{ .link = std::move(target) }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    AnchorText<Content...>::AnchorText(std::wstring name, Content&&... content)
        :
        ScopedText<Content...>{ TextLayer{ .anchor = std::move(name) }, std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ParagraphText<Content...>::ParagraphText(TextAlign align, Content&&... content)
        :
        m_look{ .align = align },
        m_content{ std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ParagraphText<Content...>::ParagraphText(ParaIndent indent, Content&&... content)
        :
        m_look{ .indent = indent.indent },
        m_content{ std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    ParagraphText<Content...>::ParagraphText(ParaLineSpacing spacing, Content&&... content)
        :
        m_look{ .lineSpacing = spacing.spacing },
        m_content{ std::forward<Content>(content)... }
    {
    }

    template<typename... Content>
    Text& Text::operator<<(const ScopedText<Content...>& scope)
    {
        const WriteStart start = openScope();
        std::apply([this](const auto&... content){
            ((*this << content), ...);
        }, scope.content());
        closeScope(start, scope.layer());
        return *this;
    }

    template<typename... Content>
    Text& Text::operator<<(const ParagraphText<Content...>& paragraph)
    {
        const WriteStart start = openParagraph();
        std::apply([this](const auto&... content){
            ((*this << content), ...);
        }, paragraph.content());
        closeParagraph(start, paragraph.look());
        return *this;
    }

    template<typename Visit>
    void Text::forEachRun(Visit visit) const
    {
        std::size_t start = 0;
        for (const Run& run : m_runs)
        {
            visit(TextRunView{
                .range = { start, run.length },
                .style = styleAt(run.style),
                .link = nameAt(run.link),
                .anchor = nameAt(run.anchor),
                .object = run.object ? &m_objects[run.object - 1] : nullptr,
            });
            start += run.length;
        }
    }

    template<typename... Args>
    TextFormat::TextFormat(Args&&... args)
        :
        TextFormat{}
    {
        (*this << ... << std::forward<Args>(args));
    }

}
