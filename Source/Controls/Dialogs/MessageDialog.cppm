module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.MessageDialog;

export import ClaFi.Controls.Base.MessageBoxBase;

import ClaFi.Controls.Button;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    /// @brief Which answer was given, or nothing where the dialog was dismissed without one -
    /// Escape, or a press outside it. A caller reads nothing as its cancelling answer, which is
    /// what makes dismissing it the safe way out.
    export using DialogAnswer = std::optional<DialogButton>;

    // An answer was given, and whether it settles the question. See Controls
    export class DialogAnswerEvent : public Event
    {
    public:
        DialogAnswerEvent(DialogButton answer, Control& button)
            :
            answer{ answer },
            button{ button }
        {
        }
        const DialogButton answer;
        Control& button;
        void keepOpen() { m_settled = false; }
        [[nodiscard]] bool settled() const { return m_settled; }
    private:
        bool m_settled{ true };
    };

    // A short question and a row of answers, dropped under the control that raised it. See Controls
    export class MessageDialog : public MessageBoxBase
    {
    public:
        MessageDialog(Control& owner, std::wstring_view title, const Text& message,
            MessageIcon = MessageIcon::None);
        MessageDialog(Control& owner, std::wstring_view title, const Text& message,
            const PaintIconFunc& iconPainter);
    public:
        // An answer was given, and whether it settles the question. See Controls
        DECLARE_EVENT(DialogAnswerEvent, OnAnswer, onAnswer)
        // A link in the message was followed. See Controls#messagedialog
        DECLARE_EVENT(LinkClickEvent, OnLinkClick, onLinkClick)
        /// Adds an answer to the row. They read left to right in the order they arrive.
        Button& add(DialogButton);
        /// Drops the question under the control it was raised from and runs it. Answers once it
        /// has closed.
        DialogAnswer execute();
    private:
        DialogAnswer m_answer{};
    };

}
