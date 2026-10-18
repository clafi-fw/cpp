module SeeDocs_App.Pages;

import SeeDocs_App.Database;
import SeeDocs_App.Notes;
import SeeDocs_App.Surface;

import ClaFi.Core.Syntax.Languages;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    namespace
    {
        namespace Syntax = ::ClaFi::Syntax;

        constexpr std::wstring_view k_matchedByName =
            L"matched by name - the comment carries no See";
        constexpr std::wstring_view k_controlWord = L"control";
        constexpr std::wstring_view k_noteExtension = L".md";
        constexpr std::wstring_view k_codeFence = L"    ";
        constexpr std::wstring_view k_bulletMark = L"- ";
        constexpr std::wstring_view k_scope = L"::";
        constexpr std::wstring_view k_separator = L" \u00B7 ";
        constexpr std::wstring_view k_baseJoin = L": ";
        constexpr std::wstring_view k_aliasJoin = L"= ";
        constexpr std::wstring_view k_colonJoin = L" : ";   // before a type's bases, an enum's base
        constexpr std::wstring_view k_listJoin = L", ";
        constexpr std::wstring_view k_cppIndent = L"    ";
        constexpr std::wstring_view k_pascalIndent = L"  ";
        constexpr std::wstring_view k_ownLabel = L"Own";
        constexpr std::wstring_view k_fromLabel = L"From ";
        constexpr std::wstring_view k_setterPrefix = L"set";
        constexpr std::wstring_view k_constWord = L"const";
        constexpr std::wstring_view k_fixedWord = L"fixed";
        constexpr std::wstring_view k_byWord = L"by\u00A0";   // kept on the name's line
        constexpr std::wstring_view k_valueMark = L"=\u00A0";   // kept on the value's line
        constexpr std::wstring_view k_sourceJoin = L" \u203A ";
        constexpr std::wstring_view k_publicMethods = L"Public methods";
        constexpr std::wstring_view k_protectedMethods = L"Protected methods";
        constexpr std::wstring_view k_typeNoteHeading = L"Details";
        constexpr std::wstring_view k_templateWord = L"template";
        constexpr std::wstring_view k_methodWord = L"method";
        constexpr std::wstring_view k_accessMark = L":";
        constexpr std::wstring_view k_overloadNoun = L"overload";
        constexpr std::wstring_view k_exportsColumn = L"Exports";
        constexpr std::wstring_view k_projectWord = L"project";
        constexpr std::wstring_view k_folderWord = L"folder";
        constexpr std::wstring_view k_sourceLabel = L"source";
        constexpr std::wstring_view k_moduleWord = L"module";
        constexpr std::wstring_view k_headerWord = L"header";
        constexpr std::wstring_view k_functionWord = L"function";
        constexpr std::wstring_view k_constantWord = L"constant";
        constexpr std::wstring_view k_variableWord = L"variable";
        constexpr std::wstring_view k_signatureJoin = L"\n";   // after the template head
        constexpr std::wstring_view k_tooltipJoin = L" ";      // the same on a tooltip's one line
        constexpr std::size_t k_maxTreeDepth = 16;   // what a base chain is followed to
        constexpr std::size_t k_valueLines = 3;      // a value cell's height, in lines

        // A table's columns, and whether they are named over its rows.
        struct ColumnSet
        {
            TableColumns columns;
            bool header{ true };
        };

        // The columns of each table a page carries, unnamed where a reader tells them apart.
        namespace Columns
        {
            const ColumnSet types = {
                .columns = { { L"Name" }, { L"Hint", true, true } },
                .header = false
            };
            const ColumnSet properties = {
                .columns = {
                    { L"Name" }, { L"Type" }, { .name = L"Default", .maxLines = k_valueLines },
                    { L"Hint", true, true }
                }
            };
            const ColumnSet events = {
                .columns = { { L"Event" }, { L"Type" }, { L"Hint", true, true } }
            };
            const ColumnSet methods = {
                .columns = { { L"Name" }, { L"Hint", true, true } },
                .header = false
            };
            const ColumnSet functions = {
                .columns = { { L"Name" }, { L"Hint", true, true } },
                .header = false
            };
            const ColumnSet fields = {
                .columns = {
                    { L"Name" }, { L"Type" }, { .name = L"Value", .maxLines = k_valueLines },
                    { L"Hint", true, true }
                }
            };
            const ColumnSet members = {
                .columns = {
                    { L"Name" }, { .name = L"Value", .maxLines = k_valueLines },
                    { L"Hint", true, true }
                },
                .header = false
            };
            const ColumnSet folders = {
                .columns = {
                    { L"Folder", true }, { L"Modules" }, { L"Types" }, { L"Controls" }
                }
            };
            // A folder's types, functions and constants, under the constants' columns.
            const ColumnSet contents = {
                .columns = {
                    { L"Name" }, { L"Type" }, { .name = L"Value", .maxLines = k_valueLines },
                    { L"Hint", true, true }
                },
                .header = false
            };
        }

        [[nodiscard]] std::wstring lowered(const std::wstring_view text)
        {
            std::wstring result;
            for (const wchar_t current : text)
                result += static_cast<wchar_t>(std::towlower(current));
            return result;
        }

        // Whether a folder's path lists before another's as a file manager lists the tree: folder
        // by folder by name regardless of case, a folder ahead of the folders under it.
        [[nodiscard]] bool listsBefore(std::wstring_view left, std::wstring_view right)
        {
            while (!left.empty() && !right.empty())
            {
                const std::wstring_view leftFolder = left.substr(0, left.find(L'/'));
                const std::wstring_view rightFolder = right.substr(0, right.find(L'/'));
                const std::wstring leftName = lowered(leftFolder);
                const std::wstring rightName = lowered(rightFolder);
                if (leftName != rightName)
                    return leftName < rightName;
                left.remove_prefix(std::min(left.size(), leftFolder.size() + 1));
                right.remove_prefix(std::min(right.size(), rightFolder.size() + 1));
            }
            return left.empty() && !right.empty();
        }

        // Whether a folder stands under another, at any depth; every folder stands under the
        // root's empty path.
        [[nodiscard]] bool standsUnder(const std::wstring_view folder,
            const std::wstring_view above)
        {
            if (above.empty())
                return !folder.empty();
            return folder.starts_with(above) && folder.size() > above.size()
                && folder[above.size()] == L'/';
        }

        [[nodiscard]] std::size_t kindRank(const TypeKind kind)
        {
            switch (kind)
            {
                case TypeKind::Class:
                    return 0;
                case TypeKind::Interface:
                    return 1;
                case TypeKind::Struct:
                case TypeKind::Union:
                    return 2;
                case TypeKind::Enum:
                    return 3;
                case TypeKind::Alias:
                case TypeKind::Defined:
                    return 4;
                case TypeKind::Concept:
                    return 5;
            }
            return 6;
        }

        [[nodiscard]] bool readsBefore(const Type& left, const Type& right)
        {
            const auto key = [](const Type& type) {
                return std::tuple{ !type.isControl, kindRank(type.kind), lowered(type.name) };
            };
            return key(left) < key(right);
        }

        // Whether a type stands inside another - a part of it rather than a type of its own.
        [[nodiscard]] bool isNested(const Type& type)
        {
            return type.name.find(k_scope) != std::wstring::npos;
        }

        [[nodiscard]] bool hasControl(const ContentsModule& module)
        {
            return std::ranges::any_of(module.types, [](const Type* type) {
                return type->isControl;
            });
        }

        [[nodiscard]] std::wstring countOf(const std::size_t value, const std::wstring_view noun)
        {
            return std::to_wstring(value) + L" " + std::wstring{ noun } + (value == 1 ? L"" : L"s");
        }

        // Runs

        [[nodiscard]] Run plain(std::wstring text, std::wstring link = {})
        {
            return { .text = std::move(text), .link = std::move(link) };
        }

        [[nodiscard]] Run code(std::wstring text, std::wstring link = {})
        {
            return { .text = std::move(text), .style = RunStyle::Code, .link = std::move(link) };
        }

        [[nodiscard]] Run pathRun(std::wstring text)
        {
            return { .text = std::move(text), .style = RunStyle::Path };
        }

        [[nodiscard]] Run muted(std::wstring text)
        {
            return { .text = std::move(text), .style = RunStyle::Muted };
        }

        [[nodiscard]] Run bold(std::wstring text)
        {
            return { .text = std::move(text), .style = RunStyle::Bold };
        }

        [[nodiscard]] Run missing(std::wstring text)
        {
            return { .text = std::move(text), .style = RunStyle::Missing };
        }

        // Where a declaration's comment is written, for a page that lets its words be edited.
        [[nodiscard]] CommentSite siteOf(const Comment& comment, const Place& place,
            std::wstring name, const bool opensScope = false)
        {
            return {
                .comment = &comment,
                .place = place,
                .name = std::move(name),
                .opensScope = opensScope
            };
        }

        [[nodiscard]] CommentSite siteOf(const Type& type)
        {
            return siteOf(type.comment, type.place, type.name, type.opensScope());
        }

        // A member on its enum's own line has no comment above it - the one there is the enum's.
        [[nodiscard]] CommentSite memberSite(const Type& type, const EnumMember& member)
        {
            CommentSite site = siteOf(member.comment, member.place, member.name);
            site.readsAbove = member.place.line != type.place.line;
            return site;
        }

        // Names in a text

        [[nodiscard]] bool isNameStart(const wchar_t current)
        {
            return std::iswalpha(current) != 0 || current == L'_';
        }

        [[nodiscard]] bool isNameChar(const wchar_t current)
        {
            return std::iswalnum(current) != 0 || current == L'_';
        }

        // Where a name stands in a text: letters, digits and underscores, parts joined by ::.
        struct NameSpan
        {
            std::size_t start{ 0 };
            std::size_t end{ 0 };
        };

        // Whether a scope mark with a name part after it stands at the index.
        [[nodiscard]] bool scopedNameAt(const std::wstring_view text, const std::size_t at)
        {
            return text.substr(at).starts_with(k_scope) && at + k_scope.size() < text.size()
                && isNameStart(text[at + k_scope.size()]);
        }

        // The first name at or after the index; a word opening with a digit is no name and is
        // passed over whole.
        [[nodiscard]] std::optional<NameSpan> nextName(const std::wstring_view text,
            std::size_t from)
        {
            while (from < text.size())
            {
                const bool scoped = scopedNameAt(text, from);
                if (!isNameStart(text[from]) && !scoped)
                {
                    const bool digit = isNameChar(text[from]);
                    ++from;
                    while (digit && from < text.size() && isNameChar(text[from]))
                        ++from;
                    continue;
                }
                NameSpan span{ .start = from, .end = scoped ? from + k_scope.size() : from };
                while (true)
                {
                    while (span.end < text.size() && isNameChar(text[span.end]))
                        ++span.end;
                    if (!scopedNameAt(text, span.end))
                        return span;
                    span.end += k_scope.size();
                }
            }
            return std::nullopt;
        }

        // The run with every name in it the surface has a page for linked to it, cut round each;
        // the names left out are never linked - a template's parameters.
        void linkNamesIn(Runs& out, const Run& run, const Surface& surface,
            const std::wstring_view nameSpace, const Names& leftOut)
        {
            const std::wstring_view text = run.text;
            std::size_t plainStart = 0;
            std::size_t at = 0;
            while (const std::optional<NameSpan> span = nextName(text, at))
            {
                at = span->end;
                const std::wstring_view name = text.substr(span->start, span->end - span->start);
                const bool left = std::ranges::any_of(leftOut, [name](const std::wstring& out) {
                    return out == name;
                });
                if (left)
                    continue;
                const Type* type = surface.publicType(name, nameSpace);
                if (!type)
                    continue;
                if (span->start != plainStart)
                {
                    out.push_back({
                        .text = std::wstring{ text.substr(plainStart, span->start - plainStart) },
                        .style = run.style
                    });
                }
                out.push_back({
                    .text = std::wstring{ name },
                    .style = run.style,
                    .link = type->qualifiedName
                });
                plainStart = span->end;
            }
            if (plainStart == 0)
            {
                out.push_back(run);
                return;
            }
            if (plainStart != text.size())
            {
                out.push_back({
                    .text = std::wstring{ text.substr(plainStart) },
                    .style = run.style
                });
            }
        }

        // The runs with the types they name linked; a run already linked, a muted one and a
        // missing one stand as they are.
        [[nodiscard]] Runs linkNames(const Surface& surface, const Runs& runs,
            const std::wstring_view nameSpace, const Names& leftOut = {})
        {
            Runs linked;
            for (const Run& run : runs)
            {
                const bool asIs = !run.link.empty() || run.style == RunStyle::Muted
                    || run.style == RunStyle::Missing;
                if (asIs)
                    linked.push_back(run);
                else
                    linkNamesIn(linked, run, surface, nameSpace, leftOut);
            }
            return linked;
        }

        // A spelled type, linked to its page where the surface has one.
        [[nodiscard]] Run typeRun(const Surface& surface, const std::wstring& spelled,
            const std::wstring_view nameSpace)
        {
            const Type* type = surface.publicType(spelled, nameSpace);
            if (type)
                return code(spelled, type->qualifiedName);
            return code(spelled);
        }

        [[nodiscard]] Run nameRun(const Type& type)
        {
            return code(type.name, type.qualifiedName);
        }

        // A value cell: the initializer as written, after an equals sign; nothing where none.
        [[nodiscard]] Runs valueCell(const std::wstring& value)
        {
            if (value.empty())
                return {};
            return { muted(std::wstring{ k_valueMark } + value) };
        }

        // The kind a type reads as in a table, after its name - a control before anything else.
        [[nodiscard]] Runs qualifierOf(const Type& type)
        {
            if (type.isControl)
                return { muted(std::wstring{ k_controlWord }) };
            return { muted(std::wstring{ kindWord(type.kind) }) };
        }

        // Prose

        // The words of a note line: backticks set a run in code, a pair of ** sets one bold, and
        // a type named in any of them is linked.
        [[nodiscard]] Runs inlineRuns(const std::wstring& text, const Surface& surface,
            const std::wstring_view nameSpace)
        {
            Runs runs;
            std::wstring words;
            const auto flush = [&]() {
                if (!words.empty())
                    runs.push_back(plain(std::exchange(words, {})));
            };
            std::size_t i = 0;
            while (i < text.size())
            {
                if (text[i] == k_codeMark)
                {
                    const std::size_t close = text.find(k_codeMark, i + 1);
                    if (close != std::wstring::npos)
                    {
                        flush();
                        runs.push_back(code(text.substr(i + 1, close - i - 1)));
                        i = close + 1;
                        continue;
                    }
                }
                if (std::wstring_view{ text }.substr(i).starts_with(k_boldMark))
                {
                    const std::size_t close = text.find(k_boldMark, i + k_boldMark.size());
                    if (close != std::wstring::npos)
                    {
                        flush();
                        const std::size_t start = i + k_boldMark.size();
                        runs.push_back(bold(text.substr(start, close - start)));
                        i = close + k_boldMark.size();
                        continue;
                    }
                }
                words += text[i];
                ++i;
            }
            flush();
            return linkNames(surface, runs, nameSpace);
        }

        // The hint read as a note line is, in a namespace. A tree read by loose rules owes no
        // hint, so one it does not give is nothing rather than missing.
        [[nodiscard]] Runs hintRuns(const Surface& surface, const Comment& comment,
            const std::wstring_view nameSpace)
        {
            if (!comment.text.empty())
                return inlineRuns(comment.text, surface, nameSpace);
            if (surface.hintRules() == HintRules::Loose)
                return {};
            return { missing(std::wstring{ k_noHint }) };
        }

        [[nodiscard]] bool isCodeLine(const std::wstring_view line)
        {
            return line.starts_with(k_codeFence) && !trimmed(line).empty();
        }

        [[nodiscard]] bool isBulletLine(const std::wstring_view line)
        {
            return line.starts_with(k_bulletMark);
        }

        // The lines of a section as blocks: paragraphs between blank lines, a heading inside the
        // section as a subheading, four-space lines as code, dashed lines as bullets; the types
        // named in any of them read in the namespace and link.
        [[nodiscard]] Blocks blocksOf(const Lines& lines, const Surface& surface,
            const std::wstring_view nameSpace)
        {
            Blocks blocks;
            std::wstring paragraph;
            const auto runsOf = [&](const std::wstring& text) {
                return inlineRuns(text, surface, nameSpace);
            };
            const auto flush = [&]() {
                if (!paragraph.empty())
                {
                    blocks.push_back({
                        .kind = BlockKind::Paragraph,
                        .runs = runsOf(std::exchange(paragraph, {}))
                    });
                }
            };
            std::size_t i = 0;
            while (i < lines.size())
            {
                const std::wstring& line = lines[i];
                const std::wstring_view words = trimmed(line);
                if (words.empty())
                {
                    flush();
                    ++i;
                    continue;
                }
                if (const std::size_t heading = headingLevel(line); heading != 0)
                {
                    flush();
                    blocks.push_back({
                        .kind = BlockKind::SubHeading,
                        .runs = runsOf(std::wstring{ trimmed(words.substr(heading)) })
                    });
                    ++i;
                    continue;
                }
                if (isCodeLine(line) && paragraph.empty())
                {
                    flush();
                    Block block{ .kind = BlockKind::Code };
                    while (i < lines.size())
                    {
                        const bool blank = trimmed(lines[i]).empty();
                        const bool continues = i + 1 < lines.size() && isCodeLine(lines[i + 1]);
                        if (!isCodeLine(lines[i]) && !(blank && continues))
                            break;
                        block.lines.push_back(blank
                            ? Runs{}
                            : linkNames(surface, { code(lines[i].substr(k_codeFence.size())) },
                                nameSpace));
                        ++i;
                    }
                    blocks.push_back(std::move(block));
                    continue;
                }
                if (isBulletLine(line))
                {
                    flush();
                    Block block{ .kind = BlockKind::Bullets };
                    std::wstring item;
                    while (i < lines.size())
                    {
                        const std::wstring& current = lines[i];
                        if (isBulletLine(current))
                        {
                            if (!item.empty())
                                block.items.push_back(runsOf(std::exchange(item, {})));
                            item = std::wstring{ trimmed(current.substr(k_bulletMark.size())) };
                        }
                        else if (current.starts_with(L"  ") && !trimmed(current).empty())
                        {
                            item += L' ';
                            item += trimmed(current);
                        }
                        else
                            break;
                        ++i;
                    }
                    if (!item.empty())
                        block.items.push_back(runsOf(item));
                    blocks.push_back(std::move(block));
                    continue;
                }
                if (!paragraph.empty())
                    paragraph += L' ';
                paragraph += words;
                ++i;
            }
            flush();
            return blocks;
        }

        // The section a declaration's words come from, and the note it stands in.
        struct FoundNote
        {
            const Note* note{ nullptr };
            const NoteSection* section{ nullptr };
            bool byName{ false };
        };

        // The note a folder's declarations cite the most, through the See references their
        // comments and their members' carry; none where they cite no note that exists.
        [[nodiscard]] std::wstring citedNote(const Surface& surface, Notes& notes,
            const std::wstring_view folder)
        {
            std::map<std::wstring_view, std::size_t> counts;
            const auto cite = [&counts, &notes](const Comment& comment) {
                if (comment.reference && notes.exists(comment.reference->note))
                    ++counts[comment.reference->note];
            };
            const auto folderAt = [&surface](const Place& place) {
                return folderOf(surface.files()[place.file].relative);
            };
            for (const Type& type : surface.types())
            {
                if (type.folder != folder)
                    continue;
                cite(type.comment);
                for (const Property& property : type.properties)
                    cite(property.comment);
                for (const Event& event : type.events)
                    cite(event.comment);
                for (const Function& function : type.functions)
                    cite(function.comment);
                for (const Using& declared : type.usings)
                    cite(declared.comment);
                for (const Field& field : type.fields)
                    cite(field.comment);
                for (const EnumMember& member : type.members)
                    cite(member.comment);
            }
            for (const FreeFunction& free : surface.functions())
            {
                if (folderAt(free.function.place) == folder)
                    cite(free.function.comment);
            }
            for (const Variable& variable : surface.variables())
            {
                if (folderAt(variable.field.place) == folder)
                    cite(variable.field.comment);
            }
            std::wstring best;
            std::size_t bestCount = 0;
            for (const auto& [stem, count] : counts)
            {
                if (count > bestCount)
                {
                    best = stem;
                    bestCount = count;
                }
            }
            return best;
        }

        // The note a folder's footnotes go into: the one its declarations cite the most, else the
        // one named after the folder's own name, made with the first footnote. The root's is its
        // own.
        [[nodiscard]] std::wstring folderNote(const Surface& surface, Notes& notes,
            const std::wstring_view folder)
        {
            if (folder.empty())
                return std::wstring{ k_rootNote };
            std::wstring cited = citedNote(surface, notes, folder);
            if (!cited.empty())
                return cited;
            return folderNameOf(folder);
        }

        // The section named after a declaration: a heading spelling it in the folder's note - the
        // same name in another note is another thing's.
        [[nodiscard]] const NoteHit* namedHit(const Surface& surface, Notes& notes,
            const NoteHits& hits, const std::wstring_view heading, const std::wstring_view folder)
        {
            const auto spells = [heading](const NoteHit& hit) {
                return hit.section->heading == heading;
            };
            if (std::ranges::none_of(hits, spells))
                return nullptr;
            const std::wstring home = folderNote(surface, notes, folder);
            for (const NoteHit& hit : hits)
            {
                if (spells(hit) && hit.note->stem == home)
                    return &hit;
            }
            return nullptr;
        }

        // The section the comment references; failing a reference, the section headed as given,
        // the way footnoteHeading spells it - none for no heading.
        [[nodiscard]] FoundNote noteFor(const Surface& surface, Notes& notes,
            const Comment& comment, const std::wstring_view name, const std::wstring_view folder,
            const std::wstring_view heading)
        {
            if (comment.reference)
            {
                const Reference& reference = *comment.reference;
                const std::wstring anchor = reference.anchor.empty()
                    ? slug(name)
                    : reference.anchor;
                return {
                    .note = notes.note(reference.note),
                    .section = notes.section(reference.note, anchor)
                };
            }
            if (heading.empty())
                return {};
            const NoteHits hits = notes.sectionsNamed(slug(heading));
            const NoteHit* hit = namedHit(surface, notes, hits, heading, folder);
            if (!hit)
                return {};
            return { .note = hit->note, .section = hit->section, .byName = true };
        }

        // A found section as the words a page shows, named by where they come from, the types
        // they name read in the namespace of the declaration they belong to.
        [[nodiscard]] std::optional<Excerpt> excerptOf(const FoundNote& found,
            const Surface& surface, const std::wstring_view nameSpace)
        {
            if (!found.section)
                return std::nullopt;
            Excerpt excerpt;
            excerpt.from = { .note = found.note->stem, .anchor = found.section->anchor };
            excerpt.byName = found.byName;
            excerpt.source = found.note->stem + std::wstring{ k_noteExtension }
                + std::wstring{ k_sourceJoin } + found.section->heading;
            if (found.byName)
            {
                excerpt.source += k_separator;
                excerpt.source += k_matchedByName;
            }
            excerpt.blocks = blocksOf(found.section->lines, surface, nameSpace);
            return excerpt;
        }

        // The note a member's comment references, or the one named after it in its owner's
        // folder, under its row or on its page.
        [[nodiscard]] std::optional<Excerpt> memberExcerpt(const Surface& surface, Notes& notes,
            const Comment& comment, const std::wstring_view name, const Type& owner)
        {
            const std::wstring heading = footnoteHeading(owner.name, name);
            return excerptOf(noteFor(surface, notes, comment, name, owner.folder, heading), surface,
                owner.nameSpace);
        }

        // Sections

        Section& addSection(Page& page, const SectionKind kind, Runs heading,
            std::wstring tooltip = {})
        {
            page.sections.push_back({
                .kind = kind,
                .heading = std::move(heading),
                .tooltip = std::move(tooltip)
            });
            return page.sections.back();
        }

        // A table of rows under one heading or none, counted in the heading's tooltip.
        Section& addTableSection(Page& page, std::wstring heading, const ColumnSet& columnSet,
            TableGroups groups)
        {
            Section& section = addSection(page, SectionKind::Table, { plain(std::move(heading)) });
            section.table.columns = columnSet.columns;
            section.table.header = columnSet.header;
            section.table.groups = std::move(groups);
            section.tooltip = countOf(section.table.rowCount(), L"element");
            return section;
        }

        void addFact(Page& page, std::wstring label, Runs value)
        {
            page.facts.push_back({ .label = std::move(label), .value = std::move(value) });
        }

        // Empty where the project no longer reads the file's source.
        [[nodiscard]] std::wstring fullPathOf(const SourceRoots& roots, const SourceFile& file)
        {
            if (file.root >= roots.size())
                return {};
            return (roots[file.root] / file.path).lexically_normal().wstring();
        }

        // A file by its path under its source's folder, atLine after it as `:42` or empty.
        void addSourceFact(Page& page, const SourceRoots& roots, const SourceFile& file,
            const std::wstring_view atLine, const SourcePlace& source)
        {
            std::wstring fullValue = fullPathOf(roots, file);
            if (!fullValue.empty())
                fullValue += atLine;
            page.facts.push_back({
                .label = std::wstring{ k_sourceLabel },
                .value = { pathRun(file.relative + std::wstring{ atLine }) },
                .fullValue = std::move(fullValue),
                .source = source
            });
        }

        // The file and line a declaration stands at.
        void addPlaceFact(Page& page, const Surface& surface, const SourceRoots& roots,
            const Place& place, const SourceLanguage language)
        {
            addSourceFact(page, roots, surface.files()[place.file],
                L":" + std::to_wstring(place.line), { .place = place, .language = language });
        }

        // What a reader calls the unit a declaration stands in: a header, which a limited read
        // takes for a module, or a module.
        [[nodiscard]] std::wstring moduleWordOf(const Surface& surface,
            const std::wstring_view name)
        {
            const auto isThisHeader = [name](const Module& module) {
                return module.isHeader && module.name == name;
            };
            const bool header = std::ranges::any_of(surface.modules(), isThisHeader);
            return std::wstring{ header ? k_headerWord : k_moduleWord };
        }

        // The unit declaring a module - its interface, a header or a Delphi unit; null for none.
        [[nodiscard]] const Module* interfaceOf(const Surface& surface,
            const std::wstring_view name)
        {
            for (const Module& module : surface.modules())
            {
                if (module.isInterface && module.name == name)
                    return &module;
            }
            return nullptr;
        }

        // The folder under every source holding a file at or below it.
        void addFolderSources(Page& page, const Surface& surface, const SourceRoots& roots,
            const std::wstring_view path)
        {
            std::vector<bool> holds(roots.size(), false);
            for (const SourceFile& file : surface.files())
            {
                if (file.root >= roots.size())
                    continue;
                const std::wstring folder = folderOf(file.relative);
                if (folder == path || standsUnder(folder, path))
                    holds[file.root] = true;
            }
            for (std::size_t root = 0; root != roots.size(); ++root)
            {
                if (!holds[root])
                    continue;
                const std::filesystem::path onDisk =
                    (roots[root] / std::filesystem::path{ path }).lexically_normal();
                page.facts.push_back({
                    .label = std::wstring{ k_sourceLabel },
                    .value = { pathRun(std::wstring{ path }) },
                    .fullValue = onDisk.wstring(),
                    .folder = onDisk
                });
            }
        }

        // The head of a type page

        void addTypeHead(Page& page, const Surface& surface, const SourceRoots& roots,
            const Type& type)
        {
            page.title = type.name;
            page.badges.push_back(muted(std::wstring{ kindWord(type.kind) }));
            if (type.isControl)
            {
                page.badges.push_back(muted(std::wstring{ k_separator }
                    + std::wstring{ k_controlWord }));
            }
            if (!type.templateParameters.empty())
            {
                page.badges.push_back(muted(std::wstring{ k_separator } + L"template <"
                    + type.templateParameters + L">"));
            }
            page.lead = hintRuns(surface, type.comment, type.nameSpace);
            page.leadSite = siteOf(type);
            addFact(page, L"namespace", { code(type.nameSpace) });
            addFact(page, moduleWordOf(surface, type.module), { code(type.module, type.module) });
            addPlaceFact(page, surface, roots, type.place, surface.languageOf(type.module));
            if (type.target.empty())
                return;
            if (type.kind == TypeKind::Concept)
            {
                addFact(page, L"requires", { code(type.target) });
                return;
            }
            const bool equated = type.kind == TypeKind::Alias || type.kind == TypeKind::Defined;
            const std::wstring_view join = equated ? k_aliasJoin : k_baseJoin;
            Runs line = { muted(std::wstring{ join }) };
            const Runs target = linkNames(surface, { code(type.target) }, type.nameSpace,
                templateParameterNames(type.templateParameters));
            line.insert(line.end(), target.begin(), target.end());
            page.titleLines.push_back(std::move(line));
        }

        // The ways up through the bases

        using NameSet = std::set<std::wstring>;
        using AccessByName = std::map<std::wstring, Access>;

        // How many parameters a function declares, and whether it is const - what tells the
        // overloads of one name apart.
        struct Shape
        {
            std::size_t parameters{ 0 };
            bool isConst{ false };
        };

        // A Delphi routine's parameters are counted by the language's own reading, a group of
        // names before one type being several; no routine of it is const.
        [[nodiscard]] Shape shapeOf(const Function& function, const SourceLanguage language)
        {
            const std::wstring_view signature = function.signature;
            if (language == SourceLanguage::Pascal)
            {
                const Syntax::SignatureParameters parameters =
                    Syntax::Languages::pascal.parameters(signature);
                return { .parameters = parameters.names.size() };
            }
            std::size_t open = signature.find(function.name + L"(");
            if (open == std::wstring_view::npos)
                open = signature.find(L'(');
            else
                open += function.name.size();
            if (open == std::wstring_view::npos)
                return {};
            int depth = 0;
            for (std::size_t i = open; i != signature.size(); ++i)
            {
                if (signature[i] == L'(')
                    ++depth;
                if (signature[i] == L')')
                {
                    --depth;
                    if (depth == 0)
                    {
                        const std::wstring_view trailing = signature.substr(i + 1);
                        return {
                            .parameters =
                                splitArguments(signature.substr(open + 1, i - open - 1)).size(),
                            .isConst = trailing.find(k_constWord) != std::wstring_view::npos
                        };
                    }
                }
            }
            return {};
        }

        // The name, the parameter count and the constness as one key - an overload's identity.
        [[nodiscard]] std::wstring shapeKey(const Function& function, const SourceLanguage language)
        {
            const Shape shape = shapeOf(function, language);
            return function.name + L'/' + std::to_wstring(shape.parameters)
                + (shape.isConst ? L"c" : L"");
        }

        // A using-declaration of a type on the way, with the base it names resolved.
        struct ResolvedUsing
        {
            const Type* base{ nullptr };
            const Using* declaration{ nullptr };
        };

        using ResolvedUsings = std::vector<ResolvedUsing>;

        // What the types below hand a base: their method names and shapes, their usings.
        struct Flow
        {
            NameSet hidden;               // the method names declared below
            NameSet shapes;               // the shape keys declared below
            AccessByName republished;     // names republished below that no type between declares
            ResolvedUsings usings;        // each waiting for the base it names
            bool constructors{ false };   // whether the constructors reaching here are the type's
        };

        // A base of the surface the ways up reach, as the chain spells it, with what reaches it.
        struct ReachedBase
        {
            std::wstring spelling;
            const Type* type{ nullptr };
            NameSet hidden;                        // its method names a type below declares again
            NameSet shapes;                        // its overloads a type below declares again
            AccessByName republished;              // its methods a type below names with using
            bool constructorsInherited{ false };   // a type below takes its constructors with using
        };

        using ReachedBases = std::vector<ReachedBase>;

        // A type on the way up, as the chain spelled it.
        struct WayStep
        {
            const Type* type{ nullptr };
            std::wstring spelling;
        };

        // The walk up through a type's bases: the tree of them it builds, the way so far, and every
        // base it reaches, once each, in the order the tree names them reading down.
        struct BaseWalk
        {
            BaseNodes bases;
            std::vector<WayStep> way;
            ReachedBases reached;
        };

        // A base's spelling and the namespace it is read in.
        struct BaseReading
        {
            std::wstring spelling;
            std::wstring_view nameSpace;
        };

        // Along a type's bases, a node each into the list given: a base that is one of the type's
        // template parameters stands for the argument the spelling gave it, which is how a mixin
        // hands its host on.
        void addBaseNodes(BaseWalk& walk, BaseNodes& into, const Surface& surface,
            const Type& type, std::wstring_view spelled, std::wstring_view nameSpace,
            std::size_t depth, const Flow& flow);

        // Where a base spelled inside a type is read from on the chain: a template parameter of
        // the type stands for the argument the chain's spelling gave it, read where the chain is;
        // nothing where the spelling gives no argument for it.
        [[nodiscard]] std::optional<BaseReading> readBase(const Type& type,
            const std::wstring_view spelled, const std::wstring_view nameSpace,
            const std::wstring& base)
        {
            const Names parameters = templateParameterNames(type.templateParameters);
            const std::wstring_view head = plainType(withoutArguments(base));
            const auto parameter = std::ranges::find(parameters, head);
            if (parameter == parameters.end())
                return BaseReading{ .spelling = base, .nameSpace = type.nameSpace };
            const Names arguments = argumentsOf(spelled);
            const std::size_t index = static_cast<std::size_t>(parameter - parameters.begin());
            if (index >= arguments.size())
                return std::nullopt;
            return BaseReading{ .spelling = arguments[index], .nameSpace = nameSpace };
        }

        // The type a reading names, a private alias followed to what it names.
        [[nodiscard]] const Type* typeOf(const Surface& surface, const BaseReading& reading)
        {
            return surface.resolve(surface.resolvedBase(reading.spelling, reading.nameSpace),
                reading.nameSpace);
        }

        // What a type hands its bases: its method names over what reached it, its
        // using-declarations after those of the types below, the constructors as they reached it.
        // A Delphi constructor is inherited as a method is, and hides as one does.
        [[nodiscard]] Flow flowFrom(const Surface& surface, const Type& type,
            const std::wstring_view spelled, const std::wstring_view nameSpace, const Flow& into)
        {
            const SourceLanguage language = surface.languageOf(type.module);
            const bool constructorsHide = language == SourceLanguage::Pascal;
            Flow flow = into;
            for (const Function& function : type.functions)
            {
                const bool hides = function.kind == FunctionKind::Function
                    || (constructorsHide && function.kind == FunctionKind::Constructor);
                if (!hides)
                    continue;
                flow.hidden.insert(function.name);
                flow.shapes.insert(shapeKey(function, language));
            }
            for (const Using& declaration : type.usings)
            {
                const std::optional<BaseReading> reading =
                    readBase(type, spelled, nameSpace, declaration.base);
                const Type* base = reading ? typeOf(surface, *reading) : nullptr;
                if (base)
                    flow.usings.push_back({ .base = base, .declaration = &declaration });
            }
            return flow;
        }

        // What the way below settles about a base it reaches: a using-declaration naming it takes
        // its constructors or republishes a method, and a republished name is not hidden - its
        // overloads of a shape declared below still are.
        [[nodiscard]] ReachedBase reachedBase(std::wstring spelling, const Type& type,
            const Flow& flow)
        {
            ReachedBase base{
                .spelling = std::move(spelling),
                .type = &type,
                .hidden = flow.hidden,
                .shapes = flow.shapes,
                .republished = flow.republished
            };
            for (const ResolvedUsing& used : flow.usings)
            {
                if (used.base != &type)
                    continue;
                if (inheritsConstructors(*used.declaration))
                {
                    base.constructorsInherited = flow.constructors;
                    continue;
                }
                base.hidden.erase(used.declaration->name);
                base.republished[used.declaration->name] = used.declaration->access;
            }
            return base;
        }

        // The republished names that reach past a type: those it declares no function for, since a
        // using-declaration brings what the named base sees, which a declaration of its own hides.
        [[nodiscard]] AccessByName carriedPast(const AccessByName& republished, const Type& type)
        {
            AccessByName carried;
            for (const auto& [name, access] : republished)
            {
                const bool declared = std::ranges::any_of(type.functions,
                    [&name](const Function& function) {
                        return function.name == name;
                    });
                if (!declared)
                    carried.emplace(name, access);
            }
            return carried;
        }

        // A base's words: its spelling, linked where the surface has its page.
        [[nodiscard]] Runs baseText(std::wstring spelling, const Type* type)
        {
            if (type && type->isPublic())
                return { code(std::move(spelling), type->qualifiedName) };
            return { code(std::move(spelling)) };
        }

        // A base met on the way, recorded the first time it is met.
        void reachBase(ReachedBases& reached, ReachedBase base)
        {
            const bool met = std::ranges::any_of(reached, [&base](const ReachedBase& known) {
                return known.type == base.type;
            });
            if (!met)
                reached.push_back(std::move(base));
        }

        // Whether a type stands on the way already: a ring, which no compiler accepts but a
        // reading by names can make of two units declaring alike. A template met again under
        // another spelling is a mixin applied twice, and no ring.
        [[nodiscard]] bool closesRing(const BaseWalk& walk, const Type& type,
            const std::wstring_view spelling)
        {
            return std::ranges::any_of(walk.way, [&](const WayStep& step) {
                return step.type == &type
                    && (type.templateParameters.empty() || step.spelling == spelling);
            });
        }

        // A base on the way: its spelling with a private alias replaced, the type its head names,
        // and that type's own bases under it; the way ends at a type it has come through. A nested
        // base is a part of some type, not a type of its own, and is left out.
        void addBaseNode(BaseWalk& walk, BaseNodes& into, const Surface& surface,
            const BaseReading& reading, const std::size_t depth, const Flow& flow)
        {
            const std::wstring resolvedSpelling =
                surface.resolvedBase(reading.spelling, reading.nameSpace);
            const Type* type = surface.resolve(resolvedSpelling, reading.nameSpace);
            if (type && isNested(*type))
                return;
            BaseNode node{ .text = baseText(resolvedSpelling, type), .type = type };
            if (type && !closesRing(walk, *type, resolvedSpelling))
            {
                ReachedBase base = reachedBase(resolvedSpelling, *type, flow);
                Flow beyond = flow;
                beyond.hidden = base.hidden;
                beyond.republished = carriedPast(base.republished, *type);
                beyond.constructors = base.constructorsInherited;
                reachBase(walk.reached, std::move(base));
                if (depth < k_maxTreeDepth)
                {
                    walk.way.push_back({ .type = type, .spelling = resolvedSpelling });
                    addBaseNodes(walk, node.bases, surface, *type, resolvedSpelling,
                        reading.nameSpace, depth + 1, beyond);
                    walk.way.pop_back();
                }
            }
            into.push_back(std::move(node));
        }

        void addBaseNodes(BaseWalk& walk, BaseNodes& into, const Surface& surface,
            const Type& type, const std::wstring_view spelled, const std::wstring_view nameSpace,
            const std::size_t depth, const Flow& flow)
        {
            const Flow outward = flowFrom(surface, type, spelled, nameSpace, flow);
            for (const std::wstring& base : type.bases)
            {
                const std::optional<BaseReading> reading =
                    readBase(type, spelled, nameSpace, base);
                if (reading)
                {
                    addBaseNode(walk, into, surface, *reading, depth, outward);
                    continue;
                }
                into.push_back({ .text = baseText(base, nullptr) });
            }
        }

        // Whether a type is of a kind that has bases and derived types: a class, a struct, an
        // interface.
        [[nodiscard]] bool derives(const Type& type)
        {
            return type.kind == TypeKind::Class || type.kind == TypeKind::Struct
                || type.kind == TypeKind::Interface;
        }

        // The ways up from a type; a type with no bases of its own kind has none.
        [[nodiscard]] BaseWalk baseWalkOf(const Surface& surface, const Type& type)
        {
            BaseWalk walk;
            if (!derives(type))
                return walk;
            walk.way.push_back({ .type = &type, .spelling = type.name });
            addBaseNodes(walk, walk.bases, surface, type, type.name, type.nameSpace, 0,
                Flow{ .constructors = true });
            return walk;
        }

        // The derived types

        // The public types standing directly on each base, not nested in another, in reading order.
        using DerivedIndex = std::unordered_map<const Type*, TypeList>;

        // The types a type names among its bases - each base, and the argument a mixin base hands
        // on: one whose template's own bases name the parameter standing in that place.
        [[nodiscard]] TypeList directBasesOf(const Surface& surface, const Type& type)
        {
            TypeList bases;
            const auto add = [&bases](const Type* base) {
                if (base && std::ranges::find(bases, base) == bases.end())
                    bases.push_back(base);
            };
            for (const std::wstring& spelled : type.bases)
            {
                if (isTemplateParameter(type, spelled))
                    continue;
                const std::wstring resolvedSpelling =
                    surface.resolvedBase(spelled, type.nameSpace);
                const Type* direct = surface.resolve(resolvedSpelling, type.nameSpace);
                if (!direct)
                    continue;
                add(direct);
                const Names parameters = templateParameterNames(direct->templateParameters);
                const Names arguments = argumentsOf(resolvedSpelling);
                for (const std::wstring& handedOn : direct->bases)
                {
                    const auto parameter =
                        std::ranges::find(parameters, plainType(withoutArguments(handedOn)));
                    if (parameter == parameters.end())
                        continue;
                    const std::size_t index =
                        static_cast<std::size_t>(parameter - parameters.begin());
                    if (index < arguments.size())
                        add(surface.resolve(arguments[index], type.nameSpace));
                }
            }
            return bases;
        }

        [[nodiscard]] DerivedIndex derivedIndexOf(const Surface& surface)
        {
            DerivedIndex index;
            for (const Type& type : surface.types())
            {
                if (!type.isPublic() || isNested(type))
                    continue;
                for (const Type* base : directBasesOf(surface, type))
                    index[base].push_back(&type);
            }
            for (TypeList& derived : index | std::views::values)
            {
                std::ranges::sort(derived, [](const Type* left, const Type* right) {
                    return readsBefore(*left, *right);
                });
            }
            return index;
        }

        // The types derived from a base, each with its own under it; a type the tree has come
        // down through already - a ring of names - stands under it no more.
        void addDerived(Branches& branches, const DerivedIndex& index, const Type& base,
            TypeList& way)
        {
            const auto derived = index.find(&base);
            if (derived == index.end())
                return;
            for (const Type* type : derived->second)
            {
                if (std::ranges::find(way, type) != way.end())
                    continue;
                Branch branch{ .text = { code(type->name) }, .type = type };
                if (way.size() <= k_maxTreeDepth)
                {
                    way.push_back(type);
                    addDerived(branch.children, index, *type, way);
                    way.pop_back();
                }
                branches.push_back(std::move(branch));
            }
        }

        void addDerivedTypes(Page& page, const DerivedIndex& index, const Type& type)
        {
            if (!derives(type))
                return;
            Branches derived;
            TypeList way = { &type };
            addDerived(derived, index, type, way);
            if (derived.empty())
                return;
            Section& section = addSection(page, SectionKind::Tree, { plain(L"Derived types") });
            section.branches = std::move(derived);
        }

        // The bases under the title

        // The public types standing directly on each base the surface does not have, not nested in
        // another, in reading order, by what the base is known by.
        using UnknownBaseIndex = std::unordered_map<std::wstring, TypeList>;

        // What a base the surface does not have is known by: its spelling, template arguments
        // aside.
        [[nodiscard]] std::wstring unknownBaseKey(const std::wstring_view spelling)
        {
            return std::wstring{ plainType(withoutArguments(spelling)) };
        }

        [[nodiscard]] UnknownBaseIndex unknownBaseIndexOf(const Surface& surface)
        {
            UnknownBaseIndex index;
            for (const Type& type : surface.types())
            {
                if (!type.isPublic() || isNested(type))
                    continue;
                for (const std::wstring& spelled : type.bases)
                {
                    if (isTemplateParameter(type, spelled))
                        continue;
                    const std::wstring resolvedSpelling =
                        surface.resolvedBase(spelled, type.nameSpace);
                    if (surface.resolve(resolvedSpelling, type.nameSpace))
                        continue;
                    TypeList& standing = index[unknownBaseKey(resolvedSpelling)];
                    if (std::ranges::find(standing, &type) == standing.end())
                        standing.push_back(&type);
                }
            }
            for (TypeList& standing : index | std::views::values)
            {
                std::ranges::sort(standing, [](const Type* left, const Type* right) {
                    return readsBefore(*left, *right);
                });
            }
            return index;
        }

        [[nodiscard]] std::wstring spellingOf(const BaseNode& base)
        {
            std::wstring result;
            for (const Run& run : base.text)
                result += run.text;
            return result;
        }

        // The types standing directly on a base: by the type it names, or by what it is known by
        // where the surface does not have it; null for none.
        [[nodiscard]] const TypeList* derivedOf(const BaseNode& base, const DerivedIndex& index,
            const UnknownBaseIndex& unknown)
        {
            if (base.type)
            {
                const auto found = index.find(base.type);
                return found == index.end() ? nullptr : &found->second;
            }
            const auto found = unknown.find(unknownBaseKey(spellingOf(base)));
            return found == unknown.end() ? nullptr : &found->second;
        }

        // Each base's derived types, and among them the one the way up came through last - the
        // way holds the types from the page's own up to the base's.
        void addDerivedLists(BaseNodes& bases, const DerivedIndex& index,
            const UnknownBaseIndex& unknown, TypeList& way)
        {
            for (BaseNode& base : bases)
            {
                if (const TypeList* derived = derivedOf(base, index, unknown))
                {
                    base.derived = *derived;
                    const std::ranges::reverse_view nearestFirst{ way };
                    const auto from = std::ranges::find_first_of(nearestFirst, base.derived);
                    if (from != nearestFirst.end())
                        base.from = *from;
                }
                way.push_back(base.type);
                addDerivedLists(base.bases, index, unknown, way);
                way.pop_back();
            }
        }

        // The tree of bases under the title; answers the bases it reaches, whose members the
        // tables carry.
        [[nodiscard]] ReachedBases addTypeBases(Page& page, const Surface& surface,
            const DerivedIndex& index, const Type& type)
        {
            BaseWalk walk = baseWalkOf(surface, type);
            const UnknownBaseIndex unknown = unknownBaseIndexOf(surface);
            TypeList way = { &type };
            addDerivedLists(walk.bases, index, unknown, way);
            page.bases = std::move(walk.bases);
            return std::move(walk.reached);
        }

        // The type's own note, where its comment references one or a section is named after it.
        void addTypeNote(Page& page, const Surface& surface, Notes& notes, const Type& type)
        {
            std::optional<Excerpt> excerpt = excerptOf(
                noteFor(surface, notes, type.comment, type.name, type.folder, type.name), surface,
                type.nameSpace);
            if (!excerpt.has_value())
                return;
            Section& section = addSection(page, SectionKind::Note,
                { plain(std::wstring{ k_typeNoteHeading }) }, excerpt->source);
            section.excerpt = std::move(*excerpt);
        }

        // Where a footnote written new on a folder's or a module's page goes: the folder's note.
        [[nodiscard]] std::optional<FootnoteHome> folderFootnoteHome(const Surface& surface,
            Notes& notes, const std::wstring_view folder)
        {
            return FootnoteHome{ .note = folderNote(surface, notes, folder) };
        }

        // Where a footnote written new for a member of the type goes: after the type's own
        // section where it has one, at the end of its folder's note otherwise.
        [[nodiscard]] std::optional<FootnoteHome> typeFootnoteHome(const Surface& surface,
            Notes& notes, const Type& type)
        {
            const FoundNote found = noteFor(surface, notes, type.comment, type.name, type.folder,
                type.name);
            if (found.section)
            {
                return FootnoteHome{
                    .note = found.note->stem,
                    .after = found.section->anchor,
                    .owner = type.name
                };
            }
            std::optional<FootnoteHome> home = folderFootnoteHome(surface, notes, type.folder);
            if (home)
                home->owner = type.name;
            return home;
        }

        // The member tables of a type page

        // Which of a type's properties or events a table takes: those of one access where it asks
        // for one, and none whose name a type below declares again.
        struct MemberFilter
        {
            std::optional<Access> access;
            const NameSet* hidden{ nullptr };
        };

        // Whether the filter takes a member of that access and name, the name read as Delphi
        // reads one: in any case.
        [[nodiscard]] bool takes(const MemberFilter& filter, const Access access,
            const std::wstring& name)
        {
            if (filter.access && access != *filter.access)
                return false;
            return !filter.hidden || !filter.hidden->contains(lowered(name));
        }

        // The types standing on a base on the page's way up to it, the page's own type first;
        // none for the type's own members.
        using TypesBelow = std::vector<const Type*>;

        [[nodiscard]] TypesBelow typesBelow(const Type& type, const ReachedBases& bases,
            const ReachedBase* reached)
        {
            TypesBelow below;
            if (!reached)
                return below;
            below.push_back(&type);
            for (const ReachedBase& base : bases)
            {
                if (&base == reached)
                    break;
                below.push_back(base.type);
            }
            return below;
        }

        // Whether a value a type hands its base is of the property's type, by bare name, the
        // further spellings the property accepts counted.
        [[nodiscard]] bool restates(const Restatement& restatement, const Property& property)
        {
            const auto bare = [](const std::wstring& spelled) {
                return bareName(plainType(withoutArguments(spelled)));
            };
            const std::wstring_view type = bare(restatement.type);
            if (type == bare(property.type))
                return true;
            return std::ranges::any_of(property.accepts, [&](const std::wstring& accepted) {
                return type == bare(accepted);
            });
        }

        // A value a type below hands the base for the property, and the type handing it.
        struct RestatedDefault
        {
            const Restatement* restatement{ nullptr };
            const Type* by{ nullptr };
        };

        // What the property starts at on the page, by the pack's own rule: the nearest type's
        // value, unless one stands fixed after the pack, when the furthest fixed one wins.
        [[nodiscard]] RestatedDefault restatedDefault(const Property& property,
            const TypesBelow& below)
        {
            RestatedDefault nearest;
            RestatedDefault fixed;
            for (const Type* type : below)
            {
                RestatedDefault own;
                for (const Restatement& restatement : type->restatements)
                {
                    if (!restates(restatement, property))
                        continue;
                    if (restatement.fixed)
                        fixed = { &restatement, type };
                    else
                        own = { &restatement, type };
                }
                if (!nearest.restatement)
                    nearest = own;
            }
            return fixed.restatement ? fixed : nearest;
        }

        // What a restated value's cell ends with: by the type handing it, fixed where it wins.
        [[nodiscard]] Runs qualifierOf(const RestatedDefault& restated)
        {
            Runs runs = {
                muted(std::wstring{ k_byWord }),
                plain(restated.by->name, restated.by->qualifiedName)
            };
            if (restated.restatement->fixed)
            {
                runs.push_back(muted(std::wstring{ k_separator }));
                runs.push_back(muted(std::wstring{ k_fixedWord }));
            }
            return runs;
        }

        // A type's own properties, one row each, those the filter takes; a base's take the value
        // a type below hands it for their default.
        [[nodiscard]] TableRows propertyRows(const Surface& surface, Notes& notes,
            const Type& type, const MemberFilter& filter, const TypesBelow& below)
        {
            TableRows rows;
            for (const Property& property : type.properties)
            {
                if (!takes(filter, property.access, property.name))
                    continue;
                Runs value = valueCell(property.defaultValue);
                Runs defaultQualifier;
                if (property.form == PropertyForm::Required)
                    value = { muted(L"required") };
                if (const RestatedDefault restated = restatedDefault(property, below);
                    restated.restatement)
                {
                    value = valueCell(restated.restatement->value);
                    defaultQualifier = qualifierOf(restated);
                }
                Runs spelled = { typeRun(surface, property.type, type.nameSpace) };
                for (const std::wstring& accepted : property.accepts)
                {
                    spelled.push_back(muted(L" | "));
                    spelled.push_back(typeRun(surface, accepted, type.nameSpace));
                }
                rows.push_back({
                    .cells = { { code(property.name) }, std::move(spelled), std::move(value),
                        hintRuns(surface, property.comment, type.nameSpace) },
                    .qualifiers = { {}, {}, std::move(defaultQualifier) },
                    .excerpt = memberExcerpt(surface, notes, property.comment, property.name,
                        type),
                    .site = siteOf(property.comment, property.place, property.name),
                    .anchor = property.name
                });
            }
            return rows;
        }

        // A type's own events, one row each, those the filter takes; the hint names what the
        // event carries.
        [[nodiscard]] TableRows eventRows(const Surface& surface, Notes& notes, const Type& type,
            const MemberFilter& filter)
        {
            TableRows rows;
            for (const Event& event : type.events)
            {
                if (!takes(filter, event.access, event.alias))
                    continue;
                Runs hint = hintRuns(surface, event.comment, type.nameSpace);
                if (const Type* payload = surface.resolve(event.type, type.nameSpace);
                    payload && !payload->fields.empty())
                {
                    std::wstring carries = L" - carries ";
                    for (std::size_t i = 0; i != payload->fields.size(); ++i)
                    {
                        if (i != 0)
                            carries += L", ";
                        carries += payload->fields[i].name;
                    }
                    hint.push_back(muted(std::move(carries)));
                }
                rows.push_back({
                    .cells = { { code(event.alias) },
                        { typeRun(surface, event.type, type.nameSpace) }, std::move(hint) },
                    .excerpt = memberExcerpt(surface, notes, event.comment, event.type, type),
                    .site = siteOf(event.comment, event.place, event.type),
                    .anchor = event.alias
                });
            }
            return rows;
        }

        // The name a property's getter has, and the one its setter has by the routine.
        [[nodiscard]] std::wstring getterName(const std::wstring& property)
        {
            std::wstring name = property;
            if (!name.empty())
                name.front() = static_cast<wchar_t>(std::towlower(name.front()));
            return name;
        }

        [[nodiscard]] std::wstring setterName(const std::wstring& property)
        {
            std::wstring name = property;
            if (!name.empty())
                name.front() = static_cast<wchar_t>(std::towupper(name.front()));
            return std::wstring{ k_setterPrefix } + name;
        }

        // Whether a function reads or writes a property of its type, which the property's row says.
        [[nodiscard]] bool isAccessor(const Type& owner, const Function& function)
        {
            if (function.kind != FunctionKind::Function)
                return false;
            return std::ranges::any_of(owner.properties, [&function](const Property& property) {
                return function.name == property.setter || function.name == property.target
                    || function.name == getterName(property.name)
                    || function.name == setterName(property.name);
            });
        }

        // The function whose comment a function without one shows: the function of the same
        // shape in the nearest base above that carries one - an override's virtual. The function
        // itself where it has one, or where nothing above does.
        [[nodiscard]] const Function& commentHolder(const Function& function,
            const std::span<const ReachedBase> above, const SourceLanguage language)
        {
            if (!function.comment.text.empty())
                return function;
            const std::wstring key = shapeKey(function, language);
            for (const ReachedBase& base : above)
            {
                for (const Function& candidate : base.type->functions)
                {
                    if (candidate.name == function.name && !candidate.comment.text.empty()
                        && shapeKey(candidate, language) == key)
                    {
                        return candidate;
                    }
                }
            }
            return function;
        }

        [[nodiscard]] CommentSite siteOf(const Function& function)
        {
            return siteOf(function.comment, function.place, function.name);
        }

        // Whether a method stands in the table: a property's accessor never, the type's own
        // otherwise; a base's when it reaches the type - a constructor taken with using, a
        // function no type below declares again by name or by shape, nothing deleted, and no
        // destructor. A Delphi constructor reaches as a function does.
        [[nodiscard]] bool reachesTable(const Type& owner, const Function& function,
            const ReachedBase* reached, const SourceLanguage language)
        {
            if (isAccessor(owner, function))
                return false;
            if (!reached)
                return true;
            if (function.isDeleted)
                return false;
            const bool inherited = language == SourceLanguage::Pascal
                && function.kind == FunctionKind::Constructor;
            if (inherited)
            {
                return !reached->hidden.contains(function.name)
                    && !reached->shapes.contains(shapeKey(function, language));
            }
            switch (function.kind)
            {
                case FunctionKind::Constructor:
                    return reached->constructorsInherited;
                case FunctionKind::Destructor:
                    return false;
                case FunctionKind::Function:
                    return !reached->hidden.contains(function.name)
                        && !reached->shapes.contains(shapeKey(function, language));
            }
            return false;
        }

        // The bases above an owner on the way up: every reached base for the type itself, those
        // after it for a base.
        [[nodiscard]] std::span<const ReachedBase> basesAbove(const ReachedBases& bases,
            const ReachedBase* reached)
        {
            const std::span<const ReachedBase> all{ bases };
            if (!reached)
                return all;
            const std::size_t index = static_cast<std::size_t>(reached - bases.data());
            return all.subspan(index + 1);
        }

        // The access a method stands under on a type's page: its own, or the one a
        // using-declaration below republished it with. A Delphi method published is public, a
        // designer's business being the properties.
        [[nodiscard]] Access shownAccess(const Function& function, const ReachedBase* reached)
        {
            if (reached)
            {
                const auto republished = reached->republished.find(function.name);
                if (republished != reached->republished.end())
                    return republished->second;
            }
            if (function.access == Access::Published)
                return Access::Public;
            return function.access;
        }

        // A function as spelled: the template head where it has one, then the signature, joined
        // as asked. A Delphi routine spells its generic parameters after its name.
        [[nodiscard]] std::wstring spelledSignature(const Function& function,
            const std::wstring_view join, const SourceLanguage language)
        {
            std::wstring spelled;
            if (!function.templateParameters.empty() && language == SourceLanguage::Cpp)
            {
                spelled = std::wstring{ k_templateWord } + L"<" + function.templateParameters
                    + L">" + std::wstring{ join };
            }
            spelled += function.signature;
            return spelled;
        }

        // A method as code: a type it names that the surface has a page for is linked, the
        // owner's and the method's own template parameters never.
        [[nodiscard]] Runs signatureRuns(const Surface& surface, const Type& owner,
            const Function& function, const std::wstring_view join)
        {
            Names leftOut = templateParameterNames(owner.templateParameters);
            for (std::wstring& name : templateParameterNames(function.templateParameters))
                leftOut.push_back(std::move(name));
            const std::wstring spelled =
                spelledSignature(function, join, surface.languageOf(owner.module));
            return linkNames(surface, { code(spelled) }, owner.nameSpace, leftOut);
        }

        // A free function as code, linked as a method's is.
        [[nodiscard]] Runs signatureRuns(const Surface& surface, const FreeFunction& free,
            const std::wstring_view join)
        {
            const Function& function = free.function;
            const std::wstring spelled =
                spelledSignature(function, join, surface.languageOf(free.module));
            return linkNames(surface, { code(spelled) }, free.nameSpace,
                templateParameterNames(function.templateParameters));
        }

        // A name's row in a table of one row per name, and whether an overload gave it a comment.
        struct NameRow
        {
            std::size_t index{ 0 };
            bool commented{ false };
        };

        // One owner's methods of one access as a group's rows, one per name: the name linked to
        // the method's page, every overload's signature in the name's tooltip a line each, and the
        // comment of the first overload that has one beside it.
        [[nodiscard]] TableGroup methodGroup(const Surface& surface, const Type& owner,
            const ReachedBase* reached, const std::span<const ReachedBase> above,
            const Access access)
        {
            const SourceLanguage language = surface.languageOf(owner.module);
            TableGroup group;
            std::map<std::wstring, NameRow> rowByName;
            for (const Function& function : owner.functions)
            {
                if (!reachesTable(owner, function, reached, language))
                    continue;
                if (shownAccess(function, reached) != access)
                    continue;
                const Function& holder = commentHolder(function, above, language);
                const Comment& comment = holder.comment;
                Runs signature = signatureRuns(surface, owner, function, k_tooltipJoin);
                const auto known = rowByName.find(function.name);
                if (known == rowByName.end())
                {
                    rowByName.emplace(function.name, NameRow{
                        .index = group.rows.size(),
                        .commented = !comment.text.empty()
                    });
                    group.rows.push_back({
                        .cells = { { code(function.name, memberLink(owner, function.name)) },
                            hintRuns(surface, comment, owner.nameSpace) },
                        .tooltip = std::move(signature),
                        .language = language,
                        .site = siteOf(holder)
                    });
                    continue;
                }
                TableRow& row = group.rows[known->second.index];
                row.tooltip.push_back(code(std::wstring{ k_signatureJoin }));
                row.tooltip.insert(row.tooltip.end(), std::make_move_iterator(signature.begin()),
                    std::make_move_iterator(signature.end()));
                if (!known->second.commented && !comment.text.empty())
                {
                    row.cells[1] = hintRuns(surface, comment, owner.nameSpace);
                    row.site = siteOf(holder);
                    known->second.commented = true;
                }
            }
            return group;
        }

        [[nodiscard]] bool isEmpty(const TableGroup& group)
        {
            return group.rows.empty() && group.groups.empty();
        }

        // A member table's groups, and how many rows the type's own group holds.
        struct MemberGroups
        {
            TableGroups groups;
            std::size_t own{ 0 };
        };

        // The type's own group under Own, then each base's under From it, up the whole chain; a
        // base with nothing is left out, and the own group with nothing inherited goes unlabelled.
        template<typename GroupOf>
        [[nodiscard]] MemberGroups ownAndInherited(const Type& type, const ReachedBases& bases,
            const GroupOf& groupOf)
        {
            MemberGroups result;
            TableGroup own = groupOf(type, nullptr);
            result.own = own.rowCount();
            const bool hasOwn = !isEmpty(own);
            if (hasOwn)
                result.groups.push_back(std::move(own));
            for (const ReachedBase& base : bases)
            {
                TableGroup group = groupOf(*base.type, &base);
                if (isEmpty(group))
                    continue;
                group.label = { plain(std::wstring{ k_fromLabel }), code(base.spelling) };
                group.owner = base.spelling;
                result.groups.push_back(std::move(group));
            }
            if (hasOwn && result.groups.size() > 1)
                result.groups.front().label = { plain(std::wstring{ k_ownLabel }) };
            return result;
        }

        // What a member table's heading says of its rows: how many, and the own and the inherited
        // apart where it holds both.
        [[nodiscard]] std::wstring memberCount(const std::size_t own, const std::size_t inherited)
        {
            const std::wstring elements = countOf(own + inherited, L"element");
            if (inherited == 0)
                return elements;
            if (own == 0)
                return elements + std::wstring{ k_separator } + L"all inherited";
            return elements + std::wstring{ k_separator } + std::to_wstring(own) + L" own"
                + std::wstring{ k_separator } + std::to_wstring(inherited) + L" inherited";
        }

        // A member table over the type and its bases, its tooltip telling the own rows apart; its
        // rows take footnotes as asked. Nothing where neither the type nor a base has a member.
        template<typename GroupOf>
        void addMemberSection(Page& page, std::wstring heading, const ColumnSet& columnSet,
            const Type& type, const ReachedBases& bases, const GroupOf& groupOf,
            const bool takesFootnotes)
        {
            MemberGroups groups = ownAndInherited(type, bases, groupOf);
            if (groups.groups.empty())
                return;
            Section& section = addTableSection(page, std::move(heading), columnSet,
                std::move(groups.groups));
            section.tooltip = memberCount(groups.own, section.table.rowCount() - groups.own);
            section.table.takesFootnotes = takesFootnotes;
        }

        // The names of the properties and the events a type declares, in lower case.
        [[nodiscard]] NameSet memberNamesOf(const Type& type)
        {
            NameSet names;
            for (const Property& property : type.properties)
                names.insert(lowered(property.name));
            for (const Event& event : type.events)
                names.insert(lowered(event.alias));
            return names;
        }

        // **Delphi declares a property again to publish it**, and the base's declaration is then
        // the type's no longer: for each base reached, the names the type and the bases met before
        // it declare.
        [[nodiscard]] std::vector<NameSet> namesBelow(const Type& type, const ReachedBases& bases)
        {
            std::vector<NameSet> below;
            NameSet declared = memberNamesOf(type);
            for (const ReachedBase& base : bases)
            {
                below.push_back(declared);
                declared.merge(memberNamesOf(*base.type));
            }
            return below;
        }

        // The accesses a Delphi type's properties and events are parted by, as its declaration
        // parts them, and the word each table's heading opens with.
        struct AccessSection
        {
            Access access{ Access::Public };
            std::wstring_view word;
        };

        constexpr auto k_accessSections = std::to_array<AccessSection>({
            { Access::Published, L"Published" },
            { Access::Public, L"Public" },
            { Access::Protected, L"Protected" }
        });

        // A member table over the type and its bases; a Delphi type's a table per access, a
        // base's member that a type below declares again left out.
        template<typename RowsOf>
        void addPartedSections(Page& page, const Surface& surface, const std::wstring_view heading,
            const ColumnSet& columnSet, const Type& type, const ReachedBases& bases,
            const RowsOf& rowsOf)
        {
            if (surface.languageOf(type.module) != SourceLanguage::Pascal)
            {
                addMemberSection(page, std::wstring{ heading }, columnSet, type, bases,
                    [&](const Type& owner, const ReachedBase* reached) {
                        return TableGroup{
                            .rows = rowsOf(owner, MemberFilter{}, typesBelow(type, bases, reached))
                        };
                    }, true);
                return;
            }
            const std::vector<NameSet> below = namesBelow(type, bases);
            for (const AccessSection& section : k_accessSections)
            {
                std::wstring parted = std::wstring{ section.word } + L' ' + lowered(heading);
                addMemberSection(page, std::move(parted), columnSet, type, bases,
                    [&](const Type& owner, const ReachedBase* reached) {
                        MemberFilter filter{ .access = section.access };
                        if (reached)
                        {
                            const std::size_t index =
                                static_cast<std::size_t>(reached - bases.data());
                            filter.hidden = &below[index];
                        }
                        return TableGroup{
                            .rows = rowsOf(owner, filter, typesBelow(type, bases, reached))
                        };
                    }, true);
            }
        }

        void addProperties(Page& page, const Surface& surface, Notes& notes, const Type& type,
            const ReachedBases& bases)
        {
            addPartedSections(page, surface, L"Properties", Columns::properties, type, bases,
                [&](const Type& owner, const MemberFilter& filter, const TypesBelow& below) {
                    return propertyRows(surface, notes, owner, filter, below);
                });
        }

        void addEvents(Page& page, const Surface& surface, Notes& notes, const Type& type,
            const ReachedBases& bases)
        {
            addPartedSections(page, surface, L"Events", Columns::events, type, bases,
                [&](const Type& owner, const MemberFilter& filter, const TypesBelow&) {
                    return eventRows(surface, notes, owner, filter);
                });
        }

        // The methods as two tables, the public ones and the protected ones, each over the type
        // and its bases.
        void addMethods(Page& page, const Surface& surface, const Type& type,
            const ReachedBases& bases)
        {
            const auto add = [&](const std::wstring_view heading, const Access access) {
                addMemberSection(page, std::wstring{ heading }, Columns::methods, type, bases,
                    [&](const Type& owner, const ReachedBase* reached) {
                        return methodGroup(surface, owner, reached, basesAbove(bases, reached),
                            access);
                    }, false);
            };
            add(k_publicMethods, Access::Public);
            add(k_protectedMethods, Access::Protected);
        }

        // The page of a method or a function

        // What a method is called on its page: a constructor or a destructor by that word.
        [[nodiscard]] std::wstring_view methodKindWord(const FunctionKind kind)
        {
            if (kind == FunctionKind::Function)
                return k_methodWord;
            return kindWord(kind);
        }

        // One overload as its page shows it - a method's or a free function's.
        struct Overload
        {
            Runs code;
            const Function* holder{ nullptr };   // whose comment speaks for it: its own or a base's
            std::wstring_view nameSpace;         // where its words are read
            std::wstring_view folder;            // where a note named after it is looked for
            std::wstring heading;                // what that note's section is headed
            std::optional<Access> access;        // none for a free function
            SourceLanguage language{ SourceLanguage::Cpp };
        };

        using Overloads = std::vector<Overload>;

        // The section declarations stand in: under their access label written the C++ way -
        // public: - where they have one, under none where they do not; made when first met.
        Section& signatureSection(Page& page, const std::optional<Access> access)
        {
            std::wstring label;
            if (access)
                label = std::wstring{ accessWord(*access) } + std::wstring{ k_accessMark };
            for (Section& section : page.sections)
            {
                if (section.kind != SectionKind::Signature)
                    continue;
                const std::wstring_view heading = section.heading.empty()
                    ? std::wstring_view{}
                    : std::wstring_view{ section.heading.front().text };
                if (heading == label)
                    return section;
            }
            Runs heading;
            if (!label.empty())
                heading.push_back(code(std::move(label)));
            return addSection(page, SectionKind::Signature, std::move(heading));
        }

        // The overloads of one name: what they are after the title, the hint under it - the first
        // commented overload's, as the name's row has it - then each overload's code with its
        // note, and its own hint where the page's is another's.
        void addOverloads(Page& page, const Surface& surface, Notes& notes,
            const std::wstring_view kind, Overloads overloads)
        {
            page.badges.push_back(muted(std::wstring{ kind }));
            if (overloads.size() > 1)
            {
                page.badges.push_back(muted(std::wstring{ k_separator }
                    + countOf(overloads.size(), k_overloadNoun)));
            }
            if (overloads.empty())
                return;
            const auto commented = std::ranges::find_if(overloads, [](const Overload& overload) {
                return !overload.holder->comment.text.empty();
            });
            const Overload& leading = commented == overloads.end()
                ? overloads.front()
                : *commented;
            page.lead = hintRuns(surface, leading.holder->comment, leading.nameSpace);
            page.leadSite = siteOf(*leading.holder);
            // The section named after the name is the name's, and stands under the leading one.
            for (Overload& overload : overloads)
            {
                const Function& holder = *overload.holder;
                const std::wstring_view heading = &overload == &leading
                    ? std::wstring_view{ overload.heading }
                    : std::wstring_view{};
                const FoundNote found = noteFor(surface, notes, holder.comment, holder.name,
                    overload.folder, heading);
                Signature signature{
                    .code = std::move(overload.code),
                    .language = overload.language,
                    .excerpt = excerptOf(found, surface, overload.nameSpace)
                };
                if (&overload != &leading)
                {
                    signature.lead = hintRuns(surface, holder.comment, overload.nameSpace);
                    signature.site = siteOf(holder);
                }
                signatureSection(page, overload.access).signatures.push_back(std::move(signature));
            }
        }

        void addFields(Page& page, const Surface& surface, Notes& notes, const Type& type)
        {
            if (type.fields.empty())
                return;
            TableGroup group;
            for (const Field& field : type.fields)
            {
                group.rows.push_back({
                    .cells = { { code(field.name) },
                        { typeRun(surface, field.type, type.nameSpace) }, valueCell(field.value),
                        hintRuns(surface, field.comment, type.nameSpace) },
                    .excerpt = memberExcerpt(surface, notes, field.comment, field.name, type),
                    .site = siteOf(field.comment, field.place, field.name),
                    .anchor = field.name
                });
            }
            TableGroups groups;
            groups.push_back(std::move(group));
            Section& section = addTableSection(page, L"Fields", Columns::fields,
                std::move(groups));
            section.table.takesFootnotes = true;
        }

        void addMembers(Page& page, const Surface& surface, Notes& notes, const Type& type)
        {
            if (type.members.empty())
                return;
            TableGroup group;
            for (const EnumMember& member : type.members)
            {
                group.rows.push_back({
                    .cells = { { code(member.name) }, valueCell(member.value),
                        hintRuns(surface, member.comment, type.nameSpace) },
                    .excerpt = memberExcerpt(surface, notes, member.comment, member.name, type),
                    .site = memberSite(type, member),
                    .anchor = member.name
                });
            }
            TableGroups groups;
            groups.push_back(std::move(group));
            Section& section = addTableSection(page, L"Members", Columns::members,
                std::move(groups));
            section.table.takesFootnotes = true;
        }

        // Whether a type is its data - a struct or a union - whose fields lead its methods.
        [[nodiscard]] bool leadsWithFields(const Type& type)
        {
            return type.kind == TypeKind::Struct || type.kind == TypeKind::Union;
        }

        // The types with a property or an event of this type

        // Whether a property's type, or one it accepts, is the type.
        [[nodiscard]] bool isPropertyOf(const Surface& surface, const Type& owner,
            const Property& property, const Type& type)
        {
            if (surface.resolve(property.type, owner.nameSpace) == &type)
                return true;
            return std::ranges::any_of(property.accepts, [&](const std::wstring& accepted) {
                return surface.resolve(accepted, owner.nameSpace) == &type;
            });
        }

        [[nodiscard]] bool hasPropertyOf(const Surface& surface, const Type& owner,
            const Type& type)
        {
            return std::ranges::any_of(owner.properties, [&](const Property& property) {
                return isPropertyOf(surface, owner, property, type);
            });
        }

        [[nodiscard]] bool hasEventOf(const Surface& surface, const Type& owner, const Type& type)
        {
            return std::ranges::any_of(owner.events, [&](const Event& event) {
                return surface.resolve(event.type, owner.nameSpace) == &type;
            });
        }

        // A tree of the types that have the type as a member of some kind, each with the types
        // derived from it under it, since they have the member too; the nodes start closed.
        template<typename Has>
        void addOwnersTree(Page& page, const Surface& surface, const DerivedIndex& index,
            std::wstring heading, const Has& has)
        {
            TypeList owners;
            for (const Type& owner : surface.types())
            {
                if (owner.isPublic() && has(owner))
                    owners.push_back(&owner);
            }
            if (owners.empty())
                return;
            std::ranges::sort(owners, [](const Type* left, const Type* right) {
                return readsBefore(*left, *right);
            });
            Section& section = addSection(page, SectionKind::Tree, { plain(std::move(heading)) });
            for (const Type* owner : owners)
            {
                Branch branch{ .text = { code(owner->name) }, .type = owner };
                TypeList way = { owner };
                addDerived(branch.children, index, *owner, way);
                section.branches.push_back(std::move(branch));
            }
        }

        void addPropertyOf(Page& page, const Surface& surface, const DerivedIndex& index,
            const Type& type)
        {
            addOwnersTree(page, surface, index, L"Property of", [&](const Type& owner) {
                return hasPropertyOf(surface, owner, type);
            });
        }

        void addEventOf(Page& page, const Surface& surface, const DerivedIndex& index,
            const Type& type)
        {
            addOwnersTree(page, surface, index, L"Event of", [&](const Type& owner) {
                return hasEventOf(surface, owner, type);
            });
        }

        // The tables of types, functions and constants

        [[nodiscard]] TableRows typeRows(const Surface& surface, const TypeList& types)
        {
            TableRows rows;
            for (const Type* type : types)
            {
                rows.push_back({
                    .cells = { { nameRun(*type) },
                        hintRuns(surface, type->comment, type->nameSpace) },
                    .qualifiers = { qualifierOf(*type) },
                    .type = type,
                    .site = siteOf(*type)
                });
            }
            return rows;
        }

        // A folder directly under another, with what stands under it at every depth counted.
        struct ChildFolder
        {
            std::wstring path;
            std::size_t modules{ 0 };
            std::size_t types{ 0 };
            std::size_t controls{ 0 };
        };

        // The folders directly under a path, in the order the folders read, each once -
        // whether or not it holds modules of its own.
        [[nodiscard]] std::vector<ChildFolder> childFolders(const ContentsFolders& folders,
            const std::wstring_view path)
        {
            std::vector<ChildFolder> result;
            for (const ContentsFolder& folder : folders)
            {
                if (!standsUnder(folder.path, path))
                    continue;
                std::wstring_view rest = folder.path;
                if (!path.empty())
                    rest.remove_prefix(path.size() + 1);
                const std::wstring_view name = rest.substr(0, rest.find(L'/'));
                std::wstring child{ path };
                if (!child.empty())
                    child += L'/';
                child += name;
                std::vector<ChildFolder>::iterator found = std::ranges::find(result, child,
                    &ChildFolder::path);
                if (found == result.end())
                {
                    result.push_back({ .path = std::move(child) });
                    found = result.end() - 1;
                }
                found->modules += folder.modules.size();
                for (const ContentsModule& module : folder.modules)
                {
                    for (const Type* type : module.types)
                    {
                        ++found->types;
                        if (type->isControl)
                            ++found->controls;
                    }
                }
            }
            return result;
        }

        // A row per folder directly under the path - its own name linked to its page, its
        // modules, types and controls counted - and nothing where there is none.
        void addFolderSection(Page& page, const ContentsFolders& folders,
            const std::wstring_view path)
        {
            const std::vector<ChildFolder> children = childFolders(folders, path);
            if (children.empty())
                return;
            TableRows rows;
            for (const ChildFolder& child : children)
            {
                rows.push_back({
                    .cells = {
                        { { .text = folderNameOf(child.path), .link = child.path } },
                        { plain(std::to_wstring(child.modules)) },
                        { plain(std::to_wstring(child.types)) },
                        { plain(std::to_wstring(child.controls)) }
                    },
                    .pageKind = PageKind::Folder
                });
            }
            TableGroups groups;
            groups.push_back({ .rows = std::move(rows) });
            addTableSection(page, L"Folders", Columns::folders, std::move(groups));
        }

        void addSiblings(Page& page, const Surface& surface, const Type& type)
        {
            TypeList siblings;
            for (const Type& other : surface.types())
            {
                if (&other != &type && other.module == type.module && other.isPublic())
                    siblings.push_back(&other);
            }
            if (siblings.empty())
                return;
            std::ranges::sort(siblings, [](const Type* left, const Type* right) {
                return readsBefore(*left, *right);
            });
            TableGroups groups;
            groups.push_back({ .rows = typeRows(surface, siblings) });
            addTableSection(page, L"Also in this module", Columns::types, std::move(groups));
        }

        // A module's functions as rows, one per name: the name linked to the function's page,
        // every overload's signature in the name's tooltip a line each, and the comment of the
        // first overload that has one beside it.
        [[nodiscard]] TableRows functionRows(const Surface& surface, const ContentsModule& module)
        {
            TableRows rows;
            std::map<std::wstring_view, NameRow> rowByName;
            for (const FreeFunction* free : module.functions)
            {
                const Function& function = free->function;
                Runs signature = signatureRuns(surface, *free, k_tooltipJoin);
                const auto known = rowByName.find(function.name);
                if (known == rowByName.end())
                {
                    rowByName.emplace(function.name, NameRow{
                        .index = rows.size(),
                        .commented = !function.comment.text.empty()
                    });
                    rows.push_back({
                        .cells = { { code(function.name, exportLink(module, function.name)) },
                            hintRuns(surface, function.comment, free->nameSpace) },
                        .tooltip = std::move(signature),
                        .language = surface.languageOf(free->module),
                        .site = siteOf(function)
                    });
                    continue;
                }
                TableRow& row = rows[known->second.index];
                row.tooltip.push_back(code(std::wstring{ k_signatureJoin }));
                row.tooltip.insert(row.tooltip.end(), std::make_move_iterator(signature.begin()),
                    std::make_move_iterator(signature.end()));
                if (!known->second.commented && !function.comment.text.empty())
                {
                    row.cells[1] = hintRuns(surface, function.comment, free->nameSpace);
                    row.site = siteOf(function);
                    known->second.commented = true;
                }
            }
            return rows;
        }

        // A module's constants as rows, each name linked to the constant's page.
        [[nodiscard]] TableRows constantRows(const Surface& surface, const ContentsModule& module)
        {
            TableRows rows;
            for (const Variable* variable : module.constants)
            {
                const Field& field = variable->field;
                rows.push_back({
                    .cells = { { code(field.name, exportLink(module, field.name)) },
                        { typeRun(surface, field.type, variable->nameSpace) },
                        valueCell(field.value),
                        hintRuns(surface, field.comment, variable->nameSpace) },
                    .site = siteOf(field.comment, field.place, field.name)
                });
            }
            return rows;
        }

        // A table of one run of rows, and nothing where there are none.
        void addRowsSection(Page& page, std::wstring heading, const ColumnSet& columnSet,
            TableRows rows)
        {
            if (rows.empty())
                return;
            TableGroups groups;
            groups.push_back({ .rows = std::move(rows) });
            addTableSection(page, std::move(heading), columnSet, std::move(groups));
        }

        // The rows with empty cells put in at that column, so they stand under a table of more
        // columns.
        [[nodiscard]] TableRows withEmptyCells(TableRows rows, const std::size_t column,
            const std::size_t count)
        {
            const std::ptrdiff_t at = static_cast<std::ptrdiff_t>(column);
            for (TableRow& row : rows)
                row.cells.insert(row.cells.begin() + at, count, Runs{});
            return rows;
        }

        // A module's rows of one kind beside the kind's word; nothing where it has none.
        void addKindGroup(TableGroups& groups, const std::wstring_view kind, TableRows rows)
        {
            if (rows.empty())
                return;
            groups.push_back({
                .label = { plain(std::wstring{ kind }) },
                .placement = LabelPlacement::Beside,
                .rows = std::move(rows)
            });
        }

        // A folder's contents as one table: each module folded under its name, linked to its page,
        // and in it its types, functions and constants beside the word for each, under the
        // constants' columns. The modules are counted with the rows in the heading's tooltip;
        // nothing where no module has a row.
        void addContents(Page& page, const Surface& surface, const ContentsFolder& folder)
        {
            TableGroups modules;
            for (const ContentsModule& module : folder.modules)
            {
                TableGroup group{
                    .label = { { .text = module.shownName, .link = module.name } },
                    .pageKind = PageKind::Module,
                    .folded = true
                };
                if (module.header)
                    group.tooltip = { plain(module.name) };
                addKindGroup(group.groups, L"Types",
                    withEmptyCells(typeRows(surface, module.types), 1, 2));
                addKindGroup(group.groups, L"Functions",
                    withEmptyCells(functionRows(surface, module), 1, 2));
                addKindGroup(group.groups, L"Constants", constantRows(surface, module));
                if (!group.groups.empty())
                    modules.push_back(std::move(group));
            }
            if (modules.empty())
                return;
            const std::size_t moduleCount = modules.size();
            Section& section = addTableSection(page, L"Modules", Columns::contents,
                std::move(modules));
            section.table.groupColumn = k_exportsColumn;
            section.table.hintTakesEmptyCells = true;
            section.tooltip += L" in " + countOf(moduleCount, k_moduleWord);
        }

        // What stands under a path at every depth, counted: controls, types, modules, the folders
        // - every one on the way down, with modules in it or not - and under the strict rules the
        // types without a hint.
        void addHoldings(Page& page, const Surface& surface, const ContentsFolders& folders,
            const std::wstring_view path)
        {
            std::size_t controls = 0;
            std::size_t types = 0;
            std::size_t modules = 0;
            std::size_t withoutHint = 0;
            std::set<std::wstring_view> below;
            for (const ContentsFolder& folder : folders)
            {
                if (folder.path != path && !standsUnder(folder.path, path))
                    continue;
                for (const ContentsModule& module : folder.modules)
                {
                    for (const Type* type : module.types)
                    {
                        ++types;
                        if (type->isControl)
                            ++controls;
                        if (type->comment.text.empty())
                            ++withoutHint;
                    }
                }
                modules += folder.modules.size();
                for (std::wstring_view way = folder.path; standsUnder(way, path);)
                {
                    below.insert(way);
                    const std::size_t slash = way.rfind(L'/');
                    way = slash == std::wstring_view::npos
                        ? std::wstring_view{}
                        : way.substr(0, slash);
                }
            }
            Runs figures = {
                plain(countOf(controls, L"control")),
                muted(std::wstring{ k_separator }),
                plain(countOf(types, L"type")),
                muted(std::wstring{ k_separator }),
                plain(countOf(modules, L"module"))
            };
            if (!below.empty())
            {
                figures.push_back(muted(std::wstring{ k_separator }));
                figures.push_back(plain(countOf(below.size(), k_folderWord)));
            }
            if (withoutHint != 0 && surface.hintRules() == HintRules::Strict)
            {
                figures.push_back(muted(std::wstring{ k_separator }));
                figures.push_back(missing(countOf(withoutHint, L"type") + L" without a hint"));
            }
            addFact(page, L"holds", std::move(figures));
        }

        // What a folder's page says of what stands in it: its holdings, the folders directly
        // under it, and where it holds modules of its own what they export, which is where its
        // footnotes go.
        void addFolderContents(Page& page, const Surface& surface, Notes& notes,
            const ContentsFolders& folders, const std::wstring_view path)
        {
            addHoldings(page, surface, folders, path);
            addFolderSection(page, folders, path);
            const auto own = std::ranges::find_if(folders, [path](const ContentsFolder& folder) {
                return folder.path == path;
            });
            if (own == folders.end())
                return;
            page.footnoteHome = folderFootnoteHome(surface, notes, path);
            addContents(page, surface, *own);
        }

        // The pages of a module's functions and constants

        // Whether a token's words stand as written in linked code: a comment's and a literal's.
        [[nodiscard]] bool standsAsWritten(const Syntax::TokenKind kind)
        {
            return kind == Syntax::TokenKind::Comment || kind == Syntax::TokenKind::String
                || kind == Syntax::TokenKind::Escape;
        }

        // The lexer a declaration of the language is read with.
        [[nodiscard]] const Syntax::Language& lexerOf(const SourceLanguage language)
        {
            if (language == SourceLanguage::Pascal)
                return Syntax::Languages::pascal;
            return Syntax::Languages::cpp;
        }

        // A declaration as written, as code: the names it spells linked as a signature's are, the
        // words of its comments and its literals left as they stand.
        [[nodiscard]] Runs declarationRuns(const Surface& surface, const std::wstring_view written,
            const std::wstring_view nameSpace, const SourceLanguage language)
        {
            const Syntax::Language& lexer = lexerOf(language);
            Runs result;
            std::wstring pending;
            const auto linkPending = [&]() {
                if (pending.empty())
                    return;
                linkNamesIn(result, code(std::move(pending)), surface, nameSpace, {});
                pending.clear();
            };
            Syntax::StateStrings strings;
            Syntax::Tokens tokens;
            Syntax::LineState state{};
            std::size_t lineStart = 0;
            while (true)
            {
                const std::size_t newline = written.find(L'\n', lineStart);
                const std::size_t lineEnd = newline == std::wstring_view::npos
                    ? written.size()
                    : newline;
                const std::wstring_view line = written.substr(lineStart, lineEnd - lineStart);
                tokens.clear();
                state = Syntax::lexLine(lexer, line, state, strings, &tokens);
                std::size_t at = 0;
                for (const Syntax::Token& token : tokens)
                {
                    if (!standsAsWritten(token.kind))
                        continue;
                    pending += line.substr(at, token.range.start - at);
                    linkPending();
                    result.push_back(code(std::wstring{
                        line.substr(token.range.start, token.range.length) }));
                    at = token.range.start + token.range.length;
                }
                pending += line.substr(at);
                if (newline == std::wstring_view::npos)
                    break;
                pending += L'\n';
                lineStart = newline + 1;
            }
            linkPending();
            return result;
        }

        void addExportFacts(Page& page, const Surface& surface, const SourceRoots& roots,
            const std::wstring_view nameSpace, const ContentsModule& module, const Place& place)
        {
            addFact(page, L"namespace", { code(std::wstring{ nameSpace }) });
            addFact(page, std::wstring{ module.header ? k_headerWord : k_moduleWord },
                { code(module.name, module.name) });
            addPlaceFact(page, surface, roots, place, surface.languageOf(module.name));
        }

        // A constant's page: what it is after the title, its hint under it, where it is
        // declared, then its declaration as written with its note.
        void fillConstantPage(Page& page, const Surface& surface, Notes& notes,
            const SourceRoots& roots, const std::wstring_view folder,
            const ContentsModule& module, const Variable& variable)
        {
            const Field& field = variable.field;
            const std::wstring_view kind = field.isConstant ? k_constantWord : k_variableWord;
            page.badges.push_back(muted(std::wstring{ kind }));
            page.lead = hintRuns(surface, field.comment, variable.nameSpace);
            page.leadSite = siteOf(field.comment, field.place, field.name);
            addExportFacts(page, surface, roots, variable.nameSpace, module, field.place);
            const FoundNote found = noteFor(surface, notes, field.comment, field.name, folder,
                field.name);
            const SourceLanguage language = surface.languageOf(variable.module);
            Section& section = addSection(page, SectionKind::Signature, {});
            section.signatures.push_back({
                .code = declarationRuns(surface, variable.declaration, variable.nameSpace,
                    language),
                .language = language,
                .excerpt = excerptOf(found, surface, variable.nameSpace)
            });
        }

        // Repeats

        // A section the page shows in full, and the words naming where.
        struct ShownSection
        {
            Reference from;
            Runs holder;
        };

        using ShownSections = std::vector<ShownSection>;

        [[nodiscard]] bool sameSection(const Reference& left, const Reference& right)
        {
            return left.note == right.note && left.anchor == right.anchor;
        }

        // The excerpt names the place showing its section already, or becomes that place.
        void pointAtShown(Excerpt& excerpt, ShownSections& shown, Runs holder)
        {
            const auto found = std::ranges::find_if(shown, [&excerpt](const ShownSection& entry) {
                return sameSection(entry.from, excerpt.from);
            });
            if (found != shown.end())
            {
                excerpt.shownAt = found->holder;
                return;
            }
            shown.push_back({ .from = excerpt.from, .holder = std::move(holder) });
        }

        void pointAtShown(TableGroup& group, ShownSections& shown)
        {
            for (TableRow& row : group.rows)
            {
                if (row.excerpt.has_value())
                    pointAtShown(*row.excerpt, shown, row.cells.front());
            }
            for (TableGroup& nested : group.groups)
                pointAtShown(nested, shown);
        }

        // **A section stands in full once on a page.** The page's own note holds its section,
        // else the first footnote reaching it does, and every later one names where it stands.
        void showSectionsOnce(Page& page)
        {
            ShownSections shown;
            for (const Section& section : page.sections)
            {
                if (section.kind == SectionKind::Note)
                    shown.push_back({ .from = section.excerpt.from, .holder = section.heading });
            }
            for (Section& section : page.sections)
            {
                for (TableGroup& group : section.table.groups)
                    pointAtShown(group, shown);
                for (Signature& signature : section.signatures)
                {
                    if (signature.excerpt.has_value())
                        pointAtShown(*signature.excerpt, shown, {});
                }
            }
        }

        // A type as a tooltip over a link to it shows it

        using Lines = std::vector<std::wstring>;

        // Whether a field is data an instance holds and a reader of its type reaches.
        [[nodiscard]] bool showsInPreview(const Field& field)
        {
            if (field.isStatic)
                return false;
            return field.access == Access::Public || field.access == Access::Published;
        }

        // The lines as one run of code, a line end between two.
        [[nodiscard]] Runs codeOf(const Lines& lines)
        {
            std::wstring text;
            for (std::size_t i = 0; i != lines.size(); ++i)
            {
                if (i != 0)
                    text += L'\n';
                text += lines[i];
            }
            return { code(std::move(text)) };
        }

        // A field the way its struct declares it, the initializer kept as spelled.
        [[nodiscard]] std::wstring cppFieldLine(const Field& field)
        {
            std::wstring line = field.type + L" " + field.name;
            if (field.value.starts_with(L'{'))
                line += field.value;
            else if (!field.value.empty())
                line += L" = " + field.value;
            return line + L";";
        }

        // Ends a declaration with its body: the lines set in between braces on lines of their own.
        void addCppBody(Lines& lines, const Lines& body)
        {
            lines.push_back(L"{");
            for (const std::wstring& line : body)
                lines.push_back(std::wstring{ k_cppIndent } + line);
            lines.push_back(L"};");
        }

        // **The C++ way**: the template head on a line of its own where the type has one, then an
        // alias on one line, or the head of a struct, a union or an enum with its public instance
        // fields or its members under it in braces, a line each. A struct holding no such field
        // shows nothing, since its body would read as empty.
        [[nodiscard]] std::optional<Lines> cppPreview(const Type& type)
        {
            Lines lines;
            if (!type.templateParameters.empty())
            {
                lines.push_back(std::wstring{ k_templateWord } + L"<" + type.templateParameters
                    + L">");
            }
            const std::wstring name{ bareName(type.name) };
            if (type.kind == TypeKind::Alias)
            {
                lines.push_back(L"using " + name + L" = " + type.target + L";");
                return lines;
            }
            if (type.kind == TypeKind::Struct || type.kind == TypeKind::Union)
            {
                Lines body;
                for (const Field& field : type.fields)
                {
                    if (showsInPreview(field))
                        body.push_back(cppFieldLine(field));
                }
                if (body.empty())
                    return std::nullopt;
                std::wstring head = std::wstring{ kindWord(type.kind) } + L" " + name;
                for (std::size_t i = 0; i != type.bases.size(); ++i)
                {
                    head += i == 0 ? k_colonJoin : k_listJoin;
                    head += type.bases[i];
                }
                lines.push_back(std::move(head));
                addCppBody(lines, body);
                return lines;
            }
            if (type.kind == TypeKind::Enum)
            {
                std::wstring head = type.scoped ? L"enum class " + name : L"enum " + name;
                if (!type.target.empty())
                    head += std::wstring{ k_colonJoin } + type.target;
                lines.push_back(std::move(head));
                Lines body;
                for (std::size_t i = 0; i != type.members.size(); ++i)
                {
                    const EnumMember& member = type.members[i];
                    std::wstring line = member.name;
                    if (!member.value.empty())
                        line += L" = " + member.value;
                    if (i + 1 != type.members.size())
                        line += L",";
                    body.push_back(std::move(line));
                }
                addCppBody(lines, body);
                return lines;
            }
            return std::nullopt;
        }

        // **The Pascal way**: the name with its generic parameters and an equals sign, then what
        // an alias or a built type stands for, a record's public fields a line each down to its
        // end, or an enum's members in brackets, run on as Delphi lists them. A record holding no
        // such field shows nothing.
        [[nodiscard]] std::optional<Lines> pascalPreview(const Type& type)
        {
            std::wstring name{ bareName(type.name) };
            if (!type.templateParameters.empty())
                name += L"<" + type.templateParameters + L">";
            const std::wstring equals = name + L" = ";
            if (type.kind == TypeKind::Alias || type.kind == TypeKind::Defined)
                return Lines{ equals + type.target + L";" };
            if (type.kind == TypeKind::Struct)
            {
                Lines lines = { equals + L"record" };
                for (const Field& field : type.fields)
                {
                    if (showsInPreview(field))
                    {
                        lines.push_back(std::wstring{ k_pascalIndent } + field.name + L": "
                            + field.type + L";");
                    }
                }
                if (lines.size() == 1)
                    return std::nullopt;
                lines.push_back(L"end;");
                return lines;
            }
            if (type.kind == TypeKind::Enum)
            {
                std::wstring list;
                for (const EnumMember& member : type.members)
                {
                    if (!list.empty())
                        list += k_listJoin;
                    list += member.name;
                    if (!member.value.empty())
                        list += L" = " + member.value;
                }
                return Lines{ equals + L"(" + list + L");" };
            }
            return std::nullopt;
        }
    }

    std::size_t TableGroup::rowCount() const
    {
        std::size_t count = rows.size();
        for (const TableGroup& group : groups)
            count += group.rowCount();
        return count;
    }

    std::size_t Table::rowCount() const
    {
        std::size_t count = 0;
        for (const TableGroup& group : groups)
            count += group.rowCount();
        return count;
    }

    // A module stands in the folder of every file it exports from - a type's folder, the
    // folder of a function's or a constant's file - so one exporting only functions has a row. A
    // header is named by its path, and a reader sees its file's name; a Delphi unit is seen by its
    // whole name, every part of which tells it from the others.
    ContentsFolders contentsOf(const Surface& surface)
    {
        using ModulesByName = std::map<std::wstring, ContentsModule>;
        std::map<std::wstring, ModulesByName> byFolder;
        std::set<std::wstring_view> headers;
        for (const Module& module : surface.modules())
        {
            if (module.isHeader)
                headers.insert(module.name);
        }
        const auto folderAt = [&surface](const Place& place) {
            return folderOf(surface.files()[place.file].relative);
        };
        for (const Type& type : surface.types())
        {
            if (type.isPublic())
                byFolder[type.folder][type.module].types.push_back(&type);
        }
        for (const FreeFunction& free : surface.functions())
        {
            if (!free.exported)
                continue;
            ContentsModule& owner = byFolder[folderAt(free.function.place)][free.module];
            owner.functions.push_back(&free);
        }
        for (const Variable& variable : surface.variables())
        {
            if (!variable.exported)
                continue;
            ContentsModule& owner = byFolder[folderAt(variable.field.place)][variable.module];
            owner.constants.push_back(&variable);
        }

        ContentsFolders folders;
        for (auto& [path, modules] : byFolder)
        {
            ContentsFolder folder{ .path = path, .name = folderNameOf(path) };
            for (auto& [name, contents] : modules)
            {
                std::ranges::sort(contents.types, [](const Type* left, const Type* right) {
                    return readsBefore(*left, *right);
                });
                contents.name = name;
                contents.header = headers.contains(name);
                if (contents.header)
                    contents.shownName = name.substr(name.rfind(L'/') + 1);
                else
                    contents.shownName = name;
                folder.modules.push_back(std::move(contents));
            }
            std::ranges::sort(folder.modules,
                [](const ContentsModule& left, const ContentsModule& right) {
                    return std::tuple{ !hasControl(left), lowered(left.shownName) }
                        < std::tuple{ !hasControl(right), lowered(right.shownName) };
                });
            folders.push_back(std::move(folder));
        }
        std::ranges::sort(folders, [](const ContentsFolder& left, const ContentsFolder& right) {
            return listsBefore(left.path, right.path);
        });
        return folders;
    }

    std::wstring folderOf(const std::wstring_view relativeFile)
    {
        const std::wstring_view path = relativeFile;
        const std::size_t slash = path.rfind(L'/');
        if (slash == std::wstring_view::npos)
            return {};
        return std::wstring{ path.substr(0, slash) };
    }

    // ContentsTree

    // Every folder is made before any module, so a folder's subfolders stand above its modules -
    // the root's modules stand under its folders.
    ContentsTree::ContentsTree(const ContentsFolders& folders)
    {
        for (const ContentsFolder& folder : folders)
        {
            if (!folder.path.empty())
                addFolder(folder.path);
        }
        for (const ContentsFolder& folder : folders)
        {
            const std::optional<std::size_t> parent = folder.path.empty()
                ? std::nullopt
                : folderRow(folder.path);
            for (const ContentsModule& module : folder.modules)
            {
                const std::size_t moduleIndex = addRow({
                    .kind = ContentsRowKind::Module,
                    .folder = folder.path,
                    .module = &module,
                    .parent = parent
                });
                m_named.emplace(&module, moduleIndex);
                m_modules.emplace(module.name, moduleIndex);
                for (const Type* type : module.types)
                {
                    const std::size_t typeIndex = addRow({
                        .kind = ContentsRowKind::Type,
                        .folder = folder.path,
                        .module = &module,
                        .type = type,
                        .parent = moduleIndex
                    });
                    m_named.emplace(type, typeIndex);
                }
            }
        }
    }

    std::optional<std::size_t> ContentsTree::rowOf(const std::wstring_view folder,
        const ContentsModule* module, const Type* type) const
    {
        const void* named = type;
        if (!named)
            named = module;
        if (named)
        {
            const auto found = m_named.find(named);
            if (found == m_named.end())
                return std::nullopt;
            return found->second;
        }
        if (folder.empty())
            return std::nullopt;
        return folderRow(folder);
    }

    std::optional<std::size_t> ContentsTree::folderRow(const std::wstring_view path) const
    {
        const auto found = m_folders.find(std::wstring{ path });
        if (found == m_folders.end())
            return std::nullopt;
        return found->second;
    }

    std::optional<std::size_t> ContentsTree::moduleRow(const std::wstring_view name) const
    {
        const auto found = m_modules.find(name);
        if (found == m_modules.end())
            return std::nullopt;
        return found->second;
    }

    std::size_t ContentsTree::addFolder(const std::wstring_view path)
    {
        if (const std::optional<std::size_t> found = folderRow(path))
            return *found;
        const std::size_t slash = path.rfind(k_folderSeparator);
        const std::optional<std::size_t> parent = slash == std::wstring_view::npos
            ? std::nullopt
            : std::optional<std::size_t>{ addFolder(path.substr(0, slash)) };
        const auto added = m_folders.emplace(std::wstring{ path }, m_rows.size()).first;
        return addRow({
            .kind = ContentsRowKind::Folder,
            .folder = added->first,
            .parent = parent
        });
    }

    std::size_t ContentsTree::addRow(ContentsRow row)
    {
        const std::size_t index = m_rows.size();
        if (row.parent)
            m_rows[*row.parent].children.push_back(index);
        else
            m_top.push_back(index);
        m_rows.push_back(std::move(row));
        return index;
    }

    std::wstring folderNameOf(const std::wstring_view path)
    {
        const std::size_t slash = path.rfind(L'/');
        if (slash == std::wstring_view::npos)
            return std::wstring{ path };
        return std::wstring{ path.substr(slash + 1) };
    }

    ContentsFolders foldersUnder(const ContentsFolders& folders,
        const std::wstring_view path)
    {
        ContentsFolders result;
        for (const ContentsFolder& folder : folders)
        {
            if (standsUnder(folder.path, path))
                result.push_back(folder);
        }
        return result;
    }

    Page folderPage(const Surface& surface, Notes& notes, const ContentsFolders& folders,
        const std::wstring_view path, const SourceRoots& roots)
    {
        Page page;
        page.kind = PageKind::Folder;
        page.surface = &surface;
        page.title = folderNameOf(path);
        page.badges.push_back(muted(std::wstring{ k_folderWord }));
        addFolderSources(page, surface, roots, path);
        addFolderContents(page, surface, notes, folders, path);
        showSectionsOnce(page);
        return page;
    }

    // **Modules standing in the tree's own folder are the root's**, which is home: the folder with
    // the empty path, whose exports the home page carries.
    Page homePage(const Surface& surface, Notes& notes, const ContentsFolders& folders,
        const std::wstring_view title)
    {
        Page page;
        page.kind = PageKind::Project;
        page.surface = &surface;
        page.title = std::wstring{ title };
        page.badges.push_back(muted(std::wstring{ k_projectWord }));
        if (!folders.empty())
            addFolderContents(page, surface, notes, folders, {});
        showSectionsOnce(page);
        return page;
    }

    Page modulePage(const Surface& surface, Notes& notes, const ContentsFolder& folder,
        const ContentsModule& module, const SourceRoots& roots)
    {
        Page page;
        const std::wstring word{ module.header ? k_headerWord : k_moduleWord };
        page.kind = PageKind::Module;
        page.surface = &surface;
        page.title = module.shownName;
        page.badges.push_back(muted(word));
        page.footnoteHome = folderFootnoteHome(surface, notes, folder.path);
        if (module.header)
            addFact(page, word, { code(module.name) });
        if (const Module* unit = interfaceOf(surface, module.name))
        {
            addSourceFact(page, roots, surface.files()[unit->file], {},
                { .place = { .file = unit->file, .line = 1 }, .language = unit->language });
        }
        if (!module.types.empty())
        {
            TableGroups groups;
            groups.push_back({ .rows = typeRows(surface, module.types) });
            addTableSection(page, L"Types", Columns::types, std::move(groups));
        }
        addRowsSection(page, L"Functions", Columns::functions, functionRows(surface, module));
        addRowsSection(page, L"Constants", Columns::fields, constantRows(surface, module));
        showSectionsOnce(page);
        return page;
    }

    Page exportPage(const Surface& surface, Notes& notes, const ContentsFolder& folder,
        const ContentsModule& module, const std::wstring_view name, const SourceRoots& roots)
    {
        Page page;
        page.surface = &surface;
        page.title = std::wstring{ name };
        page.footnoteHome = folderFootnoteHome(surface, notes, folder.path);
        Overloads overloads;
        const Function* first = nullptr;
        for (const FreeFunction* free : module.functions)
        {
            if (free->function.name != name)
                continue;
            if (!first)
                first = &free->function;
            overloads.push_back({
                .code = signatureRuns(surface, *free, k_signatureJoin),
                .holder = &free->function,
                .nameSpace = free->nameSpace,
                .folder = folder.path,
                .heading = std::wstring{ name },
                .language = surface.languageOf(free->module)
            });
        }
        if (first)
        {
            addExportFacts(page, surface, roots, overloads.front().nameSpace, module, first->place);
            addOverloads(page, surface, notes, k_functionWord, std::move(overloads));
            showSectionsOnce(page);
            return page;
        }
        for (const Variable* variable : module.constants)
        {
            if (variable->field.name == name)
            {
                fillConstantPage(page, surface, notes, roots, folder.path, module, *variable);
                break;
            }
        }
        showSectionsOnce(page);
        return page;
    }

    Page methodPage(const Surface& surface, Notes& notes, const Type& type,
        const std::wstring_view name, const SourceRoots& roots)
    {
        Page page;
        page.surface = &surface;
        page.type = &type;
        page.scope = { { .text = type.name, .link = type.qualifiedName },
            plain(std::wstring{ k_scope }) };
        page.title = std::wstring{ name };
        addFact(page, moduleWordOf(surface, type.module), { code(type.module, type.module) });
        page.footnoteHome = typeFootnoteHome(surface, notes, type);
        // An override with no comment of its own speaks with its base's, as its row does.
        const BaseWalk walk = baseWalkOf(surface, type);
        const std::span<const ReachedBase> above = basesAbove(walk.reached, nullptr);
        const SourceLanguage language = surface.languageOf(type.module);
        FunctionKind kind = FunctionKind::Function;
        Overloads overloads;
        for (const Function& function : type.functions)
        {
            if (function.name != name)
                continue;
            if (overloads.empty())
            {
                kind = function.kind;
                addPlaceFact(page, surface, roots, function.place, language);
            }
            // A constructor's name is its type's, and so is the section that name heads.
            std::wstring heading = function.kind == FunctionKind::Constructor
                ? std::wstring{}
                : footnoteHeading(type.name, name);
            overloads.push_back({
                .code = signatureRuns(surface, type, function, k_signatureJoin),
                .holder = &commentHolder(function, above, language),
                .nameSpace = type.nameSpace,
                .folder = type.folder,
                .heading = std::move(heading),
                .access = function.access,
                .language = language
            });
        }
        addOverloads(page, surface, notes, methodKindWord(kind), std::move(overloads));
        showSectionsOnce(page);
        return page;
    }

    std::vector<std::wstring_view> methodNames(const Type& type)
    {
        std::vector<std::wstring_view> result;
        for (const Function& function : type.functions)
        {
            if (isAccessor(type, function))
                continue;
            if (std::ranges::find(result, function.name) == result.end())
                result.push_back(function.name);
        }
        return result;
    }

    std::vector<std::wstring_view> exportNames(const ContentsModule& module)
    {
        std::vector<std::wstring_view> result;
        for (const FreeFunction* free : module.functions)
        {
            if (std::ranges::find(result, free->function.name) == result.end())
                result.push_back(free->function.name);
        }
        for (const Variable* variable : module.constants)
            result.push_back(variable->field.name);
        return result;
    }

    Runs hintRunsOf(const Surface& surface, const Comment& comment,
        const std::wstring_view nameSpace)
    {
        return hintRuns(surface, comment, nameSpace);
    }

    Runs noteRunsOf(const Surface& surface, const std::wstring& text,
        const std::wstring_view nameSpace)
    {
        return inlineRuns(text, surface, nameSpace);
    }

    std::wstring memberLink(const Type& type, const std::wstring_view member)
    {
        return type.qualifiedName + std::wstring{ k_scope } + std::wstring{ member };
    }

    std::wstring exportLink(const ContentsModule& module, const std::wstring_view name)
    {
        return module.name + std::wstring{ k_scope } + std::wstring{ name };
    }

    std::optional<MemberLink> readMemberLink(const std::wstring_view link)
    {
        const std::size_t cut = link.rfind(k_scope);
        if (cut == std::wstring_view::npos || cut == 0 || cut + k_scope.size() == link.size())
            return std::nullopt;
        return MemberLink{
            .type = link.substr(0, cut),
            .member = link.substr(cut + k_scope.size())
        };
    }

    Page typePage(const Surface& surface, Notes& notes, const Type& type, const SourceRoots& roots)
    {
        Page page;
        page.kind = PageKind::Type;
        page.surface = &surface;
        page.type = &type;
        addTypeHead(page, surface, roots, type);
        page.footnoteHome = typeFootnoteHome(surface, notes, type);
        const DerivedIndex derived = derivedIndexOf(surface);
        const ReachedBases bases = addTypeBases(page, surface, derived, type);
        addDerivedTypes(page, derived, type);
        addProperties(page, surface, notes, type, bases);
        addEvents(page, surface, notes, type, bases);
        if (leadsWithFields(type))
        {
            addFields(page, surface, notes, type);
            addMethods(page, surface, type, bases);
        }
        else
        {
            addMethods(page, surface, type, bases);
            addFields(page, surface, notes, type);
        }
        addMembers(page, surface, notes, type);
        addPropertyOf(page, surface, derived, type);
        addEventOf(page, surface, derived, type);
        // The note is a footnote: it stands at the foot of what is the type's own, before the
        // types beside it.
        addTypeNote(page, surface, notes, type);
        addSiblings(page, surface, type);
        showSectionsOnce(page);
        return page;
    }

    std::optional<Preview> previewOf(const Surface& surface, const std::wstring_view link)
    {
        const Type* type = surface.typeNamed(link);
        if (!type)
            return std::nullopt;
        const SourceLanguage language = surface.languageOf(type->module);
        const std::optional<Lines> lines = language == SourceLanguage::Pascal
            ? pascalPreview(*type)
            : cppPreview(*type);
        if (!lines.has_value())
            return std::nullopt;
        return Preview{ .code = codeOf(*lines), .language = language };
    }
}
