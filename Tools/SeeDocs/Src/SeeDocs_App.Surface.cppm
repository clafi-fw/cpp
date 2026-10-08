export module SeeDocs_App.Surface;

import ClaFi.StdLib;

// WHAT THE SCANNER READS OUT OF A TREE, as plain records. Nothing here knows how the records are
// found or how they are written; the scanner fills a Surface and the database and the checks
// read it.
namespace SeeDocs_App
{
    // Where a declaration stands: a file of the surface, and a line in it, counted from one.
    export struct Place
    {
        std::size_t file{ 0 };
        std::size_t line{ 0 };
    };

    // A comment's reference to a footnote - the stem of the note, and a heading in it.
    export struct Reference
    {
        std::wstring note;
        std::wstring anchor;
    };

    // The comment a declaration carries, as written and as the database reads it.
    export struct Comment
    {
        std::vector<std::wstring> above;        // the comment lines touching the declaration
        std::optional<std::wstring> trailing;   // the comment on the declaration line
        std::size_t aboveLine{ 0 };             // where the lines above start
        std::size_t aboveWidth{ 0 };            // the widest of them, in columns
        std::size_t lineWidth{ 0 };             // the declaration line's width, comment included
        std::wstring text;                      // the one line the database carries
        std::optional<Reference> reference;     // the See reference that line ended with
        [[nodiscard]] bool empty() const { return above.empty() && !trailing; }
    };

    export enum class Access
    {
        Public,
        Protected,
        Private
    };

    // How a property line is written, which decides what the class owes it.
    export enum class PropertyForm
    {
        Declared,      // DECLARE_PROPERTY
        Writable,      // DECLARE_WRITABLE_PROPERTY
        ByReference,   // DECLARE_REF_PROPERTY
        Storage,       // DECLARE_PROPERTY_STORAGE
        BoundMember,   // BIND_PROPERTY_MEMBER
        BoundCall,     // BIND_PROPERTY_CALL
        BoundValue,    // BIND_PROPERTY_VALUE
        BoundAction,   // BIND_PROPERTY_ACTION
        Read,          // READ_PROPERTY
        Required       // REQUIRE_PROPERTY - the pack has to carry it, so there is no default
    };

    // Whether a form declares storage of its own, which the constructor then has to initialise.
    export [[nodiscard]] constexpr bool declaresStorage(const PropertyForm form)
    {
        return form == PropertyForm::Declared || form == PropertyForm::Writable
            || form == PropertyForm::ByReference || form == PropertyForm::Storage;
    }

    // A value the caller passes into the constructor pack.
    export struct Property
    {
        std::wstring name;
        std::wstring type;
        std::vector<std::wstring> accepts;   // the further types bindings on the same target take
        std::wstring defaultValue;
        std::wstring setter;                 // what a writable property names
        std::wstring target;                 // where the value lands - a member, a setter, a call
        PropertyForm form{ PropertyForm::Declared };
        std::wstring valueKind;              // what a designer shows the value as, resolved later
        bool optional{ false };              // the type is a std::optional of the kind
        Access access{ Access::Public };
        Place place;
        Comment comment;
    };

    // An event a class raises, as DECLARE_EVENT spells it.
    export struct Event
    {
        std::wstring type;
        std::wstring alias;
        std::wstring method;
        Access access{ Access::Public };
        Place place;
        Comment comment;
    };

    // A data member.
    export struct Field
    {
        std::wstring name;
        std::wstring type;
        std::wstring value;        // the initializer, as spelled
        bool isStatic{ false };
        bool isConstant{ false };
        Access access{ Access::Public };
        Place place;
        Comment comment;
    };

    // A using-declaration naming a base's member: its constructors, or one republished.
    export struct Using
    {
        std::wstring base;   // as spelled before the last ::
        std::wstring name;
        Access access{ Access::Public };
        Place place;
        Comment comment;
    };

    export enum class FunctionKind
    {
        Function,
        Constructor,
        Destructor
    };

