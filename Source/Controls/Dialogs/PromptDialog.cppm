module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.PromptDialog;

export import ClaFi.Controls.Base.MessageBoxBase;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.Foundation;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A question with one line of text for its answer, dropped under its control. See Controls
    export class PromptDialog : public MessageBoxBase
    {
    public:
        PromptDialog(Control& owner, std::wstring_view title, std::wstring_view value,
            MessageIcon = MessageIcon::None);
        PromptDialog(Control& owner, std::wstring_view title, std::wstring_view value,
            const PaintIconFunc& iconPainter);
    public:
        // The text the user is committing, and the sink's answer to it. See Controls
        DECLARE_EVENT(AcceptEditEvent, OnAccept, onAccept)
        /// The value that was given. Read after execute() has answered true.
        [[nodiscard]] std::wstring text() const;
        /// Drops the question under the control it was raised from and runs it. Answers whether a
        /// value was accepted; a dialog dismissed any other way answers false, which is the
        /// cancelling answer.
        bool execute();
    protected:
        void keyDown(KeyDownEvent&) override;
    private:
        // Offers the text and answers whether it was taken. A refusal does not settle anything:
        // it leaves a value still to be corrected or abandoned, which is what the window is still
        // standing for. askedBy is the control the commit came from - the answer pressed, or the
        // box a Return was pressed in - and it is what a handler's own question stands on.
        [[nodiscard]] bool offerText(Control& askedBy);
        void buildAnswers();
    private:
        bool m_accepted{ false };
    };

}
