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

    // The documents in the folder as tiles, and the commands over them. See Documents
    export class DocumentsHomePage : public DocumentsBasePage
    {
    public:
        template<typename... Args>
        explicit DocumentsHomePage(const CreateParams&, Args&&...);
    public:
        void selectItemByPagePath(std::wstring_view);
    protected:
        std::wstring pathToOpen() override;
    private:
        // What the page owes a list that has just been built afresh: the tile a new document is
        // waiting to be named on.
        void documentsRebuilt();
        // Asks for the tile under `fileName` to be renamed once the pass that lays it out has run.
        void renameAfterAlign(std::wstring_view fileName);
        void renamePendingTile();
        // Puts the name an editor was left with on the document's file, and says where the list
        // is to come back to once the folder reports.
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
        [[nodiscard]] std::wstring fileNameAfterDeleting() const;
        // Answers whether the deletion may run. Asked once for the whole selection, and only
        // where something selected holds work - a file that holds none goes without a question.
        [[nodiscard]] bool confirmDeleting(Control& initiator);
    private:
        static constexpr IconSize k_buttonIconSize{ 18.0f };
        // The document a rename is waiting on, named by its file rather than held as a tile: the
        // list may be rebuilt between the request and the tick that answers it.
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

        // THE BOX IS THE PAGE'S, NOT THE LIST'S - see DocumentsList, which is the view alone. The
        // window gives the tiles the whole of its room, so what scrolls them stands here.
        //
        // Built last, and in the constructor rather than here: the handlers it is given name this
        // page, and a default member initializer may not capture one.
        ScrollBoxWith<DocumentsList>& m_tilesBox;
        DocumentsList& m_tiles;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    DocumentsHomePage::DocumentsHomePage(const CreateParams& params, Args&&... args)
        :
        DocumentsBasePage{ params, std::forward<Args>(args)... },
        m_tilesBox{ createBody<ScrollBoxWith<DocumentsList>>(
            HostProps{
                ScrollBars::Vertical,
                UiElement::Page
            },
            BodyProps{
                &folder(),
                SelectionMode::Multi,
                DragMode::EasySelect,
                DocumentEditHandler{ [this](DocumentTile& tile, AcceptEditEvent& event) {
                    acceptTileEdit(tile, event);
                } },
                DocumentMenuHandler{ [this](DocumentTile& tile, ContextPopupEvent& event) {
                    showTileMenu(tile, event);
                } }
            }
        ) },
        m_tiles{ m_tilesBox.body() }
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

        // Answered by the page, not by the button: the toolbar button, the Delete key and a menu
        // item carrying the same action all land here, and the selection they act on is this
        // page's.
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::del)
                event.claim({ .enabled = !m_tiles.selection().empty() });
        });
        onActionClick([this](ActionClickEvent& event) {
            // Under whatever presented the command, so a question about it stands where the user
            // is looking. The Delete key presents nothing, and the toolbar button is where the
            // command lives on this page.
            if (&event.action == &StdActions::del)
                deleteSelectedFiles(event.presenter ? *event.presenter : m_deleteButton);
        });

        m_exploreButton.onClick([this](ClickEvent&) {
            Platform::shellExecute(form(), folder().directory().wstring());
        });
        m_exploreButton.onGetState([this](GetStateEvent& event) {
            std::error_code errorCode;
            event.state.enabled = std::filesystem::exists(folder().directory(), errorCode);
            event.stopPropagation();
        });

        m_tiles.onSelectionChange([](SelectionChangeEvent&) {
            StdActions::del.invalidateState();
        });
        m_tiles.connectEvent([this](DocumentsRebuiltEvent&) {
            documentsRebuilt();
        });

        // A PRESS ON THE TILE OPENS THE DOCUMENT, AND THE KEYBOARD'S PRESS IS A PRESS. Return and
        // Space on the tile the keyboard stands on arrive as an ordinary click, so the mouse's
        // single click and the keyboard's press are one event, and the form is what tells them
        // apart. A modifier makes the press a selection command rather than an activation.
        m_tiles.onClick([this](ClickEvent& event) {
            DocumentTile* tile = m_tiles.currentTile();
            if (!tile || tile != event.control)
                return;
            if (!event.form.isKeyboardClick() || event.modifiers.ctrl || event.modifiers.shift)
                return;
            openDocument(*tile, event.stamp);
        });
        m_tiles.onDoubleClick([this](DoubleClickEvent& event) {
            DocumentTile* tile = m_tiles.currentTile();
            if (!tile || tile != event.control)
                return;
            // THE GESTURE IS SPENT HERE. A double click that nothing stopped is read by the form
            // as the press it also is, and that press would act on a tile the navigation is about
            // to take down.
            event.stopPropagation();
            openDocument(*tile, event.stamp);
        });
    }
}
