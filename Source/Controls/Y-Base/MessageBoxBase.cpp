module ClaFi.Controls.Base.MessageBoxBase;

import ClaFi.Icons.ErrorIcon;
import ClaFi.Icons.InformationIcon;
import ClaFi.Icons.OkIcon;
import ClaFi.Icons.QuestionIcon;
import ClaFi.Icons.WarningIcon;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.Foundation;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // What a bar keeps clear around its own content. The two bars carry the same numbers as each
    // other so that the title and the answers sit the same distance in from the frame.
    constexpr float k_barPaddingX = 12.0f;
    constexpr float k_barPaddingY = 8.0f;

    // The middle is the one part with two things side by side, so it states a gap as well as a
    // padding: the icon stands off the text by the same distance the text stands off the frame.
    constexpr float k_contentPadding = 12.0f;
    constexpr float k_contentSpacing = 12.0f;

    constexpr float k_answerSpacing = 8.0f;

    // Larger than the sixteen point icon a button carries. This one is not a mark on a command,
    // it is what the dialog is about, and it is read before the text beside it.
    constexpr float k_iconSize = 32.0f;

    // The same picture inside a run of text is the size of the text. A message is one line under
    // a control, and a badge taller than the line it stands in reads as a thing of its own beside
    // the words rather than as the first thing in them.
    constexpr float k_messageIconSize = 14.0f;

    // What stands between that picture and the first word.
    constexpr float k_messageIconSpacing = 5.0f;

    // Wide enough that a short question does not come up as a sliver, and the width a long one
    // wraps against. The maximum is what keeps a paragraph from taking the window sideways: the
    // text box is told how wide it may be, so the text turns instead of the window growing.
    constexpr float k_minTextWidth = 260.0f;
    constexpr float k_maxTextWidth = 360.0f;

    // How tall the text may make the window. Past this the scroll box scrolls, which is the whole
    // reason the text stands in one - a dialog is a question, and a question that fills the
    // screen is no longer being read.
    constexpr float k_maxTextHeight = 240.0f;


    //-----------------------------------------------------------------------------


    PaintIconFunc iconPainterOf(const MessageIcon value)
    {
        switch (value)
        {
        case MessageIcon::None:
            return {};
        case MessageIcon::Warning:
            return Icons::WarningIcon::paint;
        case MessageIcon::Error:
            return Icons::ErrorIcon::paint;
        case MessageIcon::Question:
            return Icons::QuestionIcon::paint;
        case MessageIcon::Information:
            return Icons::InformationIcon::paint;
        case MessageIcon::Ok:
            return Icons::OkIcon::paint;
        }
        unreachable("a message icon with no painter of its own");
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

    std::wstring_view captionOf(const DialogButton value)
    {
        switch (value)
        {
        case DialogButton::Ok:
            return L"OK";
        case DialogButton::Cancel:
            return L"Cancel";
        case DialogButton::Yes:
            return L"Yes";
        case DialogButton::No:
            return L"No";
        case DialogButton::Save:
            return L"Save";
        case DialogButton::Discard:
            return L"Don't save";
        }
        unreachable("a dialog button with no caption of its own");
    }


    //-----------------------------------------------------------------------------


    // MessageBoxBase

    MessageBoxBase::MessageBoxBase(Control& owner, const std::wstring_view title,
        const Text& text, const MessageIcon icon)
        :
        MessageBoxBase{ owner, title, text, iconPainterOf(icon) }
    {
    }

    MessageBoxBase::MessageBoxBase(Control& owner, const std::wstring_view title,
        const Text& text, const PaintIconFunc& iconPainter)
        :
        MessageBoxForm{
            owner.appContext(),
            WindowRole::Menu, // a Dialog does not act as an active popup
            &owner,
            owner.themeMetrics().secondaryWindow,
            owner.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            // Clicked and hovered, never focused. The text and the answers carry the focus
            // themselves and the frame around them is not a focus scope of its own - the same
            // division a menu draws.
            Interactivity::MouseOnly,
            // The window keeps of itself exactly the border it draws, which is what the bars run
            // up to. A stroke lies inside the bounds it is drawn on, so a bar laid over that last
            // point would paint the frame out.
            Padding{ strokeWidth(owner.themeMetrics().secondaryWindow.border) },
            // The bars meet. A gap between them would be the window's colour showing through in
            // two lines across a face that is meant to read as three bands of one thing.
            Spacing{ 0.0f }
        },
        // The title is the header of this window the way a section header is the header of a
        // section: the same colours, and the text style that names a section.
        m_title{ createTopBar<Label>(
            UiElement::Header,
            Radius{ 0.0f },
            Padding{ k_barPaddingX, k_barPaddingY },
            Text{ title }
        ) },
        m_content{ createBody<Panel>(
            UiElement::Page,
            Radius{ 0.0f },
            Padding{ k_contentPadding },
            Spacing{ k_contentSpacing }
        ) },
        m_icon{ m_content.createLeftBar<Label>(
            ButtonViewMode::IconOnly,
            IconSize{ k_iconSize },
            // Level with the first line of the text rather than with the middle of a paragraph
            // whose length the icon says nothing about.
            VerticalAlign::Top
        ) },
        m_box{ m_content.createBody<MessageBoxBody>(
            HostProps{
                ScrollBars::Vertical,
                MaxSize{ k_maxFloat, k_maxTextHeight }
            },
            BodyProps{
                Interactivity::Focusable,
                MinSize{ k_minTextWidth, 0.0f },
                MaxSize{ k_maxTextWidth, k_maxFloat },
                text
            }
        ) },
        m_answerBar{ createBottomBar<Panel>(
            UiElement::Bar,
            Radius{ 0.0f },
            Padding{ k_barPaddingX, k_barPaddingY }
        ) },
        m_answers{ m_answerBar.createBody<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_answerSpacing },
            // The answers divide the strip between them, so the row reads as one band of answers
            // rather than as buttons of three widths gathered at one end. Every share is as wide
            // as the widest answer at least, and the strip grows with the question above it, so
            // nothing is squeezed by a word.
            ItemSizing::Equal
        ) }
    {
        // The window is whatever the question and its answers came out as. A dialog is never laid
        // out into a size it was given: it has no size until it has been filled.
        setAutoFit(AutoFit::Yes);

        if (iconPainter)
        {
            m_icon.onPaintIcon(iconPainter);
            return;
        }
        // Nothing to draw. The slot goes with the picture: PanelBase skips a bar that is not
        // visible and skips the gap beside it too, so the text starts where the icon would have.
        m_icon.setVisible(false);
    }

    void MessageBoxBase::runDialog()
    {
        // Under the control the question was raised from, so the answer appears where the user is
        // looking. The target is tested rather than assumed: FormBase accepts a popup with no
        // target at all, and a dialog raised over a control that went away while it was being
        // filled is that.
        if (const Control* owner = popupTarget())
        {
            setDropdownClearance(1.0f);
            setPlacement(FormPlacement::Bottom, owner->boundsInForm());
        }
        MessageBoxForm::execute();
    }

}
