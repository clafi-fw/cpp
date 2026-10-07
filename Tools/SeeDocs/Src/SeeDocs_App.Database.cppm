export module SeeDocs_App.Database;

import SeeDocs_App.Surface;

import ClaFi.Core.DomEngine_Dt;

import ClaFi.StdLib;

// THE SURFACE AS A DOCUMENT. One file, in the framework's own format, read by the designer and
// the documentation generator. An entry states only what it has: a key at its default is left
// out, and the layout puts it back on reading.
namespace SeeDocs_App
{
    using namespace ClaFi;

    // The layout a reader applies to open the database. See the README beside the project.
    export [[nodiscard]] Dom::Dt::Section databaseLayout();

    // Writes the surface to the file, whole, replacing whatever stood there.
    export void writeDatabase(const Surface&, const std::filesystem::path&);

    // The words the Kind and Form keys take.
    export [[nodiscard]] std::wstring_view kindWord(TypeKind);
    export [[nodiscard]] std::wstring_view kindWord(FunctionKind);
    export [[nodiscard]] std::wstring_view formWord(PropertyForm);
    export [[nodiscard]] std::wstring_view accessWord(Access);
}
