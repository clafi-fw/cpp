module SeeDocs_App.Database;

import SeeDocs_App.Surface;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer: without it a node's set reads a string as a sequence.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        // What a module's Language key says of a Delphi unit; a C++ module states none.
        constexpr std::wstring_view k_pascalWord = L"pascal";

        // The steps of a write, as its times name them.
        namespace Steps
        {
            constexpr std::wstring_view fillDocument = L"fill document";
            constexpr std::wstring_view saveDatabase = L"save database";
            constexpr std::wstring_view loadDatabase = L"load database";
            constexpr std::wstring_view readDocument = L"read document";
        }

        namespace Keys
        {
            constexpr std::wstring_view modules = L"Modules";
            constexpr std::wstring_view types = L"Types";
            constexpr std::wstring_view functions = L"Functions";
            constexpr std::wstring_view constants = L"Constants";
            constexpr std::wstring_view scanned = L"Scanned";
            constexpr std::wstring_view limited = L"Limited";
            constexpr std::wstring_view looseHints = L"LooseHints";
            constexpr std::wstring_view name = L"Name";
            constexpr std::wstring_view nameSpace = L"Namespace";
            constexpr std::wstring_view kind = L"Kind";
            constexpr std::wstring_view module = L"Module";
            constexpr std::wstring_view file = L"File";
            constexpr std::wstring_view root = L"Root";
            constexpr std::wstring_view line = L"Line";
            constexpr std::wstring_view hint = L"Hint";
            constexpr std::wstring_view note = L"Note";
            constexpr std::wstring_view interface = L"Interface";
            constexpr std::wstring_view header = L"Header";
            constexpr std::wstring_view language = L"Language";
            constexpr std::wstring_view imports = L"Imports";
            constexpr std::wstring_view exportedImports = L"ExportedImports";
            constexpr std::wstring_view templateParameters = L"Template";
            constexpr std::wstring_view bases = L"Bases";
            constexpr std::wstring_view base = L"Base";
            constexpr std::wstring_view target = L"Target";
            constexpr std::wstring_view control = L"Control";
            constexpr std::wstring_view scoped = L"Scoped";
            constexpr std::wstring_view folder = L"Folder";
            constexpr std::wstring_view properties = L"Properties";
            constexpr std::wstring_view events = L"Events";
            constexpr std::wstring_view methods = L"Methods";
            constexpr std::wstring_view usings = L"Usings";
            constexpr std::wstring_view fields = L"Fields";
            constexpr std::wstring_view members = L"Members";
            constexpr std::wstring_view restates = L"Restates";
            constexpr std::wstring_view fixed = L"Fixed";
            constexpr std::wstring_view type = L"Type";
            constexpr std::wstring_view accepts = L"Accepts";
            constexpr std::wstring_view defaultValue = L"Default";
            constexpr std::wstring_view setter = L"Setter";
            constexpr std::wstring_view form = L"Form";
            constexpr std::wstring_view valueKind = L"ValueKind";
            constexpr std::wstring_view optional = L"Optional";
            constexpr std::wstring_view access = L"Access";
            constexpr std::wstring_view alias = L"Alias";
            constexpr std::wstring_view method = L"Method";
            constexpr std::wstring_view signature = L"Signature";
            constexpr std::wstring_view isStatic = L"Static";
            constexpr std::wstring_view isVirtual = L"Virtual";
            constexpr std::wstring_view isDeleted = L"Deleted";
            constexpr std::wstring_view isConstant = L"Constant";
            constexpr std::wstring_view value = L"Value";
            constexpr std::wstring_view declaration = L"Declaration";
        }

        using Sections = Dom::Sequence<Dom::Section>;
        using Strings = Dom::Sequence<std::wstring>;

        [[nodiscard]] Dom::Dt::Section placedEntry()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::name, std::wstring{} },
                Value{ Keys::line, std::size_t{ 0 } },
                Value{ Keys::hint, std::wstring{} },
                Value{ Keys::note, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section memberEntry()
        {
            using namespace Dom::Dt;
            return {
                placedEntry(),
                Value{ Keys::access, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section propertyLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::type, std::wstring{} },
                Sequence{ Keys::accepts, std::wstring{} },
                Value{ Keys::defaultValue, std::wstring{} },
                Value{ Keys::setter, std::wstring{} },
                Value{ Keys::target, std::wstring{} },
                Value{ Keys::form, std::wstring{} },
                Value{ Keys::valueKind, std::wstring{} },
                Value{ Keys::optional, false }
            };
        }

        [[nodiscard]] Dom::Dt::Section eventLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::alias, std::wstring{} },
                Value{ Keys::method, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section functionLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::kind, std::wstring{} },
                Value{ Keys::signature, std::wstring{} },
                Value{ Keys::type, std::wstring{} },
                Value{ Keys::templateParameters, std::wstring{} },
                Value{ Keys::isStatic, false },
                Value{ Keys::isVirtual, false },
                Value{ Keys::isDeleted, false }
            };
        }

        [[nodiscard]] Dom::Dt::Section usingLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::base, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section fieldLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::type, std::wstring{} },
                Value{ Keys::value, std::wstring{} },
                Value{ Keys::isStatic, false },
                Value{ Keys::isConstant, false }
            };
        }

        [[nodiscard]] Dom::Dt::Section enumMemberLayout()
        {
            using namespace Dom::Dt;
            return {
                placedEntry(),
                Value{ Keys::value, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section restatementLayout()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::type, std::wstring{} },
                Value{ Keys::value, std::wstring{} },
                Value{ Keys::fixed, false },
                Value{ Keys::line, std::size_t{ 0 } }
            };
        }

        // What an entry at namespace scope states beyond a member.
        [[nodiscard]] Dom::Dt::Section scopedEntry()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::nameSpace, std::wstring{} },
                Value{ Keys::module, std::wstring{} },
                Value{ Keys::file, std::wstring{} },
                Value{ Keys::root, std::size_t{ 0 } }
            };
        }

        [[nodiscard]] Dom::Dt::Section typeLayout()
        {
            using namespace Dom::Dt;
            return {
                placedEntry(),
                scopedEntry(),
                Value{ Keys::kind, std::wstring{} },
                Value{ Keys::templateParameters, std::wstring{} },
                Sequence{ Keys::bases, std::wstring{} },
                Value{ Keys::target, std::wstring{} },
                Value{ Keys::scoped, false },
                Value{ Keys::control, false },
                Value{ Keys::folder, std::wstring{} },
                Sequence{ Keys::properties, propertyLayout() },
                Sequence{ Keys::events, eventLayout() },
                Sequence{ Keys::methods, functionLayout() },
                Sequence{ Keys::usings, usingLayout() },
                Sequence{ Keys::fields, fieldLayout() },
                Sequence{ Keys::members, enumMemberLayout() },
                Sequence{ Keys::restates, restatementLayout() }
            };
        }

        [[nodiscard]] Dom::Dt::Section moduleLayout()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::name, std::wstring{} },
                Value{ Keys::file, std::wstring{} },
                Value{ Keys::root, std::size_t{ 0 } },
                Value{ Keys::interface, false },
                Value{ Keys::header, false },
                Value{ Keys::language, std::wstring{} },
                Sequence{ Keys::imports, std::wstring{} },
                Sequence{ Keys::exportedImports, std::wstring{} }
            };
        }

        using KeyOrder = std::vector<std::wstring>;
        using Items = std::vector<std::unique_ptr<Dom::Section>>;
        using TextNode = Dom::Value<std::wstring>;
        using FlagNode = Dom::Value<bool>;
        using NumberNode = Dom::Value<std::size_t>;

        // The keys of a fresh entry of the layout, in the order it lists them.
        [[nodiscard]] KeyOrder keyOrderOf(const Dom::Section& model)
        {
            KeyOrder order;
            for (const std::wstring_view key : model.getKeys())
                order.emplace_back(key);
            return order;
        }

        // The key order of every kind of entry, read off a fresh entry of the layout for each.
        struct KeyOrders
        {
            KeyOrder root;
            KeyOrder module;
            KeyOrder type;
            KeyOrder property;
            KeyOrder event;
            KeyOrder method;
            KeyOrder usingDeclaration;
            KeyOrder field;
            KeyOrder member;
            KeyOrder restatement;
            KeyOrder function;
            KeyOrder constant;
        };

        [[nodiscard]] KeyOrders keyOrdersOf(Dom::Section& layout)
        {
            Dom::Section& type = (layout / Keys::types).as<Sections>().add();
            return {
                .root = keyOrderOf(layout),
                .module = keyOrderOf((layout / Keys::modules).as<Sections>().add()),
                .type = keyOrderOf(type),
                .property = keyOrderOf((type / Keys::properties).as<Sections>().add()),
                .event = keyOrderOf((type / Keys::events).as<Sections>().add()),
                .method = keyOrderOf((type / Keys::methods).as<Sections>().add()),
                .usingDeclaration = keyOrderOf((type / Keys::usings).as<Sections>().add()),
                .field = keyOrderOf((type / Keys::fields).as<Sections>().add()),
                .member = keyOrderOf((type / Keys::members).as<Sections>().add()),
                .restatement = keyOrderOf((type / Keys::restates).as<Sections>().add()),
                .function = keyOrderOf((layout / Keys::functions).as<Sections>().add()),
                .constant = keyOrderOf((layout / Keys::constants).as<Sections>().add())
            };
        }

        // An entry holding what differs from the layout alone, its keys in a fresh entry's order.
        class Entry
        {
        public:
            explicit Entry(const KeyOrder&);
            void setText(std::wstring_view key, std::wstring_view text);   // kept while not empty
            void setFlag(std::wstring_view key, bool value);               // kept while true
            void setNumber(std::wstring_view key, std::size_t value);      // kept while not zero
            void setList(std::wstring_view key, const Names&);             // kept while not empty
            void setItems(std::wstring_view key, Items);                   // kept while not empty
            [[nodiscard]] std::unique_ptr<Dom::Section> finish();
        private:
            struct Stated
            {
                std::wstring_view key;
                std::size_t rank{ 0 };
                std::unique_ptr<Dom::DomNodeBase> node;
            };
        private:
            void state(std::wstring_view key, std::unique_ptr<Dom::DomNodeBase>);
        private:
            const KeyOrder& m_order;
            std::vector<Stated> m_stated;
        };

        Entry::Entry(const KeyOrder& order)
            :
            m_order{ order }
        {
        }

        void Entry::setText(const std::wstring_view key, const std::wstring_view text)
        {
            if (!text.empty())
                state(key, std::make_unique<TextNode>(nullptr, std::wstring{ text }));
        }

        void Entry::setFlag(const std::wstring_view key, const bool value)
        {
            if (value)
                state(key, std::make_unique<FlagNode>(nullptr, value));
        }

        void Entry::setNumber(const std::wstring_view key, const std::size_t value)
        {
            if (value != 0)
                state(key, std::make_unique<NumberNode>(nullptr, value));
        }

        void Entry::setList(const std::wstring_view key, const Names& names)
        {
            if (names.empty())
                return;
            std::unique_ptr<Strings> list = std::make_unique<Strings>(nullptr, std::wstring{});
            for (const std::wstring& name : names)
                list->children().push_back(std::make_unique<TextNode>(list.get(), name));
            state(key, std::move(list));
        }

        void Entry::setItems(const std::wstring_view key, Items items)
        {
            if (items.empty())
                return;
            std::unique_ptr<Sections> list =
                std::make_unique<Sections>(nullptr, Dom::Section{ nullptr });
            for (std::unique_ptr<Dom::Section>& item : items)
            {
                item->setParent(list.get());
                list->children().push_back(std::move(item));
            }
            state(key, std::move(list));
        }

        // The keys go into the section by rank, so its map lists them as a fresh entry's does.
        std::unique_ptr<Dom::Section> Entry::finish()
        {
            std::ranges::stable_sort(m_stated, {}, &Stated::rank);
            std::unique_ptr<Dom::Section> section = std::make_unique<Dom::Section>(nullptr);
            for (Stated& stated : m_stated)
            {
                stated.node->setParent(section.get());
                section->setChild(stated.key, std::move(stated.node));
            }
            return section;
        }

        void Entry::state(const std::wstring_view key, std::unique_ptr<Dom::DomNodeBase> node)
        {
            for (std::size_t rank = 0; rank != m_order.size(); ++rank)
            {
                if (m_order[rank] != key)
                    continue;
                m_stated.push_back({ .key = key, .rank = rank, .node = std::move(node) });
                return;
            }
            noReach("SeeDocs: the database layout has no key " + toUtf8(key));
        }

        // A file as an entry names it: its path under its root, and the root.
        void setFile(Entry& entry, const SourceFile& file)
        {
            entry.setText(Keys::file, file.relative);
            entry.setNumber(Keys::root, file.root);
        }

        // A stamp as the database writes it, UTC to the second - 2026-10-09T03:12:45Z.
        [[nodiscard]] std::wstring stampText(const ScanStamp& stamp)
        {
            if (!stamp)
                return {};
            const std::chrono::sys_days day = std::chrono::floor<std::chrono::days>(*stamp);
            const std::chrono::year_month_day date{ day };
            const std::chrono::hh_mm_ss<std::chrono::seconds> time{ *stamp - day };
            return std::format(L"{:04}-{:02}-{:02}T{:02}:{:02}:{:02}Z",
                static_cast<int>(date.year()),
                static_cast<unsigned>(date.month()),
                static_cast<unsigned>(date.day()),
                time.hours().count(),
                time.minutes().count(),
                time.seconds().count());
        }

        [[nodiscard]] std::wstring noteText(const Comment& comment)
        {
            if (!comment.reference)
                return {};
            return comment.reference->note + L"#" + comment.reference->anchor;
        }

        void setPlaced(Entry& entry, const std::wstring_view name, const Place& place,
            const Comment& comment)
        {
            entry.setText(Keys::name, name);
            entry.setNumber(Keys::line, place.line);
            entry.setText(Keys::hint, comment.text);
            entry.setText(Keys::note, noteText(comment));
        }

        void setScoped(Entry& entry, const Surface& surface, const std::wstring_view nameSpace,
            const std::wstring_view module, const Place& place)
        {
            entry.setText(Keys::nameSpace, nameSpace);
            entry.setText(Keys::module, module);
            setFile(entry, surface.files()[place.file]);
        }

        void setFunction(Entry& entry, const Function& function)
        {
            setPlaced(entry, function.name, function.place, function.comment);
            entry.setText(Keys::access, accessWord(function.access));
            entry.setText(Keys::kind, kindWord(function.kind));
            entry.setText(Keys::signature, function.signature);
            entry.setText(Keys::type, function.returnType);
            entry.setText(Keys::templateParameters, function.templateParameters);
            entry.setFlag(Keys::isStatic, function.isStatic);
            entry.setFlag(Keys::isVirtual, function.isVirtual);
            entry.setFlag(Keys::isDeleted, function.isDeleted);
        }

        void setField(Entry& entry, const Field& field)
        {
            setPlaced(entry, field.name, field.place, field.comment);
            entry.setText(Keys::access, accessWord(field.access));
            entry.setText(Keys::type, field.type);
            entry.setText(Keys::value, field.value);
            entry.setFlag(Keys::isStatic, field.isStatic);
            entry.setFlag(Keys::isConstant, field.isConstant);
        }

        [[nodiscard]] std::unique_ptr<Dom::Section> typeEntry(const KeyOrders& orders,
            const Surface& surface, const Type& type)
        {
            Entry entry{ orders.type };
            setPlaced(entry, type.name, type.place, type.comment);
            setScoped(entry, surface, type.nameSpace, type.module, type.place);
            entry.setText(Keys::kind, kindWord(type.kind));
            entry.setText(Keys::templateParameters, type.templateParameters);
            Names bases;
            for (const std::wstring& base : type.bases)
            {
                bases.push_back(isTemplateParameter(type, base)
                    ? base
                    : surface.resolvedBase(base, type.nameSpace));
            }
            entry.setList(Keys::bases, bases);
            entry.setText(Keys::target, type.target);
            entry.setFlag(Keys::scoped, type.scoped);
            entry.setFlag(Keys::control, type.isControl);
            entry.setText(Keys::folder, type.folder);

            Items properties;
            for (const Property& property : surface.mergedProperties(type))
            {
                if (property.access == Access::Private)
                    continue;
                Entry item{ orders.property };
                setPlaced(item, property.name, property.place, property.comment);
                item.setText(Keys::access, accessWord(property.access));
                item.setText(Keys::type, property.type);
                item.setList(Keys::accepts, property.accepts);
                item.setText(Keys::defaultValue, property.defaultValue);
                item.setText(Keys::setter, property.setter);
                item.setText(Keys::target, property.target);
                item.setText(Keys::form, formWord(property.form));
                item.setText(Keys::valueKind, property.valueKind);
                item.setFlag(Keys::optional, property.optional);
                properties.push_back(item.finish());
            }
            entry.setItems(Keys::properties, std::move(properties));

            Items events;
            for (const Event& event : type.events)
            {
                if (event.access == Access::Private)
                    continue;
                Entry item{ orders.event };
                setPlaced(item, event.type, event.place, event.comment);
                item.setText(Keys::access, accessWord(event.access));
                item.setText(Keys::alias, event.alias);
                item.setText(Keys::method, event.method);
                events.push_back(item.finish());
            }
            entry.setItems(Keys::events, std::move(events));

            Items methods;
            for (const Function& function : type.functions)
            {
                if (function.access == Access::Private)
                    continue;
                Entry item{ orders.method };
                setFunction(item, function);
                methods.push_back(item.finish());
            }
            entry.setItems(Keys::methods, std::move(methods));

            Items usings;
            for (const Using& declaration : type.usings)
            {
                if (declaration.access == Access::Private)
                    continue;
                Entry item{ orders.usingDeclaration };
                setPlaced(item, declaration.name, declaration.place, declaration.comment);
                item.setText(Keys::access, accessWord(declaration.access));
                item.setText(Keys::base, declaration.base);
                usings.push_back(item.finish());
            }
            entry.setItems(Keys::usings, std::move(usings));

            Items fields;
            for (const Field& field : type.fields)
            {
                if (field.access == Access::Private)
                    continue;
                Entry item{ orders.field };
                setField(item, field);
                fields.push_back(item.finish());
            }
            entry.setItems(Keys::fields, std::move(fields));

            Items members;
            for (const EnumMember& member : type.members)
            {
                Entry item{ orders.member };
                setPlaced(item, member.name, member.place, member.comment);
                item.setText(Keys::value, member.value);
                members.push_back(item.finish());
            }
            entry.setItems(Keys::members, std::move(members));

            Items restates;
            for (const Restatement& restatement : type.restatements)
            {
                Entry item{ orders.restatement };
                item.setText(Keys::type, restatement.type);
                item.setText(Keys::value, restatement.value);
                item.setFlag(Keys::fixed, restatement.fixed);
                item.setNumber(Keys::line, restatement.place.line);
                restates.push_back(item.finish());
            }
            entry.setItems(Keys::restates, std::move(restates));
            return entry.finish();
        }

        [[nodiscard]] std::unique_ptr<Dom::Section> moduleEntry(const KeyOrders& orders,
            const Surface& surface, const Module& module)
        {
            Entry entry{ orders.module };
            entry.setText(Keys::name, module.name);
            setFile(entry, surface.files()[module.file]);
            entry.setFlag(Keys::interface, module.isInterface);
            entry.setFlag(Keys::header, module.isHeader);
            if (module.language == SourceLanguage::Pascal)
                entry.setText(Keys::language, k_pascalWord);
            Names imports;
            Names exported;
            for (const Import& import : module.imports)
                (import.exported ? exported : imports).push_back(import.name);
            entry.setList(Keys::imports, imports);
            entry.setList(Keys::exportedImports, exported);
            return entry.finish();
        }

        // **A key an entry does not hold is at the layout's default** - the file leaves such a key
        // out, and a document built from a surface never makes one - so each reader answers that
        // default itself, and reads a document from either source alike.
        [[nodiscard]] std::wstring textOf(const Dom::Section& entry, const std::wstring_view key)
        {
            const Dom::DomNodeBase* child = entry.child(key);
            return child ? child->get<std::wstring>() : std::wstring{};
        }

        [[nodiscard]] bool flagOf(const Dom::Section& entry, const std::wstring_view key)
        {
            const Dom::DomNodeBase* child = entry.child(key);
            return child ? child->get<bool>() : false;
        }

        [[nodiscard]] std::size_t numberOf(const Dom::Section& entry, const std::wstring_view key)
        {
            const Dom::DomNodeBase* child = entry.child(key);
            return child ? child->get<std::size_t>() : 0;
        }

        [[nodiscard]] Names listOf(const Dom::Section& entry, const std::wstring_view key)
        {
            Names names;
            const Dom::DomNodeBase* child = entry.child(key);
            if (!child)
                return names;
            for (const std::wstring& name : child->as<Strings>().get())
                names.push_back(name);
            return names;
        }

        [[nodiscard]] const Sections& itemsOf(const Dom::Section& entry,
            const std::wstring_view key)
        {
            static const Sections s_none{ nullptr, Dom::Section{ nullptr } };
            const Dom::DomNodeBase* child = entry.child(key);
            return child ? child->as<Sections>() : s_none;
        }

        // The stamp a text spells; nothing for a text the database would not write.
        [[nodiscard]] ScanStamp stampOf(const std::wstring_view text)
        {
            constexpr std::wstring_view k_shape = L"0000-00-00T00:00:00Z";
            if (text.size() != k_shape.size())
                return std::nullopt;
            for (std::size_t i = 0; i != k_shape.size(); ++i)
            {
                const bool digitWanted = k_shape[i] == L'0';
                const bool digitFound = text[i] >= L'0' && text[i] <= L'9';
                if (digitWanted != digitFound)
                    return std::nullopt;
                if (!digitWanted && text[i] != k_shape[i])
                    return std::nullopt;
            }
            auto numberAt = [text](const std::size_t offset, const std::size_t count) {
                int number = 0;
                for (std::size_t i = offset; i != offset + count; ++i)
                    number = number * 10 + static_cast<int>(text[i] - L'0');
                return number;
            };
            const std::chrono::year_month_day date{
                std::chrono::year{ numberAt(0, 4) },
                std::chrono::month{ static_cast<unsigned>(numberAt(5, 2)) },
                std::chrono::day{ static_cast<unsigned>(numberAt(8, 2)) }
            };
            const int hours = numberAt(11, 2);
            const int minutes = numberAt(14, 2);
            const int seconds = numberAt(17, 2);
            if (!date.ok() || hours > 23 || minutes > 59 || seconds > 59)
                return std::nullopt;
            return std::chrono::sys_days{ date } + std::chrono::hours{ hours }
                + std::chrono::minutes{ minutes } + std::chrono::seconds{ seconds };
        }

        [[nodiscard]] std::wstring qualifiedNameOf(const std::wstring& nameSpace,
            const std::wstring& name)
        {
            if (nameSpace.empty())
                return name;
            return nameSpace + L"::" + name;
        }

        // The entries of the file, back into the records the scanner fills.
        class Reader
        {
        public:
            explicit Reader(Surface&);
            void readModules(const Sections& list);
            void readTypes(const Sections& list);
            void readFunctions(const Sections& list);
            void readConstants(const Sections& list);
        private:
            // The file the entry names, by its path and its root.
            [[nodiscard]] std::size_t fileOf(const Dom::Section& entry, const std::wstring& module);
            [[nodiscard]] static Place placeOf(const Dom::Section& entry, std::size_t file);
            [[nodiscard]] static Comment commentOf(const Dom::Section& entry);
            [[nodiscard]] static Access accessIn(const Dom::Section& entry);
            // A member states its access and its specifiers; an entry at namespace scope does not.
            [[nodiscard]] static Function functionOf(const Dom::Section& entry, std::size_t file,
                bool member);
            [[nodiscard]] static Field fieldOf(const Dom::Section& entry, std::size_t file,
                bool member);
        private:
            Surface& m_surface;
            // A file's index by its root and its path under it.
            std::map<std::pair<std::size_t, std::wstring>, std::size_t> m_files;
        };

        Reader::Reader(Surface& surface)
            :
            m_surface{ surface }
        {
        }

        void Reader::readModules(const Sections& list)
        {
            for (const Dom::Section& entry : list)
            {
                Module module{
                    .name = textOf(entry, Keys::name),
                    .isInterface = flagOf(entry, Keys::interface),
                    .isHeader = flagOf(entry, Keys::header),
                    .language = textOf(entry, Keys::language) == k_pascalWord
                        ? SourceLanguage::Pascal
                        : SourceLanguage::Cpp
                };
                module.file = fileOf(entry, module.name);
                for (const std::wstring& name : listOf(entry, Keys::imports))
                    module.imports.push_back({ .name = name, .exported = false });
                for (const std::wstring& name : listOf(entry, Keys::exportedImports))
                    module.imports.push_back({ .name = name, .exported = true });
                m_surface.addModule(std::move(module));
            }
        }

        void Reader::readTypes(const Sections& list)
        {
            for (const Dom::Section& entry : list)
            {
                Type type{
                    .name = textOf(entry, Keys::name),
                    .nameSpace = textOf(entry, Keys::nameSpace),
                    .kind = typeKindOf(textOf(entry, Keys::kind)).value_or(TypeKind::Class),
                    .module = textOf(entry, Keys::module),
                    .templateParameters = textOf(entry, Keys::templateParameters),
                    .bases = listOf(entry, Keys::bases),
                    .target = textOf(entry, Keys::target),
                    .scoped = flagOf(entry, Keys::scoped),
                    .exported = true,
                    .isControl = flagOf(entry, Keys::control),
                    .folder = textOf(entry, Keys::folder)
                };
                type.qualifiedName = qualifiedNameOf(type.nameSpace, type.name);
                const std::size_t file = fileOf(entry, type.module);
                type.place = placeOf(entry, file);
                type.comment = commentOf(entry);

                for (const Dom::Section& item : itemsOf(entry, Keys::properties))
                {
                    type.properties.push_back({
                        .name = textOf(item, Keys::name),
                        .type = textOf(item, Keys::type),
                        .accepts = listOf(item, Keys::accepts),
                        .defaultValue = textOf(item, Keys::defaultValue),
                        .setter = textOf(item, Keys::setter),
                        .target = textOf(item, Keys::target),
                        .form = formOf(textOf(item, Keys::form)).value_or(PropertyForm::Declared),
                        .valueKind = textOf(item, Keys::valueKind),
                        .optional = flagOf(item, Keys::optional),
                        .access = accessIn(item),
                        .place = placeOf(item, file),
                        .comment = commentOf(item)
                    });
                }
                for (const Dom::Section& item : itemsOf(entry, Keys::events))
                {
                    type.events.push_back({
                        .type = textOf(item, Keys::name),
                        .alias = textOf(item, Keys::alias),
                        .method = textOf(item, Keys::method),
                        .access = accessIn(item),
                        .place = placeOf(item, file),
                        .comment = commentOf(item)
                    });
                }
                for (const Dom::Section& item : itemsOf(entry, Keys::methods))
                    type.functions.push_back(functionOf(item, file, true));
                for (const Dom::Section& item : itemsOf(entry, Keys::usings))
                {
                    type.usings.push_back({
                        .base = textOf(item, Keys::base),
                        .name = textOf(item, Keys::name),
                        .access = accessIn(item),
                        .place = placeOf(item, file),
                        .comment = commentOf(item)
                    });
                }
                for (const Dom::Section& item : itemsOf(entry, Keys::fields))
                    type.fields.push_back(fieldOf(item, file, true));
                for (const Dom::Section& item : itemsOf(entry, Keys::members))
                {
                    type.members.push_back({
                        .name = textOf(item, Keys::name),
                        .value = textOf(item, Keys::value),
                        .place = placeOf(item, file),
                        .comment = commentOf(item)
                    });
                }
                for (const Dom::Section& item : itemsOf(entry, Keys::restates))
                {
                    type.restatements.push_back({
                        .type = textOf(item, Keys::type),
                        .value = textOf(item, Keys::value),
                        .fixed = flagOf(item, Keys::fixed),
                        .place = placeOf(item, file)
                    });
                }
                m_surface.addType(std::move(type));
            }
        }

        void Reader::readFunctions(const Sections& list)
        {
            for (const Dom::Section& entry : list)
            {
                FreeFunction free{
                    .nameSpace = textOf(entry, Keys::nameSpace),
                    .module = textOf(entry, Keys::module),
                    .exported = true
                };
                const std::size_t file = fileOf(entry, free.module);
                free.function = functionOf(entry, file, false);
                m_surface.addFunction(std::move(free));
            }
        }

        void Reader::readConstants(const Sections& list)
        {
            for (const Dom::Section& entry : list)
            {
                Variable variable{
                    .nameSpace = textOf(entry, Keys::nameSpace),
                    .module = textOf(entry, Keys::module),
                    .exported = true
                };
                const std::size_t file = fileOf(entry, variable.module);
                variable.field = fieldOf(entry, file, false);
                variable.declaration = textOf(entry, Keys::declaration);
                m_surface.addVariable(std::move(variable));
            }
        }

        std::size_t Reader::fileOf(const Dom::Section& entry, const std::wstring& module)
        {
            const std::wstring relative = textOf(entry, Keys::file);
            const std::size_t root = numberOf(entry, Keys::root);
            const auto found = m_files.find({ root, relative });
            if (found != m_files.end())
                return found->second;
            const std::size_t index = m_surface.addFile({
                .path = std::filesystem::path{ relative },
                .relative = relative,
                .module = module,
                .root = root
            });
            m_files[{ root, relative }] = index;
            return index;
        }

        Place Reader::placeOf(const Dom::Section& entry, const std::size_t file)
        {
            return { .file = file, .line = numberOf(entry, Keys::line) };
        }

        Comment Reader::commentOf(const Dom::Section& entry)
        {
            Comment comment{ .text = textOf(entry, Keys::hint) };
            const std::wstring note = textOf(entry, Keys::note);
            if (note.empty())
                return comment;
            const std::size_t hash = note.find(L'#');
            comment.reference = Reference{
                .note = note.substr(0, hash),
                .anchor = hash == std::wstring::npos ? std::wstring{} : note.substr(hash + 1)
            };
            return comment;
        }

        Access Reader::accessIn(const Dom::Section& entry)
        {
            return accessOf(textOf(entry, Keys::access)).value_or(Access::Public);
        }

        Function Reader::functionOf(const Dom::Section& entry, const std::size_t file,
            const bool member)
        {
            Function function{
                .name = textOf(entry, Keys::name),
                .kind = functionKindOf(textOf(entry, Keys::kind)).value_or(FunctionKind::Function),
                .signature = textOf(entry, Keys::signature),
                .returnType = textOf(entry, Keys::type),
                .templateParameters = textOf(entry, Keys::templateParameters),
                .place = placeOf(entry, file),
                .comment = commentOf(entry)
            };
            if (member)
            {
                function.isStatic = flagOf(entry, Keys::isStatic);
                function.isVirtual = flagOf(entry, Keys::isVirtual);
                function.isDeleted = flagOf(entry, Keys::isDeleted);
                function.access = accessIn(entry);
            }
            return function;
        }

        Field Reader::fieldOf(const Dom::Section& entry, const std::size_t file, const bool member)
        {
            Field field{
                .name = textOf(entry, Keys::name),
                .type = textOf(entry, Keys::type),
                .value = textOf(entry, Keys::value),
                .isConstant = flagOf(entry, Keys::isConstant),
                .place = placeOf(entry, file),
                .comment = commentOf(entry)
            };
            if (member)
            {
                field.isStatic = flagOf(entry, Keys::isStatic);
                field.access = accessIn(entry);
            }
            return field;
        }

        // The surface a document states, read the one way whether it was loaded or just built.
        [[nodiscard]] Surface surfaceOf(const Dom::Section& root)
        {
            Surface surface;
            surface.setScanned(stampOf(textOf(root, Keys::scanned)));
            surface.setMode(flagOf(root, Keys::limited) ? ScanMode::Limited : ScanMode::Full);
            surface.setHintRules(flagOf(root, Keys::looseHints)
                ? HintRules::Loose
                : HintRules::Strict);
            Reader reader{ surface };
            reader.readModules(itemsOf(root, Keys::modules));
            reader.readTypes(itemsOf(root, Keys::types));
            reader.readFunctions(itemsOf(root, Keys::functions));
            reader.readConstants(itemsOf(root, Keys::constants));
            return surface;
        }
    }

    Dom::Dt::Section databaseLayout()
    {
        using namespace Dom::Dt;
        return {
            Value{ Keys::scanned, std::wstring{} },
            Value{ Keys::limited, false },
            Value{ Keys::looseHints, false },
            Sequence{ Keys::modules, moduleLayout() },
            Sequence{ Keys::types, typeLayout() },
            Sequence
            {
                Keys::functions,
                Section
                {
                    placedEntry(),
                    scopedEntry(),
                    Value{ Keys::kind, std::wstring{} },
                    Value{ Keys::signature, std::wstring{} },
                    Value{ Keys::type, std::wstring{} },
                    Value{ Keys::templateParameters, std::wstring{} }
                }
            },
            Sequence
            {
                Keys::constants,
                Section
                {
                    placedEntry(),
                    scopedEntry(),
                    Value{ Keys::type, std::wstring{} },
                    Value{ Keys::value, std::wstring{} },
                    Value{ Keys::isConstant, false },
                    Value{ Keys::declaration, std::wstring{} }
                }
            }
        };
    }

    // **Every entry is built out of what it states alone**, a key at its default never made, so
    // the document goes to the file as built, with nothing cloned or compared after the fact.
    Database::Database(const Surface& surface, ScanSteps* steps)
        :
        m_steps{ steps }
    {
        StepClock clock{ m_steps };
        Dom::Section layout{ nullptr };
        databaseLayout().apply(layout);
        const KeyOrders orders = keyOrdersOf(layout);

        Entry root{ orders.root };
        root.setText(Keys::scanned, stampText(surface.scanned()));
        root.setFlag(Keys::limited, surface.mode() == ScanMode::Limited);
        root.setFlag(Keys::looseHints, surface.hintRules() == HintRules::Loose);

        Items modules;
        for (const Module& module : surface.modules())
            modules.push_back(moduleEntry(orders, surface, module));
        root.setItems(Keys::modules, std::move(modules));

        Items types;
        for (const Type& type : surface.types())
        {
            if (type.isPublic())
                types.push_back(typeEntry(orders, surface, type));
        }
        root.setItems(Keys::types, std::move(types));

        Items functions;
        for (const FreeFunction& free : surface.functions())
        {
            Entry entry{ orders.function };
            setPlaced(entry, free.function.name, free.function.place, free.function.comment);
            setScoped(entry, surface, free.nameSpace, free.module, free.function.place);
            entry.setText(Keys::kind, kindWord(free.function.kind));
            entry.setText(Keys::signature, free.function.signature);
            entry.setText(Keys::type, free.function.returnType);
            entry.setText(Keys::templateParameters, free.function.templateParameters);
            functions.push_back(entry.finish());
        }
        root.setItems(Keys::functions, std::move(functions));

        Items constants;
        for (const Variable& variable : surface.variables())
        {
            Entry entry{ orders.constant };
            setPlaced(entry, variable.field.name, variable.field.place, variable.field.comment);
            setScoped(entry, surface, variable.nameSpace, variable.module, variable.field.place);
            entry.setText(Keys::type, variable.field.type);
            entry.setText(Keys::value, variable.field.value);
            entry.setFlag(Keys::isConstant, variable.field.isConstant);
            entry.setText(Keys::declaration, variable.declaration);
            constants.push_back(entry.finish());
        }
        root.setItems(Keys::constants, std::move(constants));

        m_root = root.finish();
        clock.lap(Steps::fillDocument);
    }

    Database::~Database() = default;

    bool Database::save(const std::filesystem::path& path) const
    {
        StepClock clock{ m_steps };
        const Dom::FileFormat::ClaFi format{};
        const bool written = format.saveSectionToFile(*m_root, path);
        clock.lap(Steps::saveDatabase);
        return written;
    }

    Surface Database::surface() const
    {
        StepClock clock{ m_steps };
        Surface surface = surfaceOf(*m_root);
        clock.lap(Steps::readDocument);
        return surface;
    }

    bool writeDatabase(const Surface& surface, const std::filesystem::path& path,
        ScanSteps* steps)
    {
        const Database database{ surface, steps };
        return database.save(path);
    }

    std::optional<Surface> readDatabase(const std::filesystem::path& path, ScanSteps* steps)
    {
        StepClock clock{ steps };
        Dom::Document<Dom::FileFormat::ClaFi> document{
            path,
            Dom::AutoSave::No,
            databaseLayout(),
            Dom::WriteDefaults::No
        };
        if (!document.load())
            return std::nullopt;
        clock.lap(Steps::loadDatabase);
        Surface surface = surfaceOf(document);
        clock.lap(Steps::readDocument);
        return surface;
    }

    std::wstring_view kindWord(const TypeKind kind)
    {
        switch (kind)
        {
            case TypeKind::Class:
                return L"class";
            case TypeKind::Struct:
                return L"struct";
            case TypeKind::Union:
                return L"union";
            case TypeKind::Enum:
                return L"enum";
            case TypeKind::Alias:
                return L"alias";
            case TypeKind::Defined:
                return L"type";
            case TypeKind::Concept:
                return L"concept";
            case TypeKind::Interface:
                return L"interface";
        }
        return {};
    }

    std::wstring_view kindWord(const FunctionKind kind)
    {
        switch (kind)
        {
            case FunctionKind::Function:
                return L"function";
            case FunctionKind::Constructor:
                return L"constructor";
            case FunctionKind::Destructor:
                return L"destructor";
        }
        return {};
    }

    std::wstring_view formWord(const PropertyForm form)
    {
        switch (form)
        {
            case PropertyForm::Declared:
                return L"declared";
            case PropertyForm::Writable:
                return L"writable";
            case PropertyForm::ByReference:
                return L"reference";
            case PropertyForm::Storage:
                return L"storage";
            case PropertyForm::BoundMember:
                return L"member";
            case PropertyForm::BoundCall:
                return L"call";
            case PropertyForm::BoundValue:
                return L"value";
            case PropertyForm::BoundAction:
                return L"action";
            case PropertyForm::Read:
                return L"read";
            case PropertyForm::Required:
                return L"required";
        }
        return {};
    }

    std::wstring_view accessWord(const Access access)
    {
        switch (access)
        {
            case Access::Public:
                return L"public";
            case Access::Protected:
                return L"protected";
            case Access::Private:
                return L"private";
            case Access::Published:
                return L"published";
        }
        return {};
    }

    std::optional<TypeKind> typeKindOf(const std::wstring_view word)
    {
        for (const TypeKind kind : { TypeKind::Class, TypeKind::Struct, TypeKind::Union,
            TypeKind::Enum, TypeKind::Alias, TypeKind::Defined, TypeKind::Concept,
            TypeKind::Interface })
        {
            if (kindWord(kind) == word)
                return kind;
        }
        return std::nullopt;
    }

    std::optional<FunctionKind> functionKindOf(const std::wstring_view word)
    {
        for (const FunctionKind kind : { FunctionKind::Function, FunctionKind::Constructor,
            FunctionKind::Destructor })
        {
            if (kindWord(kind) == word)
                return kind;
        }
        return std::nullopt;
    }

    std::optional<PropertyForm> formOf(const std::wstring_view word)
    {
        for (const PropertyForm form : { PropertyForm::Declared, PropertyForm::Writable,
            PropertyForm::ByReference, PropertyForm::Storage, PropertyForm::BoundMember,
            PropertyForm::BoundCall, PropertyForm::BoundValue, PropertyForm::BoundAction,
            PropertyForm::Read, PropertyForm::Required })
        {
            if (formWord(form) == word)
                return form;
        }
        return std::nullopt;
    }

    std::optional<Access> accessOf(const std::wstring_view word)
    {
        for (const Access access : { Access::Public, Access::Protected, Access::Private,
            Access::Published })
        {
            if (accessWord(access) == word)
                return access;
        }
        return std::nullopt;
    }
}
