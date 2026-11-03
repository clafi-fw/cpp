module ClaFi.Controls.MessageBar;

import ClaFi.Icons.MessageIcons;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // MessageBar

    void MessageBar::setMessage(const MessageIcon icon, const Text& message)
    {
        m_icon = icon;
        const ButtonViewMode mode = icon == MessageIcon::None
            ? ButtonViewMode::TextLabel
            : ButtonViewMode::LeftIcon;
        setViewMode(mode);
        text() = message;
        invalidateFormAlign();
    }

    // **The answers are laid out in the box the band placed them in.** SplitButtonBase places its
    // part without aligning it, and this part is a stack holding a button per answer.
    void MessageBar::alignContent(AlignEvent& event, const ScaledPosition position,
        ScaledSize& dimensions)
    {
        SplitButtonBase::alignContent(event, position, dimensions);
        if (shownSecondaryPart())
            alignControl(&m_answers, event, m_answers.topLeft(), m_answers.scaledSize());
    }

    void MessageBar::drawIcon(DrawIconEvent& event)
    {
        const DrawIconFunc painter = Icons::iconPainterOf(m_icon);
        if (painter)
            painter(event);
    }

    // SplitButtonBase stops short of the rule set a UiElement names - see its adjustPaint.
    void MessageBar::adjustPaint(AdjustPaintEvent& event)
    {
        SplitButtonBase::adjustPaint(event);
        event.setColorRules(calloutElementOf(m_icon));
    }

    // The band runs down the left edge, as a callout's does.
    void MessageBar::paintFrame(PaintEvent& event)
    {
        SplitButtonBase::paintFrame(event);
        event.paintBand(RectSide::Left, event.bandRgb());
    }
}
