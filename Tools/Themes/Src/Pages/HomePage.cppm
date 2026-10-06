export module Themes_App.HomePage;

import Themes_App.WithPreview;

import ClaFi.App.ThemesList;

import ClaFi.Documents.HomePage;

// The page's box: the home page template is instantiated here, and clang wants what a template
// body names imported where it is instantiated.
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The themes as tiles, the built-in and the user's own, and the preview of the one looked at.
    export class HomePage : public WithPreview<Documents::DocumentsHomePage<ThemesList>>
    {
    public:
        template<typename... Args>
        explicit HomePage(const CreateParams&, Args&&...);
    protected:
        const AppTheme* selectedTheme() override;
        void documentsRebuilt() override;
    private:
        using Base = WithPreview;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    HomePage::HomePage(const CreateParams& params, Args&&... args)
        :
        Base{ params, std::forward<Args>(args)... }
    {
        // THE TILE BEING LOOKED AT, not the one that has been picked. Moving the current item is
        // immediate and a theme change is a movement of its own, so a current tile travelling
        // down the list on a held key would ask for one per step and the application would never
        // arrive anywhere. A preview is raised once the tile settles, whether the pointer or the
        // keyboard brought it there, so the application crosses to the theme that was stopped on.
        list().onPreview([this](PreviewEvent&) {
            invalidatePreview();
        });
    }
}
