export module SeeDocs_App.ExternalEditors;

import SeeDocs_App.Surface;

import ClaFi.App.Settings;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.System.Events;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    // What the config keeps the external editors' command lines under, one per language.
    export [[nodiscard]] ClaFi::Dom::Dt::Section createExternalEditorsSchema();
    // Fills the Settings page's External editors group: a command line each for C++ and Delphi.
    export void buildExternalEditors(ClaFi::OptionsPage&);
    // Whether Settings holds a command line for the language's sources.
    export [[nodiscard]] bool hasEditorCommand(ClaFi::AppContext&, SourceLanguage);
    // What the Settings page calls the language: C++ or Delphi.
    export [[nodiscard]] std::wstring_view editorLanguageName(SourceLanguage);
    // Connects a handler raised when the language's command line changes.
    export [[nodiscard]] ClaFi::EventConnection onEditorCommandChange(ClaFi::AppContext&,
        SourceLanguage, std::function<void()> callback);
    // Runs the language's command line, {file} and {line} filled in; nothing where none is set.
    export void openInEditor(ClaFi::AppContext&, SourceLanguage,
        const std::filesystem::path& file, std::size_t line);
}
