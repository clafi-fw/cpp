export module SeeDocs_App.ScanCache;

import SeeDocs_App.PascalReader;
import SeeDocs_App.Surface;

import ClaFi.StdLib;

// **What a scan keeps of each file it read**, so that the scan after it reads only the files that
// changed: one binary file in the project's folder, written and read by the scanner alone. See
// README#scan-cache
namespace SeeDocs_App
{
    // What the folder listing says of a file - a file whose stamp is the same is not read again.
    export struct FileStamp
    {
        std::uintmax_t size{ 0 };
        std::int64_t written{ 0 };   // the last write, in the file clock's ticks

        [[nodiscard]] bool operator==(const FileStamp&) const = default;
    };

    // A macro as a file defines it, before it joins the tree's.
    export struct MacroDefinition
    {
        std::wstring name;
        std::wstring body;
        bool takesArguments{ false };
    };

    export using MacroDefinitions = std::vector<MacroDefinition>;

    // The macros one file gives the tree's: those in branches it reads, and those in the others.
    export struct FileMacros
    {
        MacroDefinitions read;
        MacroDefinitions unread;
    };

    // The class a qualified definition belongs to, as a read found it, and what it took of it.
    export struct OwnerLookup
    {
        std::wstring spelled;
        std::wstring scope;
        std::wstring found;   // the class's qualified name; empty where none was found
        std::wstring name;    // as the class has it, which tells its constructors
        Names bases;          // which tell the initializers of its bases
    };

    export using OwnerLookups = std::vector<OwnerLookup>;

    // What a file's definitions add to a class another file declares.
    export struct AddedMembers
    {
        std::wstring owner;   // the class's qualified name
        Properties properties;
        Events events;
        Restatements restatements;
    };

    export using AddedMembersList = std::vector<AddedMembers>;

    // One file as the scan before read it.
    export struct CachedFile
    {
        std::filesystem::path path;
        FileStamp stamp;
        FileRecords records;
        OwnerLookups lookups;
        AddedMembersList added;
        FileMacros macros;              // any C++ - what it gave the tree's macros
        bool definesSymbols{ false };   // Delphi - one the conditional symbols are gathered from
    };

    export using CachedFiles = std::vector<CachedFile>;

    // A scan's files in its reading order, with how it read them.
    export struct ScanCache
    {
        std::wstring build;   // the program that read them
        ScanMode mode{ ScanMode::AnyCppAndDelphi };
        HintRules hints{ HintRules::Strict };
        PascalSymbols symbols;
        CachedFiles files;
    };

    // The file's cache, where that build wrote it and it reads whole; empty otherwise.
    export [[nodiscard]] ScanCache readScanCache(const std::filesystem::path&,
        std::wstring_view build);
    // Writes the cache to the file, whole; false where the file could not be written.
    export bool writeScanCache(const ScanCache&, const std::filesystem::path&);
}
