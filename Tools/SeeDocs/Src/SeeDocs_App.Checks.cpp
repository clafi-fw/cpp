module SeeDocs_App.Checks;

import SeeDocs_App.Surface;

import ClaFi.Documents.TextFile;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        constexpr std::wstring_view k_eventSuffix = L"Event";
        constexpr std::wstring_view k_aliasPrefix = L"On";
        constexpr std::wstring_view k_memberPrefix = L"m_";
        constexpr std::wstring_view k_unknownKind = L"unknown";
        constexpr std::wstring_view k_baseFolder = L"Source/Controls/Base/";
        constexpr std::wstring_view k_noteExtension = L".md";
        // What a trailing comment costs beyond its words: the blank, the slashes, the blank.
        constexpr std::size_t k_trailingCost = 4;

        [[nodiscard]] std::wstring number(const std::size_t value)
        {
            return std::to_wstring(value);
        }

        // The notes under RawDocs, read once each, as the anchors their headings offer.
        class Notes
        {
        public:
            explicit Notes(std::filesystem::path folder);
            [[nodiscard]] bool exists(std::wstring_view stem);
            [[nodiscard]] bool reaches(std::wstring_view stem, std::wstring_view anchor);
        private:
            using Anchors = std::set<std::wstring>;
            [[nodiscard]] const Anchors* anchorsOf(std::wstring_view stem);
        private:
            std::filesystem::path m_folder;
            std::map<std::wstring, std::optional<Anchors>> m_read;
        };

        Notes::Notes(std::filesystem::path folder)
            :
            m_folder{ std::move(folder) }
        {
        }

        bool Notes::exists(const std::wstring_view stem)
        {
            return anchorsOf(stem) != nullptr;
        }

        bool Notes::reaches(const std::wstring_view stem, const std::wstring_view anchor)
        {
            const Anchors* anchors = anchorsOf(stem);
            return anchors && anchors->contains(std::wstring{ anchor });
        }

        const Notes::Anchors* Notes::anchorsOf(const std::wstring_view stem)
        {
            const auto found = m_read.find(std::wstring{ stem });
            if (found != m_read.end())
                return found->second ? &*found->second : nullptr;

            std::optional<Anchors>& anchors = m_read[std::wstring{ stem }];
            const std::filesystem::path file =
                m_folder / (std::wstring{ stem } + std::wstring{ k_noteExtension });
            const std::optional<std::wstring> text = Documents::readTextFile(file);
            if (!text)
                return nullptr;
            anchors.emplace();
            std::size_t start = 0;
            while (start < text->size())
            {
                std::size_t end = text->find(L'\n', start);
                if (end == std::wstring::npos)
                    end = text->size();
                std::wstring_view line = std::wstring_view{ *text }.substr(start, end - start);
                start = end + 1;
                if (!line.starts_with(L'#'))
                    continue;
                while (line.starts_with(L'#'))
                    line.remove_prefix(1);
                anchors->insert(slug(trimmed(line)));
            }
            return &*anchors;
        }

        // The rules, run over one surface.
        class Checker
        {
        public:
            Checker(const Surface&, const std::filesystem::path& root);
            [[nodiscard]] Report run();
        private:
            void checkComments();
            void describe(std::wstring_view name, const Comment&, const Place&, bool opensScope);
            void checkReference(std::wstring_view text, std::wstring_view name, Place);
            void checkBindings();
            void checkEvents();
            void checkChains();
            void checkBaseTypes();
            void checkProblems();
            [[nodiscard]] bool hasFunction(const Type&, std::wstring_view name) const;
        private:
            const Surface& m_surface;
            Notes m_notes;
            Report m_report;
            std::vector<std::pair<std::wstring, Place>> m_missing;   // comments not written
        };

        Checker::Checker(const Surface& surface, const std::filesystem::path& root)
            :
            m_surface{ surface },
            m_notes{ root / k_sourceFolder / k_notesFolder }
        {
        }

        Report Checker::run()
        {
            checkComments();
            checkBindings();
            checkEvents();
            checkChains();
            checkBaseTypes();
            checkProblems();
            m_report.sort(m_surface);
            return std::move(m_report);
        }

        // Every harvested declaration carries one comment line, where the routine puts it; a
        // declaration of the design surface - a control, its properties and events, an enum -
        // carries one at all.
        void Checker::checkComments()
        {
            for (const Type& type : m_surface.types())
            {
                if (!type.isPublic() || !type.opensScope())
                    continue;
                describe(type.name, type.comment, type.place, true);
                for (const Property& property : type.properties)
                    describe(property.name, property.comment, property.place, false);
                for (const Event& event : type.events)
                    describe(event.type, event.comment, event.place, false);
                for (const EnumMember& member : type.members)
                {
                    describe(type.name + L"::" + member.name, member.comment, member.place,
                        false);
                }
                if (type.kind == TypeKind::Struct && type.name.ends_with(k_eventSuffix))
                {
                    for (const Field& field : type.fields)
                    {
                        if (field.isStatic || field.access != Access::Public)
                            continue;
                        describe(type.name + L"::" + field.name, field.comment, field.place,
                            false);
                    }
                }
            }

            std::set<std::pair<std::wstring, std::size_t>> surface;
            for (const Type& type : m_surface.types())
            {
                if (!type.isPublic())
                    continue;
                const bool listed = type.kind == TypeKind::Enum
                    ? type.exported && !type.members.empty()
                    : type.isControl;
                if (!listed)
                    continue;
                surface.insert({ type.name, type.place.line });
                if (type.kind == TypeKind::Enum)
                    continue;
                for (const Property& property : type.properties)
                    surface.insert({ property.name, property.place.line });
                for (const Event& event : type.events)
                    surface.insert({ event.type, event.place.line });
            }
            for (const auto& [name, place] : m_missing)
            {
                if (name.find(L"::") != std::wstring::npos)
                    continue;
                if (surface.contains({ name, place.line }))
                    m_report.add(Level::Warning, place, name + L" has no comment");
            }
        }

        void Checker::describe(const std::wstring_view name, const Comment& comment,
            const Place& place, const bool opensScope)
        {
            const std::wstring named = std::wstring{ name };
            if (comment.trailing)
            {
                if (!comment.above.empty())
                {
                    m_report.add(Level::Error, place,
                        named + L" carries a comment above the declaration and one on it");
                }
                if (opensScope)
                {
                    m_report.add(Level::Error, place,
                        named + L" opens a scope and its comment belongs above the declaration");
                }
                if (comment.lineWidth > k_maxColumns)
                {
                    m_report.add(Level::Error, place, named + L" runs to "
                        + number(comment.lineWidth) + L" columns, " + number(k_maxColumns)
                        + L" is the limit - the comment goes above");
                }
                checkReference(*comment.trailing, name, place);
                return;
            }
            if (comment.above.empty())
            {
                m_missing.emplace_back(named, place);
                return;
            }

            const Place start = { place.file, comment.aboveLine };
            if (comment.above.size() > 1)
            {
                m_report.add(Level::Error, start, named + L" has a comment of "
                    + number(comment.above.size()) + L" lines, one is the maximum");
            }
            else if (comment.aboveWidth > k_maxColumns)
            {
                m_report.add(Level::Error, start, L"the comment on " + named + L" runs to "
                    + number(comment.aboveWidth) + L" columns, " + number(k_maxColumns)
                    + L" is the limit");
            }
            else if (!opensScope && comment.lineWidth != 0)
            {
                const std::size_t joined = comment.lineWidth + k_trailingCost
                    + comment.above.front().size();
                if (joined <= k_maxColumns)
                {
                    m_report.add(Level::Warning, place, named
                        + L" fits its comment on the declaration line, in " + number(joined)
                        + L" columns");
                }
            }
            for (std::size_t i = 0; i != comment.above.size(); ++i)
                checkReference(comment.above[i], name, { place.file, comment.aboveLine + i });
        }

        void Checker::checkReference(const std::wstring_view text, const std::wstring_view name,
            const Place place)
        {
            std::wstring words = std::wstring{ text };
            const std::optional<Reference> reference = cutReference(words, name);
            if (!reference)
                return;
            if (!m_notes.exists(reference->note))
            {
                m_report.add(Level::Error, place,
                    L"reference to a note that does not exist: " + reference->note);
            }
            else if (!m_notes.reaches(reference->note, reference->anchor))
            {
                m_report.add(Level::Error, place, reference->note
                    + L" has no section the reference can reach: #" + reference->anchor);
            }
        }

        // A property with storage of its own is initialised in the constructor; the getter and the
        // setter a form leaves to the class are there; the value is of a kind a designer can show.
        void Checker::checkBindings()
        {
            std::set<std::wstring> bound;
            for (const Binding& binding : m_surface.bindings())
                bound.insert(binding.name);

            for (const Type& type : m_surface.types())
            {
                if (!type.isPublic())
                    continue;
                for (const Property& property : type.properties)
                {
                    if (declaresStorage(property.form))
                    {
                        const bool initialised = bound.contains(property.name)
                            || bound.contains(std::wstring{ k_memberPrefix } + property.name);
                        if (!initialised)
                        {
                            m_report.add(Level::Error, property.place,
                                L"property " + property.name + L" has no INIT_PROPERTY");
                        }
                    }
                    if (property.form == PropertyForm::Storage && !hasFunction(type, property.name))
                    {
                        m_report.add(Level::Warning, property.place, L"property " + property.name
                            + L" declares its storage and the class has no " + property.name
                            + L"() getter");
                    }
                    if (!property.setter.empty() && !hasFunction(type, property.setter))
                    {
                        m_report.add(Level::Warning, property.place, L"property " + property.name
                            + L" names " + property.setter + L" and the class does not have it");
                    }
                    if (property.valueKind == k_unknownKind)
                    {
                        m_report.add(Level::Warning, property.place, L"property " + property.name
                            + L" has no known value kind for " + property.type);
                    }
                }
            }
            for (const Place& place : m_surface.propsReads())
                m_report.add(Level::Warning, place, L"Props::get outside a declared property");
        }

        // The three spellings of an event agree, and its struct is declared.
        void Checker::checkEvents()
        {
            for (const Type& type : m_surface.types())
            {
                if (!type.isPublic())
                    continue;
                for (const Event& event : type.events)
                {
                    if (!event.type.ends_with(k_eventSuffix))
                    {
                        m_report.add(Level::Error, event.place, event.type
                            + L" is an event type and its name does not end in Event");
                    }
                    if (!event.alias.starts_with(k_aliasPrefix))
                    {
                        m_report.add(Level::Error, event.place, L"handler alias is " + event.alias
                            + L" and a handler alias starts with On");
                    }
                    std::wstring expected = event.alias;
                    if (!expected.empty())
                        expected.front() = static_cast<wchar_t>(std::towlower(expected.front()));
                    if (event.method != expected)
                    {
                        m_report.add(Level::Error, event.place, L"connect method is " + event.method
                            + L", " + expected + L" is what the alias says");
                    }
                    if (!m_surface.resolve(event.type, type.nameSpace))
                    {
                        m_report.add(Level::Warning, event.place,
                            L"no declaration found for event type " + event.type);
                    }
                }
            }
        }

        // A class with a surface of its own stands on Control, or on bases the scanner can read.
        void Checker::checkChains()
        {
            for (const Type& type : m_surface.types())
            {
                if (!type.isPublic() || type.kind != TypeKind::Class || type.isControl)
                    continue;
                if (type.bases.empty())
                    continue;
                if (type.properties.empty() && type.events.empty())
                    continue;
                const Names parameters = templateParameterNames(type.templateParameters);
                std::wstring unknown;
                for (const std::wstring& base : type.bases)
                {
                    const std::wstring_view head = plainType(withoutArguments(base));
                    if (std::ranges::find(parameters, head) != parameters.end())
                        continue;
                    if (m_surface.resolve(base, type.nameSpace))
                        continue;
                    if (!unknown.empty())
                        unknown += L", ";
                    unknown += base;
                }
                if (!unknown.empty())
                {
                    m_report.add(Level::Warning, type.place, type.name
                        + L" declares a surface and its base chain does not reach Control: "
                        + unknown);
                }
            }
        }

        // A property type declared beside a base class reaches an application only through an
        // import of that base, and using Button is not a reason to import ButtonBase.
        void Checker::checkBaseTypes()
        {
            for (const Type& type : m_surface.types())
            {
                const bool valueType = type.kind == TypeKind::Enum || type.kind == TypeKind::Struct;
                if (!type.isPublic() || !valueType || type.name.ends_with(k_eventSuffix))
                    continue;
                if (!m_surface.files()[type.place.file].relative.starts_with(k_baseFolder))
                    continue;
                m_report.add(Level::Warning, type.place, type.name
                    + L" is a property type declared beside a base class - it belongs in UiTypes, "
                    L"or in TextEngine.Text if it is a text");
            }
        }

        void Checker::checkProblems()
        {
            for (const Problem& problem : m_surface.problems())
            {
                m_report.add(problem.error ? Level::Error : Level::Warning, problem.place,
                    problem.text);
            }
        }

        // Whether the class or one of its bases declares a function of the name.
        bool Checker::hasFunction(const Type& type, const std::wstring_view name) const
        {
            std::set<const Type*> seen;
            std::vector<const Type*> queue;
            queue.push_back(&type);
            while (!queue.empty())
            {
                const Type* current = queue.back();
                queue.pop_back();
                if (!seen.insert(current).second)
                    continue;
                for (const Function& function : current->functions)
                {
                    if (function.name == name)
                        return true;
                }
                for (const std::wstring& base : current->bases)
                {
                    if (const Type* parent = m_surface.resolve(base, current->nameSpace))
                        queue.push_back(parent);
                }
            }
            return false;
        }

        [[nodiscard]] std::wstring_view levelWord(const Level level)
        {
            return level == Level::Error ? L"error" : L"warning";
        }
    }

    void Report::add(const Level level, const Place place, std::wstring text)
    {
        m_entries.push_back({ level, place, std::move(text) });
    }

    std::size_t Report::errorCount() const
    {
        return static_cast<std::size_t>(std::ranges::count_if(m_entries, [](const Entry& entry){
            return entry.level == Level::Error;
        }));
    }

    void Report::sort(const Surface& surface)
    {
        const SourceFiles& files = surface.files();
        std::ranges::stable_sort(m_entries, [&](const Entry& left, const Entry& right){
            const std::wstring& leftFile = files[left.place.file].relative;
            const std::wstring& rightFile = files[right.place.file].relative;
            if (leftFile != rightFile)
                return leftFile < rightFile;
            return left.place.line < right.place.line;
        });
    }

    Report checkSurface(const Surface& surface, const std::filesystem::path& root)
    {
        Checker checker{ surface, root };
        return checker.run();
    }

    std::wstring formatReport(const Report& report, const Surface& surface)
    {
        std::wstring text;
        for (const Entry& entry : report.entries())
        {
            text += surface.files()[entry.place.file].relative;
            text += L':';
            text += number(entry.place.line);
            text += L": ";
            text += levelWord(entry.level);
            text += L": ";
            text += entry.text;
            text += L'\n';
        }
        return text;
    }
}
