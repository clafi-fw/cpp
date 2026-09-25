export module ClaFi.Diagnostic.Log;

import ClaFi.Diagnostic.Options;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

// The diagnostic window: an Output page, which is where every diagnosticLog line goes, and an
// FPS page - see Diagnostic::FpsPage. Hidden until the application menu's Show diagnostic, or
// until the config says it was up. The whole of it stands behind Diagnostic::Options::showForm;
// off, nothing here opens a window or keeps a line.
export
namespace ClaFi
{
    // The name the window's placement is kept under in the Forms section.
    constexpr std::wstring_view k_diagnosticFormName = L"Diagnostic";

    void diagnosticLog(const Text&, const Control*, bool showPtr);
    void initializeDiagnosticLog(AppContext&);
    void finalizeDiagnosticLog();
    // Shows the window, or brings it to the front when it is already up, on the input that asked.
    void showDiagnosticWindow(InputStamp);
    // The config section of the window's own: whether it was up. See Diagnostic
    [[nodiscard]] Dom::Dt::Section createDiagnosticLogConfigSchema();
    // Shows the window where the config says it was up. Called once the config is loaded.
    void restoreDiagnosticLogState();
    // Writes whether the window is up, and its placement while it is. See Diagnostic
    void storeDiagnosticLogState();

    template<typename T>
    void diagnosticLog(const T& text)
    {
        if constexpr (Diagnostic::Options::enabled)
            diagnosticLog(Text{ text }, nullptr, false);
    }

    template<typename T>
    void diagnosticLog(const T& text, const Control* ptr)
    {
        if constexpr (Diagnostic::Options::enabled)
            diagnosticLog(Text{ text }, ptr, true);
    }
}