    // A function, member or free.
    export struct Function
    {
        std::wstring name;
        FunctionKind kind{ FunctionKind::Function };
        std::wstring signature;            // as spelled, export and the template head taken off
        std::wstring returnType;
        std::wstring templateParameters;   // what stands between the template head's brackets
        bool isStatic{ false };
        bool isVirtual{ false };
        bool isDeleted{ false };
        Access access{ Access::Public };
        Place place;
        Comment comment;
    };

    export struct EnumMember
    {
        std::wstring name;
        std::wstring value;   // the initializer, as spelled
        Place place;
        Comment comment;
    };

    export enum class TypeKind
    {
        Class,
        Struct,
        Union,
        Enum,
        Alias,
        Concept
    };

    // A type the tree declares - a class with its members, an enum with its members, an alias.
    export struct Type
    {
        std::wstring name;                  // relative to the namespace - Outer::Nested when nested
        std::wstring nameSpace;
        std::wstring qualifiedName;         // nameSpace::name, the key the surface is looked up by
        TypeKind kind{ TypeKind::Class };
        std::wstring module;
        std::wstring templateParameters;
        std::vector<std::wstring> bases;    // as spelled, access and virtual taken off
        std::wstring target;                // an alias's type, a concept's constraint, an enum base
        bool exported{ false };
        Access access{ Access::Public };    // a nested type's, under the label it stands in
        bool isControl{ false };            // the base chain reaches Control, resolved later
        std::wstring category;              // the directory under Source
        std::vector<Property> properties;
        std::vector<Event> events;
        std::vector<Function> functions;
        std::vector<Using> usings;
        std::vector<Field> fields;
        std::vector<EnumMember> members;
        Place place;
        Comment comment;
        [[nodiscard]] bool opensScope() const;
        // Whether the type is part of the surface: exported, and not private to a class.
        [[nodiscard]] bool isPublic() const { return exported && access != Access::Private; }
    };

    // A function at namespace scope.
    export struct FreeFunction
    {
        Function function;
        std::wstring nameSpace;
        std::wstring module;
        bool exported{ false };
    };

    // A constant or a variable at namespace scope.
    export struct Variable
    {
        Field field;
        std::wstring nameSpace;
        std::wstring module;
        bool exported{ false };
    };

    export struct Import
    {
        std::wstring name;
        bool exported{ false };
    };

    // A module unit: an interface or an implementation, with what it imports.
    export struct Module
    {
        std::wstring name;
        std::size_t file{ 0 };
        bool isInterface{ false };
        std::vector<Import> imports;
    };

    export struct SourceFile
    {
        std::filesystem::path path;
        std::wstring relative;   // to the tree's root, with forward slashes
        std::wstring module;
    };

    // A bind written in a constructor - INIT_PROPERTY, BIND_MEMBER and its kind - by the name it
    // names, for the checks.
    export struct Binding
    {
        std::wstring name;
        Place place;
    };

    // A declaration the scanner could not read as the routine asks, reported with the checks.
    export struct Problem
    {
        bool error{ true };
        Place place;
        std::wstring text;
    };

    export using Properties = std::vector<Property>;
    export using Events = std::vector<Event>;
    export using Functions = std::vector<Function>;
    export using Usings = std::vector<Using>;
    export using Fields = std::vector<Field>;
    export using EnumMembers = std::vector<EnumMember>;
    export using Types = std::vector<Type>;
    export using FreeFunctions = std::vector<FreeFunction>;
    export using Variables = std::vector<Variable>;
    export using Modules = std::vector<Module>;
    export using SourceFiles = std::vector<SourceFile>;
    export using Bindings = std::vector<Binding>;
    export using Places = std::vector<Place>;
    export using Problems = std::vector<Problem>;
    export using Names = std::vector<std::wstring>;

    // The tree as the scanner read it. Types are looked up by qualified name.
    export class Surface
    {
    public:
        [[nodiscard]] const SourceFiles& files() const { return m_files; }
        [[nodiscard]] const Modules& modules() const { return m_modules; }
        [[nodiscard]] const Types& types() const { return m_types; }
        [[nodiscard]] Types& types() { return m_types; }
        [[nodiscard]] const FreeFunctions& functions() const { return m_functions; }
        [[nodiscard]] const Variables& variables() const { return m_variables; }
        [[nodiscard]] const Bindings& bindings() const { return m_bindings; }
        // Where Props::get is written by hand rather than through READ_PROPERTY.
        [[nodiscard]] const Places& propsReads() const { return m_propsReads; }
        [[nodiscard]] const Problems& problems() const { return m_problems; }

