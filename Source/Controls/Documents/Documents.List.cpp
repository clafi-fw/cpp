module ClaFi.Documents.List;

import ClaFi.Documents.Folder;

import ClaFi.Icons.FolderIcon;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    // DocumentTile

    std::wstring DocumentTile::fileName() const
    {
        return path().filename().wstring();
    }

    std::wstring DocumentTile::stem() const
    {
        return path().stem().wstring();
    }

    EditorMode DocumentTile::editorMode() const
    {
        return !storage() && folder().canWriteFile(path()) ? Base::editorMode() : EditorMode::None;
    }

    void DocumentTile::drawIcon(DrawIconEvent& event)
    {
        if (storage())
        {
            drawStorageIcon(event);
            return;
        }
        folder().drawIcon(fileName(), event);
    }

    void drawStorageIcon(DrawIconEvent& event)
    {
        const FloatRect rect = event.iconRect();
        const float size = std::min(rect.width(), rect.height());
        const FloatPoint center = rect.center();
        const FloatRect folder = {
            center.x - size * 0.43f,
            center.y - size * 0.35f,
            center.x + size * 0.43f,
            center.y + size * 0.32f
        };
        Graphics::PixelPath outline;
        Icons::FolderIcon::addOutline(outline, folder);
        event.canvas().fillPath(outline, event.inkRgb(InkWell::Orange));
    }

    // DocumentsRebuiltEvent

    DocumentsRebuiltEvent::DocumentsRebuiltEvent(IDocumentTiles& tiles)
        :
        tiles{ tiles }
    {
    }

    // DocumentsList

    DocumentsList::~DocumentsList()
    {
        m_folder.listeners().erase(this);
    }

    DocumentTiles DocumentsList::tiles()
    {
        DocumentTiles result{};
        for (DocumentTile& tile : controlsAs<DocumentTile>())
            result.push_back(&tile);
        return result;
    }

    DocumentTile* DocumentsList::tileByFileName(const std::wstring_view fileName)
    {
        for (DocumentTile& tile : controlsAs<DocumentTile>())
            if (tile.fileName() == fileName)
                return &tile;
        return nullptr;
    }

    // Only a tile can be the current item - the list holds nothing else - so the cast holds.
    DocumentTile* DocumentsList::currentTile() const
    {
        return static_cast<DocumentTile*>(currentItem());
    }

    std::wstring DocumentsList::currentFileName() const
    {
        if (const DocumentTile* tile = currentTile())
            return tile->fileName();
        return {};
    }

    void DocumentsList::setCurrentFileName(const std::wstring_view fileName)
    {
        if (DocumentTile* tile = tileByFileName(fileName))
            setCurrentItem(*tile);
    }

    void DocumentsList::selectFileNameAfterRebuild(const std::wstring_view fileName)
    {
        m_fileNameToSelect = fileName;
    }

    // **The file name is what the list comes back to**, not the tile: every tile here is taken down
    // and built again, and a document is the same document under the same name.
    void DocumentsList::rebuild()
    {
        const std::wstring fileNameToStandOn = m_fileNameToSelect.empty()
            ? currentFileName()
            : m_fileNameToSelect;
        m_fileNameToSelect.clear();

        clearControls();
        if (m_storages)
        {
            for (const StorageTile& storage : *m_storages)
            {
                addStorageTile(*this, storage, m_tileMenu);
            }
        }
        else
        {
            for (const std::filesystem::path& file : m_folder.files())
            {
                addTile(file);
            }
        }

        // A tile is as many lines of name as it needs inside a fixed width, so a list built
        // afresh is a new height. Asked for, not laid out from here.
        form().invalidateAlign();
        setCurrentFileName(fileNameToStandOn);
        emitEvent<DocumentsRebuiltEvent>(*this);
    }

    DocumentTile& DocumentsList::addTile(const std::filesystem::path& path)
    {
        DocumentTile& tile = add<DocumentTile>(
            DocumentTileData{ &m_folder, path },
            IndicatorStyle::Check,
            Text{ path.stem().wstring() }
        );
        if (m_editDocument)
        {
            tile.onAcceptEdit([this, &tile](AcceptEditEvent& event) {
                m_editDocument(tile, event);
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
