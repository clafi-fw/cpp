module SeeDocs_App.Pages;

import SeeDocs_App.Database;
import SeeDocs_App.Notes;
import SeeDocs_App.Surface;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    namespace
    {
        // The tree's folders in the order a reader meets them; any other follows by name.
        constexpr auto k_chapterOrder = std::to_array<std::wstring_view>({
            L"Controls",
            L"Controls/Y-Base",
            L"Controls/Dialogs",
            L"Controls/Grids",
            L"Controls/Browser",
            L"Controls/Documents",
            L"Application",
            L"Y-Core/Foundation",
            L"Y-Core/Context",
            L"Y-Core/System",
            L"Y-Core/Graphics",
            L"Y-Core/TextEngine",
            L"Y-Core/Syntax",
            L"Y-Core/AppTheme",
            L"Y-Core/Transfer",
            L"Y-Core/Dom",
            L"Dom",
            L"Dom/Formats",
            L"Icons",
            L"PathArt",
            L"Platform",
            L"Platform/Windows",
            L"Platform/Linux",
            L"Platform/Wayland",
            L"Diagnostic",
            L"StdActions"
        });

        // The prefix a folder carries to sort last on disk, which a reader is not shown.
        constexpr std::wstring_view k_sortPrefix = L"Y-";
        constexpr std::wstring_view k_noHint = L"no hint";
        constexpr std::wstring_view k_matchedByName =
            L"matched by name - the comment carries no See";
        constexpr std::wstring_view k_controlWord = L"control";
        constexpr std::wstring_view k_noteExtension = L".md";
        constexpr std::wstring_view k_codeFence = L"    ";
        constexpr std::wstring_view k_bulletMark = L"- ";
        constexpr std::wstring_view k_boldMark = L"**";
        constexpr wchar_t k_codeMark = L'`';
        constexpr std::wstring_view k_scope = L"::";
        constexpr std::wstring_view k_separator = L" \u00B7 ";
        constexpr std::wstring_view k_baseJoin = L": ";
        constexpr std::wstring_view k_ownLabel = L"Own";
        constexpr std::wstring_view k_fromLabel = L"From ";
        constexpr std::wstring_view k_setterPrefix = L"set";
        constexpr std::wstring_view k_constWord = L"const";
        constexpr std::wstring_view k_sourceJoin = L" \u203A ";
        constexpr std::wstring_view k_publicMethods = L"Public methods";
        constexpr std::wstring_view k_protectedMethods = L"Protected methods";
        constexpr std::wstring_view k_templateWord = L"template";
        constexpr std::wstring_view k_methodWord = L"method";
        constexpr std::wstring_view k_accessMark = L":";
        constexpr std::wstring_view k_overloadNoun = L"overload";
        constexpr std::wstring_view k_signatureJoin = L"\n";   // after the template head
        constexpr std::wstring_view k_hintJoin = L" ";         // the same on a hint's one line
        constexpr std::size_t k_maxTreeDepth = 16;   // what a base chain is followed to

        // The columns of each table a page carries.
        namespace Columns
        {
            const TableColumns types = {
                { L"Name" }, { L"Kind" }, { L"Hint", true }
            };
            const TableColumns properties = {
                { L"Name" }, { L"Type" }, { L"Default" }, { L"Hint", true }
            };
            const TableColumns events = {
                { L"Event" }, { L"Type" }, { L"Hint", true }
            };
            const TableColumns methods = {
                { L"Name" }, { L"Hint", true }
            };
            const TableColumns functions = {
                { L"Signature" }, { L"Hint", true }
            };
            const TableColumns fields = {
                { L"Name" }, { L"Type" }, { L"Value" }, { L"Hint", true }
            };
            const TableColumns members = {
                { L"Name" }, { L"Value" }, { L"Hint", true }
            };
            const TableColumns chapters = {
                { L"Chapter", true }, { L"Modules" }, { L"Types" }, { L"Controls" }
            };
        }

        [[nodiscard]] std::wstring lowered(const std::wstring_view text)
        {
            std::wstring result;
            for (const wchar_t current : text)
                result += static_cast<wchar_t>(std::towlower(current));
            return result;
        }

        [[nodiscard]] std::size_t kindRank(const TypeKind kind)
        {
            switch (kind)
            {
                case TypeKind::Class:
                    return 0;
                case TypeKind::Struct:
                case TypeKind::Union:
                    return 1;
                case TypeKind::Enum:
                    return 2;
                case TypeKind::Alias:
                    return 3;
                case TypeKind::Concept:
                    return 4;
            }
            return 5;
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

        [[nodiscard]] std::wstring fileLineOf(const Surface& surface, const Place& place)
        {
            return surface.files()[place.file].relative + L":" + std::to_wstring(place.line);
        }

        [[nodiscard]] std::wstring countOf(const std::size_t value, const std::wstring_view noun)
        {
            return std::to_wstring(value) + L" " + std::wstring{ noun } + (value == 1 ? L"" : L"s");
        }

        // Runs

        [[nodiscard]] Run plain(std::wstring text)
        {
            return { .text = std::move(text) };
        }

        [[nodiscard]] Run code(std::wstring text, std::wstring link = {})
        {
            return { .text = std::move(text), .style = RunStyle::Code, .link = std::move(link) };
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

        // The type a name spelled in a namespace links to - one the surface has a page for.
        [[nodiscard]] const Type* linkedType(const Surface& surface, const std::wstring_view name,
            const std::wstring_view nameSpace)
        {
            const Type* type = surface.resolve(name, nameSpace);
            if (type && type->isPublic())
                return type;
            return nullptr;
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
                const Type* type = linkedType(surface, name, nameSpace);
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

        // The hint as it reads in a namespace, the types it names linked.
        [[nodiscard]] Runs hintRuns(const Surface& surface, const Comment& comment,
            const std::wstring_view nameSpace)
        {
            if (comment.text.empty())
                return { missing(std::wstring{ k_noHint }) };
            return linkNames(surface, { plain(comment.text) }, nameSpace);
        }

        // A spelled type, linked to its page where the surface has one.
        [[nodiscard]] Run typeRun(const Surface& surface, const std::wstring& spelled,
            const std::wstring_view nameSpace)
        {
            const Type* type = linkedType(surface, spelled, nameSpace);
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
            return { muted(L"= " + value) };
        }

        // The kind a type reads as in a table - a control before anything else it is.
        [[nodiscard]] Runs kindCell(const Type& type)
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

        // The folders a category is made of, the sort prefix taken off each.
        [[nodiscard]] std::vector<std::wstring_view> foldersOf(std::wstring_view category)
        {
            std::vector<std::wstring_view> folders;
            while (!category.empty())
            {
                const std::size_t slash = category.find(L'/');
                std::wstring_view folder = category.substr(0, slash);
                if (folder.starts_with(k_sortPrefix))
                    folder.remove_prefix(k_sortPrefix.size());
                folders.push_back(folder);
                category = slash == std::wstring_view::npos
                    ? std::wstring_view{}
                    : category.substr(slash + 1);
            }
            return folders;
        }

        // Whether a note is one of the category's own: named after one of its folders, as
        // Controls-Base is after Controls/Y-Base and Control-Foundation after Y-Core/Foundation.
        [[nodiscard]] bool noteBelongs(const Note& note, const std::wstring_view category)
        {
            for (const std::wstring_view folder : foldersOf(category))
            {
                if (note.stem.find(folder) != std::wstring::npos)
                    return true;
            }
            return false;
        }

        // The section named after a declaration: a heading spelling its name, in a note of its
        // own category - the one named exactly after a folder ahead of any other.
        [[nodiscard]] const NoteHit* namedHit(const NoteHits& hits, const std::wstring_view name,
            const std::wstring_view category)
        {
            const NoteHit* found = nullptr;
            for (const NoteHit& hit : hits)
            {
                if (hit.section->heading != name || !noteBelongs(*hit.note, category))
                    continue;
                const std::vector<std::wstring_view> folders = foldersOf(category);
                if (std::ranges::find(folders, hit.note->stem) != folders.end())
                    return &hit;
                if (!found)
                    found = &hit;
            }
            return found;
        }

        // The section the comment references; failing a reference, where asked, the section
        // named after the declaration.
        [[nodiscard]] FoundNote noteFor(Notes& notes, const Comment& comment,
            const std::wstring_view name, const std::wstring_view category, const bool byName)
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
            if (!byName)
                return {};
            const NoteHits hits = notes.sectionsNamed(slug(name));
            const NoteHit* hit = namedHit(hits, name, category);
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
            excerpt.source.push_back(muted(found.note->stem + std::wstring{ k_noteExtension }
                + std::wstring{ k_sourceJoin } + found.section->heading));
            if (found.byName)
                excerpt.source.push_back(missing(L"  " + std::wstring{ k_matchedByName }));
            excerpt.blocks = blocksOf(found.section->lines, surface, nameSpace);
            return excerpt;
        }

        // The note a member's comment references, under its row or on its page.
        [[nodiscard]] std::optional<Excerpt> memberExcerpt(const Surface& surface, Notes& notes,
            const Comment& comment, const std::wstring_view name, const Type& owner)
        {
            return excerptOf(noteFor(notes, comment, name, owner.category, false), surface,
                owner.nameSpace);
        }

        // Sections

        Section& addSection(Page& page, const SectionKind kind, Runs heading,
            std::wstring hint = {})
        {
            page.sections.push_back({
                .kind = kind,
                .heading = std::move(heading),
                .hint = std::move(hint)
            });
            return page.sections.back();
        }

        // A table of rows under one heading or none, counted in the heading's hint.
        Section& addTableSection(Page& page, std::wstring heading, const TableColumns& columns,
            TableGroups groups)
        {
            Section& section = addSection(page, SectionKind::Table, { plain(std::move(heading)) });
            section.table.columns = columns;
            section.table.groups = std::move(groups);
            section.hint = countOf(section.table.rowCount(), L"element");
            return section;
        }

        void addFact(Page& page, std::wstring label, Runs value)
        {
            page.facts.push_back({ .label = std::move(label), .value = std::move(value) });
        }

        // The head of a type page

        void addTypeHead(Page& page, const Surface& surface, const Type& type)
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
            addFact(page, L"namespace", { code(type.nameSpace) });
            addFact(page, L"module", { code(type.module, type.module) });
            addFact(page, L"source", { code(fileLineOf(surface, type.place)) });
            if (!type.target.empty())
            {
                const bool alias = type.kind == TypeKind::Alias;
                addFact(page, alias ? L"stands for" : L"requires",
                    { alias ? typeRun(surface, type.target, type.nameSpace) : code(type.target) });
            }
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

        [[nodiscard]] Shape shapeOf(const Function& function)
        {
            const std::wstring_view signature = function.signature;
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
        [[nodiscard]] std::wstring shapeKey(const Function& function)
        {
            const Shape shape = shapeOf(function);
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

        // The walk up through a type's bases: the lines it writes, the way so far, and every base
        // it reaches, once each, in the order the lines name them.
        struct BaseWalk
        {
            Chains chains;
            Runs chain;
            ReachedBases reached;
        };

        // A base's spelling and the namespace it is read in.
        struct BaseReading
        {
            std::wstring spelling;
            std::wstring_view nameSpace;
        };

        // Along a type's bases: a base that is one of the type's template parameters stands for
        // the argument the spelling gave it, which is how a mixin hands its host on.
        void addBaseChains(BaseWalk& walk, const Surface& surface, const Type& type,
            std::wstring_view spelled, std::wstring_view nameSpace, std::size_t depth,
            const Flow& flow);

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
        [[nodiscard]] Flow flowFrom(const Surface& surface, const Type& type,
            const std::wstring_view spelled, const std::wstring_view nameSpace, const Flow& into)
        {
            Flow flow = into;
            for (const Function& function : type.functions)
            {
                if (function.kind != FunctionKind::Function)
                    continue;
                flow.hidden.insert(function.name);
                flow.shapes.insert(shapeKey(function));
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

        // A base put on the way: a colon, then its spelling, linked where the surface has its page.
        void joinBase(Runs& chain, std::wstring spelling, const Type* type)
        {
            chain.push_back(muted(std::wstring{ k_baseJoin }));
            if (type && type->isPublic())
                chain.push_back(code(std::move(spelling), type->qualifiedName));
            else
                chain.push_back(code(std::move(spelling)));
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

        // A base on the way: its spelling with a private alias replaced, the type its head names,
        // and that type's own bases after it; a way that ends here is a line. A nested base is a
        // part of some type, not a type of its own, and is left out.
        void addBaseChain(BaseWalk& walk, const Surface& surface, const BaseReading& reading,
            const std::size_t depth, const Flow& flow)
        {
            const std::wstring resolvedSpelling =
                surface.resolvedBase(reading.spelling, reading.nameSpace);
            const Type* type = surface.resolve(resolvedSpelling, reading.nameSpace);
            if (type && isNested(*type))
                return;
            const std::size_t length = walk.chain.size();
            const std::size_t count = walk.chains.size();
            joinBase(walk.chain, resolvedSpelling, type);
            if (type)
            {
                ReachedBase base = reachedBase(resolvedSpelling, *type, flow);
                Flow into = flow;
                into.hidden = base.hidden;
                into.republished = carriedPast(base.republished, *type);
                into.constructors = base.constructorsInherited;
                reachBase(walk.reached, std::move(base));
                if (depth < k_maxTreeDepth)
                {
                    addBaseChains(walk, surface, *type, resolvedSpelling, reading.nameSpace,
                        depth + 1, into);
                }
            }
            if (walk.chains.size() == count)
                walk.chains.push_back(walk.chain);
            walk.chain.resize(length);
        }

        void addBaseChains(BaseWalk& walk, const Surface& surface, const Type& type,
            const std::wstring_view spelled, const std::wstring_view nameSpace,
            const std::size_t depth, const Flow& flow)
        {
            const Flow outward = flowFrom(surface, type, spelled, nameSpace, flow);
            for (const std::wstring& base : type.bases)
            {
                const std::optional<BaseReading> reading =
                    readBase(type, spelled, nameSpace, base);
                if (reading)
                {
                    addBaseChain(walk, surface, *reading, depth, outward);
                    continue;
                }
                const std::size_t length = walk.chain.size();
                joinBase(walk.chain, base, nullptr);
                walk.chains.push_back(walk.chain);
                walk.chain.resize(length);
            }
        }

        // The ways up from a type; a type with no bases of its own kind has none.
        [[nodiscard]] BaseWalk baseWalkOf(const Surface& surface, const Type& type)
        {
            BaseWalk walk;
            if (type.kind != TypeKind::Class && type.kind != TypeKind::Struct)
                return walk;
            addBaseChains(walk, surface, type, type.name, type.nameSpace, 0,
                Flow{ .constructors = true });
            return walk;
        }

        // The lines under the title; answers the bases they reach, whose members the tables carry.
        [[nodiscard]] ReachedBases addTypeBases(Page& page, const Surface& surface,
            const Type& type)
        {
            BaseWalk walk = baseWalkOf(surface, type);
            page.bases = std::move(walk.chains);
            return std::move(walk.reached);
        }

        // The derived types

        // Whether a type names another among its bases - as the base, or as the argument a mixin
        // base hands on: one whose template's own bases name the parameter standing in that place.
        [[nodiscard]] bool derivesDirectly(const Surface& surface, const Type& type,
            const Type& base)
        {
            for (const std::wstring& spelled : type.bases)
            {
                if (isTemplateParameter(type, spelled))
                    continue;
                const std::wstring resolvedSpelling =
                    surface.resolvedBase(spelled, type.nameSpace);
                const Type* direct = surface.resolve(resolvedSpelling, type.nameSpace);
                if (direct == &base)
                    return true;
                if (!direct)
                    continue;
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
                    if (index < arguments.size()
                        && surface.resolve(arguments[index], type.nameSpace) == &base)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        void addDerived(Branches& branches, const Surface& surface, const Type& base,
            const std::size_t depth)
        {
            TypeList derived;
            for (const Type& type : surface.types())
            {
                if (type.isPublic() && !isNested(type) && &type != &base
                    && derivesDirectly(surface, type, base))
                {
                    derived.push_back(&type);
                }
            }
            std::ranges::sort(derived, [](const Type* left, const Type* right) {
                return readsBefore(*left, *right);
            });
            for (const Type* type : derived)
            {
                Branch branch{ .text = { code(type->name) }, .type = type };
                if (depth < k_maxTreeDepth)
                    addDerived(branch.children, surface, *type, depth + 1);
                branches.push_back(std::move(branch));
            }
        }

        void addDerivedTypes(Page& page, const Surface& surface, const Type& type)
        {
            if (type.kind != TypeKind::Class && type.kind != TypeKind::Struct)
                return;
            Branches derived;
            addDerived(derived, surface, type, 0);
            if (derived.empty())
                return;
            Section& section = addSection(page, SectionKind::Tree, { plain(L"Derived types") });
            section.branches = std::move(derived);
        }

        // The type's own note, where its comment references one or a section is named after it.
        void addTypeNote(Page& page, const Surface& surface, Notes& notes, const Type& type)
        {
            std::optional<Excerpt> excerpt = excerptOf(
                noteFor(notes, type.comment, type.name, type.category, true), surface,
                type.nameSpace);
            if (!excerpt.has_value())
                return;
            Section& section = addSection(page, SectionKind::Note, excerpt->source);
            section.excerpt = std::move(*excerpt);
        }

        // The member tables of a type page

        // A type's own properties, one row each.
        [[nodiscard]] TableRows propertyRows(const Surface& surface, Notes& notes,
            const Type& type)
        {
            TableRows rows;
            for (const Property& property : type.properties)
            {
                Runs value = valueCell(property.defaultValue);
                if (property.form == PropertyForm::Required)
                    value = { muted(L"required") };
                Runs spelled = { typeRun(surface, property.type, type.nameSpace) };
                for (const std::wstring& accepted : property.accepts)
                {
                    spelled.push_back(muted(L" | "));
                    spelled.push_back(typeRun(surface, accepted, type.nameSpace));
                }
                rows.push_back({
                    .cells = { { code(property.name) }, std::move(spelled), std::move(value),
                        hintRuns(surface, property.comment, type.nameSpace) },
                    .excerpt = memberExcerpt(surface, notes, property.comment, property.name, type)
                });
            }
            return rows;
        }

        // A type's own events, one row each; the hint names what the event carries.
        [[nodiscard]] TableRows eventRows(const Surface& surface, Notes& notes, const Type& type)
        {
            TableRows rows;
            for (const Event& event : type.events)
            {
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
                    .excerpt = memberExcerpt(surface, notes, event.comment, event.type, type)
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

        // The comment a function without one takes: the comment on the function of the same shape
        // in the nearest base above that carries one - an override's virtual.
        [[nodiscard]] const Comment& commentOf(const Function& function,
            const std::span<const ReachedBase> above)
        {
            if (!function.comment.text.empty())
                return function.comment;
            const std::wstring key = shapeKey(function);
            for (const ReachedBase& base : above)
            {
                for (const Function& candidate : base.type->functions)
                {
                    if (candidate.name == function.name && !candidate.comment.text.empty()
                        && shapeKey(candidate) == key)
                    {
                        return candidate.comment;
                    }
                }
            }
            return function.comment;
        }

        // Whether a method stands in the table: a property's accessor never, the type's own
        // otherwise; a base's when it reaches the type - a constructor taken with using, a
        // function no type below declares again by name or by shape, nothing deleted, and no
        // destructor.
        [[nodiscard]] bool reachesTable(const Type& owner, const Function& function,
            const ReachedBase* reached)
        {
            if (isAccessor(owner, function))
                return false;
            if (!reached)
                return true;
            if (function.isDeleted)
                return false;
            switch (function.kind)
            {
                case FunctionKind::Constructor:
                    return reached->constructorsInherited;
                case FunctionKind::Destructor:
                    return false;
                case FunctionKind::Function:
                    return !reached->hidden.contains(function.name)
                        && !reached->shapes.contains(shapeKey(function));
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
        // using-declaration below republished it with.
        [[nodiscard]] Access shownAccess(const Function& function, const ReachedBase* reached)
        {
            if (reached)
            {
                const auto republished = reached->republished.find(function.name);
                if (republished != reached->republished.end())
                    return republished->second;
            }
            return function.access;
        }

        // A method as code: the template head where it has one, then the signature as spelled,
        // joined as asked; a type it names that the surface has a page for is linked, the owner's
        // and the method's own template parameters never.
        [[nodiscard]] Runs signatureRuns(const Surface& surface, const Type& owner,
            const Function& function, const std::wstring_view join)
        {
            std::wstring spelled;
            if (!function.templateParameters.empty())
            {
                spelled = std::wstring{ k_templateWord } + L"<" + function.templateParameters
                    + L">" + std::wstring{ join };
            }
            spelled += function.signature;
            Names leftOut = templateParameterNames(owner.templateParameters);
            for (std::wstring& name : templateParameterNames(function.templateParameters))
                leftOut.push_back(std::move(name));
            return linkNames(surface, { code(std::move(spelled)) }, owner.nameSpace, leftOut);
        }

        // One owner's methods of one access as a group's rows, one per name: the name linked to
        // the method's page, every overload's signature in the name's hint a line each, and the
        // comment of the first overload that has one beside it.
        [[nodiscard]] TableGroup methodGroup(const Surface& surface, const Type& owner,
            const ReachedBase* reached, const std::span<const ReachedBase> above,
            const Access access)
        {
            // The row a name has, and whether any overload so far gave it a comment.
            struct NameRow
            {
                std::size_t index{ 0 };
                bool commented{ false };
            };
            TableGroup group;
            std::map<std::wstring, NameRow> rowByName;
            for (const Function& function : owner.functions)
            {
                if (!reachesTable(owner, function, reached))
                    continue;
                if (shownAccess(function, reached) != access)
                    continue;
                const Comment& comment = commentOf(function, above);
                Runs signature = signatureRuns(surface, owner, function, k_hintJoin);
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
                        .hint = std::move(signature)
                    });
                    continue;
                }
                TableRow& row = group.rows[known->second.index];
                row.hint.push_back(code(std::wstring{ k_signatureJoin }));
                row.hint.insert(row.hint.end(), std::make_move_iterator(signature.begin()),
                    std::make_move_iterator(signature.end()));
                if (!known->second.commented && !comment.text.empty())
                {
                    row.cells[1] = hintRuns(surface, comment, owner.nameSpace);
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

        // A member table over the type and its bases, its hint telling the own rows apart.
        template<typename GroupOf>
        void addMemberSection(Page& page, std::wstring heading, const TableColumns& columns,
            const Type& type, const ReachedBases& bases, const GroupOf& groupOf)
        {
            MemberGroups groups = ownAndInherited(type, bases, groupOf);
            if (groups.groups.empty())
                return;
            Section& section = addTableSection(page, std::move(heading), columns,
                std::move(groups.groups));
            section.hint = memberCount(groups.own, section.table.rowCount() - groups.own);
        }

        void addProperties(Page& page, const Surface& surface, Notes& notes, const Type& type,
            const ReachedBases& bases)
        {
            addMemberSection(page, L"Properties", Columns::properties, type, bases,
                [&](const Type& owner, const ReachedBase*) {
                    return TableGroup{ .rows = propertyRows(surface, notes, owner) };
                });
        }

        void addEvents(Page& page, const Surface& surface, Notes& notes, const Type& type,
            const ReachedBases& bases)
        {
            addMemberSection(page, L"Events", Columns::events, type, bases,
                [&](const Type& owner, const ReachedBase*) {
                    return TableGroup{ .rows = eventRows(surface, notes, owner) };
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
                    });
            };
            add(k_publicMethods, Access::Public);
            add(k_protectedMethods, Access::Protected);
        }

        // The method page

        // What a method is called on its page: a constructor or a destructor by that word.
        [[nodiscard]] std::wstring_view methodKindWord(const FunctionKind kind)
        {
            if (kind == FunctionKind::Function)
                return k_methodWord;
            return kindWord(kind);
        }

        // The section the declarations of an access stand in, headed the C++ way - public: -
        // and made when the first of them is met.
        Section& accessSection(Page& page, const Access access)
        {
            const std::wstring label = std::wstring{ accessWord(access) }
                + std::wstring{ k_accessMark };
            for (Section& section : page.sections)
            {
                if (section.kind == SectionKind::Signature
                    && section.heading.front().text == label)
                {
                    return section;
                }
            }
            return addSection(page, SectionKind::Signature, { code(label) });
        }

        // One declaration under its access: the code, the hint under it, and the note the comment
        // references after that. A method with no comment of its own takes the one its override's
        // base carries, as its row does.
        void addSignature(Page& page, const Surface& surface, Notes& notes, const Type& owner,
            const Function& function, const ReachedBases& bases)
        {
            const Comment& comment = commentOf(function, basesAbove(bases, nullptr));
            Signature signature{
                .code = signatureRuns(surface, owner, function, k_signatureJoin),
                .lead = hintRuns(surface, comment, owner.nameSpace)
            };
            std::optional<Excerpt> excerpt =
                memberExcerpt(surface, notes, comment, function.name, owner);
            if (excerpt.has_value())
                signature.excerpt = std::move(*excerpt);
            accessSection(page, function.access).signatures.push_back(std::move(signature));
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
                    .excerpt = memberExcerpt(surface, notes, field.comment, field.name, type)
                });
            }
            TableGroups groups;
            groups.push_back(std::move(group));
            addTableSection(page, L"Fields", Columns::fields, std::move(groups));
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
                    .excerpt = memberExcerpt(surface, notes, member.comment, member.name, type)
                });
            }
            TableGroups groups;
            groups.push_back(std::move(group));
            addTableSection(page, L"Members", Columns::members, std::move(groups));
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
        void addOwnersTree(Page& page, const Surface& surface, std::wstring heading,
            const Has& has)
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
                addDerived(branch.children, surface, *owner, 0);
                section.branches.push_back(std::move(branch));
            }
        }

        void addPropertyOf(Page& page, const Surface& surface, const Type& type)
        {
            addOwnersTree(page, surface, L"Property of", [&](const Type& owner) {
                return hasPropertyOf(surface, owner, type);
            });
        }

        void addEventOf(Page& page, const Surface& surface, const Type& type)
        {
            addOwnersTree(page, surface, L"Event of", [&](const Type& owner) {
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
                    .cells = { { nameRun(*type) }, kindCell(*type),
                        hintRuns(surface, type->comment, type->nameSpace) },
                    .type = type
                });
            }
            return rows;
        }

        // The types as a table: a reader tells a name, a kind and a hint apart unnamed.
        Section& addTypesSection(Page& page, std::wstring heading, TableGroups groups)
        {
            Section& section = addTableSection(page, std::move(heading), Columns::types,
                std::move(groups));
            section.table.header = false;
            return section;
        }

        // A row per chapter - its name linked, its modules, types and controls counted - and
        // nothing where there are none.
        void addChaptersSection(Page& page, const ContentsChapters& chapters)
        {
            if (chapters.empty())
                return;
            TableRows rows;
            for (const ContentsChapter& chapter : chapters)
            {
                std::size_t controls = 0;
                std::size_t types = 0;
                for (const ContentsModule& module : chapter.modules)
                {
                    for (const Type* type : module.types)
                    {
                        ++types;
                        if (type->isControl)
                            ++controls;
                    }
                }
                rows.push_back({
                    .cells = {
                        { { .text = chapter.name, .link = chapter.name } },
                        { plain(std::to_wstring(chapter.modules.size())) },
                        { plain(std::to_wstring(types)) },
                        { plain(std::to_wstring(controls)) }
                    }
                });
            }
            TableGroups groups;
            groups.push_back({ .rows = std::move(rows) });
            addTableSection(page, L"Chapters", Columns::chapters, std::move(groups));
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
            addTypesSection(page, L"Also in this module", std::move(groups));
        }

        void addFunctions(Page& page, const Surface& surface,
            const std::function<bool(const FreeFunction&)>& takes)
        {
            TableGroup group;
            for (const FreeFunction& free : surface.functions())
            {
                if (free.exported && takes(free))
                {
                    group.rows.push_back({ .cells = { { code(free.function.signature) },
                        hintRuns(surface, free.function.comment, free.nameSpace) } });
                }
            }
            if (group.rows.empty())
                return;
            TableGroups groups;
            groups.push_back(std::move(group));
            addTableSection(page, L"Functions", Columns::functions, std::move(groups));
        }

        void addConstants(Page& page, const Surface& surface,
            const std::function<bool(const Variable&)>& takes)
        {
            TableGroup group;
            for (const Variable& variable : surface.variables())
            {
                if (variable.exported && takes(variable))
                {
                    group.rows.push_back({ .cells = { { code(variable.field.name) },
                        { typeRun(surface, variable.field.type, variable.nameSpace) },
                        valueCell(variable.field.value),
                        hintRuns(surface, variable.field.comment, variable.nameSpace) } });
                }
            }
            if (group.rows.empty())
                return;
            TableGroups groups;
            groups.push_back(std::move(group));
            addTableSection(page, L"Constants", Columns::fields, std::move(groups));
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

    ContentsChapters contentsOf(const Surface& surface)
    {
        std::map<std::wstring, std::map<std::wstring, TypeList>> byCategory;
        for (const Type& type : surface.types())
        {
            if (type.isPublic())
                byCategory[type.category][type.module].push_back(&type);
        }

        ContentsChapters chapters;
        const auto take = [&](const std::wstring& category) {
            const auto found = byCategory.find(category);
            if (found == byCategory.end())
                return;
            ContentsChapter chapter{ .category = category, .name = chapterNameOf(category) };
            for (auto& [module, types] : found->second)
            {
                std::ranges::sort(types, [](const Type* left, const Type* right) {
                    return readsBefore(*left, *right);
                });
                chapter.modules.push_back({
                    .name = module,
                    .shortName = moduleShortNameOf(module),
                    .types = std::move(types)
                });
            }
            std::ranges::sort(chapter.modules,
                [](const ContentsModule& left, const ContentsModule& right) {
                    return std::tuple{ !hasControl(left), lowered(left.shortName) }
                        < std::tuple{ !hasControl(right), lowered(right.shortName) };
                });
            chapters.push_back(std::move(chapter));
            byCategory.erase(found);
        };
        for (const std::wstring_view category : k_chapterOrder)
            take(std::wstring{ category });
        while (!byCategory.empty())
            take(byCategory.begin()->first);
        return chapters;
    }

    std::wstring chapterNameOf(const std::wstring_view category)
    {
        std::wstring result;
        std::size_t start = 0;
        while (start <= category.size())
        {
            std::size_t end = category.find(L'/', start);
            if (end == std::wstring_view::npos)
                end = category.size();
            std::wstring_view part = category.substr(start, end - start);
            if (part.starts_with(k_sortPrefix))
                part.remove_prefix(k_sortPrefix.size());
            if (!result.empty())
                result += L" / ";
            result += part;
            start = end + 1;
        }
        return result;
    }

    // The last dotted part; a partition keeps the module it belongs to in front of the colon.
    std::wstring moduleShortNameOf(const std::wstring_view module)
    {
        const std::size_t colon = module.find(L':');
        const std::wstring_view head = module.substr(0, colon);
        const std::size_t dot = head.rfind(L'.');
        const std::wstring_view last = dot == std::wstring_view::npos ? head : head.substr(dot + 1);
        if (colon == std::wstring_view::npos)
            return std::wstring{ last };
        return std::wstring{ last } + std::wstring{ module.substr(colon) };
    }

    std::wstring categoryOf(const std::wstring_view relativeFile)
    {
        const std::wstring_view path = relativeFile;
        const std::size_t slash = path.rfind(L'/');
        if (slash == std::wstring_view::npos)
            return {};
        return std::wstring{ path.substr(0, slash) };
    }

    std::wstring folderNameOf(const std::wstring_view category)
    {
        const std::size_t slash = category.rfind(L'/');
        std::wstring_view folder = category;
        if (slash != std::wstring_view::npos)
            folder = category.substr(slash + 1);
        if (folder.starts_with(k_sortPrefix))
            folder.remove_prefix(k_sortPrefix.size());
        return std::wstring{ folder };
    }

    ContentsChapters chaptersUnder(const ContentsChapters& chapters,
        const std::wstring_view category)
    {
        ContentsChapters result;
        for (const ContentsChapter& chapter : chapters)
        {
            const bool under = category.empty()
                ? !chapter.category.empty()
                : chapter.category.starts_with(category)
                    && chapter.category.size() > category.size()
                    && chapter.category[category.size()] == L'/';
            if (under)
                result.push_back(chapter);
        }
        return result;
    }

    // One row per chapter: its name linked, and its figures - modules, types, controls.
    Page contentsPage(const ContentsChapters& chapters, const std::wstring_view title)
    {
        Page page;
        page.title = std::wstring{ title };

        std::size_t controls = 0;
        std::size_t types = 0;
        std::size_t modules = 0;
        for (const ContentsChapter& chapter : chapters)
        {
            for (const ContentsModule& module : chapter.modules)
            {
                for (const Type* type : module.types)
                {
                    ++types;
                    if (type->isControl)
                        ++controls;
                }
            }
            modules += chapter.modules.size();
        }
        addFact(page, L"holds", {
            plain(countOf(controls, L"control")),
            muted(std::wstring{ k_separator }),
            plain(countOf(types, L"type")),
            muted(std::wstring{ k_separator }),
            plain(countOf(modules, L"module")),
            muted(std::wstring{ k_separator }),
            plain(countOf(chapters.size(), L"chapter"))
        });
        addChaptersSection(page, chapters);
        return page;
    }

    Page chapterPage(const Surface& surface, Notes&, const ContentsChapters& chapters,
        const ContentsChapter& chapter)
    {
        Page page;
        page.title = chapter.name;
        addFact(page, L"folder", { code(chapter.category) });

        std::size_t controls = 0;
        std::size_t types = 0;
        std::size_t withoutHint = 0;
        for (const ContentsModule& module : chapter.modules)
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
        Runs figures = {
            plain(countOf(controls, L"control")),
            muted(std::wstring{ k_separator }),
            plain(countOf(types, L"type")),
            muted(std::wstring{ k_separator }),
            plain(countOf(chapter.modules.size(), L"module"))
        };
        if (withoutHint != 0)
        {
            figures.push_back(muted(std::wstring{ k_separator }));
            figures.push_back(missing(countOf(withoutHint, L"type") + L" without a hint"));
        }
        addFact(page, L"holds", std::move(figures));
        addChaptersSection(page, chaptersUnder(chapters, chapter.category));

        TableGroups groups;
        for (const ContentsModule& module : chapter.modules)
        {
            groups.push_back({
                .label = { { .text = module.shortName, .link = module.name } },
                .hint = { plain(module.name) },
                .rows = typeRows(surface, module.types)
            });
        }
        Section& section = addTypesSection(page, L"Types", std::move(groups));
        section.table.groupColumn = L"Module";
        section.hint = countOf(types, L"element") + L" in "
            + countOf(chapter.modules.size(), L"module");
        addFunctions(page, surface, [&](const FreeFunction& free) {
            return categoryOf(surface.files()[free.function.place.file].relative)
                == chapter.category;
        });
        addConstants(page, surface, [&](const Variable& variable) {
            return categoryOf(surface.files()[variable.field.place.file].relative)
                == chapter.category;
        });
        return page;
    }

    Page modulePage(const Surface& surface, Notes&, const ContentsChapter& chapter,
        const ContentsModule& module)
    {
        Page page;
        page.title = module.shortName;
        addFact(page, L"module", { code(module.name) });
        addFact(page, L"chapter", { { .text = chapter.name, .link = chapter.name } });
        if (!module.types.empty())
        {
            TableGroups groups;
            groups.push_back({ .rows = typeRows(surface, module.types) });
            addTypesSection(page, L"Types", std::move(groups));
        }
        addFunctions(page, surface, [&](const FreeFunction& free) {
            return free.module == module.name;
        });
        addConstants(page, surface, [&](const Variable& variable) {
            return variable.module == module.name;
        });
        return page;
    }

    Page methodPage(const Surface& surface, Notes& notes, const Type& type,
        const std::wstring_view name)
    {
        Page page;
        page.scope = { { .text = type.name, .link = type.qualifiedName },
            plain(std::wstring{ k_scope }) };
        page.title = std::wstring{ name };
        std::vector<const Function*> overloads;
        for (const Function& function : type.functions)
        {
            if (function.name == name)
                overloads.push_back(&function);
        }
        const FunctionKind kind = overloads.empty()
            ? FunctionKind::Function
            : overloads.front()->kind;
        page.badges.push_back(muted(std::wstring{ methodKindWord(kind) }));
        if (overloads.size() > 1)
        {
            page.badges.push_back(muted(std::wstring{ k_separator }
                + countOf(overloads.size(), k_overloadNoun)));
        }
        addFact(page, L"module", { code(type.module, type.module) });
        const BaseWalk walk = baseWalkOf(surface, type);
        for (const Function* function : overloads)
            addSignature(page, surface, notes, type, *function, walk.reached);
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

    std::wstring memberLink(const Type& type, const std::wstring_view member)
    {
        return type.qualifiedName + std::wstring{ k_scope } + std::wstring{ member };
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

    Page typePage(const Surface& surface, Notes& notes, const Type& type)
    {
        Page page;
        addTypeHead(page, surface, type);
        const ReachedBases bases = addTypeBases(page, surface, type);
        addDerivedTypes(page, surface, type);
        addProperties(page, surface, notes, type, bases);
        addEvents(page, surface, notes, type, bases);
        addMethods(page, surface, type, bases);
        addFields(page, surface, notes, type);
        addMembers(page, surface, notes, type);
        addPropertyOf(page, surface, type);
        addEventOf(page, surface, type);
        // The note is a footnote: it stands at the foot of what is the type's own, before the
        // types beside it.
        addTypeNote(page, surface, notes, type);
        addSiblings(page, surface, type);
        return page;
    }
}
