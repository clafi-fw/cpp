module ClaFi.App.ThemePick;

import ClaFi.App.Themes;
import ClaFi.Application.ThemesManager;

import ClaFi.Controls.ComboBox;
import ClaFi.Controls.TextItems;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.AppContext;

import ClaFi.Core.DomEngine;

// Dom::Value<std::wstring> picks its specialization off the scalar serializer declared here, so
// without this import the config node is an incomplete type wherever it is read.
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    // ThemePick

    ThemePick::~ThemePick()
    {
        appThemes().listeners().erase(this);
    }

    std::wstring ThemePick::currentPath() const
    {
        if (itemIndex())
            return m_themes[itemIndex().value()].path;
        return {};
    }

    // A PATH NAMING NOTHING IS WORN AS THE BUILT-IN - see storedTheme - and the config keeps the
    // path it holds, so a theme whose file comes back is stood on again.
    void ThemePick::setCurrentPath(const std::wstring_view path)
    {
        ItemIndexValue index = findPath(path);
        if (!index)
            index = findPath(k_defaultThemePath);
        if (!index)
            return;
        setItemIndex(index.value());
    }

    // THE PATH IS WHAT THE PICKER COMES BACK TO, not the item: every item here is taken down and
    // built again, and a theme is the same theme under the same path.
    void ThemePick::rebuild()
    {
        std::wstring pathToStandOn = currentPath();
        // A picker standing on no theme is one being built, and the config is what it opens on.
        if (pathToStandOn.empty())
            pathToStandOn = appContext().themePath().get();

        m_themes.clear();
        m_themeItems.clear();

        addTheme(defaultTheme(), k_defaultThemePath, BuiltInThemes::defaultTheme);

        for (const UserThemePtr& theme : appThemes().userThemes())
        {
            const std::wstring name = theme->path().stem().wstring();
            addTheme(theme->theme(), ThemesManager::pathOf(*theme), name);
        }

        setCurrentPath(pathToStandOn);
        // An item is as wide as the name on it, so a list built afresh is a new width. See the
        // deferred-request rule: ask, do not lay out from here.
        form().invalidateAlign();
    }

    // The themes the list holds now are the manager's own, so the colours the application
    // wears are taken from them rather than from the ones this change took down.
    void ThemePick::themesManagerChanged()
    {
        rebuild();
        applyStoredTheme(appContext());
    }

    void ThemePick::showDropdown(Control& initiator)
    {
        const std::wstring openedOn = currentPath();
        ComboBox::showDropdown(initiator);
        const std::wstring picked = currentPath();
        // WHAT THE LIST GIVES BACK IS THE STATED THEME. A theme under the pointer is worn and
        // not written, so a list closed on the theme it opened on leaves the config's on.
        if (picked == openedOn)
        {
            applyStoredTheme(appContext());
            return;
        }
        applyThemePath(appContext(), picked);
    }

    const ThemeEntry& ThemePick::entryOf(const TextItem& item) const
    {
        return m_themes[item.tag().get<std::size_t>()];
    }

    ItemIndexValue ThemePick::findPath(const std::wstring_view path) const
    {
        for (std::size_t i = 0; i != m_themes.size(); ++i)
            if (m_themes[i].path == path)
                return i;
        return {};
    }

    void ThemePick::addTheme(const AppTheme& theme, const std::wstring_view path,
        const std::wstring_view name)
    {
        const Tag entryTag{ m_themes.size() };
        m_themes.push_back(ThemeEntry{ .theme = &theme, .path = std::wstring{ path } });
        m_themeItems.push_back(TextItem{ entryTag, Text{ std::wstring{ name } } });
    }
}
