export module ClaFi.Application.ThemesManager;

export import ClaFi.Application.ThemesManager_Serializers;

import ClaFi.Documents.Folder;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine;

import ClaFi.StdLib;

namespace ClaFi
{
    export constexpr std::wstring_view k_themeDataAttrName{ L"ThemeData" };

    export constexpr std::wstring_view k_defaultThemeName = L"Default.theme";

    export constexpr std::wstring_view k_themeFileExtension = L".clafitheme";
    export constexpr std::wstring_view k_builtInThemeExtension = L".theme";

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
        explicit UserTheme(const std::filesystem::path&);
    public:
        [[nodiscard]] const std::filesystem::path& path() const { return m_path; }
        [[nodiscard]] const AppTheme& theme() const { return m_theme; }
        static void loadTheme(const Dom::Value<AppTheme>&, AppTheme&);
    private:
        const std::filesystem::path m_path;
        AppTheme m_theme{};
    };

    export using UserThemePtr = std::unique_ptr<UserTheme>;
    export using UserThemeList = std::vector<UserThemePtr>;

    // The themes an application can wear - the built-in, and the user's own files. See Application
    export class ThemesManager : public Documents::DocumentsFolder
    {
    public:
        explicit ThemesManager(const std::filesystem::path& directory);
    public:
        // The user's themes, parsed, in the folder's order - read on the first ask after the
        // folder has changed.
        [[nodiscard]] const UserThemeList& userThemes();
        // The theme a file name stands for - the built-in's, or one of the user's - and nullptr
        // where nothing does. Answered from what was last read, so a caller that may be first
        // asks for userThemes ahead of it.
        [[nodiscard]] const AppTheme* themeByName(std::wstring_view) const;
        // The theme a path names - see AppTheme - and nullptr where nothing stands under it. A
        // built-in is answered from the compiled-in colours; a User path reads the directory, so
        // a path names its theme on the first ask.
        [[nodiscard]] const AppTheme* themeByPath(std::wstring_view);
        // The path a user theme is known by: the User root over its file's stem.
        [[nodiscard]] static std::wstring pathOf(const UserTheme&);
        // Whether a stem is a built-in's, which a user theme may not take. Case is not part of
        // the answer: the file system does not tell `dark` from `Dark`.
        [[nodiscard]] static bool isReservedName(std::wstring_view stem);
        void saveTheme(std::wstring_view themeName, Dom::Value<AppTheme>&) const;
        static void saveTheme(const AppTheme&, Dom::Value<AppTheme>&);
        // What a theme states: the tree the framework would build stands beside the theme's own,
        // and what differs is the answer. A file states the same, so a theme carrying nothing of
        // its own comes back as the defaults.
        [[nodiscard]] static std::unique_ptr<Dom::Section> statedTheme(const Dom::Section&);
        [[nodiscard]] bool readDocument(const std::filesystem::path&,
            Dom::DomNodeBase& into) const override;
        [[nodiscard]] bool writeDocument(const Dom::DomNodeBase&,
            const std::filesystem::path&) const override;
        [[nodiscard]] bool isEdited(const std::filesystem::path&) const override;
        void paintIcon(std::wstring_view fileName, PaintIconEvent&) override;
        void checkNameShape(Controls::AcceptEditEvent&, std::wstring_view stem) const override;
    protected:
        void filesRead(const Documents::FilePaths&) override;
    private:
        AppTheme m_defaultTheme{ defaultTheme() };
        UserThemeList m_userThemes{};
    };
}
