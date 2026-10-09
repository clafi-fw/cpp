module SeeDocs_App.Edits;

import SeeDocs_App.Notes;
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
        constexpr std::wstring_view k_brief = L"@brief ";
        constexpr std::wstring_view k_todo = L"TODO";
        constexpr std::wstring_view k_see = L"See ";
        constexpr std::wstring_view k_templateWord = L"template";
        constexpr std::wstring_view k_exportWord = L"export ";
        constexpr std::wstring_view k_codeFence = L"    ";
        constexpr std::wstring_view k_bulletMark = L"- ";
        constexpr std::wstring_view k_bulletFollow = L"  ";
        constexpr std::size_t k_tabColumns = 4;
        constexpr std::size_t k_nameReach = 3;      // the lines a declaration's name is looked on
        constexpr std::size_t k_prefixReach = 4;    // the lines a template head is looked up over
        constexpr std::size_t k_footnoteLevel = 2;  // the level a new footnote's heading takes

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

        [[nodiscard]] std::wstring indentOf(const std::wstring_view line)
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

        // Comments in a source line

        // Where the comment ending a line starts - its slashes, outside any literal - or npos.
        [[nodiscard]] std::size_t commentStart(const std::wstring_view line)
        {
            wchar_t quote = 0;
            for (std::size_t i = 0; i < line.size(); ++i)
            {
                const wchar_t current = line[i];
                if (quote != 0)
                {
                    if (current == L'\\')
                        ++i;
                    else if (current == quote)
                        quote = 0;
                    continue;
                }
                // A quote after a digit separates the digits of a number.
                const bool separator = current == L'\'' && i != 0
                    && std::iswdigit(line[i - 1]) != 0;
                if ((current == L'"' || current == L'\'') && !separator)
                {
                    quote = current;
                    continue;
                }
                if (current != L'/' || i + 1 == line.size())
                    continue;
                if (line[i + 1] == L'/')
                    return i;
                if (line[i + 1] == L'*')
                {
                    const std::size_t close = line.find(L"*/", i + 2);
                    if (close == std::wstring_view::npos)
                        return std::wstring_view::npos;
                    i = close + 1;
                }
            }
            return std::wstring_view::npos;
        }

        [[nodiscard]] bool isCommentOnly(const std::wstring_view line)
        {
            return trimmed(line).starts_with(k_lineComment);
        }

        [[nodiscard]] bool opensTemplateHead(const std::wstring_view line)
        {
            std::wstring_view words = trimmed(line);
            if (words.starts_with(k_exportWord))
                words = trimmed(words.substr(k_exportWord.size()));
            if (!words.starts_with(k_templateWord))
                return false;
            const std::wstring_view rest = trimmed(words.substr(k_templateWord.size()));
            return rest.starts_with(L'<');
        }

        // Whether a line can stand in a declaration's prefix above it - a template head, its
        // requires clause, a head running on - rather than end a statement of its own.
        [[nodiscard]] bool continuesPrefix(const std::wstring_view line)
        {
            const std::wstring_view words = trimmed(line);
            return !words.empty() && !isCommentOnly(words) && !words.ends_with(L';')
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

        // The slashes a comment opens with - two, or three for a doc comment.
        [[nodiscard]] std::wstring slashesOf(const std::wstring_view comment)
        {
            std::size_t count = 0;
            while (count < comment.size() && comment[count] == L'/')
                ++count;
            return std::wstring{ comment.substr(0, count) };
        }

        // A comment's words, as the scanner reads them: the slashes, the blanks round them and
        // an @brief taken off.
        [[nodiscard]] std::wstring_view bodyOf(std::wstring_view comment)
        {
            comment.remove_prefix(slashesOf(comment).size());
            comment = trimmed(comment);
            if (comment.starts_with(k_brief))
                comment = trimmed(comment.substr(k_brief.size()));
            return comment;
        }

        // A comment's words without the See they end with.
        [[nodiscard]] std::wstring wordsOf(const std::wstring_view body,
            const std::wstring_view name)
        {
            std::wstring words{ body };
            std::ignore = cutReference(words, name);
            return words;
        }

        // The See a comment ends with, as written; empty where it has none.
        [[nodiscard]] std::wstring referenceOf(const std::wstring_view body,
            const std::wstring_view name)
        {
            std::wstring words{ body };
            if (!cutReference(words, name))
                return {};
            return std::wstring{ trimmed(body.substr(words.size())) };
        }

        // How a See names a section: the note, and the anchor where the name's own slug is not it.
        [[nodiscard]] std::wstring seeText(const Reference& reference, const std::wstring_view name)
        {
            std::wstring result = std::wstring{ k_see } + reference.note;
            if (reference.anchor != slug(bareName(name)))
                result += L"#" + reference.anchor;
            return result;
        }

        // A comment as the file holds it.
        struct FoundComment
        {
            std::size_t line{ 0 };      // the line its words are on
            std::size_t column{ 0 };    // where its slashes start
            bool trailing{ false };     // on the declaration's own line
        };

        // A declaration as the file holds it, read the way the scanner reads it: the comment on
        // its line, else the first comment line above it that is no TODO - above the template
        // line over it where nothing stands right above it.
        struct FoundDeclaration
        {
            std::size_t line{ 0 };
            std::size_t head{ 0 };         // the line a comment above stands over
            std::size_t blockStart{ 0 };   // the first comment line over the head, else the head
            std::size_t blockWords{ 0 };   // the comment lines over the head that are no TODO
            std::optional<FoundComment> comment;
            std::wstring body;             // the comment's words as written, its See included
        };

        [[nodiscard]] std::optional<FoundDeclaration> locate(const FileLines& file,
            const CommentSite& site)
        {
            if (site.place.line == 0 || site.place.line > file.lines.size())
                return std::nullopt;
            const std::size_t line = site.place.line - 1;
            const std::wstring_view name = bareName(site.name);
            const std::size_t reach = std::min(line + k_nameReach, file.lines.size());
            bool named = false;
            for (std::size_t i = line; i < reach && !named; ++i)
                named = names(file.lines[i].text, name);
            if (!named)
                return std::nullopt;

            FoundDeclaration found;
            found.line = line;
            found.head = line;
            const std::wstring& text = file.lines[line].text;
            if (const std::size_t column = commentStart(text);
                column != std::wstring::npos && !isCommentOnly(text))
            {
                found.comment = FoundComment{ .line = line, .column = column, .trailing = true };
                found.body = std::wstring{ bodyOf(std::wstring_view{ text }.substr(column)) };
            }
            found.blockStart = line;
            if (!site.readsAbove)
                return found;
            const bool nothingAbove = line == 0 || !isCommentOnly(file.lines[line - 1].text);
            if (nothingAbove)
                found.head = prefixStart(file, line);
            found.blockStart = found.head;
            while (found.blockStart != 0 && isCommentOnly(file.lines[found.blockStart - 1].text))
                --found.blockStart;
            for (std::size_t i = found.blockStart; i != found.head; ++i)
            {
                const std::wstring_view lineText = file.lines[i].text;
                const std::wstring_view words = bodyOf(trimmed(lineText));
                if (words.starts_with(k_todo))
                    continue;
                ++found.blockWords;
                if (found.comment)
                    continue;
                found.comment = FoundComment{
                    .line = i,
                    .column = lineText.find_first_not_of(L" \t")
                };
                found.body = std::wstring{ words };
            }
            return found;
        }

        // What putting a comment's words in changed: where a line went in or out, and which way.
        struct LineChange
        {
            std::size_t at{ 0 };          // the line put in, or the line taken out
            std::ptrdiff_t count{ 0 };    // one in, one out, or none
            bool trailing{ false };       // where the words went
        };

        // The code of the declaration's line, its comment and the blanks before it taken off.
        [[nodiscard]] std::wstring codeOf(const FileLines& file, const FoundDeclaration& found)
        {
            std::wstring_view text = file.lines[found.line].text;
            if (found.comment && found.comment->trailing)
                text = text.substr(0, found.comment->column);
            return withoutTrailingBlanks(text);
        }

        // The nearest line of code over or under a line, comment lines passed over; npos for none.
        [[nodiscard]] std::size_t codeLineBeside(const FileLines& file, std::size_t line,
            const bool under)
        {
            while (under ? line + 1 < file.lines.size() : line != 0)
            {
                line = under ? line + 1 : line - 1;
                if (!isCommentOnly(file.lines[line].text))
                    return line;
            }
            return std::wstring_view::npos;
        }

        // The gap that puts a comment on the line under the comments of the lines of code round
        // it, where one of them carries one past the code; a blank otherwise.
        [[nodiscard]] std::wstring alignedGap(const FileLines& file, const std::size_t line,
            const std::wstring_view code)
        {
            for (const bool under : { false, true })
            {
                const std::size_t neighbour = codeLineBeside(file, line, under);
                if (neighbour == std::wstring_view::npos)
                    continue;
                const std::wstring_view text = file.lines[neighbour].text;
                const std::size_t column = commentStart(text);
                if (column != std::wstring_view::npos && column > code.size())
                    return std::wstring(column - code.size(), L' ');
            }
            return L" ";
        }

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

        // Puts the comment's words where the rule puts them, in the file's lines only. An empty
        // body takes the comment out. Nothing where they have no place: a declaration that reads
        // no comment above it, and a line they would run past the width of.
        [[nodiscard]] std::optional<LineChange> placeComment(FileLines& file,
            const FoundDeclaration& found, const CommentSite& site, const std::wstring& body)
        {
            const std::wstring code = codeOf(file, found);
            const bool wasTrailing = found.comment && found.comment->trailing;
            const bool wasAbove = found.comment && !found.comment->trailing;
            std::wstring slashes{ k_lineComment };
            if (found.comment)
            {
                const std::wstring_view text = file.lines[found.comment->line].text;
                slashes = slashesOf(trimmed(text.substr(found.comment->column)));
            }

            if (body.empty())
            {
                if (wasTrailing)
                    file.lines[found.line].text = code;
                if (!wasAbove)
                    return LineChange{};
                file.lines.erase(file.lines.begin()
                    + static_cast<std::ptrdiff_t>(found.comment->line));
                return LineChange{ .at = found.comment->line, .count = -1 };
            }

            // Words the comment lines above go on with keep their place above.
            const bool aboveGoesOn = wasAbove && found.blockWords > 1;
            if (!site.opensScope && !aboveGoesOn && closesItsBrackets(code))
            {
                std::wstring gap = alignedGap(file, found.line, code);
                if (wasTrailing)
                {
                    const std::wstring_view text = file.lines[found.line].text;
                    gap = text.substr(code.size(), found.comment->column - code.size());
                }
                std::wstring line = code + gap + slashes + L" " + body;
                if (columnsOf(line) > k_lineColumns && gap.size() > 1)
                    line = code + L" " + slashes + L" " + body;
                if (columnsOf(line) <= k_lineColumns)
                {
                    file.lines[found.line].text = line;
                    if (!wasAbove)
                        return LineChange{ .trailing = true };
                    file.lines.erase(file.lines.begin()
                        + static_cast<std::ptrdiff_t>(found.comment->line));
                    return LineChange{ .at = found.comment->line, .count = -1, .trailing = true };
                }
            }
            if (!site.readsAbove)
                return std::nullopt;

            if (wasAbove)
            {
                FileLine& line = file.lines[found.comment->line];
                line.text = indentOf(line.text) + slashes + L" " + body;
                return LineChange{};
            }
            if (wasTrailing)
                file.lines[found.line].text = code;
            // Under other words above, the hint would no longer be the first of them.
            const std::size_t at = found.blockWords != 0 ? found.blockStart : found.head;
            FileLine line = lineBeside(file, found.head,
                indentOf(file.lines[found.head].text) + slashes + L" " + body);
            file.lines.insert(file.lines.begin() + static_cast<std::ptrdiff_t>(at),
                std::move(line));
            return LineChange{ .at = at, .count = 1 };
        }

        // A declaration's file read, and the declaration found in it as the surface last read it.
        struct SiteInFile
        {
            std::filesystem::path path;
            FileLines file;
            FoundDeclaration declaration;
        };

        [[nodiscard]] Refusal changedSince(const std::wstring_view relative)
        {
            return std::wstring{ relative }
                + L" has changed since the tree was scanned, so nothing was written to it.";
        }

        [[nodiscard]] std::optional<SiteInFile> readSite(const Surface& surface,
            const std::filesystem::path& tree, const CommentSite& site, Refusal& refusal)
        {
            if (!site.comment || site.place.file >= surface.files().size())
            {
                refusal = L"The surface holds no comment for this.";
                return std::nullopt;
            }
            const SourceFile& source = surface.files()[site.place.file];
            const std::filesystem::path path = tree / source.path;
            std::optional<FileLines> file = readLines(path);
            if (!file)
            {
                refusal = source.relative + L" cannot be read.";
                return std::nullopt;
            }
            std::optional<FoundDeclaration> found = locate(*file, site);
            if (found && wordsOf(found->body, site.name) != site.comment->text)
                found.reset();
            if (!found)
            {
                refusal = changedSince(source.relative);
                return std::nullopt;
            }
            return SiteInFile{ .path = path, .file = std::move(*file), .declaration = *found };
        }

        [[nodiscard]] Refusal checkHint(const std::wstring_view words, const std::wstring_view name)
        {
            const bool oneLine = words.find_first_of(L"\r\n") == std::wstring_view::npos;
            if (!oneLine)
                return L"A hint is one line. Words that need more go into a footnote.";
            const bool plain = std::ranges::all_of(words, [](const wchar_t current) {
                return current >= L' ' && current <= L'~';
            });
            if (!plain)
                return L"A hint is plain ASCII, as the sources are - a dash is a spaced hyphen.";
            std::wstring cut{ words };
            if (cutReference(cut, name))
                return L"A hint is words alone. The See that reaches a footnote comes with the "
                    L"footnote.";
            return {};
        }

        // Writes the words into the comment, ending with the See named - or the one it has - and
        // brings the record and the places under it in line with the file.
        [[nodiscard]] Refusal writeComment(Surface& surface, const std::filesystem::path& tree,
            const CommentSite& site, const std::wstring& words,
            const std::optional<Reference>& reference)
        {
            Refusal refusal;
            std::optional<SiteInFile> read = readSite(surface, tree, site, refusal);
            if (!read)
                return refusal;
            Comment* record = surface.writableComment(*site.comment);
            if (!record)
                return L"The surface holds no comment for this.";
            const FoundDeclaration& found = read->declaration;
            const std::wstring see = reference
                ? seeText(*reference, site.name)
                : referenceOf(found.body, site.name);
            std::wstring body = words;
            if (!see.empty())
                body = words.empty() ? see : words + L" " + see;
            if (body == found.body)
                return {};

            const std::optional<LineChange> placed = placeComment(read->file, found, site, body);
            if (!placed)
            {
                return L"This declaration shares its line with the one above it, so its comment "
                    L"rides the line - and these words would run past "
                    + std::to_wstring(k_lineColumns) + L" columns there.";
            }
            const LineChange change = *placed;
            if (!writeLines(read->path, read->file))
                return surface.files()[site.place.file].relative + L" cannot be written.";

            record->text = words;
            if (reference)
                record->reference = *reference;
            record->trailing.reset();
            record->above.clear();
            if (change.trailing)
                record->trailing = body;
            else if (!body.empty())
                record->above.push_back(body);
            if (change.count > 0)
                surface.shiftLines(site.place.file, change.at + 1, change.count);
            else if (change.count < 0)
                surface.shiftLines(site.place.file, change.at + 2, change.count);
            return {};
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
                    blocks.push_back(std::move(blank));
                    ++i;
                    continue;
                }
                if (headingLevel(line) != 0 || isCodeLine(line))
                {
                    const NoteBlockKind kind = headingLevel(line) != 0
                        ? NoteBlockKind::Heading
                        : NoteBlockKind::Code;
                    blocks.push_back({ .kind = kind, .flowed = line, .lines = { line } });
                    ++i;
                    continue;
                }
                if (isBulletLine(line))
                {
                    NoteBlock block{
                        .kind = NoteBlockKind::Bullet,
                        .flowed = std::wstring{ k_bulletMark }
                            + std::wstring{ trimmed(line.substr(k_bulletMark.size())) },
                        .lines = { line }
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
                    .lines = { line }
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
        // marks.
        [[nodiscard]] bool opensMarkup(const std::wstring_view word)
        {
            if (word == L"-")
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
                const std::size_t level = headingLevel(text);
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
    }

    Refusal writeHint(Surface& surface, const std::filesystem::path& tree, const CommentSite& site,
        const std::wstring_view hint)
    {
        // Words left as they were are no edit, whatever they hold.
        if (!changesHint(site, hint))
            return {};
        const std::wstring words{ trimmed(hint) };
        if (Refusal refusal = checkHint(words, site.name); !refusal.empty())
            return refusal;
        return writeComment(surface, tree, site, words, std::nullopt);
    }

    bool changesHint(const CommentSite& site, const std::wstring_view hint)
    {
        return !site.comment || trimmed(hint) != site.comment->text;
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

    // THE SECTION IS COMPARED WITH WHAT WAS SHOWN before anything is written: the editor opened
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

    std::optional<NewFootnote> placeFootnote(Notes& notes, const CommentSite& site,
        const std::wstring_view note, const std::wstring_view after,
        const std::wstring_view owner, Refusal& refusal)
    {
        if (note.empty())
        {
            refusal = L"No note under .seedocs/footnotes belongs to this chapter. Make one, then "
                L"add the footnote.";
            return std::nullopt;
        }
        if (!notes.exists(note))
        {
            refusal = noteFileName(note) + L" is not among the footnotes.";
            return std::nullopt;
        }
        const std::wstring_view name = bareName(site.name);
        std::wstring heading{ name };
        std::wstring anchor = slug(heading);
        if (notes.reaches(note, anchor) && !owner.empty())
        {
            heading = std::wstring{ owner } + L" " + std::wstring{ name };
            anchor = slug(heading);
        }
        if (notes.reaches(note, anchor))
        {
            refusal = noteFileName(note) + L" already holds a section reached as #" + anchor + L".";
            return std::nullopt;
        }
        return NewFootnote{
            .reference = { .note = std::wstring{ note }, .anchor = std::move(anchor) },
            .after = std::wstring{ after },
            .heading = std::move(heading)
        };
    }

    // THE COMMENT IS READ BEFORE THE NOTE IS WRITTEN, so a source file that changed since the
    // scan leaves no section behind that nothing reaches.
    Refusal addFootnote(Surface& surface, Notes& notes, const std::filesystem::path& tree,
        const CommentSite& site, const NewFootnote& placed, const std::wstring_view flowed)
    {
        const Lines body = bodyLines(flowed, {});
        if (Refusal refusal = checkBody(body, k_footnoteLevel); !refusal.empty())
            return refusal;
        Refusal refusal;
        if (!readSite(surface, tree, site, refusal))
            return refusal;

        const std::wstring fileName = noteFileName(placed.reference.note);
        const std::filesystem::path path = notePath(notes, placed.reference.note);
        std::optional<FileLines> file = readLines(path);
        if (!file)
            return fileName + L" cannot be read.";
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

        refusal = writeComment(surface, tree, site, site.comment->text, placed.reference);
        if (!refusal.empty())
            return L"The footnote stands in " + fileName + L", and the comment did not take the "
                L"See that reaches it: " + refusal;
        return {};
    }
}
