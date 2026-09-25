export module ClaFi.App.ThemePick;

import ClaFi.App.ThemeIcon;
import ClaFi.App.Themes;
import ClaFi.Application.ThemesManager;

import ClaFi.Controls.ComboBox;
import ClaFi.Controls.TextItems;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    // A theme the picker lists: the colours its item is drawn from, and the path it is stated by.
    export struct ThemeEntry
    {
        const AppTheme* theme;
        std::wstring path;
    };

    // The picker's own list, held by a base so that it stands before the ComboBox reading it.
    export class ThemeItems
    {
    protected:
        TextItems m_themeItems{};
    };

    // The themes an application can wear, on one line - built-in first, then the user's own.
    // Looking at an item wears it, picking one states it in the config. See Application
    export class ThemePick : private ThemeItems, public ComboBox, public ThemesManager::IListener
    {
    public:
        template<typename... Args>
        explicit ThemePick(const CreateParams&, Args&&...);
        ~ThemePick() override;
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"ThemePick"; }
        // The path the picker stands on, and nothing where it stands on no theme.
        [[nodiscard]] std::wstring currentPath() const;
        void setCurrentPath(std::wstring_view);
        // Builds the items from the themes as they stand now, and comes back to the path the
        // picker was on. See Application
        void rebuild();
    protected:
        void themesManagerChanged() override;
        void showDropdown(Control& initiator) override;
    private:
        using ThemeEntries = std::vector<ThemeEntry>;
        [[nodiscard]] const ThemeEntry& entryOf(const TextItem&) const;
        [[nodiscard]] ItemIndexValue findPath(std::wstring_view) const;
        void addTheme(const AppTheme&, std::wstring_view path, std::wstring_view name);
    private:
        // Every theme the list holds, in the order it lists them. An item's tag is its index
        // here, so an item answers the theme it was built from.
        ThemeEntries m_themes{};
    };


    //-----------------------------------------------------------------------------


    // The size a theme is drawn at, on the face and on every item of the list.
    constexpr float k_themeIconExtent{ 24.0f };
    export constexpr IconSize k_themeIconSize{ k_themeIconExtent, k_themeIconExtent };
    constexpr ItemsIconSize k_themeItemIconSize{ k_themeIconExtent, k_themeIconExtent };
    // The most a lane of the list takes before it turns a second, the themes spread evenly
    // over the lanes that many needs.
    constexpr std::size_t k_themesPerLane{ 10 };

    // ThemePick

    template<typename... Args>
    ThemePick::ThemePick(const CreateParams& params, Args&&... args)
        :
        ThemeItems{},
        ComboBox{
            params,
            m_themeItems,
            // A THEME IS A PICTURE BEFORE IT IS A NAME, so the face and every item carry one.
            // Said ahead of the caller's own props, which are read after these and stand in
            // their place - see Props::get.
            ButtonViewMode::LeftIcon,
            k_themeIconSize,
            ItemsViewMode{ ButtonViewMode::LeftIcon },
            k_themeItemIconSize,
            ItemsLaneSize{ k_themesPerLane, LaneSizing::UpTo },
            std::forward<Args>(args)...
        }
    {
        appThemes().listeners().insert(this);

        onPaintItemIcon([this](PaintItemIconEvent& event) {
            paintThemeIcon(event, entryOf(event.item()).theme->colors);
        });
        // TRIED ON, NOT STATED. The colours are taken directly rather than through the config
        // node, so nothing is written for a theme the user is only looking at.
        onPreviewItem([this](PreviewItemEvent& event) {
            applyTheme(appContext(), *entryOf(event.item()).theme);
        });

        rebuild();
    }

}
