module ClaFi.App.ThemePick;

import ClaFi.App.ApplicationProperties;

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

    namespace
    {
        struct FileThemeSource
        {
            std::wstring_view root;
            std::wstring_view captionSuffix;
        };

        constexpr FileThemeSource k_fileThemeSources[] = {
            { ThemeRoots::AppExecutable, L" (Executable folder)" },
            { ThemeRoots::AppConfig, L" (App config)" },
            { ThemeRoots::User, L" (Publisher)" }
        };
    }

    // ThemePick

    ThemePick::~ThemePick()
    {
        listenToThemeFolders(false);
    }

    std::wstring ThemePick::currentPath() const
    {
        if (itemIndex())
            return m_themes[itemIndex().value()].path;
        return {};
    }

    void ThemePick::setCurrentPath(const std::wstring_view path)
    {
        ItemIndexValue index = findPath(path);
        if (!index)
            index = findPath(k_defaultThemePath);
        if (!index)
            return;
        setItemIndex(index.value());
    }

    void ThemePick::rebuild()
    {
        const std::wstring pathToStandOn = appContext().themePath().get();

        m_themes.clear();
        m_themeItems.clear();

        const NamedTheme& defaultTheme = m_manager.defaultTheme();
        addTheme(defaultTheme.theme, k_defaultThemePath, defaultTheme.name);
        for (const NamedTheme& theme : m_manager.additionalThemes())
        {
            addTheme(theme.theme, themePathOf(ThemeRoots::App, theme.name), theme.name);
        }

        for (const FileThemeSource& source : k_fileThemeSources)
        {
            ThemeFolder* folder = m_manager.fileThemeFolder(source.root);
            if (!folder)
            {
                continue;
            }
            for (const UserThemePtr& theme : folder->userThemes())
            {
                if (m_manager.offersTheme(source.root, *theme))
                {
                    const std::wstring name = theme->path().stem().wstring();
                    const std::wstring path = themePathOf(source.root, name);
                    addTheme(theme->theme(), path, std::wstring{ name }.append(source.captionSuffix));
                }
            }
        }

        setCurrentPath(pathToStandOn);
        // An item is as wide as the name on it, so a list built afresh is a new width. See the
        // deferred-request rule: ask, do not lay out from here.
        form().invalidateAlign();
    }

    // The themes the list holds now are the manager's own, so the colors the application
    // wears are taken from them rather than from the ones this change took down.
    void ThemePick::documentsFolderChanged()
    {
        if (droppedDown())
        {
            m_themeFoldersChanged = true;
            return;
        }
        rebuild();
        m_manager.restoreSelectedTheme();
    }

    void ThemePick::showDropdown(Control& initiator)
    {
        rebuild();
        const std::wstring openedOn = currentPath();
        ComboBox::showDropdown(initiator);
        const std::wstring picked = currentPath();
        // **What the list gives back is the stated theme.** A theme under the pointer is worn and
        // not written, so a list closed on the theme it opened on leaves the config's on.
        if (picked == openedOn)
        {
            m_manager.restoreSelectedTheme();
        }
        else
        {
            m_manager.selectTheme(picked);
        }
        if (m_themeFoldersChanged)
        {
            m_themeFoldersChanged = false;
            rebuild();
        }
    }

    void ThemePick::previewTheme(const std::wstring_view path)
    {
        if (const AppTheme* theme = m_manager.themeByPath(path))
        {
            m_manager.applyTheme(*theme);
        }
    }

    void ThemePick::listenToThemeFolders(const bool listen)
    {
        for (const FileThemeSource& source : k_fileThemeSources)
        {
            ThemeFolder* folder = m_manager.fileThemeFolder(source.root);
            if (!folder)
            {
                continue;
            }
            if (listen)
            {
                folder->listeners().insert(this);
            }
            else
            {
                folder->listeners().erase(this);
            }
        }
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
        m_themes.push_back(ThemeEntry{
            .iconColors = ThemeIconColors{ theme },
            .path = std::wstring{ path }
        });
        m_themeItems.push_back(TextItem{ entryTag, Text{ std::wstring{ name } } });
    }
}
