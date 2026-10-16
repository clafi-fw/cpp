module SeeDocs_App.Scanner;

import SeeDocs_App.PascalReader;
import SeeDocs_App.Reading;
import SeeDocs_App.Surface;

import ClaFi.Documents.TextFile;

import ClaFi.Core.Syntax.Languages;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        constexpr std::wstring_view k_interfaceExtension = L".cppm";
        constexpr std::wstring_view k_implementationExtension = L".cpp";
        // What a limited read takes for a module interface and for a header, in lower case.
        constexpr auto k_moduleExtensions = std::to_array<std::wstring_view>({
            L".cppm", L".ixx", L".mpp", L".cxxm", L".ccm", L".c++m"
        });
        constexpr auto k_headerExtensions = std::to_array<std::wstring_view>({
            L".h", L".hh", L".hpp", L".hxx", L".h++", L".inl", L".ipp", L".tpp", L".tcc"
        });
        constexpr auto k_pascalExtensions = std::to_array<std::wstring_view>({
            L".pas", L".pp"
        });
        constexpr std::wstring_view k_pascalInclude = L".inc";   // read for its symbols only
        constexpr wchar_t k_hiddenPrefix = L'.';   // a folder so named is not read
        constexpr std::wstring_view k_scope = L"::";
        constexpr std::wstring_view k_lineComment = L"//";
        constexpr std::wstring_view k_blockComment = L"/*";
        constexpr std::wstring_view k_blockCommentEnd = L"*/";
        constexpr std::wstring_view k_brief = L"@brief ";
        constexpr std::wstring_view k_todo = L"TODO";
        constexpr std::wstring_view k_textMacro = L"DERIVED_TEXT_CLASS";
        constexpr std::wstring_view k_textBase = L"Text";
        constexpr std::wstring_view k_memberPrefix = L"m_";
        constexpr std::wstring_view k_ellipsis = L"...";   // ends the pack handed on to a base

        enum class TokenType
        {
            Name,
            Number,
            Literal,
            Punctuator,
            Comment,
            Directive,
            Attribute
        };

        // One run of a line: where it is, and what it is. Its text is read off the line.
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

        [[nodiscard]] std::wstring_view rightTrimmed(std::wstring_view text)
        {
            while (!text.empty() && isBlank(text.back()))
                text.remove_suffix(1);
            return text;
        }

        // A stretch of a line, from its first character to past its last.
        struct Run
        {
            std::size_t start{ 0 };
            std::size_t end{ 0 };
        };

        using Runs = std::vector<Run>;

        // The string and character literals of a macro's body, which its expansion leaves alone.
        [[nodiscard]] Runs literalRuns(const std::wstring_view text)
        {
            Runs runs;
            std::size_t i = 0;
            while (i < text.size())
            {
                const wchar_t quote = text[i];
                if (quote != L'"' && quote != L'\'')
                {
                    ++i;
                    continue;
                }
                std::size_t end = i + 1;
                while (end < text.size() && text[end] != quote)
                    end += text[end] == L'\\' ? 2 : 1;
                end = std::min(end + 1, text.size());
                runs.push_back({ i, end });
                i = end;
            }
            return runs;
        }

        // A directive's text with its comments taken out: a // to the end, a /* */ where it stands.
        [[nodiscard]] std::wstring withoutComments(const std::wstring_view text)
        {
            std::wstring result;
            std::size_t i = 0;
            while (i < text.size())
            {
                const std::wstring_view rest = text.substr(i);
                if (rest.starts_with(k_lineComment))
                    break;
                if (rest.starts_with(k_blockComment))
                {
                    const std::size_t close = rest.find(k_blockCommentEnd, k_blockComment.size());
                    if (close == std::wstring_view::npos)
                        break;
                    result += L' ';
                    i += close + k_blockCommentEnd.size();
                    continue;
                }
                result += text[i];
                ++i;
            }
            return result;
        }

        // A directive's word and what follows it: if and its condition, define and its macro.
        struct Directive
        {
            std::wstring_view word;
            std::wstring_view rest;
        };

        [[nodiscard]] Directive directiveOf(const std::wstring_view text)
        {
            std::wstring_view rest = trimmed(text);
            if (rest.starts_with(L'#'))
                rest = trimmed(rest.substr(1));
            std::size_t end = 0;
            while (end < rest.size() && isNameChar(rest[end]))
                ++end;
            return { rest.substr(0, end), trimmed(rest.substr(end)) };
        }

        // The macro a define or an undef names, or nothing.
        [[nodiscard]] std::wstring_view macroName(const std::wstring_view rest)
        {
            std::size_t end = 0;
            while (end < rest.size() && isNameChar(rest[end]))
                ++end;
            if (end == 0 || !isNameStart(rest.front()))
                return {};
            return rest.substr(0, end);
        }

        // A macro the tree defines.
        struct Macro
        {
            std::wstring body;             // comments taken out, its lines joined
            Runs literals;                 // in the body
            bool takesArguments{ false };
        };

        using MacroTable = std::unordered_map<std::wstring, Macro>;
        using ActiveMacros = std::vector<std::wstring_view>;

        // An expanded line, and where each of its characters stands in the line as written.
        struct Expansion
        {
            std::wstring text;
            Runs origins;   // a character's own place, or the whole name of the macro that made it
            void append(std::wstring_view piece, std::size_t at, std::optional<Run> origin);
        };

        // A piece found at a place in the text, or made by the macro whose name is the origin.
        void Expansion::append(const std::wstring_view piece, const std::size_t at,
            const std::optional<Run> origin)
        {
            text += piece;
            for (std::size_t i = 0; i != piece.size(); ++i)
                origins.push_back(origin.value_or(Run{ at + i, at + i + 1 }));
        }

        // **The tree's own macros**, expanded where their names stand in a limited read's code. An
        // object-like macro is replaced by its body, expanded in turn; a function-like one is
        // dropped with its arguments, since what it makes of them takes a preprocessor to know.
        // The first definition met in a branch that is read stands, and a name defined only in
        // branches that are not read takes the first of those. A file's definitions join the
        // tree's when the file ends, less those it undefines again: a macro a file takes back is
        // its own, like a word a header renames around one include. The compilers' own spellings
        // of an attribute and a pragma - __declspec(...), __attribute__((...)), _Pragma(...),
        // __pragma(...) - are dropped the same way.
        class Macros
        {
        public:
            Macros();
            void define(std::wstring_view directive, bool read);
            void undefine(std::wstring_view directive, bool read);
            void endFile();   // the file's definitions join the tree's
            // The line with its code expanded, the kept runs - its comments and literals - left
            // as they stand. A dropped call's arguments may run on into the lines after it, and
            // the depth carries that from one line to the next.
            [[nodiscard]] Expansion expanded(std::wstring_view line, std::span<const Run> kept,
                int& dropDepth) const;
        private:
            [[nodiscard]] const Macro* find(std::wstring_view name) const;
            void expandInto(Expansion& result, std::wstring_view text, std::span<const Run> kept,
                int& dropDepth, ActiveMacros& active, std::optional<Run> origin) const;
        private:
            MacroTable m_read;
            MacroTable m_unread;
            MacroTable m_fileRead;     // of the file being gathered, until it ends
            MacroTable m_fileUnread;   // of the file being gathered, until it ends
        };

        Macros::Macros()
        {
            for (const std::wstring_view name : { L"__declspec", L"__attribute__", L"_Pragma",
                L"__pragma" })
            {
                m_read.emplace(std::wstring{ name }, Macro{ .takesArguments = true });
            }
        }

        void Macros::define(const std::wstring_view directive, const bool read)
        {
            const std::wstring_view rest = directiveOf(directive).rest;
            const std::wstring_view name = macroName(rest);
            if (name.empty())
                return;

            Macro macro;
            std::wstring_view body = rest.substr(name.size());
            if (body.starts_with(L'('))
            {
                macro.takesArguments = true;
                const std::size_t close = body.find(L')');
                body = close == std::wstring_view::npos
                    ? std::wstring_view{}
                    : body.substr(close + 1);
            }
            macro.body = trimmed(body);
            macro.literals = literalRuns(macro.body);
            MacroTable& table = read ? m_fileRead : m_fileUnread;
            table.try_emplace(std::wstring{ name }, std::move(macro));
        }

        // An undef in a branch that is read takes back the name's definitions in every branch.
        void Macros::undefine(const std::wstring_view directive, const bool read)
        {
            const std::wstring name{ macroName(directiveOf(directive).rest) };
            m_fileUnread.erase(name);
            if (read)
                m_fileRead.erase(name);
        }

        // A name the tree holds keeps its definition: merge moves only the names it lacks.
        void Macros::endFile()
        {
            m_read.merge(m_fileRead);
            m_unread.merge(m_fileUnread);
            m_fileRead.clear();
            m_fileUnread.clear();
        }

        Expansion Macros::expanded(const std::wstring_view line, const std::span<const Run> kept,
            int& dropDepth) const
        {
            Expansion result;
            ActiveMacros active;
            expandInto(result, line, kept, dropDepth, active, std::nullopt);
            return result;
        }

        const Macro* Macros::find(const std::wstring_view name) const
        {
            const std::wstring key{ name };
            if (const auto read = m_read.find(key); read != m_read.end())
                return &read->second;
            if (const auto unread = m_unread.find(key); unread != m_unread.end())
                return &unread->second;
            return nullptr;
        }

        // A name being expanded is not expanded again inside itself, as C++ has it.
        void Macros::expandInto(Expansion& result, const std::wstring_view text,
            const std::span<const Run> kept, int& dropDepth, ActiveMacros& active,
            const std::optional<Run> origin) const
        {
            std::size_t keptAt = 0;
            std::size_t i = 0;
            while (i < text.size())
            {
                while (keptAt < kept.size() && kept[keptAt].end <= i)
                    ++keptAt;
                if (keptAt < kept.size() && kept[keptAt].start <= i)
                {
                    if (dropDepth == 0)
                        result.append(text.substr(i, kept[keptAt].end - i), i, origin);
                    i = kept[keptAt].end;
                    continue;
                }
                const wchar_t current = text[i];
                if (dropDepth > 0)
                {
                    if (current == L'(')
                        ++dropDepth;
                    else if (current == L')')
                        --dropDepth;
                    ++i;
                    continue;
                }
                if (!isNameChar(current))
                {
                    result.append(text.substr(i, 1), i, origin);
                    ++i;
                    continue;
                }

                std::size_t end = i + 1;
                while (end < text.size() && isNameChar(text[end]))
                    ++end;
                const std::wstring_view word = text.substr(i, end - i);
                const Macro* macro = isNameStart(current) ? find(word) : nullptr;
                if (!macro || std::ranges::find(active, word) != active.end())
                {
                    result.append(word, i, origin);
                    i = end;
                    continue;
                }
                if (macro->takesArguments)
                {
                    std::size_t open = end;
                    while (open < text.size() && isBlank(text[open]))
                        ++open;
                    if (open == text.size() || text[open] != L'(')
                    {
                        result.append(word, i, origin);
                        i = end;
                        continue;
                    }
                    dropDepth = 1;
                    i = open + 1;
                    continue;
                }
                const Run written = origin.value_or(Run{ i, end });
                active.push_back(word);
                int bodyDepth = 0;
                result.append(L" ", i, written);
                expandInto(result, macro->body, macro->literals, bodyDepth, active, written);
                result.append(L" ", i, written);
                active.pop_back();
                i = end;
            }
        }

        // An include in a branch read: its line, and what it names, as spelled between the brackets
        // or the quotes.
        struct Include
        {
            std::size_t line{ 0 };
            std::wstring name;
        };

        using Includes = std::vector<Include>;
        using Lines = std::vector<std::wstring>;

        // How a file is lexed: as written, or as a limited read takes it - its conditionals read
        // down their first branch, and the tree's macros gathered from it or expanded in it.
        enum class Preprocess
        {
            None,
            Gather,
            Expand
        };

        // A file as lines and tokens, lexed line by line with the framework's C++ lexer and cut
        // once more where that lexer, built for colouring, leaves a plain name without a token.
        // Preprocessed, a line in a branch left out stands blank, and a line of code holds its
        // macros expanded; the lines keep their count, so every place is the file's own, and the
        // lines as written stay beside them. Which lines are comment lines is the hint rules':
        // // alone by strict ones, /* */ too by loose.
        class SourceText
        {
        public:
            SourceText(std::wstring_view text, HintRules, Preprocess = Preprocess::None,
                Macros* macros = nullptr);
            [[nodiscard]] const TokenList& tokens() const { return m_tokens; }
            [[nodiscard]] bool preprocessed() const { return m_preprocess != Preprocess::None; }
            // What the branches read include.
            [[nodiscard]] const Includes& includes() const { return m_includes; }
            [[nodiscard]] std::wstring_view text(const Token&) const;
            [[nodiscard]] std::size_t lineCount() const { return m_lines.size(); }
            [[nodiscard]] LineKind kindOf(const std::size_t line) const { return m_kinds[line]; }
            [[nodiscard]] std::wstring_view line(const std::size_t index) const
            {
                return m_lines[index];
            }
            // The line's width with its trailing blanks taken off.
            [[nodiscard]] std::size_t width(std::size_t line) const;
            // The comment ending a line of code, or nothing.
            [[nodiscard]] std::optional<std::wstring_view> trailingComment(std::size_t line) const;
            // How many comment-only lines under a line carry its trailing comment on: those that
            // open in the column the trailing comment opens in.
            [[nodiscard]] std::size_t continuationsOf(std::size_t line) const;
            // The text from one token to another, both included, comments cut out and lines joined
            // with one blank.
            [[nodiscard]] std::wstring slice(const Token& from, const Token& to) const;
            // The text from one token to another, both included, as the file has it: its lines
            // and comments kept, each line after the first set back by the first one's indent.
            [[nodiscard]] std::wstring written(const Token& from, const Token& to) const;
        private:
            void tokenizeLine(std::size_t index, Syntax::LineState&, Syntax::StateStrings&,
                bool& directiveContinues);
            void preprocessLine(std::size_t index, Syntax::LineState&, Syntax::StateStrings&);
            void readDirective(std::size_t index);
            void expandLine(std::size_t index, const Syntax::LineState&,
                const Syntax::StateStrings&);
            void followBlockComment(std::size_t index);
            void takeGap(std::size_t line, std::size_t from, std::size_t to);
            void push(TokenType, std::size_t line, std::size_t start, std::size_t end);
            void classifyLine(std::size_t firstToken, bool insideComment);
            [[nodiscard]] const Token* trailingToken(std::size_t line) const;
            [[nodiscard]] std::wstring_view writtenLine(std::size_t index) const;
            // Where a character of a line as read stands in the line as written.
            [[nodiscard]] Run writtenRun(std::size_t line, std::size_t column) const;
        private:
            Lines m_lines;
            Lines m_writtenLines;          // preprocessed, the lines as the file has them
            std::vector<Runs> m_origins;   // preprocessed, those of each line its macros changed
            TokenList m_tokens;
            std::vector<LineKind> m_kinds;
            std::vector<std::size_t> m_firstTokens;   // per line, the index of its first token
            Preprocess m_preprocess{ Preprocess::None };
            bool m_blockComments{ false };   // a /* */ comment stands as a comment line
            Macros* m_macros{ nullptr };
            Branches m_branches;
            Includes m_includes;
            bool m_blockOpen{ false };            // the line ended inside a /* */ comment
            bool m_directiveContinues{ false };   // the line ended a directive with a backslash
            int m_dropDepth{ 0 };                 // brackets a dropped macro call still holds open
        };

        SourceText::SourceText(const std::wstring_view text, const HintRules rules,
            const Preprocess preprocess, Macros* macros)
            :
            m_preprocess{ preprocess },
            m_blockComments{ rules == HintRules::Loose },
            m_macros{ macros }
        {
            std::size_t start = 0;
            while (start <= text.size())
            {
                const std::size_t end = text.find(L'\n', start);
                if (end == std::wstring_view::npos)
                {
                    m_lines.emplace_back(text.substr(start));
                    break;
                }
                m_lines.emplace_back(text.substr(start, end - start));
                start = end + 1;
            }
            if (preprocessed())
            {
                m_writtenLines = m_lines;
                m_origins.resize(m_lines.size());
            }

            Syntax::LineState state{};
            Syntax::StateStrings strings;
            bool directiveContinues = false;
            for (std::size_t i = 0; i != m_lines.size(); ++i)
            {
                const std::size_t firstToken = m_tokens.size();
                m_firstTokens.push_back(firstToken);
                const bool insideComment = m_blockOpen;
                if (preprocessed())
                {
                    preprocessLine(i, state, strings);
                }
                else
                {
                    tokenizeLine(i, state, strings, directiveContinues);
                    followBlockComment(i);
                }
                classifyLine(firstToken, insideComment);
            }
            if (m_preprocess == Preprocess::Gather && m_macros)
                m_macros->endFile();
        }

        std::wstring_view SourceText::text(const Token& token) const
        {
            return std::wstring_view{ m_lines[token.line] }.substr(token.start, token.length);
        }

        std::size_t SourceText::width(const std::size_t line) const
        {
            return rightTrimmed(m_lines[line]).size();
        }

        std::optional<std::wstring_view> SourceText::trailingComment(const std::size_t line) const
        {
            if (const Token* tail = trailingToken(line))
                return text(*tail);
            return std::nullopt;
        }

        std::size_t SourceText::continuationsOf(const std::size_t line) const
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

        std::wstring SourceText::slice(const Token& from, const Token& to) const
        {
            std::wstring result;
            for (std::size_t line = from.line; line <= to.line; ++line)
            {
                const std::wstring_view whole = m_lines[line];
                const std::size_t start = line == from.line ? from.start : 0;
                const std::size_t end = line == to.line ? to.end() : whole.size();
                std::wstring piece;
                std::size_t at = start;
                // Only the comments standing inside the range are cut, and preprocessed, the
                // directives; the tokens of a line are in order, so the first past the range ends
                // the walk.
                for (std::size_t i = m_firstTokens[line]; i != m_tokens.size(); ++i)
                {
                    const Token& token = m_tokens[i];
                    if (token.line != line || token.start >= end)
                        break;
                    const bool cut = token.type == TokenType::Comment
                        || (preprocessed() && token.type == TokenType::Directive);
                    if (!cut || token.end() <= at)
                        continue;
                    piece += whole.substr(at, token.start - at);
                    at = std::min(token.end(), end);
                }
                piece += whole.substr(at, end - at);
                const std::wstring_view trimmedPiece = trimmed(piece);
                if (trimmedPiece.empty())
                    continue;
                if (!result.empty())
                    result += L' ';
                result += trimmedPiece;
            }
            // Blanks inside a line are folded as well, so a spelling reads the same however the
            // source aligned it.
            std::wstring folded;
            bool blank = false;
            for (const wchar_t current : result)
            {
                if (isBlank(current))
                {
                    blank = true;
                    continue;
                }
                if (blank && !folded.empty())
                    folded += L' ';
                blank = false;
                folded += current;
            }
            return folded;
        }

        std::wstring SourceText::written(const Token& from, const Token& to) const
        {
            const std::wstring_view first = writtenLine(from.line);
            std::size_t indent = 0;
            while (indent < first.size() && isBlank(first[indent]))
                ++indent;
            const std::size_t start = writtenRun(from.line, from.start).start;
            const std::size_t end = writtenRun(to.line, to.end() - 1).end;
            std::wstring result;
            for (std::size_t line = from.line; line <= to.line; ++line)
            {
                std::wstring_view piece = writtenLine(line);
                if (line == to.line)
                    piece = piece.substr(0, end);
                if (line == from.line)
                    piece.remove_prefix(start);
                else
                {
                    std::size_t blanks = 0;
                    while (blanks < indent && blanks < piece.size() && isBlank(piece[blanks]))
                        ++blanks;
                    piece.remove_prefix(blanks);
                    result += L'\n';
                }
                result += rightTrimmed(piece);
            }
            return result;
        }

        void SourceText::tokenizeLine(const std::size_t index, Syntax::LineState& state,
            Syntax::StateStrings& strings, bool& directiveContinues)
        {
            const std::wstring_view line = m_lines[index];
            const std::wstring_view meaningful = rightTrimmed(line);
            const bool continues = !meaningful.empty() && meaningful.back() == L'\\';
            if (directiveContinues)
            {
                push(TokenType::Directive, index, 0, line.size());
                directiveContinues = continues;
                return;
            }

            Syntax::Tokens lexed;
            state = Syntax::lexLine(Syntax::Languages::cpp, line, state, strings, &lexed);
            std::size_t cursor = 0;
            for (const Syntax::Token& token : lexed)
            {
                const std::size_t start = token.range.start;
                const std::size_t end = token.range.start + token.range.length;
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
                        if (last && last->type == TokenType::Literal && last->line == index
                            && last->end() == start)
                        {
                            last->length = end - last->start;
                        }
                        else
                        {
                            push(TokenType::Literal, index, start, end);
                        }
                        break;
                    }
                    case Syntax::TokenKind::Directive:
                        // The lexer colours the directive's word alone; the line is the directive.
                        push(TokenType::Directive, index, start, line.size());
                        directiveContinues = continues;
                        return;
                    case Syntax::TokenKind::Attribute:
                        push(TokenType::Attribute, index, start, end);
                        break;
                    case Syntax::TokenKind::Number:
                        push(TokenType::Number, index, start, end);
                        break;
                    case Syntax::TokenKind::Operator:
                    case Syntax::TokenKind::Punctuation:
                        takeGap(index, start, end);
                        break;
                    default:
                        push(TokenType::Name, index, start, end);
                        break;
                }
            }
            takeGap(index, cursor, line.size());
        }

        // A directive is a token covering its lines, as the plain lexing has it; a line in a
        // branch left out is blanked before it is lexed, so nothing of it reaches the reader.
        void SourceText::preprocessLine(const std::size_t index, Syntax::LineState& state,
            Syntax::StateStrings& strings)
        {
            std::wstring& line = m_lines[index];
            const std::wstring_view ending = rightTrimmed(line);
            const bool continues = !ending.empty() && ending.back() == L'\\';
            if (m_directiveContinues)
            {
                push(TokenType::Directive, index, 0, line.size());
                m_directiveContinues = continues;
                return;
            }
            if (!m_blockOpen && trimmed(line).starts_with(L'#'))
            {
                readDirective(index);
                push(TokenType::Directive, index, line.find_first_not_of(L" \t"), line.size());
                m_directiveContinues = continues;
                return;
            }
            if (!m_branches.reading())
            {
                line.clear();
                return;
            }
            if (m_preprocess == Preprocess::Expand && m_macros)
                expandLine(index, state, strings);
            bool directiveContinues = false;
            tokenizeLine(index, state, strings, directiveContinues);
            followBlockComment(index);
        }

        // The directive's lines joined, its comments out: a conditional steers the branches, a
        // define or undef is gathered, and an include in a branch read is one of the includes.
        void SourceText::readDirective(const std::size_t index)
        {
            std::wstring joined;
            for (std::size_t i = index; i != m_lines.size(); ++i)
            {
                const std::wstring_view piece = rightTrimmed(m_lines[i]);
                if (!piece.ends_with(L'\\'))
                {
                    joined += piece;
                    break;
                }
                joined += piece.substr(0, piece.size() - 1);
                joined += L' ';
            }
            const std::wstring clean = withoutComments(joined);
            const Directive directive = directiveOf(clean);
            if (directive.word == L"define")
            {
                if (m_preprocess == Preprocess::Gather && m_macros)
                    m_macros->define(clean, m_branches.reading());
                return;
            }
            if (directive.word == L"undef")
            {
                if (m_preprocess == Preprocess::Gather && m_macros)
                    m_macros->undefine(clean, m_branches.reading());
                return;
            }
            if (directive.word == L"include")
            {
                const std::wstring_view named = directive.rest;
                const bool quoted = named.starts_with(L'"') || named.starts_with(L'<');
                const std::size_t close = named.find_first_of(L"\">", 1);
                if (m_branches.reading() && quoted && close != std::wstring_view::npos)
                {
                    m_includes.push_back({
                        .line = index,
                        .name = std::wstring{ named.substr(1, close - 1) }
                    });
                }
                return;
            }
            m_branches.take(directive.word, directive.rest);
        }

        // The line is lexed once to find what expansion leaves alone - comments and literals -
        // from a copy of the state, since the line is lexed again as it stands afterwards.
        void SourceText::expandLine(const std::size_t index, const Syntax::LineState& state,
            const Syntax::StateStrings& strings)
        {
            std::wstring& line = m_lines[index];
            Syntax::StateStrings scratch = strings;
            Syntax::Tokens lexed;
            std::ignore = Syntax::lexLine(Syntax::Languages::cpp, line, state, scratch, &lexed);
            Runs kept;
            for (const Syntax::Token& token : lexed)
            {
                const std::size_t start = token.range.start;
                const std::size_t end = token.range.start + token.range.length;
                switch (token.kind)
                {
                    case Syntax::TokenKind::Comment:
                    case Syntax::TokenKind::String:
                    case Syntax::TokenKind::Escape:
                        kept.push_back({ start, end });
                        break;
                    case Syntax::TokenKind::Directive:
                        kept.push_back({ start, line.size() });
                        break;
                    default:
                        break;
                }
            }
            Expansion expansion = m_macros->expanded(line, kept, m_dropDepth);
            if (expansion.text == line)
                return;
            line = std::move(expansion.text);
            m_origins[index] = std::move(expansion.origins);
        }

        // Whether the line ends inside a /* */ comment: a line read from inside one opens with
        // the comment's own run.
        void SourceText::followBlockComment(const std::size_t index)
        {
            bool open = m_blockOpen;
            for (std::size_t i = m_firstTokens[index]; i != m_tokens.size(); ++i)
            {
                const Token& token = m_tokens[i];
                if (token.type != TokenType::Comment)
                    continue;
                const std::wstring_view comment = text(token);
                const bool continued = open && i == m_firstTokens[index];
                if (!continued && !comment.starts_with(k_blockComment))
                    continue;
                const std::size_t shortest = continued
                    ? k_blockCommentEnd.size()
                    : k_blockComment.size() + k_blockCommentEnd.size();
                open = comment.size() < shortest || !comment.ends_with(k_blockCommentEnd);
            }
            m_blockOpen = open;
        }

        // The characters between two tokens of the lexer's: names, numbers and single characters,
        // with :: kept whole since it joins the names on either side of it.
        void SourceText::takeGap(const std::size_t line, std::size_t from, const std::size_t to)
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
                if (isNameStart(current))
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
                    while (end < to && (isNameChar(text[end]) || text[end] == L'.'))
                        ++end;
                    push(TokenType::Number, line, from, end);
                    from = end;
                    continue;
                }
                if (current == L':' && from + 1 < to && text[from + 1] == L':')
                {
                    push(TokenType::Punctuator, line, from, from + 2);
                    from += 2;
                    continue;
                }
                push(TokenType::Punctuator, line, from, from + 1);
                ++from;
            }
        }

        void SourceText::push(const TokenType type, const std::size_t line, const std::size_t start,
            const std::size_t end)
        {
            if (end > start)
                m_tokens.push_back({ type, line, start, end - start });
        }

        // By loose rules a /* */ comment is a comment line as // is, and so is a blank line inside
        // one, so a doc comment's paragraphs gather as one comment.
        void SourceText::classifyLine(const std::size_t firstToken, const bool insideComment)
        {
            if (firstToken == m_tokens.size())
            {
                const bool commentLine = m_blockComments && insideComment;
                m_kinds.push_back(commentLine ? LineKind::CommentOnly : LineKind::Blank);
                return;
            }
            bool commentsOnly = true;
            for (std::size_t i = firstToken; i != m_tokens.size(); ++i)
            {
                if (m_tokens[i].type != TokenType::Comment)
                    commentsOnly = false;
            }
            const bool lineComment = text(m_tokens[firstToken]).starts_with(k_lineComment);
            const bool counts = lineComment || m_blockComments;
            m_kinds.push_back(commentsOnly && counts ? LineKind::CommentOnly : LineKind::Code);
        }

        const Token* SourceText::trailingToken(const std::size_t line) const
        {
            if (m_kinds[line] != LineKind::Code)
                return nullptr;
            const std::size_t first = m_firstTokens[line];
            std::size_t last = first;
            while (last != m_tokens.size() && m_tokens[last].line == line)
                ++last;
            if (last == first)
                return nullptr;
            const Token& tail = m_tokens[last - 1];
            if (tail.type != TokenType::Comment || last - 1 == first)
                return nullptr;
            const std::wstring_view comment = text(tail);
            const bool block = m_blockComments && comment.starts_with(k_blockComment);
            if (!comment.starts_with(k_lineComment) && !block)
                return nullptr;
            return &tail;
        }

        std::wstring_view SourceText::writtenLine(const std::size_t index) const
        {
            return m_writtenLines.empty() ? m_lines[index] : m_writtenLines[index];
        }

        Run SourceText::writtenRun(const std::size_t line, const std::size_t column) const
        {
            if (m_origins.empty() || m_origins[line].empty())
                return { column, column + 1 };
            return m_origins[line][column];
        }

        // The words the reader takes as declaration keywords, and the macros of the routine.
        namespace Words
        {
            constexpr std::wstring_view exported = L"export";
            constexpr std::wstring_view module = L"module";
            constexpr std::wstring_view import = L"import";
            constexpr std::wstring_view nameSpace = L"namespace";
            constexpr std::wstring_view templateWord = L"template";
            constexpr std::wstring_view requiresWord = L"requires";
            constexpr std::wstring_view friendWord = L"friend";
            constexpr std::wstring_view typedefWord = L"typedef";
            constexpr std::wstring_view staticAssert = L"static_assert";
            constexpr std::wstring_view usingWord = L"using";
            constexpr std::wstring_view typenameWord = L"typename";
            constexpr std::wstring_view conceptWord = L"concept";
            constexpr std::wstring_view classWord = L"class";
            constexpr std::wstring_view structWord = L"struct";
            constexpr std::wstring_view unionWord = L"union";
            constexpr std::wstring_view enumWord = L"enum";
            constexpr std::wstring_view publicWord = L"public";
            constexpr std::wstring_view protectedWord = L"protected";
            constexpr std::wstring_view privateWord = L"private";
            constexpr std::wstring_view virtualWord = L"virtual";
            constexpr std::wstring_view finalWord = L"final";
            constexpr std::wstring_view overrideWord = L"override";
            constexpr std::wstring_view operatorWord = L"operator";
            constexpr std::wstring_view alignasWord = L"alignas";
            constexpr std::wstring_view deleteWord = L"delete";
            constexpr std::wstring_view staticWord = L"static";
            constexpr std::wstring_view constexprWord = L"constexpr";
            constexpr std::wstring_view constevalWord = L"consteval";
            constexpr std::wstring_view constinitWord = L"constinit";
            constexpr std::wstring_view inlineWord = L"inline";
            constexpr std::wstring_view explicitWord = L"explicit";
            constexpr std::wstring_view externWord = L"extern";
            constexpr std::wstring_view mutableWord = L"mutable";
            constexpr std::wstring_view threadLocal = L"thread_local";
            constexpr std::wstring_view constWord = L"const";
            constexpr std::wstring_view props = L"Props";
            constexpr std::wstring_view get = L"get";
            constexpr std::wstring_view find = L"find";
            constexpr std::wstring_view declareEvent = L"DECLARE_EVENT";
        }

        // A value handed to a base that is of no property type.
        constexpr auto k_valueWords = std::to_array<std::wstring_view>({
            L"this", L"nullptr", L"true", L"false"
        });

        // The name a property takes from its type: the type's own spelling stripped of
        // qualifiers and what wraps it, so a BrowserTab* is the property BrowserTab.
        [[nodiscard]] std::wstring propertyNameOf(const std::wstring_view type)
        {
            std::wstring bare{ type };
            for (const std::wstring_view wrap : { L"const", L"*", L"&" })
            {
                std::size_t at = bare.find(wrap);
                while (at != std::wstring::npos)
                {
                    bare.erase(at, wrap.size());
                    at = bare.find(wrap);
                }
            }
            return std::wstring{ bareName(trimmed(bare)) };
        }

        // The names of a parameter list's parameters, as spelled between its brackets: each
        // one's last name, its default argument cut off; a parameter with no name gives none.
        [[nodiscard]] Names parameterNamesOf(const std::wstring_view parameters)
        {
            Names names;
            for (const std::wstring& parameter : splitArguments(parameters))
            {
                std::wstring_view spelled = parameter;
                const std::size_t equals = spelled.find(L'=');
                if (equals != std::wstring_view::npos)
                    spelled = spelled.substr(0, equals);
                spelled = trimmed(spelled);
                std::size_t nameStart = spelled.size();
                while (nameStart > 0 && isNameChar(spelled[nameStart - 1]))
                    --nameStart;
                if (nameStart < spelled.size())
                    names.emplace_back(spelled.substr(nameStart));
            }
            return names;
        }

        // A constructor's parameters, which what it hands a base is read against: the names an
        // argument is the caller's own by, and whether a pack carries the caller's values on.
        struct ConstructorParameters
        {
            Names names;
            bool packed{ false };
        };

        // A macro of the routine, and what it declares.
        struct MacroForm
        {
            std::wstring_view name;
            PropertyForm form;
        };

        constexpr auto k_propertyMacros = std::to_array<MacroForm>({
            { L"DECLARE_PROPERTY", PropertyForm::Declared },
            { L"DECLARE_WRITABLE_PROPERTY", PropertyForm::Writable },
            { L"DECLARE_REF_PROPERTY", PropertyForm::ByReference },
            { L"DECLARE_PROPERTY_STORAGE", PropertyForm::Storage },
            { L"BIND_PROPERTY_MEMBER", PropertyForm::BoundMember },
            { L"BIND_PROPERTY_CALL", PropertyForm::BoundCall },
            { L"BIND_PROPERTY_VALUE", PropertyForm::BoundValue },
            { L"BIND_PROPERTY_ACTION", PropertyForm::BoundAction },
            { L"READ_PROPERTY", PropertyForm::Read },
            { L"REQUIRE_PROPERTY", PropertyForm::Required }
        });

        // The binds a constructor writes, by the name each starts with.
        constexpr auto k_bindMacros = std::to_array<std::wstring_view>({
            L"INIT_PROPERTY", L"BIND_MEMBER", L"BIND_CALL", L"BIND_CALL_VALUE", L"BIND_ACTION"
        });

        constexpr auto k_specifiers = std::to_array<std::wstring_view>({
            Words::staticWord, Words::virtualWord, Words::inlineWord, Words::constexprWord,
            Words::constevalWord, Words::constinitWord, Words::explicitWord, Words::friendWord,
            Words::externWord, Words::mutableWord, Words::threadLocal
        });

        constexpr auto k_baseWords = std::to_array<std::wstring_view>({
            Words::publicWord, Words::protectedWord, Words::privateWord, Words::virtualWord
        });

        [[nodiscard]] std::optional<PropertyForm> propertyMacro(const std::wstring_view word)
        {
            for (const MacroForm& macro : k_propertyMacros)
            {
                if (macro.name == word)
                    return macro.form;
            }
            return std::nullopt;
        }

        [[nodiscard]] bool isListed(const std::span<const std::wstring_view> words,
            const std::wstring_view word)
        {
            return std::ranges::find(words, word) != words.end();
        }

        // The comment's words: the slashes and the blanks around them gone, @brief dropped.
        [[nodiscard]] std::wstring_view commentBody(std::wstring_view comment)
        {
            while (comment.starts_with(L'/'))
                comment.remove_prefix(1);
            comment = trimmed(comment);
            if (comment.starts_with(k_brief))
                comment = trimmed(comment.substr(k_brief.size()));
            return comment;
        }

        // A doc comment's words, whichever way it is written - ///, //!, /** */, /*! */, the *
        // leading a line inside a block, the < turning one to the member before it.
        [[nodiscard]] std::wstring_view docCommentBody(std::wstring_view comment)
        {
            comment = trimmed(comment);
            if (comment.ends_with(k_blockCommentEnd))
            {
                comment.remove_suffix(k_blockCommentEnd.size());
                while (comment.ends_with(L'*'))
                    comment.remove_suffix(1);
            }
            while (comment.starts_with(L'/') || comment.starts_with(L'*'))
                comment.remove_prefix(1);
            if (comment.starts_with(L'!'))
                comment.remove_prefix(1);
            if (comment.starts_with(L'<'))
                comment.remove_prefix(1);
            return trimmed(comment);
        }

        // Name(...) -> Name<...>: a function with no return type ahead of its name and one after.
        [[nodiscard]] bool isDeductionGuide(const Function& function)
        {
            return function.returnType.empty()
                && function.signature.find(L"->") != std::wstring::npos;
        }

        // What a declaration gathers ahead of its keyword: export, a template head, attributes.
        struct Prefix
        {
            bool exported{ false };
            std::wstring templateParameters;
            std::optional<std::size_t> templateLine;
            const Token* first{ nullptr };   // the first token the declaration's text starts at
        };

        // A namespace or a type being read, with what everything inside it inherits.
        struct Scope
        {
            enum class Kind
            {
                Namespace,
                Type
            };
            Kind kind{ Kind::Namespace };
            std::wstring name;             // the qualified namespace, or the qualified type
            bool exported{ false };        // a namespace: export namespace, or an export block
            bool anonymous{ false };
            std::unique_ptr<Type> type;    // a type: the one being filled
            Access access{ Access::Public };
        };

        // Reads one file's tokens into the surface. A limited read takes none of the routine's
        // macros; in a header it takes what stands at namespace scope for exported, the header
        // for a module, and a using-declaration there for a re-publication.
        class FileReader
        {
        public:
            FileReader(Surface&, std::size_t file, const SourceText&, std::wstring folder,
                ScanMode, HintRules, bool header);
            void read();
        private:
            [[nodiscard]] const Token* peek(std::size_t ahead = 0) const;
            [[nodiscard]] std::wstring_view text(const Token& token) const
            {
                return m_text.text(token);
            }
            [[nodiscard]] bool isPunctuator(const Token*, std::wstring_view) const;
            [[nodiscard]] bool isName(const Token*, std::wstring_view) const;
            void advance() { ++m_at; }
            [[nodiscard]] Place placeOf(const Token&) const;
            [[nodiscard]] std::wstring currentNamespace() const;
            [[nodiscard]] bool scopeExported() const;
            [[nodiscard]] Scope* typeScope();
            [[nodiscard]] Type* ownerNamed(std::wstring_view qualifier);

            void readDeclarations();
            void readModuleLine(bool exported);
            void readImport(bool exported);
            void readNamespace(bool exported, bool inlined);
            [[nodiscard]] std::wstring readNamespaceName();
            void readBlock(bool exported);
            void noteInclude(const Token& directive);
            void readTemplateHead(const Token& keyword, Prefix&);
            void skipRequiresClause();
            void readType(TypeKind, const Prefix&);
            void readEnum(const Prefix&);
            void readUsing(const Prefix&);
            void readUsingDeclaration(const Token& keyword);
            void readTypedef(const Prefix&);
            void readConcept(const Prefix&);
            void readMacro(const Token& name, Type* owner, Access);
            void readDeclarator(const Prefix&);
            void readFunction(const Prefix&, const Token& start, std::size_t nameAt,
                std::wstring name, std::size_t parenAt);
            void readVariable(const Prefix&, const Token& start, std::size_t nameAt,
                std::size_t end);
            // The parameters are a constructor's, whose initializer list is read for what it
            // hands the bases; null for any other body.
            void skipBody(Type* owner, Access, bool initializers, const ConstructorParameters*);
            // Where the current token opens an initializer naming a base of the owner, what it
            // hands the base goes to the owner's restatements, argument by argument. See README
            void readBaseInitializer(Type& owner, const ConstructorParameters&);
            // The restatement an argument spells, from its first token to its last: a construction,
            // a qualified name, or a bare constant. Nothing for the caller's own values, a literal
            // or an expression.
            [[nodiscard]] std::optional<Restatement> restatementOf(std::size_t first,
                std::size_t last, const Names& parameters) const;
            // The index past the bracket matching the one at the index - a round, square or curly
            // one - and past the angle matching the < there; the end where none matches.
            [[nodiscard]] std::size_t pastMatching(std::size_t open) const;
            [[nodiscard]] std::size_t pastAngles(std::size_t open) const;
            void skipBalanced(std::wstring_view open, std::wstring_view close);
            void skipAngles();
            // Whether the token at the index has the punctuator written right after it, or
            // right before it, with no blank between.
            [[nodiscard]] bool touches(std::size_t at, wchar_t next) const;
            [[nodiscard]] bool touchedBy(std::size_t at, wchar_t previous) const;
            void skipDeclaration();
            void skipPast(std::wstring_view punctuator);
            [[nodiscard]] std::wstring readQualifiedName();
            [[nodiscard]] std::wstring textBetween(std::size_t first, std::size_t last) const;
            [[nodiscard]] Comment commentAt(std::size_t line, std::optional<std::size_t> fallback,
                std::wstring_view name, bool aboveCounts = true) const;
            void gatherAbove(std::size_t line, Comment&) const;
            // A loose read's trailing comment on the line, with the lines that carry it on.
            [[nodiscard]] Names trailingLines(std::size_t line, const std::wstring& trailing) const;
        private:
            Surface& m_surface;
            std::size_t m_file;
            const SourceText& m_text;
            std::wstring m_folder;
            bool m_limited{ false };
            bool m_looseHints{ false };
            bool m_header{ false };
            std::size_t m_at{ 0 };
            std::vector<Scope> m_scopes;
            std::optional<Module> m_module;
            // The file's own constants by name, which type a bare one handed to a base.
            std::unordered_map<std::wstring, std::wstring> m_localTypes;
        };

        FileReader::FileReader(Surface& surface, const std::size_t file, const SourceText& text,
            std::wstring folder, const ScanMode mode, const HintRules rules, const bool header)
            :
            m_surface{ surface },
            m_file{ file },
            m_text{ text },
            m_folder{ std::move(folder) },
            m_limited{ mode == ScanMode::Limited },
            m_looseHints{ rules == HintRules::Loose },
            m_header{ header }
        {
        }

        void FileReader::read()
        {
            if (m_header)
            {
                Module module{
                    .name = m_surface.files()[m_file].relative,
                    .file = m_file,
                    .isInterface = true,
                    .isHeader = true
                };
                for (const Include& include : m_text.includes())
                    module.imports.push_back({ .name = include.name, .exported = false });
                m_module = std::move(module);
            }
            readDeclarations();
            // A stray closing brace ends a read early; whatever follows is read on its own.
            while (peek())
            {
                advance();
                readDeclarations();
            }
            if (m_module)
                m_surface.addModule(std::move(*m_module));
        }

        const Token* FileReader::peek(const std::size_t ahead) const
        {
            const TokenList& tokens = m_text.tokens();
            if (m_at + ahead >= tokens.size())
                return nullptr;
            return &tokens[m_at + ahead];
        }

        bool FileReader::isPunctuator(const Token* token, const std::wstring_view value) const
        {
            return token && token->type == TokenType::Punctuator && text(*token) == value;
        }

        bool FileReader::isName(const Token* token, const std::wstring_view value) const
        {
            return token && token->type == TokenType::Name && text(*token) == value;
        }

        Place FileReader::placeOf(const Token& token) const
        {
            return { m_file, token.line + 1 };
        }

        std::wstring FileReader::currentNamespace() const
        {
            for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); ++scope)
            {
                if (scope->kind == Scope::Kind::Namespace)
                    return scope->name;
            }
            return {};
        }

        // A header exports what it declares anywhere but in an anonymous namespace.
        bool FileReader::scopeExported() const
        {
            for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); ++scope)
            {
                if (scope->anonymous)
                    return false;
                if (scope->kind == Scope::Kind::Namespace && !m_header)
                    return scope->exported;
            }
            return m_header;
        }

        Scope* FileReader::typeScope()
        {
            if (m_scopes.empty() || m_scopes.back().kind != Scope::Kind::Type)
                return nullptr;
            return &m_scopes.back();
        }

        // The type a qualified definition belongs to - one already read, in this file or before.
        Type* FileReader::ownerNamed(const std::wstring_view qualifier)
        {
            const Type* found = m_surface.resolve(qualifier, currentNamespace());
            if (!found)
                return nullptr;
            return m_surface.typeNamed(found->qualifiedName);
        }

        void FileReader::readDeclarations()
        {
            Prefix prefix;
            while (const Token* token = peek())
            {
                switch (token->type)
                {
                    case TokenType::Comment:
                        advance();
                        continue;
                    case TokenType::Directive:
                        if (m_header)
                            noteInclude(*token);
                        advance();
                        continue;
                    case TokenType::Attribute:
                        if (!prefix.first)
                            prefix.first = token;
                        advance();
                        continue;
                    case TokenType::Literal:
                    case TokenType::Number:
                        advance();
                        continue;
                    case TokenType::Punctuator:
                        if (text(*token) == L"}")
                            return;
                        if (text(*token) == L"~" && peek(1) && peek(1)->type == TokenType::Name)
                        {
                            readDeclarator(prefix);
                            prefix = {};
                            continue;
                        }
                        if (text(*token) == L"{")
                            skipBalanced(L"{", L"}");
                        else
                            advance();
                        prefix = {};
                        continue;
                    case TokenType::Name:
                        break;
                }

                const std::wstring_view word = text(*token);
                if (word == Words::exported)
                {
                    advance();
                    if (isPunctuator(peek(), L"{"))
                        readBlock(true);
                    else if (isName(peek(), Words::module))
                        readModuleLine(true);
                    else if (isName(peek(), Words::import))
                        readImport(true);
                    else
                    {
                        prefix.exported = true;
                        continue;
                    }
                    prefix = {};
                    continue;
                }
                if (word == Words::module)
                    readModuleLine(false);
                else if (word == Words::import)
                    readImport(false);
                else if (word == Words::nameSpace)
                    readNamespace(prefix.exported, false);
                else if (word == Words::inlineWord && isName(peek(1), Words::nameSpace))
                {
                    advance();
                    readNamespace(prefix.exported, true);
                }
                else if (word == Words::externWord && peek(1)
                    && peek(1)->type == TokenType::Literal && isPunctuator(peek(2), L"{"))
                {
                    advance();
                    advance();
                    readBlock(prefix.exported || scopeExported());
                }
                else if (word == Words::templateWord)
                {
                    advance();
                    readTemplateHead(*token, prefix);
                    continue;
                }
                else if (word == Words::requiresWord)
                {
                    advance();
                    skipRequiresClause();
                    continue;
                }
                else if (word == Words::typedefWord && m_limited)
                    readTypedef(prefix);
                else if (word == Words::friendWord || word == Words::typedefWord
                    || word == Words::staticAssert)
                {
                    skipDeclaration();
                }
                else if (word == Words::usingWord)
                    readUsing(prefix);
                else if (word == Words::conceptWord)
                    readConcept(prefix);
                else if (word == Words::classWord)
                    readType(TypeKind::Class, prefix);
                else if (word == Words::structWord)
                    readType(TypeKind::Struct, prefix);
                else if (word == Words::unionWord)
                    readType(TypeKind::Union, prefix);
                else if (word == Words::enumWord)
                    readEnum(prefix);
                else if (typeScope() && isPunctuator(peek(1), L":")
                    && (word == Words::publicWord || word == Words::protectedWord
                        || word == Words::privateWord))
                {
                    Scope& scope = *typeScope();
                    if (word == Words::publicWord)
                        scope.access = Access::Public;
                    else if (word == Words::protectedWord)
                        scope.access = Access::Protected;
                    else
                        scope.access = Access::Private;
                    advance();
                    advance();
                }
                else if (!m_limited && (propertyMacro(word) || word == Words::declareEvent))
                {
                    Scope* scope = typeScope();
                    readMacro(*token, scope ? scope->type.get() : nullptr,
                        scope ? scope->access : Access::Public);
                }
                else
                    readDeclarator(prefix);
                prefix = {};
            }
        }

        // module X; export module X :Part; module; module :private;
        void FileReader::readModuleLine(const bool exported)
        {
            advance();
            std::wstring name;
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L";"))
                    break;
                if (token->type == TokenType::Name || token->type == TokenType::Punctuator)
                    name += text(*token);
                advance();
            }
            advance();
            if (name.empty() || name.starts_with(L':'))
                return;
            Module module;
            module.name = name;
            module.file = m_file;
            module.isInterface = exported;
            m_module = std::move(module);
        }

        void FileReader::readImport(const bool exported)
        {
            advance();
            std::wstring name;
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L";"))
                    break;
                if (token->type == TokenType::Name || token->type == TokenType::Punctuator)
                    name += text(*token);
                advance();
            }
            advance();
            if (!m_module)
            {
                Module module;
                module.file = m_file;
                m_module = std::move(module);
            }
            m_module->imports.push_back({ .name = name, .exported = exported });
        }

        // An inline namespace adds nothing to a name - the namespace enclosing it is where its
        // declarations are named from - whatever name it has, a dropped macro call's included.
        void FileReader::readNamespace(const bool exported, const bool inlined)
        {
            advance();
            Scope scope;
            scope.kind = Scope::Kind::Namespace;
            if (inlined)
            {
                while (peek() && !isPunctuator(peek(), L"{") && !isPunctuator(peek(), L";"))
                    advance();
                scope.name = currentNamespace();
                scope.exported = exported || scopeExported();
            }
            else if (isPunctuator(peek(), L"{"))
            {
                scope.name = currentNamespace();
                scope.anonymous = true;
            }
            else
            {
                const std::wstring name = readNamespaceName();
                if (isPunctuator(peek(), L"="))
                {
                    skipPast(L";");
                    return;
                }
                const std::wstring enclosing = currentNamespace();
                if (name.empty())
                    scope.name = enclosing;
                else if (enclosing.empty())
                    scope.name = name;
                else
                    scope.name = enclosing + std::wstring{ k_scope } + name;
                scope.exported = exported || scopeExported();
            }
            if (!isPunctuator(peek(), L"{"))
            {
                skipPast(L";");
                return;
            }
            advance();
            m_scopes.push_back(std::move(scope));
            readDeclarations();
            m_scopes.pop_back();
            if (isPunctuator(peek(), L"}"))
                advance();
        }

        // An include standing outside any type - see Surface::encloseIncludes.
        void FileReader::noteInclude(const Token& directive)
        {
            if (typeScope())
                return;
            const std::wstring nameSpace = currentNamespace();
            for (const Include& include : m_text.includes())
            {
                if (include.line != directive.line)
                    continue;
                m_surface.addEnclosedInclude({
                    .file = m_file,
                    .included = include.name,
                    .nameSpace = nameSpace
                });
            }
        }

        // a::b::c, where a component written inline - a::inline b - adds nothing.
        std::wstring FileReader::readNamespaceName()
        {
            std::wstring name;
            while (const Token* token = peek())
            {
                if (isName(token, Words::inlineWord))
                {
                    advance();
                    if (peek() && peek()->type == TokenType::Name)
                        advance();
                }
                else if (token->type == TokenType::Name)
                {
                    if (!name.empty())
                        name += k_scope;
                    name += text(*token);
                    advance();
                }
                else
                {
                    break;
                }
                if (!isPunctuator(peek(), L"::"))
                    break;
                advance();
            }
            return name;
        }

        // export { ... } and extern "C" { ... } - a namespace scope in all but name, adding nothing
        // to the names it holds.
        void FileReader::readBlock(const bool exported)
        {
            advance();
            Scope scope;
            scope.kind = Scope::Kind::Namespace;
            scope.name = currentNamespace();
            scope.exported = exported;
            m_scopes.push_back(std::move(scope));
            readDeclarations();
            m_scopes.pop_back();
            if (isPunctuator(peek(), L"}"))
                advance();
        }

        void FileReader::readTemplateHead(const Token& keyword, Prefix& prefix)
        {
            if (!isPunctuator(peek(), L"<"))
                return;
            const std::size_t open = m_at;
            skipAngles();
            if (m_at - open > 2)
                prefix.templateParameters = textBetween(open + 1, m_at - 2);
            prefix.templateLine = keyword.line;
        }

        // A requires clause between a template head and its declaration: a bracketed expression,
        // or constraints joined with && and ||, each a name with its arguments.
        void FileReader::skipRequiresClause()
        {
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L"("))
                {
                    skipBalanced(L"(", L")");
                }
                else if (isPunctuator(token, L"!"))
                {
                    advance();
                    continue;
                }
                else
                {
                    while (peek() && (peek()->type == TokenType::Name
                        || isPunctuator(peek(), L"::")))
                    {
                        advance();
                    }
                    if (isPunctuator(peek(), L"<"))
                        skipAngles();
                }
                const bool joinsAnd = isPunctuator(peek(), L"&") && isPunctuator(peek(1), L"&");
                const bool joinsOr = isPunctuator(peek(), L"|") && isPunctuator(peek(1), L"|");
                if (!joinsAnd && !joinsOr)
                    return;
                advance();
                advance();
            }
        }

        // class X : public Y { ... }; - and the forward declaration, the specialization, the
        // DERIVED_TEXT_CLASS body the TextEngine writes as a macro.
        void FileReader::readType(const TypeKind kind, const Prefix& prefix)
        {
            const Token& keyword = *peek();
            advance();
            while (peek() && peek()->type == TokenType::Attribute)
                advance();
            if (isName(peek(), Words::alignasWord))
            {
                advance();
                skipBalanced(L"(", L")");
            }
            std::wstring name = readQualifiedName();
            if (!name.empty() && isPunctuator(peek(), L"<"))
            {
                const std::size_t open = m_at;
                skipAngles();
                name += L"<" + textBetween(open + 1, m_at - 2) + L">";
            }
            // template<...> class Outer<...>::Inner { ... } defines a class declared in Outer.
            if (isPunctuator(peek(), L"::"))
            {
                skipPast(L";");
                return;
            }
            if (isName(peek(), Words::finalWord))
                advance();
            while (peek() && peek()->type == TokenType::Comment)
                advance();

            if (name.empty())
            {
                if (isPunctuator(peek(), L"{"))
                    skipBalanced(L"{", L"}");
                skipPast(L";");
                return;
            }
            if (isPunctuator(peek(), L";"))
            {
                advance();
                return;
            }

            Scope* outer = typeScope();
            Type type;
            type.name = outer ? outer->type->name + std::wstring{ k_scope } + name : name;
            type.nameSpace = currentNamespace();
            type.qualifiedName = type.nameSpace.empty()
                ? type.name
                : type.nameSpace + std::wstring{ k_scope } + type.name;
            type.kind = kind;
            type.module = m_module ? m_module->name : std::wstring{};
            type.templateParameters = prefix.templateParameters;
            type.exported = outer ? outer->type->exported : (prefix.exported || scopeExported());
            type.access = outer ? outer->access : Access::Public;
            type.folder = m_folder;
            type.place = placeOf(keyword);
            type.comment = commentAt(keyword.line, prefix.templateLine, name);

            bool bodyless = false;
            if (isPunctuator(peek(), L":"))
            {
                advance();
                const std::size_t first = m_at;
                std::size_t last = m_at;
                while (const Token* token = peek())
                {
                    if (isPunctuator(token, L"{") || isPunctuator(token, L";"))
                        break;
                    if (isName(token, k_textMacro))
                    {
                        type.bases.emplace_back(k_textBase);
                        advance();
                        bodyless = true;
                        break;
                    }
                    if (isPunctuator(token, L"<"))
                    {
                        skipAngles();
                        last = m_at - 1;
                        continue;
                    }
                    if (isPunctuator(token, L"("))
                    {
                        skipBalanced(L"(", L")");
                        last = m_at - 1;
                        continue;
                    }
                    last = m_at;
                    advance();
                }
                if (!bodyless && last >= first)
                {
                    for (std::wstring& base : splitArguments(textBetween(first, last)))
                    {
                        std::wstring_view spelled = base;
                        bool stripped = true;
                        while (stripped)
                        {
                            stripped = false;
                            for (const std::wstring_view word : k_baseWords)
                            {
                                if (spelled.starts_with(word) && spelled.size() > word.size()
                                    && isBlank(spelled[word.size()]))
                                {
                                    spelled = trimmed(spelled.substr(word.size()));
                                    stripped = true;
                                }
                            }
                        }
                        if (!spelled.empty())
                            type.bases.emplace_back(spelled);
                    }
                }
            }

            if (bodyless || !isPunctuator(peek(), L"{"))
            {
                if (isPunctuator(peek(), L";"))
                    advance();
                m_surface.addType(std::move(type));
                return;
            }

            advance();
            Scope scope;
            scope.kind = Scope::Kind::Type;
            scope.name = type.qualifiedName;
            scope.access = kind == TypeKind::Class ? Access::Private : Access::Public;
            scope.type = std::make_unique<Type>(std::move(type));
            m_scopes.push_back(std::move(scope));
            readDeclarations();
            std::unique_ptr<Type> read = std::move(m_scopes.back().type);
            m_scopes.pop_back();
            if (isPunctuator(peek(), L"}"))
                advance();
            // A declarator after the body - `} instance;` - belongs to no surface.
            skipPast(L";");
            m_surface.addType(std::move(*read));
        }

        void FileReader::readEnum(const Prefix& prefix)
        {
            const Token& keyword = *peek();
            advance();
            const bool scoped = isName(peek(), Words::classWord)
                || isName(peek(), Words::structWord);
            if (scoped)
                advance();
            const std::wstring name = readQualifiedName();
            std::wstring base;
            if (isPunctuator(peek(), L":"))
            {
                advance();
                const std::size_t first = m_at;
                while (peek() && !isPunctuator(peek(), L"{") && !isPunctuator(peek(), L";"))
                    advance();
                if (m_at > first)
                    base = textBetween(first, m_at - 1);
            }
            if (name.empty() || !isPunctuator(peek(), L"{"))
            {
                if (isPunctuator(peek(), L"{"))
                    skipBalanced(L"{", L"}");
                skipPast(L";");
                return;
            }

            Scope* outer = typeScope();
            Type type;
            type.name = outer ? outer->type->name + std::wstring{ k_scope } + name : name;
            type.nameSpace = currentNamespace();
            type.qualifiedName = type.nameSpace.empty()
                ? type.name
                : type.nameSpace + std::wstring{ k_scope } + type.name;
            type.kind = TypeKind::Enum;
            type.module = m_module ? m_module->name : std::wstring{};
            type.target = base;
            type.scoped = scoped;
            type.exported = outer ? outer->type->exported : (prefix.exported || scopeExported());
            type.access = outer ? outer->access : Access::Public;
            type.folder = m_folder;
            type.place = placeOf(keyword);
            type.comment = commentAt(keyword.line, prefix.templateLine, name);

            advance();
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L"}"))
                    break;
                if (token->type != TokenType::Name)
                {
                    advance();
                    continue;
                }
                EnumMember member;
                member.name = text(*token);
                member.place = placeOf(*token);
                // A member on the enum's own line has only the comment on that line; the one
                // above is the enum's.
                member.comment = commentAt(token->line, std::nullopt,
                    name + std::wstring{ k_scope } + member.name, token->line != keyword.line);
                advance();
                if (isPunctuator(peek(), L"="))
                {
                    advance();
                    const std::size_t first = m_at;
                    int depth = 0;
                    while (const Token* value = peek())
                    {
                        if (isPunctuator(value, L"(") || isPunctuator(value, L"{"))
                            ++depth;
                        if (isPunctuator(value, L")") || isPunctuator(value, L"}"))
                            --depth;
                        if (depth < 0 || (depth == 0 && isPunctuator(value, L",")))
                            break;
                        advance();
                    }
                    if (m_at > first)
                        member.value = textBetween(first, m_at - 1);
                }
                type.members.push_back(std::move(member));
                if (isPunctuator(peek(), L","))
                    advance();
            }
            if (isPunctuator(peek(), L"}"))
                advance();
            skipPast(L";");
            m_surface.addType(std::move(type));
        }

        // using X = ...; is an alias and is read; using Base::name; is a using-declaration and is
        // read inside a type; a using-directive is passed over.
        void FileReader::readUsing(const Prefix& prefix)
        {
            const Token& keyword = *peek();
            advance();
            if (isName(peek(), Words::nameSpace) || isName(peek(), Words::enumWord))
            {
                skipPast(L";");
                return;
            }
            const Token* nameToken = peek();
            if (!nameToken || nameToken->type != TokenType::Name || !isPunctuator(peek(1), L"="))
            {
                readUsingDeclaration(keyword);
                return;
            }
            advance();
            advance();
            const std::size_t first = m_at;
            int depth = 0;
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L"{") || isPunctuator(token, L"("))
                    ++depth;
                if (isPunctuator(token, L"}") || isPunctuator(token, L")"))
                    --depth;
                if (depth <= 0 && isPunctuator(token, L";"))
                    break;
                advance();
            }
            Scope* outer = typeScope();
            Type type;
            const std::wstring name = std::wstring{ text(*nameToken) };
            type.name = outer ? outer->type->name + std::wstring{ k_scope } + name : name;
            type.nameSpace = currentNamespace();
            type.qualifiedName = type.nameSpace.empty()
                ? type.name
                : type.nameSpace + std::wstring{ k_scope } + type.name;
            type.kind = TypeKind::Alias;
            type.module = m_module ? m_module->name : std::wstring{};
            type.templateParameters = prefix.templateParameters;
            if (m_at > first)
                type.target = textBetween(first, m_at - 1);
            type.exported = outer ? outer->type->exported : (prefix.exported || scopeExported());
            type.access = outer ? outer->access : Access::Public;
            type.folder = m_folder;
            type.place = placeOf(keyword);
            type.comment = commentAt(keyword.line, prefix.templateLine, name);
            if (isPunctuator(peek(), L";"))
                advance();
            m_surface.addType(std::move(type));
        }

        // Inside a type, using Base::Base takes the base's constructors and using Base::name
        // republishes a member under the label the line stands in. At a header's namespace scope
        // it re-publishes a declaration into the namespace - see Surface::republish - and at a
        // module's it is passed over.
        void FileReader::readUsingDeclaration(const Token& keyword)
        {
            const TokenList& tokens = m_text.tokens();
            Scope* scope = typeScope();
            std::size_t first = m_at;
            if (isName(peek(), Words::typenameWord))
                ++first;
            std::size_t last = m_at;
            while (peek() && !isPunctuator(peek(), L";"))
            {
                last = m_at;
                advance();
            }
            if (isPunctuator(peek(), L";"))
                advance();
            const bool qualified = last >= first + 2 && tokens[last].type == TokenType::Name
                && isPunctuator(&tokens[last - 1], L"::");
            if (!scope && qualified && m_header && scopeExported())
            {
                m_surface.addRepublication({
                    .file = m_file,
                    .nameSpace = currentNamespace(),
                    .named = textBetween(first, last)
                });
                return;
            }
            if (!scope || !qualified)
                return;
            Using declaration;
            declaration.base = textBetween(first, last - 2);
            declaration.name = std::wstring{ text(tokens[last]) };
            declaration.access = scope->access;
            declaration.place = placeOf(keyword);
            declaration.comment = commentAt(keyword.line, std::nullopt, declaration.name);
            scope->type->usings.push_back(std::move(declaration));
        }

        // typedef X Y; as the alias using Y = X; states. A typedef naming a function's type, an
        // array's or a type it defines on the spot is passed over.
        void FileReader::readTypedef(const Prefix& prefix)
        {
            const TokenList& tokens = m_text.tokens();
            const Token& keyword = *peek();
            advance();
            const std::size_t first = m_at;
            std::size_t last = m_at;
            bool plain = true;
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L";"))
                    break;
                if (isPunctuator(token, L"(") || isPunctuator(token, L"[")
                    || isPunctuator(token, L"{"))
                {
                    plain = false;
                    break;
                }
                last = m_at;
                advance();
            }
            if (!plain)
            {
                skipPast(L";");
                return;
            }
            if (isPunctuator(peek(), L";"))
                advance();
            if (last <= first || tokens[last].type != TokenType::Name)
                return;

            Scope* outer = typeScope();
            Type type;
            const std::wstring name{ text(tokens[last]) };
            type.name = outer ? outer->type->name + std::wstring{ k_scope } + name : name;
            type.nameSpace = currentNamespace();
            type.qualifiedName = type.nameSpace.empty()
                ? type.name
                : type.nameSpace + std::wstring{ k_scope } + type.name;
            type.kind = TypeKind::Alias;
            type.module = m_module ? m_module->name : std::wstring{};
            type.target = textBetween(first, last - 1);
            type.exported = outer ? outer->type->exported : (prefix.exported || scopeExported());
            type.access = outer ? outer->access : Access::Public;
            type.folder = m_folder;
            type.place = placeOf(keyword);
            type.comment = commentAt(keyword.line, prefix.templateLine, name);
            m_surface.addType(std::move(type));
        }

        void FileReader::readConcept(const Prefix& prefix)
        {
            const Token& keyword = *peek();
            advance();
            const Token* nameToken = peek();
            if (!nameToken || nameToken->type != TokenType::Name || !isPunctuator(peek(1), L"="))
            {
                skipPast(L";");
                return;
            }
            advance();
            advance();
            const std::size_t first = m_at;
            int depth = 0;
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L"{") || isPunctuator(token, L"("))
                    ++depth;
                if (isPunctuator(token, L"}") || isPunctuator(token, L")"))
                    --depth;
                if (depth <= 0 && isPunctuator(token, L";"))
                    break;
                advance();
            }
            Type type;
            type.name = std::wstring{ text(*nameToken) };
            type.nameSpace = currentNamespace();
            type.qualifiedName = type.nameSpace.empty()
                ? type.name
                : type.nameSpace + std::wstring{ k_scope } + type.name;
            type.kind = TypeKind::Concept;
            type.module = m_module ? m_module->name : std::wstring{};
            type.templateParameters = prefix.templateParameters;
            if (m_at > first)
                type.target = textBetween(first, m_at - 1);
            type.exported = prefix.exported || scopeExported();
            type.folder = m_folder;
            type.place = placeOf(keyword);
            type.comment = commentAt(keyword.line, prefix.templateLine, type.name);
            if (isPunctuator(peek(), L";"))
                advance();
            m_surface.addType(std::move(type));
        }

        // DECLARE_PROPERTY and its kind, BIND_PROPERTY_*, READ_PROPERTY and REQUIRE_PROPERTY,
        // DECLARE_EVENT.
        void FileReader::readMacro(const Token& name, Type* owner, const Access access)
        {
            const std::wstring_view word = text(name);
            advance();
            Names arguments;
            if (isPunctuator(peek(), L"("))
            {
                const std::size_t open = m_at;
                skipBalanced(L"(", L")");
                if (m_at - open > 2)
                    arguments = splitArguments(textBetween(open + 1, m_at - 2));
            }
            if (isPunctuator(peek(), L";"))
                advance();
            const Place place = placeOf(name);

            if (word == Words::declareEvent)
            {
                if (arguments.size() < 3)
                {
                    m_surface.addProblem({ true, place,
                        L"DECLARE_EVENT takes an event type, a handler alias and a method" });
                    return;
                }
                if (!owner)
                {
                    m_surface.addProblem({ true, place,
                        L"event " + arguments[0] + L" declared outside a class" });
                    return;
                }
                Event event;
                event.type = arguments[0];
                event.alias = arguments[1];
                event.method = arguments[2];
                event.access = access;
                event.place = place;
                event.comment = commentAt(name.line, std::nullopt, event.type);
                owner->events.push_back(std::move(event));
                return;
            }

            const std::optional<PropertyForm> form = propertyMacro(word);
            if (!form)
                return;
            Property property;
            property.form = *form;
            property.access = access;
            property.place = place;
            if (declaresStorage(*form))
            {
                const bool writable = *form == PropertyForm::Writable;
                const std::size_t wanted = writable ? 4 : 3;
                if (arguments.size() < wanted)
                {
                    m_surface.addProblem({ true, place, writable
                        ? L"property declaration takes a type, a name, a setter and a default"
                        : L"property declaration takes a type, a name and a default" });
                    return;
                }
                property.type = arguments[0];
                property.name = arguments[1];
                if (writable)
                    property.setter = arguments[2];
                for (std::size_t i = wanted - 1; i != arguments.size(); ++i)
                {
                    if (!property.defaultValue.empty())
                        property.defaultValue += L", ";
                    property.defaultValue += arguments[i];
                }
                property.target = std::wstring{ k_memberPrefix } + property.name;
            }
            else if (*form == PropertyForm::Read)
            {
                if (arguments.size() < 2)
                {
                    m_surface.addProblem({ true, place,
                        L"READ_PROPERTY takes a type and a default" });
                    return;
                }
                property.type = arguments[0];
                property.name = arguments[0];
                for (std::size_t i = 1; i != arguments.size(); ++i)
                {
                    if (!property.defaultValue.empty())
                        property.defaultValue += L", ";
                    property.defaultValue += arguments[i];
                }
                property.target = property.name;
            }
            else if (*form == PropertyForm::Required)
            {
                if (arguments.empty())
                {
                    m_surface.addProblem({ true, place, L"REQUIRE_PROPERTY takes a type" });
                    return;
                }
                property.type = arguments[0];
                property.name = propertyNameOf(arguments[0]);
                property.target = property.name;
            }
            else
            {
                if (arguments.empty())
                {
                    m_surface.addProblem({ true, place, L"a property binding takes a type first" });
                    return;
                }
                property.type = arguments[0];
                property.name = propertyNameOf(arguments[0]);
                if (arguments.size() > 1)
                {
                    std::wstring_view target = arguments[1];
                    target = trimmed(target.substr(0, target.find(L" = ")));
                    property.target = std::wstring{ target };
                }
            }
            if (!owner)
            {
                m_surface.addProblem({ true, place,
                    L"property " + property.name + L" declared outside a class" });
                return;
            }
            property.comment = commentAt(name.line, std::nullopt, property.name);
            owner->properties.push_back(std::move(property));
        }

        // Anything else: a function or a variable, declared or defined, member or free.
        void FileReader::readDeclarator(const Prefix& prefix)
        {
            const TokenList& tokens = m_text.tokens();
            const Token& start = prefix.first ? *prefix.first : *peek();
            std::size_t i = m_at;
            int depth = 0;
            int angle = 0;
            std::size_t nameAt = tokens.size();
            std::wstring name;
            std::size_t parenAt = tokens.size();
            const Token* previous = nullptr;
            while (i != tokens.size())
            {
                const Token& token = tokens[i];
                if (token.type == TokenType::Comment || token.type == TokenType::Directive)
                {
                    ++i;
                    continue;
                }
                if (token.type == TokenType::Name)
                {
                    const std::wstring_view word = text(token);
                    if (word == Words::operatorWord && depth == 0 && angle == 0)
                    {
                        // operator==, operator(), operator bool: the name runs to the parameters.
                        nameAt = i;
                        name = std::wstring{ word };
                        ++i;
                        if (i < tokens.size() && isPunctuator(&tokens[i], L"(")
                            && i + 1 < tokens.size() && isPunctuator(&tokens[i + 1], L")"))
                        {
                            name += L"()";
                            i += 2;
                        }
                        while (i != tokens.size() && !isPunctuator(&tokens[i], L"("))
                        {
                            if (tokens[i].type == TokenType::Name && !name.ends_with(L' ')
                                && isNameChar(name.back()))
                            {
                                name += L' ';
                            }
                            name += text(tokens[i]);
                            ++i;
                        }
                        if (i != tokens.size())
                            parenAt = i;
                        break;
                    }
                    if (depth == 0 && angle == 0)
                    {
                        nameAt = i;
                        name = previous && isPunctuator(previous, L"~")
                            ? L"~" + std::wstring{ word }
                            : std::wstring{ word };
                    }
                    previous = &token;
                    ++i;
                    continue;
                }
                if (token.type != TokenType::Punctuator)
                {
                    previous = &token;
                    ++i;
                    continue;
                }
                const std::wstring_view mark = text(token);
                if (mark == L"(" && depth == 0 && angle == 0)
                {
                    parenAt = i;
                    break;
                }
                if (mark == L"(" || mark == L"[")
                    ++depth;
                else if (mark == L")" || mark == L"]")
                    --depth;
                else if (mark == L"<" && previous && (previous->type == TokenType::Name
                    || isPunctuator(previous, L"::") || isPunctuator(previous, L">")))
                {
                    ++angle;
                }
                else if (mark == L">" && angle > 0)
                    --angle;
                else if (depth == 0 && angle == 0
                    && (mark == L";" || mark == L"{" || mark == L"=" || mark == L":"))
                {
                    break;
                }
                previous = &token;
                ++i;
            }

            if (parenAt != tokens.size() && nameAt != tokens.size())
            {
                readFunction(prefix, start, nameAt, std::move(name), parenAt);
                return;
            }
            if (nameAt == tokens.size())
            {
                m_at = i;
                skipPast(L";");
                return;
            }
            readVariable(prefix, start, nameAt, i);
        }

        void FileReader::readFunction(const Prefix& prefix, const Token& start,
            const std::size_t nameAt, std::wstring name, const std::size_t parenAt)
        {
            const TokenList& tokens = m_text.tokens();

            // A name qualified by a type is a definition of a member declared elsewhere.
            std::size_t qualifierStart = nameAt;
            while (qualifierStart >= m_at + 2 && isPunctuator(&tokens[qualifierStart - 1], L"::"))
            {
                std::size_t before = qualifierStart - 2;
                if (isPunctuator(&tokens[before], L">"))
                {
                    int angle = 0;
                    while (true)
                    {
                        if (isPunctuator(&tokens[before], L">"))
                            ++angle;
                        if (isPunctuator(&tokens[before], L"<"))
                            --angle;
                        if (angle == 0 || before == m_at)
                            break;
                        --before;
                    }
                    if (before > m_at)
                        --before;
                }
                if (tokens[before].type != TokenType::Name)
                    break;
                qualifierStart = before;
            }
            const bool qualified = qualifierStart != nameAt;
            const std::size_t qualifierEnd = qualified ? nameAt - 2 : nameAt;

            Function function;
            function.name = std::move(name);
            function.templateParameters = prefix.templateParameters;
            function.place = placeOf(start);

            // The words ahead of the return type.
            std::size_t typeStart = m_at;
            while (typeStart < nameAt)
            {
                const Token& token = tokens[typeStart];
                const bool specifier = token.type == TokenType::Name
                    && isListed(k_specifiers, text(token));
                if (token.type == TokenType::Attribute || specifier)
                {
                    if (isName(&token, Words::staticWord))
                        function.isStatic = true;
                    if (isName(&token, Words::virtualWord))
                        function.isVirtual = true;
                    ++typeStart;
                    continue;
                }
                break;
            }
            const std::size_t typeEnd = qualified ? qualifierStart : nameAt;
            const bool hasTilde = typeEnd > typeStart && isPunctuator(&tokens[typeEnd - 1], L"~");
            if (typeEnd > typeStart + (hasTilde ? 1 : 0))
                function.returnType = textBetween(typeStart, typeEnd - 1 - (hasTilde ? 1 : 0));

            // The parameters, then the tail up to the body, the initializer list or the end.
            m_at = parenAt;
            skipBalanced(L"(", L")");
            if (m_at - parenAt > 2)
                function.parameters = textBetween(parenAt + 1, m_at - 2);
            int depth = 0;
            std::size_t signatureEnd = m_at - 1;
            bool opensBody = false;
            while (const Token* token = peek())
            {
                if (token->type == TokenType::Comment)
                {
                    advance();
                    continue;
                }
                if (token->type == TokenType::Name)
                {
                    const std::wstring_view word = text(*token);
                    if (word == Words::overrideWord || word == Words::finalWord)
                        function.isVirtual = true;
                    if (word == Words::deleteWord)
                        function.isDeleted = true;
                    if (word == Words::requiresWord)
                    {
                        signatureEnd = m_at;
                        advance();
                        skipRequiresClause();
                        signatureEnd = m_at - 1;
                        continue;
                    }
                }
                if (isPunctuator(token, L"(") || isPunctuator(token, L"["))
                    ++depth;
                if (isPunctuator(token, L")") || isPunctuator(token, L"]"))
                    --depth;
                if (isPunctuator(token, L"<"))
                    ++depth;
                if (isPunctuator(token, L">"))
                    --depth;
                if (depth <= 0 && isPunctuator(token, L";"))
                {
                    advance();
                    break;
                }
                if (depth <= 0 && (isPunctuator(token, L"{") || isPunctuator(token, L":")))
                {
                    opensBody = true;
                    function.hasBody = true;
                    break;
                }
                signatureEnd = m_at;
                advance();
            }
            if (text(tokens[signatureEnd]) == L"0")
                function.isVirtual = true;
            function.signature = m_text.slice(start, tokens[signatureEnd]);

            Scope* scope = typeScope();
            Type* owner = nullptr;
            if (qualified)
                owner = ownerNamed(textBetween(qualifierStart, qualifierEnd));
            else if (scope)
                owner = scope->type.get();

            // A constructor's initializer list is read for what it hands the bases.
            const bool constructs = owner && !function.name.starts_with(L'~')
                && function.name == bareName(owner->name);
            ConstructorParameters parameters;
            if (constructs)
            {
                parameters.names = parameterNamesOf(function.parameters);
                parameters.packed = function.parameters.find(k_ellipsis) != std::wstring::npos;
            }

            if (!qualified)
            {
                if (scope)
                {
                    const std::wstring_view ownerBare = bareName(scope->type->name);
                    if (function.name.starts_with(L'~'))
                        function.kind = FunctionKind::Destructor;
                    else if (function.name == ownerBare)
                        function.kind = FunctionKind::Constructor;
                    function.access = scope->access;
                    function.comment = commentAt(start.line, prefix.templateLine, function.name);
                    scope->type->functions.push_back(std::move(function));
                }
                else if ((prefix.exported || scopeExported())
                    && !(m_limited && isDeductionGuide(function)))
                {
                    function.comment = commentAt(start.line, prefix.templateLine, function.name);
                    FreeFunction free;
                    free.nameSpace = currentNamespace();
                    free.module = m_module ? m_module->name : std::wstring{};
                    free.exported = true;
                    free.function = std::move(function);
                    m_surface.addFunction(std::move(free));
                }
            }
            if (opensBody)
            {
                skipBody(owner, scope ? scope->access : Access::Public, isPunctuator(peek(), L":"),
                    constructs ? &parameters : nullptr);
            }
        }

        void FileReader::readVariable(const Prefix& prefix, const Token& start,
            const std::size_t nameAt, const std::size_t end)
        {
            const TokenList& tokens = m_text.tokens();
            // A qualified name defines a variable declared elsewhere - a class's static member.
            if (nameAt > m_at && isPunctuator(&tokens[nameAt - 1], L"::"))
            {
                m_at = end;
                skipPast(L";");
                return;
            }
            Field field;
            field.name = std::wstring{ text(tokens[nameAt]) };
            field.place = placeOf(start);

            std::size_t typeStart = m_at;
            while (typeStart < nameAt)
            {
                const Token& token = tokens[typeStart];
                const bool specifier = token.type == TokenType::Name
                    && isListed(k_specifiers, text(token));
                if (token.type == TokenType::Attribute || specifier)
                {
                    if (isName(&token, Words::staticWord))
                        field.isStatic = true;
                    const bool constant = isName(&token, Words::constexprWord)
                        || isName(&token, Words::constinitWord);
                    if (constant)
                        field.isConstant = true;
                    ++typeStart;
                    continue;
                }
                break;
            }
            if (nameAt > typeStart)
                field.type = textBetween(typeStart, nameAt - 1);
            if (field.isStatic && field.type.starts_with(Words::constWord))
                field.isConstant = true;

            // The initializer: what follows = up to the end, or the braces themselves.
            m_at = end;
            if (isPunctuator(peek(), L"="))
            {
                advance();
                const std::size_t first = m_at;
                int depth = 0;
                while (const Token* token = peek())
                {
                    if (isPunctuator(token, L"(") || isPunctuator(token, L"{")
                        || isPunctuator(token, L"["))
                    {
                        ++depth;
                    }
                    if (isPunctuator(token, L")") || isPunctuator(token, L"}")
                        || isPunctuator(token, L"]"))
                    {
                        --depth;
                    }
                    if (depth <= 0 && isPunctuator(token, L";"))
                        break;
                    advance();
                }
                if (m_at > first)
                    field.value = textBetween(first, m_at - 1);
            }
            else if (isPunctuator(peek(), L"{"))
            {
                const std::size_t first = m_at;
                skipBalanced(L"{", L"}");
                field.value = textBetween(first, m_at - 1);
            }
            skipPast(L";");
            const Token& last = tokens[m_at - 1];

            Scope* scope = typeScope();
            if (!scope)
                m_localTypes[field.name] = std::wstring{ plainType(field.type) };
            if (scope)
            {
                field.access = scope->access;
                field.comment = commentAt(start.line, prefix.templateLine, field.name);
                scope->type->fields.push_back(std::move(field));
            }
            else if (prefix.exported || scopeExported())
            {
                field.comment = commentAt(start.line, prefix.templateLine, field.name);
                Variable variable;
                variable.nameSpace = currentNamespace();
                variable.module = m_module ? m_module->name : std::wstring{};
                variable.declaration = m_text.written(start, last);
                variable.exported = true;
                variable.field = std::move(field);
                m_surface.addVariable(std::move(variable));
            }
        }

        // A body, from the initializer list or the opening brace to the closing one, read for the
        // binds written in it: the property binds and reads go to the owner, the constructor's
        // initialisers and hand-written Props::get calls to the surface, and what a constructor
        // hands a base to the owner's restatements. In an initializer list a brace after a name is
        // an initializer; the body's brace follows a bracket or a brace.
        void FileReader::skipBody(Type* owner, const Access access, const bool initializers,
            const ConstructorParameters* parameters)
        {
            const TokenList& tokens = m_text.tokens();
            int depth = 0;
            bool inBody = !initializers;
            const Token* previous = nullptr;
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L"{"))
                {
                    if (!inBody && depth == 0)
                    {
                        const bool initializer = previous
                            && (previous->type == TokenType::Name || isPunctuator(previous, L">"));
                        if (!initializer)
                            inBody = true;
                    }
                    ++depth;
                }
                else if (isPunctuator(token, L"}"))
                {
                    --depth;
                    if (inBody && depth <= 0)
                    {
                        advance();
                        return;
                    }
                }
                else if (!inBody && depth == 0 && isPunctuator(token, L";"))
                {
                    advance();
                    return;
                }
                else if (token->type == TokenType::Name && !m_limited)
                {
                    const std::wstring_view word = text(*token);
                    if (propertyMacro(word))
                    {
                        readMacro(*token, owner, access);
                        previous = &tokens[m_at - 1];
                        continue;
                    }
                    if (isListed(k_bindMacros, word) && isPunctuator(peek(1), L"("))
                    {
                        const Place place = placeOf(*token);
                        advance();
                        const std::size_t open = m_at;
                        skipBalanced(L"(", L")");
                        if (m_at - open > 2)
                        {
                            const Names arguments = splitArguments(textBetween(open + 1, m_at - 2));
                            if (!arguments.empty())
                                m_surface.addBinding({ .name = arguments[0], .place = place });
                        }
                        previous = &tokens[m_at - 1];
                        continue;
                    }
                    if (word == Words::props && isPunctuator(peek(1), L"::")
                        && (isName(peek(2), Words::get) || isName(peek(2), Words::find))
                        && (isPunctuator(peek(3), L"(") || isPunctuator(peek(3), L"<")))
                    {
                        m_surface.addPropsRead(placeOf(*token));
                    }
                    // A qualified base is read once, from its first name.
                    const bool heads = !inBody && depth == 0 && !isPunctuator(previous, L"::");
                    if (parameters && owner && heads)
                        readBaseInitializer(*owner, *parameters);
                }
                previous = token;
                advance();
            }
        }

        void FileReader::readBaseInitializer(Type& owner, const ConstructorParameters& parameters)
        {
            const TokenList& tokens = m_text.tokens();
            std::size_t nameEnd = m_at;
            while (nameEnd + 2 < tokens.size() && isPunctuator(&tokens[nameEnd + 1], L"::")
                && tokens[nameEnd + 2].type == TokenType::Name)
            {
                nameEnd += 2;
            }
            std::size_t open = nameEnd + 1;
            if (open < tokens.size() && isPunctuator(&tokens[open], L"<"))
                open = pastAngles(open);
            if (open >= tokens.size())
                return;
            if (!isPunctuator(&tokens[open], L"{") && !isPunctuator(&tokens[open], L"("))
                return;
            const std::wstring_view spelled = text(tokens[nameEnd]);
            const bool base = std::ranges::any_of(owner.bases, [&](const std::wstring& candidate) {
                return bareName(withoutArguments(candidate)) == spelled;
            });
            if (!base)
                return;

            // The arguments, cut at the commas of the initializer's own level. Past the pack, a
            // value stands after the caller's and wins over them; a constructor taking no pack
            // leaves the caller no say at all.
            std::size_t first = open + 1;
            std::size_t lastCode = open;
            int depth = 0;
            int angle = 0;
            bool fixed = !parameters.packed;
            const Token* previous = nullptr;
            for (std::size_t i = open + 1; i < tokens.size(); ++i)
            {
                const Token& token = tokens[i];
                if (token.type == TokenType::Comment)
                    continue;
                bool closes = false;
                bool parts = false;
                if (token.type == TokenType::Punctuator)
                {
                    const std::wstring_view mark = text(token);
                    if (mark == L"(" || mark == L"[" || mark == L"{")
                    {
                        ++depth;
                    }
                    else if (mark == L")" || mark == L"]" || mark == L"}")
                    {
                        closes = depth == 0;
                        if (!closes)
                            --depth;
                    }
                    else if (mark == L"<" && previous
                        && (previous->type == TokenType::Name || isPunctuator(previous, L">")))
                    {
                        ++angle;
                    }
                    else if (mark == L">" && angle > 0)
                    {
                        --angle;
                    }
                    else if (mark == L"," && depth == 0 && angle == 0)
                    {
                        parts = true;
                    }
                }
                if (closes || parts)
                {
                    if (lastCode >= first)
                    {
                        std::optional<Restatement> restatement;
                        if (textBetween(first, lastCode).ends_with(k_ellipsis))
                            fixed = true;
                        else
                            restatement = restatementOf(first, lastCode, parameters.names);
                        if (restatement)
                        {
                            restatement->fixed = fixed;
                            restatement->place = placeOf(tokens[first]);
                            owner.restatements.push_back(std::move(*restatement));
                        }
                    }
                    if (closes)
                        return;
                    first = i + 1;
                }
                lastCode = i;
                previous = &token;
            }
        }

        std::optional<Restatement> FileReader::restatementOf(const std::size_t first,
            const std::size_t last, const Names& parameters) const
        {
            const TokenList& tokens = m_text.tokens();
            const Token& head = tokens[first];
            if (head.type != TokenType::Name)
                return std::nullopt;
            const std::wstring_view word = text(head);
            const bool callers = std::ranges::any_of(parameters, [&](const std::wstring& name) {
                return name == word;
            });
            if (callers || isListed(k_valueWords, word))
                return std::nullopt;

            std::size_t nameEnd = first;
            while (nameEnd + 2 <= last && isPunctuator(&tokens[nameEnd + 1], L"::")
                && tokens[nameEnd + 2].type == TokenType::Name)
            {
                nameEnd += 2;
            }
            std::size_t next = nameEnd + 1;
            if (next <= last && isPunctuator(&tokens[next], L"<"))
                next = pastAngles(next);
            Restatement restatement{ .value = textBetween(first, last) };
            const std::wstring name = textBetween(first, nameEnd);
            if (next > last)
            {
                const std::size_t scope = name.rfind(k_scope);
                if (scope != std::wstring::npos)
                    restatement.type = name.substr(0, scope);
                else if (const auto local = m_localTypes.find(name); local != m_localTypes.end())
                    restatement.type = local->second;
                return restatement;
            }
            const bool opens = isPunctuator(&tokens[next], L"{")
                || isPunctuator(&tokens[next], L"(");
            if (!opens || pastMatching(next) != last + 1)
                return std::nullopt;
            restatement.type = name;
            return restatement;
        }

        std::size_t FileReader::pastMatching(const std::size_t open) const
        {
            const TokenList& tokens = m_text.tokens();
            int depth = 0;
            for (std::size_t i = open; i < tokens.size(); ++i)
            {
                const Token& token = tokens[i];
                if (token.type != TokenType::Punctuator)
                    continue;
                const std::wstring_view mark = text(token);
                if (mark == L"(" || mark == L"[" || mark == L"{")
                    ++depth;
                if (mark == L")" || mark == L"]" || mark == L"}")
                    --depth;
                if (depth <= 0)
                    return i + 1;
            }
            return tokens.size();
        }

        std::size_t FileReader::pastAngles(const std::size_t open) const
        {
            const TokenList& tokens = m_text.tokens();
            int depth = 0;
            for (std::size_t i = open; i < tokens.size(); ++i)
            {
                if (isPunctuator(&tokens[i], L"<"))
                    ++depth;
                if (isPunctuator(&tokens[i], L">"))
                    --depth;
                if (depth <= 0)
                    return i + 1;
            }
            return tokens.size();
        }

        // From the opening bracket at the current token to past its match.
        void FileReader::skipBalanced(const std::wstring_view open, const std::wstring_view close)
        {
            int depth = 0;
            while (const Token* token = peek())
            {
                if (isPunctuator(token, open))
                    ++depth;
                else if (isPunctuator(token, close))
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

        // From the < at the current token to past its >. An angle inside brackets is an operator,
        // as C++ has it, and so is one written into <=, <<, >= or ->.
        void FileReader::skipAngles()
        {
            int depth = 0;
            int brackets = 0;
            while (const Token* token = peek())
            {
                if (isPunctuator(token, L"(") || isPunctuator(token, L"["))
                {
                    ++brackets;
                }
                else if (isPunctuator(token, L")") || isPunctuator(token, L"]"))
                {
                    --brackets;
                }
                else if (brackets == 0 && isPunctuator(token, L"<"))
                {
                    if (touches(m_at, L'<'))
                        advance();
                    else if (!touches(m_at, L'='))
                        ++depth;
                }
                else if (brackets == 0 && isPunctuator(token, L">"))
                {
                    const bool closes = !touchedBy(m_at, L'-') && !touches(m_at, L'=');
                    if (closes && --depth <= 0)
                    {
                        advance();
                        return;
                    }
                }
                advance();
            }
        }

        bool FileReader::touches(const std::size_t at, const wchar_t next) const
        {
            const TokenList& tokens = m_text.tokens();
            if (at + 1 >= tokens.size())
                return false;
            const Token& after = tokens[at + 1];
            return after.type == TokenType::Punctuator && after.line == tokens[at].line
                && after.start == tokens[at].end() && text(after) == std::wstring_view{ &next, 1 };
        }

        bool FileReader::touchedBy(const std::size_t at, const wchar_t previous) const
        {
            const TokenList& tokens = m_text.tokens();
            if (at == 0)
                return false;
            const Token& before = tokens[at - 1];
            return before.type == TokenType::Punctuator && before.line == tokens[at].line
                && before.end() == tokens[at].start
                && text(before) == std::wstring_view{ &previous, 1 };
        }

        // To past the end of a declaration: its terminator, or the body it opens.
        void FileReader::skipDeclaration()
        {
            int depth = 0;
            while (const Token* token = peek())
            {
                if (depth == 0 && isPunctuator(token, L"{"))
                {
                    skipBalanced(L"{", L"}");
                    if (isPunctuator(peek(), L";"))
                        advance();
                    return;
                }
                if (isPunctuator(token, L"(") || isPunctuator(token, L"["))
                    ++depth;
                if (isPunctuator(token, L")") || isPunctuator(token, L"]"))
                    --depth;
                if (depth <= 0 && isPunctuator(token, L";"))
                {
                    advance();
                    return;
                }
                if (isPunctuator(token, L"}"))
                    return;
                advance();
            }
        }

        // To past the next such punctuator at bracket depth zero, or to the end.
        void FileReader::skipPast(const std::wstring_view punctuator)
        {
            int depth = 0;
            while (const Token* token = peek())
            {
                const bool opens = isPunctuator(token, L"(") || isPunctuator(token, L"{")
                    || isPunctuator(token, L"[");
                const bool closes = isPunctuator(token, L")") || isPunctuator(token, L"}")
                    || isPunctuator(token, L"]");
                if (opens)
                    ++depth;
                if (closes)
                    --depth;
                if (depth < 0)
                    return;
                if (depth == 0 && isPunctuator(token, punctuator))
                {
                    advance();
                    return;
                }
                advance();
            }
        }

        std::wstring FileReader::readQualifiedName()
        {
            std::wstring name;
            if (isPunctuator(peek(), L"::"))
                advance();
            while (peek() && peek()->type == TokenType::Name)
            {
                name += text(*peek());
                advance();
                if (!isPunctuator(peek(), L"::") || !peek(1) || peek(1)->type != TokenType::Name)
                    break;
                name += k_scope;
                advance();
            }
            return name;
        }

        std::wstring FileReader::textBetween(const std::size_t first, const std::size_t last) const
        {
            const TokenList& tokens = m_text.tokens();
            if (first > last || last >= tokens.size())
                return {};
            return m_text.slice(tokens[first], tokens[last]);
        }

        Comment FileReader::commentAt(const std::size_t line,
            const std::optional<std::size_t> fallback, const std::wstring_view name,
            const bool aboveCounts) const
        {
            Comment comment;
            if (const std::optional<std::wstring_view> trailing = m_text.trailingComment(line))
            {
                comment.trailing = m_looseHints
                    ? withoutSeparators(docCommentBody(*trailing))
                    : std::wstring{ commentBody(*trailing) };
            }
            comment.lineWidth = m_text.width(line);
            if (aboveCounts)
                gatherAbove(line, comment);
            if (aboveCounts && comment.above.empty() && fallback && *fallback != line)
                gatherAbove(*fallback, comment);

            // Loose rules are not the routine's: the hint is the brief of the doc comment above, or
            // of the one on the line where there is none above - a comment on a line is as often a
            // tool's directive as words - and a See in it is words. A hint that only names the
            // declaration is a label, which tells a reader nothing.
            if (m_looseHints)
            {
                if (comment.above.empty() && comment.trailing)
                    comment.text = briefOf(trailingLines(line, *comment.trailing));
                else
                    comment.text = briefOf(comment.above);
                if (comment.text == bareName(name))
                    comment.text.clear();
                return comment;
            }

            std::wstring source;
            if (comment.trailing)
                source = *comment.trailing;
            else if (!comment.above.empty())
                source = comment.above.front();
            comment.reference = cutReference(source, name);
            comment.text = std::move(source);
            return comment;
        }

        // By loose rules, the lines carrying on the trailing comment of the line above them are
        // that line's, not the comment of the declaration under them.
        void FileReader::gatherAbove(const std::size_t line, Comment& comment) const
        {
            std::size_t first = line;
            while (first != 0 && m_text.kindOf(first - 1) == LineKind::CommentOnly)
                --first;
            if (m_looseHints && first != 0 && first != line)
                first = std::min(line, first + m_text.continuationsOf(first - 1));
            if (first == line)
                return;
            comment.aboveLine = first + 1;
            for (std::size_t i = first; i != line; ++i)
            {
                comment.aboveWidth = std::max(comment.aboveWidth, m_text.width(i));
                const std::wstring_view written = trimmed(m_text.line(i));
                // A block opening under other lines - //! over /** - is a paragraph of its own.
                if (m_looseHints && !comment.above.empty() && written.starts_with(k_blockComment))
                    comment.above.emplace_back();
                const std::wstring body = m_looseHints
                    ? withoutSeparators(docCommentBody(written))
                    : std::wstring{ commentBody(written) };
                if (!body.starts_with(k_todo))
                    comment.above.push_back(body);
            }
        }

        Names FileReader::trailingLines(const std::size_t line, const std::wstring& trailing) const
        {
            Names lines{ trailing };
            const std::size_t continued = m_text.continuationsOf(line);
            for (std::size_t i = line + 1; i <= line + continued; ++i)
                lines.push_back(withoutSeparators(docCommentBody(m_text.line(i))));
            return lines;
        }

        [[nodiscard]] std::wstring posixRelative(const std::filesystem::path& file,
            const std::filesystem::path& root)
        {
            std::error_code error;
            const std::filesystem::path relative = std::filesystem::relative(file, root, error);
            std::wstring text = (error ? file : relative).generic_wstring();
            return text;
        }

        // The steps of a scan, as its times name them.
        namespace Steps
        {
            constexpr std::wstring_view listFiles = L"list files";
            constexpr std::wstring_view readFiles = L"read files";
            constexpr std::wstring_view lexSources = L"lex sources";
            constexpr std::wstring_view gatherMacros = L"gather macros";
            constexpr std::wstring_view gatherSymbols = L"gather symbols";
            constexpr std::wstring_view expandSources = L"expand sources";
            constexpr std::wstring_view readSources = L"read sources";
            constexpr std::wstring_view readUnits = L"read units";
            constexpr std::wstring_view mergeUnits = L"merge units";
            constexpr std::wstring_view encloseIncludes = L"enclose includes";
            constexpr std::wstring_view republish = L"republish";
            constexpr std::wstring_view dropRedeclarations = L"drop redeclarations";
            constexpr std::wstring_view settlePascal = L"settle pascal";
            constexpr std::wstring_view resolveAll = L"resolve all";
        }

        using IndexTask = std::function<void(std::size_t)>;

        // Runs the task for every index below the count, on as many threads as the hardware has.
        void forEachIndex(const std::size_t count, const IndexTask& task)
        {
            const std::size_t hardware = std::thread::hardware_concurrency();
            const std::size_t threads = std::min(count, std::max<std::size_t>(hardware, 1));
            std::atomic<std::size_t> next{ 0 };
            std::vector<std::jthread> workers;
            workers.reserve(threads);
            for (std::size_t i = 0; i != threads; ++i)
            {
                workers.emplace_back([&]() {
                    for (std::size_t index = next++; index < count; index = next++)
                        task(index);
                });
            }
        }

        // What a file of the tree is to a read.
        enum class FileRole
        {
            Module,
            Header,
            PascalUnit,
            PascalInclude
        };

        struct TreeFile
        {
            std::filesystem::path path;
            FileRole role{ FileRole::Module };
            std::size_t root{ 0 };   // the folder of sources it was found in, by its index
        };

        using TreeFiles = std::vector<TreeFile>;

        [[nodiscard]] bool listedExtension(const std::span<const std::wstring_view> extensions,
            const std::filesystem::path& file)
        {
            std::wstring extension = file.extension().wstring();
            for (wchar_t& current : extension)
                current = static_cast<wchar_t>(std::towlower(current));
            return isListed(extensions, extension);
        }

        // The files a read takes, as one folder of sources gives them, sorted into lists by role.
        class FileGathering
        {
        public:
            explicit FileGathering(ScanMode);
            // Takes the folder's files, and with its subfolders, theirs; a file taken from a
            // folder before is not taken again.
            void take(const SourceFolder&, std::size_t root);
            [[nodiscard]] TreeFiles files();
        private:
            void takeFile(const std::filesystem::path&, std::size_t root);
            [[nodiscard]] static bool readsBefore(const TreeFile&, const TreeFile&);
        private:
            ScanMode m_mode;
            TreeFiles m_first{};
            TreeFiles m_second{};
            TreeFiles m_units{};
            TreeFiles m_includes{};
            std::set<std::filesystem::path> m_taken{};
        };

        FileGathering::FileGathering(const ScanMode mode)
            :
            m_mode{ mode }
        {
        }

        // A folder whose name starts with a dot is left out with everything under it - .seedocs,
        // .git, .vs - which is what the recursion has to be told before the step that would
        // enter it.
        void FileGathering::take(const SourceFolder& folder, const std::size_t root)
        {
            std::error_code error;
            if (!folder.subfolders)
            {
                for (const std::filesystem::directory_entry& entry :
                    std::filesystem::directory_iterator{ folder.path, error })
                {
                    if (entry.is_regular_file(error))
                        takeFile(entry.path(), root);
                }
                return;
            }
            std::filesystem::recursive_directory_iterator walk{
                folder.path,
                std::filesystem::directory_options::skip_permission_denied,
                error
            };
            const std::filesystem::recursive_directory_iterator end{};
            while (walk != end)
            {
                const std::filesystem::directory_entry& entry = *walk;
                if (entry.is_directory(error))
                {
                    if (entry.path().filename().wstring().starts_with(k_hiddenPrefix))
                        walk.disable_recursion_pending();
                }
                else if (entry.is_regular_file(error))
                {
                    takeFile(entry.path(), root);
                }
                walk.increment(error);
                if (error)
                    break;
            }
        }

        void FileGathering::takeFile(const std::filesystem::path& path, const std::size_t root)
        {
            TreeFiles* list = nullptr;
            FileRole role = FileRole::Module;
            if (m_mode == ScanMode::Full)
            {
                const std::wstring extension = path.extension().wstring();
                if (extension == k_interfaceExtension)
                    list = &m_first;
                else if (extension == k_implementationExtension)
                    list = &m_second;
            }
            else if (listedExtension(k_moduleExtensions, path))
            {
                list = &m_first;
            }
            else if (listedExtension(k_headerExtensions, path))
            {
                list = &m_second;
                role = FileRole::Header;
            }
            else if (listedExtension(k_pascalExtensions, path))
            {
                list = &m_units;
                role = FileRole::PascalUnit;
            }
            else if (listedExtension({ &k_pascalInclude, 1 }, path))
            {
                list = &m_includes;
                role = FileRole::PascalInclude;
            }
            if (!list || !m_taken.insert(path.lexically_normal()).second)
                return;
            list->push_back({ .path = path, .role = role, .root = root });
        }

        // What a read takes, in reading order: a full read every module interface and then every
        // implementation unit, a limited one every module interface, every header, every Delphi
        // unit and then every Delphi include file - each by its folder's place, then by path.
        TreeFiles FileGathering::files()
        {
            TreeFiles result;
            for (TreeFiles* list : { &m_first, &m_second, &m_units, &m_includes })
            {
                std::ranges::sort(*list, readsBefore);
                result.insert(result.end(), std::make_move_iterator(list->begin()),
                    std::make_move_iterator(list->end()));
                list->clear();
            }
            m_taken.clear();
            return result;
        }

        bool FileGathering::readsBefore(const TreeFile& left, const TreeFile& right)
        {
            if (left.root != right.root)
                return left.root < right.root;
            return left.path < right.path;
        }

        [[nodiscard]] TreeFiles filesOf(const SourceFolders& sources, const ScanMode mode)
        {
            FileGathering gathering{ mode };
            for (std::size_t root = 0; root != sources.size(); ++root)
                gathering.take(sources[root], root);
            return gathering.files();
        }

        // Where a file stands in the surface: its record's index, and the folder it is read as.
        struct FilePlace
        {
            std::size_t file{ 0 };
            std::wstring folder;
        };

        // The file's record in the surface, and the folder it stands in, as a path under its
        // source's root - so that folders of one name under two sources are one.
        [[nodiscard]] FilePlace addSourceFile(Surface& surface, const SourceFolders& sources,
            const TreeFile& file)
        {
            const std::filesystem::path& root = sources[file.root].path;
            SourceFile record;
            record.path = file.path;
            record.relative = posixRelative(file.path, root);
            record.root = file.root;
            std::wstring folder = posixRelative(file.path.parent_path(), root);
            if (folder == L".")
                folder.clear();
            return { .file = surface.addFile(std::move(record)), .folder = std::move(folder) };
        }

        void readSource(Surface& surface, const FilePlace& place, const TreeFile& file,
            const SourceText& source, const ScanMode mode, const HintRules rules)
        {
            const bool header = file.role == FileRole::Header;
            FileReader reader{ surface, place.file, source, place.folder, mode, rules, header };
            reader.read();
        }

        // A file that cannot be read is left out.
        void readFull(Surface& surface, const SourceFolders& sources, const TreeFiles& files,
            const HintRules rules, StepClock& clock)
        {
            for (const TreeFile& file : files)
            {
                const std::optional<std::wstring> text = Documents::readTextFile(file.path);
                clock.lap(Steps::readFiles);
                if (!text)
                    continue;
                const SourceText source{ *text, rules };
                clock.lap(Steps::lexSources);
                const FilePlace place = addSourceFile(surface, sources, file);
                readSource(surface, place, file, source, ScanMode::Full, rules);
                clock.lap(Steps::readSources);
            }
        }

        [[nodiscard]] bool isPascal(const TreeFile& file)
        {
            return file.role == FileRole::PascalUnit || file.role == FileRole::PascalInclude;
        }

        // **Every file is gathered from before any is read**, a macro being used far from the
        // header that defines it and a Delphi symbol far from the unit testing it; each is read
        // from disk once, for both passes. A Delphi include file is gathered from only. The units
        // are read on every thread the hardware has, each into a surface of its own, and taken
        // into the one surface in file order, so its records stand as one thread would put them.
        void readLimited(Surface& surface, const SourceFolders& sources, const TreeFiles& files,
            const HintRules rules, StepClock& clock)
        {
            std::vector<std::optional<std::wstring>> texts(files.size());
            forEachIndex(files.size(), [&](const std::size_t i) {
                texts[i] = Documents::readTextFile(files[i].path);
            });
            clock.lap(Steps::readFiles);

            Macros macros;
            for (std::size_t i = 0; i != files.size(); ++i)
            {
                if (!texts[i] || isPascal(files[i]))
                    continue;
                const SourceText gathered{ *texts[i], rules, Preprocess::Gather, &macros };
            }
            clock.lap(Steps::gatherMacros);

            std::vector<std::wstring_view> pascalTexts;
            for (std::size_t i = 0; i != files.size(); ++i)
            {
                if (texts[i] && isPascal(files[i]))
                    pascalTexts.emplace_back(*texts[i]);
            }
            const PascalSymbols symbols = pascalSymbolsOf(pascalTexts);
            clock.lap(Steps::gatherSymbols);

            // The records come first, so that a unit read on a worker knows its file's index.
            std::vector<std::optional<FilePlace>> places(files.size());
            for (std::size_t i = 0; i != files.size(); ++i)
            {
                if (texts[i] && files[i].role != FileRole::PascalInclude)
                    places[i] = addSourceFile(surface, sources, files[i]);
            }
            std::vector<Surface> fragments(files.size());
            forEachIndex(files.size(), [&](const std::size_t i) {
                if (!places[i] || files[i].role != FileRole::PascalUnit)
                    return;
                readPascalUnit(fragments[i], places[i]->file, *texts[i], places[i]->folder,
                    symbols, rules);
            });
            clock.lap(Steps::readUnits);

            for (std::size_t i = 0; i != files.size(); ++i)
            {
                if (!places[i])
                    continue;
                if (files[i].role == FileRole::PascalUnit)
                {
                    surface.append(std::move(fragments[i]));
                    clock.lap(Steps::mergeUnits);
                    continue;
                }
                const SourceText source{ *texts[i], rules, Preprocess::Expand, &macros };
                clock.lap(Steps::expandSources);
                readSource(surface, *places[i], files[i], source, ScanMode::Limited, rules);
                clock.lap(Steps::readSources);
            }
            surface.encloseIncludes();
            clock.lap(Steps::encloseIncludes);
            surface.republish();
            clock.lap(Steps::republish);
            surface.dropRedeclarations();
            clock.lap(Steps::dropRedeclarations);
            settlePascal(surface);
            clock.lap(Steps::settlePascal);
        }
    }

    Surface scanSources(const SourceFolders& sources, const ScanMode mode, const HintRules rules,
        ScanSteps* steps)
    {
        StepClock clock{ steps };
        Surface surface;
        surface.setMode(mode);
        surface.setHintRules(rules);
        const TreeFiles files = filesOf(sources, mode);
        clock.lap(Steps::listFiles);
        if (mode == ScanMode::Limited)
            readLimited(surface, sources, files, rules, clock);
        else
            readFull(surface, sources, files, rules, clock);
        surface.resolveAll();
        clock.lap(Steps::resolveAll);
        surface.setScanned(std::chrono::floor<std::chrono::seconds>(
            std::chrono::system_clock::now()));
        return surface;
    }
}
