module SeeDocs_App.Pages;

import SeeDocs_App.Database;
import SeeDocs_App.Notes;
import SeeDocs_App.Surface;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    namespace
    {
        // The folders under Source in the order a reader meets them; any other follows by name.
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
        constexpr std::wstring_view k_sourceRoot = L"Source/";
        constexpr std::wstring_view k_noHint = L"no hint";
        constexpr std::wstring_view k_matchedByName =
            L"matched by name - the comment carries no See";
        constexpr std::wstring_view k_controlWord = L"control";
        constexpr std::wstring_view k_noteExtension = L".md";
        constexpr std::wstring_view k_codeFence = L"    ";
        constexpr std::wstring_view k_bulletMark = L"- ";
        constexpr std::wstring_view k_boldMark = L"**";
        constexpr wchar_t k_codeMark = L'`';

        // The groups whose tables share their columns on one page.
        namespace Groups
        {
            constexpr std::size_t properties = 1;
            constexpr std::size_t events = 2;
            constexpr std::size_t methods = 3;
            constexpr std::size_t fields = 4;
            constexpr std::size_t members = 5;
            constexpr std::size_t overview = 6;
            constexpr std::size_t functions = 7;
            constexpr std::size_t constants = 8;
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

        // Blocks

        Block& addBlock(Page& page, const BlockKind kind, const std::size_t level = 0)
        {
            page.blocks.push_back({ .kind = kind, .level = level });
            return page.blocks.back();
        }

        void addTitle(Page& page, std::wstring text)
        {
            page.title = text;
            addBlock(page, BlockKind::Title).runs = { plain(std::move(text)) };
        }

        void addLead(Page& page, const Comment& comment)
        {
            addBlock(page, BlockKind::Lead).runs = hintRuns(comment);
        }

        void addMeta(Page& page, Runs runs)
        {
            addBlock(page, BlockKind::Meta).runs = std::move(runs);
        }

        void addHeading(Page& page, std::wstring text)
        {
            addBlock(page, BlockKind::Heading).runs = { plain(std::move(text)) };
        }

        void addSubHeading(Page& page, Runs runs)
        {
            addBlock(page, BlockKind::SubHeading).runs = std::move(runs);
        }

        void addParagraph(Page& page, Runs runs, const std::size_t level = 0)
        {
            addBlock(page, BlockKind::Paragraph, level).runs = std::move(runs);
        }

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
        void addNoteLines(Page& page, const Lines& lines, const std::size_t level)
        {
            std::wstring paragraph;
            const auto flush = [&]() {
                if (!paragraph.empty())
                    addParagraph(page, inlineRuns(std::exchange(paragraph, {})), level);
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
                    Block& block = addBlock(page, BlockKind::SubHeading, level);
                    block.runs = inlineRuns(std::wstring{ trimmed(words.substr(heading)) });
                    ++i;
                    continue;
                }
                if (isCodeLine(line) && paragraph.empty())
                {
                    flush();
                    Block& block = addBlock(page, BlockKind::Code, level);
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
                    continue;
                }
                if (isBulletLine(line))
                {
                    flush();
                    Block& block = addBlock(page, BlockKind::Bullets, level);
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
                    continue;
                }
                if (!paragraph.empty())
                    paragraph += L' ';
                paragraph += words;
                ++i;
            }
            flush();
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

        void addNote(Page& page, const FoundNote& found, const std::size_t level)
        {
            if (!found.section)
                return;
            Block& source = addBlock(page, BlockKind::Source, level);
            source.runs.push_back(muted(found.note->stem + std::wstring{ k_noteExtension }
                + L" \u203A " + found.section->heading));
            if (found.byName)
                source.runs.push_back(missing(L"  " + std::wstring{ k_matchedByName }));
            addNoteLines(page, found.section->lines, level);
        }

        // Rows and entries of one group, which share their columns. A row joins the table the
        // page ends with when that is one of the group; anything written between two rows - a
        // note, a subheading - starts a new table. An entry is a block of its own.
        class TableWriter
        {
        public:
            TableWriter(Page&, std::size_t group);
            void row(Cells);
            void entry(Cells term, Runs hint);
            void note(Notes&, const Comment&, std::wstring_view name, std::wstring_view category);
        private:
            Page& m_page;
            std::size_t m_group;
        };

        TableWriter::TableWriter(Page& page, const std::size_t group)
            :
            m_page{ page },
            m_group{ group }
        {
        }

        void TableWriter::row(Cells cells)
        {
            Blocks& blocks = m_page.blocks;
            const bool open = !blocks.empty()
                && blocks.back().kind == BlockKind::Table
                && blocks.back().group == m_group
                && blocks.back().level == 0;
            if (!open)
                addBlock(m_page, BlockKind::Table).group = m_group;
            blocks.back().rows.push_back(std::move(cells));
        }

        void TableWriter::entry(Cells term, Runs hint)
        {
            Block& block = addBlock(m_page, BlockKind::Entry);
            block.group = m_group;
            block.rows.push_back(std::move(term));
            block.runs = std::move(hint);
        }

        void TableWriter::note(Notes& notes, const Comment& comment, const std::wstring_view name,
            const std::wstring_view category)
        {
            addNote(m_page, noteFor(notes, comment, name, category, false), 1);
        }

        // A value cell: the initializer as written, after an equals sign; nothing where none.
        [[nodiscard]] Runs valueCell(const std::wstring& value)
        {
            if (value.empty())
                return {};
            return { muted(L"= " + value) };
        }

        // The sections of a type page

        void addTypeHead(Page& page, const Surface& surface, Notes& notes, const Type& type)
        {
            addTitle(page, type.name);
            Runs kind = { muted(std::wstring{ kindWord(type.kind) }) };
            if (type.isControl)
                kind.push_back(muted(L" \u00B7 " + std::wstring{ k_controlWord }));
            if (!type.templateParameters.empty())
                kind.push_back(muted(L" \u00B7 template <" + type.templateParameters + L">"));
            addMeta(page, std::move(kind));
            addLead(page, type.comment);
            addMeta(page, {
                muted(L"namespace "),
                code(type.nameSpace),
                muted(L" \u00B7 module "),
                code(type.module),
                muted(L" \u00B7 "),
                code(fileLineOf(surface, type.place))
            });
            if (!type.bases.empty())
            {
                Runs runs = { muted(L"derives from ") };
                for (std::size_t i = 0; i != type.bases.size(); ++i)
                {
                    if (i != 0)
                        runs.push_back(muted(L", "));
                    runs.push_back(typeRun(surface, type.bases[i], type.nameSpace));
                }
                addMeta(page, std::move(runs));
            }
            if (!type.target.empty())
            {
                const bool alias = type.kind == TypeKind::Alias;
                addMeta(page, {
                    muted(alias ? L"stands for " : L"requires "),
                    alias ? typeRun(surface, type.target, type.nameSpace) : code(type.target)
                });
            }
            addNote(page, noteFor(notes, type.comment, type.name, type.category, true), 0);
        }

        void addProperties(Page& page, const Surface& surface, Notes& notes, const Type& type)
        {
            if (type.properties.empty())
                return;
            addHeading(page, L"Properties");
            TableWriter table{ page, Groups::properties };
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
                table.entry({ { code(property.name) }, std::move(spelled), std::move(value) },
                    hintRuns(property.comment));
                table.note(notes, property.comment, property.name, type.category);
            }
        }

        void addEvents(Page& page, const Surface& surface, Notes& notes, const Type& type)
        {
            if (type.events.empty())
                return;
            addHeading(page, L"Events");
            TableWriter table{ page, Groups::events };
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
                table.entry(
                    { { code(event.alias) }, { typeRun(surface, event.type, type.nameSpace) } },
                    std::move(hint));
                table.note(notes, event.comment, event.type, type.category);
            }
        }

        void addMethods(Page& page, Notes& notes, const Type& type)
        {
            if (type.functions.empty())
                return;
            addHeading(page, L"Methods");
            TableWriter table{ page, Groups::methods };
            for (const Access access : { Access::Public, Access::Protected })
            {
                bool any = false;
                for (const Function& function : type.functions)
                {
                    if (function.access != access)
                        continue;
                    if (!any && access == Access::Protected)
                    {
                        addSubHeading(page, { plain(L"Protected"),
                            muted(L" - what a derived class reaches") });
                    }
                    any = true;
                    table.entry({ { code(function.signature) } }, hintRuns(function.comment));
                    table.note(notes, function.comment, function.name, type.category);
                }
            }
        }

        void addFields(Page& page, const Surface& surface, Notes& notes, const Type& type)
        {
            if (type.fields.empty())
                return;
            addHeading(page, L"Fields");
            TableWriter table{ page, Groups::fields };
            for (const Field& field : type.fields)
            {
                table.entry(
                    { { code(field.name) }, { typeRun(surface, field.type, type.nameSpace) },
                        valueCell(field.value) },
                    hintRuns(field.comment));
                table.note(notes, field.comment, field.name, type.category);
            }
        }

        void addMembers(Page& page, Notes& notes, const Type& type)
        {
            if (type.members.empty())
                return;
            addHeading(page, L"Members");
            TableWriter table{ page, Groups::members };
            for (const EnumMember& member : type.members)
            {
                table.entry({ { code(member.name) }, valueCell(member.value) },
                    hintRuns(member.comment));
                table.note(notes, member.comment, member.name, type.category);
            }
        }

        void addSiblings(Page& page, const Surface& surface, const Type& type)
        {
            Runs runs;
            for (const Type& other : surface.types())
            {
                if (&other == &type || other.module != type.module || !other.isPublic())
                    continue;
                if (!runs.empty())
                    runs.push_back(muted(L", "));
                runs.push_back(nameRun(other));
            }
            if (runs.empty())
                return;
            addHeading(page, L"Also in this module");
            addParagraph(page, std::move(runs));
        }

        // The sections of a module page and a chapter page

        void addTypeRows(TableWriter& table, const TypeList& types)
        {
            for (const Type* type : types)
            {
                const Runs kind = type->isControl
                    ? Runs{ plain(std::wstring{ k_controlWord }) }
                    : Runs{ muted(std::wstring{ kindWord(type->kind) }) };
                table.row({ { nameRun(*type) }, kind, hintRuns(type->comment) });
            }
        }

        void addFunctionRows(Page& page, const Surface& surface,
            const std::function<bool(const FreeFunction&)>& takes)
        {
            std::optional<TableWriter> table;
            for (const FreeFunction& free : surface.functions())
            {
                if (!free.exported || !takes(free))
                    continue;
                if (!table)
                {
                    addHeading(page, L"Functions");
                    table.emplace(page, Groups::functions);
                }
                table->entry({ { code(free.function.signature) } },
                    hintRuns(free.function.comment));
            }
        }

        void addConstantRows(Page& page, const Surface& surface,
            const std::function<bool(const Variable&)>& takes)
        {
            std::optional<TableWriter> table;
            for (const Variable& variable : surface.variables())
            {
                if (!variable.exported || !takes(variable))
                    continue;
                if (!table)
                {
                    addHeading(page, L"Constants");
                    table.emplace(page, Groups::constants);
                }
                table->entry({ { code(variable.field.name) },
                    { typeRun(surface, variable.field.type, variable.nameSpace) },
                    valueCell(variable.field.value) }, hintRuns(variable.field.comment));
            }
        }

        [[nodiscard]] std::wstring countOf(const std::size_t value, const std::wstring_view noun)
        {
            return std::to_wstring(value) + L" " + std::wstring{ noun } + (value == 1 ? L"" : L"s");
        }
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
        std::wstring_view path = relativeFile;
        if (path.starts_with(k_sourceRoot))
            path.remove_prefix(k_sourceRoot.size());
        const std::size_t slash = path.rfind(L'/');
        if (slash == std::wstring_view::npos)
            return {};
        return std::wstring{ path.substr(0, slash) };
    }

    Page chapterPage(const Surface& surface, Notes&, const ContentsChapter& chapter)
    {
        Page page;
        addTitle(page, chapter.name);
        addMeta(page, { code(chapter.category), muted(L" under Source") });

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
            muted(L" \u00B7 "),
            plain(countOf(types, L"type")),
            muted(L" \u00B7 "),
            plain(countOf(chapter.modules.size(), L"module"))
        };
        if (withoutHint != 0)
        {
            figures.push_back(muted(L" \u00B7 "));
            figures.push_back(missing(countOf(withoutHint, L"type") + L" without a hint"));
        }
        addMeta(page, std::move(figures));

        for (const ContentsModule& module : chapter.modules)
        {
            addSubHeading(page, { { .text = module.shortName, .link = module.name },
                muted(L"  " + module.name) });
            TableWriter table{ page, Groups::overview };
            addTypeRows(table, module.types);
        }
        addFunctionRows(page, surface, [&](const FreeFunction& free) {
            return categoryOf(surface.files()[free.function.place.file].relative)
                == chapter.category;
        });
        addConstantRows(page, surface, [&](const Variable& variable) {
            return categoryOf(surface.files()[variable.field.place.file].relative)
                == chapter.category;
        });
        return page;
    }

    Page modulePage(const Surface& surface, Notes&, const ContentsChapter& chapter,
        const ContentsModule& module)
    {
        Page page;
        addTitle(page, module.shortName);
        addMeta(page, { muted(L"module "), code(module.name), muted(L" \u00B7 "),
            plain(chapter.name) });
        if (!module.types.empty())
        {
            addHeading(page, L"Types");
            TableWriter table{ page, Groups::overview };
            addTypeRows(table, module.types);
        }
        addFunctionRows(page, surface, [&](const FreeFunction& free) {
            return free.module == module.name;
        });
        addConstantRows(page, surface, [&](const Variable& variable) {
            return variable.module == module.name;
        });
        return page;
    }

    Page typePage(const Surface& surface, Notes& notes, const Type& type)
    {
        Page page;
        addTypeHead(page, surface, notes, type);
        addProperties(page, surface, notes, type);
        addEvents(page, surface, notes, type);
        addMethods(page, notes, type);
        addFields(page, surface, notes, type);
        addMembers(page, notes, type);
        addSiblings(page, surface, type);
        return page;
    }
}
