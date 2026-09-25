export module ClaFi.Browser.Actions;

import ClaFi.Core.Foundation;

namespace ClaFi::Browser::Actions
{
    // The commands a browser answers about the page it is showing, and the objects both ends
    // compare BY ADDRESS - the shape ClaFi::StdActions has for the framework's own commands.
    //
    // WHAT IS OPENED IS NAMED BY THE PAGE. BrowserControl claims both against
    // BrowserPage::pathToOpen, so a menu item, a toolbar button and any other presenter run one
    // implementation, and a page that names nothing leaves both disabled.
    //
    // The namespace is spelt for what it holds, so inside ClaFi::Browser an unqualified
    // Actions names it rather than the ClaFi::Actions class. That class is a shortcut scope
    // and is reached through FormBase::actions() or AppActions::get(), both of which spell it
    // out.
    //
    // Neither carries an icon: each is a line in a menu and names itself in words.
    export extern Action open;
    // Opens the page beside what is on screen rather than over it, so the page the user is on
    // stays where it is.
    export extern Action openInNewTab;

    // NEITHER CARRIES A SHORTCUT, and Enter is the reason. Enter on a focused control is that
    // control's press, answered by FocusNavigator after the shortcut scopes have had their turn,
    // so a browser claiming the key would take it from every button standing inside the browser -
    // which is all of them. An application that wants one says so:
    //
    //     Browser::Actions::open.setShortcut({ vkReturn });
    //
    // TODO: should a browser take Enter only while the focus is inside the page, so a tile
    // answers it and a toolbar button does not? That is a question about where a shortcut is
    // looked up, rather than about these two commands.

    // Puts both in the application's scope, which is what a shortcut is looked up in. A
    // BrowserControl calls it as it is built, so a browser holds them without writing anything.
    export void registerAll();
}
