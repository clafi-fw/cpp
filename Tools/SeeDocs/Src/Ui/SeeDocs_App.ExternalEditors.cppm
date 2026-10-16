export module SeeDocs_App.ExternalEditors;

import SeeDocs_App.Surface;

import ClaFi.App.Notices;
import ClaFi.App.Settings;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.System.CommandRun;
import ClaFi.Core.System.Events;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    // What the config keeps the external editors' command lines under, one per language.
    export [[nodiscard]] ClaFi::Dom::Dt::Section createExternalEditorsSchema();
    // Fills the Settings page's External editors group: a command line each for C++ and Delphi.
    export void buildExternalEditors(ClaFi::AppMenuPage&);
    // Whether Settings holds a command line for the language's sources.
    export [[nodiscard]] bool hasEditorCommand(ClaFi::AppContext&, SourceLanguage);
    // What the Settings page calls the language: C++ or Delphi.
    export [[nodiscard]] std::wstring_view editorLanguageName(SourceLanguage);
    // Connects a handler raised when the language's command line changes.
    export [[nodiscard]] ClaFi::EventConnection onEditorCommandChange(ClaFi::AppContext&,
        SourceLanguage, std::function<void()> callback);
    // What the button that runs the command line for a source says.
    export constexpr std::wstring_view k_openInEditorCaption = L"Open in editor";
    // What a notice about opening the file heads its message with: the caption and the file's name.
    export [[nodiscard]] std::wstring openInEditorTitle(const std::filesystem::path& file);

    // The command lines Settings keeps, run one per language, and what each answered. See README
    export class EditorRuns
    {
    public:
        explicit EditorRuns(ClaFi::AppContext&);
        EditorRuns(const EditorRuns&) = delete;
        EditorRuns& operator=(const EditorRuns&) = delete;
        ~EditorRuns();
    public:
        // Runs the language's command line, {file} and {line} filled in; nothing where none is set.
        void open(SourceLanguage, const std::filesystem::path& file, std::size_t line);
    private:
        // A language's last run, and the notice its answer put up.
        struct Run
        {
            std::unique_ptr<ClaFi::CommandRun> command{};
            std::wstring program{};   // what the line starts with, as a notice names it
            std::wstring title{};     // what a notice heads its message with
            ClaFi::NoticeHandle notice{};
            ClaFi::ScopedEventConnection edited{};   // a command line edited takes the notice down
        };
    private:
        [[nodiscard]] Run& runOf(SourceLanguage);
        // Puts up a notice where the shell says the line did not run.
        void answered(SourceLanguage, const ClaFi::CommandRun&);
    private:
        ClaFi::AppContext& m_context;
        Run m_cpp{};
        Run m_delphi{};
    };
}
