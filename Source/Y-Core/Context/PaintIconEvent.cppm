export module ClaFi.Core.Context.PaintIconEvent;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;

namespace ClaFi
{
    // An icon being painted, from a control's paint or from a text run. See Context
    export class PaintIconEvent : public ControlEventBase
    {
    public:
        PaintIconEvent(ControlPaintContext&, const FloatRect&, Tag, float disabledAmount);
        PaintIconEvent(PaintIconEvent&);
        // Whatever was attached to this icon, which is not the same thing on both routes. A
        // control's paint passes the control's own tag; a text run passes the one the InTextIcon
        // was written with, which the text engine never looks inside. Naming it after the control
        // would be true of one caller and a guess about the other.
        Tag tag() const { return m_tag; }
        const FloatRect& iconRect() const { return m_iconRect; }
        float iconWidth() const { return m_iconRect.width(); }
        FloatPoint iconCenter() const { return m_iconRect.center(); }
        [[nodiscard]] Lightness lightness() const { return m_controlContext.lightness; }
        [[nodiscard]] Color surfaceRgb() const { return m_controlContext.surfaceRgb(); }
        ControlPaintContext& controlContext() { return m_controlContext; }
        const ControlPaintContext& controlContext() const { return m_controlContext; }
        [[nodiscard]] Color textRgb(InkGrade grade) const { return inkColor(InkWell::textInk(grade)); }
        [[nodiscard]] Color accentRgb(InkGrade grade) const { return inkColor(InkWell::accentInk(grade)); }
        [[nodiscard]] Color spotRgb(InkGrade grade) const { return inkColor(InkWell::spotInk(grade)); }
        [[nodiscard]] Color inkColor(const Ink& ink) const { return m_controlContext.inkRgb(ink); }
        [[nodiscard]] Color inkColor(Pigment pigment, InkTone tone) const { return m_controlContext.inkRgb(pigment, tone); }
        [[nodiscard]] Color inkColor(Pigment pigment, float saturation, float elevation) const { return m_controlContext.inkRgb(pigment, saturation, elevation); }
        float disabledAmount() const { return m_disabledAmount; }
        void applyDisabledFactorTo(Color& color) const { color.blend(m_controlContext.surface, m_disabledAmount); }
        [[nodiscard]] Color applyDisabledFactor(Color color) const { applyDisabledFactorTo(color); return color; }
    private:
        FloatRect m_iconRect;
        ControlPaintContext m_controlContext;
        Tag m_tag;
        float m_disabledAmount;
    };

    //-------------------------------------------------------------------------

    // PaintIconEvent

    PaintIconEvent::PaintIconEvent(ControlPaintContext& controlContext, const FloatRect& iconRect, Tag tag, float disabledAmount)
        :
        ControlEventBase{ controlContext.formContext() },
        m_iconRect{ iconRect },
        m_controlContext{ controlContext },
        m_tag{ tag },
        m_disabledAmount{ disabledAmount }
    {
    }

    PaintIconEvent::PaintIconEvent(PaintIconEvent& other)
        :
        ControlEventBase{ other.formContext() },
        m_iconRect{ other.m_iconRect },
        m_controlContext{ other.m_controlContext },
        m_tag{ other.m_tag },
        m_disabledAmount{ other.m_disabledAmount }
    {
    }

}
