export module ClaFi.Documents.List;

import ClaFi.Documents.Folder;

import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;
import ClaFi.Controls.Base.Container;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    // What a tile stands for, stated when it is built.
    export struct DocumentTileData
    {
        DocumentsFolder* folder;        // the folder the file stands in, which draws the mark
        std::filesystem::path path;     // the file the tile stands for
    };

    // A document in the list: its mark over its name, which is the file's. See Documents#tiles
    export class DocumentTile : public WithInPlaceEdit<Button>
    {
    public:
        template<typename... Args>
        explicit DocumentTile(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"DocumentTile"; }
        [[nodiscard]] DocumentsFolder& folder() const { return m_folder; }
        [[nodiscard]] const std::filesystem::path& path() const { return m_path; }
        [[nodiscard]] std::wstring fileName() const;
        [[nodiscard]] std::wstring stem() const;
    protected:
        void paintIcon(PaintIconEvent&) override;
    private:
        using Base = WithInPlaceEdit<Button>;
    private:
        DocumentsFolder& m_folder;
        const std::filesystem::path m_path;
    };

    export using DocumentTiles = std::vector<DocumentTile*>;

    // A name has been typed over a tile, for a host that owns the file the name is kept on.
    export using DocumentEditHandler = std::function<void(DocumentTile&, AcceptEditEvent&)>;

    // A tile is about to raise a menu. Connected to the tile rather than reached from the list: a
    // context popup is answered where it is raised and travels no further.
    export using DocumentMenuHandler = std::function<void(DocumentTile&, ContextPopupEvent&)>;

    // What a home page works over: tiles keyed by file name, in a view. See Documents#tiles
    export class IDocumentTiles
    {
    public:
        virtual ~IDocumentTiles() = default;
    public:
        // The view the tiles stand in - the selection, the current item, the clicks.
        [[nodiscard]] virtual StackView& view() = 0;
        // Every tile, in the order it is listed.
        [[nodiscard]] virtual DocumentTiles tiles() = 0;
        // The tile standing for this file, and nullptr where the list holds none.
        [[nodiscard]] virtual DocumentTile* tileByFileName(std::wstring_view) = 0;
        // The tile the current item is on, which is the document the list stands for.
        [[nodiscard]] virtual DocumentTile* currentTile() const = 0;
        // The file the list stands on, and nothing where it stands on no tile.
        [[nodiscard]] virtual std::wstring currentFileName() const = 0;
        virtual void setCurrentFileName(std::wstring_view) = 0;
        // The file to stand on once the tiles have been built afresh - for a document whose file is
        // written but whose tile does not exist yet. Spent by the rebuild that reads it.
        virtual void selectFileNameAfterRebuild(std::wstring_view) = 0;
    };

    // The tiles have been built afresh, and stand on whichever they were asked to.
    export struct DocumentsRebuiltEvent : public Event
    {
        explicit DocumentsRebuiltEvent(IDocumentTiles& tiles);
        IDocumentTiles& tiles;   // the tiles that were rebuilt
    };

    // The documents in the folder, a tile each - the view alone, with no box. See Documents#tiles
    export class DocumentsList : public StackView, public DocumentsFolder::IListener,
        public IDocumentTiles
    {
    public:
        template<typename... Args>
        explicit DocumentsList(const CreateParams&, Args&&...);
        ~DocumentsList() override;
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"DocumentsList"; }
        [[nodiscard]] StackView& view() override { return *this; }
        [[nodiscard]] DocumentTiles tiles() override;
        [[nodiscard]] DocumentTile* tileByFileName(std::wstring_view) override;
        [[nodiscard]] DocumentTile* currentTile() const override;
        [[nodiscard]] std::wstring currentFileName() const override;
        void setCurrentFileName(std::wstring_view) override;
        void selectFileNameAfterRebuild(std::wstring_view) override;
        // Builds every tile from the folder as it stands now, and comes back to the file the list
        // was on.
        void rebuild();
    protected:
        void documentsFolderChanged() override { rebuild(); }
    private:
        DocumentTile& addTile(const std::filesystem::path&);
    private:
        DocumentsFolder& m_folder;
        // What a tile's own name is kept on, and nothing where the list does not rename. A tile
        // with no handler connected offers no Rename at all - see WithInPlaceEdit.
        DocumentEditHandler m_editDocument{};
        // What a tile raises a menu with, and nothing where the list has no commands to offer.
        DocumentMenuHandler m_tileMenu{};
        // The file to stand on after the next rebuild, named rather than held as a tile: the tiles
        // the list holds now are the ones the rebuild takes down.
        std::wstring m_fileNameToSelect{};
    };


    //-----------------------------------------------------------------------------


    // A tile is as wide as its picture's slot and a name wraps into that width, so the slot is
    // wider than the mark it centres - room for a short name on one line.
    constexpr float k_documentTileIconHeight{ 64.0f };
    constexpr float k_documentTileIconWidth{ k_documentTileIconHeight * 1.6f };
    constexpr IconSize k_documentTileIconSize{ k_documentTileIconWidth, k_documentTileIconHeight };
    constexpr float k_documentTilePadding{ 4.0f };
    constexpr float k_tileSpacing{ 4.0f };

    // DocumentTile

    template<typename... Args>
    DocumentTile::DocumentTile(const CreateParams& params, Args&&... args)
        :
        Base{
            params,
            // Not a tool, so not a ToolButton - but it wears the same look, and the element and
            // the metrics are the whole of that look.
            params.themeMetrics().toolButton,
            UiElement::ToolButton,
            ShowSelectionOnSurface::Yes,
            IndicatorVisibility::Hover,
            IndicatorPlacement::TopLeftIn,
            ButtonViewMode::TopCenterIcon,
            k_documentTileIconSize,
            HorizontalTextAnchor::Center,
            // A tile takes the share of the lane it is handed, so every tile comes out the same
            // width whatever it is called, and a long name wraps under the mark rather than
            // widening the tile.
            HorizontalAlign::Fill,
            std::forward<Args>(args)...
        },
        m_folder{ *Props::find<DocumentTileData>(args...)->folder },
        m_path{ Props::find<DocumentTileData>(args...)->path }
    {
        setPadding(k_documentTilePadding);
    }

    // DocumentsList

    template<typename... Args>
    DocumentsList::DocumentsList(const CreateParams& params, Args&&... args)
        :
        StackView{
            params,
            Orientation::HorizontalWrap,
            ItemSizing::Equal,
            // Top, not Fill: a body that fills is handed the viewport, and a wrapping stack shares
            // that height out among its lanes. Aligned Top the list keeps the height it measured.
            VerticalAlign::Top,
            Padding{ 12.0f, 4.0f },
            Spacing{ k_tileSpacing },
            PlaceHolderText{
                InkGrade::Subtle,
                L"No ",
                Props::get<DocumentsFolder*>(nullptr, args...)->kind().nounPlural
            },
            // What a list of documents is for is picking one, so a stated selection mode is what
            // makes it more than that. Said ahead of the caller's own props, which are read after
            // these and stand in their place - see Props::get.
            SelectionMode::None,
            DragMode::EasySelect,
            std::forward<Args>(args)...
        },
        m_folder{ *Props::get<DocumentsFolder*>(nullptr, args...) },
        m_editDocument{ Props::get<DocumentEditHandler>({}, args...) },
        m_tileMenu{ Props::get<DocumentMenuHandler>({}, args...) }
    {
        m_folder.listeners().insert(this);
        rebuild();
    }
}
