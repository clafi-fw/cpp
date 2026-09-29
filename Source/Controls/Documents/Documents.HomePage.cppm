export module ClaFi.Documents.HomePage;

import ClaFi.Documents.BasePage;
import ClaFi.Documents.Folder;
import ClaFi.Documents.List;

import ClaFi.Browser.Control;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

import ClaFi.Icons.OpenInExplorerIcon;
import ClaFi.Icons.PlusMark;

import ClaFi.StdActions;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    export template<typename T>
        concept IsDocumentTiles = std::derived_from<T, IDocumentTiles>;

    // The commands over the folder's documents, worked over the tiles connected. See Documents
    export class DocumentsHomePageBase : public DocumentsBasePage
    {
    public:
        template<typename... Args>
        explicit DocumentsHomePageBase(const CreateParams&, Args&&...);
    public:
        void selectItemByPagePath(std::wstring_view);
    protected:
        std::wstring pathToOpen() override;
        // Works the page over these tiles from here on. Called once, by the page that built them.
        void connectTiles(IDocumentTiles&);
        // What the tiles are built with: a name typed over a tile is put on its file, and a
        // tile's menu carries Rename, Delete, Open and Open in new tab.
        [[nodiscard]] DocumentEditHandler editHandler();
        [[nodiscard]] DocumentMenuHandler menuHandler();
        // What the page owes tiles that have just been built afresh: the tile a new document is
        // waiting to be named on. A derived page adds what it owes, after this.
        virtual void documentsRebuilt();
    private:
        // Asks for the tile under `fileName` to be renamed once the pass that lays it out has run.
        void renameAfterAlign(std::wstring_view fileName);
        void renamePendingTile();
        // Puts the name an editor was left with on the document's file, and says where the tiles
        // are to come back to once the folder reports.
        void acceptTileEdit(DocumentTile&, AcceptEditEvent&);
        // The commands a tile offers, raised on the tile they are about.
        void showTileMenu(DocumentTile&, ContextPopupEvent&);
        // Runs Open on the tile the list stands on. The stamp is the press behind the gesture.
        void openDocument(Control& presenter, InputStamp);
        void createNewFile();
        // Under `initiator`, which is what asked for the deletion - the question it may raise
        // stands there.
        void deleteSelectedFiles(Control& initiator);
        // The file to stand on once the selection has gone: the first one left standing after it,
        // and the last one before it where the selection runs to the end of the list.
        [[nodiscard]] std::wstring fileNameAfterDeleting();
        // Answers whether the deletion may run. Asked once for the whole selection, and only
        // where something selected holds work - a file that holds none goes without a question.
        [[nodiscard]] bool confirmDeleting(Control& initiator);
    private:
        static constexpr IconSize k_buttonIconSize{ 18.0f };
        // The tiles the page works over, and nullptr until the page that built them connects
        // them - which every command here runs after.
        IDocumentTiles* m_tiles{ nullptr };
        // The document a rename is waiting on, named by its file rather than held as a tile: the
        // tiles may be rebuilt between the request and the tick that answers it.
        std::wstring m_fileNameToRename{};
        UiTimer m_renameTimer{};

        Button& m_newButton{ toolBar().add<ToolButton>(
            k_buttonIconSize,
            ButtonViewMode::LeftIcon,
            Button::OnPaintIcon{ Icons::PlusMark::paint },
            Text{ L"New ", folder().kind().noun }
        ) };
        Divider& m_dividerAfterNew{ toolBar().add<Divider>(Padding{ 4.0f }) };
        // The words and the icon come from the action. Mouse only leaves the focus, and with it
        // what the action acts on, on the tiles.
        Button& m_deleteButton{ toolBar().add<ToolButton>(
            k_buttonIconSize,
            ButtonViewMode::IconOnly,
            StdActions::del,
            Interactivity::MouseOnly
        ) };
        Divider& m_dividerAfterDelete{ toolBar().add<Divider>(Padding{ 4.0f }) };
        Button& m_exploreButton{ toolBar().add<ToolButton>(
            k_buttonIconSize,
            ButtonViewMode::IconOnly,
            Button::OnPaintIcon{ Icons::OpenInExplorerIcon::paint },
            L"Open folder"
        ) };
    };

    export template<IsDocumentTiles ListType = DocumentsList>
        // The home page over a list of the named type, built as its scrolling body. See Documents
        class DocumentsHomePage : public DocumentsHomePageBase
    {
    public:
        template<typename... Args>
        explicit DocumentsHomePage(const CreateParams&, Args&&...);
    public:
        // The list the page was built over, as its own type.
        [[nodiscard]] ListType& list() { return m_list; }
    private:
        // THE BOX IS THE PAGE'S, NOT THE LIST'S - the list is the view alone. The window gives
        // the tiles the whole of its room, so what scrolls them stands here.
        ScrollBoxWith<ListType>& m_listBox;
        ListType& m_list;
    };


    //-------------------------------------------------------------------------


    // DocumentsHomePageBase

    template<typename... Args>
    DocumentsHomePageBase::DocumentsHomePageBase(const CreateParams& params, Args&&... args)
        :
        DocumentsBasePage{ params, std::forward<Args>(args)... }
    {
        m_newButton.onClick([this](ClickEvent&) {
            createNewFile();
        });
        // A folder the platform could not name is nowhere to make a file.
        m_newButton.onGetState([this](GetStateEvent& event) {
            event.state.enabled = !folder().directory().empty();
            event.stopPropagation();
        });
        // Connected here rather than given to the timer as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_renameTimer.onTick([this](TimerEvent&) {
            renamePendingTile();
        });

        m_exploreButton.onClick([this](ClickEvent&) {
            Platform::shellExecute(form(), folder().directory().wstring());
        });
        m_exploreButton.onGetState([this](GetStateEvent& event) {
            std::error_code errorCode;
            event.state.enabled = std::filesystem::exists(folder().directory(), errorCode);
            event.stopPropagation();
        });
    }

    // DocumentsHomePage

    template<IsDocumentTiles ListType>
    template<typename... Args>
    DocumentsHomePage<ListType>::DocumentsHomePage(const CreateParams& params, Args&&... args)
        :
        DocumentsHomePageBase{ params, std::forward<Args>(args)... },
        m_listBox{ createBody<ScrollBoxWith<ListType>>(
            HostProps{
                ScrollBars::Vertical,
                UiElement::Page
            },
            BodyProps{
                &folder(),
                SelectionMode::Multi,
                DragMode::EasySelect,
                editHandler(),
                menuHandler()
            }
        ) },
        m_list{ m_listBox.body() }
    {
        connectTiles(m_list);
    }
}
