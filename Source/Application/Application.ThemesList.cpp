module ClaFi.App.ThemesList;

import ClaFi.App.ThemeIcon;
import ClaFi.Application.ThemesManager;

import ClaFi.Documents.Folder;
import ClaFi.Documents.List;

import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;
    using namespace Documents;

    // ThemeTile

    EditorMode ThemeTile::editorMode() const
    {
        // Only a user theme is a file, and only a file has a name to change.
        if (!m_userTheme)
            return EditorMode::None;
        return DocumentTile::editorMode();
    }

    void ThemeTile::paintIcon(PaintIconEvent& event)
    {
        paintThemeIconOnly(event.canvas(), event.iconRect(), false,
            ThemeSampleColors{ m_linkedTheme.colors, colorModeOf(event.lightness()) }
            );
    }

    void ThemeTile::paintSurface(PaintEvent& event)
    {
        FloatRect rect = event.controlBounds();
        paintThemeBackground(event.canvas(), rect, event.scaleF(4.0f),
            ThemeSampleColors{ m_linkedTheme.colors, colorModeOf(event.lightness()) },
            event.scaledStrokeWidth(Thickness::Thin)
            );
        paintIconLayer(event);
        event.defaultPaintSurface(0.0f, true);
    }

    // ThemesList

    ThemesList::~ThemesList()
    {
        m_themes.listeners().erase(this);
    }

    DocumentTiles ThemesList::tiles()
    {
        DocumentTiles result{};
        for (StackPanel* group : { &m_builtInTiles, &m_userTiles })
            for (ThemeTile& tile : group->controlsAs<ThemeTile>())
                result.push_back(&tile);
        return result;
    }

    DocumentTile* ThemesList::tileByFileName(const std::wstring_view fileName)
    {
        for (StackPanel* group : { &m_builtInTiles, &m_userTiles })
            for (ThemeTile& tile : group->controlsAs<ThemeTile>())
                if (tile.fileName() == fileName)
                    return &tile;
        return nullptr;
    }

    // Only a tile can be the current item - canFocusItem admits nothing else here - so the cast
    // is the same one previewedTheme makes.
    DocumentTile* ThemesList::currentTile() const
    {
        return static_cast<ThemeTile*>(currentItem());
    }

    std::wstring ThemesList::currentFileName() const
    {
        if (const DocumentTile* tile = currentTile())
            return tile->fileName();
        return {};
    }

    void ThemesList::setCurrentFileName(const std::wstring_view fileName)
    {
        if (DocumentTile* tile = tileByFileName(fileName))
            setCurrentItem(*tile);
    }

    void ThemesList::selectFileNameAfterRebuild(const std::wstring_view fileName)
    {
        m_fileNameToSelect = fileName;
        m_userGroup.header().setExpanded(true);
    }

    const AppTheme* ThemesList::previewedTheme() const
    {
        Control* item = previewItem();
        if (!item)
            item = currentItem();
        if (!item)
            return nullptr;
        return &static_cast<ThemeTile*>(item)->theme();
    }

    // THE FILE NAME IS WHAT THE LIST COMES BACK TO, not the tile: every tile here is taken down
    // and built again, and a theme is the same theme under the same name. The built-in goes by
    // the name its page goes by, so a tab coming up from it lands on its tile.
    void ThemesList::rebuild()
    {
        const std::wstring fileNameToStandOn = m_fileNameToSelect.empty()
            ? currentFileName()
            : m_fileNameToSelect;
        m_fileNameToSelect.clear();

        m_builtInTiles.clearControls();
        m_userTiles.clearControls();

        addTile(m_builtInTiles, defaultTheme(), m_themes.fileOf(k_defaultThemeName), nullptr);
        for (const UserThemePtr& theme : m_themes.userThemes())
            addTile(m_userTiles, theme->theme(), theme->path(), &*theme);

        // A tile is as many lines of name as it needs inside a fixed width, so a list built
        // afresh is a new height. See the deferred-request rule: ask, do not lay out from here.
        form().invalidateAlign();
        setCurrentFileName(fileNameToStandOn);
        emitEvent<DocumentsRebuiltEvent>(*this);
    }

    ThemeTile& ThemesList::addTile(StackPanel& group, const AppTheme& theme,
        const std::filesystem::path& file, const UserTheme* userTheme)
    {
        ThemeTile& tile = group.add<ThemeTile>(
            DocumentTileData{ &m_themes, file },
            ThemeTileData{ &theme, userTheme },
            IndicatorStyle::Check,
            Text{ file.stem().wstring() }
        );
        if (m_editTheme && userTheme)
        {
            tile.onAcceptEdit([this, &tile](AcceptEditEvent& event) {
                m_editTheme(tile, event);
            });
        }
        if (m_tileMenu)
        {
            tile.onContextPopup([this, &tile](ContextPopupEvent& event) {
                m_tileMenu(tile, event);
            });
        }
        return tile;
    }
}
