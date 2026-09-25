export module ClaFi.Application.ThemesManager;

import ClaFi.Dom.Formats.ClaFi;
export import ClaFi.Application.ThemesManager_Serializers;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine;

import ClaFi.Diagnostic.Log;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Palette;

import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Csv;
import ClaFi.Core.System.DirWatch;
import ClaFi.Core.System.Events;

import ClaFi.StdLib;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    export constexpr int k_defaultAnchorHue{ 210 };

    export constexpr std::wstring_view k_themeDataAttrName{ L"ThemeData" };
    export constexpr std::wstring_view k_paletteNodeName2{ L"Palette" };

    export constexpr std::wstring_view k_anchorHueAttrName{ L"AnchorHue" };

    export constexpr std::wstring_view k_harmonyAttrName{ L"Harmony" };
    export constexpr std::wstring_view k_huesAttrName{ L"Hues" };

    //export constexpr std::wstring_view k_bgSaturationAttrName{ L"BgSaturation" };
    //export constexpr std::wstring_view k_bgLuminosityAttrName{ L"BgLuminosity" };
    export constexpr std::wstring_view k_rulesAttrName{ L"Rules" };
    export constexpr std::wstring_view k_backgroundNodeName{ L"Background" };

    export constexpr std::wstring_view k_hueActionAttrName{ L"HueAction" };
    export constexpr std::wstring_view k_hueExactValueAttrName{ L"HueExactValue" };

    export constexpr std::wstring_view k_defaultThemeName = L"Default.theme";
    export constexpr std::wstring_view k_dark2ThemeName = L"Dark2.theme";

    export constexpr std::wstring_view k_themeFileExtension = L".clafitheme";
    export constexpr std::wstring_view k_builtInThemeExtension = L".theme";

    export class UserTheme
    {
    public:
        explicit UserTheme(const std::filesystem::path&);
        const std::filesystem::path& path() const { return m_path; }
        static void loadTheme(const Dom::Value<AppTheme>&, AppTheme&);
        const AppTheme& theme() const { return m_theme; }
    private:
        const std::filesystem::path m_path;
        AppTheme m_theme{};
    };

    export using UserThemePtr = std::unique_ptr<UserTheme>;
    export using UserThemeList = std::vector<UserThemePtr>;

    export class ThemesManager
    {
    public:
        class IListener
        {
            friend ThemesManager;
        public:
            virtual ~IListener() = default;
        protected:
            virtual void themesManagerChanged() = 0;
        };
    public:
        explicit ThemesManager(const std::filesystem::path& directory);
    public:
        std::set<IListener*>& listeners() { return m_listeners; }
        [[nodiscard]] const std::filesystem::path& directory() const { return m_directory; }
        const UserThemeList& userThemes();
        const AppTheme* themeByName(std::wstring_view) const;
        // The theme a path names - see AppTheme - and nullptr where nothing stands under it. A
        // built-in is answered from the compiled-in colours; a User path reads the directory, so
        // a path names its theme on the first ask.
        [[nodiscard]] const AppTheme* themeByPath(std::wstring_view);
        // The path a user theme is known by: the User root over its file's stem.
        [[nodiscard]] static std::wstring pathOf(const UserTheme&);
        void saveTheme(std::wstring_view themeName, Dom::Value<AppTheme>&) const;
        static void saveTheme(const AppTheme&, Dom::Value<AppTheme>&);
        bool needDirectory();
    private:
        struct ParsedNameForSort
        {
            std::wstring_view name;
            int num;
        };
        ParsedNameForSort nameForSort(std::wstring_view sourceName);
        // Puts the watch over a directory that was made after the watch was set up. See Application
        void watchDirectory();
        void update();
        void notifyListeners();
    private:
        AppTheme m_defaultTheme{ defaultTheme() };

        std::filesystem::path m_directory;
        DirWatch m_dirWatcher{ m_directory };
        ScopedEventConnection m_dirWatchConnection{};
        bool m_loaded{};
        UserThemeList m_userThemes{};
        std::set<IListener*> m_listeners{};
    };


    //----------------------------------------------------------------------------------------


    // UserTheme

    UserTheme::UserTheme(const std::filesystem::path& path)
        :
        m_path{ path }
    {
        Dom::Value<AppTheme> section{nullptr, {}};
        Dom::FileFormat::ClaFi ff;
        ff.loadSectionFromFile(section, path);
        loadTheme(section, m_theme);
    }

    void UserTheme::loadTheme(const Dom::Value<AppTheme>& configNode, AppTheme& theme)
    {
        configNode.getTo(theme);
    }

    // ThemesManager

    ThemesManager::ThemesManager(const std::filesystem::path& directory)
        :
        m_directory{ directory }
    {
        m_dirWatchConnection = m_dirWatcher.onChange([this](DirWatchChangeEvent&){
            m_loaded = false;
            notifyListeners();
        });
    }

    const UserThemeList& ThemesManager::userThemes()
    {
        watchDirectory();
        update();
        return m_userThemes;
    }

    const AppTheme* ThemesManager::themeByName(std::wstring_view themeName) const
    {
        if (k_defaultThemeName == themeName)
            return &m_defaultTheme;

        for (const UserThemePtr& ptr : m_userThemes)
            if (ptr->path().filename() == themeName)
                return &ptr->theme();

        return nullptr;
    }

    const AppTheme* ThemesManager::themeByPath(const std::wstring_view path)
    {
        const std::wstring_view root = themePathRoot(path);
        const std::wstring_view name = themePathName(path);
        if (root == ThemeRoots::builtIn)
            return builtInTheme(name);
        if (root != ThemeRoots::user)
            return nullptr;

        for (const UserThemePtr& theme : userThemes())
            if (name == theme->path().stem().wstring())
                return &theme->theme();
        return nullptr;
    }

    std::wstring ThemesManager::pathOf(const UserTheme& theme)
    {
        return themePathOf(ThemeRoots::user, theme.path().stem().wstring());
    }

    void ThemesManager::saveTheme(std::wstring_view themeName, Dom::Value<AppTheme>& configNode) const
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

    bool ThemesManager::needDirectory()
    {
        if (std::filesystem::exists(m_directory))
            return false;
        std::filesystem::create_directories(m_directory);
        m_dirWatcher.restart();
        return true;
    }

    ThemesManager::ParsedNameForSort ThemesManager::nameForSort(std::wstring_view sourceName)
    {
        constexpr wchar_t k_firstNumChar = L'0';
        constexpr wchar_t k_lastNumChar = L'9';
        constexpr wchar_t k_closingBracket = L')';

        // searching for the last non digit
        std::size_t i = sourceName.size();
        if (!i)
            return { sourceName , 0};
        do --i;
        while (i && (inRange(sourceName[i], k_firstNumChar, k_lastNumChar) || sourceName[i] == k_closingBracket));

        // There's no number at the end
        if (i == sourceName.size() - 1)
            return { sourceName, 0 };

        // Ending number found
        wchar_t* endptr{};
        return {
            .name = std::wstring_view{ sourceName.data(), i + 1},
            .num = static_cast<int>(std::wcstol(sourceName.data() + i + 1, &endptr, 10))
        };
    }

    void ThemesManager::watchDirectory()
    {
        if (m_dirWatcher.watching())
            return;
        if (!std::filesystem::exists(m_directory))
            return;
        m_dirWatcher.restart();
        // Whatever the list holds was read while nothing was watching, so it is read again.
        m_loaded = false;
    }

    void ThemesManager::update()
    {
        if (m_loaded)
            return;
        m_loaded = true;
        m_userThemes.clear();
        if (std::filesystem::exists(m_directory))
        {
            // The theme extension is what makes a directory entry a theme.
            for (const auto& entry : std::filesystem::directory_iterator(m_directory))
            {
                if (entry.path().extension() != k_themeFileExtension)
                    continue;
                m_userThemes.push_back(std::make_unique<UserTheme>(entry.path()));
            }
        }

        std::ranges::sort(m_userThemes, [this](const UserThemePtr& alpha, UserThemePtr& b) {
            std::wstring strA = alpha->path().stem().wstring();
            std::wstring strB = b->path().stem().wstring();
            ParsedNameForSort dataA = nameForSort(strA);
            ParsedNameForSort dataB = nameForSort(strB);
            if (dataA.name == dataB.name)
                return dataA.num < dataB.num;
            else
                return dataA.name < dataB.name;
            });
    }

    void ThemesManager::notifyListeners()
    {
        for (IListener* listener : m_listeners)
            listener->themesManagerChanged();
    }

}
