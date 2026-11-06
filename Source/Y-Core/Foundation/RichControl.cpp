module ClaFi.Core.Foundation;

import :Action;
import :Control;
import :PaintEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.StdLib;

namespace ClaFi
{
    // RichControl

    // None where the format is empty, so the cache keys an unformatted text the one way.
    const TextFormat* RichControl::textFormat() const
    {
        return m_textFormat.empty() ? nullptr : &m_textFormat;
    }

    void RichControl::setStyle(const OptionalUiElement value)
    {
        m_style = value;
        if (m_style)
            takeStyleMetrics(styleMetrics(*m_style));
    }

    void RichControl::setMetrics(const ControlMetrics& value)
    {
        m_metrics = value;
        m_statedMetrics = MetricsFields::every();
    }

    void RichControl::setPadding(const Padding value)
    {
        m_metrics.padding = value;
        m_statedMetrics.add(MetricsField::Padding);
    }

    void RichControl::setSpacing(const Spacing value)
    {
        m_metrics.spacing = value;
        m_statedMetrics.add(MetricsField::Spacing);
    }

    void RichControl::setMinSize(const MinSize value)
    {
        m_metrics.minSize = value;
        m_statedMetrics.add(MetricsField::MinSize);
    }

    void RichControl::setPreferredSize(const PreferredSize value)
    {
        m_metrics.preferredSize = value;
        m_statedMetrics.add(MetricsField::PreferredSize);
    }

    void RichControl::setPreferredWidth(const float value)
    {
        setPreferredSize({ value, m_metrics.preferredSize.y });
    }

    void RichControl::setPreferredHeight(const float value)
    {
        setPreferredSize({ m_metrics.preferredSize.x, value });
    }

    void RichControl::setMaxSize(const MaxSize value)
    {
        m_metrics.maxSize = value;
        m_statedMetrics.add(MetricsField::MaxSize);
    }

    void RichControl::setFixedWidth(const float value)
    {
        setMinSize({ value, m_metrics.minSize.y });
        setMaxSize({ value, m_metrics.maxSize.y });
    }

    void RichControl::setFixedHeight(const float value)
    {
        setMinSize({ m_metrics.minSize.x, value });
        setMaxSize({ m_metrics.maxSize.x, value });
    }

    void RichControl::setFixedSize(const FixedSize value)
    {
        setMinSize(value);
        setMaxSize(value);
    }

    void RichControl::setBorder(const Thickness value)
    {
        m_metrics.border = value;
        m_statedMetrics.add(MetricsField::Border);
    }

    void RichControl::setRadius(const float value)
    {
        m_metrics.radius = value;
        m_statedMetrics.add(MetricsField::Radius);
    }

    void RichControl::setWindowShadow(const WindowShadow& value)
    {
        m_metrics.shadow = value;
        m_statedMetrics.add(MetricsField::ShadowBlur);
        m_statedMetrics.add(MetricsField::ShadowOffset);
        m_statedMetrics.add(MetricsField::ShadowSpread);
        m_statedMetrics.add(MetricsField::ShadowOpacity);
    }

    void RichControl::adjustMetrics(AdjustMetricsEvent& event) const
    {
        event.metrics = m_metrics;
    }

    void RichControl::adjustPaint(AdjustPaintEvent& event)
    {
        if (m_style)
            event.setColorRules(*m_style);
        Control::adjustPaint(event);
    }

    void attachAction(Action& action, RichControl& control, const PresenterRole role)
    {
        action.attach(control, role);
    }

    void RichControl::adjustText(AdjustTextEvent& event) const
    {
        // Named rather than written in: a control's own text is usually the whole answer, and
        // what a gather costs is what it copies - see AdjustTextEventBase::contribute.
        event.contribute(m_text);
        Control::adjustText(event);
    }

    void RichControl::nestedAdjustTooltip(AdjustTooltipEvent& event)
    {
        event.text << m_tooltipText;
        // The base emits the event and, if nothing produced text, adds the control's own
        // text when it is trimmed.
        Control::nestedAdjustTooltip(event);
    }

    void RichControl::styleMetricsChanged()
    {
        if (m_style)
            takeStyleMetrics(styleMetrics(*m_style));
    }

    void RichControl::takeStyleMetrics(const ControlMetrics& resolved)
    {
        ControlMetrics metrics = resolved;
        metrics.copyFields(m_metrics, m_statedMetrics);
        m_metrics = metrics;
    }

}
