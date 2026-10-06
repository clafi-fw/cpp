export module PascalScripts_App.Scripts;

import PascalScripts_App.Language;

import ClaFi.Documents.Folder;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.DomEngine;

import ClaFi.StdLib;

namespace PascalScripts_App
{
    using namespace ::ClaFi;

    // The folder the scripts stand in. A script is a text file, and the tab keeps it as text.
    export class ScriptsFolder : public Documents::DocumentsFolder
    {
    public:
        // Of the kind the language states, offering its templates. The language outlives the
        // folder: the kind views its strings.
        ScriptsFolder(const std::filesystem::path& directory, const Language&);
    public:
        // What a script is - the templates the folder offers and the names a page completes to.
        [[nodiscard]] const Language& language() const { return m_language; }
        [[nodiscard]] bool readDocument(const std::filesystem::path&,
            Dom::DomNodeBase& into) const override;
        [[nodiscard]] bool writeDocument(const Dom::DomNodeBase&,
            const std::filesystem::path&) const override;
        [[nodiscard]] bool isEdited(const std::filesystem::path&) const override;
        void paintIcon(std::wstring_view fileName, PaintIconEvent&) override;
    private:
        const Language& m_language;
    };
}
