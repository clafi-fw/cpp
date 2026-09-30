export module ThisApp.ScriptsBrowser;

import ThisApp.AppIcon;
import ThisApp.ScriptPage;

import ClaFi.Documents.Browser;
import ClaFi.Documents.HomePage;
// The home page's list and box: the page is instantiated here, and clang wants what a template
// body names imported where it is instantiated.
import ClaFi.Documents.List;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Base.ButtonBase;

import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The browser over the scripts folder: the home page as the framework lists a folder, and a
    // script page over each file.
    export class ScriptsBrowser
        : public Documents::DocumentsBrowser<Documents::DocumentsHomePage<>, ScriptPage>
    {
    public:
        template<typename... Args>
        explicit ScriptsBrowser(const CreateParams&, Args&&...);
    private:
        using Base = Documents::DocumentsBrowser<Documents::DocumentsHomePage<>, ScriptPage>;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    ScriptsBrowser::ScriptsBrowser(const CreateParams& params, Args&&... args)
        :
        Base{ params, std::forward<Args>(args)... }
    {
        // The mark itself is in AppIcon, so the same drawing answers the button and the icon
        // file. The button states its own view mode and icon size; nothing here restates them.
        appButton().onPaintIcon(AppIcon::paintIcon);
    }
}
