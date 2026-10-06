export module Themes_App.ThemesBrowser;

import Themes_App.AppIcon;
import Themes_App.HomePage;
import Themes_App.ThemePage;

import ClaFi.App.ThemeIcon;

import ClaFi.Documents.Browser;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.Base.ButtonBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.PaintIconEvent;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Browser;

    // The browser over the themes: the built-in with no file of its own, and the folder's.
    export class ThemesBrowser : public Documents::DocumentsBrowser<HomePage, ThemePage>
    {
    public:
        template<typename... Args>
        explicit ThemesBrowser(const CreateParams&, Args&&...);
        ~ThemesBrowser() override;
    protected:
        void fetchSubItems(PageData&) override;
        void paintTabIcon(BrowserTab&, PaintIconEvent&) override;
        void saveViewState(BrowserTab&) override;
        [[nodiscard]] bool isDocumentPage(const PageData&) const override;
        [[nodiscard]] std::filesystem::path documentFileOf(const PageData&) const override;
    private:
        using Base = Documents::DocumentsBrowser<HomePage, ThemePage>;
    private:
        // The theme of the page on this tab, and nothing while the tab has no page or its page is
        // not a theme.
        [[nodiscard]] static const AppTheme* tabTheme(const BrowserTab&);
        // What an unopened tab's icon is drawn from, out of the tab's entry.
        [[nodiscard]] ThemeIconColors tabIconColors(const BrowserTab&);
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    ThemesBrowser::ThemesBrowser(const CreateParams& params, Args&&... args)
        :
        Base{ params, std::forward<Args>(args)... }
    {
        // The mark itself is in AppIcon, so the same drawing answers the button and writes
        // the .ico. The button states its own view mode and icon size; nothing here restates
        // them.
        appButton().onPaintIcon(AppIcon::paintIcon);
    }
}
