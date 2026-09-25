export module ClaFi.App.ThemesList;

import ClaFi.App.ThemeIcon;
import ClaFi.App.Themes;
import ClaFi.Application.ThemesManager;

import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;
import ClaFi.Controls.TextItems;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    // Named ahead of ThemesRebuiltEvent, which carries one. EXPORTED HERE TOO: a declaration may
    // not be exported where an earlier one was not, and this is the earlier one.
    export class ThemesList;

    // What a tile stands for, stated when it is built. See Application
    export struct ThemeTileData
    {
        const AppTheme* theme;      // the theme itself, which outlives the tile
        std::wstring_view path;     // the path it is known by - see AppTheme
        const UserTheme* userTheme; // the file it came from, and nullptr for a built-in
    };

    // A theme in a list: the palette inset in the theme's own surface, its name under it. The
    // caption IS the name the theme goes by, so an editor over it is a rename. See Application
    export class ThemeTile : public WithInPlaceEdit<Button>
    {
    public:
        template<typename... Args>
        explicit ThemeTile(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"ThemeTile"; }
        [[nodiscard]] const AppTheme& theme() const { return m_linkedTheme; }
        [[nodiscard]] const std::wstring& themePath() const { return m_themePath; }
        // The file this tile came from, and nullptr for a built-in: a compiled-in theme is
        // answered whatever stands on the disk, so there is no file behind it to work on.
        [[nodiscard]] const UserTheme* userTheme() const { return m_userTheme; }
    protected:
        [[nodiscard]] EditorMode editorMode() const override;
        void paintIcon(PaintIconEvent&) override;
    private:
        using Base = WithInPlaceEdit<Button>;
    private:
        const AppTheme& m_linkedTheme;
        const std::wstring m_themePath{};
        const UserTheme* m_userTheme{ nullptr };
    };

    // A name has been typed over a tile, for a host that owns what the name is kept in.
    // See Application
    export using ThemeEditHandler = std::function<void(ThemeTile&, AcceptEditEvent&)>;

    // A tile is about to raise a menu. Connected to the tile rather than reached from the list: a
    // context popup is answered where it is raised and travels no further. See Application
    export using ThemeMenuHandler = std::function<void(ThemeTile&, ContextPopupEvent&)>;

    // The list has built its tiles afresh, and stands on whichever it was asked to. See Application
    export struct ThemesRebuiltEvent : public Event
    {
        explicit ThemesRebuiltEvent(ThemesList& list);
        ThemesList& list;   // the list that was rebuilt
    };

    // The themes an application can wear, built-in and user, each group its own section - the
    // view alone, with no box: what scrolls the themes is the host's. See Application
    export class ThemesList : public StackView, public ThemesManager::IListener
    {
    public:
        template<typename... Args>
        explicit ThemesList(const CreateParams&, Args&&...);
        ~ThemesList() override;
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"ThemesList"; }
        // The view the tiles stand in, which is the list itself. Kept as a name so a host asking
        // for the selection, the current item or the preview says which of the two it means.
        [[nodiscard]] StackView& view() { return *this; }
        // The tile standing under this path, and nullptr where the list holds none.
        [[nodiscard]] ThemeTile* tileByPath(std::wstring_view) const;
        // The tile the current item is on, which is the theme the list stands for.
        [[nodiscard]] ThemeTile* currentTile() const;
        // The theme being looked at: the tile previewed, and the one the list stands on until
        // something has been. Null on an empty list.
        [[nodiscard]] const AppTheme* previewedTheme() const;
        // The path the list stands on, and nothing where it stands on no tile.
        [[nodiscard]] std::wstring currentPath() const;
        void setCurrentPath(std::wstring_view);
        // The path to stand on once the tiles have been built afresh - for a theme whose file is
        // written but whose tile does not exist yet. Spent by the rebuild that reads it.
        void selectPathAfterRebuild(std::wstring_view value) { m_pathToSelect = value; }
        // The panel the user themes stand in, for a host walking them in the order they are
        // listed - what stands where a selection has been taken away, and what follows it.
        [[nodiscard]] StackPanel& userTiles() { return m_userTiles; }
        // Opens the user group. A tile made into a closed group is never laid out, so it has no
        // rect to be scrolled to, selected on or renamed over.
        void expandUserGroup();
        // Builds every tile from the themes as they stand now, and comes back to the path the
        // list was on. See Application
        void rebuild();
    protected:
        void themesManagerChanged() override { rebuild(); }
    private:
        // A GROUP IS A SECTION THAT CAN BE PUT AWAY. Each expander heads the panel its tiles
        // stand in, and that panel is its body - so the group and what the group holds are one
        // control, and collapsing the header takes the tiles with it. The view reaches items
        // through the whole subtree, so a tile is no less selectable for standing a level deeper.
        //
        // VerticalAlign::Top IS WHAT LETS A GROUP GROW. A wrapping panel measures itself before
        // it knows the width it will be handed, so it reports one lane and wraps into as many as
        // it needs once the width arrives - and the group holding it has to follow. A panel
        // aligned Fill has its height dictated from outside and keeps the one lane, cutting off
        // every row after the first.
        using ThemesGroup = ExpanderWith<StackPanel>;
    private:
        ThemeTile& addTile(StackPanel& group, const AppTheme&, std::wstring_view path,
            std::wstring_view name, const UserTheme*);
        // Whether a tile in this group may be taken into a selection, which is what decides
        // between a check mark and none.
        [[nodiscard]] bool isSelectableGroup(const StackPanel& group) const;
    private:
        // What a tile's own name is kept in, and nothing where the list does not rename. A tile
        // with no handler connected offers no Rename at all - see WithInPlaceEdit.
        ThemeEditHandler m_editTheme{};
        // What a tile raises a menu with, and nothing where the list has no commands to offer.
        ThemeMenuHandler m_tileMenu{};
        // How large a tile draws its theme, and how it lays that picture out against the name.
        // Stated for the list rather than for a tile, since every tile in one list shares them.
        IconSize m_tileIconSize;
        ButtonViewMode m_tileViewMode;
        // The path to stand on after the next rebuild, named rather than held as a tile: the
        // tiles the list holds now are the ones the rebuild takes down.
        std::wstring m_pathToSelect{};

        ThemesGroup& m_builtInGroup{ add<ThemesGroup>(
            HostProps{
                VerticalAlign::Top,
                ExpanderViewMode::Divider,
                HeaderText{ InkGrade::Strong, L"Built-in Themes" }
            },
            BodyProps{
                Orientation::HorizontalWrap,
                ItemSizing::Equal,
                Padding{ 12.0f, 4.0f },
                Spacing{ 4.0f }
            }
        ) };
        StackPanel& m_builtInTiles{ m_builtInGroup.body() };

        ThemesGroup& m_userGroup{ add<ThemesGroup>(
            HostProps{
                VerticalAlign::Top,
                ExpanderViewMode::Divider,
                HeaderText{ InkGrade::Strong, L"Additional Themes" }
            },
            BodyProps{
                Orientation::HorizontalWrap,
                ItemSizing::Equal,
                Padding{ 12.0f, 4.0f },
                Spacing{ 4.0f },
                PlaceHolderText{ InkGrade::Subtle, L"No items" }
            }
        ) };
        StackPanel& m_userTiles{ m_userGroup.body() };
    };


    //-----------------------------------------------------------------------------


    // The tile's picture, which is the whole of what a theme looks like before it is worn. A
    // window is wider than it is tall, and so is the picture of one.
    constexpr float k_themeTileIconHeight{ 64.0f };
    constexpr float k_themeTileIconWidth{ k_themeTileIconHeight * 1.6f };
    constexpr IconSize k_themeTileIconSize{ k_themeTileIconWidth, k_themeTileIconHeight };
    constexpr ButtonViewMode k_themeTileViewMode{ ButtonViewMode::TopCenterIcon };
    constexpr float k_themeTilePadding{ 4.0f };
    constexpr float k_tileSpacing{ 8.0f };

    // ThemeTile

    template<typename ...Args>
    ThemeTile::ThemeTile(const CreateParams& params, Args&&... args)
        :
        Base{
            params,
            // Not a tool, so not a ToolButton - but it wears the same look, and the property is
            // the whole of that look.
            ShowSurfaceAtRest::No,
            ShowSelectionOnSurface::Yes,
            IndicatorVisibility::Hover,
            IndicatorPlacement::TopLeftIn,
            k_themeTileViewMode,
            k_themeTileIconSize,
            HorizontalTextAnchor::Center,
            // A TILE TAKES THE SHARE OF THE LANE IT IS HANDED, which is what makes the pictures
            // line up: the lane divides evenly, so every tile comes out the same width whatever
            // it is called, the picture stands at its stated size in the middle of that width,
            // and a long name wraps under it rather than widening the tile. See Item-Containers
            HorizontalAlign::Fill,
            std::forward<Args>(args)...
        },
        m_linkedTheme{ *Props::find<ThemeTileData>(args...)->theme },
        m_themePath{ Props::find<ThemeTileData>(args...)->path },
        m_userTheme{ Props::find<ThemeTileData>(args...)->userTheme }
    {
        setPadding(k_themeTilePadding);
    }

    // ThemesList

    template<typename... Args>
    ThemesList::ThemesList(const CreateParams& params, Args&&... args)
        :
        StackView{
            params,
            Orientation::Vertical,
            Spacing{ k_tileSpacing },
            // WHAT A LIST OF THEMES IS FOR IS PICKING ONE, so a stated selection mode is what
            // makes it more than that. Said ahead of the caller's own props, which are read
            // after these and stand in their place - see Props::get.
            SelectionMode::None,
            PreviewMode::Focus,
            DragMode::EasySelect,
            std::forward<Args>(args)...
        },
        m_editTheme{ Props::get<ThemeEditHandler>({}, args...) },
        m_tileMenu{ Props::get<ThemeMenuHandler>({}, args...) },
        m_tileIconSize{ Props::get<ItemsIconSize>(ItemsIconSize{ k_themeTileIconSize }, args...) },
        m_tileViewMode{ Props::get<ItemsViewMode>(ItemsViewMode{ k_themeTileViewMode }, args...).value }
    {
        appThemes().listeners().insert(this);

        onCanFocusItem([this](CanFocusItemEvent& event) {
            event.canFocus = event.item.parent() == &m_userTiles
                || event.item.parent() == &m_builtInTiles;
        });
        // ONLY A FILE CAN BE HELD SELECTED. What a selection is asked for here is a command over
        // the themes - deleting them - and a built-in answers no such command.
        onCanSelectItem([this](CanSelectItemEvent& event) {
            event.canSelect = event.item.parent() == &m_userTiles;
        });

        rebuild();
    }

}
