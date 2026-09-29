module ClaFi.Core.Syntax.Languages;

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
}
