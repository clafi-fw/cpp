export module Themes_App.ThemesBrowser;

import Themes_App.AppIcon;

import ClaFi.App.ThemeIcon;
import ClaFi.Application.ThemesManager;

import ClaFi.Documents.Browser;
import ClaFi.Documents.Folder;
import ClaFi.Documents.HomePage;
import ClaFi.Documents.Page;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;

import ClaFi.Controls.AppButton;
import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.DrawIconEvent;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Browser;

    export class ThemesBrowser : public Documents::DocumentsBrowserBase
    {
    public:
        template<typename... Args>
        explicit ThemesBrowser(const CreateParams& params, ThemesManager& themes, Args&&... args);
        ~ThemesBrowser() override;
    public:
        bool openElement(std::wstring_view themePath, std::wstring_view name);
    protected:
        Documents::DocumentsHomePageBase& createHomePage(BrowserTab& tab) override;
        Documents::DocumentPage& createDocumentPage(BrowserTab& tab) override;
        void drawTabIcon(BrowserTab&, DrawIconEvent&) override;
        void saveViewState(BrowserTab&) override;
    private:
        // The theme of the page on this tab, and nothing while the tab has no page or its page is
        // not a theme.
        [[nodiscard]] static const AppTheme* tabTheme(const BrowserTab&);
        // What an unopened tab's icon is drawn from, out of the tab's entry.
        [[nodiscard]] ThemeIconColors tabIconColors(const BrowserTab&);
    private:
        ThemesManager& m_themes;
    };

    template<typename... Args>
    ThemesBrowser::ThemesBrowser(const CreateParams& params, ThemesManager& themes, Args&&... args)
        :
        DocumentsBrowserBase{ params, static_cast<Documents::DocumentsFolder*>(&themes),
            std::forward<Args>(args)... },
        m_themes{ themes }
    {
        appButton().onDrawIcon(AppIcon::drawIcon);
        setCurrentItem(pageControl());
    }
}
