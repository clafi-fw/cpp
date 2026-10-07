module SeeDocs_App.Surface;

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
    }

    bool Type::opensScope() const
    {
        return kind == TypeKind::Class || kind == TypeKind::Struct || kind == TypeKind::Union
            || kind == TypeKind::Enum;
    }

    std::size_t Surface::addFile(SourceFile file)
    {
        m_files.push_back(std::move(file));
        return m_files.size() - 1;
    }

    void Surface::addModule(Module module)
    {
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

    // The chain is walked by spelling: an alias stands for its target, and a base that is one of
    // a template's parameters stands for the argument the spelling passed in that place.
    bool Surface::reachesControl(const Type& type) const
    {
        if (type.kind != TypeKind::Class)
            return false;

        struct Spelled
        {
            std::wstring text;
            std::wstring nameSpace;
        };
        std::vector<Spelled> queue;
        queue.push_back({ type.name, type.nameSpace });
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
            if (!found)
                continue;
            if (found->kind == TypeKind::Alias)
            {
                queue.push_back({ found->target, found->nameSpace });
                continue;
            }
            const std::wstring_view bare = bareName(found->name);
            if (bare == k_control || bare == k_richControl)
                return true;

            const Names parameters = templateParameterNames(found->templateParameters);
            const Names arguments = argumentsOf(spelled.text);
            for (const std::wstring& base : found->bases)
            {
                const std::wstring_view baseHead = plainType(withoutArguments(base));
                const auto parameter = std::ranges::find(parameters, baseHead);
                if (parameter == parameters.end())
                {
                    queue.push_back({ base, found->nameSpace });
                    continue;
                }
                const std::size_t index =
                    static_cast<std::size_t>(parameter - parameters.begin());
                if (index < arguments.size())
                    queue.push_back({ arguments[index], spelled.nameSpace });
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
}
