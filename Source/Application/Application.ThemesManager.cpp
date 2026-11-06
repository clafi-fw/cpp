module ClaFi.Application.ThemesManager;

import ClaFi.App.ThemeIcon;

import ClaFi.Documents.Folder;

import ClaFi.Dom.Formats.ClaFi;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;

import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // The names a user theme may not take: every built-in's, without its extension.
        constexpr std::array<std::wstring_view, 1ull> k_reservedStems{
            BuiltInThemes::DefaultTheme
        };
    }

    // UserTheme

    UserTheme::UserTheme(const std::filesystem::path& path)
        :
        m_path{ path }
    {
        Dom::Value<AppTheme> section{ nullptr, {} };
        const Dom::FileFormat::ClaFi ff;
        ff.loadSectionFromFile(section, path);
        loadTheme(section, m_theme);
    }

    void UserTheme::loadTheme(const Dom::Value<AppTheme>& configNode, AppTheme& theme)
    {
        configNode.getTo(theme);
    }

    // addBuiltInStyles

    // Read as a theme file is, over the defaults - and a list in a file is the whole list, so
    // what comes back holds the text's styles alone. Each joins the built-in theme, replacing a
    // style of its name.
    void addBuiltInStyles(const std::wstring_view themeText)
    {
        if (themeText.empty())
            return;
        Dom::Value<AppTheme> section{ nullptr, {} };
        const Dom::FileFormat::ClaFi ff;
        std::wistringstream stream{ std::wstring{ themeText } };
        ff.loadSectionFromStream(section, stream);
        AppTheme theme{};
        UserTheme::loadTheme(section, theme);
        for (ControlStyle& style : theme.styles.list)
            defaultTheme().styles.add(std::move(style));
    }

    // ThemesManager

    ThemesManager::ThemesManager(const std::filesystem::path& directory,
        const AppElementNames requiredElements)
        :
        DocumentsFolder{ directory, k_themeKind },
        m_requiredElements{ requiredElements }
    {
    }

    const UserThemeList& ThemesManager::userThemes()
    {
        update();
        return m_userThemes;
    }

    const AppTheme* ThemesManager::themeByName(const std::wstring_view themeName) const
    {
        if (k_defaultThemeName == themeName)
            return &defaultTheme();

        for (const UserThemePtr& theme : m_userThemes)
            if (theme->path().filename() == themeName)
                return &theme->theme();

        return nullptr;
    }

    const AppTheme* ThemesManager::themeByPath(const std::wstring_view path)
    {
        const std::wstring_view root = themePathRoot(path);
        const std::wstring_view name = themePathName(path);
        if (root == ThemeRoots::BuiltIn)
            return builtInTheme(name);
        if (root != ThemeRoots::User)
            return nullptr;

        for (const UserThemePtr& theme : userThemes())
            if (name == theme->path().stem().wstring())
                return &theme->theme();
        return nullptr;
    }

    std::wstring ThemesManager::pathOf(const UserTheme& theme)
    {
        return themePathOf(ThemeRoots::User, theme.path().stem().wstring());
    }

    bool ThemesManager::isReservedName(const std::wstring_view stem)
    {
        return std::ranges::any_of(k_reservedStems, [stem](const std::wstring_view reserved) {
            return std::ranges::equal(stem, reserved, [](const wchar_t alpha, const wchar_t beta) {
                return std::towlower(alpha) == std::towlower(beta);
            });
        });
    }

    void ThemesManager::saveTheme(const std::wstring_view themeName,
        Dom::Value<AppTheme>& configNode) const
    {
        const AppTheme* theme = themeByName(themeName);
        if (!theme)
            return;
        saveTheme(*theme, configNode);
    }

    void ThemesManager::saveTheme(const AppTheme& theme, Dom::Value<AppTheme>& configNode)
    {
        configNode.set(theme);
    }

    std::unique_ptr<Dom::Section> ThemesManager::statedTheme(const Dom::Section& themeNode)
    {
        const Dom::Value<AppTheme> defaults{ nullptr, AppTheme{} };
        return Dom::withoutDefaults(themeNode, defaults);
    }

    bool ThemesManager::readDocument(const std::filesystem::path& path,
        Dom::DomNodeBase& into) const
    {
        // Seeded with the defaults and loaded over, exactly as UserTheme reads a file: the file
        // states only what it changes, so the node is a whole theme only if it is filled in the
        // same way. A file that cannot be read leaves the defaults, which is the empty theme.
        into.set(AppTheme{});
        const Dom::FileFormat::ClaFi ff;
        return ff.loadSectionFromFile(into.as<Dom::Section>(), path);
    }

    bool ThemesManager::writeDocument(const Dom::DomNodeBase& node,
        const std::filesystem::path& path) const
    {
        const Dom::FileFormat::ClaFi ff;
        ff.saveSectionToFile(*statedTheme(node.as<Dom::Section>()), path);
        return true;
    }

    // A theme file states only what it differs from the defaults in, so one that states nothing
    // is a theme nobody has touched - what New theme leaves behind, and what may go without
    // being asked about. Read off the disk, the way saving writes it.
    bool ThemesManager::isEdited(const std::filesystem::path& path) const
    {
        const UserTheme theme{ path };
        const Dom::Value<AppTheme> defaults{ nullptr, AppTheme{} };
        Dom::Value<AppTheme> themeNode{ nullptr, AppTheme{} };
        saveTheme(theme.theme(), themeNode);
        return !Dom::sameValue(themeNode, defaults);
    }

    void ThemesManager::drawIcon(const std::wstring_view fileName, DrawIconEvent& event)
    {
        return;

        // The list is asked for first, so that a theme file reaches themeByName even when nothing
        // has read the directory yet.
        static_cast<void>(userThemes());
        if (const AppTheme* theme = themeByName(fileName))
            drawThemeIcon(event, *theme);
    }

    void ThemesManager::checkNameShape(Controls::AcceptEditEvent& event,
        const std::wstring_view stem) const
    {
        DocumentsFolder::checkNameShape(event, stem);
        if (event.refused())
            return;
        if (isReservedName(stem))
            event.refuse(L"That name belongs to a built-in theme.");
    }

    // A theme lacking an element the application names is left out: what wears that element
    // would paint nothing under it.
    void ThemesManager::filesRead(const Documents::FilePaths& files)
    {
        m_userThemes.clear();
        for (const std::filesystem::path& file : files)
        {
            UserThemePtr theme = std::make_unique<UserTheme>(file);
            if (statesEveryElement(theme->theme()))
                m_userThemes.push_back(std::move(theme));
        }
    }

    bool ThemesManager::statesEveryElement(const AppTheme& theme) const
    {
        return std::ranges::all_of(m_requiredElements, [&theme](const std::wstring_view name) {
            return theme.styles.of(name) != nullptr;
        });
    }
}
