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
            constexpr std::wstring_view FillDocument = L"fill document";
            constexpr std::wstring_view SaveDatabase = L"save database";
            constexpr std::wstring_view LoadDatabase = L"load database";
            constexpr std::wstring_view ReadDocument = L"read document";
        }

        namespace Keys
        {
            constexpr std::wstring_view Modules = L"Modules";
            constexpr std::wstring_view Types = L"Types";
            constexpr std::wstring_view Functions = L"Functions";
            constexpr std::wstring_view Constants = L"Constants";
            constexpr std::wstring_view Scanned = L"Scanned";
            constexpr std::wstring_view ReadAs = L"ReadAs";
            constexpr std::wstring_view LooseHints = L"LooseHints";
            constexpr std::wstring_view Name = L"Name";
            constexpr std::wstring_view NameSpace = L"Namespace";
            constexpr std::wstring_view Kind = L"Kind";
            constexpr std::wstring_view Module = L"Module";
            constexpr std::wstring_view File = L"File";
            constexpr std::wstring_view Root = L"Root";
            constexpr std::wstring_view Line = L"Line";
            constexpr std::wstring_view Hint = L"Hint";
            constexpr std::wstring_view Note = L"Note";
            constexpr std::wstring_view Interface = L"Interface";
            constexpr std::wstring_view Header = L"Header";
            constexpr std::wstring_view Language = L"Language";
            constexpr std::wstring_view Imports = L"Imports";
            constexpr std::wstring_view ExportedImports = L"ExportedImports";
            constexpr std::wstring_view TemplateParameters = L"Template";
            constexpr std::wstring_view Bases = L"Bases";
            constexpr std::wstring_view Base = L"Base";
            constexpr std::wstring_view Target = L"Target";
            constexpr std::wstring_view Control = L"Control";
            constexpr std::wstring_view Scoped = L"Scoped";
            constexpr std::wstring_view Folder = L"Folder";
            constexpr std::wstring_view Properties = L"Properties";
            constexpr std::wstring_view Events = L"Events";
            constexpr std::wstring_view Methods = L"Methods";
            constexpr std::wstring_view Usings = L"Usings";
            constexpr std::wstring_view Fields = L"Fields";
            constexpr std::wstring_view Members = L"Members";
            constexpr std::wstring_view Restates = L"Restates";
            constexpr std::wstring_view Fixed = L"Fixed";
            constexpr std::wstring_view Type = L"Type";
            constexpr std::wstring_view Accepts = L"Accepts";
            constexpr std::wstring_view DefaultValue = L"Default";
            constexpr std::wstring_view Setter = L"Setter";
            constexpr std::wstring_view Form = L"Form";
            constexpr std::wstring_view ValueKind = L"ValueKind";
            constexpr std::wstring_view Optional = L"Optional";
            constexpr std::wstring_view Access = L"Access";
            constexpr std::wstring_view Alias = L"Alias";
            constexpr std::wstring_view Method = L"Method";
            constexpr std::wstring_view Signature = L"Signature";
            constexpr std::wstring_view IsStatic = L"Static";
            constexpr std::wstring_view IsVirtual = L"Virtual";
            constexpr std::wstring_view IsDeleted = L"Deleted";
            constexpr std::wstring_view IsConstant = L"Constant";
            constexpr std::wstring_view Value = L"Value";
            constexpr std::wstring_view Declaration = L"Declaration";
        }

        using Sections = Dom::Sequence<Dom::Section>;
        using Strings = Dom::Sequence<std::wstring>;

        [[nodiscard]] Dom::Dt::Section placedEntry()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::Name, std::wstring{} },
                Value{ Keys::Line, std::size_t{ 0 } },
                Value{ Keys::Hint, std::wstring{} },
                Value{ Keys::Note, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section memberEntry()
        {
            using namespace Dom::Dt;
            return {
                placedEntry(),
                Value{ Keys::Access, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section propertyLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::Type, std::wstring{} },
                Sequence{ Keys::Accepts, std::wstring{} },
                Value{ Keys::DefaultValue, std::wstring{} },
                Value{ Keys::Setter, std::wstring{} },
                Value{ Keys::Target, std::wstring{} },
                Value{ Keys::Form, std::wstring{} },
                Value{ Keys::ValueKind, std::wstring{} },
                Value{ Keys::Optional, false }
            };
        }

        [[nodiscard]] Dom::Dt::Section eventLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::Alias, std::wstring{} },
                Value{ Keys::Method, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section functionLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::Kind, std::wstring{} },
                Value{ Keys::Signature, std::wstring{} },
                Value{ Keys::Type, std::wstring{} },
                Value{ Keys::TemplateParameters, std::wstring{} },
                Value{ Keys::IsStatic, false },
                Value{ Keys::IsVirtual, false },
                Value{ Keys::IsDeleted, false }
            };
        }

        [[nodiscard]] Dom::Dt::Section usingLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::Base, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section fieldLayout()
        {
            using namespace Dom::Dt;
            return {
                memberEntry(),
                Value{ Keys::Type, std::wstring{} },
                Value{ Keys::Value, std::wstring{} },
                Value{ Keys::IsStatic, false },
                Value{ Keys::IsConstant, false }
            };
        }

        [[nodiscard]] Dom::Dt::Section enumMemberLayout()
        {
            using namespace Dom::Dt;
            return {
                placedEntry(),
                Value{ Keys::Value, std::wstring{} }
            };
        }

        [[nodiscard]] Dom::Dt::Section restatementLayout()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::Type, std::wstring{} },
                Value{ Keys::Value, std::wstring{} },
                Value{ Keys::Fixed, false },
                Value{ Keys::Line, std::size_t{ 0 } }
            };
        }

        // What an entry at namespace scope states beyond a member.
        [[nodiscard]] Dom::Dt::Section scopedEntry()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::NameSpace, std::wstring{} },
                Value{ Keys::Module, std::wstring{} },
                Value{ Keys::File, std::wstring{} },
                Value{ Keys::Root, std::size_t{ 0 } }
            };
        }

        [[nodiscard]] Dom::Dt::Section typeLayout()
        {
            using namespace Dom::Dt;
            return {
                placedEntry(),
                scopedEntry(),
                Value{ Keys::Kind, std::wstring{} },
                Value{ Keys::TemplateParameters, std::wstring{} },
                Sequence{ Keys::Bases, std::wstring{} },
                Value{ Keys::Target, std::wstring{} },
                Value{ Keys::Scoped, false },
                Value{ Keys::Control, false },
                Value{ Keys::Folder, std::wstring{} },
                Sequence{ Keys::Properties, propertyLayout() },
                Sequence{ Keys::Events, eventLayout() },
                Sequence{ Keys::Methods, functionLayout() },
                Sequence{ Keys::Usings, usingLayout() },
                Sequence{ Keys::Fields, fieldLayout() },
                Sequence{ Keys::Members, enumMemberLayout() },
                Sequence{ Keys::Restates, restatementLayout() }
            };
        }

        [[nodiscard]] Dom::Dt::Section moduleLayout()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::Name, std::wstring{} },
                Value{ Keys::File, std::wstring{} },
                Value{ Keys::Root, std::size_t{ 0 } },
                Value{ Keys::Interface, false },
                Value{ Keys::Header, false },
                Value{ Keys::Language, std::wstring{} },
                Sequence{ Keys::Imports, std::wstring{} },
                Sequence{ Keys::ExportedImports, std::wstring{} }
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
            Dom::Section& type = (layout / Keys::Types).as<Sections>().add();
            return {
                .root = keyOrderOf(layout),
                .module = keyOrderOf((layout / Keys::Modules).as<Sections>().add()),
                .type = keyOrderOf(type),
                .property = keyOrderOf((type / Keys::Properties).as<Sections>().add()),
                .event = keyOrderOf((type / Keys::Events).as<Sections>().add()),
                .method = keyOrderOf((type / Keys::Methods).as<Sections>().add()),
                .usingDeclaration = keyOrderOf((type / Keys::Usings).as<Sections>().add()),
                .field = keyOrderOf((type / Keys::Fields).as<Sections>().add()),
                .member = keyOrderOf((type / Keys::Members).as<Sections>().add()),
                .restatement = keyOrderOf((type / Keys::Restates).as<Sections>().add()),
                .function = keyOrderOf((layout / Keys::Functions).as<Sections>().add()),
                .constant = keyOrderOf((layout / Keys::Constants).as<Sections>().add())
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
            entry.setText(Keys::File, file.relative);
            entry.setNumber(Keys::Root, file.root);
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
            entry.setText(Keys::Name, name);
            entry.setNumber(Keys::Line, place.line);
            entry.setText(Keys::Hint, comment.text);
            entry.setText(Keys::Note, noteText(comment));
        }

        void setScoped(Entry& entry, const Surface& surface, const std::wstring_view nameSpace,
            const std::wstring_view module, const Place& place)
        {
            entry.setText(Keys::NameSpace, nameSpace);
            entry.setText(Keys::Module, module);
            setFile(entry, surface.files()[place.file]);
        }

        void setFunction(Entry& entry, const Function& function)
        {
            setPlaced(entry, function.name, function.place, function.comment);
            entry.setText(Keys::Access, accessWord(function.access));
            entry.setText(Keys::Kind, kindWord(function.kind));
            entry.setText(Keys::Signature, function.signature);
            entry.setText(Keys::Type, function.returnType);
            entry.setText(Keys::TemplateParameters, function.templateParameters);
            entry.setFlag(Keys::IsStatic, function.isStatic);
            entry.setFlag(Keys::IsVirtual, function.isVirtual);
            entry.setFlag(Keys::IsDeleted, function.isDeleted);
        }

        void setField(Entry& entry, const Field& field)
        {
            setPlaced(entry, field.name, field.place, field.comment);
            entry.setText(Keys::Access, accessWord(field.access));
            entry.setText(Keys::Type, field.type);
            entry.setText(Keys::Value, field.value);
            entry.setFlag(Keys::IsStatic, field.isStatic);
            entry.setFlag(Keys::IsConstant, field.isConstant);
        }

        [[nodiscard]] std::unique_ptr<Dom::Section> typeEntry(const KeyOrders& orders,
            const Surface& surface, const Type& type)
        {
            Entry entry{ orders.type };
            setPlaced(entry, type.name, type.place, type.comment);
            setScoped(entry, surface, type.nameSpace, type.module, type.place);
            entry.setText(Keys::Kind, kindWord(type.kind));
            entry.setText(Keys::TemplateParameters, type.templateParameters);
            Names bases;
            for (const std::wstring& base : type.bases)
            {
                bases.push_back(isTemplateParameter(type, base)
                    ? base
                    : surface.resolvedBase(base, type.enclosingScope()));
            }
            entry.setList(Keys::Bases, bases);
            entry.setText(Keys::Target, type.target);
            entry.setFlag(Keys::Scoped, type.scoped);
            entry.setFlag(Keys::Control, type.isControl);
            entry.setText(Keys::Folder, type.folder);

            Items properties;
            for (const Property& property : surface.mergedProperties(type))
            {
                if (property.access == Access::Private)
                    continue;
                Entry item{ orders.property };
                setPlaced(item, property.name, property.place, property.comment);
                item.setText(Keys::Access, accessWord(property.access));
                item.setText(Keys::Type, property.type);
                item.setList(Keys::Accepts, property.accepts);
                item.setText(Keys::DefaultValue, property.defaultValue);
                item.setText(Keys::Setter, property.setter);
                item.setText(Keys::Target, property.target);
                item.setText(Keys::Form, formWord(property.form));
                item.setText(Keys::ValueKind, property.valueKind);
                item.setFlag(Keys::Optional, property.optional);
                properties.push_back(item.finish());
            }
            entry.setItems(Keys::Properties, std::move(properties));

            Items events;
            for (const Event& event : type.events)
            {
                if (event.access == Access::Private)
                    continue;
                Entry item{ orders.event };
                setPlaced(item, event.type, event.place, event.comment);
                item.setText(Keys::Access, accessWord(event.access));
                item.setText(Keys::Alias, event.alias);
                item.setText(Keys::Method, event.method);
                events.push_back(item.finish());
            }
            entry.setItems(Keys::Events, std::move(events));

            Items methods;
            for (const Function& function : type.functions)
            {
                if (function.access == Access::Private)
                    continue;
                Entry item{ orders.method };
                setFunction(item, function);
                methods.push_back(item.finish());
            }
            entry.setItems(Keys::Methods, std::move(methods));

            Items usings;
            for (const Using& declaration : type.usings)
            {
                if (declaration.access == Access::Private)
                    continue;
                Entry item{ orders.usingDeclaration };
                setPlaced(item, declaration.name, declaration.place, declaration.comment);
                item.setText(Keys::Access, accessWord(declaration.access));
                item.setText(Keys::Base, declaration.base);
                usings.push_back(item.finish());
            }
            entry.setItems(Keys::Usings, std::move(usings));

            Items fields;
            for (const Field& field : type.fields)
            {
                if (field.access == Access::Private)
                    continue;
                Entry item{ orders.field };
                setField(item, field);
                fields.push_back(item.finish());
            }
            entry.setItems(Keys::Fields, std::move(fields));

            Items members;
            for (const EnumMember& member : type.members)
            {
                Entry item{ orders.member };
                setPlaced(item, member.name, member.place, member.comment);
                item.setText(Keys::Value, member.value);
                members.push_back(item.finish());
            }
            entry.setItems(Keys::Members, std::move(members));

            Items restates;
            for (const Restatement& restatement : type.restatements)
            {
                Entry item{ orders.restatement };
                item.setText(Keys::Type, restatement.type);
                item.setText(Keys::Value, restatement.value);
                item.setFlag(Keys::Fixed, restatement.fixed);
                item.setNumber(Keys::Line, restatement.place.line);
                restates.push_back(item.finish());
            }
            entry.setItems(Keys::Restates, std::move(restates));
            return entry.finish();
        }

        [[nodiscard]] std::unique_ptr<Dom::Section> moduleEntry(const KeyOrders& orders,
            const Surface& surface, const Module& module)
        {
            Entry entry{ orders.module };
            entry.setText(Keys::Name, module.name);
            setFile(entry, surface.files()[module.file]);
            entry.setFlag(Keys::Interface, module.isInterface);
            entry.setFlag(Keys::Header, module.isHeader);
            if (module.language == SourceLanguage::Pascal)
                entry.setText(Keys::Language, k_pascalWord);
            Names imports;
            Names exported;
            for (const Import& import : module.imports)
                (import.exported ? exported : imports).push_back(import.name);
            entry.setList(Keys::Imports, imports);
            entry.setList(Keys::ExportedImports, exported);
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
                    .name = textOf(entry, Keys::Name),
                    .isInterface = flagOf(entry, Keys::Interface),
                    .isHeader = flagOf(entry, Keys::Header),
                    .language = textOf(entry, Keys::Language) == k_pascalWord
                        ? SourceLanguage::Pascal
                        : SourceLanguage::Cpp
                };
                module.file = fileOf(entry, module.name);
                for (const std::wstring& name : listOf(entry, Keys::Imports))
                    module.imports.push_back({ .name = name, .exported = false });
                for (const std::wstring& name : listOf(entry, Keys::ExportedImports))
                    module.imports.push_back({ .name = name, .exported = true });
                m_surface.addModule(std::move(module));
            }
        }

        void Reader::readTypes(const Sections& list)
        {
            for (const Dom::Section& entry : list)
            {
                Type type{
                    .name = textOf(entry, Keys::Name),
                    .nameSpace = textOf(entry, Keys::NameSpace),
                    .kind = typeKindOf(textOf(entry, Keys::Kind)).value_or(TypeKind::Class),
                    .module = textOf(entry, Keys::Module),
                    .templateParameters = textOf(entry, Keys::TemplateParameters),
                    .bases = listOf(entry, Keys::Bases),
                    .target = textOf(entry, Keys::Target),
                    .scoped = flagOf(entry, Keys::Scoped),
                    .exported = true,
                    .isControl = flagOf(entry, Keys::Control),
                    .folder = textOf(entry, Keys::Folder)
                };
                type.qualifiedName = qualifiedNameOf(type.nameSpace, type.name);
                const std::size_t file = fileOf(entry, type.module);
                type.place = placeOf(entry, file);
                type.comment = commentOf(entry);

                for (const Dom::Section& item : itemsOf(entry, Keys::Properties))
                {
                    type.properties.push_back({
                        .name = textOf(item, Keys::Name),
                        .type = textOf(item, Keys::Type),
                        .accepts = listOf(item, Keys::Accepts),
                        .defaultValue = textOf(item, Keys::DefaultValue),
                        .setter = textOf(item, Keys::Setter),
                        .target = textOf(item, Keys::Target),
                        .form = formOf(textOf(item, Keys::Form)).value_or(PropertyForm::Declared),
                        .valueKind = textOf(item, Keys::ValueKind),
                        .optional = flagOf(item, Keys::Optional),
                        .access = accessIn(item),
                        .place = placeOf(item, file),
                        .comment = commentOf(item)
                    });
                }
                for (const Dom::Section& item : itemsOf(entry, Keys::Events))
                {
                    type.events.push_back({
                        .type = textOf(item, Keys::Name),
                        .alias = textOf(item, Keys::Alias),
                        .method = textOf(item, Keys::Method),
                        .access = accessIn(item),
                        .place = placeOf(item, file),
                        .comment = commentOf(item)
                    });
                }
                for (const Dom::Section& item : itemsOf(entry, Keys::Methods))
                    type.functions.push_back(functionOf(item, file, true));
                for (const Dom::Section& item : itemsOf(entry, Keys::Usings))
                {
                    type.usings.push_back({
                        .base = textOf(item, Keys::Base),
                        .name = textOf(item, Keys::Name),
                        .access = accessIn(item),
                        .place = placeOf(item, file),
                        .comment = commentOf(item)
                    });
                }
                for (const Dom::Section& item : itemsOf(entry, Keys::Fields))
                    type.fields.push_back(fieldOf(item, file, true));
                for (const Dom::Section& item : itemsOf(entry, Keys::Members))
                {
                    type.members.push_back({
                        .name = textOf(item, Keys::Name),
                        .value = textOf(item, Keys::Value),
                        .place = placeOf(item, file),
                        .comment = commentOf(item)
                    });
                }
                for (const Dom::Section& item : itemsOf(entry, Keys::Restates))
                {
                    type.restatements.push_back({
                        .type = textOf(item, Keys::Type),
                        .value = textOf(item, Keys::Value),
                        .fixed = flagOf(item, Keys::Fixed),
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
                    .nameSpace = textOf(entry, Keys::NameSpace),
                    .module = textOf(entry, Keys::Module),
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
                    .nameSpace = textOf(entry, Keys::NameSpace),
                    .module = textOf(entry, Keys::Module),
                    .exported = true
                };
                const std::size_t file = fileOf(entry, variable.module);
                variable.field = fieldOf(entry, file, false);
                variable.declaration = textOf(entry, Keys::Declaration);
                m_surface.addVariable(std::move(variable));
            }
        }

        std::size_t Reader::fileOf(const Dom::Section& entry, const std::wstring& module)
        {
            const std::wstring relative = textOf(entry, Keys::File);
            const std::size_t root = numberOf(entry, Keys::Root);
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
            return { .file = file, .line = numberOf(entry, Keys::Line) };
        }

        Comment Reader::commentOf(const Dom::Section& entry)
        {
            Comment comment{ .text = textOf(entry, Keys::Hint) };
            const std::wstring note = textOf(entry, Keys::Note);
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
            return accessOf(textOf(entry, Keys::Access)).value_or(Access::Public);
        }

        Function Reader::functionOf(const Dom::Section& entry, const std::size_t file,
            const bool member)
        {
            Function function{
                .name = textOf(entry, Keys::Name),
                .kind = functionKindOf(textOf(entry, Keys::Kind)).value_or(FunctionKind::Function),
                .signature = textOf(entry, Keys::Signature),
                .returnType = textOf(entry, Keys::Type),
                .templateParameters = textOf(entry, Keys::TemplateParameters),
                .place = placeOf(entry, file),
                .comment = commentOf(entry)
            };
            if (member)
            {
                function.isStatic = flagOf(entry, Keys::IsStatic);
                function.isVirtual = flagOf(entry, Keys::IsVirtual);
                function.isDeleted = flagOf(entry, Keys::IsDeleted);
                function.access = accessIn(entry);
            }
            return function;
        }

        Field Reader::fieldOf(const Dom::Section& entry, const std::size_t file, const bool member)
        {
            Field field{
                .name = textOf(entry, Keys::Name),
                .type = textOf(entry, Keys::Type),
                .value = textOf(entry, Keys::Value),
                .isConstant = flagOf(entry, Keys::IsConstant),
                .place = placeOf(entry, file),
                .comment = commentOf(entry)
            };
            if (member)
            {
                field.isStatic = flagOf(entry, Keys::IsStatic);
                field.access = accessIn(entry);
            }
            return field;
        }

        // The surface a document states, read the one way whether it was loaded or just built.
        [[nodiscard]] Surface surfaceOf(const Dom::Section& root)
        {
            Surface surface;
            surface.setScanned(stampOf(textOf(root, Keys::Scanned)));
            surface.setMode(scanModeOf(textOf(root, Keys::ReadAs))
                .value_or(ScanMode::AnyCppAndDelphi));
            surface.setHintRules(flagOf(root, Keys::LooseHints)
                ? HintRules::Loose
                : HintRules::Strict);
            Reader reader{ surface };
            reader.readModules(itemsOf(root, Keys::Modules));
            reader.readTypes(itemsOf(root, Keys::Types));
            reader.readFunctions(itemsOf(root, Keys::Functions));
            reader.readConstants(itemsOf(root, Keys::Constants));
            return surface;
        }
    }

    Dom::Dt::Section databaseLayout()
    {
        using namespace Dom::Dt;
        return {
            Value{ Keys::Scanned, std::wstring{} },
            Value{ Keys::ReadAs, std::wstring{} },
            Value{ Keys::LooseHints, false },
            Sequence{ Keys::Modules, moduleLayout() },
            Sequence{ Keys::Types, typeLayout() },
            Sequence
            {
                Keys::Functions,
                Section
                {
                    placedEntry(),
                    scopedEntry(),
                    Value{ Keys::Kind, std::wstring{} },
                    Value{ Keys::Signature, std::wstring{} },
                    Value{ Keys::Type, std::wstring{} },
                    Value{ Keys::TemplateParameters, std::wstring{} }
                }
            },
            Sequence
            {
                Keys::Constants,
                Section
                {
                    placedEntry(),
                    scopedEntry(),
                    Value{ Keys::Type, std::wstring{} },
                    Value{ Keys::Value, std::wstring{} },
                    Value{ Keys::IsConstant, false },
                    Value{ Keys::Declaration, std::wstring{} }
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
        root.setText(Keys::Scanned, stampText(surface.scanned()));
        if (surface.mode() != ScanMode::AnyCppAndDelphi)
            root.setText(Keys::ReadAs, modeWord(surface.mode()));
        root.setFlag(Keys::LooseHints, surface.hintRules() == HintRules::Loose);

        Items modules;
        for (const Module& module : surface.modules())
            modules.push_back(moduleEntry(orders, surface, module));
        root.setItems(Keys::Modules, std::move(modules));

        Items types;
        for (const Type& type : surface.types())
        {
            if (type.isPublic())
                types.push_back(typeEntry(orders, surface, type));
        }
        root.setItems(Keys::Types, std::move(types));

        Items functions;
        for (const FreeFunction& free : surface.functions())
        {
            Entry entry{ orders.function };
            setPlaced(entry, free.function.name, free.function.place, free.function.comment);
            setScoped(entry, surface, free.nameSpace, free.module, free.function.place);
            entry.setText(Keys::Kind, kindWord(free.function.kind));
            entry.setText(Keys::Signature, free.function.signature);
            entry.setText(Keys::Type, free.function.returnType);
            entry.setText(Keys::TemplateParameters, free.function.templateParameters);
            functions.push_back(entry.finish());
        }
        root.setItems(Keys::Functions, std::move(functions));

        Items constants;
        for (const Variable& variable : surface.variables())
        {
            Entry entry{ orders.constant };
            setPlaced(entry, variable.field.name, variable.field.place, variable.field.comment);
            setScoped(entry, surface, variable.nameSpace, variable.module, variable.field.place);
            entry.setText(Keys::Type, variable.field.type);
            entry.setText(Keys::Value, variable.field.value);
            entry.setFlag(Keys::IsConstant, variable.field.isConstant);
            entry.setText(Keys::Declaration, variable.declaration);
            constants.push_back(entry.finish());
        }
        root.setItems(Keys::Constants, std::move(constants));

        m_root = root.finish();
        clock.lap(Steps::FillDocument);
    }

    Database::~Database() = default;

    bool Database::save(const std::filesystem::path& path) const
    {
        StepClock clock{ m_steps };
        const Dom::FileFormat::ClaFi format{};
        const bool written = format.saveSectionToFile(*m_root, path);
        clock.lap(Steps::SaveDatabase);
        return written;
    }

    Surface Database::surface() const
    {
        StepClock clock{ m_steps };
        Surface surface = surfaceOf(*m_root);
        clock.lap(Steps::ReadDocument);
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
        clock.lap(Steps::LoadDatabase);
        Surface surface = surfaceOf(document);
        clock.lap(Steps::ReadDocument);
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

    std::wstring_view modeWord(const ScanMode mode)
    {
        switch (mode)
        {
            case ScanMode::AnyCppAndDelphi:
                return L"AnyCppAndDelphi";
            case ScanMode::ClaFiModules:
                return L"ClaFiModules";
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

    std::optional<ScanMode> scanModeOf(const std::wstring_view word)
    {
        for (const ScanMode mode : { ScanMode::AnyCppAndDelphi, ScanMode::ClaFiModules })
        {
            if (modeWord(mode) == word)
                return mode;
        }
        return std::nullopt;
    }
}
