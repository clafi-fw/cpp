export module ClaFi.Core.TextEngine.StyleSheet;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.StdLib;

namespace ClaFi
{
    // A style sheet was edited, and every window lays out again. See TextEngine-Types#stylesheet
    export struct StylesEditedEvent : public Event
    {
    };

    // Named styles kept at run time, any number of them. See TextEngine-Types#stylesheet
    export class StyleSheet
    {
    public:
        // The styles a sheet lists, in its order.
        using StyleList = std::vector<const TextStyle*>;

        // Where names read from a file are looked up. See TextEngine-Types#stylesheet-readingscope
        class ReadingScope
        {
        public:
            explicit ReadingScope(const StyleSheet*);
            ~ReadingScope();
            ReadingScope(const ReadingScope&) = delete;
            ReadingScope& operator=(const ReadingScope&) = delete;
        private:
            const StyleSheet* m_outerSheet{ nullptr }; // back in effect once this scope ends
        };

    public:
        StyleSheet() = default;
        StyleSheet(const StyleSheet& other);
        // Keeps where they stand the styles both name alike. See TextEngine-Types#stylesheet
        StyleSheet& operator=(const StyleSheet& other);
        StyleSheet(StyleSheet&& other) noexcept = default;
        // The same as a copy: a style here that a text may name stays where it stands.
        StyleSheet& operator=(StyleSheet&& other);
        ~StyleSheet() = default;

    public:
        // The named styles a text is set in beyond the framework's, and those they are based on.
        [[nodiscard]] static StyleSheet NamedIn(const Text&);
        // The sheet a style named in a file is read from: the scope's, else the application's.
        [[nodiscard]] static const StyleSheet* reading();
        // The style a name read from a file stands for: the reading sheet's, else the framework's.
        [[nodiscard]] static const TextStyle* findReading(std::wstring_view name);
        // The sheet read from where no scope states one, held by whoever states it.
        static void setApplicationSheet(const StyleSheet*);
        // Moves on with every edit to any sheet.
        [[nodiscard]] static std::uint64_t revision();
        // Where StylesEditedEvent is raised.
        [[nodiscard]] static EventDispatcher& events();

        // The sheet's style of that name, else the framework's; none where neither has one.
        [[nodiscard]] const TextStyle* find(std::wstring_view name) const;
        // The sheet's own style of that name; none where it has none.
        [[nodiscard]] const TextStyle* findOwn(std::wstring_view name) const;
        // What the sheet sets in place of a framework style: its own of that name, or that one.
        [[nodiscard]] const TextStyle& styleFor(const TextStyle& frameworkStyle) const;
        // Whether the style is one of the sheet's, a removed one included.
        [[nodiscard]] bool holds(const TextStyle*) const;
        [[nodiscard]] StyleList styles() const;
        [[nodiscard]] bool empty() const;
        bool operator==(const StyleSheet&) const;

        // States a style, the sheet's own; none where refused. See TextEngine-Types#stylesheet-set
        const TextStyle* set(const TextStyle&);
        // Gives a style another name; false where the name is empty or another style has it.
        bool rename(std::wstring_view name, std::wstring_view newName);
        // Takes a style off the list. See TextEngine-Types#stylesheet-remove
        bool remove(std::wstring_view name);
        // Lists a style at another place among those listed; false where none has the name.
        bool moveTo(std::wstring_view name, std::size_t position);

    private:
        // A style and the strings it names, at an address that holds for the sheet's life.
        struct Entry
        {
            std::wstring name{};
            std::optional<std::wstring> fontFamily{};
            TextStyle style{};
            bool removed{ false };
        };

        using EntryPtr = std::unique_ptr<Entry>;
        using Entries = std::vector<EntryPtr>;

    private:
        [[nodiscard]] Entry* liveEntry(std::wstring_view name);
        [[nodiscard]] const Entry* liveEntry(std::wstring_view name) const;
        // Whether a style may be a base here: one of the framework's, or one the sheet lists.
        [[nodiscard]] bool takesAsBase(const TextStyle*) const;
        // Whether a base would lead back to a style named as namesake, or to the style itself.
        [[nodiscard]] static bool leadsBack(const TextStyle* base, const TextStyle* style,
            const TextStyle* namesake);
        // Bases the framework's namesake of the entry here based on the entry; true where one was.
        bool standInFor(const Entry& entry, const TextStyle* namesake);
        // Takes the other's styles by name; true where a style already here changed.
        bool takeStylesOf(const StyleSheet& other);
        // The fields of source written into the entry, which keeps its address, name and base.
        static void stateFields(Entry&, const TextStyle& source);
        static void edited();

    private:
        static std::uint64_t s_revision;
        static const StyleSheet* s_applicationSheet;
        static thread_local const StyleSheet* s_scopeSheet;
        Entries m_entries{};
    };
}
