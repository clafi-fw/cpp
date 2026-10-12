export module SeeDocs_App.ExternalEditors;

import ClaFi.App.Settings;

import ClaFi.Core.DomEngine_Dt;

namespace SeeDocs_App
{
    // What the config keeps the external editors' command lines under, one per language.
    export [[nodiscard]] ClaFi::Dom::Dt::Section createExternalEditorsSchema();
    // Fills the Settings page's External editors group: a command line each for C++ and Delphi.
    export void buildExternalEditors(ClaFi::OptionsPage&);
}
