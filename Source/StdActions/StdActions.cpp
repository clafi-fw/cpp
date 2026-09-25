module ClaFi.StdActions;

import ClaFi.Icons.CopyIcon;
import ClaFi.Icons.CutIcon;
import ClaFi.Icons.DeleteIcon;
import ClaFi.Icons.PasteIcon;
import ClaFi.Icons.RedoIcon;
import ClaFi.Icons.RenameIcon;
import ClaFi.Icons.SaveIcon;
import ClaFi.Icons.SelectAllIcon;
import ClaFi.Icons.UndoIcon;
import ClaFi.Icons.WipeIcon;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::StdActions
{
    // THESE ARE CONSTRUCTED BEFORE MAIN, so nothing built here may touch the platform, the
    // theme or the text engine - none of them exists yet. Text is a string and its markers,
    // and EventComponent connects handlers into a map of its own, so both are safe; anything
    // added to an action below has to answer the same question. An icon is a function pointer
    // until the first paint asks it for a shape, so it answers it too.
    //
    // The virtual key code of a letter is its own uppercase character.
    Action cut{
        Text{ L"Cut" },
        Shortcut{ L'X', { .ctrl = true } },
        Action::OnPaintIcon{ Icons::CutIcon::paint }
    };
    Action copy{
        Text{ L"Copy" },
        Shortcut{ L'C', { .ctrl = true } },
        Action::OnPaintIcon{ Icons::CopyIcon::paint }
    };
    Action paste{
        Text{ L"Paste" },
        Shortcut{ L'V', { .ctrl = true } },
        Action::OnPaintIcon{ Icons::PasteIcon::paint }
    };
    // Delete removes what is selected. A control whose Delete key means more than that keeps the
    // key: a text box with nothing selected deletes forward, by a character or by a word, and
    // answers this action for the selection alone.
    Action del{
        Text{ L"Delete" },
        Shortcut{ Keys::Delete },
        Action::OnPaintIcon{ Icons::DeleteIcon::paint }
    };
    Action selectAll{
        Text{ L"Select all" },
        Shortcut{ L'A', { .ctrl = true } },
        Action::OnPaintIcon{ Icons::SelectAllIcon::paint }
    };
    // No shortcut. Every key that would read as Clear is one an application may want for itself,
    // and a key given here is given to every application at once. One that wants it says so:
    //
    //     StdActions::clear.setShortcut({ vkDelete, { .shift = true } });
    Action clear{
        Text{ L"Clear" },
        Action::OnPaintIcon{ Icons::WipeIcon::paint }
    };
    // Which direction to travel along the subject's own history, and no more than that: what a
    // step is, and how many of them there are, belongs to whatever kept it.
    Action undo{
        Text{ L"Undo" },
        Shortcut{ L'Z', { .ctrl = true } },
        Action::OnPaintIcon{ Icons::UndoIcon::paint }
    };
    Action redo{
        Text{ L"Redo" },
        Shortcut{ L'Y', { .ctrl = true } },
        Action::OnPaintIcon{ Icons::RedoIcon::paint }
    };

    Action save{
        Text{ L"Save" },
        Shortcut{ L'S', { .ctrl = true } },
        Action::OnPaintIcon{ Icons::SaveIcon::paintSaveIcon }
    };
    // No icon. It is presented beside Save rather than instead of it - the second half of a split
    // button, a line in a menu - and in both places the word is what tells the two apart. A
    // picture differing from Save's only in some corner would be read as the same command.
    Action saveAs{
        Text{ L"Save as" },
        Shortcut{ L'S', { .shift = true, .ctrl = true } }
    };
    // THE KEY IS STATED HERE AND ANSWERED BY THE CONTROL. WithInPlaceEdit takes F2 in its own
    // keyDown, so the press never reaches the shortcut scopes and there is no contest - the same
    // shape a text box's Delete has. It has to be answered there: the subject walk starts at the
    // focused control, and a container holding the focus answers for itself rather than for the
    // item it is on, so a tile inside a StackView is never reached from a shortcut. What the
    // shortcut here buys is the key printed on the menu line.
    Action rename{
        Text{ L"Rename" },
        Shortcut{ Keys::F2 },
        Action::OnPaintIcon{ Icons::RenameIcon::paint }
    };

    void registerAll()
    {
        // The application's scope is one object for the whole process and outlives any single
        // ApplicationBase, so a second application starting would otherwise register the same
        // set behind the first one. One registration is all there is.
        static bool registered = false;
        if (registered)
            return;
        registered = true;

        AppActions& actions = AppActions::get();
        actions.add(cut);
        actions.add(copy);
        actions.add(paste);
        actions.add(del);
        actions.add(selectAll);
        // Carries no shortcut, so nothing in the scope can ever match it. Added anyway, so the
        // scope holds the standard set whole and an application that gives it a key needs to do
        // no more than that.
        actions.add(clear);
        actions.add(undo);
        actions.add(redo);
        actions.add(save);
        actions.add(saveAs);
        actions.add(rename);
    }

}
