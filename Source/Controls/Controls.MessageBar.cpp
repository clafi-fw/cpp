module ClaFi.Controls.MessageBar;

import ClaFi.Controls.Base.MessageBoxBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.PaintIconEvent;
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
        unreachable("a message icon with no stripe of its own");
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

    void MessageBar::paintIcon(PaintIconEvent& event)
    {
        const PaintIconFunc painter = iconPainterOf(m_icon);
        if (painter)
            painter(event);
    }

    // SplitButtonBase stops short of the rule set a UiElement names - see its adjustPaint.
    void MessageBar::adjustPaint(AdjustPaintEvent& event)
    {
        SplitButtonBase::adjustPaint(event);
        event.setColorRules(UiElement::Hint);
    }

    void MessageBar::paintSurface(PaintEvent& event)
    {
        SplitButtonBase::paintSurface(event);
        if (m_icon == MessageIcon::None)
            return;

        // The band's own shape cut down to its left edge, so the stripe keeps the band's corners.
        const FloatRect bounds = event.controlBounds();
        FloatRect stripe = bounds;
        stripe.right = stripe.left + event.scaleF(k_stripeWidth);
        event.canvas().pushClip(stripe);
        event.canvas().fillPartialRoundedRectangle(
            { .bounds = bounds, .radii = event.cornerRadii() },
            event.inkRgb(stripeInk(m_icon))
        );
        event.canvas().popClip();
    }
}
