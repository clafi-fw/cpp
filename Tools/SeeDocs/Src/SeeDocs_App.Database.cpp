module SeeDocs_App.Database;

import SeeDocs_App.Surface;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// The std::wstring serializer: without it a node's set reads a string as a sequence.
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        namespace Keys
        {
            constexpr std::wstring_view modules = L"Modules";
            constexpr std::wstring_view types = L"Types";
            constexpr std::wstring_view functions = L"Functions";
            constexpr std::wstring_view constants = L"Constants";
            constexpr std::wstring_view name = L"Name";
            constexpr std::wstring_view nameSpace = L"Namespace";
            constexpr std::wstring_view kind = L"Kind";
            constexpr std::wstring_view module = L"Module";
            constexpr std::wstring_view file = L"File";
            constexpr std::wstring_view line = L"Line";
            constexpr std::wstring_view hint = L"Hint";
            constexpr std::wstring_view note = L"Note";
            constexpr std::wstring_view interface = L"Interface";
            constexpr std::wstring_view imports = L"Imports";
            constexpr std::wstring_view exportedImports = L"ExportedImports";
            constexpr std::wstring_view templateParameters = L"Template";
            constexpr std::wstring_view bases = L"Bases";
            constexpr std::wstring_view target = L"Target";
            constexpr std::wstring_view control = L"Control";
            constexpr std::wstring_view category = L"Category";
            constexpr std::wstring_view properties = L"Properties";
            constexpr std::wstring_view events = L"Events";
            constexpr std::wstring_view methods = L"Methods";
            constexpr std::wstring_view fields = L"Fields";
            constexpr std::wstring_view members = L"Members";
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

        // What an entry at namespace scope states beyond a member.
        [[nodiscard]] Dom::Dt::Section scopedEntry()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::nameSpace, std::wstring{} },
                Value{ Keys::module, std::wstring{} },
                Value{ Keys::file, std::wstring{} }
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
                Value{ Keys::control, false },
                Value{ Keys::category, std::wstring{} },
                Sequence{ Keys::properties, propertyLayout() },
                Sequence{ Keys::events, eventLayout() },
                Sequence{ Keys::methods, functionLayout() },
                Sequence{ Keys::fields, fieldLayout() },
                Sequence{ Keys::members, enumMemberLayout() }
            };
        }

        [[nodiscard]] Dom::Dt::Section moduleLayout()
        {
            using namespace Dom::Dt;
            return {
                Value{ Keys::name, std::wstring{} },
                Value{ Keys::file, std::wstring{} },
                Value{ Keys::interface, false },
                Sequence{ Keys::imports, std::wstring{} },
                Sequence{ Keys::exportedImports, std::wstring{} }
            };
        }

        void setText(Dom::Section& section, const std::wstring_view key,
            const std::wstring_view text)
        {
            (section / key).set(std::wstring{ text });
        }

        void setFlag(Dom::Section& section, const std::wstring_view key, const bool value)
        {
            (section / key).set(value);
        }

        void setNumber(Dom::Section& section, const std::wstring_view key, const std::size_t value)
        {
            (section / key).set(value);
        }

        void setList(Dom::Section& section, const std::wstring_view key, const Names& names)
        {
            Strings& list = (section / key).as<Strings>();
            for (const std::wstring& name : names)
                list.add().set(name);
        }

        [[nodiscard]] std::wstring noteText(const Comment& comment)
        {
            if (!comment.reference)
                return {};
            return comment.reference->note + L"#" + comment.reference->anchor;
        }

        void setPlaced(Dom::Section& section, const std::wstring_view name, const Place& place,
            const Comment& comment)
        {
            setText(section, Keys::name, name);
            setNumber(section, Keys::line, place.line);
            setText(section, Keys::hint, comment.text);
            setText(section, Keys::note, noteText(comment));
        }

        void setScoped(Dom::Section& section, const Surface& surface,
            const std::wstring_view nameSpace, const std::wstring_view module, const Place& place)
        {
            setText(section, Keys::nameSpace, nameSpace);
            setText(section, Keys::module, module);
            setText(section, Keys::file, surface.files()[place.file].relative);
        }

        void setFunction(Dom::Section& section, const Function& function)
        {
            setPlaced(section, function.name, function.place, function.comment);
            setText(section, Keys::access, accessWord(function.access));
            setText(section, Keys::kind, kindWord(function.kind));
            setText(section, Keys::signature, function.signature);
            setText(section, Keys::type, function.returnType);
            setText(section, Keys::templateParameters, function.templateParameters);
            setFlag(section, Keys::isStatic, function.isStatic);
            setFlag(section, Keys::isVirtual, function.isVirtual);
            setFlag(section, Keys::isDeleted, function.isDeleted);
        }

        void setField(Dom::Section& section, const Field& field)
        {
            setPlaced(section, field.name, field.place, field.comment);
            setText(section, Keys::access, accessWord(field.access));
            setText(section, Keys::type, field.type);
            setText(section, Keys::value, field.value);
            setFlag(section, Keys::isStatic, field.isStatic);
            setFlag(section, Keys::isConstant, field.isConstant);
        }

        void writeType(Dom::Section& section, const Surface& surface, const Type& type)
        {
            setPlaced(section, type.name, type.place, type.comment);
            setScoped(section, surface, type.nameSpace, type.module, type.place);
            setText(section, Keys::kind, kindWord(type.kind));
            setText(section, Keys::templateParameters, type.templateParameters);
            setList(section, Keys::bases, type.bases);
            setText(section, Keys::target, type.target);
            setFlag(section, Keys::control, type.isControl);
            setText(section, Keys::category, type.category);

            Sections& properties = (section / Keys::properties).as<Sections>();
            for (const Property& property : surface.mergedProperties(type))
            {
                if (property.access == Access::Private)
                    continue;
                Dom::Section& entry = properties.add();
                setPlaced(entry, property.name, property.place, property.comment);
                setText(entry, Keys::access, accessWord(property.access));
                setText(entry, Keys::type, property.type);
                setList(entry, Keys::accepts, property.accepts);
                setText(entry, Keys::defaultValue, property.defaultValue);
                setText(entry, Keys::setter, property.setter);
                setText(entry, Keys::target, property.target);
                setText(entry, Keys::form, formWord(property.form));
                setText(entry, Keys::valueKind, property.valueKind);
                setFlag(entry, Keys::optional, property.optional);
            }

            Sections& events = (section / Keys::events).as<Sections>();
            for (const Event& event : type.events)
            {
                if (event.access == Access::Private)
                    continue;
                Dom::Section& entry = events.add();
                setPlaced(entry, event.type, event.place, event.comment);
                setText(entry, Keys::access, accessWord(event.access));
                setText(entry, Keys::alias, event.alias);
                setText(entry, Keys::method, event.method);
            }

            Sections& methods = (section / Keys::methods).as<Sections>();
            for (const Function& function : type.functions)
            {
                if (function.access == Access::Private)
                    continue;
                setFunction(methods.add(), function);
            }

            Sections& fields = (section / Keys::fields).as<Sections>();
            for (const Field& field : type.fields)
            {
                if (field.access == Access::Private)
                    continue;
                setField(fields.add(), field);
            }

            Sections& members = (section / Keys::members).as<Sections>();
            for (const EnumMember& member : type.members)
            {
                Dom::Section& entry = members.add();
                setPlaced(entry, member.name, member.place, member.comment);
                setText(entry, Keys::value, member.value);
            }
        }

        void writeModule(Dom::Section& section, const Surface& surface, const Module& module)
        {
            setText(section, Keys::name, module.name);
            setText(section, Keys::file, surface.files()[module.file].relative);
            setFlag(section, Keys::interface, module.isInterface);
            Names imports;
            Names exported;
            for (const Import& import : module.imports)
                (import.exported ? exported : imports).push_back(import.name);
            setList(section, Keys::imports, imports);
            setList(section, Keys::exportedImports, exported);
        }

        [[nodiscard]] bool isSectionLike(const Dom::DomNodeType type)
        {
            return type == Dom::DomNodeType::Section || type == Dom::DomNodeType::CompositeValue;
        }

        std::unique_ptr<Dom::Section> compacted(const Dom::Section&, const Dom::Section&);

        // The items of a sequence, each compacted against the fresh item the layout makes.
        [[nodiscard]] std::unique_ptr<Dom::DomNodeBase> compactedSequence(
            const Dom::SequenceBase& sequence, const Dom::SequenceBase& layout)
        {
            std::unique_ptr<Dom::DomNodeBase> blueprintHolder = layout.clone(nullptr);
            const Dom::DomNodeBase& blueprint = blueprintHolder->as<Dom::SequenceBase>().addNode();

            // The layout's sequence is empty and makes the same items, so its clone is the start.
            std::unique_ptr<Dom::DomNodeBase> copy = layout.clone(nullptr);
            Dom::SequenceBase& target = copy->as<Dom::SequenceBase>();
            for (std::size_t i = 0; i != sequence.size(); ++i)
            {
                const Dom::DomNodeBase& item = *sequence.child(i);
                std::unique_ptr<Dom::DomNodeBase> itemCopy;
                if (isSectionLike(item.type()) && isSectionLike(blueprint.type()))
                {
                    itemCopy = compacted(static_cast<const Dom::Section&>(item),
                        static_cast<const Dom::Section&>(blueprint));
                }
                else
                {
                    itemCopy = item.clone(&target);
                }
                itemCopy->setParent(&target);
                target.children().push_back(std::move(itemCopy));
            }
            return copy;
        }

        // A copy of the section carrying only what differs from the layout, at every depth - the
        // items of a sequence included, which withoutDefaults copies whole.
        std::unique_ptr<Dom::Section> compacted(const Dom::Section& section,
            const Dom::Section& layout)
        {
            std::unique_ptr<Dom::Section> result = std::make_unique<Dom::Section>(nullptr);
            for (const std::wstring_view key : section.getKeys())
            {
                const Dom::DomNodeBase& child = *section.child(key);
                const Dom::DomNodeBase* layoutChild = layout.child(key);
                if (layoutChild && Dom::sameValue(child, *layoutChild))
                    continue;

                std::unique_ptr<Dom::DomNodeBase> copy;
                const bool sections = layoutChild && isSectionLike(child.type())
                    && isSectionLike(layoutChild->type());
                const bool sequences = layoutChild && child.type() == Dom::DomNodeType::Sequence
                    && layoutChild->type() == Dom::DomNodeType::Sequence;
                if (sections)
                {
                    copy = compacted(static_cast<const Dom::Section&>(child),
                        static_cast<const Dom::Section&>(*layoutChild));
                }
                else if (sequences)
                {
                    copy = compactedSequence(static_cast<const Dom::SequenceBase&>(child),
                        static_cast<const Dom::SequenceBase&>(*layoutChild));
                }
                else
                {
                    copy = child.clone(result.get());
                }
                copy->setParent(result.get());
                result->setChild(key, std::move(copy));
            }
            return result;
        }
    }

    Dom::Dt::Section databaseLayout()
    {
        using namespace Dom::Dt;
        return {
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
                    Value{ Keys::isConstant, false }
                }
            }
        };
    }

    void writeDatabase(const Surface& surface, const std::filesystem::path& path)
    {
        Dom::Document<Dom::FileFormat::ClaFi> document{
            path,
            Dom::AutoSave::No,
            databaseLayout(),
            Dom::WriteDefaults::Yes
        };
        // The layout, applied once more and left untouched, is what every entry is compared with.
        const Dom::Document<Dom::FileFormat::ClaFi> layout{
            path,
            Dom::AutoSave::No,
            databaseLayout(),
            Dom::WriteDefaults::Yes
        };

        Sections& modules = (document / Keys::modules).as<Sections>();
        for (const Module& module : surface.modules())
            writeModule(modules.add(), surface, module);

        Sections& types = (document / Keys::types).as<Sections>();
        for (const Type& type : surface.types())
        {
            if (type.isPublic())
                writeType(types.add(), surface, type);
        }

        Sections& functions = (document / Keys::functions).as<Sections>();
        for (const FreeFunction& free : surface.functions())
        {
            Dom::Section& entry = functions.add();
            setPlaced(entry, free.function.name, free.function.place, free.function.comment);
            setScoped(entry, surface, free.nameSpace, free.module, free.function.place);
            setText(entry, Keys::kind, kindWord(free.function.kind));
            setText(entry, Keys::signature, free.function.signature);
            setText(entry, Keys::type, free.function.returnType);
            setText(entry, Keys::templateParameters, free.function.templateParameters);
        }

        Sections& constants = (document / Keys::constants).as<Sections>();
        for (const Variable& variable : surface.variables())
        {
            Dom::Section& entry = constants.add();
            setPlaced(entry, variable.field.name, variable.field.place, variable.field.comment);
            setScoped(entry, surface, variable.nameSpace, variable.module, variable.field.place);
            setText(entry, Keys::type, variable.field.type);
            setText(entry, Keys::value, variable.field.value);
            setFlag(entry, Keys::isConstant, variable.field.isConstant);
        }

        document.format().saveSectionToFile(*compacted(document, layout), path);
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
                return L"type";
            case TypeKind::Concept:
                return L"concept";
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
        }
        return {};
    }
}
