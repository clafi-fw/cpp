export module ThisApp.Language;

import ClaFi.Documents.Folder;

import ClaFi.Core.Syntax.Types;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;

    // A script a new file starts as, offered under its name.
    export struct ScriptTemplate
    {
        std::wstring name;
        std::wstring newStem;   // the stem a new file takes - the kind's when empty
        std::wstring text;
    };

    export using ScriptTemplates = std::vector<ScriptTemplate>;

    // What a script is to the application - read from Language.cfg, and compiled in without one.
    export class Language
    {
    public:
        explicit Language(const std::filesystem::path& configFolder);
        Language(const Language&) = delete;
        Language& operator=(const Language&) = delete;
    public:
        [[nodiscard]] const Documents::DocumentKind& kind() const { return m_kind; }
        [[nodiscard]] const ScriptTemplates& templates() const { return m_templates; }
        // The names the host offers, for a CodeBox to complete. Empty without a file.
        [[nodiscard]] const Syntax::CompletionEntries& completion() const { return m_completion; }
    private:
        [[nodiscard]] static std::filesystem::path findFile(
            const std::filesystem::path& configFolder);
        void read();
    private:
        static constexpr std::wstring_view k_fileName{ L"Language.cfg" };
        std::filesystem::path m_file;   // the config folder's, else the executable's, else empty
        std::wstring m_extension;   // what the kind's extension views, so it stands ahead of it
        Documents::DocumentKind m_kind;
        ScriptTemplates m_templates{};
        Syntax::CompletionEntries m_completion{};
    };
}
