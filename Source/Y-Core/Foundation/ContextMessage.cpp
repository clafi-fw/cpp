module ClaFi.Core.Foundation;

import :ContextMessage;
import :Control;
import :Form;
import :Hint;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ClaFi
{
    Control* ContextMessage::s_control{ nullptr };
    Text ContextMessage::s_text{};
    ContextMessage::Clock::time_point ContextMessage::s_raisedAt{};

    void ContextMessage::show(Control& about, Text text)
    {
        s_control = &about;
        s_text = std::move(text);
        s_raisedAt = Clock::now();

        // Taken down before it is put up, rather than moved onto the new words: a hint already
        // standing about this control was measured and placed from what the control says for
        // itself, and a message is a different answer in a different place. Nothing on this path
        // ends the message being raised - what ends one is the pointer moving on and the user
        // acting, and neither of those is happening inside this call.
        Hint::stopAndHide();
        about.form().hint().showRightNow(about);
    }

    bool ContextMessage::justRaised()
    {
        return s_control && Clock::now() - s_raisedAt < k_movesIgnoredFor;
    }

    bool ContextMessage::answer(const Control& about, GetHintEvent& event)
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
