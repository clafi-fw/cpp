module ClaFi.Application.ThemesManager;

import ClaFi.App.ApplicationProperties;

import ClaFi.App.ThemeIcon;

import ClaFi.Documents.Folder;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // UserTheme

    UserTheme::UserTheme(const std::filesystem::path& path, const AppTheme& defaults)
        :
        m_path{ path }
    {
        Dom::Value<AppTheme> section{ nullptr, defaults };
        const Dom::FileFormat::ClaFi ff;
        m_readable = ff.loadSectionFromFile(section, path);
        if (m_readable)
        {
            loadTheme(section, m_theme);
        }
    }

    void UserTheme::loadTheme(const Dom::Value<AppTheme>& configNode, AppTheme& theme)
    {
        configNode.getTo(theme);
    }

    ThemeFolder::ThemeFolder(const std::filesystem::path& directory,
        const AppTheme* legacyThemeFileDefaults)
        :
        DocumentsFolder{ directory, k_themeKind },
        m_themeFileDefaults{ legacyThemeFileDefaults ? *legacyThemeFileDefaults : AppTheme{} }
    {
    }

    const UserThemeList& ThemeFolder::userThemes()
    {
        update();
        return m_userThemes;
    }

    const AppTheme* ThemeFolder::themeByName(const std::wstring_view themeName) const
    {
        for (const UserThemePtr& theme : m_userThemes)
            if (theme->readable() && theme->path().filename() == themeName)
                return &theme->theme();

        return nullptr;
    }

    void ThemeFolder::saveTheme(const std::wstring_view themeName,
        Dom::Value<AppTheme>& configNode) const
    {
        const AppTheme* theme = themeByName(themeName);
        if (!theme)
            return;
        saveTheme(*theme, configNode);
    }

    void ThemeFolder::saveTheme(const AppTheme& theme, Dom::Value<AppTheme>& configNode)
    {
        configNode.set(theme);
    }

    std::unique_ptr<Dom::Section> ThemeFolder::statedTheme(const Dom::Section& themeNode) const
    {
        const Dom::Value<AppTheme> defaults{ nullptr, m_themeFileDefaults };
        return Dom::withoutDefaults(themeNode, defaults);
    }

    bool ThemeFolder::readDocument(const std::filesystem::path& path,
        Dom::DomNodeBase& into) const
    {
        into.set(m_themeFileDefaults);
        const Dom::FileFormat::ClaFi ff;
        return ff.loadSectionFromFile(into.as<Dom::Section>(), path);
    }

    bool ThemeFolder::writeDocument(const Dom::DomNodeBase& node,
        const std::filesystem::path& path) const
    {
        const Dom::FileFormat::ClaFi ff;
        return ff.saveSectionToFile(node.as<Dom::Section>(), path);
    }

    bool ThemeFolder::isEdited(const std::filesystem::path& path) const
    {
        const UserTheme theme{ path, m_themeFileDefaults };
        if (!theme.readable())
        {
            return true;
        }
        const Dom::Value<AppTheme> defaults{ nullptr, m_themeFileDefaults };
        Dom::Value<AppTheme> themeNode{ nullptr, AppTheme{} };
        saveTheme(theme.theme(), themeNode);
        return !Dom::sameValue(themeNode, defaults);
    }

    void ThemeFolder::drawIcon(const std::wstring_view fileName, DrawIconEvent& event)
    {
        return;

        // The list is asked for first, so that a theme file reaches themeByName even when nothing
        // has read the directory yet.
        static_cast<void>(userThemes());
        if (const AppTheme* theme = themeByName(fileName))
            drawThemeIcon(event, *theme);
    }

    void ThemeFolder::filesRead(const Documents::FilePaths& files)
    {
        m_userThemes.clear();
        m_userThemes.reserve(files.size());
        for (const std::filesystem::path& file : files)
            m_userThemes.push_back(std::make_unique<UserTheme>(file, m_themeFileDefaults));
    }

    ThemesManager::ThemesManager(const std::filesystem::path& publisherDirectory,
        const AppElementNames requiredElements, const NamedTheme& defaultTheme,
        const NamedThemes& additionalThemes, const AppTheme* legacyThemeFileDefaults,
        const std::filesystem::path& appConfigDirectory,
        const std::filesystem::path& appExecutableDirectory)
        :
        ThemeFolder{ publisherDirectory, legacyThemeFileDefaults },
        m_requiredElements{ requiredElements },
        m_defaultTheme{ defaultTheme },
        m_additionalThemes{ additionalThemes },
        m_appConfigThemes{ appConfigDirectory, legacyThemeFileDefaults },
        m_appExecutableThemes{ appExecutableDirectory, legacyThemeFileDefaults }
    {
        m_appConfigThemes.setReadOnly(true);
        m_appExecutableThemes.setReadOnly(true);
        if (m_defaultTheme.name.empty())
        {
            noReach("A window application must declare a default theme.");
        }
        std::unordered_set<std::wstring_view> names{
            themePathName(k_defaultThemePath), m_defaultTheme.name
        };
        for (const NamedTheme& theme : m_additionalThemes)
        {
            if (theme.name.empty() || theme.name.find(ThemeRoots::Separator) != std::wstring::npos
                || !names.insert(theme.name).second)
            {
                noReach("An additional theme must have a unique Settings name without a slash.");
            }
        }
    }

    ThemeFolder* ThemesManager::fileThemeFolder(const std::wstring_view root)
    {
        if (root == ThemeRoots::AppConfig && !m_appConfigThemes.directory().empty())
        {
            return &m_appConfigThemes;
        }
        if (root == ThemeRoots::AppExecutable && !m_appExecutableThemes.directory().empty())
        {
            return &m_appExecutableThemes;
        }
        if (root == ThemeRoots::User && !directory().empty())
        {
            return this;
        }
        return nullptr;
    }

    bool ThemesManager::acceptsTheme(const AppTheme& theme) const
    {
        return std::ranges::all_of(m_requiredElements, [&theme](const std::wstring_view name) {
            return theme.styles.of(name) != nullptr;
        });
    }

    bool ThemesManager::offersTheme(const std::wstring_view root, const UserTheme& theme) const
    {
        return theme.readable() && (root != ThemeRoots::User || acceptsTheme(theme.theme()));
    }

    const AppTheme* ThemesManager::themeByPath(const std::wstring_view path)
    {
        if (path == k_defaultThemePath || path == L"BuiltIn/Default")
        {
            return &m_defaultTheme.theme;
        }
        const std::wstring_view root = themePathRoot(path);
        const std::wstring_view name = themePathName(path);
        if (root == ThemeRoots::App)
        {
            const auto theme = std::ranges::find(m_additionalThemes, name, &NamedTheme::name);
            return theme == m_additionalThemes.end() ? nullptr : &theme->theme;
        }
        if (ThemeFolder* folder = fileThemeFolder(root))
        {
            for (const UserThemePtr& theme : folder->userThemes())
            {
                if (name == theme->path().stem().wstring() && offersTheme(root, *theme))
                {
                    return &theme->theme();
                }
            }
        }
        return nullptr;
    }

    std::wstring ThemesManager::pathOf(const UserTheme& theme)
    {
        return themePathOf(ThemeRoots::User, theme.path().stem().wstring());
    }
}
