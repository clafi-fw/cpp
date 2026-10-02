module ClaFi.Diagnostic.Log;

import ClaFi.Diagnostic.Options;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

// The entry points, each standing behind Diagnostic::Options::enabled. See Diagnostic#Options
namespace ClaFi
{
    void diagnosticLog(const Text& text, const Control* ptr, bool showPtr)
    {
        if constexpr (Diagnostic::Options::enabled)
            writeDiagnosticLine(text, ptr, showPtr);
    }

    void initializeDiagnosticLog(AppContext& appContext)
    {
        if constexpr (Diagnostic::Options::enabled)
            openDiagnosticWindow(appContext);
    }

    void finalizeDiagnosticLog()
    {
        if constexpr (Diagnostic::Options::enabled)
            closeDiagnosticWindow();
    }

    void showDiagnosticWindow(const InputStamp stamp)
    {
        if constexpr (Diagnostic::Options::enabled)
            activateDiagnosticWindow(stamp);
    }

    Dom::Dt::Section createDiagnosticLogConfigSchema()
    {
        return Dom::Dt::Section{
            k_sectionName,
            Dom::Dt::Value{ k_visibleName, false },
            Dom::Dt::Value{ k_tabName, k_outputTabCaption }
        };
    }

    void restoreDiagnosticLogState()
    {
        if constexpr (Diagnostic::Options::enabled)
            restoreDiagnosticWindow();
    }

    void storeDiagnosticLogState()
    {
        if constexpr (Diagnostic::Options::enabled)
            storeDiagnosticWindow();
    }
}
