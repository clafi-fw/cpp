export module ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi
{
    // WHICH text, and WHICH state of it. See TextEngine-Types
    export struct TextStamp
    {
        std::size_t id{ 0 };
        std::size_t revision{ 0 };
        [[nodiscard]] bool named() const { return id != 0; }
        bool operator==(const TextStamp&) const = default;
    };

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
        // marker that puts a character in - a space, an icon, a line end - stands at that
        // character.
        explicit Text(std::wstring plainText, Markers markers);

        // Anything the stream below takes, written as an assignment. What is assigned REPLACES
        // what the text holds, which is the one thing that separates this from operator<<: the
        // stream adds to a text, this states the whole of it. One argument, because an
        // assignment operator takes exactly one - several parts are a Text of their own:
        //     caption = Text{ Ink::accent, name, L" saved" };
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
        // WHAT THE FRAMEWORK COUNTS WITH, and so what a caller has in hand to write: a line, a
        // column, a length. Stated on its own rather than left to the int above, because an
        // unsigned reaches int and wchar_t by the same rank of conversion and neither wins - a
        // caller writing a count would be told the two are ambiguous.
        Text& operator<<(std::size_t);
        Text& operator<<(Ink);
        // The framework's own ink at one of the steps it draws in, written as the step alone:
        // caption << InkGrade::Muted. A share of a caller's own names no step, so it is stated as
        // the Ink it is.
        Text& operator<<(InkGrade);
        // One of the colours at Strongest, written as the colour alone: caption << InkColor::Red.
        Text& operator<<(InkColor);
        Text& operator<<(const PushThemeColor&);
        Text& operator<<(const PushCustomColor&);
        Text& operator<<(Color);
        Text& operator<<(const PopColor&);
        Text& operator<<(TextStyleId);
        Text& operator<<(const PopTextStyle&);
        Text& operator<<(TextAlign);
        Text& operator<<(const InTextIcon&);
        Text& operator<<(const SetIndent&);
        Text& operator<<(const Space&);
        Text& operator<<(const VSpace&);
        Text& operator<<(const FlexSpace&);
        Text& operator<<(const TabTo&);
        Text& operator<<(TextOp);
        Text& operator<<(const PushFontSize&);
        Text& operator<<(const PopFontSize&);
        Text& operator<<(const PushFontFamily&);
        Text& operator<<(const PopFontFamily&);
        Text& operator<<(const PushLink&);
        Text& operator<<(const PopLink&);
        Text& operator<<(const PushAnchor&);
        Text& operator<<(const PopAnchor&);
        Text& operator<<(const Text&);

        Text& setIndent(float indent);
        Text& setLineSpacing(float spacing);

        bool empty() const { return m_plainText.empty(); }
        // Nothing written into it at all, markers included. A text carrying only markers says
        // nothing and is not empty, which is the difference a caller collecting contributions has
        // to see - see GetTextEventBase::contribute.
        [[nodiscard]] bool blank() const { return m_plainText.empty() && m_markers.empty(); }
        void clear();
        std::size_t hash() const;
        // Whether a flex space is in here, which is the one thing a paint lays out differently
        // from a calculate: it fills the box it is drawn in and contributes nothing to a size.
        // Swept when it is asked for, so a Text carries no field for it.
        [[nodiscard]] bool hasFlexSpace() const;

        const std::wstring& plainText() const { return m_plainText; }
        bool operator==(const Text& other) const = default;
        [[nodiscard]] const Markers& markers() const { return m_markers; }
        // The text the first anchor of that name holds, or nothing where no anchor has it. A walk
        // over the markers, since an anchor is asked for when a link to it is followed and not
        // before. See TextEngine-Types#anchors
        [[nodiscard]] std::optional<TextRange> anchorRange(std::wstring_view name) const;

        // Edit support
        void replaceText(const TextRange&, const std::wstring_view = {});
        void replaceText(const TextRange&, const Text&);
        Text selectedText(const TextRange&) const;

    private:
        std::wstring m_plainText;
        Markers m_markers;
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
        // A copy and a move both produce a NEW text, so neither takes the other's id - and the
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

        ControlText& setIndent(float indent)
        {
            Text::setIndent(indent);
            return changed();
        }

        ControlText& setLineSpacing(float spacing)
        {
            Text::setLineSpacing(spacing);
            return changed();
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

}
