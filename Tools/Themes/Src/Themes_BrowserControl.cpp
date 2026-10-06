module Themes_App.ThemesBrowser;

import Themes_App.Consts;
import Themes_App.ThemePage;

import ClaFi.Application.ThemesManager;
import ClaFi.App.ThemeIcon;

import ClaFi.Documents.Browser;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;
import ClaFi.Browser.Settings;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Browser;

    ThemesBrowser::~ThemesBrowser()
    {
        // Written while the pages still stand; the base takes them down after this.
        for (BrowserTab& tab : tabs())
            if (tab.page())
                ThemesBrowser::saveViewState(tab);
    }

    void ThemesBrowser::fetchSubItems(PageData& data)
    {
        if (!isHomePage(data))
            return;
        // The built-in first, then the folder's files as the base names them: the same themes
        // the home page shows, in the order it shows them.
        addSubItem(data, k_defaultPageName);
        Base::fetchSubItems(data);
    }

    void ThemesBrowser::paintTabIcon(BrowserTab& tab, PaintIconEvent& event)
    {
        const PageData* data = tab.pageData();
        if (!data || !isDocumentPage(*data))
        {
            Base::paintTabIcon(tab, event);
            return;
        }
        paintThemeIcon(event, tabIconColors(tab));
    }

    // Writes into the entry what the icon is drawn from, for the runs in which this tab is
    // restored and never opened.
    void ThemesBrowser::saveViewState(BrowserTab& tab)
    {
        if (const AppTheme* theme = tabTheme(tab))
        {
            const ThemeIconColors iconColors{ theme->colors };
            (settings().tabEntry(tab.id()) / k_tabIconAttrName).set(iconColors);
        }
    }

    // A built-in's page is named with the extension the compiled-in themes go by, a user's with
    // the one a theme file carries - and both are theme pages.
    bool ThemesBrowser::isDocumentPage(const PageData& data) const
    {
        return Base::isDocumentPage(data) || data.name.ends_with(k_builtInThemeExtension);
    }

    std::filesystem::path ThemesBrowser::documentFileOf(const PageData& data) const
    {
        // The same test that keeps a user theme from taking a built-in's name. The extension
        // cannot answer it: a file the user drops in the directory called Foo.theme carries the
        // built-ins' extension and is not one of them.
        if (ThemesManager::isReservedName(std::filesystem::path{ data.name }.stem().wstring()))
            return {};
        return Base::documentFileOf(data);
    }

    const AppTheme* ThemesBrowser::tabTheme(const BrowserTab& tab)
    {
        const Control* page = tab.page();
        if (!page || page->tag().value != Documents::k_documentPageTag)
            return nullptr;
        return &static_cast<const ThemePage*>(page)->tabTheme();
    }

    ThemeIconColors ThemesBrowser::tabIconColors(const BrowserTab& tab)
    {
        if (const AppTheme* theme = tabTheme(tab))
            return ThemeIconColors{ theme->colors };
        return (settings().tabEntry(tab.id()) / k_tabIconAttrName).get<ThemeIconColors>();
    }
}
