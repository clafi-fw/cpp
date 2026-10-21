module SeeDocs_App.Edits;

import SeeDocs_App.Notes;
import SeeDocs_App.Reading;
import SeeDocs_App.Surface;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        constexpr std::string_view k_byteOrderMark = "\xEF\xBB\xBF";
        constexpr std::wstring_view k_noteExtension = L".md";
        constexpr std::wstring_view k_lineComment = L"//";
        constexpr std::wstring_view k_blockComment = L"/*";
        constexpr std::wstring_view k_blockCommentEnd = L"*/";
        constexpr std::wstring_view k_braceComment = L"{";
        constexpr std::wstring_view k_braceCommentEnd = L"}";
        constexpr std::wstring_view k_parenComment = L"(*";
        constexpr std::wstring_view k_parenCommentEnd = L"*)";
        constexpr std::wstring_view k_seeWord = L"See";
        constexpr std::wstring_view k_todo = L"TODO";
        constexpr std::wstring_view k_templateWord = L"template";
        constexpr std::wstring_view k_codeFence = L"    ";
        constexpr std::wstring_view k_bulletMark = L"- ";
        constexpr std::wstring_view k_bulletFollow = L"  ";
        constexpr std::size_t k_tabColumns = 4;
        constexpr std::size_t k_nameReach = 3;      // the lines a declaration's name is looked on
        constexpr std::size_t k_prefixReach = 12;   // the lines a template head is looked up over
        constexpr std::size_t k_footnoteLevel = 2;  // the level a new footnote's heading takes
        constexpr wchar_t k_noteJoin = L'-';        // between the words of a note's stem

        // Files

        // One line of a text file and the end it had.
        struct FileLine
        {
            std::wstring text;
            bool carriageReturn{ false };   // ended in CR LF rather than LF
        };

        // A text file as its lines, with the byte order mark it opened with.
        struct FileLines
        {
            std::vector<FileLine> lines;
            bool byteOrderMark{ false };
            bool endsWithLineEnd{ true };
            bool utf8{ true };   // the bytes read as UTF-8, so writing them gives them back
        };

        [[nodiscard]] std::optional<FileLines> readLines(const std::filesystem::path& path)
        {
            std::ifstream file{ path, std::ios::binary };
            if (!file)
                return std::nullopt;
            using ByteIterator = std::istreambuf_iterator<char>;
            std::string bytes{ ByteIterator{ file }, ByteIterator{} };
            FileLines result;
            if (bytes.starts_with(k_byteOrderMark))
            {
                result.byteOrderMark = true;
                bytes.erase(0, k_byteOrderMark.size());
            }
            const std::wstring text = fromUtf8(bytes);
            result.utf8 = toUtf8(text) == bytes;
            std::size_t start = 0;
            while (start < text.size())
            {
                const std::size_t end = text.find(L'\n', start);
                if (end == std::wstring::npos)
                {
                    result.lines.push_back({ .text = text.substr(start) });
                    result.endsWithLineEnd = false;
                    break;
                }
                FileLine line{ .text = text.substr(start, end - start) };
                if (line.text.ends_with(L'\r'))
                {
                    line.text.pop_back();
                    line.carriageReturn = true;
                }
                result.lines.push_back(std::move(line));
                start = end + 1;
            }
            return result;
        }

        [[nodiscard]] bool writeLines(const std::filesystem::path& path, const FileLines& file)
        {
            std::wstring text;
            for (std::size_t i = 0; i != file.lines.size(); ++i)
            {
                text += file.lines[i].text;
                const bool last = i + 1 == file.lines.size();
                if (last && !file.endsWithLineEnd)
                    break;
                if (file.lines[i].carriageReturn)
                    text += L'\r';
                text += L'\n';
            }
            std::string bytes = toUtf8(text);
            if (file.byteOrderMark)
                bytes.insert(0, k_byteOrderMark);
            std::ofstream out{ path, std::ios::binary | std::ios::trunc };
            if (!out)
                return false;
            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            return static_cast<bool>(out);
        }

        // A line put in beside another takes that line's end.
        [[nodiscard]] FileLine lineBeside(const FileLines& file, const std::size_t index,
            std::wstring text)
        {
            bool carriageReturn = false;
            if (!file.lines.empty())
                carriageReturn = file.lines[std::min(index, file.lines.size() - 1)].carriageReturn;
            return { .text = std::move(text), .carriageReturn = carriageReturn };
        }

        [[nodiscard]] std::size_t columnsOf(const std::wstring_view line)
        {
            std::size_t columns = 0;
            for (const wchar_t current : line)
                columns += current == L'\t' ? k_tabColumns : 1;
            return columns;
        }

        [[nodiscard]] bool isBlank(const std::wstring_view line)
        {
            return trimmed(line).empty();
        }

        [[nodiscard]] std::wstring indentText(const std::wstring_view line)
        {
            const std::size_t end = line.find_first_not_of(L" \t");
            return std::wstring{ line.substr(0, std::min(end, line.size())) };
        }

        [[nodiscard]] std::wstring withoutTrailingBlanks(std::wstring_view text)
        {
            while (!text.empty() && (text.back() == L' ' || text.back() == L'\t'))
                text.remove_suffix(1);
            return std::wstring{ text };
        }

        [[nodiscard]] Lines textsOf(const std::vector<FileLine>& lines, const std::size_t start,
            const std::size_t end)
        {
            Lines texts;
            for (std::size_t i = start; i < end; ++i)
                texts.push_back(lines[i].text);
            return texts;
        }

        void trimBlankLines(Lines& lines)
        {
            while (!lines.empty() && isBlank(lines.back()))
                lines.pop_back();
            std::size_t leading = 0;
            while (leading < lines.size() && isBlank(lines[leading]))
                ++leading;
            lines.erase(lines.begin(), lines.begin() + static_cast<std::ptrdiff_t>(leading));
        }

        [[nodiscard]] Lines splitLines(const std::wstring_view text)
        {
            Lines lines;
            std::size_t start = 0;
            while (start <= text.size())
            {
                std::size_t end = text.find(L'\n', start);
                if (end == std::wstring_view::npos)
                    end = text.size();
                std::wstring_view line = text.substr(start, end - start);
                if (line.ends_with(L'\r'))
                    line.remove_suffix(1);
                lines.emplace_back(line);
                start = end + 1;
            }
            return lines;
        }

        // Footnotes

        enum class NoteBlockKind
        {
            Blank,
            Heading,
            Code,
            Bullet,
            Paragraph
        };

        // A block of a note's lines: what kind, its words on one line, and the lines it was.
        struct NoteBlock
        {
            NoteBlockKind kind{ NoteBlockKind::Blank };
            std::wstring flowed;
            Lines lines;
            std::size_t first{ 0 };   // the first of the lines read, counted in all of them
        };

        using NoteBlocks = std::vector<NoteBlock>;

        [[nodiscard]] bool isCodeLine(const std::wstring_view line)
        {
            return line.starts_with(k_codeFence) && !isBlank(line);
        }

        [[nodiscard]] bool isBulletLine(const std::wstring_view line)
        {
            return line.starts_with(k_bulletMark);
        }

        // The lines cut into blocks the way a page reads them: a paragraph runs to a blank line,
        // a heading or a bullet, a bullet takes the lines set in under it, and a code line stands
        // alone.
        [[nodiscard]] NoteBlocks blocksOf(const Lines& lines)
        {
            NoteBlocks blocks;
            std::size_t i = 0;
            while (i < lines.size())
            {
                const std::wstring& line = lines[i];
                if (isBlank(line))
                {
                    NoteBlock blank;
                    blank.lines.push_back(line);
                    blank.first = i;
                    blocks.push_back(std::move(blank));
                    ++i;
                    continue;
                }
                if (headingLevel(line) != 0 || isCodeLine(line))
                {
                    const NoteBlockKind kind = headingLevel(line) != 0
                        ? NoteBlockKind::Heading
                        : NoteBlockKind::Code;
                    blocks.push_back({
                        .kind = kind,
                        .flowed = line,
                        .lines = { line },
                        .first = i
                    });
                    ++i;
                    continue;
                }
                if (isBulletLine(line))
                {
                    NoteBlock block{
                        .kind = NoteBlockKind::Bullet,
                        .flowed = std::wstring{ k_bulletMark }
                            + std::wstring{ trimmed(line.substr(k_bulletMark.size())) },
                        .lines = { line },
                        .first = i
                    };
                    ++i;
                    while (i < lines.size() && lines[i].starts_with(k_bulletFollow)
                        && !isBlank(lines[i]))
                    {
                        block.flowed += L' ';
                        block.flowed += trimmed(lines[i]);
                        block.lines.push_back(lines[i]);
                        ++i;
                    }
                    blocks.push_back(std::move(block));
                    continue;
                }
                NoteBlock block{
                    .kind = NoteBlockKind::Paragraph,
                    .flowed = std::wstring{ trimmed(line) },
                    .lines = { line },
                    .first = i
                };
                ++i;
                while (i < lines.size() && !isBlank(lines[i]) && headingLevel(lines[i]) == 0
                    && !isBulletLine(lines[i]))
                {
                    block.flowed += L' ';
                    block.flowed += trimmed(lines[i]);
                    block.lines.push_back(lines[i]);
                    ++i;
                }
                blocks.push_back(std::move(block));
            }
            return blocks;
        }

        // Whether a word opening a line would read as markup there - a bullet's dash, a heading's
        // marks, a TODO, a doc command or tag.
        [[nodiscard]] bool opensMarkup(const std::wstring_view word)
        {
            if (word == L"-" || word.starts_with(k_todo) || opensPart(word, HintRules::Loose))
                return true;
            return !word.empty() && word.find_first_not_of(L'#') == std::wstring_view::npos;
        }

        // The words broken into lines no wider than the line width, the first line opening with
        // its prefix and every other with the rest prefix. A word that would open a line as
        // markup goes down with the word before it.
        [[nodiscard]] Lines wrapped(const std::wstring_view words, const std::wstring_view first,
            const std::wstring_view rest)
        {
            std::vector<std::wstring_view> pieces;
            std::size_t start = 0;
            while (start < words.size())
            {
                const std::size_t space = words.find(L' ', start);
                const std::size_t end = space == std::wstring_view::npos ? words.size() : space;
                if (end != start)
                    pieces.push_back(words.substr(start, end - start));
                start = end + 1;
            }
            // A See ending the words is one piece: a reference stands on one line.
            if (pieces.size() >= 2 && pieces[pieces.size() - 2] == k_seeWord)
            {
                const std::wstring_view see = pieces[pieces.size() - 2];
                const std::wstring_view stem = pieces.back();
                pieces.pop_back();
                pieces.back() = std::wstring_view{ see.data(),
                    static_cast<std::size_t>(stem.data() + stem.size() - see.data()) };
            }

            Lines result;
            std::vector<std::wstring_view> line;
            const auto columnsWith = [&](const std::wstring_view word) {
                std::size_t columns = columnsOf(result.empty() ? first : rest);
                for (const std::wstring_view piece : line)
                    columns += columnsOf(piece) + 1;
                return columns + columnsOf(word);
            };
            const auto flush = [&]() {
                std::wstring text{ result.empty() ? first : rest };
                for (std::size_t i = 0; i != line.size(); ++i)
                {
                    if (i != 0)
                        text += L' ';
                    text += line[i];
                }
                result.push_back(std::move(text));
                line.clear();
            };
            for (const std::wstring_view piece : pieces)
            {
                if (line.empty() || columnsWith(piece) <= k_lineColumns)
                {
                    line.push_back(piece);
                    continue;
                }
                if (opensMarkup(piece) && line.size() > 1)
                {
                    const std::wstring_view carried = line.back();
                    line.pop_back();
                    flush();
                    line.push_back(carried);
                }
                else
                    flush();
                line.push_back(piece);
            }
            if (!line.empty())
                flush();
            return result;
        }

        // The edited words as a section's body. A paragraph or a bullet that reads as one of the
        // original's did takes that one's lines; any other is wrapped.
        [[nodiscard]] Lines bodyLines(const std::wstring_view flowed, const Lines& original)
        {
            std::map<std::wstring, std::deque<Lines>> kept;
            for (NoteBlock& block : blocksOf(original))
            {
                if (block.kind == NoteBlockKind::Paragraph || block.kind == NoteBlockKind::Bullet)
                    kept[block.flowed].push_back(std::move(block.lines));
            }
            Lines result;
            for (const NoteBlock& block : blocksOf(splitLines(flowed)))
            {
                switch (block.kind)
                {
                    case NoteBlockKind::Blank:
                        result.emplace_back();
                        break;
                    case NoteBlockKind::Heading:
                    case NoteBlockKind::Code:
                        result.push_back(withoutTrailingBlanks(block.lines.front()));
                        break;
                    case NoteBlockKind::Bullet:
                    case NoteBlockKind::Paragraph:
                    {
                        const auto found = kept.find(block.flowed);
                        if (found != kept.end() && !found->second.empty())
                        {
                            for (const std::wstring& line : found->second.front())
                                result.push_back(line);
                            found->second.pop_front();
                            break;
                        }
                        const bool bullet = block.kind == NoteBlockKind::Bullet;
                        const std::wstring_view words = bullet
                            ? std::wstring_view{ block.flowed }.substr(k_bulletMark.size())
                            : std::wstring_view{ block.flowed };
                        const std::wstring_view none{};
                        const std::wstring_view first = bullet ? k_bulletMark : none;
                        const std::wstring_view rest = bullet ? k_bulletFollow : none;
                        for (std::wstring& line : wrapped(words, first, rest))
                        {
                            result.push_back(std::move(line));
                        }
                        break;
                    }
                }
            }
            trimBlankLines(result);
            return result;
        }

        // Refuses a body a page could not read back as one section of that level.
        [[nodiscard]] Refusal checkBody(const Lines& body, const std::size_t level)
        {
            if (body.empty())
                return L"A footnote holds words. Write some, or press Escape to leave it.";
            for (const std::wstring& line : body)
            {
                const std::size_t inner = headingLevel(line);
                if (inner != 0 && inner <= level)
                {
                    return L"A heading of " + std::wstring(inner, L'#')
                        + L" would end the footnote. Headings inside it take "
                        + std::wstring(level + 1, L'#') + L" or more.";
                }
            }
            return {};
        }

        // Where a section stands in a note's lines: its heading, the line after its end, and its
        // level - the first section that anchor reaches, as the notes find it.
        struct SectionRange
        {
            std::size_t heading{ 0 };
            std::size_t end{ 0 };
            std::size_t level{ 0 };
        };

        [[nodiscard]] std::optional<SectionRange> sectionRange(const FileLines& file,
            const std::wstring_view anchor, const std::size_t endLevel = 0)
        {
            for (std::size_t i = 0; i != file.lines.size(); ++i)
            {
                const std::wstring_view text = file.lines[i].text;
                const std::size_t level = sectionLevel(text);
                if (level == 0 || slug(trimmed(text.substr(level))) != anchor)
                    continue;
                const std::size_t closing = endLevel == 0 ? level : std::min(level, endLevel);
                SectionRange range{ .heading = i, .end = file.lines.size(), .level = level };
                for (std::size_t j = i + 1; j != file.lines.size(); ++j)
                {
                    const std::size_t inner = headingLevel(file.lines[j].text);
                    if (inner != 0 && inner <= closing)
                    {
                        range.end = j;
                        break;
                    }
                }
                return range;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::filesystem::path notePath(const Notes& notes,
            const std::wstring_view note)
        {
            return notes.folder() / (std::wstring{ note } + std::wstring{ k_noteExtension });
        }

        [[nodiscard]] std::wstring noteFileName(const std::wstring_view note)
        {
            return std::wstring{ note } + std::wstring{ k_noteExtension };
        }

        // A note's lines before any section: its title and the blank under it.
        [[nodiscard]] FileLines newNote(Notes& notes, const std::wstring_view note)
        {
            bool carriageReturn = false;
            if (!notes.stems().empty())
            {
                const std::optional<FileLines> first = readLines(notePath(notes,
                    notes.stems().front()));
                carriageReturn = first && !first->lines.empty()
                    && first->lines.front().carriageReturn;
            }
            std::wstring title{ note };
            std::ranges::replace(title, k_noteJoin, L' ');
            FileLines file;
            file.lines.push_back({ .text = L"# " + title, .carriageReturn = carriageReturn });
            file.lines.push_back({ .text = {}, .carriageReturn = carriageReturn });
            return file;
        }

        // Comments in a source file

        enum class LineKind
        {
            Blank,
            CommentOnly,
            Code
        };

        // What a line of a source file is to a reader of its comments.
        struct SourceLine
        {
            LineKind kind{ LineKind::Blank };
            std::size_t trailing{ std::wstring::npos };   // where a comment ending its code opens
            bool directive{ false };                      // a compiler directive, no code
            bool carried{ false };                        // inside a block a line above opened
        };

        using SourceLines = std::vector<SourceLine>;

        // A run of a line, as far as its comments go.
        struct LineToken
        {
            std::size_t start{ 0 };
            bool comment{ false };
            bool lineComment{ false };   // opens with //
            bool blockOpener{ false };   // opens a C++ block comment
            bool directive{ false };
        };

        using LineTokens = std::vector<LineToken>;

        // **A comment line is one holding comments alone**: by strict C++ rules a // comment, by
        // loose rules and in Delphi any, a blank line inside a block included. A comment ending a
        // line of code is the line's last run - in C++ one that is not its first, opening with //,
        // or by loose rules with /*.
        [[nodiscard]] SourceLine sourceLineOf(const LineTokens& tokens, const bool insideComment,
            const SourceLanguage language, const HintRules rules)
        {
            const bool everyComment = language == SourceLanguage::Pascal
                || rules == HintRules::Loose;
            SourceLine line{ .carried = insideComment };
            if (tokens.empty())
            {
                line.kind = insideComment && everyComment ? LineKind::CommentOnly : LineKind::Blank;
                return line;
            }
            const bool commentsOnly = std::ranges::all_of(tokens, [](const LineToken& token) {
                return token.comment;
            });
            const bool counts = tokens.front().lineComment || everyComment;
            line.kind = commentsOnly && counts ? LineKind::CommentOnly : LineKind::Code;
            const bool anyDirective = std::ranges::any_of(tokens, [](const LineToken& token) {
                return token.directive;
            });
            const bool noCode = std::ranges::all_of(tokens, [](const LineToken& token) {
                return token.comment || token.directive;
            });
            line.directive = anyDirective && noCode;
            if (line.kind != LineKind::Code)
                return line;
            const LineToken& tail = tokens.back();
            if (!tail.comment)
                return line;
            if (language == SourceLanguage::Pascal)
            {
                line.trailing = tail.start;
                return line;
            }
            const bool opens = tail.lineComment
                || (rules == HintRules::Loose && tail.blockOpener);
            if (opens && tokens.size() > 1)
                line.trailing = tail.start;
            return line;
        }

        // Whether a quote stands inside a number, separating its digits - 1'000, 0xFF'66.
        [[nodiscard]] bool separatesDigits(const std::wstring_view text, const std::size_t at)
        {
            std::size_t start = at;
            while (start != 0 && (std::iswalnum(text[start - 1]) != 0 || text[start - 1] == L'\''))
                --start;
            return start != at && std::iswdigit(text[start]) != 0;
        }

        // Where a C++ string or character literal opening at a quote ends, past its closing quote.
        [[nodiscard]] std::size_t literalEnd(const std::wstring_view text, const std::size_t at)
        {
            const wchar_t quote = text[at];
            std::size_t i = at + 1;
            while (i < text.size() && text[i] != quote)
                i += text[i] == L'\\' ? 2 : 1;
            return std::min(i + 1, text.size());
        }

        [[nodiscard]] SourceLines cppLines(const FileLines& file, const HintRules rules)
        {
            SourceLines result;
            bool inBlock = false;
            for (const FileLine& fileLine : file.lines)
            {
                const std::wstring_view text = fileLine.text;
                LineTokens tokens;
                const bool insideComment = inBlock;
                std::size_t i = 0;
                if (inBlock)
                {
                    const std::size_t close = text.find(k_blockCommentEnd);
                    if (close != std::wstring_view::npos || !isBlank(text))
                        tokens.push_back({ .start = 0, .comment = true });
                    if (close == std::wstring_view::npos)
                    {
                        i = text.size();
                    }
                    else
                    {
                        i = close + k_blockCommentEnd.size();
                        inBlock = false;
                    }
                }
                while (i < text.size())
                {
                    const wchar_t current = text[i];
                    if (current == L' ' || current == L'\t')
                    {
                        ++i;
                        continue;
                    }
                    const std::wstring_view rest = text.substr(i);
                    if (rest.starts_with(k_lineComment))
                    {
                        tokens.push_back({ .start = i, .comment = true, .lineComment = true });
                        break;
                    }
                    if (rest.starts_with(k_blockComment))
                    {
                        tokens.push_back({ .start = i, .comment = true, .blockOpener = true });
                        const std::size_t close = text.find(k_blockCommentEnd,
                            i + k_blockComment.size());
                        if (close == std::wstring_view::npos)
                        {
                            inBlock = true;
                            break;
                        }
                        i = close + k_blockCommentEnd.size();
                        continue;
                    }
                    const bool separator = current == L'\'' && separatesDigits(text, i);
                    tokens.push_back({ .start = i });
                    if ((current == L'"' || current == L'\'') && !separator)
                        i = literalEnd(text, i);
                    else
                        ++i;
                }
                result.push_back(sourceLineOf(tokens, insideComment, SourceLanguage::Cpp, rules));
            }
            return result;
        }

        // A Delphi string runs between single quotes, two of them standing for one.
        [[nodiscard]] std::size_t pascalStringEnd(const std::wstring_view text,
            const std::size_t at)
        {
            std::size_t i = at + 1;
            while (i < text.size())
            {
                if (text[i] != L'\'')
                {
                    ++i;
                    continue;
                }
                if (i + 1 < text.size() && text[i + 1] == L'\'')
                {
                    i += 2;
                    continue;
                }
                return i + 1;
            }
            return text.size();
        }

        [[nodiscard]] SourceLines pascalLines(const FileLines& file)
        {
            SourceLines result;
            std::wstring_view closer;   // closes the comment a line ended inside
            for (const FileLine& fileLine : file.lines)
            {
                const std::wstring_view text = fileLine.text;
                LineTokens tokens;
                const bool insideComment = !closer.empty();
                std::size_t i = 0;
                if (insideComment)
                {
                    const std::size_t close = text.find(closer);
                    if (close != std::wstring_view::npos || !isBlank(text))
                        tokens.push_back({ .start = 0, .comment = true });
                    if (close == std::wstring_view::npos)
                    {
                        i = text.size();
                    }
                    else
                    {
                        i = close + closer.size();
                        closer = {};
                    }
                }
                while (i < text.size())
                {
                    const wchar_t current = text[i];
                    if (current == L' ' || current == L'\t')
                    {
                        ++i;
                        continue;
                    }
                    const std::wstring_view rest = text.substr(i);
                    if (rest.starts_with(k_lineComment))
                    {
                        tokens.push_back({ .start = i, .comment = true, .lineComment = true });
                        break;
                    }
                    const bool brace = rest.starts_with(k_braceComment);
                    const bool paren = rest.starts_with(k_parenComment);
                    if (brace || paren)
                    {
                        const std::wstring_view opener = brace ? k_braceComment : k_parenComment;
                        const std::wstring_view ending = brace
                            ? k_braceCommentEnd
                            : k_parenCommentEnd;
                        const bool directive = rest.substr(opener.size()).starts_with(L'$');
                        tokens.push_back({
                            .start = i,
                            .comment = !directive,
                            .directive = directive
                        });
                        const std::size_t close = text.find(ending, i + opener.size());
                        if (close == std::wstring_view::npos)
                        {
                            if (!directive)
                                closer = ending;
                            break;
                        }
                        i = close + ending.size();
                        continue;
                    }
                    tokens.push_back({ .start = i });
                    i = current == L'\'' ? pascalStringEnd(text, i) : i + 1;
                }
                result.push_back(sourceLineOf(tokens, insideComment, SourceLanguage::Pascal,
                    HintRules::Strict));
            }
            return result;
        }

        [[nodiscard]] SourceLines sourceLinesOf(const FileLines& file,
            const SourceLanguage language, const HintRules rules)
        {
            return language == SourceLanguage::Pascal ? pascalLines(file) : cppLines(file, rules);
        }

        // The comment lines under a line that carry on the comment ending it: those opening in its
        // column.
        [[nodiscard]] std::size_t continuationsOf(const FileLines& file, const SourceLines& lines,
            const std::size_t line)
        {
            if (lines[line].trailing == std::wstring::npos)
                return 0;
            const std::size_t column = columnOf(file.lines[line].text, lines[line].trailing);
            std::size_t count = 0;
            for (std::size_t next = line + 1; next < lines.size(); ++next)
            {
                if (lines[next].kind != LineKind::CommentOnly
                    || indentOf(file.lines[next].text) != column)
                {
                    break;
                }
                ++count;
            }
            return count;
        }

        // The first of the comment lines touching the line from above that are its own, the
        // lines carrying on a comment that ends a line above them passed over - and the rest of a
        // block a line of code opened; the line where none.
        [[nodiscard]] std::size_t aboveStart(const FileLines& file, const SourceLines& lines,
            const std::size_t line)
        {
            std::size_t first = line;
            while (first != 0 && lines[first - 1].kind == LineKind::CommentOnly)
                --first;
            if (first != 0 && first != line)
                first = std::min(line, first + continuationsOf(file, lines, first - 1));
            while (first != line && lines[first].carried)
                ++first;
            return first;
        }

        // Whether a line opens a template head, after the words export or a macro sets before it.
        [[nodiscard]] bool opensTemplateHead(const std::wstring_view line)
        {
            std::wstring_view words = trimmed(line);
            while (!words.empty())
            {
                if (words.starts_with(k_templateWord))
                {
                    const std::wstring_view rest = trimmed(words.substr(k_templateWord.size()));
                    if (rest.starts_with(L'<'))
                        return true;
                }
                std::size_t end = 0;
                while (end < words.size() && (std::iswalnum(words[end]) != 0 || words[end] == L'_'))
                    ++end;
                if (end == 0)
                    return false;
                words = trimmed(words.substr(end));
            }
            return false;
        }

        // Whether a line can stand in a declaration's prefix above it - a template head, its
        // requires clause, a head running on - rather than end a statement of its own.
        [[nodiscard]] bool continuesPrefix(const std::wstring_view line)
        {
            const std::wstring_view words = trimmed(line);
            return !words.empty() && !words.starts_with(k_lineComment) && !words.ends_with(L';')
                && !words.ends_with(L'{') && !words.ends_with(L'}');
        }

        // The line a declaration's template head opens on, the head and its requires clause
        // running down to the declaration; the declaration's own line where it has none.
        [[nodiscard]] std::size_t prefixStart(const FileLines& file, const std::size_t line)
        {
            std::size_t at = line;
            while (at != 0 && line - at < k_prefixReach
                && continuesPrefix(file.lines[at - 1].text))
            {
                --at;
                if (opensTemplateHead(file.lines[at].text))
                    return at;
            }
            return line;
        }

        // A line holding a Delphi attribute alone - [Weak], [Ref] - which a declaration under it
        // reads its comments from.
        [[nodiscard]] bool isAttributeLine(const FileLines& file, const SourceLines& lines,
            const std::size_t line)
        {
            const std::wstring_view text = file.lines[line].text;
            const std::wstring_view words = trimmed(text.substr(0, lines[line].trailing));
            return words.starts_with(L'[') && words.ends_with(L']');
        }

        // Whether the name stands in the text, blanks aside - operator == is operator==.
        [[nodiscard]] bool names(const std::wstring_view text, const std::wstring_view name)
        {
            std::wstring packed;
            for (const wchar_t current : text)
            {
                if (current != L' ' && current != L'\t')
                    packed += current;
            }
            std::wstring packedName;
            for (const wchar_t current : name)
            {
                if (current != L' ' && current != L'\t')
                    packedName += current;
            }
            return packed.find(packedName) != std::wstring::npos;
        }

        [[nodiscard]] bool sameWordIgnoringCase(const std::wstring_view left,
            const std::wstring_view right)
        {
            return left.size() == right.size() && std::ranges::equal(left, right,
                [](const wchar_t a, const wchar_t b) {
                    return std::towlower(a) == std::towlower(b);
                });
        }

        // Where a comment the hint is read from stands to its declaration.
        enum class CommentPlace
        {
            None,       // the declaration has none
            Trailing,   // on the declaration's line, with the lines carrying it on
            Above,      // the comment lines touching the declaration from above
            Body        // under a Delphi routine's header in the implementation
        };

        // A comment as the file holds it: where it stands, its lines cut, and all it says.
        struct FoundComment
        {
            CommentPlace place{ CommentPlace::None };
            std::size_t first{ 0 };   // the file's line it starts on
            CommentLines lines;
            std::wstring words;       // every word, a See included
        };

        [[nodiscard]] FoundComment cutFound(const FileLines& file, const CommentPlace place,
            const std::size_t first, const std::size_t end, const std::size_t start,
            const SourceLanguage language, const HintRules rules)
        {
            std::vector<std::wstring_view> views;
            for (std::size_t i = first; i != end; ++i)
                views.push_back(file.lines[i].text);
            FoundComment found{ .place = place, .first = first };
            found.lines = cutComment(views, start, language, rules);
            found.words = commentText(found.lines);
            return found;
        }

        // **A Delphi routine's body comment** runs from its first comment line down over blank
        // lines and directives to its last comment line; those lines part its paragraphs.
        [[nodiscard]] std::optional<FoundComment> bodyComment(const FileLines& file,
            const SourceLines& lines, const std::size_t from)
        {
            if (from >= lines.size() || lines[from].kind != LineKind::CommentOnly)
                return std::nullopt;
            std::size_t last = from;
            for (std::size_t i = from; i < lines.size(); ++i)
            {
                const SourceLine& line = lines[i];
                if (line.kind == LineKind::CommentOnly)
                    last = i;
                else if (line.kind != LineKind::Blank && !line.directive)
                    break;
            }
            FoundComment found{ .place = CommentPlace::Body, .first = from };
            std::size_t at = from;
            while (at <= last)
            {
                if (lines[at].kind != LineKind::CommentOnly)
                {
                    found.lines.push_back({
                        .lead = file.lines[at].text,
                        .words = {},
                        .tail = {},
                        .speaks = false,
                        .opensParagraph = true
                    });
                    ++at;
                    continue;
                }
                std::size_t end = at;
                while (end <= last && lines[end].kind == LineKind::CommentOnly)
                    ++end;
                const FoundComment run = cutFound(file, CommentPlace::Body, at, end,
                    leadingBlanks(file.lines[at].text), SourceLanguage::Pascal, HintRules::Loose);
                found.lines.insert(found.lines.end(), run.lines.begin(), run.lines.end());
                at = end;
            }
            found.words = commentText(found.lines);
            return found;
        }

        // A declaration as the file holds it, read the way the readers read it.
        struct FoundDeclaration
        {
            std::size_t line{ 0 };      // where its comment on the line is read - its attributes'
            std::size_t head{ 0 };      // the line a comment above it stands over
            std::wstring code;          // the line's code, the comment and the blanks before it off
            FoundComment comment;       // the one the hint is read from
            bool aboveTaken{ false };   // a comment above it says other words than the hint
            bool attributed{ false };   // Delphi attributes stand on the lines over it
        };

        // **Strict rules read the comment on the line first, loose ones the comment above** when
        // it says any words - as the readers do. A C++ declaration with no comment right above it
        // reads the one above its template head.
        [[nodiscard]] std::optional<FoundDeclaration> locate(const FileLines& file,
            const SourceLines& lines, const CommentSite& site, const SourceLanguage language,
            const HintRules rules)
        {
            if (site.place.line == 0 || site.place.line > file.lines.size())
                return std::nullopt;
            const std::size_t declared = site.place.line - 1;
            const std::wstring_view name = withoutArguments(bareName(site.name));
            const std::size_t reach = std::min(declared + k_nameReach, file.lines.size());
            bool named = false;
            for (std::size_t i = declared; i < reach && !named; ++i)
                named = names(file.lines[i].text, name);
            if (!named)
                return std::nullopt;

            FoundDeclaration found;
            std::size_t line = declared;
            if (language == SourceLanguage::Pascal)
            {
                while (line != 0 && isAttributeLine(file, lines, line - 1))
                    --line;
            }
            found.line = line;
            found.head = line;
            found.attributed = line != declared;
            const std::wstring& text = file.lines[line].text;
            std::optional<FoundComment> trailing;
            if (const std::size_t start = lines[line].trailing; start != std::wstring::npos)
            {
                found.code = withoutTrailingBlanks(std::wstring_view{ text }.substr(0, start));
                const std::size_t end = line + 1 + continuationsOf(file, lines, line);
                trailing = cutFound(file, CommentPlace::Trailing, line, end, start, language,
                    rules);
            }
            else
            {
                found.code = withoutTrailingBlanks(text);
            }
            if (site.comment && site.comment->readAt != 0)
            {
                const std::optional<FoundComment> body = bodyComment(file, lines,
                    site.comment->readAt - 1);
                if (!body)
                    return std::nullopt;
                found.comment = *body;
                return found;
            }

            std::optional<FoundComment> above;
            if (site.readsAbove)
            {
                std::size_t end = line;
                std::size_t first = aboveStart(file, lines, line);
                if (first == line && language == SourceLanguage::Cpp)
                {
                    found.head = prefixStart(file, line);
                    end = found.head;
                    first = aboveStart(file, lines, found.head);
                }
                if (first != end)
                {
                    above = cutFound(file, CommentPlace::Above, first, end,
                        leadingBlanks(file.lines[first].text), language, rules);
                }
            }
            const auto says = [](const std::optional<FoundComment>& comment) {
                return comment && !comment->words.empty();
            };
            const bool fromLine = rules == HintRules::Strict
                ? trailing.has_value()
                : !says(above) && (says(trailing) || !above);
            if (fromLine && trailing)
            {
                found.comment = *trailing;
                found.aboveTaken = above.has_value();
            }
            else if (above)
            {
                found.comment = *above;
            }
            return found;
        }

        // The words with each run of blanks and line ends one blank, the ends trimmed.
        [[nodiscard]] std::wstring folded(const std::wstring_view text)
        {
            std::wstring result;
            bool blank = false;
            for (const wchar_t current : text)
            {
                if (current == L' ' || current == L'\t' || current == L'\n')
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

        // What a reader takes from a comment: the hint, and the See cut off it by strict rules.
        struct ReadWords
        {
            std::wstring text;
            std::optional<Reference> reference;
            std::wstring see;   // the See as written
        };

        // By loose rules words that only name the declaration are a label, which says nothing.
        [[nodiscard]] ReadWords readOf(const FoundComment& comment, const std::wstring_view name,
            const SourceLanguage language, const HintRules rules)
        {
            ReadWords read{ .text = comment.words };
            if (comment.place == CommentPlace::None || comment.place == CommentPlace::Body)
                return read;
            if (rules == HintRules::Strict)
            {
                read.reference = cutReference(read.text, name);
                if (read.reference)
                    read.see = folded(std::wstring_view{ comment.words }.substr(read.text.size()));
                return read;
            }
            const std::wstring_view bare = bareName(name);
            const bool label = language == SourceLanguage::Pascal
                ? sameWordIgnoringCase(read.text, bare)
                : read.text == bare;
            if (label)
                read.text.clear();
            return read;
        }

        // The words as blocks of a note, the blank ones left out and the blanks inside a paragraph
        // or a bullet folded, as a rewrap leaves them: what two texts are compared by.
        [[nodiscard]] Lines blockWords(const std::wstring_view text)
        {
            Lines result;
            for (const NoteBlock& block : blocksOf(splitLines(text)))
            {
                const bool flows = block.kind == NoteBlockKind::Paragraph
                    || block.kind == NoteBlockKind::Bullet;
                if (block.kind != NoteBlockKind::Blank)
                    result.push_back(flows ? folded(block.flowed) : block.flowed);
            }
            return result;
        }

        // The words ending with the See: on the last paragraph's end, or as a paragraph of its own
        // after a bullet or code.
        [[nodiscard]] std::wstring withSee(const std::wstring& words, const std::wstring& see)
        {
            if (see.empty())
                return words;
            if (words.empty())
                return see;
            const NoteBlocks blocks = blocksOf(splitLines(words));
            const bool paragraph = !blocks.empty()
                && blocks.back().kind == NoteBlockKind::Paragraph;
            return words + (paragraph ? L" " : L"\n\n") + see;
        }

        // The words broken into lines under the leads given, each paragraph and bullet at the line
        // width, code as written, a paragraph's end the blank lead.
        [[nodiscard]] Lines wordLines(const std::wstring_view words, const std::wstring_view first,
            const std::wstring_view rest, const std::wstring_view blank)
        {
            Lines result;
            for (const NoteBlock& block : blocksOf(splitLines(words)))
            {
                const std::wstring lead{ result.empty() ? first : rest };
                switch (block.kind)
                {
                    case NoteBlockKind::Blank:
                        if (!result.empty())
                            result.emplace_back(blank);
                        break;
                    case NoteBlockKind::Heading:
                    case NoteBlockKind::Code:
                        result.push_back(withoutTrailingBlanks(lead + block.lines.front()));
                        break;
                    case NoteBlockKind::Bullet:
                    {
                        const std::wstring_view said =
                            std::wstring_view{ block.flowed }.substr(k_bulletMark.size());
                        const std::wstring bulletFirst = lead + std::wstring{ k_bulletMark };
                        const std::wstring bulletRest = std::wstring{ rest }
                            + std::wstring{ k_bulletFollow };
                        for (std::wstring& line : wrapped(said, bulletFirst, bulletRest))
                            result.push_back(std::move(line));
                        break;
                    }
                    case NoteBlockKind::Paragraph:
                        for (std::wstring& line : wrapped(block.flowed, lead, rest))
                            result.push_back(std::move(line));
                        break;
                }
            }
            while (!result.empty() && result.back() == blank)
                result.pop_back();
            return result;
        }

        // A comment's marks and where they stand in a lead: what is before them, and their end.
        struct Marks
        {
            std::size_t start{ 0 };
            std::size_t end{ 0 };
            std::wstring_view closer;   // a block's closer; empty for a line comment
        };

        // The marks opening a comment at a place: the opener, the stars after it, the ! and the <
        // of a doc comment.
        [[nodiscard]] std::optional<Marks> marksAt(const std::wstring_view text,
            const std::size_t at)
        {
            const std::wstring_view rest = text.substr(std::min(at, text.size()));
            Marks marks{ .start = at };
            wchar_t repeated = L'*';
            if (rest.starts_with(k_lineComment))
            {
                marks.end = at + k_lineComment.size();
                repeated = L'/';
            }
            else if (rest.starts_with(k_blockComment))
            {
                marks.end = at + k_blockComment.size();
                marks.closer = k_blockCommentEnd;
            }
            else if (rest.starts_with(k_parenComment))
            {
                marks.end = at + k_parenComment.size();
                marks.closer = k_parenCommentEnd;
            }
            else if (rest.starts_with(k_braceComment))
            {
                marks.end = at + k_braceComment.size();
                marks.closer = k_braceCommentEnd;
            }
            else
            {
                return std::nullopt;
            }
            while (marks.end < text.size() && text[marks.end] == repeated
                && (marks.closer.empty() || !text.substr(marks.end).starts_with(marks.closer)))
            {
                ++marks.end;
            }
            if (marks.end < text.size() && text[marks.end] == L'!')
                ++marks.end;
            if (marks.end < text.size() && text[marks.end] == L'<')
                ++marks.end;
            return marks;
        }

        // A run of a comment's lines in one shape: lines of one line comment's marks, or one
        // block from its opener to its closer.
        struct Segment
        {
            std::size_t first{ 0 };
            std::size_t end{ 0 };
            bool block{ false };
            std::wstring firstLead;       // the lead of its first line, the opener in it
            std::wstring innerLead;       // the lead of a line inside it
            std::wstring blankLead;       // a line between two of its paragraphs
            std::wstring wordsTail;       // the closer, where it ends the last line of words
            bool opensOnWords{ false };   // the first line of words carries the opener
        };

        using Segments = std::vector<Segment>;

        // The marks a lead opens a comment with past its indent; none for a line inside a block.
        [[nodiscard]] std::optional<Marks> leadMarks(const std::wstring_view lead)
        {
            return marksAt(lead, leadingBlanks(lead));
        }

        [[nodiscard]] bool saysWords(const CommentLine& line)
        {
            return line.speaks && !trimmed(line.words).empty();
        }

        // The lead a line inside a segment takes where no later line of it shows one, made after
        // its first line's: the slashes again, a block's dressing star, or blanks setting the
        // words in under the first line's.
        [[nodiscard]] std::wstring madeInnerLead(const std::wstring_view lead)
        {
            const std::size_t indent = leadingBlanks(lead);
            const std::optional<Marks> marks = marksAt(lead, indent);
            if (!marks)
                return std::wstring{ lead };
            const std::wstring_view opener = lead.substr(indent, marks->end - indent);
            const std::wstring before{ lead.substr(0, indent) };
            if (marks->closer.empty())
                return before + std::wstring{ opener } + L" ";
            if (opener.starts_with(L"/**") || opener.starts_with(L"/*!"))
                return before + L" * ";
            return before + std::wstring(opener.size() + 1, L' ');
        }

        // The comment's lines cut into segments, and the segment each line is in.
        [[nodiscard]] Segments segmentsOf(const CommentLines& lines,
            std::vector<std::size_t>& segmentOf)
        {
            Segments segments;
            segmentOf.assign(lines.size(), 0);
            bool insideBlock = false;
            std::wstring lineMarks;   // the marks of the line comment the last segment is made of
            for (std::size_t i = 0; i != lines.size(); ++i)
            {
                const CommentLine& line = lines[i];
                const std::optional<Marks> marks = insideBlock
                    ? std::nullopt
                    : leadMarks(line.lead);
                bool opens = !insideBlock;
                if (marks && marks->closer.empty())
                {
                    const std::wstring own = line.lead.substr(marks->start,
                        marks->end - marks->start);
                    opens = segments.empty() || segments.back().block || own != lineMarks;
                    lineMarks = own;
                }
                else if (!insideBlock)
                {
                    lineMarks.clear();
                }
                if (opens)
                {
                    const bool block = marks && !marks->closer.empty();
                    segments.push_back({ .first = i, .end = i, .block = block });
                }
                segments.back().end = i + 1;
                segmentOf[i] = segments.size() - 1;
                if (marks && !marks->closer.empty())
                    insideBlock = line.tail.empty();
                else if (insideBlock && !line.tail.empty())
                    insideBlock = false;
            }
            for (Segment& segment : segments)
            {
                // The lead of a line inside it that says words, else of one that says other
                // words - a TODO - else one made after the first line's.
                segment.firstLead = lines[segment.first].lead;
                segment.innerLead = madeInnerLead(segment.firstLead);
                std::optional<std::size_t> inner;
                for (std::size_t i = segment.first + 1; i < segment.end; ++i)
                {
                    const bool opener = leadMarks(lines[i].lead).has_value() && segment.block;
                    if (opener || trimmed(lines[i].words).empty())
                        continue;
                    if (saysWords(lines[i]))
                    {
                        inner = i;
                        break;
                    }
                    if (!inner)
                        inner = i;
                }
                if (inner)
                    segment.innerLead = leadWithoutBrief(lines[*inner].lead);
                segment.opensOnWords = saysWords(lines[segment.first])
                    && segment.firstLead != segment.innerLead;
                segment.blankLead = withoutTrailingBlanks(segment.innerLead);
                for (std::size_t i = segment.first; i < segment.end; ++i)
                {
                    const CommentLine& line = lines[i];
                    const bool blank = line.speaks && trimmed(line.words).empty()
                        && line.tail.empty() && leadWithoutBrief(line.lead) == line.lead;
                    if (blank)
                    {
                        segment.blankLead = withoutTrailingBlanks(line.lead);
                        break;
                    }
                }
                for (std::size_t i = segment.end; i-- > segment.first;)
                {
                    if (saysWords(lines[i]))
                    {
                        segment.wordsTail = lines[i].tail;
                        break;
                    }
                }
            }
            return segments;
        }

        // A comment line written again: its lead, words and tail, or the line as it stands.
        struct WrittenLine
        {
            std::wstring lead;
            std::wstring words;
            std::wstring tail;
            std::wstring asWritten;
            std::size_t from{ std::wstring::npos };   // the comment line it is; npos for a new one
            std::size_t segment{ 0 };
            bool speaks{ false };
            bool remade{ false };   // written from its lead, words and tail, not as it stood
        };

        using WrittenLines = std::vector<WrittenLine>;

        // One step of the old blocks set against the new ones.
        struct BlockStep
        {
            std::size_t before{ std::wstring::npos };   // the old block; npos for one put in
            std::size_t after{ std::wstring::npos };    // the new block; npos for one taken out
        };

        // The longest run of blocks the two texts share, in order, and the steps between them.
        [[nodiscard]] std::vector<BlockStep> alignBlocks(const Lines& before, const Lines& after)
        {
            const std::size_t rows = before.size();
            const std::size_t columns = after.size();
            std::vector<std::size_t> lengths((rows + 1) * (columns + 1), 0);
            const auto at = [columns](const std::size_t row, const std::size_t column) {
                return row * (columns + 1) + column;
            };
            for (std::size_t row = rows; row-- != 0;)
            {
                for (std::size_t column = columns; column-- != 0;)
                {
                    lengths[at(row, column)] = before[row] == after[column]
                        ? lengths[at(row + 1, column + 1)] + 1
                        : std::max(lengths[at(row + 1, column)], lengths[at(row, column + 1)]);
                }
            }
            std::vector<BlockStep> steps;
            std::size_t row = 0;
            std::size_t column = 0;
            while (row < rows || column < columns)
            {
                if (row < rows && column < columns && before[row] == after[column])
                {
                    steps.push_back({ .before = row++, .after = column++ });
                    continue;
                }
                const bool takeOut = column == columns
                    || (row < rows && lengths[at(row + 1, column)] >= lengths[at(row, column + 1)]);
                if (takeOut)
                    steps.push_back({ .before = row++, .after = std::wstring::npos });
                else
                    steps.push_back({ .before = std::wstring::npos, .after = column++ });
            }
            // Blocks taken out where others are put in are those blocks changed, in their places
            // and in order; what is left of either run follows them.
            std::vector<BlockStep> merged;
            std::size_t run = 0;
            while (run != steps.size())
            {
                std::size_t takenEnd = run;
                while (takenEnd != steps.size() && steps[takenEnd].after == std::wstring::npos)
                    ++takenEnd;
                std::size_t putEnd = takenEnd;
                while (putEnd != steps.size() && steps[putEnd].before == std::wstring::npos)
                    ++putEnd;
                if (takenEnd == run && putEnd == takenEnd)
                {
                    merged.push_back(steps[run++]);
                    continue;
                }
                const std::size_t taken = takenEnd - run;
                const std::size_t put = putEnd - takenEnd;
                const std::size_t paired = std::min(taken, put);
                for (std::size_t i = 0; i != paired; ++i)
                {
                    merged.push_back({
                        .before = steps[run + i].before,
                        .after = steps[takenEnd + i].after
                    });
                }
                for (std::size_t i = paired; i < taken; ++i)
                    merged.push_back(steps[run + i]);
                for (std::size_t i = paired; i < put; ++i)
                    merged.push_back(steps[takenEnd + i]);
                run = putEnd;
            }
            return merged;
        }

        // A block of a comment's words and the comment lines it stands on.
        struct OldBlock
        {
            std::wstring flowed;
            std::size_t first{ 0 };
            std::size_t end{ 0 };
        };

        // **The comment written again in its own marks.** A block that reads as one of the old
        // ones did keeps its lines, and so do the lines between two such; any other is broken at
        // the line width under the leads of the part of the comment it stands in - lines of one
        // line comment's marks, or one block. The lines saying no words - a TODO, a tag, an
        // opener or a closer alone - stay where they stood, and a block left saying nothing goes.
        [[nodiscard]] Lines rebuiltComment(const FoundComment& comment, const FileLines& file,
            const std::wstring_view words, const std::wstring_view indent)
        {
            const CommentLines& lines = comment.lines;
            std::vector<std::size_t> segmentOf;
            Segments segments = segmentsOf(lines, segmentOf);
            const CommentTextLines text = textLinesOf(lines);
            Lines oldWords;
            for (const CommentTextLine& line : text)
                oldWords.push_back(line.words);
            std::vector<OldBlock> before;
            for (const NoteBlock& block : blocksOf(oldWords))
            {
                if (block.kind == NoteBlockKind::Blank)
                    continue;
                const std::size_t last = text[block.first + block.lines.size() - 1].line;
                before.push_back({
                    .flowed = block.flowed,
                    .first = text[block.first].line,
                    .end = last + 1
                });
            }
            NoteBlocks after;
            std::vector<bool> parted;
            for (const NoteBlock& block : blocksOf(splitLines(words)))
            {
                if (block.kind == NoteBlockKind::Blank)
                {
                    if (!parted.empty())
                        parted.back() = true;
                    continue;
                }
                after.push_back(block);
                parted.push_back(false);
            }

            const auto kept = [&](const std::size_t index) {
                const CommentLine& line = lines[index];
                return WrittenLine{
                    .lead = line.lead,
                    .words = line.words,
                    .tail = line.tail,
                    .asWritten = file.lines[comment.first + index].text,
                    .from = index,
                    .segment = segmentOf[index],
                    .speaks = saysWords(line)
                };
            };
            WrittenLines written;
            WrittenLines pending;   // lines saying no words, waiting for the next block
            const auto keepSilent = [&](const std::size_t first, const std::size_t end) {
                for (std::size_t i = first; i < end; ++i)
                {
                    if (!lines[i].speaks)
                        pending.push_back(kept(i));
                }
            };
            const auto lastSpokenSegment = [&]() -> std::optional<std::size_t> {
                for (std::size_t i = written.size(); i-- != 0;)
                {
                    if (written[i].speaks)
                        return written[i].segment;
                }
                return std::nullopt;
            };
            const auto openBlock = [&](const std::size_t newBlock, const std::size_t segment) {
                const bool silentLines = !pending.empty();
                for (WrittenLine& line : pending)
                    written.push_back(std::move(line));
                pending.clear();
                const bool split = newBlock == 0 || parted[newBlock - 1];
                if (!silentLines && split && lastSpokenSegment())
                {
                    written.push_back({
                        .segment = segment,
                        .remade = true
                    });
                }
            };
            // A block written in place of an old one keeps the brief's command or the summary's
            // tag the old one opened with.
            const auto newLines = [&](const std::size_t newBlock, const std::size_t segment,
                const std::size_t oldFirst = std::wstring::npos) {
                const Segment& part = segments[segment];
                const bool firstInPart = std::ranges::none_of(written,
                    [segment](const WrittenLine& line) {
                        return line.speaks && line.segment == segment;
                    });
                bool briefed = false;
                if (oldFirst != std::wstring::npos)
                {
                    const std::wstring_view old = lines[oldFirst].lead;
                    const std::optional<Marks> marks = leadMarks(old);
                    briefed = leadWithoutBrief(old).size() != old.size()
                        && (!marks || marks->closer.empty());
                }
                std::wstring lead = part.innerLead;
                if (firstInPart && part.opensOnWords)
                    lead = part.firstLead;
                else if (briefed)
                    lead = lines[oldFirst].lead;
                const Lines made = wordLines(after[newBlock].flowed, lead, part.innerLead,
                    part.blankLead);
                for (std::size_t i = 0; i != made.size(); ++i)
                {
                    const std::wstring& prefix = i == 0 ? lead : part.innerLead;
                    const std::size_t cut = made[i].starts_with(prefix) ? prefix.size() : 0;
                    written.push_back({
                        .lead = made[i].substr(0, cut),
                        .words = made[i].substr(cut),
                        .segment = segment,
                        .speaks = true,
                        .remade = true
                    });
                }
            };

            // **Words put into a comment that says none open its first line comment**, or stand
            // under a block's opener standing alone - a TODO they went after would carry them on
            // as its own. A comment with room for neither, such as a separator's block, takes a
            // line comment of their own under it.
            std::size_t prefixEnd = before.empty() ? lines.size() : before.front().first;
            std::size_t firstSegment = before.empty() ? segments.size() : segmentOf[prefixEnd];
            for (std::size_t i = 0; before.empty() && i != segments.size(); ++i)
            {
                const CommentLine& opening = lines[segments[i].first];
                const std::optional<Marks> marks = leadMarks(opening.lead);
                if (!marks)
                    continue;
                const bool lineRun = marks->closer.empty();
                const bool openerAlone = !lineRun && !opening.speaks && opening.tail.empty();
                if (lineRun || openerAlone)
                {
                    prefixEnd = segments[i].first + (openerAlone ? 1 : 0);
                    firstSegment = i;
                    break;
                }
            }
            if (firstSegment == segments.size())
            {
                const std::wstring marks = std::wstring{ indent } + std::wstring{ k_lineComment };
                segments.push_back({
                    .first = lines.size(),
                    .end = lines.size(),
                    .firstLead = marks + L" ",
                    .innerLead = marks + L" ",
                    .blankLead = marks
                });
            }
            for (std::size_t i = 0; i != prefixEnd; ++i)
                written.push_back(kept(i));

            // The old block written last with nothing put in after it: the lines between it and
            // the next old block stand as they did.
            std::size_t lastOld = std::wstring::npos;
            Lines beforeFlowed;
            for (const OldBlock& block : before)
                beforeFlowed.push_back(block.flowed);
            Lines afterFlowed;
            for (const NoteBlock& block : after)
                afterFlowed.push_back(block.flowed);
            for (const BlockStep& step : alignBlocks(beforeFlowed, afterFlowed))
            {
                if (step.before == std::wstring::npos)
                {
                    const std::size_t segment = lastSpokenSegment().value_or(firstSegment);
                    openBlock(step.after, segment);
                    newLines(step.after, segment);
                    lastOld = std::wstring::npos;
                    continue;
                }
                const std::size_t index = step.before;
                const std::size_t segment = segmentOf[before[index].first];
                const bool follows = index != 0 && lastOld == index - 1
                    && step.after != std::wstring::npos;
                if (index != 0)
                {
                    const std::size_t gapFirst = before[index - 1].end;
                    const std::size_t gapEnd = before[index].first;
                    if (follows)
                    {
                        for (std::size_t i = gapFirst; i < gapEnd; ++i)
                            written.push_back(kept(i));
                    }
                    else
                    {
                        keepSilent(gapFirst, gapEnd);
                    }
                }
                if (step.after == std::wstring::npos)
                {
                    keepSilent(before[index].first, before[index].end);
                    lastOld = std::wstring::npos;
                    continue;
                }
                if (!follows)
                    openBlock(step.after, segment);
                if (before[index].flowed == after[step.after].flowed)
                {
                    for (std::size_t i = before[index].first; i < before[index].end; ++i)
                        written.push_back(kept(i));
                }
                else
                {
                    newLines(step.after, segment, before[index].first);
                    keepSilent(before[index].first, before[index].end);
                }
                lastOld = index;
            }
            for (WrittenLine& line : pending)
                written.push_back(std::move(line));
            const std::size_t suffixStart = before.empty() ? prefixEnd : before.back().end;
            for (std::size_t i = suffixStart; i < lines.size(); ++i)
                written.push_back(kept(i));

            // **In each part the first line of words carries the opener and the last the closer**,
            // wherever the words moved; a block whose words all left goes whole.
            std::vector<bool> dropped(written.size(), false);
            for (std::size_t segment = 0; segment != segments.size(); ++segment)
            {
                const Segment& part = segments[segment];
                std::vector<std::size_t> spoken;
                for (std::size_t i = 0; i != written.size(); ++i)
                {
                    if (written[i].speaks && written[i].segment == segment)
                        spoken.push_back(i);
                }
                if (spoken.empty())
                {
                    const bool said = std::ranges::any_of(
                        std::span{ lines }.subspan(part.first, part.end - part.first), saysWords);
                    if (!part.block || !said)
                        continue;
                    for (std::size_t i = 0; i != written.size(); ++i)
                        dropped[i] = dropped[i] || written[i].segment == segment;
                    continue;
                }
                for (std::size_t n = 0; n != spoken.size(); ++n)
                {
                    WrittenLine& line = written[spoken[n]];
                    const bool first = n == 0;
                    const bool last = n + 1 == spoken.size();
                    const bool leadWrong = part.opensOnWords
                        && (line.lead == part.firstLead) != first;
                    if (leadWrong)
                    {
                        line.lead = first ? part.firstLead : part.innerLead;
                        line.remade = true;
                    }
                    const bool tailWrong = !part.wordsTail.empty()
                        && (line.tail == part.wordsTail) != last;
                    if (tailWrong)
                    {
                        line.tail = last ? part.wordsTail : std::wstring{};
                        line.remade = true;
                    }
                }
            }
            Lines result;
            for (std::size_t i = 0; i != written.size(); ++i)
            {
                const WrittenLine& line = written[i];
                if (dropped[i])
                    continue;
                if (!line.remade)
                    result.push_back(line.asWritten);
                else if (!line.speaks && line.from == std::wstring::npos)
                    result.push_back(segments[line.segment].blankLead);
                else
                    result.push_back(withoutTrailingBlanks(line.lead + line.words + line.tail));
            }
            return result;
        }

        // Lines of a file taken out and put in at a line, all in the file's lines as they were.
        struct Replacement
        {
            std::size_t at{ 0 };
            std::size_t count{ 0 };
            Lines lines;
        };

        using Replacements = std::vector<Replacement>;

        // Whether the code closes every bracket it opens, so the declaration ends on its line.
        [[nodiscard]] bool closesItsBrackets(const std::wstring_view code)
        {
            int depth = 0;
            for (const wchar_t current : code)
            {
                if (current == L'(')
                    ++depth;
                else if (current == L')')
                    --depth;
            }
            return depth <= 0;
        }

        // The nearest line of code over or under a line, comment lines passed over; npos for none.
        [[nodiscard]] std::size_t codeLineBeside(const SourceLines& lines, std::size_t line,
            const bool under)
        {
            while (under ? line + 1 < lines.size() : line != 0)
            {
                line = under ? line + 1 : line - 1;
                if (lines[line].kind != LineKind::CommentOnly)
                    return line;
            }
            return std::wstring_view::npos;
        }

        // The gap that puts a comment on the line under the comments of the lines of code round
        // it, where one of them carries one past the code; a blank otherwise.
        [[nodiscard]] std::wstring alignedGap(const SourceLines& lines, const std::size_t line,
            const std::wstring_view code)
        {
            for (const bool under : { false, true })
            {
                const std::size_t neighbour = codeLineBeside(lines, line, under);
                if (neighbour == std::wstring_view::npos)
                    continue;
                const std::size_t column = lines[neighbour].trailing;
                if (column != std::wstring::npos && column > code.size())
                    return std::wstring(column - code.size(), L' ');
            }
            return L" ";
        }

        // The words on the declaration's line after the code and the gap, in the marks given; the
        // gap one blank where the one asked for would run the line past the width. Nothing where
        // even that does.
        [[nodiscard]] std::optional<std::wstring> ridingLine(const std::wstring_view code,
            const std::wstring_view gap, const std::wstring_view marks,
            const std::wstring_view words, const std::wstring_view tail)
        {
            for (const std::wstring_view spacing : { gap, std::wstring_view{ L" " } })
            {
                const std::wstring line = std::wstring{ code } + std::wstring{ spacing }
                    + std::wstring{ marks } + L" " + std::wstring{ words } + std::wstring{ tail };
                if (columnsOf(line) <= k_lineColumns)
                    return line;
            }
            return std::nullopt;
        }

        // The marks a comment above a declaration takes after the one on its line: a doc
        // comment's < goes, the comment no longer pointing back.
        [[nodiscard]] std::wstring marksAbove(std::wstring_view marks)
        {
            if (marks.ends_with(L'<'))
                marks.remove_suffix(1);
            return std::wstring{ marks };
        }

        // The marks a comment on a declaration's line takes after the one above it: by loose
        // rules a C++ doc comment points back to the declaration with a <.
        [[nodiscard]] std::wstring marksOnLine(const std::wstring_view marks,
            const SourceLanguage language, const HintRules rules)
        {
            const bool doc = marks == L"///" || marks == L"//!";
            if (doc && language == SourceLanguage::Cpp && rules == HintRules::Loose)
                return std::wstring{ marks } + L"<";
            return std::wstring{ marks };
        }

        // **A comment written new above a declaration**, in the marks given: a line comment's
        // marks on every line, a C++ block's opener and closer on lines of their own with a star
        // dressing the lines between, and a Delphi block's words set in under its opener with the
        // closer after the last of them.
        [[nodiscard]] Lines newComment(const std::wstring& indent, const std::wstring_view marks,
            const std::wstring_view closer, const std::wstring_view words)
        {
            const std::wstring opener = indent + std::wstring{ marks };
            if (closer.empty())
                return wordLines(words, opener + L" ", opener + L" ", opener);
            if (marks.starts_with(k_blockComment))
            {
                Lines result{ opener };
                const std::wstring dressed = indent + L" * ";
                for (std::wstring& line : wordLines(words, dressed, dressed, indent + L" *"))
                    result.push_back(std::move(line));
                result.push_back(indent + L" " + std::wstring{ closer });
                return result;
            }
            const std::wstring setIn = indent + std::wstring(marks.size() + 1, L' ');
            Lines result = wordLines(words, opener + L" ", setIn, {});
            result.back() += L" " + std::wstring{ closer };
            return result;
        }

        // Whether a comment above is the routine's plain shape - lines of one line comment's
        // marks, every one saying words or parting two paragraphs - which one line on the
        // declaration can stand for.
        [[nodiscard]] bool isPlainLineComment(const FoundComment& comment)
        {
            std::optional<std::wstring> shared;
            for (const CommentLine& line : comment.lines)
            {
                if (!line.speaks || !line.tail.empty())
                    return false;
                const std::size_t indent = leadingBlanks(line.lead);
                const std::optional<Marks> marks = marksAt(line.lead, indent);
                if (!marks || !marks->closer.empty())
                    return false;
                const std::wstring own{ line.lead.substr(indent, marks->end - indent) };
                if (shared && *shared != own)
                    return false;
                shared = own;
            }
            return shared.has_value();
        }

        // The lines of a comment that stay when its words are taken out: in lines of a line
        // comment the TODOs; in a block, none.
        [[nodiscard]] Lines silentRemains(const FoundComment& comment, const FileLines& file)
        {
            Lines result;
            for (std::size_t i = 0; i != comment.lines.size(); ++i)
            {
                const CommentLine& line = comment.lines[i];
                const std::size_t indent = leadingBlanks(line.lead);
                const std::optional<Marks> marks = marksAt(line.lead, indent);
                const bool lineComment = marks && marks->closer.empty() && line.tail.empty();
                if (!lineComment && comment.place != CommentPlace::Body)
                    return {};
                const bool todo = !line.speaks && !line.opensParagraph;
                const bool noComment = comment.place == CommentPlace::Body && !marks
                    && line.lead == file.lines[comment.first + i].text;
                if (todo || noComment)
                    result.push_back(file.lines[comment.first + i].text);
            }
            return result;
        }

        // **Where the words go**: words that fit one line stand on the declaration's line while the
        // whole line fits the width - never a type opening a scope, nor a comment above that says
        // more than the words, such as a TODO - and any others stand above it. A comment under a
        // Delphi routine's header keeps its place.
        [[nodiscard]] std::optional<Replacements> placeWords(const FileLines& file,
            const SourceLines& lines, const FoundDeclaration& found, const CommentSite& site,
            const std::wstring& words, const SourceLanguage language, const HintRules rules,
            Refusal& refusal)
        {
            const FoundComment& comment = found.comment;
            const std::size_t line = found.line;
            const bool oneLine = words.find(L'\n') == std::wstring::npos;
            const bool mayRide = !site.opensScope && !found.attributed
                && closesItsBrackets(found.code);
            const std::wstring indent = indentText(file.lines[found.head].text);
            const auto rideRefusal = [&refusal]() {
                refusal = L"This declaration shares its line with the one above it, so its "
                    L"comment rides the line - and these words would not fit on it.";
            };
            Replacements result;
            switch (comment.place)
            {
                case CommentPlace::Body:
                {
                    Lines remade = words.empty()
                        ? silentRemains(comment, file)
                        : rebuiltComment(comment, file, words,
                            indentText(file.lines[comment.first].text));
                    result.push_back({ comment.first, comment.lines.size(), std::move(remade) });
                    return result;
                }
                case CommentPlace::Trailing:
                {
                    const CommentLine& first = comment.lines.front();
                    const std::size_t start = lines[line].trailing;
                    const std::optional<Marks> marks = marksAt(file.lines[line].text, start);
                    const std::wstring own = marks
                        ? file.lines[line].text.substr(start, marks->end - start)
                        : std::wstring{ k_lineComment };
                    const std::wstring_view text = file.lines[line].text;
                    const std::wstring gap{ text.substr(found.code.size(),
                        start - found.code.size()) };
                    const Replacement onLine = { line, comment.lines.size(), { found.code } };
                    if (words.empty())
                    {
                        result.push_back(onLine);
                        return result;
                    }
                    if (oneLine && !found.attributed)
                    {
                        if (const std::optional<std::wstring> riding =
                            ridingLine(found.code, gap, own, words, first.tail))
                        {
                            result.push_back({ line, comment.lines.size(), { *riding } });
                            return result;
                        }
                    }
                    if (!site.readsAbove)
                    {
                        rideRefusal();
                        return std::nullopt;
                    }
                    if (found.aboveTaken)
                    {
                        refusal = L"A comment stands above this declaration besides the one on "
                            L"its line, so words that take more than its line would run into it.";
                        return std::nullopt;
                    }
                    const std::wstring_view closer = marks ? marks->closer : std::wstring_view{};
                    result.push_back(onLine);
                    result.push_back({ found.head, 0,
                        newComment(indent, marksAbove(own), closer, words) });
                    return result;
                }
                case CommentPlace::Above:
                {
                    const std::size_t count = comment.lines.size();
                    if (words.empty())
                    {
                        result.push_back({ comment.first, count, silentRemains(comment, file) });
                        return result;
                    }
                    const bool free = lines[line].trailing == std::wstring::npos;
                    if (oneLine && mayRide && free && isPlainLineComment(comment))
                    {
                        const std::wstring_view lead = comment.lines.front().lead;
                        const std::size_t at = leadingBlanks(lead);
                        const std::optional<Marks> marks = marksAt(lead, at);
                        const std::wstring own{ lead.substr(at, marks->end - at) };
                        const std::optional<std::wstring> riding = ridingLine(found.code,
                            alignedGap(lines, line, found.code),
                            marksOnLine(own, language, rules), words, {});
                        if (riding)
                        {
                            result.push_back({ line, 1, { *riding } });
                            result.push_back({ comment.first, count, {} });
                            return result;
                        }
                    }
                    result.push_back({ comment.first, count,
                        rebuiltComment(comment, file, words, indent) });
                    return result;
                }
                case CommentPlace::None:
                {
                    if (words.empty())
                        return result;
                    if (oneLine && mayRide)
                    {
                        const std::optional<std::wstring> riding = ridingLine(found.code,
                            alignedGap(lines, line, found.code), k_lineComment, words, {});
                        if (riding)
                        {
                            result.push_back({ line, 1, { *riding } });
                            return result;
                        }
                    }
                    if (!site.readsAbove)
                    {
                        rideRefusal();
                        return std::nullopt;
                    }
                    result.push_back({ found.head, 0,
                        newComment(indent, k_lineComment, {}, words) });
                    return result;
                }
            }
            return result;
        }

        // A declaration's file read, and the declaration found in it as the surface last read it.
        struct SiteInFile
        {
            std::filesystem::path path;
            FileLines file;
            SourceLines lines;
            SourceLanguage language{ SourceLanguage::Cpp };
            HintRules rules{ HintRules::Strict };
            FoundDeclaration declaration;
            ReadWords read;
        };

        // The comment is found again by reading the file the way the scan did. A file changed
        // since reads otherwise, and so does a declaration shaped past what the writer follows.
        [[nodiscard]] Refusal readOtherwise(const std::wstring_view relative)
        {
            return L"SeeDocs can't save this hint into " + std::wstring{ relative }
                + L" because source code around it is too complex or needs a rescan.";
        }

        // A comment the declaration borrows from a base is none of its own, so the file shows none.
        [[nodiscard]] std::optional<SiteInFile> readSite(const Surface& surface,
            const SourceRoots& roots, const CommentSite& site, Refusal& refusal)
        {
            if (!site.comment || site.place.file >= surface.files().size())
            {
                refusal = L"The surface holds no comment for this.";
                return std::nullopt;
            }
            const SourceFile& source = surface.files()[site.place.file];
            if (source.root >= roots.size())
            {
                refusal = L"The project no longer reads the folder " + source.relative
                    + L" was read from, so nothing was written to it.";
                return std::nullopt;
            }
            SiteInFile read;
            read.path = roots[source.root] / source.path;
            std::optional<FileLines> file = readLines(read.path);
            if (!file)
            {
                refusal = source.relative + L" cannot be read.";
                return std::nullopt;
            }
            if (!file->utf8)
            {
                refusal = source.relative + L" is not UTF-8, and writing it would change the "
                    L"characters it holds past ASCII, so nothing was written to it.";
                return std::nullopt;
            }
            read.file = std::move(*file);
            read.language = surface.languageOf(source.module);
            read.rules = surface.hintRules();
            read.lines = sourceLinesOf(read.file, read.language, read.rules);
            std::optional<FoundDeclaration> found = locate(read.file, read.lines, site,
                read.language, read.rules);
            if (!found)
            {
                refusal = readOtherwise(source.relative);
                return std::nullopt;
            }
            read.read = readOf(found->comment, site.name, read.language, read.rules);
            const std::wstring expected = site.comment->borrowed
                ? std::wstring{}
                : site.comment->text;
            if (read.read.text != expected)
            {
                refusal = readOtherwise(source.relative);
                return std::nullopt;
            }
            read.declaration = std::move(*found);
            return read;
        }

        // The words with the blank lines round them and the blanks ending each line taken off.
        [[nodiscard]] std::wstring trimmedWords(const std::wstring_view text)
        {
            Lines lines = splitLines(text);
            for (std::wstring& line : lines)
                line = withoutTrailingBlanks(line);
            trimBlankLines(lines);
            std::wstring result;
            for (std::size_t i = 0; i != lines.size(); ++i)
            {
                if (i != 0)
                    result += L'\n';
                result += lines[i];
            }
            return result;
        }

        // **The words are written into the comment**, ending with the See it ends with, and the
        // record and the places under it are brought in line with what a read of the file now
        // gives.
        [[nodiscard]] Refusal writeComment(Surface& surface, const SourceRoots& roots,
            const CommentSite& site, const std::wstring& words)
        {
            Refusal refusal;
            std::optional<SiteInFile> read = readSite(surface, roots, site, refusal);
            if (!read)
                return refusal;
            Comment* record = surface.writableComment(*site.comment);
            if (!record)
                return L"The surface holds no comment for this.";
            const FoundDeclaration& found = read->declaration;
            const std::wstring body = withSee(words, read->read.see);
            if (blockWords(body) == blockWords(found.comment.words))
                return {};
            std::optional<Replacements> placed = placeWords(read->file, read->lines, found, site,
                body, read->language, read->rules, refusal);
            if (!placed)
                return refusal;
            std::ranges::sort(*placed, [](const Replacement& left, const Replacement& right) {
                return left.at > right.at;
            });
            std::ptrdiff_t declarationShift = 0;
            for (const Replacement& replacement : *placed)
            {
                const std::size_t at = replacement.at;
                const auto begin = read->file.lines.begin() + static_cast<std::ptrdiff_t>(at);
                std::vector<FileLine> put;
                for (const std::wstring& line : replacement.lines)
                    put.push_back(lineBeside(read->file, at, line));
                const auto end = begin + static_cast<std::ptrdiff_t>(replacement.count);
                read->file.lines.erase(begin, end);
                read->file.lines.insert(read->file.lines.begin() + static_cast<std::ptrdiff_t>(at),
                    put.begin(), put.end());
                const std::ptrdiff_t delta = static_cast<std::ptrdiff_t>(replacement.lines.size())
                    - static_cast<std::ptrdiff_t>(replacement.count);
                if (at + replacement.count < site.place.line)
                    declarationShift += delta;
            }

            // **The file is written only where a read of it gives the words back**, with the See
            // they ended with and no other.
            CommentSite moved = site;
            moved.place.line = static_cast<std::size_t>(
                static_cast<std::ptrdiff_t>(site.place.line) + declarationShift);
            const SourceLines lines = sourceLinesOf(read->file, read->language, read->rules);
            const std::optional<FoundDeclaration> now = locate(read->file, lines, moved,
                read->language, read->rules);
            const ReadWords reread = now
                ? readOf(now->comment, site.name, read->language, read->rules)
                : ReadWords{};
            if (reread.see != read->read.see)
            {
                return L"A hint is words alone. The See that reaches a footnote comes with the "
                    L"footnote.";
            }
            // A body comment taken out leaves no comment for the declaration to read.
            const bool givenBack = now
                ? blockWords(reread.text) == blockWords(words)
                : words.empty();
            if (!givenBack)
                return L"These words would not read back as written, so nothing was written.";
            if (!writeLines(read->path, read->file))
                return surface.files()[site.place.file].relative + L" cannot be written.";
            for (const Replacement& replacement : *placed)
            {
                const std::ptrdiff_t delta = static_cast<std::ptrdiff_t>(replacement.lines.size())
                    - static_cast<std::ptrdiff_t>(replacement.count);
                const std::size_t after = replacement.at + replacement.count + 1;
                if (delta != 0)
                    surface.shiftLines(site.place.file, after, delta);
            }

            // The record, as the read of the file gives it.
            record->borrowed = false;
            record->trailing.reset();
            record->above.clear();
            record->text = reread.text;
            record->reference = reread.reference;
            if (!now)
                record->readAt = 0;
            if (now && now->comment.place == CommentPlace::Trailing)
                record->trailing = reread.text;
            else if (now && now->comment.place == CommentPlace::Above)
                record->above = splitLines(reread.text);
            return {};
        }
    }

    Refusal writeHint(Surface& surface, const SourceRoots& roots, const CommentSite& site,
        const std::wstring_view hint)
    {
        // Words left as they were are no edit, whatever they hold.
        if (!changesHint(site, hint))
            return {};
        return writeComment(surface, roots, site, trimmedWords(hint));
    }

    // **Words are compared block by block**, the blank lines between them aside, so an editor
    // showing a paragraph on one line changes nothing by being left.
    bool changesHint(const CommentSite& site, const std::wstring_view hint)
    {
        return !site.comment || blockWords(hint) != blockWords(site.comment->text);
    }

    std::wstring hintSource(const Comment& comment)
    {
        return flowedText(splitLines(comment.text));
    }

    std::wstring flowedText(const Lines& lines)
    {
        const NoteBlocks blocks = blocksOf(lines);
        std::wstring result;
        for (std::size_t i = 0; i != blocks.size(); ++i)
        {
            if (i != 0)
                result += L'\n';
            result += blocks[i].flowed;
        }
        return result;
    }

    std::size_t referencesTo(const Surface& surface, const Reference& reference)
    {
        std::size_t count = 0;
        surface.forEachComment([&](const Comment& comment) {
            if (comment.reference && comment.reference->note == reference.note
                && comment.reference->anchor == reference.anchor)
            {
                ++count;
            }
        });
        return count;
    }

    // **The section is compared with what was shown** before anything is written: the editor opened
    // on the notes as last read, and a file changed under it since would lose the change.
    Refusal writeFootnote(Notes& notes, const Reference& reference, const std::wstring_view flowed)
    {
        const NoteSection* shown = notes.section(reference.note, reference.anchor);
        const std::wstring fileName = noteFileName(reference.note);
        if (!shown)
            return fileName + L" no longer holds this section.";
        const Lines shownLines = shown->lines;
        const std::size_t level = shown->level;
        const Lines body = bodyLines(flowed, shownLines);
        if (Refusal refusal = checkBody(body, level); !refusal.empty())
            return refusal;

        const std::filesystem::path path = notePath(notes, reference.note);
        std::optional<FileLines> file = readLines(path);
        if (!file)
            return fileName + L" cannot be read.";
        const std::optional<SectionRange> range = sectionRange(*file, reference.anchor);
        Lines onDisk = range
            ? textsOf(file->lines, range->heading + 1, range->end)
            : Lines{};
        trimBlankLines(onDisk);
        if (!range || onDisk != shownLines)
        {
            notes.forget(reference.note);
            return fileName + L" has changed since it was read. It is read again now - open the "
                L"footnote again to edit what it holds.";
        }

        // The blank lines round the body stay as they were; a section with no body yet takes
        // one under its heading and one before the heading that follows.
        std::size_t lead = range->heading + 1;
        while (lead < range->end && isBlank(file->lines[lead].text))
            ++lead;
        std::size_t trail = range->end;
        while (trail > lead && isBlank(file->lines[trail - 1].text))
            --trail;
        const bool hadBody = lead != range->end;
        if (!hadBody)
        {
            lead = range->heading + 1;
            trail = lead;
        }
        std::vector<FileLine> lines;
        if (!hadBody)
            lines.push_back(lineBeside(*file, range->heading, {}));
        for (const std::wstring& line : body)
            lines.push_back(lineBeside(*file, range->heading, line));
        if (!hadBody && range->end != file->lines.size())
            lines.push_back(lineBeside(*file, range->heading, {}));
        file->lines.erase(file->lines.begin() + static_cast<std::ptrdiff_t>(lead),
            file->lines.begin() + static_cast<std::ptrdiff_t>(trail));
        file->lines.insert(file->lines.begin() + static_cast<std::ptrdiff_t>(lead),
            lines.begin(), lines.end());
        if (!writeLines(path, *file))
            return fileName + L" cannot be written.";
        notes.forget(reference.note);
        return {};
    }

    // **A note not made yet takes a footnote all the same**, made when the footnote is written.
    std::optional<NewFootnote> placeFootnote(Notes& notes, const CommentSite& site,
        const std::wstring_view note, const std::wstring_view after,
        const std::wstring_view owner, Refusal& refusal)
    {
        if (note.empty())
        {
            refusal = L"No note takes the footnotes of this folder.";
            return std::nullopt;
        }
        const std::wstring heading = footnoteHeading(owner, bareName(site.name));
        const std::wstring anchor = slug(heading);
        if (notes.reaches(note, anchor))
        {
            refusal = noteFileName(note) + L" already holds a section reached as #" + anchor + L".";
            return std::nullopt;
        }
        return NewFootnote{
            .reference = { .note = std::wstring{ note }, .anchor = anchor },
            .after = std::wstring{ after },
            .heading = heading
        };
    }

    // **A note made new** opens with its title - its stem, a blank for each hyphen - and ends its
    // lines the way the folder's first note does.
    Refusal addFootnote(Notes& notes, const NewFootnote& placed, const std::wstring_view flowed)
    {
        const Lines body = bodyLines(flowed, {});
        if (Refusal refusal = checkBody(body, k_footnoteLevel); !refusal.empty())
            return refusal;

        const std::wstring fileName = noteFileName(placed.reference.note);
        const std::filesystem::path path = notePath(notes, placed.reference.note);
        std::optional<FileLines> file;
        std::error_code error;
        if (std::filesystem::exists(path, error))
        {
            file = readLines(path);
            if (!file)
                return fileName + L" cannot be read.";
        }
        else
        {
            std::filesystem::create_directories(notes.folder(), error);
            if (error)
                return L"The folder of footnotes cannot be made.";
            file = newNote(notes, placed.reference.note);
        }
        // After the section named, past whatever it holds, and never inside the section of a
        // level above that holds it.
        std::size_t at = file->lines.size();
        if (!placed.after.empty())
        {
            if (const std::optional<SectionRange> range =
                sectionRange(*file, placed.after, k_footnoteLevel))
            {
                at = range->end;
            }
        }
        std::vector<FileLine> lines;
        if (at != 0 && !isBlank(file->lines[at - 1].text))
            lines.push_back(lineBeside(*file, at - 1, {}));
        lines.push_back(lineBeside(*file, at,
            std::wstring(k_footnoteLevel, L'#') + L" " + placed.heading));
        lines.push_back(lineBeside(*file, at, {}));
        for (const std::wstring& line : body)
            lines.push_back(lineBeside(*file, at, line));
        if (at != file->lines.size())
            lines.push_back(lineBeside(*file, at, {}));
        file->lines.insert(file->lines.begin() + static_cast<std::ptrdiff_t>(at), lines.begin(),
            lines.end());
        if (!writeLines(path, *file))
            return fileName + L" cannot be written.";
        notes.forget(placed.reference.note);
        return {};
    }
}
