module ClaFi.Controls.MessageBar;

import ClaFi.Icons.MessageIcons;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // The pigment each message icon fills its badge with - see Icons::MessageBadge.
    static Ink stripeInk(const MessageIcon icon)
    {
        switch (icon)
        {
            case MessageIcon::Warning:
                return InkWell::Yellow;
            case MessageIcon::Error:
                return InkWell::Red;
            case MessageIcon::Question:
            case MessageIcon::Information:
                return InkWell::Blue;
            case MessageIcon::Ok:
                return InkWell::Green;
            case MessageIcon::None:
                break;
        }
        noReach("a message icon with no stripe of its own");
    }

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
        event.setColorRules(k_colorRules);
    }

    void MessageBar::paintFrame(PaintEvent& event)
    {
        SplitButtonBase::paintFrame(event);
        if (m_icon == MessageIcon::None)
            return;

        event.canvas().fillEdgeIndicator(
            { .bounds = event.controlBounds(), .radii = event.cornerRadii() },
            RectSide::Left,
            event.scaleF(k_stripeWidth),
            event.inkRgb(stripeInk(m_icon))
        );
    }
}
