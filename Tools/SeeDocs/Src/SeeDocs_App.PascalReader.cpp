module SeeDocs_App.PascalReader;

import SeeDocs_App.Reading;
import SeeDocs_App.Surface;

import ClaFi.Core.Syntax.Languages;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        constexpr std::wstring_view k_scope = L"::";
        // How many aliases a type is followed through before it is given up as circular.
        constexpr std::size_t k_aliasDepth = 8;
        // How many times a tree's symbols are gathered before they are taken as settled.
        constexpr std::size_t k_symbolPasses = 8;

        // The words no declaration may be named, which the language reserves. The directives -
        // virtual, read, message - are names anywhere they are not directives.
        constexpr auto k_reservedWords = std::to_array<std::wstring_view>({
            L"and", L"array", L"as", L"asm", L"begin", L"case", L"class", L"const",
            L"constructor", L"destructor", L"dispinterface", L"div", L"do", L"downto", L"else",
            L"end", L"except", L"exports", L"file", L"finalization", L"finally", L"for",
            L"function", L"goto", L"if", L"implementation", L"in", L"inherited",
            L"initialization", L"inline", L"interface", L"is", L"label", L"library", L"mod",
            L"nil", L"not", L"object", L"of", L"or", L"packed", L"procedure", L"program",
            L"property", L"raise", L"record", L"repeat", L"resourcestring", L"set", L"shl",
            L"shr", L"string", L"then", L"threadvar", L"to", L"try", L"type", L"unit", L"until",
            L"uses", L"var", L"while", L"with", L"xor"
        });

        // A directive that ends a routine's declaration with a semicolon of its own.
        constexpr auto k_routineDirectives = std::to_array<std::wstring_view>({
            L"abstract", L"assembler", L"cdecl", L"delayed", L"deprecated", L"dispid",
            L"dynamic", L"experimental", L"export", L"external", L"far", L"final", L"forward",
            L"inline", L"library", L"local", L"message", L"near", L"overload", L"override",
            L"pascal", L"platform", L"register", L"reintroduce", L"safecall", L"static",
            L"stdcall", L"unsafe", L"varargs", L"virtual", L"winapi"
        });

        // What a method type is called with, written after its semicolon.
        constexpr auto k_callingConventions = std::to_array<std::wstring_view>({
            L"cdecl", L"pascal", L"register", L"safecall", L"stdcall", L"winapi"
        });

        // What a declaration may say of itself before its semicolon.
        constexpr auto k_hintDirectives = std::to_array<std::wstring_view>({
            L"deprecated", L"experimental", L"library", L"platform"
        });

        // What may follow a variable's type before its semicolon.
        constexpr auto k_variableStops = std::to_array<std::wstring_view>({
            L"absolute", L"deprecated", L"experimental", L"library", L"platform"
        });

        // The words a property's declaration is written with after its type.
        constexpr auto k_propertySpecifiers = std::to_array<std::wstring_view>({
            L"default", L"dispid", L"implements", L"index", L"nodefault", L"read", L"readonly",
            L"stored", L"write", L"writeonly"
        });

        constexpr auto k_routineWords = std::to_array<std::wstring_view>({
            L"constructor", L"destructor", L"function", L"operator", L"procedure"
        });

        // The symbols the newest compiler - Delphi 13, VER370 - defines building for Windows,
        // which a conditional may read as defined.
        constexpr auto k_compilerSymbols = std::to_array<std::wstring_view>({
            L"assembler", L"conditionalexpressions", L"cpu32bits", L"cpu386", L"cpu64bits",
            L"cpux64", L"cpux86", L"dcc", L"mswindows", L"nativecode", L"underscoreimportname",
            L"unicode", L"ver370", L"win32", L"win64"
        });

        using Words = std::span<const std::wstring_view>;

        enum class TokenType
        {
            Name,
            Number,
            Literal,
            Punctuator,
            Comment,
            Directive
        };

        struct Token
        {
            TokenType type{ TokenType::Name };
            std::size_t line{ 0 };
            std::size_t start{ 0 };
            std::size_t length{ 0 };
            [[nodiscard]] std::size_t end() const { return start + length; }
        };

        using TokenList = std::vector<Token>;

        enum class LineKind
        {
            Blank,
            CommentOnly,
            Code
        };

        // Whether a bracket counts towards the depth a type's end is looked for at: a generic's
        // angle brackets do in a type, and in a value they compare.
        enum class Angles
        {
            Nest,
            Compare
        };

        [[nodiscard]] bool isNameStart(const wchar_t value)
        {
            return value == L'_' || std::iswalpha(value) != 0;
        }

        [[nodiscard]] bool isNameChar(const wchar_t value)
        {
            return value == L'_' || std::iswalnum(value) != 0;
        }

        [[nodiscard]] bool isBlank(const wchar_t value)
        {
            return value == L' ' || value == L'\t';
        }

        [[nodiscard]] std::wstring lowered(const std::wstring_view text)
        {
            std::wstring result{ text };
            for (wchar_t& current : result)
                current = static_cast<wchar_t>(std::towlower(current));
            return result;
        }

        // Whether two words are one, the way the language reads them: in any case.
        [[nodiscard]] bool sameWord(const std::wstring_view left, const std::wstring_view right)
        {
            if (left.size() != right.size())
                return false;
            for (std::size_t i = 0; i != left.size(); ++i)
            {
                if (std::towlower(left[i]) != std::towlower(right[i]))
                    return false;
            }
            return true;
        }

        [[nodiscard]] bool isListed(const Words words, const std::wstring_view word)
        {
            return std::ranges::any_of(words, [word](const std::wstring_view listed){
                return sameWord(listed, word);
            });
        }

        // The words one line of a comment says: its brackets, slashes and stars taken off, and
        // the mark a doc comment opens with - //! and //< - after them.
        [[nodiscard]] std::wstring_view commentWords(std::wstring_view comment)
        {
            comment = trimmed(comment);
            if (comment.starts_with(L"(*"))
                comment.remove_prefix(2);
            else if (comment.starts_with(L'{'))
                comment.remove_prefix(1);
            while (comment.starts_with(L'/'))
                comment.remove_prefix(1);
            if (comment.ends_with(L"*)"))
                comment.remove_suffix(2);
            else if (comment.ends_with(L'}'))
                comment.remove_suffix(1);
            comment = trimmed(comment);
            while (comment.starts_with(L'*'))
                comment.remove_prefix(1);
            const bool marked = comment.starts_with(L'!')
                || (comment.starts_with(L"< ") && !comment.starts_with(L"< /"));
            if (marked)
                comment.remove_prefix(1);
            return trimmed(comment);
        }

        // The words with the markup a doc comment writes in them taken out: an XML tag goes, the
        // name a <see cref="X"/> gives stays, and a PasDoc @link(X) is its X.
        [[nodiscard]] std::wstring withoutMarkup(const std::wstring_view text)
        {
            constexpr std::wstring_view crefOpener = L"cref=\"";
            std::wstring result;
            std::size_t i = 0;
            while (i < text.size())
            {
                const wchar_t current = text[i];
                if (current == L'<')
                {
                    const std::size_t close = text.find(L'>', i);
                    if (close == std::wstring_view::npos)
                        break;
                    const std::wstring_view tag = text.substr(i, close - i + 1);
                    const std::size_t cref = tag.find(crefOpener);
                    if (cref != std::wstring_view::npos)
                    {
                        const std::size_t from = cref + crefOpener.size();
                        result += tag.substr(from, tag.find(L'"', from) - from);
                    }
                    i = close + 1;
                    continue;
                }
                if (current == L'@' && i + 1 < text.size() && isNameStart(text[i + 1]))
                {
                    std::size_t end = i + 1;
                    while (end < text.size() && isNameChar(text[end]))
                        ++end;
                    const std::size_t close = text.find(L')', end);
                    if (end < text.size() && text[end] == L'(' && close != std::wstring_view::npos)
                    {
                        result += text.substr(end + 1, close - end - 1);
                        i = close + 1;
                        continue;
                    }
                }
                result += current;
                ++i;
            }
            return result;
        }

        // Blanks and line ends folded into one blank each, the ends trimmed.
        [[nodiscard]] std::wstring folded(const std::wstring_view text)
        {
            std::wstring result;
            bool blank = false;
            for (const wchar_t current : text)
            {
                if (isBlank(current) || current == L'\n' || current == L'\r')
                {
                    blank = true;
                    continue;
                }
                if (blank && !result.empty())
                    result += L' ';
                blank = false;
                result += current;
            }
            return result;
        }

        // THE HINT A DELPHI DOC COMMENT GIVES: its <summary>, Delphi's own brief, where it has
        // one; else what any doc comment gives, a line opening with a tag ending a paragraph as
        // a command does.
        [[nodiscard]] std::wstring pascalBrief(const std::span<const std::wstring> lines)
        {
            constexpr std::wstring_view opener = L"<summary>";
            constexpr std::wstring_view closer = L"</summary>";
            std::wstring joined;
            for (const std::wstring& line : lines)
                joined.append(line).append(1, L'\n');
            const std::wstring lower = lowered(joined);
            const std::size_t open = lower.find(opener);
            if (open != std::wstring::npos)
            {
                const std::size_t from = open + opener.size();
                const std::size_t close = lower.find(closer, from);
                const std::size_t length = close == std::wstring::npos
                    ? std::wstring::npos
                    : close - from;
                return folded(withoutMarkup(std::wstring_view{ joined }.substr(from, length)));
            }
            Names words;
            for (const std::wstring& line : lines)
            {
                if (line.starts_with(L'<'))
                    words.emplace_back(L"@tag");
                else
                    words.push_back(withoutMarkup(line));
            }
            return briefOf(words);
        }

        // The text cut at a separator standing outside brackets and quotes, each piece trimmed.
        [[nodiscard]] std::vector<std::wstring_view> piecesOf(const std::wstring_view text,
            const wchar_t separator)
        {
            std::vector<std::wstring_view> pieces;
            int depth = 0;
            bool quoted = false;
            std::size_t start = 0;
            for (std::size_t i = 0; i != text.size(); ++i)
            {
                const wchar_t current = text[i];
                if (current == L'\'')
                    quoted = !quoted;
                else if (quoted)
                    continue;
                else if (current == L'(' || current == L'[' || current == L'<')
                    ++depth;
                else if (current == L')' || current == L']' || current == L'>')
                    --depth;
                else if (current == separator && depth <= 0)
                {
                    pieces.push_back(trimmed(text.substr(start, i - start)));
                    start = i + 1;
                }
            }
            pieces.push_back(trimmed(text.substr(start)));
            return pieces;
        }

        // A parameter list the way overloads are told apart by it: per parameter, how it is
        // passed and its type, in lower case with no blanks - the names and defaults left out.
        [[nodiscard]] Names parameterShape(const std::wstring_view parameters)
        {
            constexpr auto passings = std::to_array<std::wstring_view>({
                L"const", L"constref", L"out", L"var"
            });
            Names shape;
            for (std::wstring_view group : piecesOf(parameters, L';'))
            {
                while (group.starts_with(L'['))
                {
                    const std::size_t close = group.find(L']');
                    if (close == std::wstring_view::npos)
                        break;
                    group = trimmed(group.substr(close + 1));
                }
                if (group.empty())
                    continue;
                std::size_t wordEnd = 0;
                while (wordEnd < group.size() && isNameChar(group[wordEnd]))
                    ++wordEnd;
                std::wstring passing;
                if (wordEnd < group.size() && isBlank(group[wordEnd])
                    && isListed(passings, group.substr(0, wordEnd)))
                {
                    passing = lowered(group.substr(0, wordEnd));
                    group = trimmed(group.substr(wordEnd));
                }
                const std::vector<std::wstring_view> typed = piecesOf(group, L':');
                std::wstring type;
                if (typed.size() > 1)
                {
                    const std::wstring_view spelled = piecesOf(typed[1], L'=').front();
                    for (const wchar_t current : spelled)
                    {
                        if (!isBlank(current))
                            type += static_cast<wchar_t>(std::towlower(current));
                    }
                }
                const std::size_t count = piecesOf(typed.front(), L',').size();
                for (std::size_t i = 0; i != count; ++i)
                    shape.push_back(passing + L':' + type);
            }
            return shape;
        }

        // A directive's word in lower case, and what follows it.
        struct Directive
        {
            std::wstring word;
            std::wstring_view rest;
        };

        // {$WORD rest} or (*$WORD rest*); nothing for a comment that is no directive.
        [[nodiscard]] std::optional<Directive> directiveOf(std::wstring_view body)
        {
            if (body.starts_with(L"{$"))
                body.remove_prefix(2);
            else if (body.starts_with(L"(*$"))
                body.remove_prefix(3);
            else
                return std::nullopt;
            if (body.ends_with(L"*)"))
                body.remove_suffix(2);
            else if (body.ends_with(L'}'))
                body.remove_suffix(1);
            std::size_t end = 0;
            while (end < body.size() && isNameChar(body[end]))
                ++end;
            return Directive{
                .word = lowered(body.substr(0, end)),
                .rest = trimmed(body.substr(end))
            };
        }

        // The symbol a condition opens with.
        [[nodiscard]] std::wstring_view leadingName(const std::wstring_view condition)
        {
            std::size_t end = 0;
            while (end < condition.size() && isNameChar(condition[end]))
                ++end;
            return condition.substr(0, end);
        }

        // The symbol of a condition that is Defined(X) and nothing else; empty for any other.
        [[nodiscard]] std::wstring_view definedTest(std::wstring_view condition)
        {
            constexpr std::wstring_view opener = L"defined(";
            const bool tests = condition.size() > opener.size()
                && sameWord(condition.substr(0, opener.size()), opener)
                && condition.ends_with(L')');
            if (!tests)
                return {};
            condition.remove_prefix(opener.size());
            condition.remove_suffix(1);
            condition = trimmed(condition);
            if (leadingName(condition).size() != condition.size())
                return {};
            return condition;
        }

        // Whether a text says $DEFINE anywhere, which spares lexing one that does not.
        [[nodiscard]] bool mentionsDefine(const std::wstring_view text)
        {
            constexpr std::wstring_view word = L"define";
            std::size_t at = text.find(L'$');
            while (at != std::wstring_view::npos)
            {
                if (sameWord(text.substr(at + 1, word.size()), word))
                    return true;
                at = text.find(L'$', at + 1);
            }
            return false;
        }

        // The branches a text's conditionals leave read - {$IFDEF X}, {$IF ...}, {$IFOPT ...},
        // {$ELSEIF ...}, {$ELSE}, {$ENDIF}, {$IFEND} - by the symbols defined. A conditional
        // that cannot hold is taken as one on a plain 0 is.
        class Conditionals
        {
        public:
            explicit Conditionals(const PascalSymbols&);
            // Any other directive steers nothing.
            void take(const Directive&);
            [[nodiscard]] bool reading() const { return m_branches.reading(); }
        private:
            // Whether a conditional can be read true: it tests no symbol nothing defines.
            [[nodiscard]] bool mayHold(std::wstring_view word, std::wstring_view condition) const;
        private:
            const PascalSymbols& m_symbols;
            Branches m_branches;
        };

        Conditionals::Conditionals(const PascalSymbols& symbols)
            :
            m_symbols{ symbols }
        {
        }

        void Conditionals::take(const Directive& directive)
        {
            std::wstring_view word = directive.word;
            if (word == L"ifopt")
                word = L"if";
            else if (word == L"elseif")
                word = L"elif";
            else if (word == L"ifend")
                word = L"endif";
            if (mayHold(word, directive.rest))
                m_branches.take(word, directive.rest);
            else
                m_branches.take(word == L"elif" ? L"elif" : L"if", L"0");
        }

        bool Conditionals::mayHold(const std::wstring_view word,
            const std::wstring_view condition) const
        {
            std::wstring_view symbol;
            if (word == L"ifdef")
                symbol = leadingName(condition);
            else if (word == L"if" || word == L"elif")
                symbol = definedTest(condition);
            if (symbol.empty())
                return true;
            const std::wstring lower = lowered(symbol);
            return m_symbols.contains(lower) || isListed(k_compilerSymbols, lower);
        }

        // Adds what a text defines in the branches it leaves read, a conditional reading by
        // the symbols seen so far: those gathered before and those the text defines above it.
        void gatherFrom(const std::wstring_view text, PascalSymbols& seen, PascalSymbols& found)
        {
            Conditionals conditionals{ seen };
            Syntax::LineState state{};
            Syntax::StateStrings strings;
            Syntax::Tokens lexed;
            std::size_t start = 0;
            while (start < text.size())
            {
                std::size_t end = text.find(L'\n', start);
                if (end == std::wstring_view::npos)
                    end = text.size();
                const std::wstring_view line = text.substr(start, end - start);
                lexed.clear();
                state = Syntax::lexLine(Syntax::Languages::pascal, line, state, strings, &lexed);
                for (const Syntax::Token& token : lexed)
                {
                    if (token.kind != Syntax::TokenKind::Directive)
                        continue;
                    const std::optional<Directive> directive =
                        directiveOf(line.substr(token.range.start, token.range.length));
                    if (!directive)
                        continue;
                    if (directive->word != L"define")
                    {
                        conditionals.take(*directive);
                        continue;
                    }
                    const std::wstring_view symbol = leadingName(directive->rest);
                    if (symbol.empty() || !conditionals.reading())
                        continue;
                    seen.insert(lowered(symbol));
                    found.insert(lowered(symbol));
                }
                start = end + 1;
            }
        }

        // A declaration under the word of the section it stands in: on one line with it, or
        // where it runs over several, set in under it as the unit sets it.
        [[nodiscard]] std::wstring inSection(const std::wstring_view section,
            const std::wstring_view written)
        {
            constexpr std::wstring_view indent = L"  ";
            std::wstring result{ section };
            if (written.find(L'\n') == std::wstring_view::npos)
                return result.append(1, L' ').append(written);
            std::size_t start = 0;
            while (start <= written.size())
            {
                std::size_t end = written.find(L'\n', start);
                if (end == std::wstring_view::npos)
                    end = written.size();
                const std::wstring_view line = written.substr(start, end - start);
                result += L'\n';
                if (!line.empty())
                    result.append(indent).append(line);
                start = end + 1;
            }
            return result;
        }

        // A unit as lines and tokens, lexed with the framework's Pascal lexer. A token in a
        // branch left out is never taken - see Conditionals - so a directive inside a line steers
        // the reading as one on a line of its own does.
        class UnitText
        {
        public:
            UnitText(std::wstring_view text, const PascalSymbols&);
            [[nodiscard]] const TokenList& tokens() const { return m_tokens; }
            [[nodiscard]] std::wstring_view text(const Token&) const;
            [[nodiscard]] LineKind kindOf(const std::size_t line) const { return m_kinds[line]; }
            [[nodiscard]] std::wstring_view line(std::size_t index) const;
            // The comment ending a line of code, or nothing.
            [[nodiscard]] std::optional<std::wstring_view> trailingComment(std::size_t line) const;
            // How many comment-only lines under a line carry its trailing comment on: those that
            // open in the column the trailing comment opens in.
            [[nodiscard]] std::size_t continuationsOf(std::size_t line) const;
            // The text from one token to another, both included, its comments and directives cut
            // out and its blanks folded.
            [[nodiscard]] std::wstring slice(const Token& from, const Token& to) const;
            // The text from one token to another as written, each line after the first set back
            // by the first one's indent.
            [[nodiscard]] std::wstring written(const Token& from, const Token& to) const;
        private:
            void lexLine(std::size_t index, Syntax::LineState&, Syntax::StateStrings&);
            void takeDirective(const Token&);
            void takeGap(std::size_t line, std::size_t from, std::size_t to);
            void push(TokenType, std::size_t line, std::size_t start, std::size_t end);
            void followComment(std::size_t firstToken);
            void classifyLine(std::size_t firstToken, bool insideComment);
            [[nodiscard]] const Token* trailingToken(std::size_t line) const;
        private:
            std::vector<std::wstring> m_lines;
            TokenList m_tokens;
            std::vector<LineKind> m_kinds;
            std::vector<std::size_t> m_firstTokens;
            Conditionals m_conditionals;
            std::wstring_view m_closer;   // closes the comment a line ended inside; empty for none
        };

        UnitText::UnitText(const std::wstring_view text, const PascalSymbols& symbols)
            :
            m_conditionals{ symbols }
        {
            std::size_t start = 0;
            while (start <= text.size())
            {
                const std::size_t end = text.find(L'\n', start);
                std::wstring_view piece = end == std::wstring_view::npos
                    ? text.substr(start)
                    : text.substr(start, end - start);
                if (piece.ends_with(L'\r'))
                    piece.remove_suffix(1);
                m_lines.emplace_back(piece);
                if (end == std::wstring_view::npos)
                    break;
                start = end + 1;
            }
            if (!m_lines.empty() && m_lines.front().starts_with(L'\xFEFF'))
                m_lines.front().erase(0, 1);

            Syntax::LineState state{};
            Syntax::StateStrings strings;
            for (std::size_t i = 0; i != m_lines.size(); ++i)
            {
                const std::size_t firstToken = m_tokens.size();
                m_firstTokens.push_back(firstToken);
                const bool insideComment = !m_closer.empty();
                lexLine(i, state, strings);
                followComment(firstToken);
                classifyLine(firstToken, insideComment);
            }
        }

        std::wstring_view UnitText::text(const Token& token) const
        {
            return std::wstring_view{ m_lines[token.line] }.substr(token.start, token.length);
        }

        std::wstring_view UnitText::line(const std::size_t index) const
        {
            return m_lines[index];
        }

        std::optional<std::wstring_view> UnitText::trailingComment(const std::size_t line) const
        {
            if (const Token* tail = trailingToken(line))
                return text(*tail);
            return std::nullopt;
        }

        std::size_t UnitText::continuationsOf(const std::size_t line) const
        {
            const Token* tail = trailingToken(line);
            if (!tail)
                return 0;
            const std::size_t column = columnOf(m_lines[line], tail->start);
            std::size_t count = 0;
            for (std::size_t next = line + 1; next < m_lines.size(); ++next)
            {
                if (m_kinds[next] != LineKind::CommentOnly || indentOf(m_lines[next]) != column)
                    break;
                ++count;
            }
            return count;
        }

        std::wstring UnitText::slice(const Token& from, const Token& to) const
        {
            std::wstring result;
            for (std::size_t line = from.line; line <= to.line; ++line)
            {
                const std::wstring_view whole = m_lines[line];
                const std::size_t start = line == from.line ? from.start : 0;
                const std::size_t end = line == to.line ? to.end() : whole.size();
                std::wstring piece;
                std::size_t at = start;
                for (std::size_t i = m_firstTokens[line]; i != m_tokens.size(); ++i)
                {
                    const Token& token = m_tokens[i];
                    if (token.line != line || token.start >= end)
                        break;
                    const bool cut = token.type == TokenType::Comment
                        || token.type == TokenType::Directive;
                    if (!cut || token.end() <= at)
                        continue;
                    const std::size_t cutStart = std::max(token.start, at);
                    piece += whole.substr(at, cutStart - at);
                    piece += L' ';
                    at = std::min(token.end(), end);
                }
                piece += whole.substr(at, end - at);
                if (!result.empty())
                    result += L' ';
                result += piece;
            }
            return folded(result);
        }

        std::wstring UnitText::written(const Token& from, const Token& to) const
        {
            const std::wstring_view first = m_lines[from.line];
            std::size_t indent = 0;
            while (indent < first.size() && isBlank(first[indent]))
                ++indent;
            std::wstring result;
            for (std::size_t line = from.line; line <= to.line; ++line)
            {
                std::wstring_view piece = m_lines[line];
                if (line == to.line)
                    piece = piece.substr(0, to.end());
                if (line == from.line)
                {
                    piece.remove_prefix(from.start);
                }
                else
                {
                    std::size_t blanks = 0;
                    while (blanks < indent && blanks < piece.size() && isBlank(piece[blanks]))
                        ++blanks;
                    piece.remove_prefix(blanks);
                    result += L'\n';
                }
                while (!piece.empty() && isBlank(piece.back()))
                    piece.remove_suffix(1);
                result += piece;
            }
            return result;
        }

        void UnitText::lexLine(const std::size_t index, Syntax::LineState& state,
            Syntax::StateStrings& strings)
        {
            const std::wstring_view line = m_lines[index];
            Syntax::Tokens lexed;
            state = Syntax::lexLine(Syntax::Languages::pascal, line, state, strings, &lexed);
            std::size_t cursor = 0;
            for (const Syntax::Token& token : lexed)
            {
                const std::size_t start = token.range.start;
                const std::size_t end = token.range.start + token.range.length;
                if (token.kind == Syntax::TokenKind::Directive)
                {
                    if (m_conditionals.reading())
                        takeGap(index, cursor, start);
                    push(TokenType::Directive, index, start, end);
                    takeDirective(m_tokens.back());
                    cursor = end;
                    continue;
                }
                if (!m_conditionals.reading())
                {
                    cursor = end;
                    continue;
                }
                takeGap(index, cursor, start);
                cursor = end;
                switch (token.kind)
                {
                    case Syntax::TokenKind::Comment:
                        push(TokenType::Comment, index, start, end);
                        break;
                    case Syntax::TokenKind::String:
                    case Syntax::TokenKind::Escape:
                    {
                        Token* last = m_tokens.empty() ? nullptr : &m_tokens.back();
                        const bool abuts = last && last->type == TokenType::Literal
                            && last->line == index && last->end() == start;
                        if (abuts)
                            last->length = end - last->start;
                        else
                            push(TokenType::Literal, index, start, end);
                        break;
                    }
                    case Syntax::TokenKind::Number:
                        push(TokenType::Number, index, start, end);
                        break;
                    case Syntax::TokenKind::Plain:
                    case Syntax::TokenKind::Operator:
                    case Syntax::TokenKind::Punctuation:
                        takeGap(index, start, end);
                        break;
                    default:
                        push(TokenType::Name, index, start, end);
                        break;
                }
            }
            if (m_conditionals.reading())
                takeGap(index, cursor, line.size());
        }

        // In braces or in (*$ *).
        void UnitText::takeDirective(const Token& token)
        {
            const std::optional<Directive> directive = directiveOf(text(token));
            if (directive)
                m_conditionals.take(*directive);
        }

        // What the lexer leaves between its tokens, as names, numbers and single marks. An
        // ampersand makes a name of a reserved word, and stays the name's.
        void UnitText::takeGap(const std::size_t line, std::size_t from, const std::size_t to)
        {
            const std::wstring_view text = m_lines[line];
            while (from < to)
            {
                const wchar_t current = text[from];
                if (isBlank(current))
                {
                    ++from;
                    continue;
                }
                const bool escaped = current == L'&' && from + 1 < to
                    && isNameStart(text[from + 1]);
                if (isNameStart(current) || escaped)
                {
                    std::size_t end = from + 1;
                    while (end < to && isNameChar(text[end]))
                        ++end;
                    push(TokenType::Name, line, from, end);
                    from = end;
                    continue;
                }
                if (std::iswdigit(current) != 0)
                {
                    std::size_t end = from + 1;
                    while (end < to && isNameChar(text[end]))
                        ++end;
                    push(TokenType::Number, line, from, end);
                    from = end;
                    continue;
                }
                push(TokenType::Punctuator, line, from, from + 1);
                ++from;
            }
        }

        void UnitText::push(const TokenType type, const std::size_t line, const std::size_t start,
            const std::size_t end)
        {
            if (end > start)
                m_tokens.push_back({ type, line, start, end - start });
        }

        // A { } or a (* *) comment left open at the line's end makes the lines after it a
        // comment's until the bracket that closes it.
        void UnitText::followComment(const std::size_t firstToken)
        {
            for (std::size_t i = firstToken; i != m_tokens.size(); ++i)
            {
                const Token& token = m_tokens[i];
                if (token.type != TokenType::Comment)
                    continue;
                const std::wstring_view comment = text(token);
                if (!m_closer.empty() && i == firstToken)
                {
                    if (comment.ends_with(m_closer))
                        m_closer = {};
                    continue;
                }
                const bool braceOpen = comment.starts_with(L'{') && !comment.ends_with(L'}');
                const bool parenOpen = comment.starts_with(L"(*")
                    && (comment.size() < 4 || !comment.ends_with(L"*)"));
                if (braceOpen)
                    m_closer = L"}";
                else if (parenOpen)
                    m_closer = L"*)";
            }
        }

        void UnitText::classifyLine(const std::size_t firstToken, const bool insideComment)
        {
            if (firstToken == m_tokens.size())
            {
                m_kinds.push_back(insideComment ? LineKind::CommentOnly : LineKind::Blank);
                return;
            }
            const auto first = m_tokens.begin() + static_cast<std::ptrdiff_t>(firstToken);
            const bool commentsOnly = std::all_of(first, m_tokens.end(), [](const Token& token){
                return token.type == TokenType::Comment;
            });
            m_kinds.push_back(commentsOnly ? LineKind::CommentOnly : LineKind::Code);
        }

        const Token* UnitText::trailingToken(const std::size_t line) const
        {
            if (m_kinds[line] != LineKind::Code)
                return nullptr;
            std::size_t last = m_firstTokens[line];
            while (last != m_tokens.size() && m_tokens[last].line == line)
                ++last;
            const Token& tail = m_tokens[last - 1];
            if (tail.type != TokenType::Comment)
                return nullptr;
            return &tail;
        }

        // Reads a unit's interface section into the surface. The interface is what the unit
        // publishes, so every declaration it holds is exported; a class member is the access
        // its section gives it, one before any section public. The implementation section gives
        // the hints the routines' headers there carry.
        class UnitReader
        {
        public:
            UnitReader(Surface&, std::size_t file, const UnitText&, std::wstring category);
            void read();
        private:
            [[nodiscard]] const Token* peek(std::size_t ahead = 0) const;
            void advance() { ++m_at; }
            [[nodiscard]] std::wstring_view text(const Token&) const;
            [[nodiscard]] bool isWord(const Token*, std::wstring_view word) const;
            [[nodiscard]] bool isMark(const Token*, std::wstring_view mark) const;
            [[nodiscard]] bool isListedWord(const Token*, Words) const;
            // A name the unit may declare: one the language does not reserve.
            [[nodiscard]] bool isDeclaredName(const Token*) const;
            // Whether the token so far ahead opens a routine, rather than naming a field.
            [[nodiscard]] bool opensRoutine(std::size_t ahead) const;
            // Whether a field list starts here - a name, then a comma or a colon.
            [[nodiscard]] bool opensFields() const;
            [[nodiscard]] const Token& code(std::size_t index) const;
            [[nodiscard]] Place placeOf(const Token&) const;
            [[nodiscard]] Comment commentAt(std::size_t line, std::wstring_view name);
            void gatherAbove(std::size_t line, Comment&) const;
            // The trailing comment on the line, with the lines that carry it on.
            [[nodiscard]] Names trailingLines(std::size_t line, const std::wstring& trailing) const;

            [[nodiscard]] bool readHeader();
            void readInterface();
            void readUses();
            void readTypeSection(const Type* outer, Access);
            void readTypeDeclaration(const Type* outer, Access);
            void readStructure(Type&);
            void readMembers(Type&);
            void readEnum(Type&);
            void readConstants(std::wstring_view section, Type* owner, Access);
            void readVariables(std::wstring_view section, Type* owner, Access, bool isStatic);
            void readRoutine(Type* owner, Access, bool isClass);
            void readProperty(Type& owner, Access);
            void readImplementation();
            // Whether the routine word here opens a routine's header, rather than typing a field,
            // a parameter or a procedural type.
            [[nodiscard]] bool opensHeader() const;
            void readRoutineBody();
            // The hint of the comment lines standing between two lines.
            [[nodiscard]] std::wstring hintBetween(std::size_t afterLine,
                std::size_t beforeLine) const;
            [[nodiscard]] Function* declarationOf(const Names& qualified,
                std::wstring_view parameters);
            void skipAttributes();
            void skipHintDirectives();
            void skipToSemicolon();
            void skipVariantPart();
            void skipBalanced(std::wstring_view open, std::wstring_view close);
            // From the current token to the first at bracket depth zero that is a semicolon, an
            // end, the mark or a word of the list; answers where it stopped.
            [[nodiscard]] std::size_t endOfType(Words stops, std::wstring_view stopMark, Angles);
            [[nodiscard]] std::wstring textBetween(std::size_t first, std::size_t last) const;
            [[nodiscard]] std::wstring typeBetween(std::size_t first, std::size_t last) const;
            [[nodiscard]] Type newType(const Token& name, const Type* outer, Access) const;
        private:
            Surface& m_surface;
            std::size_t m_file;
            const UnitText& m_text;
            std::wstring m_category;
            std::vector<std::size_t> m_code;   // the code tokens, comments and directives left out
            std::size_t m_at{ 0 };
            std::wstring m_nameSpace;
            std::optional<Module> m_module;
            std::optional<std::size_t> m_attributeLine;   // where what follows has attributes from
            std::size_t m_typesBegin{ 0 };       // the first of the surface's types the unit adds
            std::size_t m_functionsBegin{ 0 };   // the first of the surface's functions it adds
        };

        UnitReader::UnitReader(Surface& surface, const std::size_t file, const UnitText& text,
            std::wstring category)
            :
            m_surface{ surface },
            m_file{ file },
            m_text{ text },
            m_category{ std::move(category) },
            m_typesBegin{ surface.types().size() },
            m_functionsBegin{ surface.functions().size() }
        {
            const TokenList& tokens = m_text.tokens();
            for (std::size_t i = 0; i != tokens.size(); ++i)
            {
                const TokenType type = tokens[i].type;
                if (type != TokenType::Comment && type != TokenType::Directive)
                    m_code.push_back(i);
            }
        }

        void UnitReader::read()
        {
            if (!readHeader())
                return;
            readInterface();
            if (isWord(peek(), L"implementation"))
                readImplementation();
            m_surface.addModule(std::move(*m_module));
        }

        const Token* UnitReader::peek(const std::size_t ahead) const
        {
            if (m_at + ahead >= m_code.size())
                return nullptr;
            return &code(m_at + ahead);
        }

        std::wstring_view UnitReader::text(const Token& token) const
        {
            return m_text.text(token);
        }

        bool UnitReader::isWord(const Token* token, const std::wstring_view word) const
        {
            return token && token->type == TokenType::Name && sameWord(text(*token), word);
        }

        bool UnitReader::isMark(const Token* token, const std::wstring_view mark) const
        {
            return token && token->type == TokenType::Punctuator && text(*token) == mark;
        }

        bool UnitReader::isListedWord(const Token* token, const Words words) const
        {
            return token && token->type == TokenType::Name && isListed(words, text(*token));
        }

        bool UnitReader::isDeclaredName(const Token* token) const
        {
            return token && token->type == TokenType::Name
                && !isListed(k_reservedWords, text(*token));
        }

        bool UnitReader::opensRoutine(const std::size_t ahead) const
        {
            const Token* next = peek(ahead + 1);
            return isListedWord(peek(ahead), k_routineWords) && !isMark(next, L":")
                && !isMark(next, L",");
        }

        bool UnitReader::opensFields() const
        {
            return isDeclaredName(peek()) && (isMark(peek(1), L",") || isMark(peek(1), L":"));
        }

        const Token& UnitReader::code(const std::size_t index) const
        {
            return m_text.tokens()[m_code[index]];
        }

        Place UnitReader::placeOf(const Token& token) const
        {
            return { m_file, token.line + 1 };
        }

        // The comment above comes before the one on the line, as a limited read takes them; the
        // attributes a declaration is written under stand between it and the comment above. A
        // hint that only names the declaration is a label - { TName } - which tells nothing.
        Comment UnitReader::commentAt(std::size_t line, const std::wstring_view name)
        {
            if (m_attributeLine)
                line = std::min(line, *m_attributeLine);
            m_attributeLine.reset();
            Comment comment;
            if (const std::optional<std::wstring_view> trailing = m_text.trailingComment(line))
                comment.trailing = withoutSeparators(commentWords(*trailing));
            gatherAbove(line, comment);
            if (comment.above.empty() && comment.trailing)
                comment.text = pascalBrief(trailingLines(line, *comment.trailing));
            else
                comment.text = pascalBrief(comment.above);
            if (sameWord(comment.text, name))
                comment.text.clear();
            return comment;
        }

        // The lines carrying on the trailing comment of the line above them are that line's, not
        // the comment of the declaration under them.
        void UnitReader::gatherAbove(const std::size_t line, Comment& comment) const
        {
            std::size_t first = line;
            while (first != 0 && m_text.kindOf(first - 1) == LineKind::CommentOnly)
                --first;
            if (first != 0 && first != line)
                first = std::min(line, first + m_text.continuationsOf(first - 1));
            if (first == line)
                return;
            comment.aboveLine = first + 1;
            for (std::size_t i = first; i != line; ++i)
                comment.above.push_back(withoutSeparators(commentWords(m_text.line(i))));
        }

        Names UnitReader::trailingLines(const std::size_t line, const std::wstring& trailing) const
        {
            Names lines{ trailing };
            const std::size_t continued = m_text.continuationsOf(line);
            for (std::size_t i = line + 1; i <= line + continued; ++i)
                lines.push_back(withoutSeparators(commentWords(m_text.line(i))));
            return lines;
        }

        // unit A.B.C platform; interface - the unit's name is its module's, and with :: for its
        // dots, the namespace its declarations are named in. A name of the unit is any word, a
        // directive's included - Acme.Platform.Utils - and what follows it to the semicolon is
        // what the unit says of itself.
        bool UnitReader::readHeader()
        {
            if (!isWord(peek(), L"unit"))
                return false;
            advance();
            std::wstring name;
            while (peek() && peek()->type == TokenType::Name)
            {
                name += text(*peek());
                advance();
                if (!isMark(peek(), L"."))
                    break;
                name += L'.';
                advance();
            }
            skipToSemicolon();
            if (name.empty() || !isWord(peek(), L"interface"))
                return false;
            advance();
            for (const wchar_t current : name)
            {
                if (current == L'.')
                    m_nameSpace += k_scope;
                else
                    m_nameSpace += current;
            }
            m_module = Module{
                .name = std::move(name),
                .file = m_file,
                .isInterface = true,
                .language = SourceLanguage::Pascal
            };
            return true;
        }

        void UnitReader::readInterface()
        {
            while (const Token* token = peek())
            {
                const bool ends = isWord(token, L"implementation")
                    || isWord(token, L"initialization") || isWord(token, L"begin")
                    || isWord(token, L"end");
                if (ends)
                    return;
                if (isWord(token, L"uses"))
                {
                    readUses();
                }
                else if (isWord(token, L"type"))
                {
                    advance();
                    readTypeSection(nullptr, Access::Public);
                }
                else if (isWord(token, L"const") || isWord(token, L"resourcestring"))
                {
                    readConstants(lowered(text(*token)), nullptr, Access::Public);
                }
                else if (isWord(token, L"var") || isWord(token, L"threadvar"))
                {
                    readVariables(lowered(text(*token)), nullptr, Access::Public, false);
                }
                else if (opensRoutine(0))
                {
                    readRoutine(nullptr, Access::Public, false);
                }
                else if (isMark(token, L"["))
                {
                    skipAttributes();
                }
                else
                {
                    skipToSemicolon();
                }
            }
        }

        // uses A, B in 'b.pas', C;
        void UnitReader::readUses()
        {
            advance();
            std::wstring name;
            while (const Token* token = peek())
            {
                const bool ends = isMark(token, L";");
                if (ends || isMark(token, L","))
                {
                    if (!name.empty())
                        m_module->imports.push_back({ .name = name, .exported = false });
                    name.clear();
                    advance();
                    if (ends)
                        return;
                    continue;
                }
                if (isWord(token, L"in"))
                {
                    advance();
                    if (peek() && peek()->type == TokenType::Literal)
                        advance();
                    continue;
                }
                if (token->type == TokenType::Name)
                    name += text(*token);
                else if (isMark(token, L"."))
                    name += L'.';
                advance();
            }
        }

        // Name = ...; Name<T> = ...; until a word that declares no type.
        void UnitReader::readTypeSection(const Type* outer, const Access access)
        {
            while (const Token* token = peek())
            {
                if (isMark(token, L"["))
                {
                    skipAttributes();
                    continue;
                }
                const bool declares = isDeclaredName(token)
                    && (isMark(peek(1), L"=") || isMark(peek(1), L"<"));
                if (!declares)
                    return;
                readTypeDeclaration(outer, access);
            }
        }

        // A class, a record, an interface, an enumeration or anything else, which is an alias of
        // what follows the equals sign. A forward declaration - class; interface; - is none.
        void UnitReader::readTypeDeclaration(const Type* outer, const Access access)
        {
            const Token& nameToken = *peek();
            Comment comment = commentAt(nameToken.line, text(nameToken));
            advance();
            Type type = newType(nameToken, outer, access);
            type.comment = std::move(comment);
            if (isMark(peek(), L"<"))
            {
                const std::size_t open = m_at;
                skipBalanced(L"<", L">");
                type.templateParameters = textBetween(open + 1, m_at - 2);
            }
            if (!isMark(peek(), L"="))
            {
                skipToSemicolon();
                return;
            }
            advance();
            const std::size_t valueStart = m_at;
            if (isWord(peek(), L"type"))
                advance();
            if (isWord(peek(), L"packed"))
                advance();

            const Token* kind = peek();
            const bool forward = isMark(peek(1), L";");
            if (isWord(kind, L"class") && !isWord(peek(1), L"of"))
            {
                if (forward)
                {
                    skipToSemicolon();
                    return;
                }
                type.kind = TypeKind::Class;
                readStructure(type);
                return;
            }
            if (isWord(kind, L"record") || isWord(kind, L"object"))
            {
                type.kind = isWord(kind, L"record") ? TypeKind::Struct : TypeKind::Class;
                readStructure(type);
                return;
            }
            if (isWord(kind, L"interface") || isWord(kind, L"dispinterface"))
            {
                if (forward)
                {
                    skipToSemicolon();
                    return;
                }
                type.kind = TypeKind::Interface;
                readStructure(type);
                return;
            }
            if (isMark(kind, L"("))
            {
                type.kind = TypeKind::Enum;
                readEnum(type);
                return;
            }

            // An alias: what follows the equals sign to the semicolon, a method type's calling
            // convention after it.
            const std::size_t end = endOfType(k_hintDirectives, L";", Angles::Nest);
            if (end > valueStart)
                type.target = typeBetween(valueStart, end - 1);
            skipToSemicolon();
            while (isListedWord(peek(), k_callingConventions) && isMark(peek(1), L";"))
            {
                type.target += L"; " + std::wstring{ text(*peek()) };
                advance();
                advance();
            }
            m_surface.addType(std::move(type));
        }

        // class abstract(TBase, IFoo) ... end; record ... end; interface(IBase) ['{...}'] ... end;
        // class(TBase); - a class with nothing of its own - and a helper's helper for X.
        void UnitReader::readStructure(Type& type)
        {
            advance();
            while (isWord(peek(), L"abstract") || isWord(peek(), L"sealed"))
                advance();
            if (isWord(peek(), L"helper"))
            {
                advance();
                if (isMark(peek(), L"("))
                    skipBalanced(L"(", L")");
                if (isWord(peek(), L"for") && peek(1) && peek(1)->type == TokenType::Name)
                {
                    advance();
                    const std::size_t first = m_at;
                    advance();
                    while (isMark(peek(), L".") && peek(1) && peek(1)->type == TokenType::Name)
                    {
                        advance();
                        advance();
                    }
                    type.target = L"helper for " + typeBetween(first, m_at - 1);
                }
            }
            if (isMark(peek(), L"("))
            {
                advance();
                std::size_t first = m_at;
                int depth = 0;
                while (const Token* token = peek())
                {
                    if (isMark(token, L"<"))
                        ++depth;
                    if (isMark(token, L">"))
                        --depth;
                    const bool closes = isMark(token, L")") && depth <= 0;
                    if ((isMark(token, L",") && depth <= 0) || closes)
                    {
                        if (m_at > first)
                            type.bases.push_back(typeBetween(first, m_at - 1));
                        advance();
                        first = m_at;
                        if (closes)
                            break;
                        continue;
                    }
                    advance();
                }
            }
            if (isMark(peek(), L"["))
                skipBalanced(L"[", L"]");
            if (isMark(peek(), L";"))
            {
                advance();
                m_surface.addType(std::move(type));
                return;
            }
            readMembers(type);
            skipHintDirectives();
            if (isMark(peek(), L";"))
                advance();
            m_surface.addType(std::move(type));
        }

        // The members up to the structure's end, each under the access its section gives it. A
        // word that opens none of them - a directive the reader does not know - is passed over to
        // its semicolon, so every turn of the walk moves it on.
        void UnitReader::readMembers(Type& type)
        {
            Access access = Access::Public;
            while (const Token* token = peek())
            {
                if (isWord(token, L"end"))
                {
                    advance();
                    return;
                }
                if (isWord(token, L"strict"))
                {
                    advance();
                    continue;
                }
                const bool labels = isWord(token, L"private") || isWord(token, L"protected")
                    || isWord(token, L"public") || isWord(token, L"published")
                    || isWord(token, L"automated");
                if (labels && !isMark(peek(1), L":") && !isMark(peek(1), L","))
                {
                    if (isWord(token, L"private"))
                        access = Access::Private;
                    else if (isWord(token, L"protected"))
                        access = Access::Protected;
                    else if (isWord(token, L"published"))
                        access = Access::Published;
                    else
                        access = Access::Public;
                    advance();
                    continue;
                }
                if (isMark(token, L"["))
                {
                    skipAttributes();
                    continue;
                }
                if (isWord(token, L"type"))
                {
                    advance();
                    readTypeSection(&type, access);
                    continue;
                }
                if (isWord(token, L"const"))
                {
                    readConstants(L"const", &type, access);
                    continue;
                }
                if (isWord(token, L"var"))
                {
                    readVariables(L"var", &type, access, false);
                    continue;
                }
                if (isWord(token, L"class"))
                {
                    if (isWord(peek(1), L"var"))
                    {
                        advance();
                        readVariables(L"var", &type, access, true);
                    }
                    else if (isWord(peek(1), L"property"))
                    {
                        advance();
                        readProperty(type, access);
                    }
                    else if (opensRoutine(1))
                    {
                        readRoutine(&type, access, true);
                    }
                    else
                    {
                        skipToSemicolon();
                    }
                    continue;
                }
                if (opensRoutine(0))
                {
                    readRoutine(&type, access, false);
                    continue;
                }
                if (isWord(token, L"property"))
                {
                    readProperty(type, access);
                    continue;
                }
                if (isWord(token, L"case"))
                {
                    skipVariantPart();
                    return;
                }
                if (opensFields())
                {
                    readVariables({}, &type, access, false);
                    continue;
                }
                skipToSemicolon();
            }
        }

        // (A, B = 2, C) - a member written first on its line carries the comment its line has.
        void UnitReader::readEnum(Type& type)
        {
            advance();
            std::size_t lastLine = type.place.line - 1;
            while (const Token* token = peek())
            {
                if (isMark(token, L")"))
                {
                    advance();
                    break;
                }
                if (token->type != TokenType::Name)
                {
                    advance();
                    continue;
                }
                EnumMember member;
                member.name = std::wstring{ text(*token) };
                member.place = placeOf(*token);
                if (token->line != lastLine)
                    member.comment = commentAt(token->line, member.name);
                lastLine = token->line;
                advance();
                if (isMark(peek(), L"="))
                {
                    advance();
                    const std::size_t first = m_at;
                    int depth = 0;
                    while (const Token* value = peek())
                    {
                        if (isMark(value, L"("))
                            ++depth;
                        if (isMark(value, L")"))
                            --depth;
                        if (depth < 0 || (depth == 0 && isMark(value, L",")))
                            break;
                        advance();
                    }
                    if (m_at > first)
                        member.value = textBetween(first, m_at - 1);
                }
                type.members.push_back(std::move(member));
                if (isMark(peek(), L","))
                    advance();
            }
            skipHintDirectives();
            if (isMark(peek(), L";"))
                advance();
            m_surface.addType(std::move(type));
        }

        // Name = value; Name: Type = value; - the unit's constants, or a type's. A constant of
        // the unit is a variable whose declaration is written as the unit spells it.
        void UnitReader::readConstants(const std::wstring_view section, Type* owner,
            const Access access)
        {
            advance();
            while (true)
            {
                if (isMark(peek(), L"["))
                    skipAttributes();
                const Token* nameToken = peek();
                const bool declares = isDeclaredName(nameToken)
                    && (isMark(peek(1), L"=") || isMark(peek(1), L":"));
                if (!declares)
                    return;
                Field field;
                field.name = std::wstring{ text(*nameToken) };
                field.place = placeOf(*nameToken);
                field.comment = commentAt(nameToken->line, field.name);
                field.isConstant = true;
                field.isStatic = owner != nullptr;
                field.access = access;
                const std::size_t nameAt = m_at;
                advance();
                if (isMark(peek(), L":"))
                {
                    advance();
                    const std::size_t first = m_at;
                    const std::size_t end = endOfType({}, L"=", Angles::Nest);
                    if (end > first)
                        field.type = typeBetween(first, end - 1);
                }
                if (isMark(peek(), L"="))
                {
                    advance();
                    const std::size_t first = m_at;
                    const std::size_t end = endOfType(k_hintDirectives, L";", Angles::Compare);
                    if (end > first)
                        field.value = textBetween(first, end - 1);
                }
                skipToSemicolon();
                if (owner)
                {
                    owner->fields.push_back(std::move(field));
                    continue;
                }
                Variable variable;
                variable.field = std::move(field);
                variable.nameSpace = m_nameSpace;
                variable.module = m_module->name;
                variable.declaration =
                    inSection(section, m_text.written(code(nameAt), code(m_at - 1)));
                variable.exported = true;
                m_surface.addVariable(std::move(variable));
            }
        }

        // A, B: Type; Name: Type = value; - the unit's variables, or a type's fields.
        void UnitReader::readVariables(const std::wstring_view section, Type* owner,
            const Access access, const bool isStatic)
        {
            if (!section.empty())
                advance();
            while (true)
            {
                if (isMark(peek(), L"["))
                    skipAttributes();
                if (!opensFields())
                    return;
                const std::size_t declarationAt = m_at;
                std::vector<const Token*> names;
                while (isDeclaredName(peek()))
                {
                    names.push_back(peek());
                    advance();
                    if (!isMark(peek(), L","))
                        break;
                    advance();
                }
                if (!isMark(peek(), L":"))
                {
                    skipToSemicolon();
                    continue;
                }
                advance();
                const std::size_t first = m_at;
                const std::size_t typeEnd = endOfType(k_variableStops, L"=", Angles::Nest);
                std::wstring type;
                if (typeEnd > first)
                    type = typeBetween(first, typeEnd - 1);
                std::wstring value;
                if (isMark(peek(), L"="))
                {
                    advance();
                    const std::size_t valueStart = m_at;
                    const std::size_t valueEnd = endOfType(k_hintDirectives, L";", Angles::Compare);
                    if (valueEnd > valueStart)
                        value = textBetween(valueStart, valueEnd - 1);
                }
                skipToSemicolon();
                std::wstring words;
                if (!owner)
                {
                    words = inSection(section,
                        m_text.written(code(declarationAt), code(m_at - 1)));
                }
                for (const Token* nameToken : names)
                {
                    Field field;
                    field.name = std::wstring{ text(*nameToken) };
                    field.type = type;
                    field.value = value;
                    field.isStatic = isStatic;
                    field.access = access;
                    field.place = placeOf(*nameToken);
                    field.comment = commentAt(nameToken->line, field.name);
                    if (owner)
                    {
                        owner->fields.push_back(std::move(field));
                        continue;
                    }
                    Variable variable;
                    variable.field = std::move(field);
                    variable.nameSpace = m_nameSpace;
                    variable.module = m_module->name;
                    variable.declaration = words;
                    variable.exported = true;
                    m_surface.addVariable(std::move(variable));
                }
            }
        }

        // [class] procedure Name<T>(params): Result; directive; ... - the signature is what is
        // written from its first word to the last directive's semicolon. A method resolution
        // clause - procedure IFoo.Bar = Baz; - declares nothing.
        void UnitReader::readRoutine(Type* owner, const Access access, const bool isClass)
        {
            const std::size_t startAt = m_at;
            const Token& start = *peek();
            if (isClass)
                advance();
            Function function;
            if (isWord(peek(), L"constructor"))
                function.kind = FunctionKind::Constructor;
            else if (isWord(peek(), L"destructor"))
                function.kind = FunctionKind::Destructor;
            advance();
            const Token* nameToken = peek();
            if (!nameToken || nameToken->type != TokenType::Name)
            {
                skipToSemicolon();
                return;
            }
            function.name = std::wstring{ text(*nameToken) };
            advance();
            if (isMark(peek(), L"."))
            {
                skipToSemicolon();
                return;
            }
            if (isMark(peek(), L"<"))
            {
                const std::size_t open = m_at;
                skipBalanced(L"<", L">");
                function.templateParameters = textBetween(open + 1, m_at - 2);
            }
            if (isMark(peek(), L"("))
            {
                const std::size_t open = m_at;
                skipBalanced(L"(", L")");
                if (m_at - open > 2)
                    function.parameters = textBetween(open + 1, m_at - 2);
            }
            if (isMark(peek(), L":"))
            {
                advance();
                const std::size_t first = m_at;
                const std::size_t end = endOfType({}, L";", Angles::Nest);
                if (end > first)
                    function.returnType = typeBetween(first, end - 1);
            }
            skipToSemicolon();
            std::size_t last = m_at - 1;
            while (isListedWord(peek(), k_routineDirectives) && !isMark(peek(1), L":")
                && !isMark(peek(1), L","))
            {
                const std::wstring_view word = text(*peek());
                const bool dispatches = sameWord(word, L"virtual") || sameWord(word, L"dynamic")
                    || sameWord(word, L"override") || sameWord(word, L"abstract");
                if (dispatches)
                    function.isVirtual = true;
                skipToSemicolon();
                last = m_at - 1;
            }
            function.signature = textBetween(startAt, last);
            function.isStatic = isClass;
            function.access = access;
            function.place = placeOf(start);
            function.comment = commentAt(start.line, function.name);
            if (owner)
            {
                owner->functions.push_back(std::move(function));
                return;
            }
            FreeFunction free;
            free.function = std::move(function);
            free.nameSpace = m_nameSpace;
            free.module = m_module->name;
            free.exported = true;
            m_surface.addFunction(std::move(free));
        }

        // property Name[Index: Integer]: Type read Get write Set default Value; default; - a
        // property written with no type declares again one a base has.
        void UnitReader::readProperty(Type& owner, const Access access)
        {
            const Token& start = *peek();
            advance();
            const Token* nameToken = peek();
            if (!nameToken || nameToken->type != TokenType::Name)
            {
                skipToSemicolon();
                return;
            }
            Property property;
            property.name = std::wstring{ text(*nameToken) };
            property.access = access;
            property.place = placeOf(start);
            property.comment = commentAt(start.line, property.name);
            advance();
            if (isMark(peek(), L"["))
                skipBalanced(L"[", L"]");
            if (isMark(peek(), L":"))
            {
                advance();
                const std::size_t first = m_at;
                const std::size_t end = endOfType(k_propertySpecifiers, L";", Angles::Nest);
                if (end > first)
                    property.type = typeBetween(first, end - 1);
            }
            while (const Token* token = peek())
            {
                if (isMark(token, L";") || isWord(token, L"end"))
                    break;
                if (!isListedWord(token, k_propertySpecifiers))
                {
                    advance();
                    continue;
                }
                const std::wstring_view word = text(*token);
                advance();
                const std::size_t first = m_at;
                const std::size_t end = endOfType(k_propertySpecifiers, L";", Angles::Compare);
                const std::wstring value = end > first
                    ? textBetween(first, end - 1)
                    : std::wstring{};
                if (sameWord(word, L"read"))
                    property.target = value;
                else if (sameWord(word, L"write"))
                    property.setter = value;
                else if (sameWord(word, L"default"))
                    property.defaultValue = value;
            }
            if (isMark(peek(), L";"))
                advance();
            if (isWord(peek(), L"default") && isMark(peek(1), L";"))
            {
                advance();
                advance();
            }
            owner.properties.push_back(std::move(property));
        }

        // A ROUTINE'S COMMENT MAY STAND UNDER ITS HEADER in the implementation section, between
        // the header and what the routine opens with - var, const, begin. It gives the hint of
        // the routine the interface declares, where the interface gives that one none.
        void UnitReader::readImplementation()
        {
            advance();
            while (const Token* token = peek())
            {
                if (isWord(token, L"initialization") || isWord(token, L"finalization"))
                    return;
                if (opensHeader())
                    readRoutineBody();
                else
                    advance();
            }
        }

        bool UnitReader::opensHeader() const
        {
            if (m_at == 0 || !isListedWord(peek(), k_routineWords) || !isDeclaredName(peek(1)))
                return false;
            const Token* before = &code(m_at - 1);
            const bool types = isMark(before, L":") || isMark(before, L"=")
                || isMark(before, L"(") || isMark(before, L",") || isWord(before, L"of")
                || isWord(before, L"to");
            return !types;
        }

        // procedure TOuter.TInner.Name<T>(Parameters): Result; directives; - a forward or an
        // external routine has no body, and what follows its header is not its own.
        void UnitReader::readRoutineBody()
        {
            advance();
            Names qualified;
            while (isDeclaredName(peek()))
            {
                qualified.emplace_back(text(*peek()));
                advance();
                if (isMark(peek(), L"<"))
                    skipBalanced(L"<", L">");
                if (!isMark(peek(), L"."))
                    break;
                advance();
            }
            std::wstring parameters;
            if (isMark(peek(), L"("))
            {
                const std::size_t open = m_at;
                skipBalanced(L"(", L")");
                if (m_at - open > 2)
                    parameters = textBetween(open + 1, m_at - 2);
            }
            skipToSemicolon();
            bool hasBody = true;
            while (isListedWord(peek(), k_routineDirectives) && !isMark(peek(1), L":")
                && !isMark(peek(1), L","))
            {
                if (isWord(peek(), L"forward") || isWord(peek(), L"external"))
                    hasBody = false;
                skipToSemicolon();
            }
            if (!hasBody || !peek())
                return;
            const std::wstring hint = hintBetween(code(m_at - 1).line, peek()->line);
            if (hint.empty())
                return;
            Function* function = declarationOf(qualified, parameters);
            if (function && function->comment.text.empty())
                function->comment.text = hint;
        }

        // A blank line or a directive between the comment lines parts their paragraphs.
        std::wstring UnitReader::hintBetween(const std::size_t afterLine,
            const std::size_t beforeLine) const
        {
            Names lines;
            for (std::size_t i = afterLine + 1; i < beforeLine; ++i)
            {
                if (m_text.kindOf(i) == LineKind::CommentOnly)
                    lines.push_back(withoutSeparators(commentWords(m_text.line(i))));
                else
                    lines.emplace_back();
            }
            return pascalBrief(lines);
        }

        // The routine of the interface a header of the implementation defines: by its name, the
        // types it is a member of, and where the name is overloaded, the parameters, all read in
        // any case.
        Function* UnitReader::declarationOf(const Names& qualified,
            const std::wstring_view parameters)
        {
            if (qualified.empty())
                return nullptr;
            const std::wstring_view name = qualified.back();
            std::vector<Function*> candidates;
            if (qualified.size() == 1)
            {
                FreeFunctions& functions = m_surface.functions();
                for (std::size_t i = m_functionsBegin; i < functions.size(); ++i)
                {
                    if (sameWord(functions[i].function.name, name))
                        candidates.push_back(&functions[i].function);
                }
            }
            else
            {
                std::wstring owner;
                for (std::size_t i = 0; i + 1 < qualified.size(); ++i)
                {
                    if (i != 0)
                        owner += k_scope;
                    owner += qualified[i];
                }
                Types& types = m_surface.types();
                for (std::size_t i = m_typesBegin; i < types.size(); ++i)
                {
                    if (!sameWord(types[i].name, owner))
                        continue;
                    for (Function& function : types[i].functions)
                    {
                        if (sameWord(function.name, name))
                            candidates.push_back(&function);
                    }
                    break;
                }
            }
            if (candidates.size() == 1)
                return candidates.front();
            const Names shape = parameterShape(parameters);
            for (Function* candidate : candidates)
            {
                if (parameterShape(candidate->parameters) == shape)
                    return candidate;
            }
            return nullptr;
        }

        void UnitReader::skipAttributes()
        {
            if (!m_attributeLine)
                m_attributeLine = peek()->line;
            while (isMark(peek(), L"["))
                skipBalanced(L"[", L"]");
        }

        void UnitReader::skipHintDirectives()
        {
            while (isListedWord(peek(), k_hintDirectives))
            {
                advance();
                if (peek() && peek()->type == TokenType::Literal)
                    advance();
            }
        }

        // To past the next semicolon at bracket depth zero. A record written inside what is
        // skipped holds its semicolons to its end, and an end at depth zero is where the
        // structure around stops, so it is left where it stands.
        void UnitReader::skipToSemicolon()
        {
            int depth = 0;
            int records = 0;
            while (const Token* token = peek())
            {
                if (isMark(token, L"(") || isMark(token, L"["))
                {
                    ++depth;
                }
                else if (isMark(token, L")") || isMark(token, L"]"))
                {
                    --depth;
                }
                else if (isWord(token, L"record"))
                {
                    ++records;
                }
                else if (isWord(token, L"end"))
                {
                    if (records == 0)
                        return;
                    --records;
                }
                else if (depth <= 0 && records == 0 && isMark(token, L";"))
                {
                    advance();
                    return;
                }
                advance();
            }
        }

        // case Tag: Type of 1: (A: Integer); 2: (B: Double); end - to past the record's end.
        void UnitReader::skipVariantPart()
        {
            int depth = 0;
            int records = 0;
            while (const Token* token = peek())
            {
                advance();
                if (isMark(token, L"("))
                {
                    ++depth;
                }
                else if (isMark(token, L")"))
                {
                    --depth;
                }
                else if (isWord(token, L"record"))
                {
                    ++records;
                }
                else if (isWord(token, L"end"))
                {
                    if (records > 0)
                        --records;
                    else if (depth <= 0)
                        return;
                }
            }
        }

        void UnitReader::skipBalanced(const std::wstring_view open, const std::wstring_view close)
        {
            int depth = 0;
            while (const Token* token = peek())
            {
                if (isMark(token, open))
                {
                    ++depth;
                }
                else if (isMark(token, close))
                {
                    --depth;
                    if (depth <= 0)
                    {
                        advance();
                        return;
                    }
                }
                advance();
            }
        }

        std::size_t UnitReader::endOfType(const Words stops, const std::wstring_view stopMark,
            const Angles angles)
        {
            const bool nests = angles == Angles::Nest;
            int depth = 0;
            int records = 0;
            while (const Token* token = peek())
            {
                const bool opens = isMark(token, L"(") || isMark(token, L"[")
                    || (nests && isMark(token, L"<"));
                const bool closes = isMark(token, L")") || isMark(token, L"]")
                    || (nests && isMark(token, L">"));
                if (opens)
                {
                    ++depth;
                }
                else if (closes)
                {
                    --depth;
                }
                else if (isWord(token, L"record"))
                {
                    ++records;
                }
                else if (isWord(token, L"end") && records > 0)
                {
                    --records;
                }
                else if (depth <= 0 && records == 0)
                {
                    const bool stopped = isMark(token, L";") || isMark(token, stopMark)
                        || isWord(token, L"end") || isListedWord(token, stops);
                    if (stopped)
                        return m_at;
                }
                advance();
            }
            return m_at;
        }

        std::wstring UnitReader::textBetween(const std::size_t first, const std::size_t last) const
        {
            if (first > last || last >= m_code.size())
                return {};
            return m_text.slice(code(first), code(last));
        }

        // A type as a page spells it: a unit's dots in a qualified name written the C++ way,
        // System.Classes.TStrings as System::Classes::TStrings. A literal's dots and a range's
        // stay as they are.
        std::wstring UnitReader::typeBetween(const std::size_t first, const std::size_t last) const
        {
            const std::wstring spelled = textBetween(first, last);
            std::wstring result;
            bool quoted = false;
            for (std::size_t i = 0; i != spelled.size(); ++i)
            {
                const wchar_t current = spelled[i];
                if (current == L'\'')
                    quoted = !quoted;
                const bool joins = !quoted && current == L'.' && i > 0 && i + 1 < spelled.size()
                    && isNameChar(spelled[i - 1]) && isNameStart(spelled[i + 1]);
                if (joins)
                    result += k_scope;
                else
                    result += current;
            }
            return result;
        }

        Type UnitReader::newType(const Token& name, const Type* outer, const Access access) const
        {
            const std::wstring spelled{ text(name) };
            Type type;
            type.name = outer ? outer->name + std::wstring{ k_scope } + spelled : spelled;
            type.nameSpace = m_nameSpace;
            type.qualifiedName = m_nameSpace.empty()
                ? type.name
                : m_nameSpace + std::wstring{ k_scope } + type.name;
            type.kind = TypeKind::Alias;
            type.module = m_module->name;
            type.exported = true;
            type.access = outer ? access : Access::Public;
            type.category = m_category;
            type.place = placeOf(name);
            return type;
        }

        // Settling

        enum class MethodType
        {
            Yes,
            No,
            Unknown
        };

        // Whether a type, followed through the aliases the tree declares, is a method type:
        // procedure ... of object, function ... of object, or reference to.
        [[nodiscard]] MethodType methodTypeOf(const Surface& surface, std::wstring spelled,
            std::wstring nameSpace)
        {
            for (std::size_t depth = 0; depth != k_aliasDepth; ++depth)
            {
                const Type* type = surface.named(spelled, nameSpace);
                if (!type)
                    return MethodType::Unknown;
                if (type->kind != TypeKind::Alias)
                    return MethodType::No;
                const std::wstring target = lowered(type->target);
                if (target.starts_with(L"reference to"))
                    return MethodType::Yes;
                const bool routine = target.starts_with(L"procedure")
                    || target.starts_with(L"function");
                if (routine && target.find(L"of object") != std::wstring::npos)
                    return MethodType::Yes;
                if (routine)
                    return MethodType::No;
                spelled = type->target;
                nameSpace = type->nameSpace;
            }
            return MethodType::Unknown;
        }

        // A METHOD TYPE MAKES AN EVENT, as the Object Inspector has it. A type the tree does not
        // declare - the RTL's TNotifyEvent - says nothing either way, and there Delphi's own
        // naming decides: On and a capital. A type of the language's own is no method type.
        [[nodiscard]] bool isEvent(const Surface& surface, const Type& owner,
            const Property& property)
        {
            if (!property.type.empty())
            {
                const MethodType answer = methodTypeOf(surface, property.type, owner.nameSpace);
                if (answer != MethodType::Unknown)
                    return answer == MethodType::Yes;
                if (isListed(Syntax::Languages::pascal.types, plainType(property.type)))
                    return false;
            }
            const std::wstring_view name = property.name;
            return name.size() > 2 && name.starts_with(L"On") && std::iswupper(name[2]) != 0;
        }

        // What a property declared again takes from the one it declares again.
        struct Redeclared
        {
            std::wstring type;
            std::wstring defaultValue;
            std::wstring hint;
        };

        // The nearest declaration of the name with a type, up the bases the tree declares.
        [[nodiscard]] std::optional<Redeclared> redeclared(const Surface& surface, const Type& type,
            const std::wstring_view name)
        {
            std::vector<const Type*> pending;
            std::set<const Type*> seen;
            for (const std::wstring& base : type.bases)
            {
                if (const Type* found = surface.resolve(base, type.nameSpace))
                    pending.push_back(found);
            }
            for (std::size_t i = 0; i != pending.size(); ++i)
            {
                const Type* base = pending[i];
                if (!seen.insert(base).second)
                    continue;
                for (const Property& property : base->properties)
                {
                    if (!property.type.empty() && sameWord(property.name, name))
                    {
                        return Redeclared{
                            .type = property.type,
                            .defaultValue = property.defaultValue,
                            .hint = property.comment.text
                        };
                    }
                }
                for (const std::wstring& above : base->bases)
                {
                    if (const Type* found = surface.resolve(above, base->nameSpace))
                        pending.push_back(found);
                }
            }
            return std::nullopt;
        }
    }

    // Each pass gathers afresh, by what the one before found, so a symbol gathered from a
    // branch that a later pass leaves out is dropped; the passes end when one finds what the
    // one before did.
    PascalSymbols pascalSymbolsOf(const std::span<const std::wstring_view> texts)
    {
        std::vector<std::wstring_view> defining;
        for (const std::wstring_view text : texts)
        {
            if (mentionsDefine(text))
                defining.push_back(text);
        }
        PascalSymbols known;
        for (std::size_t pass = 0; pass != k_symbolPasses; ++pass)
        {
            PascalSymbols seen = known;
            PascalSymbols found;
            for (const std::wstring_view text : defining)
                gatherFrom(text, seen, found);
            if (found == known)
                break;
            known = std::move(found);
        }
        return known;
    }

    void readPascalUnit(Surface& surface, const std::size_t file, const std::wstring_view text,
        const std::wstring& category, const PascalSymbols& symbols)
    {
        const UnitText unit{ text, symbols };
        UnitReader reader{ surface, file, unit, category };
        reader.read();
    }

    // The redeclarations are settled first, so a property declared again is an event where the
    // one it declares again is.
    void settlePascal(Surface& surface)
    {
        Types& types = surface.types();
        for (Type& type : types)
        {
            if (surface.languageOf(type.module) != SourceLanguage::Pascal)
                continue;
            for (Property& property : type.properties)
            {
                if (!property.type.empty())
                    continue;
                const std::optional<Redeclared> found = redeclared(surface, type, property.name);
                if (!found)
                    continue;
                property.type = found->type;
                if (property.defaultValue.empty())
                    property.defaultValue = found->defaultValue;
                if (property.comment.text.empty())
                    property.comment.text = found->hint;
            }
        }
        for (Type& type : types)
        {
            if (surface.languageOf(type.module) != SourceLanguage::Pascal)
                continue;
            Properties kept;
            for (Property& property : type.properties)
            {
                if (!isEvent(surface, type, property))
                {
                    kept.push_back(std::move(property));
                    continue;
                }
                type.events.push_back({
                    .type = property.type,
                    .alias = property.name,
                    .method = property.target,
                    .access = property.access,
                    .place = property.place,
                    .comment = std::move(property.comment)
                });
            }
            type.properties = std::move(kept);
        }
    }
}
