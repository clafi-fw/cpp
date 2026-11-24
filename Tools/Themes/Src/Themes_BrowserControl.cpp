module Themes_App.ThemesBrowser;

import Themes_App.Consts;
import Themes_App.HomePage;
import Themes_App.ThemePage;

import ClaFi.Application.ThemesManager;
import ClaFi.App.ThemeIcon;

import ClaFi.Documents.Browser;
import ClaFi.Documents.Folder;
import ClaFi.Documents.HomePage;
import ClaFi.Documents.List;
import ClaFi.Documents.Page;

import ClaFi.Browser.Control;
import ClaFi.Browser.PageData;
import ClaFi.Browser.Settings;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Url;

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

    bool ThemesBrowser::openElement(const std::wstring_view themePath, const std::wstring_view name)
    {
        if (!themePath.starts_with(L'/') || name.empty())
        {
            return false;
        }

        const BrowserPage* page = currentPage();
        if (!page)
        {
            return false;
        }
        if (page->tag().value == Documents::k_documentPageTag
            && page->pageData().path() == themePath)
        {
            if (!static_cast<const ThemePage&>(*page).tabTheme().styles.of(name))
            {
                return false;
            }
            goTo(Url{ themePath, styleAnchor(name) });
            return true;
        }

        const PageData* data = findPageData(themePath);
        if (!data || !isDocumentPage(*data))
        {
            return false;
        }
        const std::filesystem::path file = documentFileOf(*data);
        if (file.empty())
        {
            return false;
        }
        const UserTheme theme = { file, storageFolder(*data) };
        if (!theme.readable() || !theme.theme().styles.of(name))
        {
            return false;
        }

        goToLater(Url{ themePath, styleAnchor(name) });
        return true;
    }

    Documents::DocumentsHomePageBase& ThemesBrowser::createHomePage(BrowserTab& tab)
    {
        return tab.createPage<HomePage>(
            &m_themes,
            Tag{ Documents::k_homePageTag },
            &folder(),
            listingView(*tab.pageData()),
            Documents::StorageTilesView{ storageTilesFor(*tab.pageData()) }
        );
    }

    Documents::DocumentPage& ThemesBrowser::createDocumentPage(BrowserTab& tab)
    {
        const PageData& data = *tab.pageData();
        return tab.createPage<ThemePage>(
            &m_themes,
            Tag{ Documents::k_documentPageTag },
            &folder(),
            Documents::DocumentStorageView{ &storageFolder(data), isHomePage(*data.parent) }
        );
    }

    void ThemesBrowser::drawTabIcon(BrowserTab& tab, DrawIconEvent& event)
    {
        const PageData* data = tab.pageData();
        if (!data || !isDocumentPage(*data))
        {
            DocumentsBrowserBase::drawTabIcon(tab, event);
            return;
        }
        drawThemeIcon(event, tabIconColors(tab));
    }

    // Writes into the entry what the icon is drawn from, for the runs in which this tab is
    // restored and never opened.
    void ThemesBrowser::saveViewState(BrowserTab& tab)
    {
        if (const AppTheme* theme = tabTheme(tab))
        {
            const ThemeIconColors iconColors{ *theme };
            (settings().tabEntry(tab.id()) / k_tabIconAttrName).set(iconColors);
        }
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
            return ThemeIconColors{ *theme };
        return (settings().tabEntry(tab.id()) / k_tabIconAttrName).get<ThemeIconColors>();
    }
}
