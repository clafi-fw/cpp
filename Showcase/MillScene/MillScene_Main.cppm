export module ClaFi.Showcase.MillScene.Main;

import ClaFi.Showcase.MillScene;

import ClaFi.Controls.Panel;

import ClaFi.App.Application;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

// NOTHING HERE NAMES A PLATFORM OR A BACKEND. What the project file keeps is the entry point its
// operating system asks for and the two types the application is built out of; everything else -
// the form, its size, its title and what is built into it - stands here and would run over any
// platform the framework has.
namespace ClaFi::Showcase
{
    using namespace ::ClaFi::Controls;
    // A bare panel. The scene draws the whole of the window's inside, and the frame around it is
    // the framework's own.
    export class MillSceneForm : public Panel
    {
    public:
        using Panel::Panel;
    private:
        // To move the scene away from the bottom edge, so the window rounded corners remain intact
        // RichControl rather then Spacer, because Spacer doesn't have color
        Control& m_bottomSpacer{ createBottomBar<RichControl>(
            MinSize{ 6.0f },
            UiElement::Section
        ) };
    };

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams millSceneAppParams();

    // WHAT THIS APPLICATION KEEPS OF ITS OWN, handed to the application beside the framework's
    // schema and merged with it. The picked scene theme is the whole of it so far; empty is no
    // pick yet, and the scene then stands on the theme it starts with.
    export [[nodiscard]] Dom::Dt::Section createMillSceneConfigSchema();

    /// @brief Builds the showcase into a form of the given application, runs it, and answers what
    /// the form answered.
    export int runMillSceneShowcase(ApplicationBase&);


//-----------------------------------------------------------------------------


    AppParams millSceneAppParams()
    {
        return {
            .name = Text{ L"MillScene" },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Shows the ClaFi path painter at work: "
                L"a watermill landscape animated on a single CPU thread or through the graphics card, "
                L"in a choice of scene themes, with its frame times charted live."
            }
        };
    }

    Dom::Dt::Section createMillSceneConfigSchema()
    {
        return Dom::Dt::Section{ Dom::Dt::Value{ k_sceneThemeNodeName, L"" } };
    }

    constexpr float k_minWindowWidth{ 640.0f };
    constexpr float k_minWindowHeight{ 480.0f };

    int runMillSceneShowcase(ApplicationBase& application)
    {
        const auto form = application.createDialog<MillSceneForm>(
            application.metrics().primaryWindow,
            application.metrics().primaryWindowShadow,
            UiElement::Form,
            MinSize{ k_minWindowWidth / 2.0f, k_minWindowHeight / 2.0f },
            PreferredSize{ k_minWindowWidth, k_minWindowHeight }
        );
        form->setConfigName(AppContext::k_mainFormName);

        // Every handler it plants reads it, so it has to stand until the form has finished.
        MillSceneShowcase showcase{ application, form->content() };

        return form->execute();
    }
}
