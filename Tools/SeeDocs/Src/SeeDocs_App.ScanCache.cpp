module SeeDocs_App.ScanCache;

import SeeDocs_App.PascalReader;
import SeeDocs_App.Surface;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    namespace
    {
        // What opens the file, and the layout it is written in: a file stating another is none.
        constexpr std::uint32_t k_signature = 0x53434453u;
        constexpr std::uint32_t k_layoutVersion = 1u;

        template<typename Value, typename Record>
        concept IsRecordOf = std::same_as<std::remove_const_t<Value>, Record>;

        template<typename Value>
        concept IsText = std::same_as<Value, std::wstring> || std::same_as<Value, std::string>;

        template<typename Value>
        concept IsOptional = requires { typename Value::value_type; }
            && std::same_as<Value, std::optional<typename Value::value_type>>;

        template<typename Value>
        concept IsVector = requires { typename Value::value_type; }
            && std::same_as<Value, std::vector<typename Value::value_type>>;

        template<typename Value>
        concept IsSet = requires { typename Value::value_type; }
            && std::same_as<Value, std::set<typename Value::value_type>>;

        // **Each record's fields, in the one order** both the writer and the reader go by.
        template<typename Archive, IsRecordOf<Place> Value>
        void fields(Archive& archive, Value& place)
        {
            archive(place.file, place.line);
        }

        template<typename Archive, IsRecordOf<Reference> Value>
        void fields(Archive& archive, Value& reference)
        {
            archive(reference.note, reference.anchor);
        }

        template<typename Archive, IsRecordOf<Comment> Value>
        void fields(Archive& archive, Value& comment)
        {
            archive(comment.above, comment.trailing, comment.aboveLine, comment.aboveWidth,
                comment.lineWidth, comment.text, comment.reference);
        }

        template<typename Archive, IsRecordOf<Property> Value>
        void fields(Archive& archive, Value& property)
        {
            archive(property.name, property.type, property.accepts, property.defaultValue,
                property.setter, property.target, property.form, property.valueKind,
                property.optional, property.access, property.place, property.comment);
        }

        template<typename Archive, IsRecordOf<Restatement> Value>
        void fields(Archive& archive, Value& restatement)
        {
            archive(restatement.type, restatement.value, restatement.fixed, restatement.place);
        }

        template<typename Archive, IsRecordOf<Event> Value>
        void fields(Archive& archive, Value& event)
        {
            archive(event.type, event.alias, event.method, event.access, event.place,
                event.comment);
        }

        template<typename Archive, IsRecordOf<Field> Value>
        void fields(Archive& archive, Value& field)
        {
            archive(field.name, field.type, field.value, field.isStatic, field.isConstant,
                field.access, field.place, field.comment);
        }

        template<typename Archive, IsRecordOf<Using> Value>
        void fields(Archive& archive, Value& declaration)
        {
            archive(declaration.base, declaration.name, declaration.access, declaration.place,
                declaration.comment);
        }

        template<typename Archive, IsRecordOf<Function> Value>
        void fields(Archive& archive, Value& function)
        {
            archive(function.name, function.kind, function.signature, function.returnType,
                function.parameters, function.templateParameters, function.isStatic,
                function.isVirtual, function.isDeleted, function.hasBody, function.access,
                function.place, function.comment);
        }

        template<typename Archive, IsRecordOf<EnumMember> Value>
        void fields(Archive& archive, Value& member)
        {
            archive(member.name, member.value, member.place, member.comment);
        }

        template<typename Archive, IsRecordOf<Type> Value>
        void fields(Archive& archive, Value& type)
        {
            archive(type.name, type.nameSpace, type.qualifiedName, type.kind, type.module,
                type.templateParameters, type.bases, type.target, type.scoped, type.exported,
                type.access, type.isControl, type.folder, type.properties, type.events,
                type.functions, type.usings, type.fields, type.members, type.restatements,
                type.place, type.comment);
        }

        template<typename Archive, IsRecordOf<FreeFunction> Value>
        void fields(Archive& archive, Value& free)
        {
            archive(free.function, free.nameSpace, free.module, free.exported);
        }

        template<typename Archive, IsRecordOf<Variable> Value>
        void fields(Archive& archive, Value& variable)
        {
            archive(variable.field, variable.nameSpace, variable.module, variable.declaration,
                variable.exported);
        }

        template<typename Archive, IsRecordOf<Import> Value>
        void fields(Archive& archive, Value& imported)
        {
            archive(imported.name, imported.exported);
        }

        template<typename Archive, IsRecordOf<Module> Value>
        void fields(Archive& archive, Value& module)
        {
            archive(module.name, module.file, module.isInterface, module.isHeader,
                module.language, module.imports);
        }

        template<typename Archive, IsRecordOf<Binding> Value>
        void fields(Archive& archive, Value& binding)
        {
            archive(binding.name, binding.place);
        }

        template<typename Archive, IsRecordOf<Problem> Value>
        void fields(Archive& archive, Value& problem)
        {
            archive(problem.error, problem.place, problem.text);
        }

        template<typename Archive, IsRecordOf<Republication> Value>
        void fields(Archive& archive, Value& republication)
        {
            archive(republication.file, republication.nameSpace, republication.named);
        }

        template<typename Archive, IsRecordOf<EnclosedInclude> Value>
        void fields(Archive& archive, Value& include)
        {
            archive(include.file, include.included, include.nameSpace);
        }

        template<typename Archive, IsRecordOf<FileRecords> Value>
        void fields(Archive& archive, Value& records)
        {
            archive(records.modules, records.types, records.functions, records.variables,
                records.bindings, records.propsReads, records.problems, records.republications,
                records.enclosedIncludes);
        }

        template<typename Archive, IsRecordOf<FileStamp> Value>
        void fields(Archive& archive, Value& stamp)
        {
            archive(stamp.size, stamp.written);
        }

        template<typename Archive, IsRecordOf<MacroDefinition> Value>
        void fields(Archive& archive, Value& macro)
        {
            archive(macro.name, macro.body, macro.takesArguments);
        }

        template<typename Archive, IsRecordOf<FileMacros> Value>
        void fields(Archive& archive, Value& macros)
        {
            archive(macros.read, macros.unread);
        }

        template<typename Archive, IsRecordOf<OwnerLookup> Value>
        void fields(Archive& archive, Value& lookup)
        {
            archive(lookup.spelled, lookup.scope, lookup.found, lookup.name, lookup.bases);
        }

        template<typename Archive, IsRecordOf<AddedMembers> Value>
        void fields(Archive& archive, Value& added)
        {
            archive(added.owner, added.properties, added.events, added.restatements);
        }

        template<typename Archive, IsRecordOf<CachedFile> Value>
        void fields(Archive& archive, Value& file)
        {
            archive(file.path, file.stamp, file.records, file.lookups, file.added, file.macros,
                file.definesSymbols);
        }

        // Lays values out as they stand in memory - only the build that wrote the file reads it.
        class Writer
        {
        public:
            template<typename... Values>
            void operator()(const Values&... values)
            {
                (put(values), ...);
            }
            [[nodiscard]] const std::string& bytes() const { return m_bytes; }
        private:
            template<typename Value>
            void put(const Value&);
            void putBytes(const void* data, std::size_t size);
        private:
            std::string m_bytes;
        };

        // Takes the values back in the writer's order; a file that runs short fails the read.
        class Reader
        {
        public:
            explicit Reader(std::string_view bytes);
            template<typename... Values>
            void operator()(Values&... values)
            {
                (take(values), ...);
            }
            [[nodiscard]] bool failed() const { return m_failed; }
            [[nodiscard]] bool atEnd() const { return m_at == m_bytes.size(); }
        private:
            template<typename Value>
            void take(Value&);
            void takeBytes(void* data, std::size_t size);
            // A count of items each at least the size given that the bytes left can hold.
            [[nodiscard]] std::size_t takeCount(std::size_t itemSize);
        private:
            std::string_view m_bytes;
            std::size_t m_at{ 0 };
            bool m_failed{ false };
        };

        template<typename Value>
        void Writer::put(const Value& value)
        {
            if constexpr (std::is_arithmetic_v<Value> || std::is_enum_v<Value>)
            {
                putBytes(&value, sizeof(Value));
            }
            else if constexpr (IsText<Value>)
            {
                put(value.size());
                putBytes(value.data(), value.size() * sizeof(typename Value::value_type));
            }
            else if constexpr (std::same_as<Value, std::filesystem::path>)
            {
                put(value.native());
            }
            else if constexpr (IsOptional<Value>)
            {
                put(value.has_value());
                if (value)
                    put(*value);
            }
            else if constexpr (IsVector<Value> || IsSet<Value>)
            {
                put(value.size());
                for (const auto& item : value)
                    put(item);
            }
            else
            {
                fields(*this, value);
            }
        }

        void Writer::putBytes(const void* data, const std::size_t size)
        {
            m_bytes.append(static_cast<const char*>(data), size);
        }

        Reader::Reader(const std::string_view bytes)
            :
            m_bytes{ bytes }
        {
        }

        template<typename Value>
        void Reader::take(Value& value)
        {
            if (m_failed)
                return;
            if constexpr (std::is_arithmetic_v<Value> || std::is_enum_v<Value>)
            {
                takeBytes(&value, sizeof(Value));
            }
            else if constexpr (IsText<Value>)
            {
                using Unit = typename Value::value_type;
                const std::size_t size = takeCount(sizeof(Unit));
                value.resize(size);
                takeBytes(value.data(), size * sizeof(Unit));
            }
            else if constexpr (std::same_as<Value, std::filesystem::path>)
            {
                std::filesystem::path::string_type native;
                take(native);
                value = std::filesystem::path{ std::move(native) };
            }
            else if constexpr (IsOptional<Value>)
            {
                bool present = false;
                take(present);
                value.reset();
                if (!present)
                    return;
                typename Value::value_type item{};
                take(item);
                value = std::move(item);
            }
            else if constexpr (IsVector<Value>)
            {
                const std::size_t count = takeCount(1);
                value.clear();
                value.reserve(count);
                for (std::size_t i = 0; i != count && !m_failed; ++i)
                    take(value.emplace_back());
            }
            else if constexpr (IsSet<Value>)
            {
                const std::size_t count = takeCount(1);
                value.clear();
                for (std::size_t i = 0; i != count && !m_failed; ++i)
                {
                    typename Value::value_type item{};
                    take(item);
                    value.insert(std::move(item));
                }
            }
            else
            {
                fields(*this, value);
            }
        }

        void Reader::takeBytes(void* data, const std::size_t size)
        {
            if (m_failed || size > m_bytes.size() - m_at)
            {
                m_failed = true;
                return;
            }
            std::memcpy(data, m_bytes.data() + m_at, size);
            m_at += size;
        }

        std::size_t Reader::takeCount(const std::size_t itemSize)
        {
            std::size_t count = 0;
            takeBytes(&count, sizeof(count));
            if (m_failed || count > (m_bytes.size() - m_at) / itemSize)
            {
                m_failed = true;
                return 0;
            }
            return count;
        }
    }

    // **The build is read first**, so that a file another build wrote is put aside unread.
    ScanCache readScanCache(const std::filesystem::path& path, const std::wstring_view build)
    {
        std::error_code error;
        const std::uintmax_t size = std::filesystem::file_size(path, error);
        if (error)
            return {};
        std::string bytes;
        bytes.resize(static_cast<std::size_t>(size));
        std::ifstream stream{ path, std::ios::binary };
        if (!stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size())))
            return {};

        Reader reader{ bytes };
        std::uint32_t signature = 0;
        std::uint32_t version = 0;
        ScanCache cache;
        reader(signature, version, cache.build);
        if (reader.failed() || signature != k_signature || version != k_layoutVersion
            || cache.build != build)
        {
            return {};
        }
        reader(cache.mode, cache.hints, cache.symbols, cache.files);
        if (reader.failed() || !reader.atEnd())
            return {};
        return cache;
    }

    bool writeScanCache(const ScanCache& cache, const std::filesystem::path& path)
    {
        Writer writer;
        writer(k_signature, k_layoutVersion, cache.build, cache.mode, cache.hints, cache.symbols,
            cache.files);
        std::ofstream stream{ path, std::ios::binary | std::ios::trunc };
        if (!stream)
            return false;
        const std::string& bytes = writer.bytes();
        stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return static_cast<bool>(stream);
    }
}
