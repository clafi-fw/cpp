module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.App.ThemesList;

import ClaFi.App.Themes;
import ClaFi.Application.ThemesManager;

import ClaFi.Documents.Folder;
import ClaFi.Documents.List;

import ClaFi.Controls.Base.StackBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;
    using namespace Documents;

    // What a theme tile stands for beyond its file, stated when it is built.
    export struct ThemeTileData
    {
        const UserTheme* theme{ nullptr };
    };

    // A theme in a list: the palette inset in its own surface, its name under it. See Application
    export class ThemeTile : public DocumentTile
    {
    public:
        template<typename... Args>
        explicit ThemeTile(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"ThemeTile"; }
        [[nodiscard]] const AppTheme& theme() const { return m_userTheme.theme(); }
        [[nodiscard]] const UserTheme* userTheme() const { return &m_userTheme; }
    protected:
        void drawIcon(DrawIconEvent&) override;
        void paintSurface(PaintEvent&) override;
        void adjustChildPaint(AdjustPaintEvent&) override;
    private:
        const UserTheme& m_userTheme;
    };

    // The theme files and storage folders as tiles. See Application
    export class ThemesList : public StackView, public DocumentsFolder::IListener,
        public IDocumentTiles
    {
    public:
        template<typename... Args>
        explicit ThemesList(const CreateParams&, Args&&...);
        ~ThemesList() override;
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"ThemesList"; }
        [[nodiscard]] StackView& view() override { return *this; }
        [[nodiscard]] DocumentTiles tiles() override;
        [[nodiscard]] DocumentTile* tileByFileName(std::wstring_view) override;
        [[nodiscard]] DocumentTile* currentTile() const override;
        [[nodiscard]] std::wstring currentFileName() const override;
        void setCurrentFileName(std::wstring_view) override;
        // Opens the user group as well: the file named is the user's, and a tile made into a
        // closed group is never laid out, so it has no rect to be scrolled to or renamed over.
        void selectFileNameAfterRebuild(std::wstring_view) override;
        // The theme being looked at: the tile previewed, and the one the list stands on until
        // something has been. Null on an empty list.
        [[nodiscard]] const AppTheme* previewedTheme() const;
        // Builds every tile from the themes as they stand now, and comes back to the file the
        // list was on. See Application
        void rebuild() override;
    protected:
        void documentsFolderChanged() override { rebuild(); }
    private:
        // **A group is a section that can be put away.** Each expander heads the panel its tiles
        // stand in, and that panel is its body - so the group and what the group holds are one
        // control, and collapsing the header takes the tiles with it. The view reaches items
        // through the whole subtree, so a tile is no less selectable for standing a level deeper.
        //
        // VerticalAlign::Top **is what lets a group grow.** A wrapping panel measures itself before
        // it knows the width it will be handed, so it reports one lane and wraps into as many as
        // it needs once the width arrives - and the group holding it has to follow. A panel
        // aligned Fill has its height dictated from outside and keeps the one lane, cutting off
        // every row after the first.
        using ThemesGroup = ExpanderWith<StackPanel>;
    private:
        ThemeTile& addTile(StackPanel& group, const UserTheme&);
    private:
        ThemesManager& m_themes;
        DocumentStorageView m_storage;
        const StorageTiles* m_storages;
        UserThemeList m_storageThemes{};
        // What a tile's own name is kept in, and nothing where the list does not rename. A tile
        // with no handler connected offers no Rename at all - see WithInPlaceEdit.
        DocumentEditHandler m_editTheme{};
        // What a tile raises a menu with, and nothing where the list has no commands to offer.
        DocumentMenuHandler m_tileMenu{};
        // The file to stand on after the next rebuild, named rather than held as a tile: the
        // tiles the list holds now are the ones the rebuild takes down.
        std::wstring m_fileNameToSelect{};

        ThemesGroup& m_userGroup{ add<ThemesGroup>(
            HostProps{
                VerticalAlign::Top,
                ExpanderViewMode::Divider,
                HeaderText{ ColoredText{ InkGrade::Strong, m_storages ? L"Storages" : L"Additional Themes" } }
            },
            BodyProps{
                Orientation::HorizontalWrap,
                ItemSizing::Equal,
                Padding{ 12.0f, 4.0f },
                Spacing{ 4.0f },
                PlaceHolderText{ ColoredText{ InkGrade::Subtle, L"No items" } }
            }
        ) };
        StackPanel& m_userTiles{ m_userGroup.body() };
    };


    //-----------------------------------------------------------------------------


    constexpr float k_groupSpacing{ 8.0f };

    // ThemeTile

    template<typename ...Args>
    ThemeTile::ThemeTile(const CreateParams& params, Args&&... args)
        :
        DocumentTile{ params, std::forward<Args>(args)... },
        // The storage must keep the parsed theme alive until the tile is rebuilt.
        m_userTheme{ *REQUIRE_PROPERTY(ThemeTileData).theme }
    {
    }

    // ThemesList

    template<typename... Args>
    ThemesList::ThemesList(const CreateParams& params, Args&&... args)
        :
        StackView{
            params,
            Orientation::Vertical,
            Spacing{ k_groupSpacing },
            // **What a list of themes is for is picking one**, so a stated selection mode is what
            // makes it more than that. Said ahead of the caller's own props, which are read
            // after these and stand in their place - see Props::get.
            SelectionMode::None,
            PreviewMode::Focus,
            DragMode::EasySelect,
            std::forward<Args>(args)...
        },
        // The application's own manager: a themes list is built over nothing else, whatever
        // folder prop a host hands its tiles.
        m_themes{ appThemes() },
        // The application theme picker uses the primary view when it has no browser storage.
        m_storage{ READ_PROPERTY(DocumentStorageView, (DocumentStorageView{ &m_themes, true })) },
        // Browser home supplies one tile per configured folder.
        m_storages{ READ_PROPERTY(StorageTilesView, StorageTilesView{}).tiles },
        // The host's answer to a name typed over a tile, where it wants one.
        m_editTheme{ READ_PROPERTY(DocumentEditHandler, nullptr) },
        // The host's answer to a tile raising its menu, where it wants one.
        m_tileMenu{ READ_PROPERTY(DocumentMenuHandler, nullptr) }
    {
        m_storage.folder->listeners().insert(this);
        onCanFocusItem([this](CanFocusItemEvent& event) {
            event.canFocus = event.item.parent() == &m_userTiles;
        });
        onCanSelectItem([this](CanSelectItemEvent& event) {
            event.canSelect = event.item.parent() == &m_userTiles;
        });

        rebuild();
    }
}
