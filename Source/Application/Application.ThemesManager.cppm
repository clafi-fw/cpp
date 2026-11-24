export module ClaFi.Application.ThemesManager;

export import ClaFi.Application.ThemesManager_Serializers;

import ClaFi.App.ApplicationProperties;

import ClaFi.Documents.Folder;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    export constexpr std::wstring_view k_themeDataAttrName{ L"ThemeData" };

    export constexpr std::wstring_view k_themeFileExtension = L".clafitheme";

    // What a theme is called and how its files are named - see DocumentKind.
    export constexpr Documents::DocumentKind k_themeKind{
        .extension = k_themeFileExtension,
        .noun = L"theme",
        .nounPlural = L"themes",
        .newStem = L"New Theme",
        .attrName = k_themeDataAttrName
    };

    // A theme read from a file in the themes folder, kept beside the file it came from.
    export class UserTheme
    {
    public:
        explicit UserTheme(const std::filesystem::path&, const AppTheme& defaults = {});
        UserTheme(const std::filesystem::path& path, const Documents::DocumentsFolder& folder);
    public:
        [[nodiscard]] bool readable() const { return m_readable; }
        [[nodiscard]] const std::filesystem::path& path() const { return m_path; }
        [[nodiscard]] const AppTheme& theme() const { return m_theme; }
        static void loadTheme(const Dom::Value<AppTheme>&, AppTheme&);
    private:
        const std::filesystem::path m_path;
        AppTheme m_theme{};
        bool m_readable{ false };
    };

    export using UserThemePtr = std::unique_ptr<UserTheme>;
    export using UserThemeList = std::vector<UserThemePtr>;

    export class ThemeFolder : public Documents::DocumentsFolder
    {
    public:
        ThemeFolder(const std::filesystem::path& directory, const AppTheme* legacyThemeFileDefaults);
    public:
        [[nodiscard]] const AppTheme& themeFileDefaults() const { return m_themeFileDefaults; }
        [[nodiscard]] const UserThemeList& userThemes(); // all files, parsed in folder order
        [[nodiscard]] const AppTheme* themeByName(std::wstring_view) const;
        void saveTheme(std::wstring_view themeName, Dom::Value<AppTheme>&) const;
        static void saveTheme(const AppTheme&, Dom::Value<AppTheme>&);
        [[nodiscard]] std::unique_ptr<Dom::Section> statedTheme(const Dom::Section&) const;
        [[nodiscard]] bool readDocument(const std::filesystem::path&,
            Dom::DomNodeBase& into) const override;
        [[nodiscard]] bool writeDocument(const Dom::DomNodeBase&,
            const std::filesystem::path&) const override;
        [[nodiscard]] bool isEdited(const std::filesystem::path&) const override;
        void drawIcon(std::wstring_view fileName, DrawIconEvent&) override;
    protected:
        void filesRead(const Documents::FilePaths&) override;
    private:
        const AppTheme m_themeFileDefaults;
        UserThemeList m_userThemes{};
    };

    // The application's declared and discovered themes. See Application
    export class ThemesManager : public ThemeFolder
    {
    public:
        ThemesManager(AppContext& context, const std::filesystem::path& publisherDirectory,
            AppElementNames requiredElements,
            const NamedTheme& defaultTheme, const NamedThemes& additionalThemes = {},
            const AppTheme* legacyThemeFileDefaults = nullptr,
            const std::filesystem::path& appConfigDirectory = {},
            const std::filesystem::path& appExecutableDirectory = {});
    public:
        [[nodiscard]] const NamedTheme& defaultTheme() const { return m_defaultTheme; }
        [[nodiscard]] const NamedThemes& additionalThemes() const { return m_additionalThemes; }
        [[nodiscard]] ThemeFolder* fileThemeFolder(std::wstring_view root);
        [[nodiscard]] bool acceptsTheme(const AppTheme&) const;
        [[nodiscard]] bool offersTheme(std::wstring_view root, const UserTheme&) const;
        [[nodiscard]] const AppTheme* themeByPath(std::wstring_view path);
        // The Settings path of a publisher theme. See Application#themesmanager
        [[nodiscard]] static std::wstring pathOf(const UserTheme&);
        [[nodiscard]] ColorMode resolvedColorMode() const;
        void setColorMode(ColorModeSetting mode);
        void selectTheme(std::wstring_view path);
        void restoreSelectedTheme();
        void applyTheme(const AppTheme& theme);
        void applyTheme(const AppTheme& theme, ColorMode mode);
        [[nodiscard]] const AppTheme& selectedTheme();
    private:
        AppContext& m_context;
        const AppElementNames m_requiredElements;
        const NamedTheme m_defaultTheme;
        const NamedThemes m_additionalThemes;
        ThemeFolder m_appConfigThemes;
        ThemeFolder m_appExecutableThemes;
        ScopedEventConnection m_themePathConnection{};
        ScopedEventConnection m_colorModeConnection{};
        ScopedEventConnection m_systemColorModeConnection{};
    };
}
