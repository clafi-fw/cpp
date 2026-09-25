export module ThisApp.ThemeButton;

import ThisApp.Consts;
import ThisApp.Utils;

import ClaFi.Diagnostic.Log;

import ClaFi.Application.ThemesManager;

import ClaFi.Browser.Actions;

import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Menu;


import ClaFi.StdActions;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;


    // The caption IS the theme file's name without its extension, so the in-place editor over it
    // renames the file. WithInPlaceEdit carries the whole of that - the gestures, the geometry -
    // and what is left here is what is about themes.
    export class ThemeButton : public WithInPlaceEdit<Button>
    {
    public:
        struct Initializer
        {
            const std::wstring_view pagePath;
            const AppTheme& theme;
        };
    public:
        template<typename... Args>
        ThemeButton(const CreateParams&, Args&&...);
    public:
        const AppTheme& theme() { return m_theme; }
        const std::wstring& pagePath() { return m_pagePath; }
    protected:
        void click(ClickEvent&) override;
        void doubleClick(DoubleClickEvent&) override;
        [[nodiscard]] EditorMode editorMode() const override;
        void contextPopup(ContextPopupEvent&) override;
        void paintIcon(PaintIconEvent&) override;
    private:
        using Base = WithInPlaceEdit<Button>;
    private:
        // Runs Open on this tile. See the definition for why the command answers rather than the
        // page navigating. The stamp is the press behind the gesture that asked.
        void openTheme(InputStamp);
    private:
        const AppTheme m_theme{ {}, ThemeColors{ ColorMode::Dark } };
        const std::wstring m_pagePath{};
    };


    //-----------------------------------------------------------------------------

    constexpr float k_themeIconSizeY = 64.0f;
    constexpr float k_themeIconSizeX = k_themeIconSizeY * 1.6f;
    constexpr IconSize k_themeIconSize{ k_themeIconSizeX, k_themeIconSizeY };


    //-----------------------------------------------------------------------------


    template<typename ...Args>
    ThemeButton::ThemeButton(const CreateParams& params, Args&&... args)
        :
        Base{
            params,
            // Not a tool, so not a ToolButton - but it wears the same look, and the property is
            // the whole of that look.
            ShowSurfaceAtRest::No,
            ShowSelectionOnSurface::Yes,
            IndicatorVisibility::Hover,
            IndicatorStyle::Check,
            IndicatorPlacement::TopLeftIn,
            ButtonViewMode::TopCenterIcon,
            k_themeIconSize,
            HorizontalTextAnchor::Center,
            // The tag carries the UserTheme, and Control's constructor has already read it out of
            // the pack, so the path is reachable from here on. Naming this tile inside the handler
            // is safe - the editor stops offering the text once its target is gone.
            OnEvent{ [this](AcceptEditEvent& event) {
                if (renameThemeFile(tag<UserTheme*>()->path(), event).empty())
                    return;
                // THE FILE IS RENAMED AND THIS TILE STILL SAYS OTHERWISE. The list is rebuilt from
                // the directory, and the watch behind that waits out a quiet period first, so for a
                // moment after the editor closes this caption is the only thing on screen naming the
                // theme - and it would be naming it by the name it no longer has. The rebuild
                // replaces this tile with one that reads the same.
                text().clear();
                text() << event.text.plainText();
                // A name is as many lines as it needs inside a fixed width, so a new one is a new
                // height. See the deferred-request rule: ask, do not lay out from here.
                invalidateFormAlign();
            } },
            std::forward<Args>(args)...
        },
        m_pagePath{ Props::find<ThemeButton::Initializer>(std::forward<Args>(args)...)->pagePath},
        m_theme{ Props::find<ThemeButton::Initializer>(std::forward<Args>(args)...)->theme }
    {
        float newPadding = 4.0f;
        setFixedWidth(theme().metrics.checkMark.minSize.x + newPadding + k_themeIconSizeX + newPadding * 2);
        setPadding({ newPadding });
    }

    // A PRESS ON THE TILE OPENS THE THEME, AND THE KEYBOARD'S PRESS IS A PRESS. Return and Space
    // on the tile the keyboard stands on arrive here as an ordinary click - FocusNavigator answers
    // both by clicking the item the focus delegates to - so the mouse's single click and the
    // keyboard's press are one event, and the form is what tells them apart.
    //
    // A MODIFIER MAKES THE PRESS A SELECTION COMMAND rather than an activation: Ctrl+Space takes
    // the tile in or out of the selection and Shift+Space ends a range, both answered by the
    // container above, and opening a theme on top of either is a second thing nobody asked for.
    //
    // The modifiers are read before the base runs, because ButtonBase reads a press over the check
    // mark as a press with Ctrl held, and it reads it off the pointer rather than off the event -
    // a hand left resting on the mark would otherwise be enough to swallow Return.
    //
    // The event travels on: the container above reads it for the selection, and what it settles
    // there is the same whether or not a theme is opening.
    void ThemeButton::click(ClickEvent& event)
    {
        const bool opensTheme = event.form.isKeyboardClick()
            && !event.modifiers.ctrl
            && !event.modifiers.shift;
        Base::click(event);
        if (opensTheme)
            openTheme(event.stamp);
    }

    // A double click on the check mark never arrives here: the indicator answers that one itself
    // and stops it, which is what keeps taking a tile into the selection from opening it.
    void ThemeButton::doubleClick(DoubleClickEvent& event)
    {
        Base::doubleClick(event);
        // THE GESTURE IS SPENT HERE. A double click that nothing stopped is read by the form as
        // the press it also is, and that press would act on a tile the navigation is about to
        // take down. Said before the command runs, because the command owns what happens next and
        // the event outlives this tile either way.
        event.stopPropagation();
        openTheme(event.stamp);
    }

    EditorMode ThemeButton::editorMode() const
    {
        // Only a user theme is a file, and only a file has a name to change. The built-in pair
        // carry no UserTheme in their tag, so there is nothing on disk behind their captions.
        if (!tag<UserTheme*>())
            return EditorMode::None;
        return Base::editorMode();
    }

    void ThemeButton::contextPopup(ContextPopupEvent& event)
    {
        // The application gets first refusal, and a handler that stops the event has replaced
        // this menu outright.
        Base::contextPopup(event);
        if (event.propagationStopped())
            return;

        // THE MENU NAMES FOUR COMMANDS AND SETTLES NONE OF THEM. Rename is answered by the tile,
        // against the caption that is the theme file's name; Delete by the page, against the
        // selection; Open and Open in new tab by the browser, against the path this page names
        // for whichever tile is current. So a menu item, a toolbar button and the Delete key all
        // run one implementation, and a built-in theme arrives with the first two greyed rather
        // than missing.
        //
        // The selection Delete acts on is already the one the user is looking at: the press that
        // raised this menu moved the focus, which selects a tile standing outside the selection
        // and leaves a selection the tile is part of as it stands.
        Menu menu{ *this };
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

    void ThemeButton::paintIcon(PaintIconEvent& event)
    {
        const ThemeColors& colors{ m_theme.colors };
        paintThemeIcon(event, colors);
    }

    // OPEN IS THE COMMAND, AND THE BROWSER ANSWERS IT. What is opened is named by the page,
    // against the tile the user is on - which is this one, because a press reaches a tile through
    // the focus and the focus is what moves the current item. So the line in this tile's menu, a
    // double click and a keyboard press run one implementation, and it is the one that navigates
    // on a wait rather than from inside the press that asked.
    //
    // The tile is the presenter: it is where the command was given, and it is the nearest control
    // the walk that finds the browser can start from.
    void ThemeButton::openTheme(const InputStamp stamp)
    {
        Browser::Actions::open.invoke(form(), this, stamp);
    }
}
