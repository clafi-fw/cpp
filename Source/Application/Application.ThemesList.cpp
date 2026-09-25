module ClaFi.App.ThemesList;

import ClaFi.App.ThemeIcon;
import ClaFi.App.Themes;
import ClaFi.Application.ThemesManager;

import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Expander;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    // ThemeTile

    EditorMode ThemeTile::editorMode() const
    {
        // Only a user theme is a file, and only a file has a name to change.
        if (!m_userTheme)
            return EditorMode::None;
        return Base::editorMode();
    }

    void ThemeTile::paintIcon(PaintIconEvent& event)
    {
        paintThemeIcon(event, m_linkedTheme.colors);
    }

    // ThemesRebuiltEvent

    ThemesRebuiltEvent::ThemesRebuiltEvent(ThemesList& list)
        :
        list{ list }
    {
    }

    // ThemesList

    ThemesList::~ThemesList()
    {
        appThemes().listeners().erase(this);
    }

    ThemeTile* ThemesList::tileByPath(const std::wstring_view path) const
    {
        for (StackPanel* group : { &m_builtInTiles, &m_userTiles })
            for (ThemeTile& tile : group->controlsAs<ThemeTile>())
                if (tile.themePath() == path)
                    return &tile;
        return nullptr;
    }

    // Only a tile can be the current item - canFocusItem admits nothing else here - so the cast
    // is the same one previewedTheme makes.
    ThemeTile* ThemesList::currentTile() const
    {
        return static_cast<ThemeTile*>(currentItem());
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

    std::wstring ThemesList::currentPath() const
    {
        if (const ThemeTile* tile = currentTile())
            return tile->themePath();
        return {};
    }

    void ThemesList::setCurrentPath(const std::wstring_view path)
    {
        if (ThemeTile* tile = tileByPath(path))
            setCurrentItem(*tile);
    }

    void ThemesList::expandUserGroup()
    {
        m_userGroup.header().setExpanded(true);
    }

    // THE PATH IS WHAT THE LIST COMES BACK TO, not the tile: every tile here is taken down and
    // built again, and a theme is the same theme under the same path.
    void ThemesList::rebuild()
    {
        std::wstring pathToStandOn = m_pathToSelect.empty() ? currentPath() : m_pathToSelect;
        m_pathToSelect.clear();

        m_builtInTiles.clearControls();
        m_userTiles.clearControls();

        addTile(m_builtInTiles, defaultTheme(), k_defaultThemePath, BuiltInThemes::defaultTheme,
            nullptr);

        for (const UserThemePtr& theme : appThemes().userThemes())
        {
            const std::wstring name = theme->path().stem().wstring();
            addTile(m_userTiles, theme->theme(), ThemesManager::pathOf(*theme), name, &*theme);
        }

        // A tile is as many lines of name as it needs inside a fixed width, so a list built
        // afresh is a new height. See the deferred-request rule: ask, do not lay out from here.
        form().invalidateAlign();
        setCurrentPath(pathToStandOn);
        emitEvent<ThemesRebuiltEvent>(*this);
    }

    ThemeTile& ThemesList::addTile(StackPanel& group, const AppTheme& theme,
        const std::wstring_view path, const std::wstring_view name, const UserTheme* userTheme)
    {
        ThemeTile& tile = group.add<ThemeTile>(
            ThemeTileData{ &theme, path, userTheme },
            IndicatorStyle::Check,
            m_tileViewMode,
            m_tileIconSize,
            Text{ std::wstring{ name } }
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

    bool ThemesList::isSelectableGroup(const StackPanel& group) const
    {
        return &group == &m_userTiles
            && selectionMode() == SelectionMode::Multi;
    }
}
