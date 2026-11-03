export module ClaFi.Core.TextEngine.StyleSheet;

import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.StdLib;

namespace ClaFi
{
    // A style sheet was edited, and every window lays out again. See TextEngine-Types#stylesheet
    export struct StylesEditedEvent : public Event
    {
    };

    // A style name written in code or read from a file was found in no sheet.
    export class UnknownStyleEvent : public Event
    {
    public:
        explicit UnknownStyleEvent(std::wstring_view name);
        [[nodiscard]] std::wstring_view name() const { return m_name; }
    private:
        std::wstring m_name;
    };

    // Named styles and unnamed ones cached by value, over a parent. See TextEngine-Types#stylesheet
    export class StyleSheet
    {
    public:
        // The named styles a sheet lists, in its order.
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
        // A sheet based on the framework's.
        StyleSheet();
        // A sheet based on that one, or on none.
        explicit StyleSheet(const StyleSheet* parent);
        // Every style at the position it holds in the other, over the same parent.
        StyleSheet(const StyleSheet& other);
        StyleSheet(StyleSheet&& other) noexcept = default;
        // Refused: a style a text names keeps its address. See TextEngine-Types#stylesheet
        StyleSheet& operator=(const StyleSheet&) = delete;
        StyleSheet& operator=(StyleSheet&&) = delete;
        ~StyleSheet() = default;

    public:
        // Body, Title, SubTitle, Heading, SubHeading, Section, SubSection, SubBody and Code.
        [[nodiscard]] static const StyleSheet& framework();
        // The sheet a text is built with where none is named: the application's or the framework's.
        [[nodiscard]] static const StyleSheet& application();
        // The application's sheet, held by whoever states it; none leaves the framework's.
        static void setApplicationSheet(const StyleSheet*);
        // The sheet a text read from a file is built with, where a scope names one.
        [[nodiscard]] static const StyleSheet* reading();
        // Moves on with every edit to any sheet.
        [[nodiscard]] static std::uint64_t revision();
        // Where StylesEditedEvent and UnknownStyleEvent are raised.
        [[nodiscard]] static EventDispatcher& events();

        [[nodiscard]] const StyleSheet* parent() const { return m_parent; }
        // The style of that name, the sheet's own or else its parents'; none where none has it.
        [[nodiscard]] const TextStyle* find(std::wstring_view name) const;
        // The sheet's own style of that name; none where it has none.
        [[nodiscard]] const TextStyle* findOwn(std::wstring_view name) const;
        // The Body found from here, which draws whatever a text leaves unstated.
        [[nodiscard]] const TextStyle& body() const;
        // Whether the style is one of the sheet's own, a removed one included.
        [[nodiscard]] bool holds(const TextStyle*) const;
        // Whether the style is listed here or up the parents, where a text built here finds it.
        [[nodiscard]] bool reaches(const TextStyle*) const;
        [[nodiscard]] StyleList styles() const;
        [[nodiscard]] bool empty() const; // whether no named style is listed
        bool operator==(const StyleSheet&) const;

        // Where a style named none stating the values stands. See TextEngine-Types#stylesheet-cache
        std::size_t cache(const TextStyle& values);
        [[nodiscard]] std::size_t count() const; // every style held, removed and unnamed included
        [[nodiscard]] const TextStyle& at(std::size_t position) const;

        // States a style, the sheet's own; none where refused. See TextEngine-Types#stylesheet-set
        const TextStyle* set(const TextStyle&);
        // States a style and raises no edit. See TextEngine-Types#stylesheet-carry
        const TextStyle* carry(const TextStyle&);
        // Gives a style another name; false where the name is empty or another style has it.
        bool rename(std::wstring_view name, std::wstring_view newName);
        // Takes a style off the list. See TextEngine-Types#stylesheet-remove
        bool remove(std::wstring_view name);
        // Lists a style at another place among those listed; false where none has the name.
        bool moveTo(std::wstring_view name, std::size_t position);
        // Takes the other's named styles, each named alike here keeping its address.
        void takeStyles(const StyleSheet& other);
        // Exchanges what the two hold; a text based on a style follows the style.
        void swap(StyleSheet& other) noexcept;

    private:
        // A style and whether it has been taken off the list, at an address that holds.
        struct Entry
        {
            TextStyle style{};
            bool removed{ false };
        };

        using EntryPtr = std::unique_ptr<Entry>;
        using Entries = std::vector<EntryPtr>;

    private:
        [[nodiscard]] Entry* liveEntry(std::wstring_view name);
        [[nodiscard]] const Entry* liveEntry(std::wstring_view name) const;
        // The parents' style of that name, which one of the sheet's own of that name restates.
        [[nodiscard]] const TextStyle* namesakeOf(std::wstring_view name) const;
        // Whether a base would lead back to a style named as namesake, or to the style itself.
        [[nodiscard]] static bool leadsBack(const TextStyle* base, const TextStyle* style,
            const TextStyle* namesake);
        const TextStyle* state(const TextStyle&, bool quiet);
        // Bases the namesake of the entry here based on the entry; true where one was.
        bool standInFor(const Entry& entry, const TextStyle* namesake);
        // Takes the other's named styles by name; true where a style already here changed.
        bool mergeFrom(const StyleSheet& other);
        // The fields of source written into the entry, which keeps its name and its base.
        static void stateFields(Entry&, const TextStyle& source);
        static void edited();

    private:
        static std::uint64_t s_revision;
        static const StyleSheet* s_applicationSheet;
        static thread_local const StyleSheet* s_scopeSheet;
        const StyleSheet* m_parent{ nullptr };
        Entries m_entries{};
    };
}
