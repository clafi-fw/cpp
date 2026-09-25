module ClaFi.Controls.MessageDialog;

import ClaFi.Controls.Button;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // MessageDialog

    MessageDialog::MessageDialog(Control& owner, const std::wstring_view title,
        const Text& message, const MessageIcon icon)
        :
        MessageDialog{ owner, title, message, iconPainterOf(icon) }
    {
    }

    MessageDialog::MessageDialog(Control& owner, const std::wstring_view title,
        const Text& message, const PaintIconFunc& iconPainter)
        :
        MessageBoxBase{ owner, title, message, iconPainter }
    {
        // The message is what the dialog says, not what it asks the user to write. Read-only is
        // the whole of that: the box still takes the focus and still selects and copies, so the
        // text can be got out, and there is nothing in it the user can change and then be asked
        // about.
        box().setReadOnly(ReadOnly::Yes);
        // The same event, so a handler that stops it keeps the box from opening the target.
        box().onLinkClick([this](LinkClickEvent& event) {
            emitEvent(event);
        });
    }

    Button& MessageDialog::add(const DialogButton button)
    {
        return addAnswer(button, [this, button](ClickEvent& event) {
            // Offered while the dialog is still standing, so that a handler with a question of
            // its own can raise it on top of this one - owned by the button just pressed.
            DialogAnswerEvent answerEvent{ button, *event.control };
            emitEvent(answerEvent);
            if (!answerEvent.settled())
                return;

            // Recorded first: closeForm stops the event, so the other order would leave the
            // dialog answering nothing at all.
            m_answer = button;
            event.closeForm();
        });
    }

    DialogAnswer MessageDialog::execute()
    {
        runDialog();
        return m_answer;
    }

}
