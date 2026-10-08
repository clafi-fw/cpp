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
        constexpr std::wstring_view k_sourceJoin = L" \u203A ";
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
            const TableColumns functions = {
                { L"Signature" }, { L"Hint", true }
            };
            const TableColumns fields = {
                { L"Name" }, { L"Type" }, { L"Value" }, { L"Hint", true }
            };
            const TableColumns members = {
                { L"Name" }, { L"Value" }, { L"Hint", true }
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

        [[nodiscard]] Runs hintRuns(const Comment& comment)
        {
            if (comment.text.empty())
                return { missing(std::wstring{ k_noHint }) };
            return { plain(comment.text) };
        }

        // A spelled type, linked to its page where the surface has one.
        [[nodiscard]] Run typeRun(const Surface& surface, const std::wstring& spelled,
            const std::wstring_view nameSpace)
        {
            const Type* type = surface.resolve(spelled, nameSpace);
            if (type && type->isPublic())
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

        // The words of a note line: backticks set a run in code, a pair of ** sets one bold.
        [[nodiscard]] Runs inlineRuns(const std::wstring& text)
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
            return runs;
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
        // section as a subheading, four-space lines as code, dashed lines as bullets.
        [[nodiscard]] Blocks blocksOf(const Lines& lines)
        {
            Blocks blocks;
            std::wstring paragraph;
            const auto flush = [&]() {
                if (!paragraph.empty())
                {
                    blocks.push_back({
                        .kind = BlockKind::Paragraph,
                        .runs = inlineRuns(std::exchange(paragraph, {}))
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
                        .runs = inlineRuns(std::wstring{ trimmed(words.substr(heading)) })
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
                            ? std::wstring{}
                            : lines[i].substr(k_codeFence.size()));
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
                                block.items.push_back(inlineRuns(std::exchange(item, {})));
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
                        block.items.push_back(inlineRuns(item));
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

        // A found section as the words a page shows, named by where they come from.
        [[nodiscard]] std::optional<Excerpt> excerptOf(const FoundNote& found)
        {
            if (!found.section)
                return std::nullopt;
            Excerpt excerpt;
            excerpt.source.push_back(muted(found.note->stem + std::wstring{ k_noteExtension }
                + std::wstring{ k_sourceJoin } + found.section->heading));
            if (found.byName)
                excerpt.source.push_back(missing(L"  " + std::wstring{ k_matchedByName }));
            excerpt.blocks = blocksOf(found.section->lines);
            return excerpt;
        }

        // The note a member's comment references, under its row.
        [[nodiscard]] std::optional<Excerpt> memberExcerpt(Notes& notes,
            const Comment& comment, const std::wstring_view name, const std::wstring_view category)
        {
            return excerptOf(noteFor(notes, comment, name, category, false));
        }

        // Sections

        Section& addSection(Page& page, const SectionKind kind, Runs heading,
            std::wstring count = {})
        {
            page.sections.push_back({
                .kind = kind,
                .heading = std::move(heading),
                .count = std::move(count)
            });
            return page.sections.back();
        }

        // A table of rows under one heading or none, counted in the section's heading.
        Section& addTableSection(Page& page, std::wstring heading, const TableColumns& columns,
            TableGroups groups)
        {
            Section& section = addSection(page, SectionKind::Table, { plain(std::move(heading)) });
            section.table.columns = columns;
            section.table.groups = std::move(groups);
            section.count = std::to_wstring(section.table.rowCount());
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
            page.lead = hintRuns(type.comment);
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

        // A base of the surface the ways up reach, as the chain spells it.
        struct ReachedBase
        {
            std::wstring spelling;
            const Type* type{ nullptr };
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

        // Along a type's bases: a base that is one of the type's template parameters stands for
        // the argument the spelling gave it, which is how a mixin hands its host on.
        void addBaseChains(BaseWalk& walk, const Surface& surface, const Type& type,
            std::wstring_view spelled, std::wstring_view nameSpace, std::size_t depth);

        // A base put on the way: a colon, then its spelling, linked where the surface has its page.
        void joinBase(Runs& chain, std::wstring spelling, const Type* type)
        {
            chain.push_back(muted(std::wstring{ k_baseJoin }));
            if (type && type->isPublic())
                chain.push_back(code(std::move(spelling), type->qualifiedName));
            else
                chain.push_back(code(std::move(spelling)));
        }

        // A base met on the way, recorded the first time with the spelling it was met by.
        void reachBase(ReachedBases& reached, const std::wstring& spelling, const Type& type)
        {
            const bool met = std::ranges::any_of(reached, [&type](const ReachedBase& base) {
                return base.type == &type;
            });
            if (!met)
                reached.push_back({ .spelling = spelling, .type = &type });
        }

        // A base on the way: its spelling with a private alias replaced, the type its head names,
        // and that type's own bases after it; a way that ends here is a line. A nested base is a
        // part of some type, not a type of its own, and is left out.
        void addBaseChain(BaseWalk& walk, const Surface& surface, const std::wstring_view spelled,
            const std::wstring_view nameSpace, const std::size_t depth)
        {
            const std::wstring resolvedSpelling = surface.resolvedBase(spelled, nameSpace);
            const Type* type = surface.resolve(resolvedSpelling, nameSpace);
            if (type && isNested(*type))
                return;
            const std::size_t length = walk.chain.size();
            const std::size_t count = walk.chains.size();
            joinBase(walk.chain, resolvedSpelling, type);
            if (type)
                reachBase(walk.reached, resolvedSpelling, *type);
            if (type && depth < k_maxTreeDepth)
                addBaseChains(walk, surface, *type, resolvedSpelling, nameSpace, depth + 1);
            if (walk.chains.size() == count)
                walk.chains.push_back(walk.chain);
            walk.chain.resize(length);
        }

        void addBaseChains(BaseWalk& walk, const Surface& surface, const Type& type,
            const std::wstring_view spelled, const std::wstring_view nameSpace,
            const std::size_t depth)
        {
            const Names parameters = templateParameterNames(type.templateParameters);
            const Names arguments = argumentsOf(spelled);
            for (const std::wstring& base : type.bases)
            {
                const std::wstring_view head = plainType(withoutArguments(base));
                const auto parameter = std::ranges::find(parameters, head);
                if (parameter == parameters.end())
                {
                    addBaseChain(walk, surface, base, type.nameSpace, depth);
                    continue;
                }
                const std::size_t index = static_cast<std::size_t>(parameter - parameters.begin());
                if (index < arguments.size())
                {
                    addBaseChain(walk, surface, arguments[index], nameSpace, depth);
                    continue;
                }
                const std::size_t length = walk.chain.size();
                joinBase(walk.chain, base, nullptr);
                walk.chains.push_back(walk.chain);
                walk.chain.resize(length);
            }
        }

        // The lines under the title; answers the bases they reach, whose members the tables carry.
        [[nodiscard]] ReachedBases addTypeBases(Page& page, const Surface& surface,
            const Type& type)
        {
            if (type.kind != TypeKind::Class && type.kind != TypeKind::Struct)
                return {};
            BaseWalk walk;
            addBaseChains(walk, surface, type, type.name, type.nameSpace, 0);
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

        void addTypeNote(Page& page, Notes& notes, const Type& type)
        {
            std::optional<Excerpt> excerpt =
                excerptOf(noteFor(notes, type.comment, type.name, type.category, true));
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
                        hintRuns(property.comment) },
                    .excerpt = memberExcerpt(notes, property.comment, property.name, type.category)
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
                Runs hint = hintRuns(event.comment);
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
                    .excerpt = memberExcerpt(notes, event.comment, event.type, type.category)
                });
            }
            return rows;
        }

        // A type's own methods: the public ones as the group's rows, the protected ones under a
        // heading of their own.
        [[nodiscard]] TableGroup methodGroup(Notes& notes, const Type& type)
        {
            TableGroup group;
            TableGroup protectedGroup{
                .label = { plain(L"Protected"), muted(L" - what a derived class reaches") }
            };
            for (const Function& function : type.functions)
            {
                TableGroup& target = function.access == Access::Protected ? protectedGroup : group;
                target.rows.push_back({
                    .cells = { { code(function.signature) }, hintRuns(function.comment) },
                    .excerpt = memberExcerpt(notes, function.comment, function.name,
                        type.category)
                });
            }
            if (!protectedGroup.rows.empty())
                group.groups.push_back(std::move(protectedGroup));
            return group;
        }

        [[nodiscard]] bool isEmpty(const TableGroup& group)
        {
            return group.rows.empty() && group.groups.empty();
        }

        // The type's own group under Own, then each base's under From it, up the whole chain; a
        // base with nothing is left out, and the own group with nothing inherited goes unlabelled.
        template<typename GroupOf>
        [[nodiscard]] TableGroups ownAndInherited(const Type& type, const ReachedBases& bases,
            const GroupOf& groupOf)
        {
            TableGroups groups;
            TableGroup own = groupOf(type);
            const bool hasOwn = !isEmpty(own);
            if (hasOwn)
                groups.push_back(std::move(own));
            for (const ReachedBase& base : bases)
            {
                TableGroup group = groupOf(*base.type);
                if (isEmpty(group))
                    continue;
                group.label = { plain(std::wstring{ k_fromLabel }), code(base.spelling) };
                groups.push_back(std::move(group));
            }
            if (hasOwn && groups.size() > 1)
                groups.front().label = { plain(std::wstring{ k_ownLabel }) };
            return groups;
        }

        // What stands after a member table's heading: the own rows and the inherited ones apart.
        [[nodiscard]] std::wstring memberCount(const std::size_t own, const std::size_t inherited)
        {
            if (inherited == 0)
                return std::to_wstring(own);
            if (own == 0)
                return std::to_wstring(inherited) + L" inherited";
            return std::to_wstring(own) + L" own" + std::wstring{ k_separator }
                + std::to_wstring(inherited) + L" inherited";
        }

        // A member table over the type and its bases, its count telling the own rows apart.
        template<typename GroupOf>
        void addMemberSection(Page& page, std::wstring heading, const TableColumns& columns,
            const Type& type, const ReachedBases& bases, const std::size_t own,
            const GroupOf& groupOf)
        {
            TableGroups groups = ownAndInherited(type, bases, groupOf);
            if (groups.empty())
                return;
            Section& section = addTableSection(page, std::move(heading), columns,
                std::move(groups));
            section.count = memberCount(own, section.table.rowCount() - own);
        }

        void addProperties(Page& page, const Surface& surface, Notes& notes, const Type& type,
            const ReachedBases& bases)
        {
            addMemberSection(page, L"Properties", Columns::properties, type, bases,
                type.properties.size(), [&](const Type& owner) {
                    return TableGroup{ .rows = propertyRows(surface, notes, owner) };
                });
        }

        void addEvents(Page& page, const Surface& surface, Notes& notes, const Type& type,
            const ReachedBases& bases)
        {
            addMemberSection(page, L"Events", Columns::events, type, bases, type.events.size(),
                [&](const Type& owner) {
                    return TableGroup{ .rows = eventRows(surface, notes, owner) };
                });
        }

        void addMethods(Page& page, Notes& notes, const Type& type, const ReachedBases& bases)
        {
            addMemberSection(page, L"Methods", Columns::functions, type, bases,
                type.functions.size(), [&](const Type& owner) {
                    return methodGroup(notes, owner);
                });
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
                        hintRuns(field.comment) },
                    .excerpt = memberExcerpt(notes, field.comment, field.name, type.category)
                });
            }
            TableGroups groups;
            groups.push_back(std::move(group));
            addTableSection(page, L"Fields", Columns::fields, std::move(groups));
        }

        void addMembers(Page& page, Notes& notes, const Type& type)
        {
            if (type.members.empty())
                return;
            TableGroup group;
            for (const EnumMember& member : type.members)
            {
                group.rows.push_back({
                    .cells = { { code(member.name) }, valueCell(member.value),
                        hintRuns(member.comment) },
                    .excerpt = memberExcerpt(notes, member.comment, member.name, type.category)
                });
            }
            TableGroups groups;
            groups.push_back(std::move(group));
            addTableSection(page, L"Members", Columns::members, std::move(groups));
        }

        // The tables of types, functions and constants

        [[nodiscard]] TableRows typeRows(const TypeList& types)
        {
            TableRows rows;
            for (const Type* type : types)
            {
                rows.push_back({
                    .cells = { { nameRun(*type) }, kindCell(*type), hintRuns(type->comment) },
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
            groups.push_back({ .rows = typeRows(siblings) });
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
                        hintRuns(free.function.comment) } });
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
                        valueCell(variable.field.value), hintRuns(variable.field.comment) } });
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

    Page chapterPage(const Surface& surface, Notes&, const ContentsChapter& chapter)
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

        TableGroups groups;
        for (const ContentsModule& module : chapter.modules)
        {
            groups.push_back({
                .label = { { .text = module.shortName, .link = module.name } },
                .hint = { plain(module.name) },
                .rows = typeRows(module.types)
            });
        }
        Section& section = addTypesSection(page, L"Types", std::move(groups));
        section.table.groupColumn = L"Module";
        section.count = std::to_wstring(types) + L" in "
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
        addFact(page, L"chapter", { plain(chapter.name) });
        if (!module.types.empty())
        {
            TableGroups groups;
            groups.push_back({ .rows = typeRows(module.types) });
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

    Page typePage(const Surface& surface, Notes& notes, const Type& type)
    {
        Page page;
        addTypeHead(page, surface, type);
        const ReachedBases bases = addTypeBases(page, surface, type);
        addDerivedTypes(page, surface, type);
        addTypeNote(page, notes, type);
        addProperties(page, surface, notes, type, bases);
        addEvents(page, surface, notes, type, bases);
        addMethods(page, notes, type, bases);
        addFields(page, surface, notes, type);
        addMembers(page, notes, type);
        addSiblings(page, surface, type);
        return page;
    }
}
