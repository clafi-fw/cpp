export module SeeDocs_App.Database;

import SeeDocs_App.Surface;

import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine;

import ClaFi.StdLib;

// **The surface as a document.** One file, in the framework's own format, read by the designer and
// the documentation generator. An entry states only what it has: a key at its default is left
// out, and the layout puts it back on reading.
namespace SeeDocs_App
{
    using namespace ClaFi;

    // The layout a reader applies to open the database. See the README beside the project.
    export [[nodiscard]] Dom::Dt::Section databaseLayout();

    // The surface as the document the file holds - built once, saved, and read back without it.
    export class Database
    {
    public:
        // Builds the document; the steps, where a list is handed in, time it, a save and a read.
        explicit Database(const Surface&, ScanSteps* steps = nullptr);
        ~Database();
        // Writes the document to the file, whole, replacing whatever stood there; false where
        // the file could not be written.
        [[nodiscard]] bool save(const std::filesystem::path&) const;
        [[nodiscard]] Surface surface() const;   // what a reader of the saved file is given
    private:
        ScanSteps* m_steps;
        std::unique_ptr<Dom::Section> m_root;
    };

    // Writes the surface to the file as its database; false where the file could not be written.
    export bool writeDatabase(const Surface&, const std::filesystem::path&,
        ScanSteps* steps = nullptr);
    // Reads the file back into a surface, through the layout that wrote it. Nothing where the
    // file cannot be read. The steps, where a list is handed in, time the load and the read.
    export [[nodiscard]] std::optional<Surface> readDatabase(const std::filesystem::path&,
        ScanSteps* steps = nullptr);

    // The words the Kind and Form keys take.
    export [[nodiscard]] std::wstring_view kindWord(TypeKind);
    export [[nodiscard]] std::wstring_view kindWord(FunctionKind);
    export [[nodiscard]] std::wstring_view formWord(PropertyForm);
    export [[nodiscard]] std::wstring_view accessWord(Access);
    // The enumerators those words stand for; nothing for a word the database does not write.
    export [[nodiscard]] std::optional<TypeKind> typeKindOf(std::wstring_view word);
    export [[nodiscard]] std::optional<FunctionKind> functionKindOf(std::wstring_view word);
    export [[nodiscard]] std::optional<PropertyForm> formOf(std::wstring_view word);
    export [[nodiscard]] std::optional<Access> accessOf(std::wstring_view word);
}
