export module ClaFi.StdActions;

import ClaFi.Core.Foundation;

namespace ClaFi::StdActions
{
    // The commands the framework's own controls answer, and the objects they answer BY ADDRESS: a
    // text box claims Copy without ever having seen the button that presents it, so the object
    // both ends compare has to be one the framework owns.
    //
    // They live above Core rather than on AppActions, which is where the scope they are looked up
    // in lives: an action carries an icon, and Icons sits above Core. AppActions::get() holds them
    // once registerAll() has run.
    //
    // Ordinary objects. An application translates one, gives it an icon or clears its shortcut by
    // writing to it:
    //
    //     StdActions::copy.text().clear();
    //     StdActions::copy.text() << tr(L"Copy");
    //     StdActions::copy.invalidateText();   // the size every presenter asked for is stale
    //     StdActions::copy.setShortcut({});
    //
    export extern Action cut;
    export extern Action copy;
    export extern Action paste;
    // Spelt short because `delete` is a keyword.
    export extern Action del;
    export extern Action selectAll;
    // Puts a value back to having none - a rule that names no colour, a field with nothing typed
    // in it. Distinct from del, which removes what is selected: there is nothing selected here,
    // and what is cleared is the whole of what the subject holds.
    export extern Action clear;
    export extern Action undo;
    export extern Action redo;
    // Puts what the subject holds where it belongs, over whatever was there. What a subject is
    // and where its work goes are its own - the action says only that it is time.
    export extern Action save;
    // The same work, somewhere else, and the subject goes on standing over the new place: what a
    // subject is called is what changes. It carries no icon - it is the second half of a split
    // button and a line in a menu, and both name it in words.
    export extern Action saveAs;
    // Gives what the subject is called a new name, typed where the old name is written rather
    // than in a window of its own. Every control built on WithInPlaceEdit answers it against its
    // own caption, so a menu item or a toolbar button renames a tile or a breadcrumb with nothing
    // written by the application.
    export extern Action rename;

    // Puts all eleven in the application's scope, which is what a shortcut is looked up in.
    // ApplicationBase::initialize calls it, so an application has the standard commands without
    // writing anything. An application that wants a key back clears that action's shortcut.
    export void registerAll();
}
