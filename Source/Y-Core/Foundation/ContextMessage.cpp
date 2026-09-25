module ClaFi.Core.Foundation;

import :ContextMessage;
import :Control;
import :Form;
import :Tooltip;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ClaFi
{
    Control* ContextMessage::s_control{ nullptr };
    Text ContextMessage::s_text{};

    void ContextMessage::show(Control& about, Text text)
    {
        s_control = &about;
        s_text = std::move(text);

        // Taken down before it is put up, rather than moved onto the new words: a tooltip already
        // standing about this control was measured and placed from what the control says for
        // itself, and a message is a different answer in a different place. Nothing on this path
        // ends the message being raised - what ends one is the pointer moving on and the user
        // acting, and neither of those is happening inside this call.
        Tooltip::stopAndHide();
        about.form().tooltip().showRightNow(about);
    }

    bool ContextMessage::answer(const Control& about, GetTooltipEvent& event)
    {
        if (s_control != &about)
            return false;
        event.text << s_text;
        // Under the whole control rather than at the pointer: the pointer is not what raised
        // this, and it may be anywhere, including nowhere near the control being spoken about.
        event.placement = FormPlacement::Bottom;
        event.anchorRect = about.boundsInForm();
        return true;
    }

    void ContextMessage::forget()
    {
        s_control = nullptr;
        s_text.clear();
    }

}
