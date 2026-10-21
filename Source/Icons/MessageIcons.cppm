export module ClaFi.Icons.MessageIcons;

import ClaFi.Icons.ErrorIcon;
import ClaFi.Icons.InformationIcon;
import ClaFi.Icons.OkIcon;
import ClaFi.Icons.QuestionIcon;
import ClaFi.Icons.WarningIcon;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Icons
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


    //-------------------------------------------------------------------------


    // The picture inside a run of text is the size of the text. A message is one line under a
    // control, and a badge taller than the line it stands in reads as a thing of its own beside
    // the words rather than as the first thing in them.
    constexpr float k_messageIconSize = 14.0f;

    // What stands between that picture and the first word.
    constexpr float k_messageIconSpacing = 5.0f;

    PaintIconFunc iconPainterOf(const MessageIcon value)
    {
        switch (value)
        {
        case MessageIcon::None:
            return {};
        case MessageIcon::Warning:
            return WarningIcon::paint;
        case MessageIcon::Error:
            return ErrorIcon::paint;
        case MessageIcon::Question:
            return QuestionIcon::paint;
        case MessageIcon::Information:
            return InformationIcon::paint;
        case MessageIcon::Ok:
            return OkIcon::paint;
        }
        noReach("a message icon with no painter of its own");
    }

    Text messageText(const MessageIcon icon, const std::wstring_view text)
    {
        Text result{};
        if (icon != MessageIcon::None)
        {
            result << InTextIcon{ k_messageIconSize, iconPainterOf(icon) };
            result << Space{ k_messageIconSpacing };
        }
        result << text;
        return result;
    }
}
