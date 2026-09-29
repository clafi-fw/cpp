module ClaFi.Core.Syntax.Languages;

import ClaFi.Core.Syntax.Indent;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi::Syntax
{
    namespace
    {
        namespace PascalMode
        {
            constexpr std::uint8_t braceDirective = Mode::firstLanguageMode;
            constexpr std::uint8_t parenDirective = Mode::firstLanguageMode + 1;
        }

        // A directive that is an everyday name as well - a property's accessors, a method's
        // message - and a keyword only past a declaration's signature. Sorted, and lower case.
        constexpr auto k_pascalContextual = std::to_array<std::wstring_view>({
            L"default", L"dispid", L"implements", L"index", L"message", L"name", L"nodefault",
            L"read", L"stored", L"write"
        });
        static_assert(std::ranges::is_sorted(k_pascalContextual));

        // The words a declaration line opens with. Sorted, and lower case.
        constexpr auto k_pascalDeclarers = std::to_array<std::wstring_view>({
            L"class", L"constructor", L"destructor", L"function", L"operator", L"procedure",
            L"property"
        });
        static_assert(std::ranges::is_sorted(k_pascalDeclarers));

        // The header of a source file: one of these, a name, and a semicolon closing the line.
        constexpr auto k_pascalHeaders = std::to_array<std::wstring_view>({
            L"library", L"program", L"unit"
        });

        constexpr auto k_pascalBlockWords = std::to_array<std::wstring_view>({
            L"begin", L"end"
        });

        constexpr auto k_pascalRoutineWords = std::to_array<std::wstring_view>({
            L"function", L"procedure"
        });

        using DigitTest = bool (*)(wchar_t);

        [[nodiscard]] bool isHexDigit(const wchar_t value)
        {
            return std::iswxdigit(value) != 0;
        }

        [[nodiscard]] bool isBinaryDigit(const wchar_t value)
        {
            return value == L'0' || value == L'1';
        }

        // From index to the end of the digits the test accepts from `digits` on, as one token.
        void takeDigits(Scan& scan, const std::size_t digits, const DigitTest isDigitOf,
            const TokenKind kind)
        {
            std::size_t end = digits;
            while (end != scan.line.size() && isDigitOf(scan.line[end]))
                ++end;

            scan.emit(scan.index, end, kind);
            scan.index = end;
        }

        // A string: quoted with single quotes, closed on its own line, and carrying a quote as
        // two - drawn as an escape, the way a backslash and what it escapes are elsewhere.
        void takePascalString(Scan& scan)
        {
            std::size_t runStart = scan.index;
            std::size_t at = scan.index + 1;
            while (at < scan.line.size())
            {
                if (scan.line[at] != L'\'')
                {
                    ++at;
                    continue;
                }
                if (at + 1 < scan.line.size() && scan.line[at + 1] == L'\'')
                {
                    scan.emit(runStart, at, TokenKind::String);
                    scan.emit(at, at + 2, TokenKind::Escape);
                    at += 2;
                    runStart = at;
                    continue;
                }
                ++at;
                break;
            }
            scan.emit(runStart, at, TokenKind::String);
            scan.index = at;
        }

        // A character by its code - #13, #$0D - which a string may abut on either side.
        void takeCharCode(Scan& scan)
        {
            if (scan.peek() == L'$')
                takeDigits(scan, scan.index + 2, isHexDigit, TokenKind::Escape);
            else
                takeDigits(scan, scan.index + 1, isDigit, TokenKind::Escape);
        }

        // Whether the line opens with a word a declaration does.
        [[nodiscard]] bool opensDeclaration(const Scan& scan)
        {
            const std::size_t start = scan.pastBlanks(0);
            const std::size_t end = endOfName(scan.line, start, scan.language);
            return isListedNoCase(k_pascalDeclarers, scan.line.substr(start, end - start));
        }

        // Whether a colon or a semicolon outside every bracket stands before index on the line -
        // past a routine's parameters and a property's own name, where the directives begin.
        [[nodiscard]] bool pastSignature(const Scan& scan)
        {
            int depth = 0;
            for (std::size_t i = 0; i != scan.index; ++i)
            {
                switch (scan.line[i])
                {
                    case L'(':
                    case L'[':
                        ++depth;
                        break;
                    case L')':
                    case L']':
                        --depth;
                        break;
                    case L':':
                    case L';':
                        if (depth == 0)
                            return true;
                        break;
                    default:
                        break;
                }
            }
            return false;
        }

        // A directive that is also a name, as a keyword where it stands as a directive. Anywhere
        // else it is left to the tables, which list it nowhere.
        [[nodiscard]] bool takeContextualDirective(Scan& scan)
        {
            const std::size_t end = endOfName(scan.line, scan.index, scan.language);
            const std::wstring_view word = scan.line.substr(scan.index, end - scan.index);
            if (!isListedNoCase(k_pascalContextual, word))
                return false;
            if (!opensDeclaration(scan) || !pastSignature(scan))
                return false;

            scan.emit(scan.index, end, TokenKind::Keyword);
            scan.index = end;
            return true;
        }

        // What a reading of the lines finds of the language's shape.
        struct PascalSigns
        {
            bool header{ false };      // unit, program or library: the word, a name, a semicolon
            bool directive{ false };   // a line opening a compiler directive
            bool block{ false };       // a line opening with begin or end
            bool routine{ false };     // a line opening with procedure or function
        };

        [[nodiscard]] PascalSigns readPascalSigns(const std::wstring_view text)
        {
            PascalSigns signs;
            std::size_t start = 0;
            while (start < text.size())
            {
                const std::wstring_view line = lineFrom(text, start);
                if (line.empty())
                    continue;

                if (line.back() == L';' && opensWithAnyWordNoCase(line, k_pascalHeaders))
                    signs.header = true;
                if (line.starts_with(L"{$"))
                    signs.directive = true;
                if (opensWithAnyWordNoCase(line, k_pascalBlockWords))
                    signs.block = true;
                if (opensWithAnyWordNoCase(line, k_pascalRoutineWords))
                    signs.routine = true;
            }
            return signs;
        }


        //---------------------------------------------------------------------
        // Reading what a text declares


        // The words that open a routine's header. Sorted, and lower case.
        constexpr auto k_pascalRoutineOpeners = std::to_array<std::wstring_view>({
            L"constructor", L"destructor", L"function", L"operator", L"procedure"
        });
        static_assert(std::ranges::is_sorted(k_pascalRoutineOpeners));

        // The directives a header may close with, each ended by its own semicolon. A header with
        // forward or external among them has no body. Sorted, and lower case.
        constexpr auto k_pascalRoutineDirectives = std::to_array<std::wstring_view>({
            L"abstract", L"assembler", L"cdecl", L"delayed", L"deprecated", L"dispid", L"dynamic",
            L"experimental", L"export", L"external", L"far", L"final", L"forward", L"inline",
            L"library", L"message", L"near", L"overload", L"override", L"pascal", L"platform",
            L"register", L"reintroduce", L"safecall", L"static", L"stdcall", L"unsafe",
            L"varargs", L"virtual", L"winapi"
        });
        static_assert(std::ranges::is_sorted(k_pascalRoutineDirectives));

        // The words that open a section, a routine or a block, at which an entry left without its
        // semicolon ends. Sorted, and lower case.
        constexpr auto k_pascalSectionWords = std::to_array<std::wstring_view>({
            L"begin", L"const", L"constructor", L"destructor", L"end", L"exports",
            L"finalization", L"function", L"implementation", L"initialization", L"operator",
            L"procedure", L"resourcestring", L"threadvar", L"type", L"uses", L"var"
        });
        static_assert(std::ranges::is_sorted(k_pascalSectionWords));

        // The words a statement block opens with, each closed by an end. Sorted.
        constexpr auto k_pascalBlockOpeners = std::to_array<std::wstring_view>({
            L"asm", L"begin", L"case", L"try"
        });
        static_assert(std::ranges::is_sorted(k_pascalBlockOpeners));

        // The modifiers a parameter group opens with. Sorted, and lower case.
        constexpr auto k_pascalParameterModifiers = std::to_array<std::wstring_view>({
            L"const", L"constref", L"out", L"var"
        });
        static_assert(std::ranges::is_sorted(k_pascalParameterModifiers));

        // One word of a text as the reader walks it - a name, a keyword, a number or a mark,
        // with the comments, strings and directives left out.
        struct PascalWord
        {
            std::wstring_view text;
            std::size_t start{ 0 };   // in the whole text
            TokenKind kind{ TokenKind::Plain };
            [[nodiscard]] std::size_t end() const { return start + text.size(); }
        };

        using PascalWords = std::vector<PascalWord>;
        using WordIndices = std::vector<std::size_t>;

        // Whether the word spells the lower-case one, in any case.
        [[nodiscard]] bool spells(const PascalWord& word, const std::wstring_view lower)
        {
            const auto foldCase = [](const wchar_t value){
                return static_cast<wchar_t>(std::towlower(value));
            };
            return std::ranges::equal(word.text, lower, std::ranges::equal_to{}, foldCase);
        }

        [[nodiscard]] bool isMark(const PascalWord& word)
        {
            return word.kind == TokenKind::Operator || word.kind == TokenKind::Punctuation;
        }

        [[nodiscard]] bool isMark(const PascalWord& word, const std::wstring_view mark)
        {
            return isMark(word) && word.text == mark;
        }

        // A name of the text's own: what no table lists, a name the type rule takes, or a name
        // standing before its call's bracket.
        [[nodiscard]] bool isName(const PascalWord& word)
        {
            return word.kind == TokenKind::Plain
                || word.kind == TokenKind::Type
                || word.kind == TokenKind::Function;
        }

        // The marks of an operator run, one word each: := and .. stand whole, the rest alone.
        void appendMarks(const std::wstring_view line, const std::size_t lineStart,
            const TextRange& run, PascalWords& words)
        {
            std::size_t at = run.start;
            while (at != run.end())
            {
                const wchar_t next = at + 1 != run.end() ? line[at + 1] : L'\0';
                const bool assign = line[at] == L':' && next == L'=';
                const bool range = line[at] == L'.' && next == L'.';
                const std::size_t length = assign || range ? 2 : 1;
                words.push_back({ line.substr(at, length), lineStart + at, TokenKind::Operator });
                at += length;
            }
        }

        // The names in a run of the line no token claims.
        void appendNames(const std::wstring_view line, const std::size_t lineStart,
            const std::size_t from, const std::size_t to, PascalWords& words)
        {
            std::size_t at = from;
            while (at < to)
            {
                if (!isNameStart(line[at], Languages::pascal))
                {
                    ++at;
                    continue;
                }
                const std::size_t end = std::min(endOfName(line, at, Languages::pascal), to);
                words.push_back({ line.substr(at, end - at), lineStart + at, TokenKind::Plain });
                at = end;
            }
        }

        void appendLineWords(const std::wstring_view line, const std::size_t lineStart,
            const Tokens& tokens, PascalWords& words)
        {
            std::size_t at = 0;
            for (const Token& token : tokens)
            {
                appendNames(line, lineStart, at, token.range.start, words);
                const std::wstring_view text = line.substr(token.range.start, token.range.length);
                switch (token.kind)
                {
                    case TokenKind::Comment:
                    case TokenKind::String:
                    case TokenKind::Escape:
                    case TokenKind::Directive:
                        break;
                    case TokenKind::Operator:
                        appendMarks(line, lineStart, token.range, words);
                        break;
                    default:
                        words.push_back({ text, lineStart + token.range.start, token.kind });
                        break;
                }
                at = token.range.end();
            }
            appendNames(line, lineStart, at, line.size(), words);
        }

        // The whole text as words, lexed line by line the way the box colours it.
        [[nodiscard]] PascalWords pascalWords(const std::wstring_view text)
        {
            PascalWords words;
            StateStrings strings;
            Tokens tokens;
            LineState state;
            std::size_t lineStart = 0;
            while (true)
            {
                std::size_t lineEnd = text.find(L'\n', lineStart);
                const bool last = lineEnd == std::wstring_view::npos;
                if (last)
                    lineEnd = text.size();

                const std::wstring_view line = text.substr(lineStart, lineEnd - lineStart);
                tokens.clear();
                state = lexLine(Languages::pascal, line, state, strings, &tokens);
                appendLineWords(line, lineStart, tokens, words);
                if (last)
                    return words;
                lineStart = lineEnd + 1;
            }
        }

        // Walks the words of a text once and gathers what they declare. The grammar it reads is
        // the declaring part of the language - sections, headers, blocks - and every statement
        // between is stepped over. See Syntax#declarations
        class PascalReader
        {
        public:
            PascalReader(std::wstring_view text, const PascalWords&);
            [[nodiscard]] Declarations read();
        private:
            [[nodiscard]] bool atEnd() const { return m_at >= m_words.size(); }
            [[nodiscard]] const PascalWord& current() const { return m_words[m_at]; }
            [[nodiscard]] bool currentSpells(std::wstring_view lower) const;
            [[nodiscard]] bool currentIsMark(std::wstring_view mark) const;
            [[nodiscard]] bool currentIsName() const;
            [[nodiscard]] bool nextIsMark(std::wstring_view mark) const;
            // Whether the word before the current one spells the lower-case one, or is the mark.
            [[nodiscard]] bool afterWord(std::wstring_view lower) const;
            [[nodiscard]] bool afterMark(std::wstring_view mark) const;
            [[nodiscard]] bool opensRoutine() const;
            [[nodiscard]] bool endsSection() const;
            // Whether the current word opens a type body that an end closes.
            [[nodiscard]] bool opensTypeBody() const;
            // Whether the class, interface or object at the current word carries a body.
            [[nodiscard]] bool classHasBody() const;
            // Where the last word taken ends, or zero with none taken yet.
            [[nodiscard]] std::size_t takenEnd() const;
            void readTopLevel();
            // At a header's first word. A header-only routine, an interface part's, declares
            // nothing of its own.
            void readRoutine(std::size_t depth, bool headerOnly);
            void readParameters(std::size_t depth);
            void readConstSection(std::size_t depth);
            void readVarSection(std::size_t depth);
            void readTypeSection();
            // At the block's opener, past its closing end when done.
            void readBody(std::size_t depth);
            // At an inline var or const inside a block.
            void readInlineDeclaration(std::size_t depth);
            // The names an entry opens with, separated by commas.
            [[nodiscard]] WordIndices readNames();
            // Steps over a type as spelled, up to the mark that ends it, which is left standing.
            void skipType();
            // Steps over a type and answers its one name, or nothing where more words spell it.
            [[nodiscard]] std::wstring_view readType();
            // At a type body's opener, past its closing end when done.
            void skipTypeBody();
            // Steps over a value up to the semicolon or the closing bracket that ends it, or a
            // section word, all left standing.
            void skipValue();
            void skipAngles();
            void declare(std::wstring_view name, CompletionKind, std::wstring spelled,
                std::wstring_view type, std::size_t depth);
            // Gives every declaration made from `first` on at that depth the scope that runs
            // from `from` to the last word taken.
            void closeScope(std::size_t first, std::size_t depth, std::size_t from);
            // The text from the word at `from` up to the current word, with what the words
            // leave out - a string, a comment - kept, and its blanks collapsed.
            [[nodiscard]] std::wstring spelled(std::size_t from) const;
        private:
            // A hint's line is cut past this many characters.
            static constexpr std::size_t k_spelledLimit{ 100 };
            std::wstring_view m_text;
            const PascalWords& m_words;
            std::size_t m_at{ 0 };
            Declarations m_out{};
        };

        PascalReader::PascalReader(const std::wstring_view text, const PascalWords& words)
            :
            m_text{ text },
            m_words{ words }
        {
        }

        Declarations PascalReader::read()
        {
            readTopLevel();
            return std::move(m_out);
        }

        bool PascalReader::currentSpells(const std::wstring_view lower) const
        {
            return !atEnd() && spells(current(), lower);
        }

        bool PascalReader::currentIsMark(const std::wstring_view mark) const
        {
            return !atEnd() && isMark(current(), mark);
        }

        bool PascalReader::currentIsName() const
        {
            return !atEnd() && isName(current());
        }

        bool PascalReader::nextIsMark(const std::wstring_view mark) const
        {
            return m_at + 1 < m_words.size() && isMark(m_words[m_at + 1], mark);
        }

        bool PascalReader::afterWord(const std::wstring_view lower) const
        {
            return m_at != 0 && spells(m_words[m_at - 1], lower);
        }

        bool PascalReader::afterMark(const std::wstring_view mark) const
        {
            return m_at != 0 && isMark(m_words[m_at - 1], mark);
        }

        bool PascalReader::opensRoutine() const
        {
            if (atEnd())
                return false;
            if (isListedNoCase(k_pascalRoutineOpeners, current().text))
                return true;
            return currentSpells(L"class") && m_at + 1 < m_words.size()
                && isListedNoCase(k_pascalRoutineOpeners, m_words[m_at + 1].text);
        }

        bool PascalReader::endsSection() const
        {
            return !atEnd() && isListedNoCase(k_pascalSectionWords, current().text);
        }

        bool PascalReader::opensTypeBody() const
        {
            if (currentSpells(L"record"))
                return true;
            // A procedural type's `of object` opens nothing.
            if (currentSpells(L"object"))
                return !afterWord(L"of");
            const bool classWord = currentSpells(L"class") || currentSpells(L"interface")
                || currentSpells(L"dispinterface");
            return classWord && classHasBody();
        }

        bool PascalReader::classHasBody() const
        {
            // A type's declaration is the one place the word follows an equals sign; inside a
            // body it is a member's own class word and opens nothing.
            if (!afterMark(L"="))
                return false;
            // Past the modifiers, a helper's subject, the parent list and an interface's GUID
            // stands either the body's first member or what says there is none: a semicolon for a
            // forward or a short declaration, a class reference's of.
            std::size_t at = m_at + 1;
            while (at < m_words.size()
                && (spells(m_words[at], L"abstract") || spells(m_words[at], L"sealed")))
            {
                ++at;
            }
            if (at < m_words.size() && spells(m_words[at], L"helper"))
            {
                at += 2;
                if (at < m_words.size() && isName(m_words[at]))
                    ++at;
            }
            for (const std::wstring_view bracket : { L"(", L"[" })
            {
                const std::wstring_view closer = bracket == L"(" ? L")" : L"]";
                if (at < m_words.size() && isMark(m_words[at], bracket))
                {
                    while (at < m_words.size() && !isMark(m_words[at], closer))
                        ++at;
                    ++at;
                }
            }
            if (at >= m_words.size())
                return true;
            return !isMark(m_words[at], L";") && !spells(m_words[at], L"of");
        }

        std::size_t PascalReader::takenEnd() const
        {
            return m_at == 0 ? 0 : m_words[m_at - 1].end();
        }

        void PascalReader::readTopLevel()
        {
            // A unit's interface part declares its routines by their headers alone.
            bool headerOnly = false;
            while (!atEnd())
            {
                if (currentSpells(L"interface"))
                {
                    headerOnly = true;
                    ++m_at;
                }
                else if (currentSpells(L"implementation"))
                {
                    headerOnly = false;
                    ++m_at;
                }
                else if (currentSpells(L"const") || currentSpells(L"resourcestring"))
                    readConstSection(0);
                else if (currentSpells(L"var") || currentSpells(L"threadvar"))
                    readVarSection(0);
                else if (currentSpells(L"type"))
                    readTypeSection();
                else if (opensRoutine())
                    readRoutine(0, headerOnly);
                else if (currentSpells(L"begin"))
                    readBody(0);
                else
                    ++m_at;
            }
        }

        void PascalReader::readRoutine(const std::size_t depth, const bool headerOnly)
        {
            const std::size_t headerStart = current().start;
            if (currentSpells(L"class"))
                ++m_at;
            const bool function = currentSpells(L"function");
            ++m_at;
            // The name, qualified by its class for a method's implementation, either with its
            // generic parameters.
            while (!atEnd())
            {
                if (currentIsName() || currentIsMark(L"."))
                    ++m_at;
                else if (currentIsMark(L"<"))
                    skipAngles();
                else
                    break;
            }

            const std::size_t firstLocal = m_out.size();
            if (currentIsMark(L"("))
                readParameters(depth + 1);
            std::wstring resultSpelled = L"Result";
            std::wstring_view resultType;
            if (function && currentIsMark(L":"))
            {
                ++m_at;
                const std::size_t typeFrom = m_at;
                resultType = readType();
                if (m_at != typeFrom)
                    resultSpelled = L"Result: " + spelled(typeFrom);
            }
            while (!atEnd() && !currentIsMark(L";") && !endsSection())
                ++m_at;
            if (currentIsMark(L";"))
                ++m_at;

            bool hasBody = !headerOnly;
            while (!atEnd() && isListedNoCase(k_pascalRoutineDirectives, current().text))
            {
                if (currentSpells(L"forward") || currentSpells(L"external"))
                    hasBody = false;
                while (!atEnd() && !currentIsMark(L";") && !endsSection())
                    ++m_at;
                if (currentIsMark(L";"))
                    ++m_at;
            }
            if (!hasBody)
            {
                // Parameters of a header alone are in force nowhere.
                m_out.resize(firstLocal);
                return;
            }
            if (function)
                declare(L"Result", CompletionKind::Variable, std::move(resultSpelled), resultType,
                    depth + 1);

            // The declaring part, and then the block. A routine left without its block ends at
            // the word that opens what follows it.
            while (!atEnd())
            {
                if (currentSpells(L"const") || currentSpells(L"resourcestring"))
                    readConstSection(depth + 1);
                else if (currentSpells(L"var") || currentSpells(L"threadvar"))
                    readVarSection(depth + 1);
                else if (currentSpells(L"type"))
                    readTypeSection();
                else if (opensRoutine())
                    readRoutine(depth + 1, false);
                else if (currentSpells(L"begin") || currentSpells(L"asm"))
                {
                    readBody(depth + 1);
                    break;
                }
                else if (endsSection())
                    break;
                else
                    ++m_at;
            }
            if (currentIsMark(L";"))
                ++m_at;
            closeScope(firstLocal, depth + 1, headerStart);
        }

        void PascalReader::readParameters(const std::size_t depth)
        {
            ++m_at;
            while (!atEnd() && !currentIsMark(L")"))
            {
                const std::size_t from = m_at;
                if (currentIsMark(L"["))
                {
                    while (!atEnd() && !currentIsMark(L"]"))
                        ++m_at;
                    if (!atEnd())
                        ++m_at;
                }
                // A modifier opens a group, so it stands where one starts; the same word anywhere
                // else opens the section that follows a list left open, where the list ends.
                const bool groupStart = afterMark(L"(") || afterMark(L";") || afterMark(L"]");
                if (groupStart && !atEnd()
                    && isListedNoCase(k_pascalParameterModifiers, current().text))
                    ++m_at;
                else if (endsSection())
                    break;
                const WordIndices names = readNames();
                std::wstring_view type;
                if (currentIsMark(L":"))
                {
                    ++m_at;
                    type = readType();
                }
                if (currentIsMark(L"="))
                {
                    ++m_at;
                    skipValue();
                }
                for (const std::size_t name : names)
                {
                    declare(m_words[name].text, CompletionKind::Variable, spelled(from), type,
                        depth);
                }
                if (currentIsMark(L";"))
                    ++m_at;
                else if (!atEnd() && !currentIsMark(L")") && !endsSection())
                    ++m_at;
            }
            if (currentIsMark(L")"))
                ++m_at;
        }

        void PascalReader::readConstSection(const std::size_t depth)
        {
            ++m_at;
            // An entry is a name with its equals sign or its type's colon after it, so a name
            // being typed at the section's end is no entry yet.
            while (currentIsName() && (nextIsMark(L"=") || nextIsMark(L":")))
            {
                const std::size_t from = m_at;
                ++m_at;
                std::wstring_view type;
                if (currentIsMark(L":"))
                {
                    ++m_at;
                    type = readType();
                }
                if (currentIsMark(L"="))
                {
                    ++m_at;
                    skipValue();
                }
                declare(m_words[from].text, CompletionKind::Constant, spelled(from), type, depth);
                if (currentIsMark(L";"))
                    ++m_at;
            }
        }

        void PascalReader::readVarSection(const std::size_t depth)
        {
            ++m_at;
            while (currentIsName() && (nextIsMark(L",") || nextIsMark(L":")))
            {
                const std::size_t from = m_at;
                const WordIndices names = readNames();
                std::wstring_view type;
                if (currentIsMark(L":"))
                {
                    ++m_at;
                    type = readType();
                }
                if (currentSpells(L"absolute"))
                {
                    ++m_at;
                    skipValue();
                }
                if (currentIsMark(L"="))
                {
                    ++m_at;
                    skipValue();
                }
                for (const std::size_t name : names)
                {
                    declare(m_words[name].text, CompletionKind::Variable, spelled(from), type,
                        depth);
                }
                if (currentIsMark(L";"))
                    ++m_at;
            }
        }

        void PascalReader::readTypeSection()
        {
            ++m_at;
            // Types declare no name a box completes to yet; the section is read so that a class
            // body's members and a record's fields are not taken for declarations of their own.
            while (currentIsName())
            {
                ++m_at;
                if (currentIsMark(L"<"))
                    skipAngles();
                if (!currentIsMark(L"="))
                    break;
                ++m_at;
                if (currentSpells(L"type"))
                    ++m_at;
                skipType();
                if (currentIsMark(L";"))
                    ++m_at;
            }
        }

        void PascalReader::readBody(const std::size_t depth)
        {
            int open = 0;
            while (!atEnd())
            {
                if (isListedNoCase(k_pascalBlockOpeners, current().text))
                {
                    ++open;
                    ++m_at;
                }
                else if (currentSpells(L"end"))
                {
                    ++m_at;
                    if (--open == 0)
                        return;
                }
                else if (currentSpells(L"var") || currentSpells(L"const"))
                    readInlineDeclaration(depth);
                else
                    ++m_at;
            }
        }

        void PascalReader::readInlineDeclaration(const std::size_t depth)
        {
            const bool constant = currentSpells(L"const");
            ++m_at;
            const std::size_t from = m_at;
            const WordIndices names = readNames();
            if (names.empty())
                return;
            std::wstring_view type;
            if (currentIsMark(L":"))
            {
                ++m_at;
                type = readType();
            }
            else if (!currentIsMark(L":=") && !currentIsMark(L"="))
                return;
            if (constant && currentIsMark(L"="))
            {
                ++m_at;
                skipValue();
            }
            const CompletionKind kind = constant
                ? CompletionKind::Constant
                : CompletionKind::Variable;
            for (const std::size_t name : names)
                declare(m_words[name].text, kind, spelled(from), type, depth);
        }

        WordIndices PascalReader::readNames()
        {
            WordIndices names;
            while (currentIsName())
            {
                names.push_back(m_at);
                ++m_at;
                if (!currentIsMark(L","))
                    break;
                ++m_at;
            }
            return names;
        }

        void PascalReader::skipType()
        {
            const std::size_t start = m_at;
            if (currentSpells(L"packed"))
                ++m_at;
            int depth = 0;
            while (!atEnd())
            {
                if (opensTypeBody())
                {
                    skipTypeBody();
                    continue;
                }
                const PascalWord& word = current();
                if (isMark(word))
                {
                    const bool ends = word.text == L";" || word.text == L"=" || word.text == L":=";
                    if (depth == 0 && (ends || word.text == L")"))
                        return;
                    if (word.text == L"(" || word.text == L"[")
                        ++depth;
                    if (word.text == L")" || word.text == L"]")
                        --depth;
                    ++m_at;
                    continue;
                }
                // A procedural type opens with the routine word, or has it after `reference to`;
                // anywhere else the word opens the routine that follows an unfinished entry.
                const bool routineWord = spells(word, L"procedure") || spells(word, L"function");
                if (routineWord && m_at != start && !afterWord(L"to"))
                    return;
                if (!routineWord && depth == 0 && (spells(word, L"absolute") || endsSection()))
                    return;
                ++m_at;
            }
        }

        std::wstring_view PascalReader::readType()
        {
            const std::size_t start = m_at;
            skipType();
            if (m_at == start + 1 && isName(m_words[start]))
                return m_words[start].text;
            return {};
        }

        void PascalReader::skipTypeBody()
        {
            ++m_at;
            int open = 1;
            while (!atEnd())
            {
                if (currentSpells(L"end"))
                {
                    ++m_at;
                    if (--open == 0)
                        return;
                    continue;
                }
                if (opensTypeBody())
                    ++open;
                ++m_at;
            }
        }

        void PascalReader::skipValue()
        {
            int depth = 0;
            while (!atEnd())
            {
                const PascalWord& word = current();
                if (isMark(word))
                {
                    if (depth == 0 && (word.text == L";" || word.text == L")"))
                        return;
                    if (word.text == L"(" || word.text == L"[")
                        ++depth;
                    if (word.text == L")" || word.text == L"]")
                        --depth;
                }
                else if (depth == 0 && endsSection())
                    return;
                ++m_at;
            }
        }

        void PascalReader::skipAngles()
        {
            int open = 0;
            while (!atEnd())
            {
                if (currentIsMark(L"<"))
                    ++open;
                if (currentIsMark(L">"))
                    --open;
                ++m_at;
                if (open == 0)
                    return;
            }
        }

        void PascalReader::declare(const std::wstring_view name, const CompletionKind kind,
            std::wstring spelled, const std::wstring_view type, const std::size_t depth)
        {
            m_out.push_back({
                .entry = {
                    .name = std::wstring{ name },
                    .kind = kind,
                    .signature = std::move(spelled),
                },
                .type = std::wstring{ type },
                .scope = { 0, m_text.size() },
                .depth = depth,
            });
        }

        void PascalReader::closeScope(const std::size_t first, const std::size_t depth,
            const std::size_t from)
        {
            // A routine the words run out in is being written, and reaches the text's end.
            const std::size_t end = atEnd() ? m_text.size() : takenEnd();
            const TextRange scope = { from, end - from };
            for (std::size_t i = first; i != m_out.size(); ++i)
            {
                if (m_out[i].depth == depth)
                    m_out[i].scope = scope;
            }
        }

        std::wstring PascalReader::spelled(const std::size_t from) const
        {
            const std::size_t start = m_words[from].start;
            const std::size_t end = atEnd() ? m_text.size() : std::max(start, current().start);
            std::wstring result;
            bool blank = false;
            for (const wchar_t value : m_text.substr(start, end - start))
            {
                if (isBlank(value) || value == L'\n' || value == L'\r')
                {
                    blank = true;
                    continue;
                }
                if (blank && !result.empty())
                    result.push_back(L' ');
                blank = false;
                result.push_back(value);
                if (result.size() == k_spelledLimit)
                {
                    result.append(L"...");
                    break;
                }
            }
            return result;
        }


        //---------------------------------------------------------------------
        // Placing a line


        // The words that open a block an end closes, whatever follows them. Sorted, and lower
        // case - as are the tables below.
        constexpr auto k_pascalIndentOpeners = std::to_array<std::wstring_view>({
            L"asm", L"begin", L"case", L"initialization", L"record", L"repeat", L"try"
        });
        static_assert(std::ranges::is_sorted(k_pascalIndentOpeners));

        // The words that open a type body where one follows them - see opensBody.
        constexpr auto k_pascalBodyWords = std::to_array<std::wstring_view>({
            L"class", L"dispinterface", L"interface", L"object"
        });
        static_assert(std::ranges::is_sorted(k_pascalBodyWords));

        constexpr auto k_pascalIndentClosers = std::to_array<std::wstring_view>({
            L"end", L"until"
        });
        static_assert(std::ranges::is_sorted(k_pascalIndentClosers));

        // The words that stand at their block's opener and indent what follows them.
        constexpr auto k_pascalIndentMiddles = std::to_array<std::wstring_view>({
            L"except", L"finalization", L"finally", L"private", L"protected", L"public",
            L"published", L"strict"
        });
        static_assert(std::ranges::is_sorted(k_pascalIndentMiddles));

        // The words a statement goes on past, onto the line after them.
        constexpr auto k_pascalIndentHangers = std::to_array<std::wstring_view>({
            L"do", L"then"
        });
        static_assert(std::ranges::is_sorted(k_pascalIndentHangers));

        // The words that open a section of entries, which the next section word closes.
        constexpr auto k_pascalIndentSections = std::to_array<std::wstring_view>({
            L"const", L"exports", L"label", L"resourcestring", L"threadvar", L"type", L"uses",
            L"var"
        });
        static_assert(std::ranges::is_sorted(k_pascalIndentSections));

        // The sections whose one entry is the whole section, ended by its semicolon.
        constexpr auto k_pascalOneEntrySections = std::to_array<std::wstring_view>({
            L"exports", L"uses"
        });
        static_assert(std::ranges::is_sorted(k_pascalOneEntrySections));

        // The words a file's parts open with, standing at the file's own column.
        constexpr auto k_pascalFileParts = std::to_array<std::wstring_view>({
            L"implementation", L"initialization", L"interface", L"library", L"program", L"unit"
        });
        static_assert(std::ranges::is_sorted(k_pascalFileParts));

        // The blocks whose opener at a line's end is closed below it by the block completion.
        constexpr auto k_pascalCompletedOpeners = std::to_array<std::wstring_view>({
            L"asm", L"begin", L"record", L"try"
        });
        static_assert(std::ranges::is_sorted(k_pascalCompletedOpeners));

        constexpr std::wstring_view k_pascalBlockCloser = L"end;";

        // Where a Pascal line stands, read back from it over the words of the lines above - the
        // block it is in, the statement before it, the section it closes. See Syntax#indent
        class PascalIndent
        {
        public:
            PascalIndent(const SourceLines&, std::size_t width);
        public:
            [[nodiscard]] LineIndent place(std::size_t line);
            // Every block an end or an until closes on a later line than it opens on, read from
            // the text's start. See Syntax#blocks
            [[nodiscard]] SourceBlocks blocks();
        private:
            // A word's place: its line, and where it stands among the line's words.
            struct Place
            {
                std::size_t line{ 0 };
                std::size_t index{ 0 };
            };
            // A statement's first word, and the word before it that ended what came before -
            // nothing at the text's start.
            struct StatementStart
            {
                Place first{};
                std::optional<Place> stop{};
            };
            using LineWords = std::unordered_map<std::size_t, PascalWords>;
        private:
            [[nodiscard]] const PascalWords& wordsOf(std::size_t line);
            [[nodiscard]] const PascalWord& wordAt(const Place&);
            // The word before the place, from the lines above where the place is a line's first.
            [[nodiscard]] std::optional<Place> before(const Place&);
            [[nodiscard]] std::optional<Place> after(const Place&);
            [[nodiscard]] std::size_t columnOf(std::size_t line) const;
            // Whether the word is the keyword of that lower-case spelling.
            [[nodiscard]] bool isWord(const Place&, std::wstring_view lower);
            [[nodiscard]] bool isListed(const Place&, Words lowerWords);
            [[nodiscard]] bool isMarkAt(const Place&, std::wstring_view mark);
            [[nodiscard]] bool opensBlock(const Place&);
            // Whether a class, an interface or an object word opens a body an end closes.
            [[nodiscard]] bool opensBody(const Place&);
            [[nodiscard]] bool opensAny(const Place&);
            // The else of a case, which follows its last branch's semicolon.
            [[nodiscard]] bool isCaseElse(const Place&);
            [[nodiscard]] bool isMiddle(const Place&);
            // A file part's word - an interface word opening a type body is not one.
            [[nodiscard]] bool isFilePart(const Place&);
            // The first word of a routine's header, a procedural type's word excepted.
            [[nodiscard]] bool opensRoutine(const Place&);
            // What a statement begun past this word cannot reach back over.
            [[nodiscard]] bool isStop(const Place&);
            // The opener of the block the place stands in, read back from it.
            [[nodiscard]] std::optional<Place> openerBefore(const Place&);
            // The opening bracket the closing one at the place closes.
            [[nodiscard]] std::optional<Place> bracketOpener(const Place&);
            // The statement the word ends, whole blocks and brackets inside it read as one.
            [[nodiscard]] StatementStart statementStart(const Place& last);
            // The column of the if an else pairs with, nothing where no then reaches it.
            [[nodiscard]] std::optional<std::size_t> ifColumn(const Place& elseWord);
            // The column of the section a section's word at the place closes - nothing inside a
            // block, where the word is a statement's.
            [[nodiscard]] std::optional<std::size_t> sectionColumn(const Place&);
            // Where a routine's header at the place stands: beside the routine closed before it,
            // or one level inside a routine still open - nothing for a member, or a file's first.
            [[nodiscard]] std::optional<std::size_t> nestedRoutineColumn(const Place&);
            // Whether a header at the place declares a routine whose body stands elsewhere - a
            // member's header, or one of a unit's interface part.
            [[nodiscard]] bool declaresOnly(const Place&);
            // Whether a closing square bracket ends a line's attribute - an interface's GUID, a
            // [Weak] - which is whole with no semicolon.
            [[nodiscard]] bool closesAttribute(const Place&);
            // Where a line stands that follows the word as a statement would.
            [[nodiscard]] std::size_t followingColumn(const Place& previous);
            // What closes the block the line leaves open at its end.
            [[nodiscard]] std::wstring_view closerOf(std::size_t line);
        private:
            const SourceLines& m_lines;
            std::size_t m_width;
            LineWords m_words{};
        };

        PascalIndent::PascalIndent(const SourceLines& lines, const std::size_t width)
            :
            m_lines{ lines },
            m_width{ width }
        {
        }

        LineIndent PascalIndent::place(const std::size_t line)
        {
            LineIndent result;
            result.closer = closerOf(line);
            const Place first = { line, 0 };
            const std::optional<Place> previous = before(first);
            if (wordsOf(line).empty())
            {
                result.column = previous.has_value() ? followingColumn(previous.value()) : 0;
                return result;
            }

            // A closer, or a word standing where its block opens: the opener's line.
            if (isListed(first, k_pascalIndentClosers) || isListed(first, k_pascalIndentMiddles))
            {
                result.placesItself = true;
                const std::optional<Place> opener = openerBefore(first);
                result.column = opener.has_value() ? columnOf(opener->line) : 0;
                return result;
            }

            if (isWord(first, L"else"))
            {
                result.placesItself = true;
                if (previous.has_value() && isMarkAt(previous.value(), L";"))
                {
                    const std::optional<Place> opener = openerBefore(first);
                    if (opener.has_value() && isWord(opener.value(), L"case"))
                    {
                        result.column = columnOf(opener->line);
                        return result;
                    }
                }
                if (const std::optional<std::size_t> column = ifColumn(first))
                {
                    result.column = column.value();
                    return result;
                }
                result.column = previous.has_value() ? followingColumn(previous.value()) : 0;
                return result;
            }

            if (isMarkAt(first, L")") || isMarkAt(first, L"]"))
            {
                result.placesItself = true;
                const std::optional<Place> opener = bracketOpener(first);
                result.column = opener.has_value() ? columnOf(opener->line) : 0;
                return result;
            }

            const bool beginsBlock = isWord(first, L"begin");
            const bool closesSection = beginsBlock || isFilePart(first) || opensRoutine(first)
                || isListed(first, k_pascalIndentSections);
            if (!closesSection)
            {
                result.column = previous.has_value() ? followingColumn(previous.value()) : 0;
                return result;
            }

            result.placesItself = true;
            if (!previous.has_value() || isFilePart(first))
                return result;
            if (opensRoutine(first))
            {
                if (const std::optional<std::size_t> column = nestedRoutineColumn(first))
                {
                    result.column = column.value();
                    return result;
                }
            }
            // Under the statement a then, a do or an else leaves waiting for it, not past it.
            const bool afterHanger = isListed(previous.value(), k_pascalIndentHangers)
                || isWord(previous.value(), L"else");
            if (beginsBlock && afterHanger)
            {
                result.column = columnOf(statementStart(previous.value()).first.line);
                return result;
            }
            if (const std::optional<std::size_t> column = sectionColumn(first))
            {
                result.column = column.value();
                return result;
            }
            result.column = followingColumn(previous.value());
            return result;
        }

        SourceBlocks PascalIndent::blocks()
        {
            SourceBlocks result;
            std::vector<Place> open;
            for (std::size_t line = 0; line != m_lines.count(); ++line)
            {
                const std::size_t count = wordsOf(line).size();
                for (std::size_t index = 0; index != count; ++index)
                {
                    const Place place = { line, index };
                    if (isListed(place, k_pascalIndentClosers))
                    {
                        if (open.empty())
                            continue;
                        const Place opener = open.back();
                        open.pop_back();
                        if (opener.line != line)
                            result.push_back({ opener.line, line });
                        continue;
                    }
                    if (!opensAny(place))
                        continue;
                    // A record's variant part opens with a case the record's own end closes.
                    if (isWord(place, L"case") && !open.empty() && isWord(open.back(), L"record"))
                        continue;
                    open.push_back(place);
                }
            }
            return result;
        }

        const PascalWords& PascalIndent::wordsOf(const std::size_t line)
        {
            const LineWords::const_iterator read = m_words.find(line);
            if (read != m_words.end())
                return read->second;
            PascalWords& words = m_words[line];
            appendLineWords(m_lines.text(line), 0, m_lines.tokens(line), words);
            return words;
        }

        const PascalWord& PascalIndent::wordAt(const Place& place)
        {
            return wordsOf(place.line)[place.index];
        }

        std::optional<PascalIndent::Place> PascalIndent::before(const Place& place)
        {
            if (place.index != 0)
                return Place{ place.line, place.index - 1 };
            std::size_t line = place.line;
            while (line != 0)
            {
                --line;
                const PascalWords& words = wordsOf(line);
                if (!words.empty())
                    return Place{ line, words.size() - 1 };
            }
            return std::nullopt;
        }

        std::optional<PascalIndent::Place> PascalIndent::after(const Place& place)
        {
            if (place.index + 1 < wordsOf(place.line).size())
                return Place{ place.line, place.index + 1 };
            for (std::size_t line = place.line + 1; line < m_lines.count(); ++line)
            {
                if (!wordsOf(line).empty())
                    return Place{ line, 0 };
            }
            return std::nullopt;
        }

        std::size_t PascalIndent::columnOf(const std::size_t line) const
        {
            return indentColumns(m_lines.text(line));
        }

        bool PascalIndent::isWord(const Place& place, const std::wstring_view lower)
        {
            const PascalWord& word = wordAt(place);
            return word.kind == TokenKind::Keyword && spells(word, lower);
        }

        bool PascalIndent::isListed(const Place& place, const Words lowerWords)
        {
            const PascalWord& word = wordAt(place);
            return word.kind == TokenKind::Keyword && isListedNoCase(lowerWords, word.text);
        }

        bool PascalIndent::isMarkAt(const Place& place, const std::wstring_view mark)
        {
            return isMark(wordAt(place), mark);
        }

        bool PascalIndent::opensBlock(const Place& place)
        {
            return isListed(place, k_pascalIndentOpeners);
        }

        bool PascalIndent::opensBody(const Place& place)
        {
            if (!isListed(place, k_pascalBodyWords))
                return false;
            const std::optional<Place> previous = before(place);
            // A procedural type's `of object` opens nothing.
            if (isWord(place, L"object"))
                return !previous.has_value() || !isWord(previous.value(), L"of");
            // A type's declaration is the one place the word follows an equals sign; anywhere
            // else it is a member's own class word, or a unit's interface part.
            if (!previous.has_value() || !isMarkAt(previous.value(), L"="))
                return false;

            // Past the modifiers, a helper's subject, the parent list and an interface's GUID
            // stands the body's first word, or what says there is none: the semicolon of a
            // forward or a short declaration, a class reference's of.
            std::optional<Place> next = after(place);
            while (next.has_value() && (isWord(next.value(), L"abstract")
                || isWord(next.value(), L"sealed")))
            {
                next = after(next.value());
            }
            if (next.has_value() && isWord(next.value(), L"helper"))
            {
                next = after(next.value());
                if (next.has_value())
                    next = after(next.value());
                if (next.has_value())
                    next = after(next.value());
            }
            for (const std::wstring_view bracket : { L"(", L"[" })
            {
                if (!next.has_value() || !isMarkAt(next.value(), bracket))
                    continue;
                const std::wstring_view closer = bracket == L"(" ? L")" : L"]";
                while (next.has_value() && !isMarkAt(next.value(), closer))
                    next = after(next.value());
                if (next.has_value())
                    next = after(next.value());
            }
            if (!next.has_value())
                return true;
            return !isMarkAt(next.value(), L";") && !isWord(next.value(), L"of");
        }

        bool PascalIndent::opensAny(const Place& place)
        {
            return opensBlock(place) || opensBody(place);
        }

        bool PascalIndent::isCaseElse(const Place& place)
        {
            if (!isWord(place, L"else"))
                return false;
            const std::optional<Place> previous = before(place);
            return previous.has_value() && isMarkAt(previous.value(), L";");
        }

        bool PascalIndent::isMiddle(const Place& place)
        {
            return isListed(place, k_pascalIndentMiddles) || isCaseElse(place);
        }

        bool PascalIndent::isFilePart(const Place& place)
        {
            if (!isListed(place, k_pascalFileParts))
                return false;
            return !isWord(place, L"interface") || !opensBody(place);
        }

        bool PascalIndent::opensRoutine(const Place& place)
        {
            if (isWord(place, L"class"))
            {
                const std::optional<Place> next = after(place);
                return next.has_value() && next->line == place.line
                    && isListed(next.value(), k_pascalRoutineOpeners);
            }
            if (!isListed(place, k_pascalRoutineOpeners))
                return false;
            const std::optional<Place> previous = before(place);
            if (!previous.has_value())
                return true;
            for (const std::wstring_view typeMark : { L"=", L":", L":=", L"(", L"," })
            {
                if (isMarkAt(previous.value(), typeMark))
                    return false;
            }
            return !isWord(previous.value(), L"of") && !isWord(previous.value(), L"class")
                && !isWord(previous.value(), L"to");
        }

        bool PascalIndent::isStop(const Place& place)
        {
            return isMarkAt(place, L";") || isMarkAt(place, L"(") || isMarkAt(place, L"[")
                || opensAny(place) || isListed(place, k_pascalIndentSections)
                || isMiddle(place) || isFilePart(place);
        }

        std::optional<PascalIndent::Place> PascalIndent::openerBefore(const Place& place)
        {
            std::optional<Place> at = before(place);
            while (at.has_value())
            {
                if (isListed(at.value(), k_pascalIndentClosers))
                {
                    const std::optional<Place> opener = openerBefore(at.value());
                    if (!opener.has_value())
                        return std::nullopt;
                    at = before(opener.value());
                    continue;
                }
                if (isMarkAt(at.value(), L")") || isMarkAt(at.value(), L"]"))
                {
                    const std::optional<Place> bracket = bracketOpener(at.value());
                    if (!bracket.has_value())
                        return std::nullopt;
                    at = before(bracket.value());
                    continue;
                }
                if (opensAny(at.value()))
                {
                    // A record's variant part opens with a case the record's own end closes.
                    if (isWord(at.value(), L"case"))
                    {
                        const std::optional<Place> enclosing = openerBefore(at.value());
                        if (enclosing.has_value() && isWord(enclosing.value(), L"record"))
                            return enclosing;
                    }
                    return at;
                }
                at = before(at.value());
            }
            return std::nullopt;
        }

        std::optional<PascalIndent::Place> PascalIndent::bracketOpener(const Place& place)
        {
            std::size_t depth = 0;
            std::optional<Place> at = before(place);
            while (at.has_value())
            {
                if (isMarkAt(at.value(), L")") || isMarkAt(at.value(), L"]"))
                    ++depth;
                else if (isMarkAt(at.value(), L"(") || isMarkAt(at.value(), L"["))
                {
                    if (depth == 0)
                        return at;
                    --depth;
                }
                at = before(at.value());
            }
            return std::nullopt;
        }

        PascalIndent::StatementStart PascalIndent::statementStart(const Place& last)
        {
            StatementStart result{ .first = last };
            std::optional<Place> at = last;
            while (at.has_value())
            {
                if (at->line != last.line || at->index != last.index)
                {
                    if (isStop(at.value()))
                    {
                        result.stop = at;
                        return result;
                    }
                }
                std::optional<Place> groupStart;
                if (isListed(at.value(), k_pascalIndentClosers))
                    groupStart = openerBefore(at.value());
                else if (isMarkAt(at.value(), L")") || isMarkAt(at.value(), L"]"))
                    groupStart = bracketOpener(at.value());
                else
                    groupStart = at;
                if (!groupStart.has_value())
                    return result;
                result.first = groupStart.value();
                at = before(groupStart.value());
            }
            return result;
        }

        std::optional<std::size_t> PascalIndent::ifColumn(const Place& elseWord)
        {
            // The then first, whole blocks and brackets read past, and then its if.
            bool thenFound = false;
            std::optional<Place> at = before(elseWord);
            while (at.has_value())
            {
                if (isListed(at.value(), k_pascalIndentClosers))
                {
                    const std::optional<Place> opener = openerBefore(at.value());
                    if (!opener.has_value())
                        return std::nullopt;
                    at = before(opener.value());
                    continue;
                }
                if (isMarkAt(at.value(), L")") || isMarkAt(at.value(), L"]"))
                {
                    const std::optional<Place> bracket = bracketOpener(at.value());
                    if (!bracket.has_value())
                        return std::nullopt;
                    at = before(bracket.value());
                    continue;
                }
                if (thenFound ? isWord(at.value(), L"if") : isWord(at.value(), L"then"))
                {
                    if (thenFound)
                        return columnOf(at->line);
                    thenFound = true;
                }
                else if (isMarkAt(at.value(), L";") || opensAny(at.value()) || isMiddle(at.value()))
                    return std::nullopt;
                at = before(at.value());
            }
            return std::nullopt;
        }

        std::optional<std::size_t> PascalIndent::sectionColumn(const Place& place)
        {
            // A routine whose body has been read past is closed, and so are its sections: the
            // section the word closes stands before the routine's header. Each body read past
            // waits for its own header, nested routines' included.
            std::size_t bodies = 0;
            std::optional<Place> at = before(place);
            while (at.has_value())
            {
                if (isListed(at.value(), k_pascalIndentClosers))
                {
                    const std::optional<Place> opener = openerBefore(at.value());
                    if (!opener.has_value())
                        return 0;
                    if (isWord(opener.value(), L"begin") || isWord(opener.value(), L"asm"))
                        ++bodies;
                    at = before(opener.value());
                    continue;
                }
                if (isMarkAt(at.value(), L")") || isMarkAt(at.value(), L"]"))
                {
                    const std::optional<Place> bracket = bracketOpener(at.value());
                    if (!bracket.has_value())
                        return 0;
                    at = before(bracket.value());
                    continue;
                }
                if (bodies != 0 && opensRoutine(at.value()))
                {
                    --bodies;
                    at = before(at.value());
                    continue;
                }
                if (bodies == 0 && (isListed(at.value(), k_pascalIndentSections)
                    || opensRoutine(at.value()) || isFilePart(at.value())))
                {
                    return columnOf(at->line);
                }
                if (isFilePart(at.value()) || opensAny(at.value()) || isMiddle(at.value())
                    || isMarkAt(at.value(), L"(") || isMarkAt(at.value(), L"["))
                {
                    return std::nullopt;
                }
                at = before(at.value());
            }
            return 0;
        }

        std::optional<std::size_t> PascalIndent::nestedRoutineColumn(const Place& place)
        {
            // Each body read past - or a forward or an external, which says there is none - waits
            // for its own header. The header that settles the first of them is the routine this
            // one stands beside; a header with none waiting is a routine still open, its body to
            // come, and this one stands inside it.
            std::size_t bodies = 0;
            std::optional<Place> at = before(place);
            while (at.has_value())
            {
                if (isListed(at.value(), k_pascalIndentClosers))
                {
                    const std::optional<Place> opener = openerBefore(at.value());
                    if (!opener.has_value())
                        return std::nullopt;
                    if (isWord(opener.value(), L"begin") || isWord(opener.value(), L"asm"))
                        ++bodies;
                    at = before(opener.value());
                    continue;
                }
                if (isMarkAt(at.value(), L")") || isMarkAt(at.value(), L"]"))
                {
                    const std::optional<Place> bracket = bracketOpener(at.value());
                    if (!bracket.has_value())
                        return std::nullopt;
                    at = before(bracket.value());
                    continue;
                }
                if (isWord(at.value(), L"forward") || isWord(at.value(), L"external"))
                    ++bodies;
                else if (opensRoutine(at.value()))
                {
                    if (bodies == 1)
                        return columnOf(at->line);
                    if (bodies != 0)
                    {
                        --bodies;
                        at = before(at.value());
                        continue;
                    }
                    if (declaresOnly(at.value()))
                        return std::nullopt;
                    return columnOf(at->line) + m_width;
                }
                else if (isFilePart(at.value()) || opensAny(at.value()) || isMiddle(at.value()))
                    return std::nullopt;
                at = before(at.value());
            }
            return std::nullopt;
        }

        bool PascalIndent::declaresOnly(const Place& place)
        {
            // Back to what the header stands in: a type's body or a unit's interface part, where
            // headers declare, or a routine's body, which says the headers here have their own.
            std::optional<Place> at = before(place);
            while (at.has_value())
            {
                if (isFilePart(at.value()))
                    return isWord(at.value(), L"interface");
                if (opensAny(at.value()) || isMiddle(at.value()))
                    return true;
                if (isListed(at.value(), k_pascalIndentClosers))
                {
                    const std::optional<Place> opener = openerBefore(at.value());
                    if (!opener.has_value())
                        return false;
                    if (isWord(opener.value(), L"begin") || isWord(opener.value(), L"asm"))
                        return false;
                    at = before(opener.value());
                    continue;
                }
                at = before(at.value());
            }
            return false;
        }

        bool PascalIndent::closesAttribute(const Place& place)
        {
            if (!isMarkAt(place, L"]"))
                return false;
            const std::optional<Place> opener = bracketOpener(place);
            return opener.has_value() && opener->index == 0;
        }

        std::size_t PascalIndent::followingColumn(const Place& previous)
        {
            const std::size_t previousColumn = columnOf(previous.line);
            if (isListed(previous, k_pascalIndentHangers))
                return columnOf(statementStart(previous).first.line) + m_width;
            if (isWord(previous, L"else") || opensAny(previous) || isMiddle(previous)
                || isListed(previous, k_pascalIndentSections) || isWord(previous, L"of")
                || isMarkAt(previous, L"(") || isMarkAt(previous, L"["))
            {
                return previousColumn + m_width;
            }
            if (isFilePart(previous))
                return previousColumn;
            // A class's parent list ends the line that opens its body.
            if (isMarkAt(previous, L")"))
            {
                const std::optional<Place> bracket = bracketOpener(previous);
                const std::optional<Place> named = bracket.has_value()
                    ? before(bracket.value())
                    : std::nullopt;
                if (named.has_value() && opensBody(named.value()))
                    return columnOf(named->line) + m_width;
            }

            const StatementStart start = statementStart(previous);
            const std::size_t firstColumn = columnOf(start.first.line);
            const bool bracketed = start.stop.has_value()
                && (isMarkAt(start.stop.value(), L"(") || isMarkAt(start.stop.value(), L"["));
            const bool sameLine = start.stop.has_value()
                && start.stop->line == start.first.line;
            const bool complete = isMarkAt(previous, L";") || isMarkAt(previous, L",")
                || isListed(previous, k_pascalIndentClosers) || closesAttribute(previous);
            if (!complete)
            {
                // The statement goes on past the line: one level in from where it starts, or
                // the items of a bracket where the bracket opened a line before.
                if (bracketed && !sameLine)
                    return firstColumn;
                if (bracketed)
                    return columnOf(start.stop->line) + m_width;
                return firstColumn + m_width;
            }

            if (!start.stop.has_value())
                return firstColumn;
            const Place stop = start.stop.value();
            // A uses clause ends with its semicolon, and so does the section it is.
            if (isMarkAt(previous, L";") && isListed(stop, k_pascalOneEntrySections))
                return columnOf(stop.line);
            // A statement on the same line as the block or the section it opens stands one
            // level inside it, as the next one does.
            const bool opening = opensAny(stop) || isMiddle(stop) || bracketed
                || isListed(stop, k_pascalIndentSections);
            if (opening && sameLine)
                return columnOf(stop.line) + m_width;
            return firstColumn;
        }

        std::wstring_view PascalIndent::closerOf(const std::size_t line)
        {
            const PascalWords& words = wordsOf(line);
            if (words.empty())
                return {};
            const Place last = { line, words.size() - 1 };
            if (isListed(last, k_pascalCompletedOpeners) || opensBody(last))
                return k_pascalBlockCloser;
            if (isMarkAt(last, L")"))
            {
                const std::optional<Place> bracket = bracketOpener(last);
                const std::optional<Place> named = bracket.has_value()
                    ? before(bracket.value())
                    : std::nullopt;
                if (named.has_value() && opensBody(named.value()))
                    return k_pascalBlockCloser;
                return {};
            }
            if (!isWord(last, L"of"))
                return {};
            // A case's header - but not a record's variant part, which the record's end closes.
            for (std::size_t index = 0; index != words.size(); ++index)
            {
                const Place word = { line, index };
                if (!isWord(word, L"case"))
                    continue;
                const std::optional<Place> enclosing = openerBefore(word);
                if (enclosing.has_value() && isWord(enclosing.value(), L"record"))
                    return {};
                return k_pascalBlockCloser;
            }
            return {};
        }
    }

    bool pascalHook(Scan& scan)
    {
        switch (scan.state.mode)
        {
            case PascalMode::braceDirective:
                takeToCloser(scan, scan.index, L"}", TokenKind::Directive,
                    PascalMode::braceDirective);
                return true;
            case PascalMode::parenDirective:
                takeToCloser(scan, scan.index, L"*)", TokenKind::Directive,
                    PascalMode::parenDirective);
                return true;
            default:
                break;
        }

        const wchar_t current = scan.current();
        const wchar_t next = scan.peek();
        if (current == L'\'')
        {
            takePascalString(scan);
            return true;
        }
        if (current == L'#' && (isDigit(next) || next == L'$'))
        {
            takeCharCode(scan);
            return true;
        }
        if (current == L'$' && isHexDigit(next))
        {
            takeDigits(scan, scan.index + 1, isHexDigit, TokenKind::Number);
            return true;
        }
        if (current == L'%' && isBinaryDigit(next))
        {
            takeDigits(scan, scan.index + 1, isBinaryDigit, TokenKind::Number);
            return true;
        }

        // A compiler directive is a comment whose first character is a dollar, closed the way the
        // comment is. The comments without one are the table's.
        const std::size_t start = scan.index;
        if (current == L'{' && next == L'$')
        {
            scan.index += 2;
            takeToCloser(scan, start, L"}", TokenKind::Directive, PascalMode::braceDirective);
            return true;
        }
        if (current == L'(' && next == L'*' && scan.peek(2) == L'$')
        {
            scan.index += 3;
            takeToCloser(scan, start, L"*)", TokenKind::Directive, PascalMode::parenDirective);
            return true;
        }

        // An ampersand makes a name of a reserved word, and the name is as plain as any.
        if (current == L'&' && isNameStart(next, scan.language))
        {
            scan.index = endOfName(scan.line, start + 1, scan.language);
            return true;
        }
        if (isNameStart(current, scan.language))
            return takeContextualDirective(scan);
        return false;
    }

    // A file's header, a compiler directive, or a program closed with `end.` past a begin: no
    // other language here spells them. A block with an assignment or a routine in it is the
    // shape of a script, which a text of another language could share.
    Claim pascalDetector(const std::wstring_view text)
    {
        const PascalSigns signs = readPascalSigns(text);
        const bool closesProgram = signs.block && endsWithNoCase(trimmed(text), L"end.");
        if (signs.header || signs.directive || closesProgram)
            return Claim::Certain;

        const bool assigns = text.find(L":=") != std::wstring_view::npos;
        if (signs.block && (signs.routine || assigns))
            return Claim::Likely;
        return Claim::None;
    }

    Declarations pascalDeclarations(const std::wstring_view text)
    {
        const PascalWords words = pascalWords(text);
        PascalReader reader{ text, words };
        return reader.read();
    }

    LineIndent pascalIndent(const SourceLines& lines, const std::size_t line,
        const std::size_t width)
    {
        PascalIndent placing{ lines, width };
        return placing.place(line);
    }

    SourceBlocks pascalBlocks(const SourceLines& lines)
    {
        PascalIndent reading{ lines, 0 };
        return reading.blocks();
    }
}
