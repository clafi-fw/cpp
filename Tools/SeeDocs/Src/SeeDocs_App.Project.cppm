export module SeeDocs_App.Project;

import ClaFi.StdLib;

// A PROJECT IS A TREE WITH A DATA FOLDER IN IT. The tree is the folder of sources, read by the
// scanner as it stands - nothing is appended to the path the user chose; the data folder is where
// everything SeeDocs makes of the tree is kept - the database the scanner writes and the project's
// own properties - so that the tree itself is left as it was found. The folder is made once, with
// the user's consent, and a tree that has one is a project from then on.
namespace SeeDocs_App
{
    // The data folder's name, under the project's own folder.
    export constexpr std::wstring_view k_projectDataFolder = L".seedocs";
    export constexpr std::wstring_view k_projectFileName = L"Project.cfg";
    export constexpr std::wstring_view k_databaseFileName = L"Surface.cfg";

    // A project: the tree it stands on and what it is called.
    export struct Project
    {
        std::filesystem::path folder;
        std::wstring name;

        [[nodiscard]] std::filesystem::path dataFolder() const;
        [[nodiscard]] std::filesystem::path databasePath() const;
        [[nodiscard]] std::filesystem::path notesFolder() const;
    };

    // The data folder a project in that folder keeps, whether or not one stands there yet.
    export [[nodiscard]] std::filesystem::path dataFolderOf(const std::filesystem::path& folder);
    // Whether the folder carries a project's data folder.
    export [[nodiscard]] bool holdsProject(const std::filesystem::path& folder);
    // Reads the project kept in the folder; nothing where it holds none.
    export [[nodiscard]] std::optional<Project> readProject(const std::filesystem::path& folder);
    // Makes the data folder and the properties in it, the project named after the folder.
    // Nothing where the folder could not be made.
    export [[nodiscard]] std::optional<Project> createProject(const std::filesystem::path& folder);
    // Writes the project's properties into its data folder.
    export void writeProject(const Project&);
}
