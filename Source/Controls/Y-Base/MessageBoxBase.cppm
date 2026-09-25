export module ClaFi.Controls.Base.MessageBoxBase;

import ClaFi.Controls.Base.PanelBase;

import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    /// @brief The painter a named icon stands for, and nothing for None. A dialog needing a
    /// picture this set does not carry hands its own painter in place of a name.
    export [[nodiscard]] PaintIconFunc iconPainterOf(MessageIcon);

    /// @brief A line led by one of these icons, drawn at the size of the words beside it. It is
    /// what a context message raised about a control is written as - the icon says what kind of
    /// news it is before the words are read, the way a dialog's picture does.
    /// @note The icon and the gap after it are stated in one place, so two messages raised from
    /// opposite ends of an application never lead differently. MessageIcon::None leads with
    /// nothing and answers the words alone.
    export [[nodiscard]] Text messageText(MessageIcon, std::wstring_view);

    /// @brief The word an answer is offered under. Stated once, here, so two dialogs asking the
    /// same question never word it differently.
    export [[nodiscard]] std::wstring_view captionOf(DialogButton);

    /// @brief What a dialog's text stands in: a text box inside a scroll box. One line of text or
    /// fifty, the window comes out the same width, and past a height the text scrolls instead of
    /// taking the screen.
    export using MessageBoxBody = ScrollBoxWith<EditBox>;

    using MessageBoxForm = Form<PanelBase>;

    // The shell every dialog of this family is - title, middle, answers. See Controls-Base
    export class MessageBoxBase : public MessageBoxForm
    {
    public:
        MessageBoxBase(Control& owner, std::wstring_view title, const Text& text, MessageIcon);
        MessageBoxBase(Control& owner, std::wstring_view title, const Text& text,
            const PaintIconFunc& iconPainter);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"MessageBox"; }
    protected:
        // The text box itself - what a dialog reaches for to say anything about the text in it.
        [[nodiscard]] EditBox& box() { return m_box.body(); }
        [[nodiscard]] const EditBox& box() const { return m_box.body(); }
        // The scroll box around it, which is what a dialog reaches for to say anything about the
        // field rather than about the text: a message stands on the middle's own surface, an
        // answer is drawn as something to write in.
        [[nodiscard]] MessageBoxBody& field() { return m_box; }
        // Adds an answer to the row. They read left to right in the order they arrive.
        //
        // THE FIRST ANSWER IS WHAT THE DIALOG OPENS ON. Return and Space run the focused
        // control, so a dialog opening anywhere else is a dialog only the mouse can answer: the
        // text takes the focus otherwise, being the first focusable thing in slot order, and
        // pressing Return on it does nothing. A dialog whose answer is the text it holds says so
        // by focusing its own box after its answers are in.
        template <typename Handler>
        Button& addAnswer(DialogButton button, Handler&& onClick)
        {
            const bool isFirst = m_answers.controls().empty();
            Button& result = m_answers.add<Button>(
                captionOf(button),
                // The word sits in the middle of the answer. The row divides itself evenly, so an
                // answer is wider than its own caption and a Left anchored one reads as a word
                // resting against the edge of a box rather than as the answer that box is. Both
                // axes, so a caption that takes two lines stays centred in a row the other
                // answers have been stretched to.
                VerticalTextAnchor::Center,
                HorizontalTextAnchor::Center,
                OnEvent{ std::forward<Handler>(onClick) }
            );
            if (isFirst)
                result.setFocus();
            return result;
        }
        // Drops the question under the control it was raised from and runs it. Returns once the
        // dialog has closed, leaving the derived class to say what the answer was.
        void runDialog();
    private:
        Control& m_title;
        Panel& m_content;
        Label& m_icon;
        MessageBoxBody& m_box;
        Panel& m_answerBar;
        StackPanel& m_answers;
    };

}
