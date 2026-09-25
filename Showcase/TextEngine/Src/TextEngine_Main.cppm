export module ClaFi.Showcase.TextEngine.Main;

import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.App.Application;

import ClaFi.StdLib;

// NOTHING HERE NAMES A PLATFORM OR A BACKEND. What an entry point keeps is what its operating
// system asks for and the two types the application is built out of; everything else - the form,
// its pages, its tabs and the caret readout in each corner - stands here and runs over any platform
// the framework has.
namespace ClaFi::Showcase::TextEngine
{
    using namespace ::ClaFi::Controls;

    export class MainForm : public WithBody<Panel, TabbedBox>
    {
    public:
        using WithBody::WithBody;
    };

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams textEngineAppParams();

    // Builds the showcase into a form of the given application, runs it, and answers what the form
    // answered.
    export int runTextEngineShowcase(ApplicationBase&);
}