        std::size_t addFile(SourceFile);
        void addModule(Module);
        void addType(Type);
        void addFunction(FreeFunction);
        void addVariable(Variable);
        void addBinding(Binding);
        void addPropsRead(Place);
        void addProblem(Problem);

        [[nodiscard]] const Type* typeNamed(std::wstring_view qualifiedName) const;
        [[nodiscard]] Type* typeNamed(std::wstring_view qualifiedName);
        // The type a name spelled inside a namespace stands for: the namespace's own, an
        // enclosing namespace's, or the one type of that bare name anywhere. Aliases are followed.
        [[nodiscard]] const Type* resolve(std::wstring_view spelled,
            std::wstring_view nameSpace) const;
        // A base's spelling with an alias the database does not carry replaced by what it names,
        // followed until the spelling names a type the database does carry.
        [[nodiscard]] std::wstring resolvedBase(std::wstring_view spelled,
            std::wstring_view nameSpace) const;
        // Whether a type's base chain reaches Control, template arguments walked as bases.
        [[nodiscard]] bool reachesControl(const Type&) const;
        // What a designer shows a property of the type as.
        [[nodiscard]] std::wstring valueKindOf(std::wstring_view type, std::wstring_view nameSpace,
            bool& optional) const;
        // A type's properties with one entry per place a value lands: several bindings on one
        // target are one property accepting several spellings.
        [[nodiscard]] Properties mergedProperties(const Type&) const;
        // Fills in what the scan leaves to the whole: control marks and value kinds.
        void resolveAll();
    private:
        [[nodiscard]] const Type* lookUp(std::wstring_view bare, std::wstring_view nameSpace) const;
    private:
        SourceFiles m_files;
        Modules m_modules;
        Types m_types;
        FreeFunctions m_functions;
        Variables m_variables;
        Bindings m_bindings;
        Places m_propsReads;
        Problems m_problems;
        std::unordered_map<std::wstring, std::size_t> m_byQualifiedName;
        std::unordered_map<std::wstring, std::vector<std::size_t>> m_byBareName;
    };

    // Whether a using-declaration names the base's constructors - using Base::Base.
    export [[nodiscard]] bool inheritsConstructors(const Using&);
    // The last component of a qualified name.
    export [[nodiscard]] std::wstring_view bareName(std::wstring_view qualified);
    // A spelled type without its template arguments - the name before the first bracket.
    export [[nodiscard]] std::wstring_view withoutArguments(std::wstring_view spelled);
    // The template arguments of a spelled type, split at the top level.
    export [[nodiscard]] Names argumentsOf(std::wstring_view spelled);
    // A type spelling with const, references and pointers taken off.
    export [[nodiscard]] std::wstring_view plainType(std::wstring_view spelled);
    // The names a template head declares - T of typename T, N of std::size_t N.
    export [[nodiscard]] Names templateParameterNames(std::wstring_view parameters);
    // Whether a base spelled inside a type is one of the type's own template parameters.
    export [[nodiscard]] bool isTemplateParameter(const Type&, std::wstring_view spelled);
    // The text cut at its top-level commas, each piece trimmed - a macro's or template's arguments.
    export [[nodiscard]] Names splitArguments(std::wstring_view text);
    // The text with its blanks trimmed off both ends.
    export [[nodiscard]] std::wstring_view trimmed(std::wstring_view text);
    // A markdown heading's anchor, the way a renderer makes one, of the name's last component.
    export [[nodiscard]] std::wstring slug(std::wstring_view text);
    // The reference a comment line ends with, cut off the line: the word See, the note's stem, an
    // optional # and anchor, and nothing after. The anchor defaults to the name's own slug.
    export [[nodiscard]] std::optional<Reference> cutReference(std::wstring& text,
        std::wstring_view name);
}
