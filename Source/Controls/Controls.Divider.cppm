module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Divider;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A line across whatever it stands in.
    export class Divider : public Control
    {
    public:
        template <typename... Args>
        Divider(const CreateParams&, Args&&...);
    public:
        // Room kept at the ends of the line.
        DECLARE_WRITABLE_PROPERTY(Padding, padding, setPadding, Padding{})
        // How heavy the line is.
        DECLARE_PROPERTY(Thickness, thickness, Thickness::Thin)
    public:
        void setPadding(Padding value) { m_padding = value; }
    protected:
        void adjustMetrics(AdjustMetricsEvent&) const override;
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
        ScaledDimensions calculateContent(AlignEvent&) override;
    };


    //-------------------------------------------------------------------------


    // Divider

    template<typename ...Args>
    Divider::Divider(const CreateParams& params, Args && ...args)
        :
        Control{ params, std::forward<Args>(args)...},
        INIT_PROPERTY(padding),
        INIT_PROPERTY(thickness)
    {
    }

    // The ends are round, so a heavy line reads as a bar rather than a slab.
    void Divider::adjustMetrics(AdjustMetricsEvent& event) const
    {
        event.metrics.padding = m_padding;
        event.metrics.radius = strokeWidth(m_thickness) / 2.0f;
    }

    void Divider::adjustPaint(AdjustPaintEvent& event)
    {
        event.setColorRules(UiElement::Divider);
    }

    // The padding shortens the drawn line without moving what sits either side of it, so it insets
    // the paint rather than the layout.
    void Divider::paintSurface(PaintEvent& event)
    {
        event.defaultPaintSurface(m_padding);
    }

    ScaledDimensions Divider::calculateContent(AlignEvent& event)
    {
        const float scaledW = event.scaledStrokeWidth(m_thickness);
        return { scaledW, scaledW };
    }

} // of namespace OjGui
