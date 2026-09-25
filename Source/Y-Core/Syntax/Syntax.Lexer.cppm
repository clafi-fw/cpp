export module ClaFi.Core.Syntax.Lexer;

import ClaFi.Core.Syntax.Types;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    // A comment: what opens it, and what closes it - nothing, for one that ends with its line.
    export struct CommentRule
    {
        std::wstring_view open;
        std::wstring_view close;
    };

    // A quoted literal, closed on its own line. The escape is L'\0' for a literal that has none.
    export struct StringRule
    {
        wchar_t open;
        wchar_t close;
        wchar_t escape;
    };

    // How a name no table lists is taken for a type.
    export enum class TypeRule
    {
        None,
        LeadingCapital,   // the framework's own convention, and most C++ code bases'
        PrefixedCapital   // Delphi's convention - a T, I, E or P with a capital after it
    };

    // The modes the lexer itself carries across lines. A language's own start at
    // firstLanguageMode, and a line standing in one of those is handed to the language's hook
    // before anything else looks at it.
    export namespace Mode
    {
        constexpr std::uint8_t normal = 0;
        constexpr std::uint8_t blockComment = 1;
        constexpr std::uint8_t firstLanguageMode = 8;
    }

    export using Words = std::span<const std::wstring_view>;

    export struct Scan;

    // A language's own rule, asked before the tables at every token start. See Syntax
    export using Hook = bool (*)(Scan&);

    // How sure a language is that a text is in it. Ordered, the surest greatest. See Syntax
    export enum class Claim
    {
        None,       // nothing of the language was seen
        Possible,   // nothing rules the language out, which is all a plain text can say
        Likely,     // the language's shape, which another language could share
        Certain     // a signature no other language spells
    };

    // A language's reading of a whole text, answering how sure it is of it. See Syntax
    export using Detector = Claim (*)(std::wstring_view text);

    // A language, stated as tables, one hook and one detector. See Syntax
    export struct Language
    {
        std::wstring_view name;
        Words keywords;                          // sorted, so a name is looked up by bisection
        Words types;                             // sorted
        Words constants;                         // sorted
        std::span<const CommentRule> comments;
        std::span<const StringRule> strings;
        std::wstring_view operators;             // every character that is one
        std::wstring_view punctuation;           // every character that is one
        std::wstring_view nameChars;             // what a name may hold besides letters, digits, _
        bool numbers{ false };                    // whether a digit opens a number
        bool ignoreCase{ false };                // a word matches its lower-case table in any case
        TypeRule typeRule{ TypeRule::None };
        bool callIsFunction{ false };            // a name standing before ( is a function
        bool keyBeforeColon{ false };            // a string standing before : is a property
        Hook hook{ nullptr };
        Detector detect{ nullptr };              // null for a language only ever picked by hand
    };

    // The strings a line state cannot spell, held by id. See Syntax
    export class StateStrings
    {
    public:
        // The id of that string, given on its first appearance and answered again after.
        [[nodiscard]] std::uint16_t idOf(std::wstring_view);
        [[nodiscard]] std::wstring_view stringOf(std::uint16_t id) const;
        void clear() { m_strings.clear(); }
    private:
        std::vector<std::wstring> m_strings;
    };

    // One line as the lexer walks it, which is what a hook is handed. See Syntax
    export struct Scan
    {
        const Language& language;
        std::wstring_view line;
        StateStrings& strings;
        Tokens* tokens;         // nothing while only the state the line ends in is asked for
        std::size_t index{ 0 };
        LineState state{};

        // Whether nothing but blanks stands before index.
        [[nodiscard]] bool atLineStart() const;
        [[nodiscard]] bool startsWith(std::wstring_view) const;
        [[nodiscard]] wchar_t current() const { return line[index]; }
        // The character that many past index, or L'\0' past the end of the line.
        [[nodiscard]] wchar_t peek(std::size_t ahead = 1) const;
        // The index of the first character past the blanks from `from`, or the line's size.
        [[nodiscard]] std::size_t pastBlanks(std::size_t from) const;
        void emit(std::size_t start, std::size_t end, TokenKind);
        // A quoted literal, from `start` to the close on this line or the line's end: the string,
        // and every escape inside it as a token of its own. The scan begins after `quote`, which
        // is where the literal's prefix, if it has one, ends. Leaves index past what it took.
        void takeQuoted(std::size_t start, std::size_t quote, wchar_t close, wchar_t escape);
    };

    // Lexes one line from the state it starts in and answers the state the next line starts in.
    // Tokens are appended, in order, when a vector is given.
    export [[nodiscard]] LineState lexLine(const Language&, std::wstring_view line, LineState,
        StateStrings&, Tokens*);

    // The language of a text: the one surest of it, the first among equals, and Language{} where
    // none claims it at all. A list's order says nothing else, so a language is added anywhere.
    export [[nodiscard]] Language detect(std::span<const Language>, std::wstring_view text);

    // Whether the word stands in a sorted table.
    export [[nodiscard]] bool isListed(Words, std::wstring_view word);
    // The same for a word spelled in any case, against a table written in lower case.
    export [[nodiscard]] bool isListedNoCase(Words, std::wstring_view word);
    // A space or a tab. Neither newline is one: a line never holds its own.
    export [[nodiscard]] bool isBlank(wchar_t);
    // One of the ten ASCII digits, which is what opens a number in every language here.
    export [[nodiscard]] bool isDigit(wchar_t);
    export [[nodiscard]] bool isNameStart(wchar_t, const Language&);
    export [[nodiscard]] bool isNameChar(wchar_t, const Language&);
    // One past the name starting at `start`.
    export [[nodiscard]] std::size_t endOfName(std::wstring_view line, std::size_t start,
        const Language&);
    // One past the number starting at `start`. A number swallows what follows it that could belong
    // to one - a radix, an exponent with its sign, a suffix, a digit separator - because telling
    // 0x7F from 0 followed by x7F needs a parser and drawing them as one run does not. A dot with
    // a dot after it belongs to no number: it is a range's, as in 0..9.
    export [[nodiscard]] std::size_t endOfNumber(std::wstring_view line, std::size_t start);
}
