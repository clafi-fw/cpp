module SeeDocs_App.Surface;

import SeeDocs_App.PascalLibraries;

import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        constexpr std::wstring_view k_scope = L"::";

        // What a designer can type into a box, by the type's spelling.
        struct KnownKind
        {
            std::wstring_view type;
            std::wstring_view kind;
        };

        constexpr auto k_knownKinds = std::to_array<KnownKind>({
            { L"bool", L"bool" },
            { L"int", L"int" },
            { L"float", L"float" },
            { L"double", L"float" },
            { L"std::size_t", L"int" },
            { L"std::uint32_t", L"int" },
            { L"std::int32_t", L"int" },
            { L"std::uintptr_t", L"int" },
            { L"std::uint8_t", L"int" },
            { L"std::uint64_t", L"int" },
            { L"std::wstring", L"text" },
            { L"std::wstring_view", L"text" },
            { L"std::string", L"text" },
            { L"wchar_t", L"text" },
            { L"char", L"text" },
            { L"CustomFloatPoint", L"float2" },
            { L"FloatPoint", L"float2" },
            { L"IntPoint", L"int2" },
            { L"FloatRect", L"float4" },
            { L"IntRect", L"int4" },
            { L"Color", L"color" },
            { L"Rgb", L"color" },
            // A callback is wired, not typed: a designer offers it the way it offers an event.
            { L"std::function", L"handler" }
        });

        constexpr std::wstring_view k_unknownKind = L"unknown";
        constexpr std::wstring_view k_objectKind = L"object";
        constexpr std::wstring_view k_enumKind = L"enum";
        constexpr std::wstring_view k_numberKind = L"number";
        constexpr std::wstring_view k_optionalType = L"std::optional";
        constexpr std::wstring_view k_numericWrapper = L"NumericValueWrapper";
        constexpr std::wstring_view k_control = L"Control";
        constexpr std::wstring_view k_richControl = L"RichControl";
        // How many aliases a spelling is followed through before it is given up as circular.
        constexpr std::size_t k_aliasDepth = 8;

        [[nodiscard]] std::optional<std::wstring_view> knownKind(const std::wstring_view type)
        {
            for (const KnownKind& known : k_knownKinds)
            {
                if (known.type == type)
                    return known.kind;
            }
            return std::nullopt;
        }

        // The known kind of a spelling, tried whole and then by its last component.
        [[nodiscard]] std::optional<std::wstring_view> knownKindOf(const std::wstring_view type)
        {
            if (const std::optional<std::wstring_view> whole = knownKind(type))
                return whole;
            return knownKind(bareName(type));
        }

        // Where the bracket opened at `open` closes, or the text's size where it never does.
        [[nodiscard]] std::size_t closeOf(const std::wstring_view text, const std::size_t open)
        {
            int depth = 0;
            for (std::size_t i = open; i != text.size(); ++i)
            {
                const wchar_t current = text[i];
                if (current == L'<' || current == L'(' || current == L'[' || current == L'{')
                    ++depth;
                if (current == L'>' || current == L')' || current == L']' || current == L'}')
                    --depth;
                if (depth == 0)
                    return i;
            }
            return text.size();
        }

        [[nodiscard]] bool isNameChar(const wchar_t value)
        {
            return value == L'_' || std::iswalnum(value) != 0;
        }

        [[nodiscard]] bool isBlank(const wchar_t value)
        {
            return value == L' ' || value == L'\t';
        }

        constexpr std::wstring_view k_see = L"See";

        // What a note's stem and a heading's anchor are spelled with.
        [[nodiscard]] bool isStemChar(const wchar_t value)
        {
            return isNameChar(value) || value == L'.' || value == L'-' || value == L' ';
        }

        [[nodiscard]] bool isAnchorChar(const wchar_t value)
        {
            return isNameChar(value) || value == L'-';
        }

        // The namespace enclosing a namespace - empty for a top-level one and for the global one.
        [[nodiscard]] std::wstring_view enclosing(const std::wstring_view nameSpace)
        {
            const std::size_t cut = nameSpace.rfind(k_scope);
            if (cut == std::wstring_view::npos)
                return {};
            return nameSpace.substr(0, cut);
        }

        // Whether a namespace is that one or stands inside it.
        [[nodiscard]] bool isWithin(const std::wstring_view nameSpace,
            const std::wstring_view outer)
        {
            if (!nameSpace.starts_with(outer))
                return false;
            const std::wstring_view rest = nameSpace.substr(outer.size());
            return rest.empty() || rest.starts_with(k_scope);
        }

        // Where a declaration's default stands: the first = at the top level that is no
        // comparison.
        [[nodiscard]] std::size_t defaultOf(const std::wstring_view parameter)
        {
            int depth = 0;
            for (std::size_t i = 0; i != parameter.size(); ++i)
            {
                const wchar_t current = parameter[i];
                if (current == L'<' || current == L'(' || current == L'[' || current == L'{')
                    ++depth;
                if (current == L'>' || current == L')' || current == L']' || current == L'}')
                    --depth;
                if (current != L'=' || depth != 0)
                    continue;
                const bool joined = (i + 1 < parameter.size() && parameter[i + 1] == L'=')
                    || (i > 0 && std::wstring_view{ L"=!<>" }.find(parameter[i - 1])
                        != std::wstring_view::npos);
                if (!joined)
                    return i;
            }
            return std::wstring_view::npos;
        }

        // A parameter list as a declaration and its definition both spell it: defaults off.
        [[nodiscard]] std::wstring withoutDefaults(const std::wstring_view parameters)
        {
            std::wstring result;
            for (const std::wstring& parameter : splitArguments(parameters))
            {
                if (!result.empty())
                    result += L", ";
                result += trimmed(std::wstring_view{ parameter }.substr(0, defaultOf(parameter)));
            }
            return result;
        }

        // How many leading components two namespaces share.
        [[nodiscard]] std::size_t sharedPrefix(const std::wstring_view left,
            const std::wstring_view right)
        {
            std::size_t shared = 0;
            std::size_t i = 0;
            while (i < left.size() && i < right.size() && left[i] == right[i])
            {
                if (left.substr(i).starts_with(k_scope))
                    ++shared;
                ++i;
            }
            if (i == left.size() && i == right.size())
                ++shared;
            return shared;
        }

        // The library a Delphi unit's class names are read in, by the units it uses.
        [[nodiscard]] ClassLibrary libraryOf(const Modules& modules, const std::wstring_view module)
        {
            for (const Module& candidate : modules)
            {
                if (candidate.name != module)
                    continue;
                const bool fireMonkey = std::ranges::any_of(candidate.imports,
                    [](const Import& imported){
                        return isFireMonkeyUnit(imported.name);
                    });
                return fireMonkey ? ClassLibrary::FireMonkey : ClassLibrary::Vcl;
            }
            return ClassLibrary::Vcl;
        }

        // Every declaration's place and comment, a type's own ahead of its members', until the
        // visitor answers true. The containers are the surface's, const or not alike.
        template<typename Types, typename Functions, typename Variables, typename Visit>
        void visitDeclarations(Types& types, Functions& functions, Variables& variables,
            const Visit& visit)
        {
            const auto visitAll = [&](auto& records) {
                for (auto& record : records)
                {
                    if (visit(record.place, record.comment))
                        return true;
                }
                return false;
            };
            for (auto& type : types)
            {
                const bool stopped = visit(type.place, type.comment)
                    || visitAll(type.properties)
                    || visitAll(type.events)
                    || visitAll(type.functions)
                    || visitAll(type.usings)
                    || visitAll(type.fields)
                    || visitAll(type.members);
                if (stopped)
                    return;
            }
            for (auto& free : functions)
            {
                if (visit(free.function.place, free.function.comment))
                    return;
            }
            for (auto& variable : variables)
            {
                if (visit(variable.field.place, variable.field.comment))
                    return;
            }
        }
    }

    bool Type::opensScope() const
    {
        return kind == TypeKind::Class || kind == TypeKind::Struct || kind == TypeKind::Union
            || kind == TypeKind::Enum || kind == TypeKind::Interface;
    }

    std::size_t Surface::addFile(SourceFile file)
    {
        m_files.push_back(std::move(file));
        return m_files.size() - 1;
    }

    void Surface::addModule(Module module)
    {
        if (module.language != SourceLanguage::Cpp)
            m_languageByModule[module.name] = module.language;
        m_modules.push_back(std::move(module));
    }

    void Surface::addType(Type type)
    {
        const std::size_t index = m_types.size();
        m_byQualifiedName[type.qualifiedName] = index;
        m_byBareName[std::wstring{ bareName(type.name) }].push_back(index);
        m_types.push_back(std::move(type));
    }

    void Surface::addFunction(FreeFunction function)
    {
        m_functions.push_back(std::move(function));
    }

    void Surface::addVariable(Variable variable)
    {
        m_variables.push_back(std::move(variable));
    }

    void Surface::addBinding(Binding binding)
    {
        m_bindings.push_back(std::move(binding));
    }

    void Surface::addPropsRead(const Place place)
    {
        m_propsReads.push_back(place);
    }

    void Surface::addProblem(Problem problem)
    {
        m_problems.push_back(std::move(problem));
    }

    void Surface::addRepublication(Republication republication)
    {
        m_republications.push_back(std::move(republication));
    }

    void Surface::addEnclosedInclude(EnclosedInclude include)
    {
        m_enclosedIncludes.push_back(std::move(include));
    }

    const Type* Surface::typeNamed(const std::wstring_view qualifiedName) const
    {
        const auto found = m_byQualifiedName.find(std::wstring{ qualifiedName });
        if (found == m_byQualifiedName.end())
            return nullptr;
        return &m_types[found->second];
    }

    Type* Surface::typeNamed(const std::wstring_view qualifiedName)
    {
        const auto found = m_byQualifiedName.find(std::wstring{ qualifiedName });
        if (found == m_byQualifiedName.end())
            return nullptr;
        return &m_types[found->second];
    }

    const Type* Surface::resolve(const std::wstring_view spelled,
        const std::wstring_view nameSpace) const
    {
        std::wstring_view name = plainType(withoutArguments(spelled));
        std::wstring_view from = nameSpace;
        for (std::size_t depth = 0; depth != k_aliasDepth; ++depth)
        {
            const Type* type = lookUp(name, from);
            if (!type || type->kind != TypeKind::Alias)
                return type;
            name = plainType(withoutArguments(type->target));
            from = type->nameSpace;
        }
        return nullptr;
    }

    const Type* Surface::named(const std::wstring_view spelled,
        const std::wstring_view nameSpace) const
    {
        return lookUp(plainType(withoutArguments(spelled)), nameSpace);
    }

    const Type* Surface::publicType(const std::wstring_view spelled,
        const std::wstring_view nameSpace) const
    {
        std::wstring_view name = plainType(withoutArguments(spelled));
        std::wstring_view from = nameSpace;
        for (std::size_t depth = 0; depth != k_aliasDepth; ++depth)
        {
            const Type* type = lookUp(name, from);
            if (!type || type->isPublic())
                return type;
            if (type->kind != TypeKind::Alias)
                return nullptr;
            name = plainType(withoutArguments(type->target));
            from = type->nameSpace;
        }
        return nullptr;
    }

    SourceLanguage Surface::languageOf(const std::wstring_view module) const
    {
        const auto found = m_languageByModule.find(std::wstring{ module });
        return found == m_languageByModule.end() ? SourceLanguage::Cpp : found->second;
    }

    std::wstring Surface::resolvedBase(const std::wstring_view spelled,
        std::wstring_view nameSpace) const
    {
        std::wstring result = std::wstring{ spelled };
        for (std::size_t depth = 0; depth != k_aliasDepth; ++depth)
        {
            const Type* type = lookUp(plainType(withoutArguments(result)), nameSpace);
            if (!type || type->kind != TypeKind::Alias || type->isPublic())
                return result;
            result = type->target;
            nameSpace = type->nameSpace;
        }
        return result;
    }

    // The chain is walked by spelling: an alias stands for its target, and a base that is one of
    // a template's parameters stands for the argument the spelling passed in that place.
    bool Surface::reachesControl(const Type& type) const
    {
        if (type.kind != TypeKind::Class)
            return false;

        const bool pascal = languageOf(type.module) == SourceLanguage::Pascal;

        struct Spelled
        {
            std::wstring text;
            std::wstring nameSpace;
            std::wstring module;    // the unit spelling it, whose uses say which library it means
        };
        std::vector<Spelled> queue;
        queue.push_back({ type.name, type.nameSpace, type.module });
        std::set<std::wstring> seen;
        while (!queue.empty())
        {
            const Spelled spelled = std::move(queue.back());
            queue.pop_back();
            const std::wstring_view head = plainType(withoutArguments(spelled.text));
            const std::wstring key =
                spelled.nameSpace + std::wstring{ k_scope } + std::wstring{ head };
            if (!seen.insert(key).second)
                continue;

            const Type* found = lookUp(head, spelled.nameSpace);
            // The libraries answer for a class the tree does not declare, and only for one.
            if (!found)
            {
                const bool libraryControl = pascal
                    && isLibraryControl(bareName(head), libraryOf(m_modules, spelled.module));
                if (libraryControl)
                    return true;
                continue;
            }
            if (found->kind == TypeKind::Alias)
            {
                queue.push_back({ found->target, found->nameSpace, found->module });
                continue;
            }
            const std::wstring_view bare = bareName(found->name);
            const bool root = pascal
                ? isControlRoot(bare)
                : bare == k_control || bare == k_richControl;
            if (root)
                return true;

            const Names parameters = templateParameterNames(found->templateParameters);
            const Names arguments = argumentsOf(spelled.text);
            for (const std::wstring& base : found->bases)
            {
                const std::wstring_view baseHead = plainType(withoutArguments(base));
                const auto parameter = std::ranges::find(parameters, baseHead);
                if (parameter == parameters.end())
                {
                    queue.push_back({ base, found->nameSpace, found->module });
                    continue;
                }
                const std::size_t index =
                    static_cast<std::size_t>(parameter - parameters.begin());
                if (index < arguments.size())
                    queue.push_back({ arguments[index], spelled.nameSpace, spelled.module });
            }
        }
        return false;
    }

    std::wstring Surface::valueKindOf(const std::wstring_view type, std::wstring_view nameSpace,
        bool& optional) const
    {
        optional = false;
        std::wstring spelled = std::wstring{ plainType(type) };
        // A pointer to member is the kind of what it points at: the class it reaches into says
        // where the value lives, not what the value is.
        if (spelled.find(L"::*") != std::wstring::npos)
            spelled = spelled.substr(0, spelled.find(L' '));

        for (std::size_t depth = 0; depth != k_aliasDepth; ++depth)
        {
            // An optional reached through an alias is seen through the same way.
            if (withoutArguments(spelled) == k_optionalType)
            {
                const Names arguments = argumentsOf(spelled);
                if (arguments.empty())
                    return std::wstring{ k_unknownKind };
                optional = true;
                spelled = std::wstring{ plainType(arguments.front()) };
            }
            const std::wstring_view head = withoutArguments(spelled);
            if (const std::optional<std::wstring_view> known = knownKindOf(head))
                return std::wstring{ *known };

            const Type* found = lookUp(head, nameSpace);
            if (!found)
                return std::wstring{ k_unknownKind };
            if (found->kind == TypeKind::Alias)
            {
                spelled = std::wstring{ plainType(found->target) };
                nameSpace = found->nameSpace;
                continue;
            }
            if (found->kind == TypeKind::Defined)
                return std::wstring{ k_unknownKind };
            if (found->kind == TypeKind::Enum)
                return std::wstring{ k_enumKind };

            for (const std::wstring& base : found->bases)
            {
                const std::wstring_view baseHead = withoutArguments(base);
                if (const std::optional<std::wstring_view> known = knownKindOf(baseHead))
                    return std::wstring{ *known };
                if (bareName(baseHead) == k_numericWrapper)
                {
                    const Names arguments = argumentsOf(base);
                    if (!arguments.empty())
                    {
                        const std::optional<std::wstring_view> inner =
                            knownKindOf(arguments.front());
                        if (inner)
                            return std::wstring{ *inner };
                    }
                    return std::wstring{ k_numberKind };
                }
            }
            if (found->kind == TypeKind::Struct)
            {
                const Field* only = nullptr;
                std::size_t count = 0;
                for (const Field& field : found->fields)
                {
                    if (field.isStatic || field.access != Access::Public)
                        continue;
                    only = &field;
                    ++count;
                }
                if (count == 1)
                {
                    const std::optional<std::wstring_view> known =
                        knownKindOf(plainType(only->type));
                    if (known)
                        return std::wstring{ *known };
                }
            }
            // A type the tree declares and nothing reduces to a primitive: a designer shows it
            // as an object of its own, not as a value typed into a box.
            return std::wstring{ k_objectKind };
        }
        return std::wstring{ k_unknownKind };
    }

    Properties Surface::mergedProperties(const Type& type) const
    {
        Properties merged;
        std::unordered_map<std::wstring, std::size_t> byTarget;
        for (const Property& property : type.properties)
        {
            const std::wstring key = property.target.empty() ? property.name : property.target;
            const auto found = byTarget.find(key);
            if (!declaresStorage(property.form) && found != byTarget.end())
            {
                // A second read of the same type is the same property read again, not a
                // further spelling it accepts.
                Property& first = merged[found->second];
                const bool known = first.type == property.type
                    || std::find(first.accepts.begin(), first.accepts.end(), property.type)
                        != first.accepts.end();
                if (!known)
                    first.accepts.push_back(property.type);
                continue;
            }
            byTarget[key] = merged.size();
            merged.push_back(property);
        }
        return merged;
    }

    void Surface::resolveAll()
    {
        for (Type& type : m_types)
        {
            type.isControl = reachesControl(type);
            for (Property& property : type.properties)
                property.valueKind = valueKindOf(property.type, type.nameSpace, property.optional);
        }
    }

    // **An include puts the header's text where it stands**, so a header written to be included
    // from inside a namespace declares into that one, and its own namespaces stand inside it - as
    // do the headers it includes in turn. The first include reaching a header decides.
    void Surface::encloseIncludes()
    {
        std::set<std::size_t> enclosed;
        std::deque<EnclosedInclude> pending;
        for (const EnclosedInclude& include : m_enclosedIncludes)
        {
            if (!include.nameSpace.empty())
                pending.push_back(include);
        }
        while (!pending.empty())
        {
            const EnclosedInclude include = std::move(pending.front());
            pending.pop_front();
            const std::optional<std::size_t> file = fileIncluded(include);
            if (!file || !enclosed.insert(*file).second)
                continue;
            const auto within = [&include](const std::wstring& nameSpace) {
                if (nameSpace.empty())
                    return include.nameSpace;
                return include.nameSpace + std::wstring{ k_scope } + nameSpace;
            };
            for (std::size_t i = 0; i != m_types.size(); ++i)
            {
                if (m_types[i].place.file == *file)
                    rekeyType(i, within(m_types[i].nameSpace));
            }
            for (FreeFunction& free : m_functions)
            {
                if (free.function.place.file == *file)
                    free.nameSpace = within(free.nameSpace);
            }
            for (Variable& variable : m_variables)
            {
                if (variable.field.place.file == *file)
                    variable.nameSpace = within(variable.nameSpace);
            }
            for (Republication& republication : m_republications)
            {
                if (republication.file == *file)
                    republication.nameSpace = within(republication.nameSpace);
            }
            for (const EnclosedInclude& inner : m_enclosedIncludes)
            {
                if (inner.file != *file)
                    continue;
                EnclosedInclude enclosedInner = inner;
                enclosedInner.nameSpace = within(inner.nameSpace);
                pending.push_back(std::move(enclosedInner));
            }
        }
    }

    // **The namespace a header re-publishes from is its library's implementation**: what the
    // library means to be used it names again where a reader is to find it, and the rest of that
    // namespace, and of every namespace inside it, is how it is made.
    void Surface::republish()
    {
        std::set<std::wstring> implementation;
        for (const Republication& republication : m_republications)
        {
            if (const std::optional<std::wstring> from = moveRepublished(republication))
                implementation.insert(*from);
        }
        if (implementation.empty())
            return;

        const auto hidden = [&implementation](const std::wstring_view nameSpace) {
            return std::ranges::any_of(implementation, [nameSpace](const std::wstring& outer) {
                return isWithin(nameSpace, outer);
            });
        };
        for (Type& type : m_types)
        {
            if (hidden(type.nameSpace))
                type.exported = false;
        }
        std::erase_if(m_functions, [&hidden](const FreeFunction& free) {
            return hidden(free.nameSpace);
        });
        std::erase_if(m_variables, [&hidden](const Variable& variable) {
            return hidden(variable.nameSpace);
        });
    }

    // A definition following its declaration states the function again: the same namespace,
    // name, template head and parameters, defaults aside.
    void Surface::dropRedeclarations()
    {
        std::unordered_map<std::wstring, std::size_t> keptAt;
        FreeFunctions kept;
        for (FreeFunction& free : m_functions)
        {
            const Function& function = free.function;
            const std::wstring key = free.nameSpace + std::wstring{ k_scope } + function.name
                + L'<' + withoutDefaults(function.templateParameters) + L">("
                + withoutDefaults(function.parameters) + L')';
            const auto found = keptAt.find(key);
            if (found == keptAt.end())
            {
                keptAt.emplace(key, kept.size());
                kept.push_back(std::move(free));
                continue;
            }
            FreeFunction& first = kept[found->second];
            const bool firstCommented = !first.function.comment.text.empty();
            const bool commented = !function.comment.text.empty();
            const bool declares = first.function.hasBody && !function.hasBody;
            if ((commented && !firstCommented) || (commented == firstCommented && declares))
                first = std::move(free);
        }
        m_functions = std::move(kept);
    }

    void Surface::forEachComment(const std::function<void(const Comment&)>& visit) const
    {
        const auto each = [&](const Place&, const Comment& comment) {
            visit(comment);
            return false;
        };
        visitDeclarations(m_types, m_functions, m_variables, each);
    }

    Comment* Surface::writableComment(const Comment& comment)
    {
        Comment* found = nullptr;
        const auto match = [&](Place&, Comment& candidate) {
            if (&candidate == &comment)
                found = &candidate;
            return found != nullptr;
        };
        visitDeclarations(m_types, m_functions, m_variables, match);
        return found;
    }

    void Surface::shiftLines(const std::size_t file, const std::size_t fromLine,
        const std::ptrdiff_t count)
    {
        const auto shift = [&](std::size_t& line) {
            if (line >= fromLine)
                line = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(line) + count);
        };
        const auto shiftPlace = [&](Place& place) {
            if (place.file == file)
                shift(place.line);
        };
        const auto move = [&](Place& place, Comment& comment) {
            if (place.file == file && comment.aboveLine != 0)
                shift(comment.aboveLine);
            shiftPlace(place);
            return false;
        };
        visitDeclarations(m_types, m_functions, m_variables, move);
        for (Binding& binding : m_bindings)
            shiftPlace(binding.place);
        for (Place& place : m_propsReads)
            shiftPlace(place);
        for (Problem& problem : m_problems)
            shiftPlace(problem.place);
    }

    // A qualified spelling is matched by its tail; a bare one through the namespaces enclosing
    // the caller, and then among every type of that name, the one declared nearest the caller.
    const Type* Surface::lookUp(std::wstring_view bare, const std::wstring_view nameSpace) const
    {
        if (bare.starts_with(k_scope))
            bare.remove_prefix(k_scope.size());
        if (bare.empty())
            return nullptr;

        if (bare.find(k_scope) == std::wstring_view::npos)
        {
            std::wstring_view prefix = nameSpace;
            while (true)
            {
                const std::wstring qualified = prefix.empty()
                    ? std::wstring{ bare }
                    : std::wstring{ prefix } + std::wstring{ k_scope } + std::wstring{ bare };
                if (const Type* type = typeNamed(qualified))
                    return type;
                if (prefix.empty())
                    break;
                prefix = enclosing(prefix);
            }
        }

        const auto found = m_byBareName.find(std::wstring{ bareName(bare) });
        if (found == m_byBareName.end())
            return nullptr;

        const Type* nearest = nullptr;
        std::size_t nearestShare = 0;
        for (const std::size_t index : found->second)
        {
            const Type& type = m_types[index];
            const std::wstring tail = std::wstring{ k_scope } + std::wstring{ bare };
            if (type.qualifiedName != bare && !type.qualifiedName.ends_with(tail))
                continue;
            const std::size_t share = sharedPrefix(type.nameSpace, nameSpace);
            if (!nearest || share > nearestShare)
            {
                nearest = &type;
                nearestShare = share;
            }
        }
        return nearest;
    }

    // Looked up as C++ looks a using-declaration's name up: in the namespace it stands in, then
    // in each one enclosing it. A type moves with the types nested in it and its specializations,
    // a function with every overload of its name.
    std::optional<std::wstring> Surface::moveRepublished(const Republication& republication)
    {
        std::wstring_view named = republication.named;
        std::wstring_view prefix = republication.nameSpace;
        if (named.starts_with(k_scope))
        {
            named.remove_prefix(k_scope.size());
            prefix = {};
        }
        const std::wstring_view bare = bareName(named);
        if (bare.size() == named.size())
            return std::nullopt;

        const std::wstring nested = std::wstring{ bare } + std::wstring{ k_scope };
        const std::wstring specialized = std::wstring{ bare } + L'<';
        while (true)
        {
            const std::wstring qualified = prefix.empty()
                ? std::wstring{ named }
                : std::wstring{ prefix } + std::wstring{ k_scope } + std::wstring{ named };
            const std::wstring from{ enclosing(qualified) };

            std::vector<std::size_t> types;
            for (std::size_t i = 0; i != m_types.size(); ++i)
            {
                const Type& type = m_types[i];
                const bool reached = type.name == bare || type.name.starts_with(nested)
                    || type.name.starts_with(specialized);
                if (type.nameSpace == from && reached)
                    types.push_back(i);
            }
            const auto functionThere = [&from, bare](const FreeFunction& free) {
                return free.nameSpace == from && free.function.name == bare;
            };
            const auto variableThere = [&from, bare](const Variable& variable) {
                return variable.nameSpace == from && variable.field.name == bare;
            };
            const bool reachesFunction = std::ranges::any_of(m_functions, functionThere);
            const bool reachesVariable = std::ranges::any_of(m_variables, variableThere);

            if (!types.empty() || reachesFunction || reachesVariable)
            {
                if (from == republication.nameSpace)
                    return std::nullopt;
                for (const std::size_t index : types)
                    rekeyType(index, republication.nameSpace);
                for (FreeFunction& free : m_functions)
                {
                    if (functionThere(free))
                        free.nameSpace = republication.nameSpace;
                }
                for (Variable& variable : m_variables)
                {
                    if (variableThere(variable))
                        variable.nameSpace = republication.nameSpace;
                }
                return from;
            }
            if (prefix.empty())
                return std::nullopt;
            prefix = enclosing(prefix);
        }
    }

    void Surface::rekeyType(const std::size_t index, std::wstring nameSpace)
    {
        Type& type = m_types[index];
        const auto found = m_byQualifiedName.find(type.qualifiedName);
        if (found != m_byQualifiedName.end() && found->second == index)
            m_byQualifiedName.erase(found);
        type.nameSpace = std::move(nameSpace);
        type.qualifiedName = type.nameSpace.empty()
            ? type.name
            : type.nameSpace + std::wstring{ k_scope } + type.name;
        m_byQualifiedName[type.qualifiedName] = index;
    }

    // Beside the file the include stands in, under the root of any folder read, or - the build
    // saying where else headers are looked for - the one file whose path ends with the name.
    std::optional<std::size_t> Surface::fileIncluded(const EnclosedInclude& include) const
    {
        const SourceFile& including = m_files[include.file];
        const std::filesystem::path from =
            std::filesystem::path{ including.relative }.parent_path();
        const std::wstring beside = (from / include.included).lexically_normal().generic_wstring();
        const std::wstring ending = L'/' + include.included;
        std::optional<std::size_t> underRoot;
        std::optional<std::size_t> endingIn;
        std::size_t endings = 0;
        for (std::size_t i = 0; i != m_files.size(); ++i)
        {
            const std::wstring& relative = m_files[i].relative;
            if (relative == beside && m_files[i].root == including.root)
                return i;
            if (relative == include.included && !underRoot)
                underRoot = i;
            if (relative.ends_with(ending))
            {
                endingIn = i;
                ++endings;
            }
        }
        if (underRoot)
            return underRoot;
        if (endings == 1)
            return endingIn;
        return std::nullopt;
    }

    bool inheritsConstructors(const Using& declaration)
    {
        return declaration.name == bareName(withoutArguments(declaration.base));
    }

    std::wstring_view bareName(const std::wstring_view qualified)
    {
        int depth = 0;
        std::size_t cut = std::wstring_view::npos;
        for (std::size_t i = 0; i != qualified.size(); ++i)
        {
            const wchar_t current = qualified[i];
            if (current == L'<')
                ++depth;
            if (current == L'>')
                --depth;
            if (depth == 0 && qualified.substr(i).starts_with(k_scope))
                cut = i;
        }
        if (cut == std::wstring_view::npos)
            return qualified;
        return qualified.substr(cut + k_scope.size());
    }

    std::wstring_view withoutArguments(const std::wstring_view spelled)
    {
        return trimmed(spelled.substr(0, spelled.find(L'<')));
    }

    Names argumentsOf(const std::wstring_view spelled)
    {
        const std::size_t open = spelled.find(L'<');
        if (open == std::wstring_view::npos)
            return {};
        const std::size_t close = closeOf(spelled, open);
        return splitArguments(spelled.substr(open + 1, close - open - 1));
    }

    std::wstring_view plainType(std::wstring_view spelled)
    {
        spelled = trimmed(spelled);
        for (const std::wstring_view prefix : { L"const ", L"volatile ", L"typename " })
        {
            while (spelled.starts_with(prefix))
                spelled = trimmed(spelled.substr(prefix.size()));
        }
        while (!spelled.empty() && (spelled.back() == L'&' || spelled.back() == L'*'))
            spelled = trimmed(spelled.substr(0, spelled.size() - 1));
        while (spelled.ends_with(L" const"))
            spelled = trimmed(spelled.substr(0, spelled.size() - 6));
        return spelled;
    }

    Names splitArguments(const std::wstring_view text)
    {
        Names parts;
        int depth = 0;
        std::size_t start = 0;
        for (std::size_t i = 0; i != text.size(); ++i)
        {
            const wchar_t current = text[i];
            if (current == L'<' || current == L'(' || current == L'[' || current == L'{')
                ++depth;
            if (current == L'>' || current == L')' || current == L']' || current == L'}')
                --depth;
            if (current == L',' && depth == 0)
            {
                parts.emplace_back(trimmed(text.substr(start, i - start)));
                start = i + 1;
            }
        }
        const std::wstring_view last = trimmed(text.substr(start));
        if (!last.empty())
            parts.emplace_back(last);
        return parts;
    }

    std::wstring_view trimmed(std::wstring_view text)
    {
        trim(text);
        return text;
    }

    std::wstring slug(const std::wstring_view text)
    {
        std::wstring result;
        bool dash = false;
        for (const wchar_t current : bareName(text))
        {
            const wchar_t lower = static_cast<wchar_t>(std::towlower(current));
            const bool letter = lower >= L'a' && lower <= L'z';
            const bool digit = lower >= L'0' && lower <= L'9';
            const bool keep = letter || digit;
            if (keep)
            {
                if (dash && !result.empty())
                    result += L'-';
                dash = false;
                result += lower;
            }
            else
            {
                dash = true;
            }
        }
        return result;
    }

    std::optional<Reference> cutReference(std::wstring& text,
        const std::wstring_view name)
    {
        std::size_t at = text.find(k_see);
        while (at != std::wstring::npos)
        {
            const bool boundary = at == 0 || !isNameChar(text[at - 1]);
            const std::size_t afterWord = at + k_see.size();
            const bool spaced = afterWord < text.size() && isBlank(text[afterWord]);
            if (boundary && spaced)
            {
                std::wstring_view rest = trimmed(std::wstring_view{ text }.substr(afterWord));
                const std::size_t hash = rest.find(L'#');
                const std::wstring_view stem = trimmed(rest.substr(0, hash));
                const std::wstring_view anchor = hash == std::wstring_view::npos
                    ? std::wstring_view{}
                    : rest.substr(hash + 1);
                const bool stemFits = !stem.empty() && std::ranges::all_of(stem, isStemChar);
                const bool anchorFits = std::ranges::all_of(anchor, isAnchorChar)
                    && (hash == std::wstring_view::npos || !anchor.empty());
                if (stemFits && anchorFits)
                {
                    Reference reference = {
                        .note = std::wstring{ stem },
                        .anchor = anchor.empty() ? slug(bareName(name)) : std::wstring{ anchor }
                    };
                    text = std::wstring{ trimmed(std::wstring_view{ text }.substr(0, at)) };
                    return reference;
                }
            }
            at = text.find(k_see, at + 1);
        }
        return std::nullopt;
    }

    Names templateParameterNames(const std::wstring_view parameters)
    {
        Names names;
        for (const std::wstring& parameter : splitArguments(parameters))
        {
            std::wstring_view declared = parameter;
            declared = trimmed(declared.substr(0, declared.find(L'=')));
            std::size_t end = declared.size();
            while (end != 0 && !isNameChar(declared[end - 1]))
                --end;
            std::size_t start = end;
            while (start != 0 && isNameChar(declared[start - 1]))
                --start;
            const std::wstring_view name = declared.substr(start, end - start);
            // A parameter spelled by its kind alone - `typename` - names nothing a base could be.
            if (name.empty() || name == L"typename" || name == L"class" || start == 0)
                continue;
            names.emplace_back(name);
        }
        return names;
    }

    bool isTemplateParameter(const Type& type, const std::wstring_view spelled)
    {
        const Names parameters = templateParameterNames(type.templateParameters);
        return std::ranges::find(parameters, plainType(withoutArguments(spelled)))
            != parameters.end();
    }
}
