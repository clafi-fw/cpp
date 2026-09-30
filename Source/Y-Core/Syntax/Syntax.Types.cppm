export module ClaFi.Core.Syntax.Types;

import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    // What a run of source is, as far as colouring it goes. See Syntax
    export enum class TokenKind : std::uint8_t
    {
        Plain,       // what no rule claims - an identifier, a space, a stray character
        Comment,
        Keyword,
        Type,        // a built-in type, or a name the language's rule takes for one
        Function,    // a name standing before its call parentheses
        Property,    // a key - a JSON key, a ClaFi key
        Constant,    // a named literal - true, nullptr, null
        String,      // a literal with its quotes, or a raw one with its delimiters
        Escape,      // an escape sequence inside a string, or an entity inside XML text
        Number,
        Operator,
        Punctuation, // a bracket, a separator, a terminator
        Directive,   // a preprocessor line, a ClaFi metadata line, an XML processing instruction
        Attribute,   // a C++ attribute, or an XML attribute name
        Tag,         // an XML element name
        Count
    };

    export constexpr std::size_t k_tokenKindCount = static_cast<std::size_t>(TokenKind::Count);

    // One run of a line, in the line's own coordinates.
    export struct Token
    {
        TextRange range;
        TokenKind kind{ TokenKind::Plain };
        bool operator==(const Token&) const = default;
    };

    export using Tokens = std::vector<Token>;

    // What a line starts in, carried over from the line before it. See Syntax
    export struct LineState
    {
        std::uint8_t mode{ 0 };   // zero is ordinary source; the rest the lexer's and the language's
        std::uint8_t aux{ 0 };    // which of a mode's several rules is open
        std::uint16_t text{ 0 };  // a string the mode carries, held in the StateStrings by this id
        bool operator==(const LineState&) const = default;
    };

    // What a name offered for completion is. See Syntax#completion
    export enum class CompletionKind : std::uint8_t
    {
        Keyword,
        Type,
        Class,
        Function,
        Procedure,
        Property,
        Field,
        Constant,
        Variable,
        Key,         // a key a title block states
        Value,       // a value a title block's key takes
        Count
    };

    export constexpr std::size_t k_completionKindCount =
        static_cast<std::size_t>(CompletionKind::Count);

    export struct CompletionEntry;
    export using CompletionEntries = std::vector<CompletionEntry>;

    // A name a host offers for completion, and what a hint says about it. See Syntax#completion
    export struct CompletionEntry
    {
        std::wstring name;
        CompletionKind kind{ CompletionKind::Variable };
        std::wstring signature;        // the line a hint shows - empty where the name is all of it
        std::wstring hint;             // what it does, in a sentence
        std::wstring type;             // the type it holds or a function answers - empty for none
        std::wstring parent;           // the class this one derives from - empty for none
        CompletionEntries methods;     // a class's methods - empty elsewhere
        CompletionEntries properties;  // a class's properties - empty elsewhere
        bool operator==(const CompletionEntry&) const = default;
    };

    // A key a text's title block states, and the values it takes. See Syntax#titleblock
    export struct TitleKey
    {
        // The key: its name, of kind Key, the line a hint shows and the hint's sentence.
        CompletionEntry entry;
        bool list{ false };          // whether it takes several values, a comma between each
        CompletionEntries values;    // what it takes, each of kind Value - empty for any text
        bool operator==(const TitleKey&) const = default;
    };

    export using TitleKeys = std::vector<TitleKey>;

    // The comment lines a text opens with, each stating a key and its value. See Syntax#titleblock
    export struct TitleBlock
    {
        std::wstring prefix;   // what a key's line opens with - a comment's opener and a mark
        TitleKeys keys;
        bool operator==(const TitleBlock&) const = default;
    };

    // A name a text declares for itself, and where it is in force. See Syntax#declarations
    export struct Declaration
    {
        CompletionEntry entry;    // the name, its kind and type, and the declaration as spelled
        TextRange scope;          // the run of the text the name is in force in
        std::size_t depth{ 0 };   // how many routines stand around the declaration
        bool operator==(const Declaration&) const = default;
    };

    export using Declarations = std::vector<Declaration>;

    // A language's reading of the names a text declares. See Syntax#declarations
    export using DeclarationReader = Declarations (*)(std::wstring_view text);

    // The parameters a signature spells, and where its list opens. See Syntax#parameters
    export struct SignatureParameters
    {
        std::size_t bracket{ 0 };      // where the list opens - the signature's size with no list
        std::vector<TextRange> names;  // each parameter's name, in the order a call fills them
        bool operator==(const SignatureParameters&) const = default;
    };

    // A language's reading of the parameters a signature spells. See Syntax#parameters
    export using ParameterReader = SignatureParameters (*)(std::wstring_view signature);

    // Where a language places one line of a text. See Syntax#indent
    export struct LineIndent
    {
        std::size_t column{ 0 };      // where the line stands, worked out from the lines above it
        bool placesItself{ false };   // its first word decides its column - a closer, an else
        std::wstring_view closer{};   // what ends the block the line leaves open - empty for none
    };

    // The lines of a text as a language's indent rule reads them. See Syntax#indent
    export class SourceLines
    {
    public:
        virtual ~SourceLines() = default;
    public:
        [[nodiscard]] virtual std::size_t count() const = 0;
        // The line without its newline.
        [[nodiscard]] virtual std::wstring_view text(std::size_t line) const = 0;
        // The line's tokens, in its own coordinates.
        [[nodiscard]] virtual const Tokens& tokens(std::size_t line) const = 0;
        // Whether the line starts inside a comment or a string the line before it left open.
        [[nodiscard]] virtual bool continues(std::size_t line) const = 0;
    };

    // A language's reading of where a line stands, a level being `width` columns. See Syntax#indent
    export using IndentRule = LineIndent (*)(const SourceLines&, std::size_t line,
        std::size_t width);

    // A block of source that stands on more than one line: where it opens and where it closes.
    export struct SourceBlock
    {
        std::size_t opener{ 0 };   // the line the block's opener stands on
        std::size_t closer{ 0 };   // the line its closer stands on, below the opener's
        bool operator==(const SourceBlock&) const = default;
    };

    export using SourceBlocks = std::vector<SourceBlock>;

    // A language's reading of the blocks a text opens and closes. See Syntax#blocks
    export using BlockReader = SourceBlocks (*)(const SourceLines&);

    // What each kind of token is drawn in. See Syntax
    export class Inks
    {
    public:
        constexpr Inks() = default;
        [[nodiscard]] constexpr const Ink& operator[](TokenKind kind) const { return m_inks[at(kind)]; }
        [[nodiscard]] constexpr Ink& operator[](TokenKind kind) { return m_inks[at(kind)]; }
        bool operator==(const Inks&) const = default;
    private:
        [[nodiscard]] static constexpr std::size_t at(TokenKind kind) { return static_cast<std::size_t>(kind); }
    private:
        std::array<Ink, k_tokenKindCount> m_inks{};
    };

    // The framework's own table, drawn from the palette's slots and the theme's rules so that it
    // holds on either side of the theme. Operators and punctuation stand a step behind the text,
    // so a line's structure recedes from what it carries.
    export [[nodiscard]] constexpr Inks defaultInks()
    {
        Inks result;
        result[TokenKind::Comment] = InkWell::textInk(InkGrade::Muted);
        result[TokenKind::Operator] = InkWell::textInk(InkGrade::Strong);
        result[TokenKind::Punctuation] = InkWell::textInk(InkGrade::Strong);
        result[TokenKind::Keyword] = InkWell::Blue;
        result[TokenKind::Type] = InkWell::spotInk();
        result[TokenKind::Function] = InkWell::Yellow;
        result[TokenKind::Property] = InkWell::accentInk();
        result[TokenKind::Constant] = InkWell::Blue;
        result[TokenKind::String] = InkWell::Red;
        result[TokenKind::Escape] = InkWell::Yellow;
        result[TokenKind::Number] = InkWell::Green;
        result[TokenKind::Directive] = InkWell::spotInk(InkGrade::Strong);
        result[TokenKind::Attribute] = InkWell::accentInk();
        result[TokenKind::Tag] = InkWell::Blue;
        return result;
    }
}
