module SeeDocs_App.Search;

import SeeDocs_App.Database;
import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Core.TextEngine.Searcher;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        constexpr std::wstring_view k_scope = L"::";
        constexpr wchar_t k_space = L' ';
        constexpr wchar_t k_folderSeparator = L'/';
        constexpr wchar_t k_anchorMark = L'#';
        constexpr std::wstring_view k_destructorMark = L"~";
        constexpr std::wstring_view k_codeFence = L"    ";
        constexpr std::wstring_view k_bulletMark = L"- ";
        constexpr std::wstring_view k_sentenceEnd = L". ";
        constexpr std::wstring_view k_noteJoin = L" › ";
        constexpr std::wstring_view k_searchWord = L"search";
        constexpr std::wstring_view k_controlWord = L"control";
        constexpr std::wstring_view k_headerWord = L"header";
        constexpr std::size_t k_sectionRows = 200;   // what a section of the page lists at most
        constexpr std::size_t k_maxBaseDepth = 16;   // what a base chain is followed to

        // Where a word of a name starts: after _, :, . or /, at a capital after a lower-case
        // letter or a digit, and at the last capital of a run before a lower-case letter - the P
        // of HTMLParser.
        [[nodiscard]] bool isWordStart(const std::wstring_view name, const std::size_t at)
        {
            if (at == 0)
                return true;
            const wchar_t previous = name[at - 1];
            if (previous == L'_' || previous == L':' || previous == L'.'
                || previous == k_folderSeparator)
            {
                return true;
            }
            const wchar_t current = name[at];
            if (!std::iswupper(current))
                return false;
            if (std::iswlower(previous) || std::iswdigit(previous))
                return true;
            return std::iswupper(previous) && at + 1 < name.size() && std::iswlower(name[at + 1]);
        }

        // How a name holds the words, and where.
        struct Match
        {
            MatchRank rank{ MatchRank::Inside };
            std::size_t start{ 0 };
            std::size_t length{ 0 };
        };

        // The best of the places the searcher finds in the name, read into the caller's vector.
        [[nodiscard]] std::optional<Match> matchName(const std::wstring_view name,
            const Searcher& searcher, std::vector<TextRange>& places)
        {
            searcher.findIn(name, places);
            if (places.empty())
                return std::nullopt;
            const TextRange& first = places.front();
            if (first.start == 0)
            {
                const MatchRank rank = first.length == name.size()
                    ? MatchRank::Exact
                    : MatchRank::Start;
                return Match{ rank, 0, first.length };
            }
            for (const TextRange& place : places)
            {
                if (isWordStart(name, place.start))
                    return Match{ MatchRank::WordStart, place.start, place.length };
            }
            return Match{ MatchRank::Inside, first.start, first.length };
        }

        // A query of two parts: the owner it names and the name, cut at its last ::, else at its
        // last space.
        struct QueryParts
        {
            std::wstring owner;
            std::wstring name;
        };

        [[nodiscard]] std::optional<QueryParts> partsOf(const std::wstring_view query)
        {
            std::size_t cut = query.rfind(k_scope);
            std::size_t width = k_scope.size();
            if (cut == std::wstring_view::npos)
            {
                cut = query.rfind(k_space);
                width = 1;
            }
            if (cut == std::wstring_view::npos)
                return std::nullopt;
            const std::wstring_view owner = trimmed(query.substr(0, cut));
            const std::wstring_view name = trimmed(query.substr(cut + width));
            if (owner.empty() || name.empty())
                return std::nullopt;
            return QueryParts{ std::wstring{ owner }, std::wstring{ name } };
        }

        // What a qualified name stands in: everything before its last ::.
        [[nodiscard]] std::wstring_view scopeOf(const std::wstring_view qualified)
        {
            const std::size_t cut = qualified.rfind(k_scope);
            if (cut == std::wstring_view::npos)
                return {};
            return qualified.substr(0, cut);
        }

        [[nodiscard]] std::wstring_view parentOf(const std::wstring_view path)
        {
            const std::size_t cut = path.rfind(k_folderSeparator);
            if (cut == std::wstring_view::npos)
                return {};
            return path.substr(0, cut);
        }

        [[nodiscard]] std::wstring citeKey(const std::wstring_view note,
            const std::wstring_view anchor)
        {
            std::wstring result{ note };
            result += k_anchorMark;
            result += anchor;
            return result;
        }

        // The type's bases up the chain as far as the surface reads it, each once; a base that is
        // one of the type's own template parameters names none.
        void addBases(const Surface& surface, const Type& type,
            std::unordered_set<const Type*>& bases, const std::size_t depth)
        {
            if (depth == k_maxBaseDepth)
                return;
            for (const std::wstring& spelled : type.bases)
            {
                if (isTemplateParameter(type, spelled))
                    continue;
                const Type* base = surface.resolve(withoutArguments(spelled), type.nameSpace);
                if (!base || !bases.insert(base).second)
                    continue;
                addBases(surface, *base, bases, depth + 1);
            }
        }

        // Runs

        [[nodiscard]] Run plain(std::wstring text)
        {
            return {
                .text = std::move(text),
                .style = RunStyle::Plain,
                .link = {}
            };
        }

        [[nodiscard]] Run muted(std::wstring text)
        {
            return {
                .text = std::move(text),
                .style = RunStyle::Muted,
                .link = {}
            };
        }

        void respell(Runs& runs, const std::size_t project, const LinkSpeller& spell)
        {
            for (Run& run : runs)
            {
                if (!run.link.empty())
                    run.link = spell(project, run.link);
            }
        }

        // Where the words stand in a text as a reader sees it, its code and bold marks taken out:
        // the start and the length in the text as written.
        struct WrittenPlace
        {
            std::size_t start{ 0 };
            std::size_t length{ 0 };
        };

        [[nodiscard]] std::optional<WrittenPlace> findRead(const std::wstring_view written,
            const Searcher& searcher)
        {
            if (searcher.empty())
                return std::nullopt;
            std::wstring read;
            std::vector<std::size_t> places;   // where each character read stands as written
            for (std::size_t i = 0; i < written.size(); ++i)
            {
                if (written[i] == k_codeMark)
                    continue;
                if (written.substr(i).starts_with(k_boldMark))
                {
                    i += k_boldMark.size() - 1;
                    continue;
                }
                read += written[i];
                places.push_back(i);
            }
            std::vector<TextRange> found;
            searcher.findIn(read, found);
            if (found.empty())
                return std::nullopt;
            const std::size_t start = places[found.front().start];
            const std::size_t end = places[found.front().end() - 1] + 1;
            return WrittenPlace{ start, end - start };
        }

        // **A section's own words stop at the first heading under it**, which heads a section of
        // its own: its lines as paragraphs, run on up to an empty line, a bullet and a line of code
        // each standing alone.
        [[nodiscard]] std::vector<std::wstring> paragraphsOf(const Lines& lines)
        {
            std::vector<std::wstring> result;
            std::wstring open;
            const auto close = [&]() {
                if (!open.empty())
                    result.push_back(std::exchange(open, {}));
            };
            for (const std::wstring& line : lines)
            {
                const std::wstring_view words = trimmed(line);
                if (words.empty())
                {
                    close();
                    continue;
                }
                const bool code = std::wstring_view{ line }.starts_with(k_codeFence);
                if (!code && headingLevel(words) != 0)
                    break;
                if (code)
                {
                    close();
                    result.push_back(std::wstring{ words });
                    continue;
                }
                if (words.starts_with(k_bulletMark))
                {
                    close();
                    open = words.substr(k_bulletMark.size());
                    continue;
                }
                if (!open.empty())
                    open += k_space;
                open += words;
            }
            close();
            return result;
        }

        // The sentence of the paragraph the words stand in, from the end of the one before it to
        // its own full stop.
        [[nodiscard]] std::wstring sentenceAt(const std::wstring& paragraph,
            const std::size_t at, const std::size_t length)
        {
            const std::wstring_view words = paragraph;
            std::size_t start = 0;
            const std::size_t before = words.substr(0, at).rfind(k_sentenceEnd);
            if (before != std::wstring_view::npos)
                start = before + k_sentenceEnd.size();
            std::size_t end = words.find(k_sentenceEnd, at + length);
            end = end == std::wstring_view::npos ? words.size() : end + 1;
            return std::wstring{ trimmed(words.substr(start, end - start)) };
        }

        // The first sentence holding the words, or the first sentence of the section where only
        // its heading does; empty for a section with no words.
        [[nodiscard]] std::optional<std::wstring> wordsOf(const NoteSection& section,
            const Searcher& searcher, const bool headingHolds)
        {
            const std::vector<std::wstring> paragraphs = paragraphsOf(section.lines);
            for (const std::wstring& paragraph : paragraphs)
            {
                if (const std::optional<WrittenPlace> place = findRead(paragraph, searcher))
                    return sentenceAt(paragraph, place->start, place->length);
            }
            if (!headingHolds)
                return std::nullopt;
            if (paragraphs.empty())
                return std::wstring{};
            return sentenceAt(paragraphs.front(), 0, 0);
        }

        // The page's sections

        // One section of the page as it is gathered: its heading, its columns, and a group of rows
        // per project, each counted whole though it keeps only the first rows.
        struct Gathered
        {
            std::wstring heading;
            TableColumns columns;
            std::vector<TableGroup> groups;
            std::vector<std::size_t> counts;   // how many each project has, kept or not
        };

        [[nodiscard]] TableColumn column(std::wstring name, const bool fills = false)
        {
            return { .name = std::move(name), .fills = fills, .hint = false, .maxLines = 0 };
        }

        [[nodiscard]] Gathered gathered(std::wstring heading, TableColumns columns,
            const std::size_t projects)
        {
            return {
                .heading = std::move(heading),
                .columns = std::move(columns),
                .groups = std::vector<TableGroup>(projects),
                .counts = std::vector<std::size_t>(projects, 0)
            };
        }

        // Where a name of each kind is listed on the page, by the kind's place in NameKind.
        constexpr auto k_nameHeadings = std::to_array<std::wstring_view>({
            L"Types",
            L"Folders",
            L"Modules",
            L"Functions",
            L"Constants",
            L"Properties",
            L"Events",
            L"Methods",
            L"Fields",
            L"Members"
        });

        [[nodiscard]] TableColumns columnsOf(const NameKind kind)
        {
            switch (kind)
            {
                case NameKind::Type:
                    return { column(L"Name"), column(L"Kind"), column(L"Module"),
                        column(L"Hint", true) };
                case NameKind::Folder:
                    return { column(L"Folder"), column(L"Path", true) };
                case NameKind::Module:
                    return { column(L"Module"), column(L"Folder", true) };
                case NameKind::Function:
                case NameKind::Constant:
                    return { column(L"Name"), column(L"Module"), column(L"Hint", true) };
                case NameKind::Property:
                case NameKind::Event:
                case NameKind::Method:
                case NameKind::Field:
                case NameKind::Member:
                    break;
            }
            return { column(L"Name"), column(L"Hint", true) };
        }

        // A name's first cell: a member's type before it, muted, and the name as code, linked.
        [[nodiscard]] Runs nameCell(const NamedEntry& entry)
        {
            Runs runs;
            if (entry.type && entry.kind != NameKind::Type)
                runs.push_back(muted(entry.type->name + std::wstring{ k_scope }));
            runs.push_back({
                .text = entry.name,
                .style = RunStyle::Code,
                .link = entry.link
            });
            return runs;
        }

        [[nodiscard]] Runs hintCell(const Surface& surface, const NamedEntry& entry)
        {
            if (!entry.comment)
                return {};
            return hintRunsOf(surface, *entry.comment, entry.nameSpace);
        }

        [[nodiscard]] TableRow nameRow(const Surface& surface, const NameHit& hit)
        {
            const NamedEntry& entry = *hit.entry;
            TableRow row;
            row.cells.push_back(nameCell(entry));
            switch (entry.kind)
            {
                case NameKind::Type:
                    row.type = entry.type;
                    row.cells.push_back({ muted(std::wstring{ entry.kindWord }) });
                    row.cells.push_back({ Run{
                        .text = entry.module->shortName,
                        .style = RunStyle::Plain,
                        .link = entry.module->name
                    } });
                    row.cells.push_back(hintCell(surface, entry));
                    break;
                case NameKind::Folder:
                    row.cells.push_back({ muted(entry.folder) });
                    break;
                case NameKind::Module:
                    row.cells.push_back({ muted(entry.folder) });
                    break;
                case NameKind::Function:
                case NameKind::Constant:
                    row.cells.push_back({ Run{
                        .text = entry.module->shortName,
                        .style = RunStyle::Plain,
                        .link = entry.module->name
                    } });
                    row.cells.push_back(hintCell(surface, entry));
                    break;
                case NameKind::Property:
                case NameKind::Event:
                case NameKind::Method:
                case NameKind::Field:
                case NameKind::Member:
                    row.cells.push_back(hintCell(surface, entry));
                    break;
            }
            return row;
        }

        // Keeps the row where the group has room, and counts it either way.
        void keep(Gathered& into, const std::size_t project, TableRow row,
            const LinkSpeller& spell)
        {
            ++into.counts[project];
            TableRows& rows = into.groups[project].rows;
            if (rows.size() == k_sectionRows)
                return;
            for (Runs& cell : row.cells)
                respell(cell, project, spell);
            rows.push_back(std::move(row));
        }

        // The names of the project holding the words, each in its kind's section, best first.
        void gatherNames(std::vector<Gathered>& sections, const SearchedProject& project,
            const std::size_t index, const NameHits& hits, const LinkSpeller& spell)
        {
            for (const NameHit& hit : hits)
            {
                const std::size_t kind = static_cast<std::size_t>(hit.entry->kind);
                keep(sections[kind], index, nameRow(*project.surface, hit), spell);
            }
        }

        // The declarations whose hints hold the words and whose names do not, in the tree's order.
        void gatherHints(Gathered& section, const SearchedProject& project,
            const std::size_t index, const NameHits& hits, const Searcher& searcher,
            const LinkSpeller& spell)
        {
            std::unordered_set<const NamedEntry*> named;
            for (const NameHit& hit : hits)
                named.insert(hit.entry);
            for (const NamedEntry& entry : project.index->entries())
            {
                if (!entry.comment || named.contains(&entry))
                    continue;
                if (!findRead(entry.comment->text, searcher))
                    continue;
                TableRow row;
                if (entry.kind == NameKind::Type)
                    row.type = entry.type;
                row.cells.push_back(nameCell(entry));
                row.cells.push_back(hintCell(*project.surface, entry));
                keep(section, index, std::move(row), spell);
            }
        }

        // The footnote sections holding the words, in the folder's order: where each stands, linked
        // to the declaration it belongs to, and the sentence the words stand in.
        void gatherFootnotes(Gathered& section, const SearchedProject& project,
            const std::size_t index, const Searcher& searcher, const LinkSpeller& spell)
        {
            Notes& notes = *project.notes;
            for (const std::wstring& stem : notes.stems())
            {
                const Note* note = notes.note(stem);
                if (!note)
                    continue;
                for (const NoteSection& noteSection : note->sections)
                {
                    const bool headingHolds = findRead(noteSection.heading, searcher).has_value();
                    const std::optional<std::wstring> words = wordsOf(noteSection, searcher,
                        headingHolds);
                    if (!words.has_value())
                        continue;
                    Runs where = { muted(note->stem + std::wstring{ k_noteJoin }), Run{
                        .text = noteSection.heading,
                        .style = RunStyle::Plain,
                        .link = project.index->ownerOf(note->stem, noteSection)
                    } };
                    TableRow row;
                    row.cells.push_back(std::move(where));
                    row.cells.push_back(noteRunsOf(*project.surface, *words, {}));
                    keep(section, index, std::move(row), spell);
                }
            }
        }

        [[nodiscard]] std::wstring countWords(const std::size_t kept, const std::size_t total)
        {
            const std::wstring elements = std::to_wstring(total)
                + (total == 1 ? L" element" : L" elements");
            if (kept == total)
                return elements;
            return std::to_wstring(kept) + L" of " + elements;
        }

        // A section with rows, a group per project with any where there are several, each
        // labelled with its project's name.
        void addGathered(Page& page, Gathered& gatheredSection,
            const SearchedProjects& projects)
        {
            std::size_t kept = 0;
            std::size_t total = 0;
            Section section;
            section.kind = SectionKind::Table;
            section.heading = { plain(gatheredSection.heading) };
            section.table.columns = std::move(gatheredSection.columns);
            section.table.header = false;
            for (std::size_t i = 0; i != gatheredSection.groups.size(); ++i)
            {
                TableGroup& group = gatheredSection.groups[i];
                if (group.rows.empty())
                    continue;
                kept += group.rows.size();
                total += gatheredSection.counts[i];
                if (projects.size() > 1)
                    group.label = { plain(std::wstring{ projects[i].name }) };
                section.table.groups.push_back(std::move(group));
            }
            if (kept == 0)
                return;
            section.hint = countWords(kept, total);
            page.sections.push_back(std::move(section));
        }

        [[nodiscard]] std::wstring placeWords(const SearchedProjects& projects)
        {
            if (projects.size() == 1)
                return std::wstring{ projects.front().name };
            return L"the projects open";
        }
    }

    // SearchIndex

    // **The tree's own order**: the folders as the tree lists them, each folder's modules, each
    // module's types with their members, then its functions and its constants.
    SearchIndex::SearchIndex(const Surface& surface, const ContentsFolders& folders)
        :
        m_surface{ &surface }
    {
        addFolders(folders);
        for (const ContentsFolder& folder : folders)
        {
            for (const ContentsModule& module : folder.modules)
                addModule(surface, folder, module);
        }
    }

    NameHits SearchIndex::find(const std::wstring_view query, const std::size_t limit) const
    {
        const std::wstring_view typed = trimmed(query);
        Searcher whole;
        whole.setText(typed, SearchSplit::Phrase);
        if (whole.empty())
            return {};
        const std::optional<QueryParts> parts = partsOf(typed);
        Searcher ownerPart;
        Searcher namePart;
        // **A member a type's page lists is the type's to find**: one of a base the first part
        // names exactly stands after the type's own.
        std::unordered_set<const Type*> under;
        if (parts.has_value())
        {
            ownerPart.setText(parts->owner, SearchSplit::Phrase);
            namePart.setText(parts->name, SearchSplit::Phrase);
            under = typesUnder(ownerPart);
        }

        // A hit, how well its owner holds a two-part query's first part, and where it stands.
        struct Ranked
        {
            NameHit hit;
            MatchRank ownerRank{ MatchRank::Exact };
            std::size_t index{ 0 };
        };

        std::vector<TextRange> places;
        std::vector<Ranked> ranked;
        for (std::size_t i = 0; i != m_entries.size(); ++i)
        {
            const NamedEntry& entry = m_entries[i];
            std::optional<Match> best;
            std::size_t bestSpelling = 0;
            const Searcher* matched = &whole;
            MatchRank ownerRank = MatchRank::Exact;
            const std::size_t spellings = entry.spellings.size() + 1;   // the shown one first
            const auto spelledAt = [&entry](const std::size_t at) -> std::wstring_view {
                return at == 0 ? std::wstring_view{ entry.name } : entry.spellings[at - 1];
            };
            for (std::size_t at = 0; at != spellings; ++at)
            {
                const std::optional<Match> match = matchName(spelledAt(at), whole, places);
                if (match && (!best || match->rank < best->rank))
                {
                    best = match;
                    bestSpelling = at;
                }
            }
            if (parts.has_value())
            {
                std::optional<Match> owner = matchName(entry.owner, ownerPart, places);
                if (!owner && entry.kind != NameKind::Type && under.contains(entry.type))
                    owner = Match{ MatchRank::Inside, 0, 0 };
                for (std::size_t at = 0; owner && at != spellings; ++at)
                {
                    const std::optional<Match> match = matchName(spelledAt(at), namePart,
                        places);
                    const bool better = match
                        && (!best || match->rank < best->rank
                            || (match->rank == best->rank && owner->rank < ownerRank));
                    if (!better)
                        continue;
                    best = match;
                    bestSpelling = at;
                    matched = &namePart;
                    ownerRank = owner->rank;
                }
            }
            if (!best)
                continue;

            NameHit hit{
                .entry = &entry,
                .rank = best->rank,
                .start = best->start,
                .length = best->length
            };
            if (bestSpelling != 0)
            {
                matched->findIn(entry.name, places);
                hit.start.reset();
                if (!places.empty())
                    hit.start = places.front().start;
            }
            ranked.push_back({ .hit = hit, .ownerRank = ownerRank, .index = i });
        }

        // **Best first**: by how the name holds the words, then how its owner holds a two-part
        // query's first part, types before folders and modules before the rest, the shorter name,
        // and the tree's own order.
        std::ranges::sort(ranked, [](const Ranked& left, const Ranked& right) {
            const auto key = [](const Ranked& value) {
                return std::tuple{
                    value.hit.rank,
                    value.ownerRank,
                    value.hit.entry->kind,
                    value.hit.entry->name.size(),
                    value.index
                };
            };
            return key(left) < key(right);
        });

        NameHits result;
        result.reserve(std::min(limit, ranked.size()));
        for (const Ranked& value : ranked)
        {
            if (result.size() == limit)
                break;
            result.push_back(value.hit);
        }
        return result;
    }

    std::wstring SearchIndex::ownerOf(const std::wstring_view note,
        const NoteSection& section) const
    {
        const SectionOwners::const_iterator named = m_named.find(section.heading);
        if (named != m_named.end())
            return named->second;
        const SectionOwners::const_iterator cited = m_cited.find(citeKey(note, section.anchor));
        if (cited != m_cited.end())
            return cited->second;
        return {};
    }

    // Every folder on the way to one with modules is a folder, whether or not it holds modules of
    // its own, each once, in the order the folders read; the root is the project's home and no
    // name.
    void SearchIndex::addFolders(const ContentsFolders& folders)
    {
        std::unordered_set<std::wstring> added;
        for (const ContentsFolder& folder : folders)
        {
            std::wstring way;
            std::wstring_view rest = folder.path;
            while (!rest.empty())
            {
                const std::size_t slash = rest.find(k_folderSeparator);
                const std::wstring_view name = rest.substr(0, slash);
                if (!way.empty())
                    way += k_folderSeparator;
                way += name;
                rest = slash == std::wstring_view::npos
                    ? std::wstring_view{}
                    : rest.substr(slash + 1);
                if (!added.insert(way).second)
                    continue;
                add({
                    .kind = NameKind::Folder,
                    .kindWord = L"folder",
                    .name = std::wstring{ name },
                    .spellings = { way },
                    .owner = std::wstring{ parentOf(way) },
                    .nameSpace = {},
                    .type = nullptr,
                    .module = nullptr,
                    .folder = way,
                    .comment = nullptr,
                    .link = way
                });
            }
        }
    }

    // A function is one name however many overloads it has, its hint the first commented one's.
    void SearchIndex::addModule(const Surface& surface, const ContentsFolder& folder,
        const ContentsModule& module)
    {
        add({
            .kind = NameKind::Module,
            .kindWord = module.header ? k_headerWord : std::wstring_view{ L"module" },
            .name = module.shortName,
            .spellings = { module.name },
            .owner = folder.path,
            .nameSpace = {},
            .type = nullptr,
            .module = &module,
            .folder = folder.path,
            .comment = nullptr,
            .link = module.name
        });
        for (const Type* type : module.types)
            addType(surface, folder, module, *type);

        std::unordered_map<std::wstring_view, std::size_t> functions;
        for (const FreeFunction* free : module.functions)
        {
            const Function& function = free->function;
            const auto [found, made] = functions.try_emplace(function.name, m_entries.size());
            if (!made)
            {
                NamedEntry& entry = m_entries[found->second];
                if (entry.comment && entry.comment->text.empty() && !function.comment.text.empty())
                    entry.comment = &function.comment;
                continue;
            }
            add({
                .kind = NameKind::Function,
                .kindWord = L"function",
                .name = function.name,
                .spellings = {},
                .owner = free->nameSpace,
                .nameSpace = free->nameSpace,
                .type = nullptr,
                .module = &module,
                .folder = folder.path,
                .comment = &function.comment,
                .link = exportLink(module, function.name)
            });
        }
        for (const Variable* variable : module.constants)
        {
            add({
                .kind = NameKind::Constant,
                .kindWord = variable->field.isConstant ? std::wstring_view{ L"constant" }
                    : std::wstring_view{ L"variable" },
                .name = variable->field.name,
                .spellings = {},
                .owner = variable->nameSpace,
                .nameSpace = variable->nameSpace,
                .type = nullptr,
                .module = &module,
                .folder = folder.path,
                .comment = &variable->field.comment,
                .link = exportLink(module, variable->field.name)
            });
        }
    }

    // **A type's members are the ones its page lists**: a private one is no member of the surface,
    // a constructor and a destructor are the type's own, a property's getter and setter are the
    // property, and an override with no words of its own is its base's method - the base's row
    // stands for it.
    void SearchIndex::addType(const Surface& surface, const ContentsFolder& folder,
        const ContentsModule& module, const Type& type)
    {
        const auto member = [&](const NameKind kind, const std::wstring_view word,
            const std::wstring& name, const Comment& comment) {
            return NamedEntry{
                .kind = kind,
                .kindWord = word,
                .name = name,
                .spellings = {},
                .owner = type.qualifiedName,
                .nameSpace = type.nameSpace,
                .type = &type,
                .module = &module,
                .folder = folder.path,
                .comment = &comment,
                .link = memberLink(type, name)
            };
        };

        add({
            .kind = NameKind::Type,
            .kindWord = type.isControl ? k_controlWord : kindWord(type.kind),
            .name = type.name,
            .spellings = { type.qualifiedName },
            .owner = std::wstring{ scopeOf(type.qualifiedName) },
            .nameSpace = type.nameSpace,
            .type = &type,
            .module = &module,
            .folder = folder.path,
            .comment = &type.comment,
            .link = type.qualifiedName
        });

        for (const Property& property : type.properties)
        {
            if (property.access != Access::Private)
                add(member(NameKind::Property, L"property", property.name, property.comment));
        }
        for (const Event& event : type.events)
        {
            if (event.access == Access::Private)
                continue;
            NamedEntry entry = member(NameKind::Event, L"event", event.alias, event.comment);
            entry.spellings = { event.type, event.method };
            add(std::move(entry));
        }

        std::unordered_set<const Type*> bases{ &type };
        addBases(surface, type, bases, 0);
        std::unordered_set<std::wstring> inherited;
        for (const Type* base : bases)
        {
            if (base == &type)
                continue;
            for (const Function& function : base->functions)
                inherited.insert(function.name);
        }
        const std::wstring_view bare = bareName(type.name);
        for (const std::wstring_view name : methodNames(type))
        {
            if (name == bare || name.starts_with(k_destructorMark))
                continue;
            const Comment* comment = nullptr;
            bool reached = false;
            for (const Function& function : type.functions)
            {
                if (function.name != name || function.access == Access::Private)
                    continue;
                reached = true;
                if (!comment || (comment->text.empty() && !function.comment.text.empty()))
                    comment = &function.comment;
            }
            if (!reached)
                continue;
            if (comment->text.empty() && inherited.contains(std::wstring{ name }))
                continue;
            add(member(NameKind::Method, L"method", std::wstring{ name }, *comment));
        }

        for (const Field& field : type.fields)
        {
            if (field.access != Access::Private)
                add(member(NameKind::Field, L"field", field.name, field.comment));
        }
        for (const EnumMember& enumMember : type.members)
            add(member(NameKind::Member, L"member", enumMember.name, enumMember.comment));
    }

    void SearchIndex::add(NamedEntry entry)
    {
        addOwner(entry);
        m_entries.push_back(std::move(entry));
    }

    // A section a See reaches is the first such declaration's; one found by name is the
    // declaration's its heading spells - an event's by its struct, as a page finds it.
    void SearchIndex::addOwner(const NamedEntry& entry)
    {
        if (!entry.comment)
            return;
        const bool event = entry.kind == NameKind::Event;
        const std::wstring_view citedName = event ? std::wstring_view{ entry.spellings.front() }
            : std::wstring_view{ entry.name };
        if (const std::optional<Reference>& reference = entry.comment->reference)
        {
            const std::wstring anchor = reference->anchor.empty()
                ? slug(citedName)
                : reference->anchor;
            m_cited.try_emplace(citeKey(reference->note, anchor), entry.link);
        }
        const bool isMember = entry.type && entry.kind != NameKind::Type;
        const std::wstring heading = isMember
            ? footnoteHeading(entry.type->name, citedName)
            : std::wstring{ citedName };
        m_named.try_emplace(heading, entry.link);
    }

    std::unordered_set<const Type*> SearchIndex::typesUnder(const Searcher& owner) const
    {
        std::unordered_set<const Type*> result;
        std::vector<TextRange> places;
        for (const NamedEntry& entry : m_entries)
        {
            if (entry.kind != NameKind::Type)
                continue;
            const std::optional<Match> match = matchName(entry.name, owner, places);
            if (!match || match->rank != MatchRank::Exact)
                continue;
            result.insert(entry.type);
            addBases(*m_surface, *entry.type, result, 0);
        }
        return result;
    }

    Page searchPage(const SearchedProjects& projects, const std::wstring_view query,
        const LinkSpeller& spell)
    {
        const std::wstring_view typed = trimmed(query);
        Searcher whole;
        whole.setText(typed, SearchSplit::Phrase);
        const std::size_t count = projects.size();

        std::vector<Gathered> sections;
        for (std::size_t kind = 0; kind != k_nameHeadings.size(); ++kind)
        {
            sections.push_back(gathered(std::wstring{ k_nameHeadings[kind] },
                columnsOf(static_cast<NameKind>(kind)), count));
        }
        Gathered hints = gathered(L"In hints", { column(L"Name"), column(L"Hint", true) }, count);
        Gathered footnotes = gathered(L"In footnotes",
            { column(L"Section"), column(L"Words", true) }, count);
        for (std::size_t i = 0; i != count && !whole.empty(); ++i)
        {
            const SearchedProject& project = projects[i];
            const NameHits hits = project.index->find(typed);
            gatherNames(sections, project, i, hits, spell);
            gatherHints(hints, project, i, hits, whole, spell);
            gatherFootnotes(footnotes, project, i, whole, spell);
        }

        Page page;
        page.kind = PageKind::Search;
        page.title = std::wstring{ typed };
        // A word a part: a query cut at :: reads as its two parts, as a query cut at a space does.
        const std::optional<QueryParts> parts = partsOf(typed);
        page.searched = parts.has_value()
            ? parts->owner + L' ' + parts->name
            : std::wstring{ typed };
        page.badges = { muted(std::wstring{ k_searchWord }) };
        for (Gathered& section : sections)
            addGathered(page, section, projects);
        addGathered(page, hints, projects);
        addGathered(page, footnotes, projects);
        const std::wstring place = placeWords(projects);
        std::wstring lead = page.sections.empty()
            ? L"Nothing in " + place + L" holds the words."
            : L"The names, hints and footnotes of " + place + L" that hold the words.";
        page.lead = { plain(std::move(lead)) };
        return page;
    }
}
