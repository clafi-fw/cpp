module ClaFi.Documents.List;

import ClaFi.Documents.Folder;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.StackView;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    // DocumentTile

    std::wstring DocumentTile::fileName() const
    {
        return m_path.filename().wstring();
    }

    std::wstring DocumentTile::stem() const
    {
        return m_path.stem().wstring();
    }

    void DocumentTile::paintIcon(PaintIconEvent& event)
    {
        m_folder.paintIcon(fileName(), event);
    }

    // DocumentsRebuiltEvent

    DocumentsRebuiltEvent::DocumentsRebuiltEvent(DocumentsList& list)
        :
        list{ list }
    {
    }

    // DocumentsList

    DocumentsList::~DocumentsList()
    {
        m_folder.listeners().erase(this);
    }

    DocumentTile* DocumentsList::tileByFileName(const std::wstring_view fileName)
    {
        for (DocumentTile& tile : tiles())
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

    // THE FILE NAME IS WHAT THE LIST COMES BACK TO, not the tile: every tile here is taken down
    // and built again, and a document is the same document under the same name.
    void DocumentsList::rebuild()
    {
        const std::wstring fileNameToStandOn = m_fileNameToSelect.empty()
            ? currentFileName()
            : m_fileNameToSelect;
        m_fileNameToSelect.clear();

        clearControls();
        for (const std::filesystem::path& file : m_folder.files())
            addTile(file);

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
