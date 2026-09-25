export module ThisApp.HomePage;

import ThisApp.Consts;
import ThisApp.BasePage;
import ThisApp.Utils;

import ClaFi.App.ThemesList;
import ClaFi.Application.ThemesManager;

import ClaFi.Browser;
import ClaFi.Browser.Actions;

import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Menu;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.StackView;

import ClaFi.Icons.PlusMark;
import ClaFi.Icons.OpenInExplorerIcon;

import ClaFi.StdActions;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.DomEngine;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
// HostProps and BodyProps are declared here - see WithBody, which the themes box is one of.
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    export class HomePage : public BasePage
    {
    public:
        template<typename... Args>
        explicit HomePage(const CreateParams&, Args&&...);
    public:
        void selectItemByPagePath(std::wstring_view);
    protected:
        const AppTheme* selectedTheme() override;
        std::wstring pathToOpen() override;
    private:
        // What the page owes a list that has just been built afresh: the tile a new theme is
        // waiting to be named on, and a preview of whatever the list now stands on.
        void themesRebuilt();
        // Asks for the tile under `themePath` to be renamed once the pass that lays it out has
        // run. See the body.
        void renameAfterAlign(std::wstring_view themePath);
        void renamePendingTile();
        // Puts the name an editor was left with on the theme's file, and says where the list is
        // to come back to once the directory reports.
        void acceptTileEdit(ThemeTile&, AcceptEditEvent&);
        // The commands a tile offers, raised on the tile they are about.
        void showTileMenu(ThemeTile&, ContextPopupEvent&);
        // Runs Open on the tile the list stands on. See the definition for why the command
        // answers rather than the page navigating. The stamp is the press behind the gesture.
        void openTheme(Control& presenter, InputStamp);
        [[nodiscard]] std::filesystem::path buildNewThemeFileName(int counter) const;
        void createNewThemeFile();
        // Under `initiator`, which is what asked for the deletion - the question it may raise
        // stands there.
        void deleteSelectedThemes(Control& initiator);
        // The theme to stand on once the selection has gone: the first one left standing after
        // it, and the last one before it where the selection runs to the end of the list.
        [[nodiscard]] std::wstring pathAfterDeleting() const;
        // Whether the theme states anything of its own. A theme file states only what it differs
        // from the defaults in, so one that states nothing is a theme nobody has touched - what
        // New theme leaves behind, and what may go without being asked about.
        [[nodiscard]] static bool isThemeEdited(const UserTheme&);
        // Answers whether the deletion may run. Asked only where something selected has been
        // edited, and asked once for the whole selection.
        [[nodiscard]] bool confirmDeletingEdited(Control& initiator);
    private:
        static constexpr IconSize k_btnIconSize{ 18.0f };
        // The theme a rename is waiting on, named by its theme path rather than held as a
        // pointer: the list may be rebuilt between the request and the tick that answers it.
        std::wstring m_pathToRename{};
        UiTimer m_renameTimer{};

        Button& m_newThemeButton{ toolBar().add<ToolButton>(
            k_btnIconSize,
            ButtonViewMode::LeftIcon,
            Button::OnPaintIcon{ Icons::PlusMark::paint },
            L"New theme"
        ) };
        Controls::Divider& m_sep1{ toolBar().add<Controls::Divider>(Padding{ 4.0f }) };
        // The words and the icon come from the action. This button is icon-only, so the text is
        // what its tooltip shows, and translating the action changes that and the menu item
        // together.
        Button& m_deleteButton{ toolBar().add<ToolButton>(
            k_btnIconSize,
            ButtonViewMode::IconOnly,
            StdActions::del,
            // The subject is found by walking up from the focused control, so a presenter that
            // takes the focus moves what the action acts on. MouseOnly leaves it on the tiles.
            Interactivity::MouseOnly
        ) };
        Controls::Divider& m_sep2{ toolBar().add<Controls::Divider>(Padding{ 4.0f }) };
        Button& m_exploreButton{ toolBar().add<ToolButton>(
            k_btnIconSize,
            ButtonViewMode::IconOnly,
            Button::OnPaintIcon{ Icons::OpenInExplorerIcon::paint },
            L"Open folder"
        ) };

        // THE BOX IS THE PAGE'S, NOT THE LIST'S - see ThemesList, which is the view alone. A
        // window gives the themes the whole of its room, so what scrolls them stands here.
        //
        // Built last, and in the constructor rather than here: the handlers it is given name this
        // page, and a default member initializer may not capture one.
        ScrollBoxWith<ThemesList>& m_themesBox;
        ThemesList& m_themesList;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    HomePage::HomePage(const CreateParams& params, Args&&... args)
        :
        BasePage{ params, std::forward<Args>(args)... },
        m_themesBox{ createBody<ScrollBoxWith<ThemesList>>(
            HostProps{
                ScrollBars::Vertical,
                UiElement::Page
            },
            BodyProps{
                SelectionMode::Multi,
                DragMode::EasySelect,
                ThemeEditHandler{ [this](ThemeTile& tile, AcceptEditEvent& event) {
                    acceptTileEdit(tile, event);
                } },
                ThemeMenuHandler{ [this](ThemeTile& tile, ContextPopupEvent& event) {
                    showTileMenu(tile, event);
                } }
            }
        ) },
        m_themesList{ m_themesBox.body() }
    {
        m_newThemeButton.onClick([this](ClickEvent&) { createNewThemeFile(); });
        // Connected here rather than given to the timer as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_renameTimer.onTick([this](TimerEvent&) { renamePendingTile(); });

        // Answered by the page, not by the button: the toolbar button, the Delete key and a menu
        // item carrying the same action all land here, and the selection they act on is this
        // page's. Claiming states that this page is what Delete is about; what it claims states
        // whether it can run now.
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::del)
                event.claim({ .enabled = !m_themesList.view().selection().empty() });
        });
        onActionClick([this](ActionClickEvent& event) {
            // Under whatever presented the command, so a question about it stands where the user
            // is looking. The Delete key presents nothing, and the toolbar button is where the
            // command lives on this page.
            if (&event.action == &StdActions::del)
                deleteSelectedThemes(event.presenter ? *event.presenter : m_deleteButton);
        });

        m_exploreButton.onClick([this](ClickEvent&) {
            Platform::shellExecute(form(), themesManager().directory().wstring());
            });
        m_exploreButton.onGetState([this](GetStateEvent& event) {
            event.state.enabled = std::filesystem::exists(themesManager().directory());
            event.stopPropagation();
            });

        // THE TILE BEING LOOKED AT, not the one that has been picked. Moving the current item is
        // immediate and a theme change is a movement of its own, so a current tile travelling
        // down the list on a held key would ask for one per step and the application would never
        // arrive anywhere. A preview is raised once the tile settles, whether the pointer or the
        // keyboard brought it there, so the application crosses to the theme that was stopped on.
        m_themesList.view().onPreview([this](PreviewEvent&) {
            invalidatePreview();
            });
        m_themesList.view().onSelectionChange([](SelectionChangeEvent&) {
            StdActions::del.invalidateState();
            });
        m_themesList.connectEvent([this](ThemesRebuiltEvent&) {
            themesRebuilt();
            });

        // A PRESS ON THE TILE OPENS THE THEME, AND THE KEYBOARD'S PRESS IS A PRESS. Return and
        // Space on the tile the keyboard stands on arrive as an ordinary click - FocusNavigator
        // answers both by clicking the item the focus delegates to - so the mouse's single click
        // and the keyboard's press are one event, and the form is what tells them apart.
        //
        // A MODIFIER MAKES THE PRESS A SELECTION COMMAND rather than an activation: Ctrl+Space
        // takes the tile in or out of the selection and Shift+Space ends a range, both answered
        // by the view, and opening a theme on top of either is a second thing nobody asked for.
        //
        // The tile is the current item by the time the press is read - that is what a press moves
        // - so a click on anything else in the list is a click on something that is not a theme.
        m_themesList.onClick([this](ClickEvent& event) {
            ThemeTile* tile = m_themesList.currentTile();
            if (!tile || tile != event.control)
                return;
            if (!event.form.isKeyboardClick() || event.modifiers.ctrl || event.modifiers.shift)
                return;
            openTheme(*tile, event.stamp);
            });
        m_themesList.onDoubleClick([this](DoubleClickEvent& event) {
            ThemeTile* tile = m_themesList.currentTile();
            if (!tile || tile != event.control)
                return;
            // THE GESTURE IS SPENT HERE. A double click that nothing stopped is read by the form
            // as the press it also is, and that press would act on a tile the navigation is about
            // to take down. Said before the command runs, because the command owns what happens
            // next and the event outlives this tile either way.
            event.stopPropagation();
            openTheme(*tile, event.stamp);
            });
    }

    void HomePage::selectItemByPagePath(const std::wstring_view value)
    {
        m_themesList.setCurrentPath(themePathOfPage(value));
    }

    const AppTheme* HomePage::selectedTheme()
    {
        return m_themesList.previewedTheme();
    }

    // THE TILE THE USER IS ON, which a right click has just moved the current item to, so the
    // browser's Open and Open in new tab are about the tile the menu was raised from as well as
    // about the one the keyboard stands on.
    std::wstring HomePage::pathToOpen()
    {
        return pagePathOfTheme(m_themesList.currentPath());
    }

    void HomePage::themesRebuilt()
    {
        if (!m_pathToRename.empty())
            renameAfterAlign(m_pathToRename);
        // The rebuild took the previewed tile with the rest, so what the application is wearing
        // is a theme nothing on screen stands for any more. Asked for outright because no gesture
        // is going to ask: the pointer has not moved and the current item was set from the list.
        invalidatePreview();
    }

    // ASKED FOR ON A TICK, because the rebuild that created the tile is still on the stack: it
    // holds the list this tile is in, and clears it, and the editor runs a message loop of its
    // own. The tile has no layout yet either, and the tick does not promise one - on Wayland it
    // can arrive ahead of the pass - so the editor waits for the form to settle by itself; see
    // WithInPlaceEdit::openEditor.
    void HomePage::renameAfterAlign(const std::wstring_view themePath)
    {
        m_pathToRename = themePath;
        m_renameTimer.start(MilliSeconds{ 0u });
    }

    void HomePage::renamePendingTile()
    {
        // Found again rather than kept as a pointer: another rebuild may have run in between and
        // taken the tile with it. A tile under that path is the tile the user was promised.
        ThemeTile* tile = m_themesList.tileByPath(m_pathToRename);
        m_pathToRename.clear();
        if (!tile)
            return;
        // THE FOCUS, not just the current item. The two are separate - see StackPanelBase - and
        // the focus is still on the button that made the theme, which is where it would go back
        // to when the editor closes. This is also the whole of "the new theme is focused": the
        // container answers for the item, and setFocus on the item is how it is told which.
        tile->setFocus();
        tile->openEditor();
    }

    void HomePage::acceptTileEdit(ThemeTile& tile, AcceptEditEvent& event)
    {
        if (renameThemeFile(tile.userTheme()->path(), event).empty())
            return;
        // THE FILE IS RENAMED AND THIS TILE STILL SAYS OTHERWISE. The list is rebuilt from the
        // directory, and the watch behind that waits out a quiet period first, so for a moment
        // after the editor closes this caption is the only thing on screen naming the theme - and
        // it would be naming it by the name it no longer has. The rebuild replaces this tile with
        // one that reads the same.
        tile.text().clear();
        tile.text() << event.text.plainText();
        // A name is as many lines as it needs inside a fixed width, so a new one is a new height.
        // See the deferred-request rule: ask, do not lay out from here.
        tile.invalidateFormAlign();
        // A RENAME MOVES THE TILE THE LIST REMEMBERS. The path is the whole of a tile's identity
        // here, so the one to come back to after the rebuild is under the NEW name. It is
        // self-correcting: a rename that was refused never reaches this line, and the list comes
        // back to the path it was already on.
        m_themesList.selectPathAfterRebuild(
            themePathOf(ThemeRoots::user, event.text.plainText()));
    }

    // THE MENU NAMES FOUR COMMANDS AND SETTLES NONE OF THEM. Rename is answered by the tile,
    // against the caption that is the theme file's name; Delete by the page, against the
    // selection; Open and Open in new tab by the browser, against the path this page names for
    // whichever tile is current. So a menu item, a toolbar button and the Delete key all run one
    // implementation, and a built-in theme arrives with the first two greyed rather than missing.
    //
    // The selection Delete acts on is already the one the user is looking at: the press that
    // raised this menu moved the focus, which selects a tile standing outside the selection and
    // leaves a selection the tile is part of as it stands.
    void HomePage::showTileMenu(ThemeTile& tile, ContextPopupEvent& event)
    {
        Menu menu{ tile };
        // WHAT THE TILE IS, in the strip: a theme has a name and a file, and a picture says
        // either at a glance, so the two of them stand side by side in what the list would spend
        // on one line. WHERE IT GOES, in the list: opening is a sentence rather than a picture,
        // and the two lines differ by their words alone.
        menu.addToCommandBar(StdActions::rename);
        menu.addToCommandBar(StdActions::del);
        menu.add(Browser::Actions::open);
        menu.add(Browser::Actions::openInNewTab);
        // The tile has answered, so nothing above it raises a second menu behind this one. Said
        // before the menu runs, because a command is free to take this tile down with it and the
        // event outlives it either way.
        event.stopPropagation();
        menu.execute();
    }

    // OPEN IS THE COMMAND, AND THE BROWSER ANSWERS IT. What is opened is named by the page,
    // against the tile the user is on - which is the current one, because a press reaches a tile
    // through the focus and the focus is what moves the current item. So the line in a tile's
    // menu, a double click and a keyboard press run one implementation, and it is the one that
    // navigates on a wait rather than from inside the press that asked.
    //
    // The tile is the presenter: it is where the command was given, and it is the nearest control
    // the walk that finds the browser can start from.
    void HomePage::openTheme(Control& presenter, const InputStamp stamp)
    {
        Browser::Actions::open.invoke(form(), &presenter, stamp);
    }

    std::filesystem::path HomePage::buildNewThemeFileName(int counter) const
    {
        std::wstring name = L"New Theme";
        if (counter > 1)
            name.append(L" (").append(std::to_wstring(counter)).append(L")");
        name.append(k_themeFileExtension);
        return themesManager().directory() / name;
    }

    void HomePage::createNewThemeFile()
    {
        constexpr std::wstring_view contentStub = L"";

        // A NEW THEME IS SHOWN, so the group it lands in is open. Said here rather than left to
        // the list: the rebuild that creates the tile runs from the manager, and a tile made into
        // a closed group is never laid out - it would have no rect to be selected on, scrolled to
        // or renamed over. Asked before the file is written, so the group is open by the time the
        // rebuild arrives.
        m_themesList.expandUserGroup();

        if (themesManager().needDirectory())
            m_exploreButton.invalidateState();
        int counter = 0;
        std::filesystem::path fileName{};
        do ++counter;
        while (std::filesystem::exists(fileName = buildNewThemeFileName(counter)));
        std::wofstream myFile(fileName, std::ios::out);
        if (contentStub.size())
            myFile.write(contentStub.data(), contentStub.size());

        // A theme that has just been made is unnamed in every sense but the file system's, so
        // standing on it and opening an editor over it is the rest of creating it.
        const std::wstring themePath = themePathOf(ThemeRoots::user, fileName.stem().wstring());
        m_themesList.selectPathAfterRebuild(themePath);
        m_pathToRename = themePath;
    }

    void HomePage::deleteSelectedThemes(Control& initiator)
    {
        if (!confirmDeletingEdited(initiator))
            return;

        // Taken after the question, so a selection the user kept is left standing as it was.
        m_themesList.selectPathAfterRebuild(pathAfterDeleting());
        for (Control* item : m_themesList.view().selection())
        {
            const UserTheme* theme = static_cast<ThemeTile*>(item)->userTheme();
            std::filesystem::remove(theme->path());
        }
    }

    std::wstring HomePage::pathAfterDeleting() const
    {
        std::wstring previous{};
        bool seenSelected = false;
        for (ThemeTile& tile : m_themesList.userTiles().controlsAs<ThemeTile>())
        {
            if (m_themesList.view().selection().contains(&tile))
            {
                seenSelected = true;
                continue;
            }
            if (seenSelected)
                return tile.themePath();
            previous = tile.themePath();
        }
        return previous;
    }

    bool HomePage::isThemeEdited(const UserTheme& theme)
    {
        // Written out the way saving writes it and compared against the seed reading starts from,
        // so the two sides are whole themes. A theme that differs from the defaults in nothing
        // states nothing, and its file holds no more than its header. ThemePage's dirty test asks
        // the same question of a whole theme.
        const Dom::Value<AppTheme> defaults{ nullptr, AppTheme{} };
        Dom::Value<AppTheme> themeNode{ nullptr, AppTheme{} };
        ThemesManager::saveTheme(theme.theme(), themeNode);
        return !Dom::sameValue(themeNode, defaults);
    }

    bool HomePage::confirmDeletingEdited(Control& initiator)
    {
        // The one edited theme where there is exactly one, so the question can name it. Cleared
        // by the second, which is where a count is all that can be said.
        const UserTheme* soleEdited{};
        int selectedCount = 0;
        int editedCount = 0;
        for (Control* item : m_themesList.view().selection())
        {
            // Only a user tile can be held selected - see ThemesList - so every item here has a
            // file behind it.
            const UserTheme* theme = static_cast<ThemeTile*>(item)->userTheme();
            if (!theme)
                continue;

            ++selectedCount;
            if (!isThemeEdited(*theme))
                continue;

            soleEdited = editedCount == 0 ? theme : nullptr;
            ++editedCount;
        }

        // Nothing selected carries any work, so there is nothing to lose and nothing to ask.
        if (editedCount == 0)
            return true;

        // ONE QUESTION FOR THE WHOLE SELECTION, AND IT SETTLES ALL OF IT. Deleting the untouched
        // ones first and then asking about the rest would leave the user answering about a
        // selection that is no longer the one they made.
        //
        // TWO LINES, AND EACH SAYS ONE THING. The first is what stands to be lost, marked either
        // way because either way the sentence opens on what the question is about - the one theme
        // by its name, several of them by how many. The second is the question itself.
        //
        // A count is given against the whole selection, so a user about to delete thirteen themes
        // reads how many of the thirteen carry work rather than a number standing on its own. One
        // theme is named instead: a name against a total reads as neither.
        Text message{};
        if (soleEdited)
        {
            message << themeInQuestionText(soleEdited->path().stem().wstring())
                << L" has been edited.";
        }
        else
        {
            message << themeInQuestionText(std::to_wstring(editedCount).append(L" themes"))
                << L" of " << selectedCount << L" have been edited.";
        }

        // Permanently, said in the question rather than left to the icon: what goes from here goes
        // from the disk, and there is nothing in this application that brings it back.
        message << TextOp::EndLine
            << L"Are you sure you want to permanently delete "
            << (soleEdited ? L"it?" : L"them?");

        // A warning, not a question: the triangle is what says the answer cannot be taken back,
        // and it says it where a theme's own hues are not the ones the reader expects a colour to
        // mean.
        MessageDialog dialog{
            initiator,
            soleEdited ? L"Delete theme" : L"Delete themes",
            message,
            MessageIcon::Warning
        };
        dialog.add(DialogButton::Yes);
        dialog.add(DialogButton::No);
        // Dismissed without an answer is no, which is what makes Escape the safe way out of a
        // question about something that cannot be brought back.
        return dialog.execute() == DialogButton::Yes;
    }
}
