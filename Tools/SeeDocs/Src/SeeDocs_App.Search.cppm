export module SeeDocs_App.Search;

import SeeDocs_App.Notes;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Core.TextEngine.Searcher;

import ClaFi.StdLib;

// **What a search finds in a project**: the names its surface carries, ranked against what is
// typed, and the hints and the footnote sections holding the words. See the README beside the
// project.
namespace SeeDocs_App
{
    // What a name a search can find stands for, in the order a kind is ranked.
    export enum class NameKind
    {
        Type,
        Folder,
        Module,
        Function,
        Constant,
        Property,
        Event,
        Method,
        Field,
        Member     // an enum's
    };

    // A name a search can find: what it is, how a page spells it, and what it stands in.
    export struct NamedEntry
    {
        NameKind kind{ NameKind::Type };
        std::wstring_view kindWord;   // what its row calls its kind - control before a type's own
        std::wstring name;     // as its row spells it - an event by its alias, a folder by its own
        Names spellings;       // what else finds it - an event's struct, a folder's path
        std::wstring owner;    // what a two-part query's first part is found in
        std::wstring nameSpace;   // what the names its hint holds are read in
        const Type* type{ nullptr };               // the type, or a member's
        const ContentsModule* module{ nullptr };   // where it is declared; null for a folder
        std::wstring folder;   // the path of the folder it stands in, or a folder's own
        const Comment* comment{ nullptr };         // where its hint is read from; null for none
        std::wstring link;     // what a page links it by
    };

    export using NamedEntries = std::vector<NamedEntry>;

    // How a name holds what is typed, best first. See README
    export enum class MatchRank
    {
        Exact,       // the name is what is typed
        Start,       // the name starts with it
        WordStart,   // a word of the name starts with it
        Inside       // it stands anywhere else in the name
    };

    // A name found: its entry, how it holds the words, and where they stand in the name a row
    // shows - nothing there for a name found by another of its spellings.
    export struct NameHit
    {
        const NamedEntry* entry{ nullptr };
        MatchRank rank{ MatchRank::Inside };
        std::optional<std::size_t> start{};
        std::size_t length{ 0 };
    };

    export using NameHits = std::vector<NameHit>;

    // The declaration a footnote section belongs to, by its place among the index's entries.
    using SectionOwners = std::unordered_map<std::wstring, std::size_t>;

    // Every name a search can find in a project - its folders, modules, types and their members,
    // functions and constants - in the tree's own order, and what each footnote section belongs to.
    export class SearchIndex
    {
    public:
        SearchIndex() = default;
        SearchIndex(const Surface&, const ContentsFolders&);
    public:
        [[nodiscard]] const NamedEntries& entries() const { return m_entries; }
        // The names holding the query, best first, as many as the limit takes. See README
        [[nodiscard]] NameHits find(std::wstring_view query,
            std::size_t limit = std::numeric_limits<std::size_t>::max()) const;
        // The declaration the section's heading names, else the first whose See reaches it.
        [[nodiscard]] const NamedEntry* ownerOf(std::wstring_view note, const NoteSection&) const;
    private:
        void addFolders(const ContentsFolders&);
        void addModule(const Surface&, const ContentsFolder&, const ContentsModule&);
        void addType(const Surface&, const ContentsFolder&, const ContentsModule&, const Type&);
        void add(NamedEntry);
        void addOwner(const NamedEntry&, std::size_t index);
        // The types named exactly so, and every type they stand on, as far as the surface reads.
        [[nodiscard]] std::unordered_set<const Type*> typesUnder(
            const ClaFi::Searcher& owner) const;
    private:
        const Surface* m_surface{ nullptr };
        NamedEntries m_entries{};
        SectionOwners m_cited{};   // by note and anchor, from the comments' See references
        SectionOwners m_named{};   // by the heading a footnote found by name is written under
    };

    // A project searched: what its name is, what is read in it, and the folder searched.
    export struct SearchedProject
    {
        std::wstring_view name;
        const Surface* surface{ nullptr };
        Notes* notes{ nullptr };
        const SearchIndex* index{ nullptr };
        std::wstring_view folder;   // with every folder under it; empty for the whole project
    };

    export using SearchedProjects = std::vector<SearchedProject>;

    // Turns a link a project's page spells into the one the search page is to carry.
    export using LinkSpeller = std::function<std::wstring(std::size_t project,
        std::wstring_view link)>;

    // The page of what the projects hold of the words: the names by kind, then the hints and the
    // footnote sections holding them, a group per project where there are several. See README
    export [[nodiscard]] Page searchPage(const SearchedProjects&, std::wstring_view query,
        const LinkSpeller&);

    // The words a page bands for a query: its two parts where it has two, else the query whole.
    export [[nodiscard]] std::wstring searchedWords(std::wstring_view query);
}
