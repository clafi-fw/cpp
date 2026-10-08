module SeeDocs_App.Scanner;

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
        constexpr wchar_t k_hiddenPrefix = L'.';   // a folder so named is not read
        constexpr std::wstring_view k_scope = L"::";
        constexpr std::wstring_view k_lineComment = L"//";
        constexpr std::wstring_view k_brief = L"@brief ";
        constexpr std::wstring_view k_todo = L"TODO";
        constexpr std::wstring_view k_textMacro = L"DERIVED_TEXT_CLASS";
        constexpr std::wstring_view k_textBase = L"Text";
        constexpr std::wstring_view k_memberPrefix = L"m_";

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

        // A file as lines and tokens, lexed line by line with the framework's C++ lexer and cut
        // once more where that lexer, built for colouring, leaves a plain name without a token.
        class SourceText
        {
        public:
            explicit SourceText(std::wstring_view text);
            [[nodiscard]] const TokenList& tokens() const { return m_tokens; }
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
            // The text from one token to another, both included, comments cut out and lines joined
            // with one blank.
            [[nodiscard]] std::wstring slice(const Token& from, const Token& to) const;
        private:
            void tokenizeLine(std::size_t index, Syntax::LineState&, Syntax::StateStrings&,
                bool& directiveContinues);
            void takeGap(std::size_t line, std::size_t from, std::size_t to);
            void push(TokenType, std::size_t line, std::size_t start, std::size_t end);
            void classifyLine(std::size_t firstToken);
        private:
            std::vector<std::wstring> m_lines;
            TokenList m_tokens;
            std::vector<LineKind> m_kinds;
            std::vector<std::size_t> m_firstTokens;   // per line, the index of its first token
        };

        SourceText::SourceText(const std::wstring_view text)
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

            Syntax::LineState state{};
            Syntax::StateStrings strings;
            bool directiveContinues = false;
            for (std::size_t i = 0; i != m_lines.size(); ++i)
            {
                const std::size_t firstToken = m_tokens.size();
                m_firstTokens.push_back(firstToken);
                tokenizeLine(i, state, strings, directiveContinues);
                classifyLine(firstToken);
            }
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
            if (m_kinds[line] != LineKind::Code)
                return std::nullopt;
            const std::size_t first = m_firstTokens[line];
            std::size_t last = first;
            while (last != m_tokens.size() && m_tokens[last].line == line)
                ++last;
            if (last == first)
                return std::nullopt;
            const Token& tail = m_tokens[last - 1];
            if (tail.type != TokenType::Comment || last - 1 == first)
                return std::nullopt;
            const std::wstring_view comment = text(tail);
            if (!comment.starts_with(k_lineComment))
                return std::nullopt;
            return comment;
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
                // Only the comments standing inside the range are cut; the tokens of a line are
                // in order, so the first past the range ends the walk.
                for (std::size_t i = m_firstTokens[line]; i != m_tokens.size(); ++i)
                {
                    const Token& token = m_tokens[i];
                    if (token.line != line || token.start >= end)
                        break;
                    if (token.type != TokenType::Comment || token.end() <= at)
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

        void SourceText::classifyLine(const std::size_t firstToken)
        {
            if (firstToken == m_tokens.size())
            {
                m_kinds.push_back(LineKind::Blank);
                return;
            }
            bool commentsOnly = true;
            for (std::size_t i = firstToken; i != m_tokens.size(); ++i)
            {
                if (m_tokens[i].type != TokenType::Comment)
                    commentsOnly = false;
            }
            const bool lineComment = text(m_tokens[firstToken]).starts_with(k_lineComment);
            m_kinds.push_back(commentsOnly && lineComment ? LineKind::CommentOnly : LineKind::Code);
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

        // Reads one file's tokens into the surface.
        class FileReader
        {
        public:
            FileReader(Surface&, std::size_t file, const SourceText&, std::wstring category);
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
            void readNamespace(bool exported);
            void readExportBlock();
            void readTemplateHead(const Token& keyword, Prefix&);
            void skipRequiresClause();
            void readType(TypeKind, const Prefix&);
            void readEnum(const Prefix&);
            void readUsing(const Prefix&);
            void readUsingDeclaration(const Token& keyword);
            void readConcept(const Prefix&);
            void readMacro(const Token& name, Type* owner, Access);
            void readDeclarator(const Prefix&);
            void readFunction(const Prefix&, const Token& start, std::size_t nameAt,
                std::wstring name, std::size_t parenAt);
            void readVariable(const Prefix&, const Token& start, std::size_t nameAt,
                std::size_t end);
            void skipBody(Type* owner, Access, bool initializers);
            void skipBalanced(std::wstring_view open, std::wstring_view close);
            void skipDeclaration();
            void skipPast(std::wstring_view punctuator);
            [[nodiscard]] std::wstring readQualifiedName();
            [[nodiscard]] std::wstring textBetween(std::size_t first, std::size_t last) const;
            [[nodiscard]] Comment commentAt(std::size_t line, std::optional<std::size_t> fallback,
                std::wstring_view name, bool aboveCounts = true) const;
            void gatherAbove(std::size_t line, Comment&) const;
        private:
            Surface& m_surface;
            std::size_t m_file;
            const SourceText& m_text;
            std::wstring m_category;
            std::size_t m_at{ 0 };
            std::vector<Scope> m_scopes;
            std::optional<Module> m_module;
        };

        FileReader::FileReader(Surface& surface, const std::size_t file, const SourceText& text,
            std::wstring category)
            :
            m_surface{ surface },
            m_file{ file },
            m_text{ text },
            m_category{ std::move(category) }
        {
        }

        void FileReader::read()
        {
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

        bool FileReader::scopeExported() const
        {
            for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); ++scope)
            {
                if (scope->anonymous)
                    return false;
                if (scope->kind == Scope::Kind::Namespace)
                    return scope->exported;
            }
            return false;
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
                    case TokenType::Directive:
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
                        readExportBlock();
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
                    readNamespace(prefix.exported);
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
                else if (propertyMacro(word) || word == Words::declareEvent)
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

        void FileReader::readNamespace(const bool exported)
        {
            advance();
            Scope scope;
            scope.kind = Scope::Kind::Namespace;
            if (isPunctuator(peek(), L"{"))
            {
                scope.name = currentNamespace();
                scope.anonymous = true;
            }
            else
            {
                const std::wstring name = readQualifiedName();
                if (isPunctuator(peek(), L"="))
                {
                    skipPast(L";");
                    return;
                }
                const std::wstring enclosing = currentNamespace();
                scope.name = enclosing.empty() ? name : enclosing + std::wstring{ k_scope } + name;
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

        // export { ... } - a namespace scope in all but name, exporting what it holds.
        void FileReader::readExportBlock()
        {
            advance();
            Scope scope;
            scope.kind = Scope::Kind::Namespace;
            scope.name = currentNamespace();
            scope.exported = true;
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
            skipBalanced(L"<", L">");
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
                        skipBalanced(L"<", L">");
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
                skipBalanced(L"<", L">");
                name += L"<" + textBetween(open + 1, m_at - 2) + L">";
            }
            if (isName(peek(), Words::finalWord))
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
            type.category = m_category;
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
                        skipBalanced(L"<", L">");
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
            if (isName(peek(), Words::classWord) || isName(peek(), Words::structWord))
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
            type.exported = outer ? outer->type->exported : (prefix.exported || scopeExported());
            type.access = outer ? outer->access : Access::Public;
            type.category = m_category;
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
            type.category = m_category;
            type.place = placeOf(keyword);
            type.comment = commentAt(keyword.line, prefix.templateLine, name);
            if (isPunctuator(peek(), L";"))
                advance();
            m_surface.addType(std::move(type));
        }

        // Inside a type, using Base::Base takes the base's constructors and using Base::name
        // republishes a member under the label the line stands in; at namespace scope it is
        // passed over.
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
            if (!scope || last < first + 2 || tokens[last].type != TokenType::Name
                || !isPunctuator(&tokens[last - 1], L"::"))
            {
                return;
            }
            Using declaration;
            declaration.base = textBetween(first, last - 2);
            declaration.name = std::wstring{ text(tokens[last]) };
            declaration.access = scope->access;
            declaration.place = placeOf(keyword);
            declaration.comment = commentAt(keyword.line, std::nullopt, declaration.name);
            scope->type->usings.push_back(std::move(declaration));
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
            type.category = m_category;
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
                else if (prefix.exported || scopeExported())
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
                skipBody(owner, scope ? scope->access : Access::Public, isPunctuator(peek(), L":"));
        }

        void FileReader::readVariable(const Prefix& prefix, const Token& start,
            const std::size_t nameAt, const std::size_t end)
        {
            const TokenList& tokens = m_text.tokens();
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

            Scope* scope = typeScope();
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
                variable.exported = true;
                variable.field = std::move(field);
                m_surface.addVariable(std::move(variable));
            }
        }

        // A body, from the initializer list or the opening brace to the closing one, read for the
        // binds written in it: the property binds and reads go to the owner, the constructor's
        // initialisers and hand-written Props::get calls to the surface. In an initializer list a
        // brace after a name is an initializer; the body's brace follows a bracket or a brace.
        void FileReader::skipBody(Type* owner, const Access access, const bool initializers)
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
                else if (token->type == TokenType::Name)
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
                }
                previous = token;
                advance();
            }
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
                comment.trailing = std::wstring{ commentBody(*trailing) };
            comment.lineWidth = m_text.width(line);
            if (aboveCounts)
                gatherAbove(line, comment);
            if (aboveCounts && comment.above.empty() && fallback && *fallback != line)
                gatherAbove(*fallback, comment);

            std::wstring source;
            if (comment.trailing)
                source = *comment.trailing;
            else if (!comment.above.empty())
                source = comment.above.front();
            comment.reference = cutReference(source, name);
            comment.text = std::move(source);
            return comment;
        }

        void FileReader::gatherAbove(const std::size_t line, Comment& comment) const
        {
            std::size_t first = line;
            while (first != 0 && m_text.kindOf(first - 1) == LineKind::CommentOnly)
                --first;
            if (first == line)
                return;
            comment.aboveLine = first + 1;
            for (std::size_t i = first; i != line; ++i)
            {
                comment.aboveWidth = std::max(comment.aboveWidth, m_text.width(i));
                const std::wstring_view body = commentBody(trimmed(m_text.line(i)));
                if (!body.starts_with(k_todo))
                    comment.above.emplace_back(body);
            }
        }

        [[nodiscard]] std::wstring posixRelative(const std::filesystem::path& file,
            const std::filesystem::path& root)
        {
            std::error_code error;
            const std::filesystem::path relative = std::filesystem::relative(file, root, error);
            std::wstring text = (error ? file : relative).generic_wstring();
            return text;
        }
    }

    // A folder whose name starts with a dot is left out with everything under it - .seedocs, .git,
    // .vs - which is what the recursion has to be told before the step that would enter it.
    Surface scanTree(const std::filesystem::path& root)
    {
        Surface surface;
        std::vector<std::filesystem::path> interfaces;
        std::vector<std::filesystem::path> implementations;
        std::error_code error;
        std::filesystem::recursive_directory_iterator walk{ root, error };
        const std::filesystem::recursive_directory_iterator end{};
        while (walk != end)
        {
            const std::filesystem::directory_entry& entry = *walk;
            if (entry.is_directory())
            {
                if (entry.path().filename().wstring().starts_with(k_hiddenPrefix))
                    walk.disable_recursion_pending();
            }
            else if (entry.is_regular_file())
            {
                const std::wstring extension = entry.path().extension().wstring();
                if (extension == k_interfaceExtension)
                    interfaces.push_back(entry.path());
                else if (extension == k_implementationExtension)
                    implementations.push_back(entry.path());
            }
            ++walk;
        }
        std::ranges::sort(interfaces);
        std::ranges::sort(implementations);
        for (const std::filesystem::path& file : interfaces)
            scanFile(surface, root, file);
        for (const std::filesystem::path& file : implementations)
            scanFile(surface, root, file);
        surface.resolveAll();
        return surface;
    }

    bool scanFile(Surface& surface, const std::filesystem::path& root,
        const std::filesystem::path& file)
    {
        const std::optional<std::wstring> text = Documents::readTextFile(file);
        if (!text)
            return false;
        SourceFile record;
        record.path = file;
        record.relative = posixRelative(file, root);
        const std::size_t index = surface.addFile(std::move(record));
        std::wstring category = posixRelative(file.parent_path(), root);
        if (category == L".")
            category.clear();
        const SourceText source{ *text };
        FileReader reader{ surface, index, source, category };
        reader.read();
        return true;
    }
}
