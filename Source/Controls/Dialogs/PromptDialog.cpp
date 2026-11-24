module ClaFi.Controls.PromptDialog;

import ClaFi.Icons.MessageIcons;

import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.Foundation;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // PromptDialog

    PromptDialog::PromptDialog(Control& owner, const std::wstring_view title,
        const std::wstring_view value, const MessageIcon icon)
        :
        PromptDialog{ owner, title, value, Icons::iconPainterOf(icon) }
    {
    }

    PromptDialog::PromptDialog(Control& owner, const std::wstring_view title,
        const std::wstring_view value, const DrawIconFunc& iconPainter)
        :
        // The value is what the user is being asked to change, so it goes in as the plain run
        // of characters it is: nothing about it is marked, and the box is about to be typed in.
        MessageBoxBase{ owner, title, Text{ value }, iconPainter },
        m_acceptButton{ buildAnswers() }
    {
        const ControlMetrics& inputMetrics = styleMetrics(UiElement::Button);
        box().setStyle(UiElement::Section);
        box().setBorder(inputMetrics.border);
        box().setRadius(inputMetrics.radius);
        box().setPadding(inputMetrics.padding);

        // The answer is the text, so the box is where this dialog opens - not on OK, which
        // addAnswer named first. It goes after the answers for that reason: it is the last word
        // on where the dialog opens.
        box().makeCurrentItem();
        // The value arrives selected, so the first character typed replaces it - a name offered
        // to be changed, rather than one to be edited from wherever the caret happened to land.
        box().selectAll();
    }

    std::wstring PromptDialog::text() const
    {
        return std::wstring{ box().text().plainText() };
    }

    bool PromptDialog::execute()
    {
        runDialog();
        return m_accepted;
    }

    void PromptDialog::nestedKeyDown(KeyDownEvent& event)
    {
        // Return commits **for the box**, and only while the box holds the focus. A key walks leaf
        // to root and the navigator's click on the focused control is the last thing to run, so
        // this root sees every Return in the dialog before the answer the user is standing on
        // does: claimed unconditionally, Return on Cancel would commit the text.
        //
        // The box hands a plain Return up rather than breaking the line - see PromptBox::charPress
        // - and keeps Shift+Return for itself.
        if (event.key == Keys::Return && !event.modifiers.shift && box().isFocused())
        {
            event.handled = true;
            if (!m_acceptButton.enabled())
            {
                return;
            }
            // No answer was pressed, so the box is what asked - which is also where the value a
            // handler may have a question about was typed.
            if (offerText(box()))
            {
                close();
                return;
            }
            box().showRefusal();
            return;
        }
        MessageBoxBase::nestedKeyDown(event);
    }

    bool PromptDialog::offerText(Control& askedBy)
    {
        AcceptEditEvent event{ askedBy, box().text() };
        emitEvent(event);
        // Recorded whichever way it went, so a reason left over from the last attempt is not the
        // one shown for this one.
        box().setRefusal(event.reason());
        if (event.refused())
            return false;

        m_accepted = true;
        return true;
    }

    Button& PromptDialog::buildAnswers()
    {
        Button& accept = addAnswer(DialogAnswer::Ok, [this](ClickEvent& event) {
            // The answer that was pressed is what asked, so a handler with a question of its own
            // hangs it there - on top of this dialog, at the button the user just pressed.
            if (offerText(*event.control))
            {
                event.closeForm();
                return;
            }
            box().showRefusal();
        });
        addAnswer(DialogAnswer::Cancel, [](ClickEvent& event) {
            event.closeForm();
        });
        return accept;
    }

}
