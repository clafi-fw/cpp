export module ClaFi.Tools.WhatsClip.Main;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.App.Application;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;

import ClaFi.StdLib;
import ClaFi.Core.TextEngine.Text;

// NOTHING HERE NAMES A PLATFORM OR A BACKEND. What an entry point keeps is what its operating
// system asks for and the two types the application is built out of; the form, its pages and
// everything they read stand here and run over any platform the framework has.
namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams applicationParams();
    //AppParams whatsClipAppParams2
    //{
    //    .name = Text{
    //        L"What",
    //        InkWell::textInk(InkGrade::Muted), L"s", PopColor{},
    //        InkWell::spotInk(), L"Clip", PopColor{}
    //    },
    //    .publisher = L"ClaFi Framework"
    //};

    // Builds the viewer into a form of the given application, runs it, and answers what the form
    // answered. The lists are what its text pages pick an encoding and a language from - see
    // defaultEncodings and defaultLanguages for the framework's own, and an application built over
    // this adds what it reads.
    export int runWhatsClip(ApplicationBase&, const PickLists&);
}
